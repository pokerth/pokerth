/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2025 Felix Hammer, Florian Thauer, Lothar May          *
 *****************************************************************************/

#include "gamehandler.h"
#include "chattranslator.h"
#include "chatemotes.h"
#include "gui/chat_emote_shortcuts.h"
#include <session.h>
#include <game.h>
#include <handinterface.h>
#include <playerinterface.h>
#include <boardinterface.h>
#include <berointerface.h>
#include <cardsvalue.h>
#include <playerdata.h>
#include <game_defs.h>
#include <gamedata.h>
#include <configfile.h>
#include <soundevents.h>
#include <net/socket_msg.h>
#include <QString>
#include <QTimer>
#include <QDebug>
#include <QUrl>
#include <QFileInfo>
#include <QDateTime>
#include <QRegularExpression>
#include "chatcolors.h"
#include "styleprovider.h"

#include <QCoreApplication>
#include <QEvent>
#include <algorithm>
#include <list>

namespace
{
// Card code (0-51) → a short form with a Unicode suit symbol, e.g. "K♥". Identical to
// QmlGuiInterface::fmtCard, so that showdown cards look exactly like the board cards in the
// game history.  0-12 diamonds(♦), 13-25 hearts(♥), 26-38 spades(♠), 39-51 clubs(♣).
QString logCard(int code)
{
	if (code < 0 || code > 51)
		return QStringLiteral("?");
	static const char *ranks[] = {"2","3","4","5","6","7","8","9","10","J","Q","K","A"};
	static const QChar suits[] = { QChar(0x2666), QChar(0x2665), QChar(0x2660), QChar(0x2663) };
	return QString::fromLatin1(ranks[code % 13]) + QString(suits[code / 13]);
}

// Colourise a game history line as HTML. Instead of fixed hex values the line
// contains colour ROLES (chatcolors.h); the concrete colour is only delivered by the
// table theme when it is delivered. The roles as in the Qt widgets client: normal =
// the main text, the winner of the main pot / a side pot in gold, sit-out and the board in the
// signal colour.
QString formatLogLine(const QString &text, int type)
{
	// chatEscape instead of toHtmlEscaped: it additionally removes the control characters of the
	// colour placeholders, so that no player name can smuggle in a colour.
	const QString esc = ChatColors::chatEscape(text);
	auto span = [&esc](TableChatColors::Role role, const QString &extraStyle = QString()) {
		return QStringLiteral("<span style=\"") + TableChatColors::colorStyle(role)
			   + QStringLiteral(";") + extraStyle + QStringLiteral("\">") + esc
			   + QStringLiteral("</span>");
	};
	switch (type) {
	case GameHandler::LogHeader:
		return span(TableChatColors::Text, QStringLiteral(" font-weight:bold;"));
	case GameHandler::LogWinnerMain:
		return span(TableChatColors::Winner);
	case GameHandler::LogWinnerSide:
		return span(TableChatColors::WinnerSide);
	case GameHandler::LogSitOut:
		return QStringLiteral("<i>") + span(TableChatColors::Board) + QStringLiteral("</i>");
	case GameHandler::LogBoard:
		return span(TableChatColors::Board);
	case GameHandler::LogGameWin:
		return QStringLiteral("<b><i>") + span(TableChatColors::Text) + QStringLiteral("</i></b>");
	default:
		return span(TableChatColors::Text);
	}
}

// Avatar path → a QML image source. getMyAvatar() delivers (as in the widgets client)
// a local file path; if the file exists, return it as a file:// URL.
QString resolveAvatarSource(const std::string &raw)
{
	if (raw.empty())
		return QString();
	const QString path = QString::fromStdString(raw);
	if (!QFileInfo::exists(path))
		return QString();
	return QUrl::fromLocalFile(path).toString();
}
} // namespace

GameHandler::GameHandler(QObject *parent)
	: QObject(parent), m_phaseText("Preflop")
{
	// Initialize empty player list (10 seats)
	for (int i = 0; i < 10; ++i) {
		QVariantMap p;
		p["name"]    = QString("");
		p["stack"]   = 0;
		p["bet"]     = 0;
		p["active"]  = false;
		p["myTurn"]  = false;
		p["seatId"]  = i;
		p["button"]  = 0;
		p["action"]  = 0;
		p["card0"]   = -1;
		p["card1"]   = -1;
		p["fade0"]   = false;
		p["fade1"]   = false;
		p["playerId"] = 0;
		p["countryCode"] = QString("");
		p["isGuest"] = false;
		p["reserved"] = false;
		m_players.append(p);
	}
	// Initialize empty board cards (5 slots, -1 = not dealt)
	for (int i = 0; i < 5; ++i)
		m_boardCards.append(-1);

	m_timeoutBeepTimer = new QTimer(this);
	m_timeoutBeepTimer->setSingleShot(true);
	connect(m_timeoutBeepTimer, &QTimer::timeout, this, [this]() {
		playYourTurnTimeoutSound();
	});

	// The chat translator operates directly on m_chatLog; every line it changes
	// triggers (in a bundle) chatLogChanged(), so that the QML binding renders anew.
	m_chatTranslator = new ChatTranslator(&m_chatLog, this);
	connect(m_chatTranslator, &ChatTranslator::chatLogMutated,
			this, &GameHandler::notifyChatLogChanged);

	// The AFK reset: real user activity (mouse/keyboard) holds the server side
	// inactivity timeout back. IMPORTANT: game actions (fold/call/raise) do NOT
	// count as activity on the server side (Type_MyActionRequestMessage is
	// excluded from IsClientActivity, so that auto check/fold does not undermine the AFK
	// timeout) – only a Type_ResetTimeoutMessage resets the in-game
	// timer (21 min). Without this the QML client was kicked by the server after ~21 min
	// despite active play (like the widgets client via an
	// eventFilter). An app-wide filter, rate limited – which events count as
	// activity is defined at isUserActivityEvent().
	m_afkResetTimer.start();
	if (qApp)
		qApp->installEventFilter(this);
}

// Does an event count as real user activity (→ reset the AFK timer of the
// server)?
//
// IMPORTANT – shortcuts: in a pure QGuiApplication (QML)
// QGuiApplicationPrivate::processKeyEvent checks the key press AGAINST THE
// QShortcutMap BEFORE the event is delivered to the window, and returns
// early on a hit. A key press occupied by a QML shortcut
// (F1–F8, Alt+…) thus never runs through QCoreApplication::notify –
// and therefore not through this app-wide filter either. Only the QEvent::Shortcut delivered
// to the shortcut object afterwards becomes visible. Without this branch a
// player who acts exclusively via the F keys (fold/call/raise) stayed
// "inactive" for the server for 21 min and got the timeout warning, although they
// played continuously. The widget client is not affected by that: there
// QApplication::notify resolves the shortcuts only AFTER the app filters.
//
// Touch: QQuickWindow gets real touch events; the mouse synthesis
// only happens inside the item delivery, not via notify. Without
// TouchBegin/TouchUpdate there would be no activity at all on Android/iOS.
static bool isUserActivityEvent(QEvent::Type t)
{
	switch (t) {
	case QEvent::MouseButtonPress:
	case QEvent::KeyPress:
	case QEvent::Shortcut:
	case QEvent::ShortcutOverride:
	case QEvent::Wheel:
	case QEvent::TouchBegin:
	case QEvent::TouchUpdate:
		return true;
	default:
		return false;
	}
}

bool GameHandler::eventFilter(QObject *watched, QEvent *event)
{
	if (isUserActivityEvent(event->type())) {
		if (m_afkResetTimer.elapsed() >= kAfkResetIntervalMs
				&& m_session && m_session->isNetworkClientRunning()) {
			qDebug() << "[AFK] ResetTimeout gesendet, ausgelöst von Event-Typ"
					 << event->type();
			m_session->resetNetworkTimeout();
			m_afkResetTimer.restart();
		}
	}
	return QObject::eventFilter(watched, event);
}

GameHandler::~GameHandler() = default;

void GameHandler::setConfig(ConfigFile *config)
{
	m_config = config;
	if (m_chatTranslator)
		m_chatTranslator->setConfig(config);
}

void GameHandler::setSoundEvents(SoundEvents *soundEvents)
{
	m_soundEventHandler = soundEvents;
}

void GameHandler::setStyleProvider(StyleProvider *styleProvider)
{
	m_styleProvider = styleProvider;
	if (!styleProvider)
		return;
	// A style change during a running game: adopt the palette and recolour the
	// history/chat that is already there (the lines themselves stay raw).
	connect(styleProvider, &StyleProvider::changed,
			this, &GameHandler::refreshTableChatPalette);
	refreshTableChatPalette();
}

void GameHandler::refreshTableChatPalette()
{
	if (!m_styleProvider)
		return;
	TableChatColors::Palette palette;
	palette.color[TableChatColors::Text]       = m_styleProvider->chatLogText();
	palette.color[TableChatColors::Accent]     = m_styleProvider->chatLogAccent();
	palette.color[TableChatColors::Winner]     = m_styleProvider->chatLogWinner();
	palette.color[TableChatColors::WinnerSide] = m_styleProvider->chatLogWinnerSide();
	palette.color[TableChatColors::Board]      = m_styleProvider->chatLogBoard();
	m_tableChatPalette = palette;
	m_gameLogModel.setPalette(palette);
	// chatLog() delivers the new colours from now on – the boxes rebuild their
	// rich text document.
	emit chatLogChanged();
}

QObject* GameHandler::chatTranslator() const
{
	return m_chatTranslator;
}

void GameHandler::setSession(boost::shared_ptr<Session> session)
{
	m_session = session;
}

void GameHandler::setGame(boost::shared_ptr<Game> game)
{
	if (m_soundEventHandler)
		m_soundEventHandler->newGameStarts();

	m_localGameExitRequested = false;
	m_localGameOver = false;
	m_game = game;
	// Adopt the spectator mode of the network client (always false in a local
	// game). It is already settled when the engine reports the game: the
	// JoinGameAck came before the game start.
	const bool spectating = m_session && m_session->isNetworkClientRunning()
							&& m_session->isClientSpectating();
	if (spectating != m_spectating) {
		m_spectating = spectating;
		emit spectatingChanged();
	}
	m_leftPlayers.clear();
	// Discard pending busted player timers from the previous game.
	qDeleteAll(m_bustedLocalTimers);
	m_bustedLocalTimers.clear();
	// Reset state for new game
	m_pot = 0;
	m_gameId = m_game ? m_game->getMyGameID() : 0;
	m_phaseText = "Preflop";
	m_handNumber = 0;
	m_myTurn = false;
	m_myTurnWindowClosed = true;
	m_engineTurnPointedAtMe = false;
	m_awaitingMyAction = false;
	m_callAmount = 0;
	m_minRaiseAmount = 0;
	m_maxRaiseAmount = 0;
	m_boardCardCount = 0;
	m_boardCards = QVariantList{-1, -1, -1, -1, -1};
	m_winnerSeatIds.clear();
	m_winningHandText.clear();
	m_holeFade0.clear();
	m_holeFade1.clear();
	m_boardCardFade = QVariantList{false, false, false, false, false};
	setShowdownActive(false);
	m_gameLogModel.clear();
	m_chatLog.clear();
	notifyChatLogChanged();
	for (int i = 0; i < 10; ++i) {
		m_lastSeenAction[i] = 0;
		m_actionToken[i] = -1;
	}

	// Re-build player list (seats may differ between games)
	refreshPlayerData();

	emit potChanged();
	emit gameIdChanged();
	emit phaseTextChanged();
	emit handNumberChanged();
	emit myTurnChanged();
	emit awaitingMyActionChanged();
	emit callAmountChanged();
	emit minRaiseAmountChanged();
	emit maxRaiseAmountChanged();
	emit boardCardCountChanged();
	emit boardCardsChanged();
	emit winnerSeatIdsChanged();
	emit winningHandTextChanged();
	emit boardCardFadeChanged();
}

QString GameHandler::tableStatsUrl() const
{
	if (!m_game || !m_session)
		return QString();

	// The table name from the server game info (as in refreshSpectators): the GameHandler
	// does not hold the name itself, but it does hold the current game ID of the session.
	const unsigned gameId = m_session->getClientCurrentGameId();
	if (gameId == 0)
		return QString();
	const GameInfo info = m_session->getClientGameInfo(gameId);

	QString nickList;
	const QStringList nicks = tableStatsNicks();
	for (int i = 0; i < nicks.size(); ++i) {
		nickList += QString("&nick%1=").arg(i + 1);
		nickList += QString::fromUtf8(QUrl::toPercentEncoding(nicks.at(i)));
	}

	return QStringLiteral("https://www.pokerth.net/redirect_user_profile.php?tableview=1")
		   + nickList
		   + QStringLiteral("&table=")
		   + QString::fromUtf8(QUrl::toPercentEncoding(QString::fromStdString(info.name)));
}

QStringList GameHandler::tableStatsNicks() const
{
	QStringList nicks;
	if (!m_game || !m_session || m_session->getClientCurrentGameId() == 0)
		return nicks;

	// The nick list 1:1 like the Qt widgets client (MyNameLabel): active players only.
	// In addition – as when building m_players – skip players who have already dropped
	// out (m_leftPlayers), so that the list corresponds exactly to the visible
	// player boxes and follows along when somebody leaves.
	PlayerList seats = m_game->getSeatsList();
	for (auto it = seats->begin(); it != seats->end(); ++it) {
		if (m_leftPlayers.contains((*it)->getMyUniqueID()))
			continue;
		if (!(*it)->getMyActiveStatus())
			continue;
		nicks << QString::fromStdString((*it)->getMyName());
	}
	return nicks;
}

// ─── private helpers ────────────────────────────────────────────────────────

void GameHandler::playYourTurnTimeoutSound()
{
	if (m_soundEventHandler)
		m_soundEventHandler->playSound("yourturn", 0);
}

void GameHandler::appendGameLog(const QString &message, int type)
{
	if (message.isEmpty()) return;
	// Incremental appending + the limiting in the model (no full reset of the view).
	const int kMaxLines = 400;
	m_gameLogModel.append(formatLogLine(message, type), kMaxLines);
}

void GameHandler::appendChat(const QString &playerName, const QString &message)
{
	if (message.isEmpty()) return;

	// Discard messages of ignored players (before the reaction handling, so that
	// their emoji reactions stay silent as well). Read the list freshly on every
	// message as in the lobby chat (the chattools.cpp pattern, which applies to
	// the game chat there as well).
	if (m_config) {
		const std::list<std::string> ignoreList = m_config->readConfigStringList("PlayerIgnoreList");
		for (const auto &entry : ignoreList) {
			if (playerName == QString::fromUtf8(entry.c_str()))
				return;
		}
	}

	// Emoji reactions (the convention of the web client): "/emoji 🎉" or the legacy
	// "[R]🎉". Do not take them into the chat history but play them as a reaction
	// animation at the seat of the sender. Check on the trimmed text
	// (clients may append whitespace); the length limit is more generous
	// than in the web client (22 instead of 18), so that ZWJ/variation selector
	// sequences pass as well – normal messages still do not match.
	const QString trimmedMsg = message.trimmed();
	bool isReactionMsg = false;
	QString reactionEmoji;
	if (trimmedMsg.startsWith(QStringLiteral("/emoji ")) && trimmedMsg.size() < 22) {
		isReactionMsg = true;
		reactionEmoji = trimmedMsg.mid(7).trimmed();
	} else if (trimmedMsg.startsWith(QStringLiteral("[R]")) && trimmedMsg.size() < 14) {
		isReactionMsg = true;
		reactionEmoji = trimmedMsg.mid(3).trimmed();
	}
	if (isReactionMsg) {
		// Reaction messages never appear in the chat history. Only play real
		// emojis – text disguised as a reaction is discarded.
		if (isEmojiOnlyReaction(reactionEmoji)) {
			qDebug() << "[REACT] incoming reaction from" << playerName << ":" << reactionEmoji;
			emit reactionReceived(playerName, reactionEmoji);
		} else {
			qDebug() << "[REACT] discarding non-emoji reaction from" << playerName << ":" << reactionEmoji;
		}
		return;
	}

	// The formatting analogous to the lobby chat: a /me action, emojis, a mention.
	const QString myNick = m_config ? QString::fromStdString(m_config->readConfigString("MyName")) : QString();
	const bool isAction = message.startsWith(QStringLiteral("/me "));
	const QString rawDisplay = isAction ? message.mid(4) : message;

	// chatEscape: besides the HTML escaping it removes the control characters of the
	// colour placeholders as well – otherwise a message could set its own colour.
	QString escapedMsg = ChatColors::chatEscape(rawDisplay);
	// Convert the ASCII shortcuts (":-)", "8-)", "<3", …) on the raw text before
	// link/style markup is added – that way short shortcuts never collide with
	// our own HTML (e.g. "color:#...").
	escapedMsg = applyChatEmoteShortcuts(escapedMsg);
	static const QRegularExpression urlRe(QStringLiteral("(https?://\\S+)"));
	escapedMsg.replace(urlRe, QStringLiteral("<a href=\"\\1\">\\1</a>"));

	const bool isMention = !myNick.isEmpty() && rawDisplay.contains(myNick, Qt::CaseInsensitive);
	// A colour role instead of a hex value – the colour comes from the table theme (chatcolors.h).
	const TableChatColors::Role role =
		isMention ? TableChatColors::Accent : TableChatColors::Text;
	QString styledMsg = QStringLiteral("<span style=\"") + TableChatColors::colorStyle(role)
						+ (isMention ? QStringLiteral("; font-weight:bold") : QString())
						+ QStringLiteral(";\">") + escapedMsg + QStringLiteral("</span>");
	styledMsg = enlargeEmojis(styledMsg);

	const QString tsPrefix = chatTimestampPrefix(m_config);
	const QString name = ChatColors::chatEscape(playerName);
	QString line;
	if (isAction)
		line = tsPrefix + QStringLiteral("<i>* ") + name + QStringLiteral(" ") + styledMsg + QStringLiteral(" *</i>");
	else
		line = tsPrefix + QStringLiteral("<b>") + name + QStringLiteral(":</b> ") + styledMsg;

	// The translate symbol only on messages of others (rawDisplay = the source text without
	// HTML/style markup; styledMsg = the message body, which is replaced by the
	// translation when it is shown).
	if (m_chatTranslator && playerName != myNick)
		line = m_chatTranslator->decorate(line, rawDisplay, styledMsg);

	m_chatLog.append(line);
	const int kMaxLines = 400;
	if (m_chatLog.size() > kMaxLines)
		m_chatLog.erase(m_chatLog.begin(), m_chatLog.begin() + (m_chatLog.size() - kMaxLines));
	notifyChatLogChanged();
}

void GameHandler::notifyChatLogChanged()
{
	// See gamehandler.h: several changes of one event loop pass
	// result in a single rebuild of the chat document.
	if (m_chatLogNotifyPending)
		return;
	m_chatLogNotifyPending = true;
	QMetaObject::invokeMethod(this, [this]() {
		m_chatLogNotifyPending = false;
		emit chatLogChanged();
	}, Qt::QueuedConnection);
}

void GameHandler::sendChat(const QString &message)
{
	if (!m_session || message.trimmed().isEmpty()) return;
	// Limit it to 128 bytes of UTF-8 (as in the lobby chat).
	QString text = message;
	while (!text.isEmpty() && text.toUtf8().size() > 128)
		text.chop(1);
	if (text.isEmpty()) return;
	m_session->sendGameChatMessage(text.toStdString());
}

bool GameHandler::localGameCallbacksBlocked() const
{
	if (!m_localGameExitRequested) return false;
	if (!m_session) return true;
	return !m_session->isNetworkClientRunning();
}

void GameHandler::refreshPlayerData()
{
	// Build a fresh 10-slot list
	QVariantList newPlayers;
	for (int i = 0; i < 10; ++i) {
		QVariantMap p;
		p["name"]   = QString("");
		p["stack"]  = 0;
		p["bet"]    = 0;
		p["active"] = false;
		p["myTurn"] = false;
		p["seatId"] = i;
		p["button"] = 0;
		p["action"] = 0;
		p["card0"]  = -1;
		p["card1"]  = -1;
		p["fade0"]  = false;
		p["fade1"]  = false;
		p["playerId"] = 0;
		p["countryCode"] = QString("");
		p["isGuest"] = false;
		// The empty seat of a player who has left the table (a disconnect,
		// a kick, leaving, being knocked out) – see the marking below.
		p["reserved"] = false;
		newPlayers.append(p);
	}

	// Lazy-init m_game for local games: session creates the game internally
	if (!m_game && m_session && !m_localGameExitRequested) {
		auto g = m_session->getCurrentGame();
		if (g) m_game = g;
	}

	// A unique token of the current betting round (hand no. × 8 + the round). An action
	// is only shown while this token is unchanged → at the start of a round
	// (across hands as well) all action displays disappear automatically.
	int currentToken = -1;
	if (m_game) {
		auto hand = m_game->getCurrentHand();
		if (hand) {
			currentToken = hand->getMyID() * 8 + static_cast<int>(hand->getCurrentRound());
			// The showdown only applies in the post-river phase. Reset it in every active betting
			// round (preflop–river), so that a flag that stayed set (e.g.
			// when onNextRoundCleanGui does not fire in a network game) does not hide the
			// action badges of the following hands.
			if (hand->getCurrentRound() != GAME_STATE_POST_RIVER)
				setShowdownActive(false);
		}
	}

	// A new betting round → the last aggression (bet/raise) does not apply any more.
	if (currentToken != m_aggressorToken) {
		m_aggressorToken = currentToken;
		m_lastAggressorSeq = 0;
	}

	int humanCount = 0;
	// Only in a network game is getMyUniqueID() the server-wide player id with
	// which the PlayerInfo (the flag, the guest status) can be resolved.
	const bool networkGame = m_session && m_session->isNetworkClientRunning();
	if (m_game) {
		PlayerList seats = m_game->getSeatsList();

		// A preliminary pass: record the action changes, give every action a consecutive
		// sequence number and remember the most recent aggression (bet/raise) of the
		// round – independently of the seat order.
		for (auto it = seats->begin(); it != seats->end(); ++it) {
			int id = (*it)->getMyID();
			if (id < 0 || id >= 10) continue;
			int act = (*it)->getMyAction();
			int curSet = (*it)->getMySet();
			// A fresh action = the action type OR the bet has changed. That way
			// calling again after a raise counts as a new action as well (the type stays CALL, the bet
			// rises) → the badge that was cleared before appears again.
			if (act != m_lastSeenAction[id] || curSet != m_lastSeenSet[id]) {
				m_lastSeenAction[id] = act;
				m_lastSeenSet[id] = curSet;
				m_actionToken[id] = currentToken;
				m_actionSeq[id] = ++m_actionCounter;
				// Aggression (= all the others have to react again): bet/raise
				// always; an all-in only when its bet lies ABOVE the current
				// highest bet of the other players (a real raise – an
				// all-in call at/below the highest bet does not trigger it).
				bool aggressive = (act == PLAYER_ACTION_BET || act == PLAYER_ACTION_RAISE);
				if (act == PLAYER_ACTION_ALLIN) {
					int maxOtherSet = 0;
					for (auto jt = seats->begin(); jt != seats->end(); ++jt) {
						if (jt == it) continue;
						int s = (*jt)->getMySet();
						if (s > maxOtherSet) maxOtherSet = s;
					}
					aggressive = (curSet > maxOtherSet);
				}
				if (aggressive)
					m_lastAggressorSeq = m_actionSeq[id];
			}
		}

		for (auto it = seats->begin(); it != seats->end(); ++it) {
			int id = (*it)->getMyID();
			// A player who has left the game: treat the seat as empty, but
			// mark it as "reserved". The table view can thereby keep the seat
			// – depending on the setting – as an invisible placeholder in the ring,
			// so that the remaining boxes do not move up.
			if (m_leftPlayers.contains((*it)->getMyUniqueID())) {
				if (id >= 0 && id < 10) {
					QVariantMap gone = newPlayers[id].toMap();
					gone["reserved"] = true;
					newPlayers[id] = gone;
				}
				continue;
			}
			if (!(*it)->getMyName().empty() && (*it)->getMyType() == PLAYER_TYPE_HUMAN)
				++humanCount;
			if (id >= 0 && id < 10) {
				int cards[2] = {-1, -1};
				(*it)->getMyCards(cards);
				const bool cardsKnown = cards[0] >= 0 && cards[1] >= 0;
				// Only show the opponents' cards in a real showdown – and only for the
				// players who have to reveal according to the engine (as in the widgets client:
				// not folded AND checkIfINeedToShowCards()). The showdown flag
				// prevents the still stale playerNeedToShowCards list from revealing wrongly
				// during the river betting round of the next hand.
				// The fold/reveal state via the showdown snapshot (the next hand
				// may long since have reset getMyAction()/playerNeedToShowCards
				// – see captureShowdownSnapshot()).
				const bool isFolded = showdownFolded((*it)->getMyUniqueID(),
													 (*it)->getMyAction() == PLAYER_ACTION_FOLD);
				const bool showdownReveal = m_showdownActive
											&& !isFolded
											&& showdownNeedsToShow((*it)->getMyUniqueID(),
													(*it)->checkIfINeedToShowCards());
				// The all-in reveal: after an AllInShowCardsMessage the cards are visible for all
				// players who have not folded (until the next hand).
				const bool allInReveal = m_allInRevealed && !isFolded;
				// Voluntary showing after the hand (AfterHandShowCards): the player
				// does not necessarily have to be in playerNeedToShowCards (a win without a
				// showdown), so reveal separately.
				const bool postRiverShown = m_postRiverShownPlayers.contains((*it)->getMyUniqueID());
				const bool faceUp = cardsKnown && (id == 0 || showdownReveal || allInReveal || postRiverShown);
				if (id != 0 && m_allInRevealed) {
					qDebug() << "[ALLIN] refreshPD seat" << id
							 << "cardsKnown=" << cardsKnown
							 << "cards=" << cards[0] << "/" << cards[1]
							 << "action=" << (int)(*it)->getMyAction()
							 << "allInReveal=" << allInReveal
							 << "faceUp=" << faceUp;
				}

				// In the showdown ALL action badges are removed (all-in and
				// fold as well) – now only the revealed cards, the winning hand
				// and the winner count.
				// Otherwise: all-in stays visible throughout the hand (the engine
				// keeps PLAYER_ACTION_ALLIN across all rounds and only resets it
				// for the next hand). The other actions disappear at the start of a round
				// (the token logic) and as soon as another player has bet/raised
				// (the sequence < the last aggression).
				int act = (*it)->getMyAction();
				const bool sameRound = (currentToken >= 0 && m_actionToken[id] == currentToken);
				int displayAction;
				if (m_showdownActive || allInReveal || postRiverShown) {
					// The showdown, the all-in runout and voluntary showing (the cards are
					// revealed): remove the badge, so that the revealed cards are
					// not covered.
					displayAction = 0;
				} else if (act == PLAYER_ACTION_ALLIN) {
					displayAction = act;
				} else if (act == PLAYER_ACTION_FOLD) {
					// "Fold" stays for the round – a later bet/raise
					// of another player does not remove it (only players who have not
					// folded have to act again).
					displayAction = sameRound ? act : 0;
				} else if (sameRound && m_actionSeq[id] >= m_lastAggressorSeq) {
					displayAction = act;
				} else {
					displayAction = 0;
				}

				QVariantMap p;
				p["name"]   = QString::fromStdString((*it)->getMyName());
				p["stack"]  = (*it)->getMyCash();
				p["bet"]    = (*it)->getMySet();
				p["active"] = (*it)->getMyActiveStatus();
				p["myTurn"] = (*it)->getMyTurn();
				p["seatId"] = id;
				// The dealer/small/big blind only for active players: otherwise those who dropped out
				// (0 coins, out) keep their old BB/SB/D icon until the
				// next hand. All-in players stay active and keep it correctly.
				p["button"] = (*it)->getMyActiveStatus() ? (*it)->getMyButton() : BUTTON_NONE;
				p["action"] = displayAction;
				// Players who have folded stay folded throughout the hand → show the cards
				// translucently (as in the Qt widgets client).
				p["folded"] = isFolded;
				// Computer opponents: there are no context actions for them
				// (ignore/stats) – as in the Qt widgets client (MyAvatarLabel).
				p["isComputer"] = ((*it)->getMyType() == PLAYER_TYPE_COMPUTER);
				// A local game: no network player id, no flag.
				p["playerId"] = 0;
				p["countryCode"] = QString("");
				p["isGuest"] = false;
				// A network game: the player id, the country flag and the guest status directly
				// via the unique player id from the session – exactly like
				// gameTableImpl::refreshPlayerAvatar() in the Qt widgets client. The
				// detour via the player list of the game (gamePlayersInGame) is
				// unreliable at the table: during the game the client is unsubscribed from
				// the lobby messages, and a GamePlayerJoined does NOT add
				// the player to the GameInfo player list. Whoever comes to the table only
				// during the running game (a spectator whom the server seats for the next hand)
				// is therefore missing there – their flag stayed away,
				// although the waiting room still showed it.
				if (networkGame) {
					const unsigned uniqueId = (*it)->getMyUniqueID();
					const PlayerInfo info = m_session->getClientPlayerInfo(uniqueId);
					p["playerId"] = uniqueId;
					p["countryCode"] = QString::fromStdString(info.countryCode).toLower();
					p["isGuest"] = info.isGuest;
				}
				// The avatar (the player avatar that is set); seat 0 from the
				// config if need be – but only if I sit there myself. As a spectator
				// seat 0 is a foreign player who would otherwise get MY avatar.
				std::string avatarRaw = (*it)->getMyAvatar();
				if (avatarRaw.empty() && id == 0 && !m_spectating && m_config)
					avatarRaw = m_config->readConfigString("MyAvatar");
				p["avatar"] = resolveAvatarSource(avatarRaw);
				p["card0"]  = faceUp ? cards[0] : -1;
				p["card1"]  = faceUp ? cards[1] : -1;
				// The showdown spotlight: dim a hole card of the winner if it
				// does not count towards their best hand (the sets are filled in the showdown,
				// otherwise empty → fade0/fade1 false).
				p["fade0"]  = m_holeFade0.contains(id);
				p["fade1"]  = m_holeFade1.contains(id);
				if (id == 0) {
					// qDebug() << "[DBG] seat0 cards:" << cards[0] << cards[1]
					//          << "faceUp:" << faceUp;
				}
				newPlayers[id] = p;
			}
		}
	}

	// The chat icon only if there is another human player besides me.
	const bool newHasHumanOpponents = humanCount > 1;
	if (newHasHumanOpponents != m_hasHumanOpponents) {
		m_hasHumanOpponents = newHasHumanOpponents;
		emit hasHumanOpponentsChanged();
	}

	m_players = newPlayers;
	emit playersChanged();

	// Update the odds + the current hand (a fold/active change, new hole cards).
	refreshChanceAndHand();

	// A local game: hide players with 0 coins after 10 seconds.
	checkBustedLocalPlayers();
}

void GameHandler::refreshPotData()
{
	if (!m_game) return;
	auto hand = m_game->getCurrentHand();
	if (!hand) return;
	auto board = hand->getBoard();
	if (!board) return;

	int newPot = board->getPot();
	if (newPot != m_pot) {
		m_pot = newPot;
		emit potChanged();
	}

	int newTotalPot = board->getPot() + board->getSets();
	if (newTotalPot != m_totalPot) {
		m_totalPot = newTotalPot;
		emit totalPotChanged();
	}
}

void GameHandler::computeCallAndRaiseAmounts()
{
	int newCallAmount = 0;
	int newMinRaise = 0;
	int newMaxRaise = 0;
	bool newCanAct = false;
	int dbgMyAction = -1;   // [ACTDBG] the engine action read last (for the log)
	int dbgPrevId   = -99;  // [ACTDBG] getPreviousPlayerID() (for the log)
	int dbgHandId   = -1;   // [ACTDBG] the current hand ID (for the log)
	bool dbgRoundClosed = false;  // [ACTDBG] the betting round is finished (for the log)

	// As a spectator there is no seat of your own: the call/raise amounts stay 0
	// and canAct false (the GamePage hides the action bar anyway).
	if (m_game && !m_spectating) {
		auto hand = m_game->getCurrentHand();
		if (hand) {
			auto bero = hand->getCurrentBeRo();
			auto seats = hand->getSeatsList();
			if (bero && seats && !seats->empty()) {
				auto humanPlayer = seats->front();
				const int highestSet = bero->getHighestSet();
				const int humanSet = humanPlayer->getMySet();
				const int humanCash = humanPlayer->getMyCash();

				// The buttons are active (for a preselection OR a real turn) when I am in
				// the hand and not all-in AND either am to act right now
				// (m_myTurn) OR was NOT the player who acted last.
				//
				// That mirrors gameTableImpl::updateMyButtonsState() of the
				// widgets reference exactly: there the buttons are "checkable" (a preselection)
				// while getPreviousPlayerID() != 0 (= I was not the last
				// actor). getMyAction() == NONE is NOT suitable as a criterion: after
				// my action (CALL/CHECK) and a subsequent raise of
				// an opponent I have to be able to act again – but getMyAction()
				// is != NONE by then, so that the preselection wrongly
				// stayed locked until it is my turn. previousPlayerID is set to -1 on a
				// round change/deal (the preselection stays open, no
				// flicker) and to the actor after every action – after MY
				// action therefore to 0, which switches the buttons off cleanly until an
				// opponent acts.
				const int myAction = humanPlayer->getMyAction();
				const int prevPlayerId = hand->getPreviousPlayerID();
				dbgMyAction = myAction;
				dbgPrevId   = prevPlayerId;
				dbgHandId   = hand->getMyID();
				const bool baseEligible =
					myAction != PLAYER_ACTION_FOLD
					&& myAction != PLAYER_ACTION_ALLIN
					&& humanCash > 0
					&& humanPlayer->isSessionActive();

				// Is the betting round finished? After the first pass of the betting round
				// (!firstRound) and as soon as all players who are still running have reached the
				// highest bet (allHighestSet), the round is
				// decided – exactly the criterion from LocalBeRo::run(). In
				// this time window (the last action has happened, the next round/hand
				// has not started yet) NO preselection with stale
				// values may stay active → the buttons are inactive. It does not apply when I
				// am to act myself right now (e.g. a closing check as the last
				// player) – that is handled by the regular m_myTurn path.
				//
				// "Is the first pass of the betting round over?" is delivered locally by the engine via
				// getFirstRound(). In the NETWORK client, however, firstRound is NEVER
				// set to false (clientstate.cpp never calls setFirstRound(false))
				// → roundClosed would always be false online, canAct would stay
				// true after the last turn and bettingRoundEnded would never fire. That
				// is exactly the 1–2 second window (the last action → the next round)
				// in which a preselection (with stale values) was still possible online.
				// A network capable replacement: the round is only "over"
				// when ALL players who are still running have already acted in this betting
				// round (getMyAction() != NONE; at the start of a round
				// ResetPlayerActions() sets all of them to NONE, locally LocalBeRo::run()). Both
				// criteria become true at the same point (the last player acts),
				// but they exclude the false alarm at the start of a round (all sets ==
				// highestSet BEFORE anybody has acted). The OR leaves the local
				// behaviour unchanged: there !firstRound becomes true even earlier.
				bool roundClosed = false;
				if (!m_myTurn) {
					auto running = hand->getRunningPlayerList();
					if (running && !running->empty()) {
						bool allRunningActed = true;
						for (auto it = running->begin(); it != running->end(); ++it) {
							if ((*it)->getMyAction() == PLAYER_ACTION_NONE) {
								allRunningActed = false;
								break;
							}
						}
						if (!bero->getFirstRound() || allRunningActed) {
							// Finished when all running players have reached the highest
							// bet. In the transition to the next round, however, the client
							// already collects the sets into the pot (all mySet == 0),
							// while highestSet still briefly stands at the
							// old value – then the pure comparison with
							// highestSet wrongly delivered "not finished": canAct
							// flickered back to true and showed stale
							// call/raise values with (re-)activated buttons for 1–2 s. Therefore
							// the collected state (all sets == 0) counts as
							// finished as well. Both cases are secured by
							// allRunningActed/!firstRound, so that the
							// start of a round (sets == 0, but the actions freshly on NONE) does
							// NOT wrongly count as finished. The tricky
							// heads-up all-in-over-bet case stays correct: there
							// my set is != 0 and != highestSet → neither allMatched nor
							// allCollected, so not finished (and it is my turn
							// anyway → the m_myTurn guard applies).
							bool allMatched = true;
							bool allCollected = true;
							for (auto it = running->begin(); it != running->end(); ++it) {
								if ((*it)->getMySet() != highestSet) allMatched = false;
								if ((*it)->getMySet() != 0)          allCollected = false;
							}
							roundClosed = allMatched || allCollected;
						}
					}
				}
				dbgRoundClosed = roundClosed;

				newCanAct = baseEligible
							&& (m_myTurn || prevPlayerId != 0)
							&& !m_showdownActive
							&& !roundClosed;

				if (humanCash + humanSet <= highestSet) {
					newCallAmount = humanCash;
				} else {
					newCallAmount = highestSet - humanSet;
				}
				if (newCallAmount < 0) {
					newCallAmount = 0;
				}

				const bool buttonsDisabled =
					humanPlayer->getMyAction() == PLAYER_ACTION_ALLIN ||
					humanPlayer->getMyAction() == PLAYER_ACTION_FOLD ||
					humanCash == 0 ||
					(humanSet == highestSet && humanPlayer->getMyAction() != PLAYER_ACTION_NONE) ||
					!humanPlayer->isSessionActive();

				// Publish the raise amounts ONLY while I am allowed to act at
				// all (newCanAct). In the round transition the formula otherwise delivers
				// seemingly valid but STALE values: if the sets are already
				// collected into the pot (mySet == 0) but BeRo/round is still
				// preflop (network: collectPot() before the round change),
				// minimum = highestSet + minimumRaise = 2×BB. QML seeds the
				// default bet from it (syncRaiseAmount/the self-healing in GameActionBar)
				// and afterwards only clamps it to [min,max] – the stale
				// default ("Bet $2×BB" instead of $BB at the flop) thus stayed.
				// newCanAct is false in exactly these windows (roundClosed/
				// the showdown/already acted) – then there are no valid
				// raise amounts either.
				if (newCanAct && !buttonsDisabled && !bero->getFullBetRule()) {
					int minimum = 0;
					bool canBetRaise = false;

					if (hand->getCurrentRound() == 0) {
						if (humanCash + humanSet > highestSet) {
							minimum = highestSet - humanSet + bero->getMinimumRaise();
							canBetRaise = true;
						}
					} else {
						if (highestSet == 0) {
							minimum = hand->getSmallBlind() * 2;
							canBetRaise = true;
						} else if (highestSet > humanSet && humanCash + humanSet > highestSet) {
							minimum = highestSet - humanSet + bero->getMinimumRaise();
							canBetRaise = true;
						}
					}

					if (canBetRaise) {
						if (minimum < 0) {
							minimum = 0;
						}
						newMaxRaise = humanCash;
						newMinRaise = std::min(minimum, newMaxRaise);
					}
				}
			}
		}
	}

	// SUSPICION: myTurn=true but the engine already shows us as fold/all-in → stale data?
	if (m_myTurn && (dbgMyAction == PLAYER_ACTION_FOLD || dbgMyAction == PLAYER_ACTION_ALLIN)) {
		qDebug() << "[ACTDBG] SUSPECT: myTurn=true but myAction=" << dbgMyAction
				 << "(FOLD=1,ALLIN=6) handId=" << dbgHandId
				 << "prevId=" << dbgPrevId << "newCanAct=" << newCanAct;
	}
	// The edge "the betting round has just been decided": exactly now (the last action has happened,
	// the next round/hand has yet to start) the action buttons have to become
	// inactive immediately and stale preselections/amounts have to be discarded. roundClosed
	// is only true in exactly this recompute; afterwards (a new BeRo, firstRound true
	// again) it falls back, hence the edge detection via m_roundClosed.
	if (dbgRoundClosed && !m_roundClosed) {
		m_roundClosed = true;
		qDebug() << "[ACTDBG] bettingRoundEnded (roundClosed rising)"
				 << "handId=" << dbgHandId << "prevPlayerId=" << dbgPrevId;
		emit bettingRoundEnded();
	} else if (!dbgRoundClosed) {
		m_roundClosed = false;
	}
	if (newCanAct != m_canAct) {
		m_canAct = newCanAct;
		qDebug() << "[ACTDBG] canAct=" << m_canAct << "prevPlayerId=" << dbgPrevId
				 << "myAction=" << dbgMyAction
				 << "(NONE=0,FOLD=1,CHK=2,CALL=3,BET=4,RAISE=5,ALLIN=6)"
				 << "handId=" << dbgHandId << "roundClosed=" << dbgRoundClosed
				 << "myTurn=" << m_myTurn << "tSeat=" << m_timeoutSeatId;
		emit canActChanged();
	}
	if (newCallAmount != m_callAmount) {
		m_callAmount = newCallAmount;
		emit callAmountChanged();
	}
	if (newMinRaise != m_minRaiseAmount) {
		m_minRaiseAmount = newMinRaise;
		emit minRaiseAmountChanged();
	}
	if (newMaxRaise != m_maxRaiseAmount) {
		m_maxRaiseAmount = newMaxRaise;
		emit maxRaiseAmountChanged();
	}

	// The central update point: computeCallAndRaiseAmounts() hangs off EVERY
	// refresh path (onRefreshSet/Cash/Pot/GameLabels, onMeInAction, doActionDone),
	// so off the onRefreshSet triggered by the PlayersTurnMessage as well –
	// currentPlayersTurnId is already set there.
	updateAwaitingMyAction();
}

bool GameHandler::humanCanAct() const
{
	// As a spectator there is no seat of your own. Without this guard the function
	// below would read seats->front() – the FOREIGN player on seat 0 – and
	// startTimeoutAnimation() would wrongly turn that into "it is my turn".
	if (m_spectating) return false;
	if (!m_game) return false;
	auto hand = m_game->getCurrentHand();
	if (!hand) return false;
	auto seats = hand->getSeatsList();
	if (!seats || seats->empty()) return false;
	auto human = seats->front();
	if (!human) return false;
	const int a = human->getMyAction();
	return a != PLAYER_ACTION_FOLD
		   && a != PLAYER_ACTION_ALLIN
		   && human->getMyCash() > 0
		   && human->isSessionActive();
}

bool GameHandler::isMyTurnToAct() const
{
	if (m_spectating) return false;
	if (m_session && m_session->isNetworkClientRunning()) {
		// Network: the turn window flag is authoritative. It replaces the earlier
		// implicit "after the action both flags are cleared" and at the same time
		// allows the authoritative engine source as a third opener.
		return !m_myTurnWindowClosed
			   && (m_myTurn || m_timeoutSeatId == 0 || engineAwaitsMyAction());
	}
	// A local game: unchanged. There is no turn pointer from a server there that
	// could open a window again – m_myTurn/m_timeoutSeatId stay the
	// only source (set by LocalBeRo via meInAction/startTimeoutAnimation).
	return m_myTurn || m_timeoutSeatId == 0;
}

bool GameHandler::engineTurnPointsAtMe() const
{
	// A PURE pointer comparison, deliberately without humanCanAct(): only that way is the
	// value monotonic and usable as an edge for OPENING a turn window. If one took
	// humanCanAct() into it, a mere state change of your own
	// player (the action reset to NONE by ResetPlayerActions, the cash > 0 again after
	// a won pot) could tear a window that has long been closed
	// open again, although no new PlayersTurnMessage arrived at all.
	//
	// Only in a network game: there the server assigns the unique IDs and 0 is
	// explicitly invalid (serverlobbythread.cpp), so that the initial value of a
	// fresh BeRo (currentPlayersTurnId == 0) never accidentally matches me.
	// In a local game, by contrast, your own unique ID is 0 – there
	// the initial value could not be distinguished from a real turn marking.
	if (m_spectating) return false;
	if (!m_session || !m_session->isNetworkClientRunning()) return false;
	if (!m_game) return false;
	auto hand = m_game->getCurrentHand();
	if (!hand) return false;
	auto bero = hand->getCurrentBeRo();
	if (!bero) return false;
	auto seats = hand->getSeatsList();
	if (!seats || seats->empty()) return false;
	auto human = seats->front();
	if (!human) return false;
	return bero->getCurrentPlayersTurnId() == human->getMyUniqueID();
}

bool GameHandler::engineAwaitsMyAction() const
{
	// An open turn window = the turn pointer points at me, I can act at
	// all, and the window has not already been closed.
	return engineTurnPointsAtMe() && !m_myTurnWindowClosed && humanCanAct();
}

void GameHandler::openMyTurnWindow()
{
	if (!m_myTurnWindowClosed) return;
	m_myTurnWindowClosed = false;
	updateAwaitingMyAction();
}

void GameHandler::closeMyTurnWindow()
{
	if (m_myTurnWindowClosed) return;
	m_myTurnWindowClosed = true;
	updateAwaitingMyAction();
}

void GameHandler::updateAwaitingMyAction()
{
	// A rising edge of the turn pointer = a PlayersTurnMessage for me has
	// arrived → a new window. If the pointer goes away (another player is to act,
	// a new betting round/hand with a fresh BeRo), the window stays closed
	// until it points at me again.
	const bool pointsAtMe = engineTurnPointsAtMe();
	if (pointsAtMe != m_engineTurnPointedAtMe) {
		m_engineTurnPointedAtMe = pointsAtMe;
		m_myTurnWindowClosed = !pointsAtMe;
	}
	const bool waiting = pointsAtMe && !m_myTurnWindowClosed && humanCanAct();
	if (waiting != m_awaitingMyAction) {
		m_awaitingMyAction = waiting;
		emit awaitingMyActionChanged();
	}
}

void GameHandler::doActionDone()
{
	if (!m_session) return;
	if (localGameCallbacksBlocked()) return;

	// The action for this turn window is out → the window is closed. For the
	// engine based turn detection that replaces what clearing the two flags below
	// did implicitly so far: no second turn in the same window.
	closeMyTurnWindow();

	if (m_myTurn) {
		m_myTurn = false;
		emit myTurnChanged();
	}
	// Close my action window: while m_timeoutSeatId == 0 I would still count as
	// "to act" via isMyTurnToAct() → a second action could slip through.
	// Now that I have acted, end it immediately (stopTimeoutAnimation follows anyway).
	if (m_timeoutSeatId == 0) {
		m_timeoutSeatId = -1;
		emit timeoutChanged();
	}
	qDebug() << "[ACTDBG] doActionDone sent, net="
			 << (m_session && m_session->isNetworkClientRunning());

	// I have acted → switch the buttons inactive immediately. canAct derives that
	// from getMyAction() (!= NONE); at this point the action is already
	// set on the player (fold/call/raise), so the recompute delivers the
	// correct value (inactive until the next round or until it is my turn again).
	computeCallAndRaiseAmounts();

	if (m_session->isNetworkClientRunning()) {
		// Network game: send action to server
		m_session->sendClientPlayerAction();
	} else {
		// Local game: advance game loop (equivalent to Qt5 nextPlayerAnimation -> switchRounds)
		boost::shared_ptr<Game> game = m_game;
		QTimer::singleShot(300, this, [game]() {
			if (game && game->getCurrentHand())
				game->getCurrentHand()->switchRounds();
		});
	}
}

void GameHandler::setShowdownActive(bool active)
{
	if (m_showdownActive == active)
		return;
	m_showdownActive = active;
	emit showdownActiveChanged();
}

// ─── slots called from QmlGuiInterface ──────────────────────────────────────

void GameHandler::onRefreshSet()
{
	if (localGameCallbacksBlocked()) return;
	qDebug() << "[ACTDBG] >> onRefreshSet myTurn=" << m_myTurn;
	refreshPlayerData();
	computeCallAndRaiseAmounts();
}

void GameHandler::onRefreshAction(int playerId, int playerAction)
{
	if (localGameCallbacksBlocked()) return;
	qDebug() << "[ACTDBG] >> onRefreshAction id=" << playerId << "act=" << playerAction << "myTurn=" << m_myTurn;
	refreshPlayerData();
	computeCallAndRaiseAmounts();

	// It corresponds to gametableimpl::refreshAction: only play the action sound on a
	// specific action (not on a global refresh).
	if (playerId < 0 || playerAction <= 0 || playerAction > 6)
		return;
	// A real player action (CHECK/CALL/FOLD/BET/RAISE/ALLIN): a signal to QML
	emit refreshActionTriggered();
	if (!m_config || m_config->readConfigInt("PlayGameActions") == 0)
		return;

	static const char *kActionSounds[] = {
		"", "fold", "check", "call", "bet", "raise", "allin"
	};

	if (m_soundEventHandler)
		m_soundEventHandler->playSound(kActionSounds[playerAction], playerId);
}

void GameHandler::onRefreshCash()
{
	if (localGameCallbacksBlocked()) return;
	qDebug() << "[ACTDBG] >> onRefreshCash myTurn=" << m_myTurn;
	refreshPlayerData();
	computeCallAndRaiseAmounts();
}

void GameHandler::onRefreshPlayerName()
{
	if (localGameCallbacksBlocked()) return;
	refreshPlayerData();
}

void GameHandler::onRefreshPot()
{
	if (localGameCallbacksBlocked()) return;
	qDebug() << "[ACTDBG] >> onRefreshPot myTurn=" << m_myTurn;
	refreshPotData();
	refreshPlayerData();
	computeCallAndRaiseAmounts();
}

void GameHandler::onRefreshGameLabels(int gameState)
{
	if (localGameCallbacksBlocked()) return;
	QString newPhase;
	switch (gameState) {
	case 0:
		newPhase = "Preflop";
		break;
	case 1:
		newPhase = "Flop";
		break;
	case 2:
		newPhase = "Turn";
		break;
	case 3:
		newPhase = "River";
		break;
	default:
		newPhase = "";
		break;
	}

	const bool phaseChanged = (newPhase != m_phaseText);
	if (phaseChanged) {
		// Reset myTurn before the phase change: onNextRoundCleanGui may arrive via a
		// QueuedConnection only after this synchronous method. Without the reset
		// m_myTurn would still be true → QML myTurnNow=true → the buttons would be enabled with
		// stale values from the previous round.
		if (m_myTurn) {
			m_myTurn = false;
			emit myTurnChanged();
		}
		// A round boundary (the flop/turn/river has been dealt): my turn window of the
		// previous betting round is over. Only the next PlayersTurnMessage
		// for me opens one again.
		closeMyTurnWindow();
		m_phaseText = newPhase;
	}

	bool handChanged = false;
	if (m_game) {
		auto hand = m_game->getCurrentHand();
		if (hand) {
			int newHandNum = hand->getMyID();
			if (newHandNum != m_handNumber) {
				m_handNumber = newHandNum;
				handChanged = true;
			}
		}
	}

	// FIRST recompute the amounts, THEN signal the round/hand boundary:
	// on a phaseText/handNumber change QML sets raiseAmount to 0 and fills
	// it again immediately from minRaiseAmount (the self-healing in GameActionBar).
	// If the signals fired BEFORE the recompute (the old order), this
	// re-seeding still saw the values of the OLD round/hand – and syncRaiseAmount()
	// never lowers an amount that has been set once to the fresh minimum
	// again (it only clamps to [min,max]). The consequence: the default raise stuck e.g. at the
	// preflop minimum (2×BB) instead of following the BB at the flop.
	computeCallAndRaiseAmounts();
	if (phaseChanged)
		emit phaseTextChanged();
	if (handChanged)
		emit handNumberChanged();
	// After computeCallAndRaiseAmounts() all values of the new round are correct
	// (switchRounds() is already finished). QML can release the preselection
	// now – independently of whether callAmountChanged has fired.
	if (phaseChanged)
		emit roundValuesReady();
}

void GameHandler::onMeInAction()
{
	qDebug() << "[ACTDBG] onMeInAction() blocked=" << localGameCallbacksBlocked()
			 << "myTurn=" << m_myTurn << "tSeat=" << m_timeoutSeatId;
	if (localGameCallbacksBlocked()) return;
	// The authoritative turn start of the engine → the window opens (see doActionDone).
	openMyTurnWindow();
	refreshPlayerData();
	// Set m_myTurn FIRST, THEN compute the amounts. computeCallAndRaiseAmounts()
	// evaluates roundClosed only with !m_myTurn – if I am in position (the last actor)
	// and all opponents have checked, all sets are equal (postflop 0) → roundClosed
	// would wrongly become true, newCanAct false and the raise amounts would stay 0 (the slider/
	// bet locked, only all-in possible). Since it is becoming MY turn right here, m_myTurn=true
	// is correct and switches the roundClosed misdetection off cleanly.
	//
	// Fire the myTurnChanged signal ONLY AFTER the recompute: QML reacts to it
	// (onMyTurnChanged) among other things by switching the focus into the amount field, provided
	// raiseAvailable – and that presupposes correctly computed min/maxRaiseAmount.
	const bool becameMyTurn = !m_myTurn;
	if (becameMyTurn)
		m_myTurn = true;
	computeCallAndRaiseAmounts();
	if (becameMyTurn)
		emit myTurnChanged();
	qDebug() << "[ACTDBG] meInAction myTurn=" << m_myTurn << "tSeat=" << m_timeoutSeatId;
	if (m_game) {
		auto dh = m_game->getCurrentHand();
		if (dh) {
			auto db = dh->getCurrentBeRo();
			auto ds = dh->getSeatsList();
			if (db && ds && !ds->empty()) {
				auto hp = ds->front();
				qDebug() << "[ACTDBG]   amounts call=" << m_callAmount
						 << "minRaise=" << m_minRaiseAmount << "maxRaise=" << m_maxRaiseAmount
						 << "| mySet=" << hp->getMySet() << "highestSet=" << db->getHighestSet()
						 << "cash=" << hp->getMyCash() << "myButton=" << hp->getMyButton()
						 << "(1=D,2=SB,3=BB) myAction=" << hp->getMyAction()
						 << "round=" << dh->getCurrentRound()
						 << "fullBetRule=" << db->getFullBetRule()
						 << "minRaiseEngine=" << db->getMinimumRaise();
				qDebug() << "[BBDBG] onMeInAction BB-check:"
						 << "bbPosId=" << (int)db->getBigBlindPositionId()
						 << "sbPosId=" << (int)db->getSmallBlindPositionId()
						 << "p0UniqueId=" << hp->getMyUniqueID()
						 << "prevPlayerId=" << dh->getPreviousPlayerID()
						 << "firstRound=" << db->getFirstRound()
						 << "isP0BB=" << (hp->getMyUniqueID() == db->getBigBlindPositionId());
			}
		}
	}
	// The authoritative "it is my turn" point (like meInAction in the widgets client):
	// here – and only here – trigger the noted/automatic action,
	// ALWAYS (even when m_myTurn was already true above, e.g. via the action timer).
	emit meInActionTriggered();
}

void GameHandler::onDisableMyButtons()
{
	if (localGameCallbacksBlocked()) return;
	int dbgAct = -1;
	if (m_game) {
		auto hand = m_game->getCurrentHand();
		if (hand) {
			auto seats = hand->getSeatsList();
			if (seats && !seats->empty())
				dbgAct = seats->front()->getMyAction();
		}
	}
	qDebug() << "[ACTDBG] onDisableMyButtons myTurn=" << m_myTurn
			 << "p0Action=" << dbgAct
			 << "(NONE=0,FOLD=1,CHK=2,CALL=3,BET=4,RAISE=5,ALLIN=6)";
	if (m_myTurn) {
		m_myTurn = false;
		emit myTurnChanged();
	}
	// The server has booked an action for MY seat (my own turn, a timeout
	// with the default action or an auto fold) → my turn window is definitely over.
	// Without this closing, awaitingMyAction stayed set after a timeout until the
	// next PlayersTurnMessage and kept the buttons armed beyond the
	// end of the round.
	closeMyTurnWindow();
}

void GameHandler::onStartTimeoutAnimation(int playerNum, int timeoutSec)
{
	// Log BEFORE the guard, so that blocked calls are visible
	if (playerNum == 0)
		qDebug() << "[ACTDBG] onStartTimeout(0) blocked=" << localGameCallbacksBlocked()
				 << "myTurn=" << m_myTurn << "humanCanAct=" << humanCanAct()
				 << "tSeat=" << m_timeoutSeatId << "timeoutSec=" << timeoutSec;
	if (localGameCallbacksBlocked()) return;

	// The server is now counting MY action time → the window opens.
	if (playerNum == 0)
		openMyTurnWindow();

	// A progress bar (a replacement for the action badge) for the seat that is currently active.
	if (m_timeoutSeatId != playerNum || m_timeoutSec != timeoutSec) {
		m_timeoutSeatId = playerNum;
		m_timeoutSec = timeoutSec;
		emit timeoutChanged();
	}

	// The server is now counting the action time down for THIS seat. If it is
	// my seat (0) and I can act, it is definitely my turn. Set m_myTurn
	// here (not only in onMeInAction): startTimeoutAnimation comes
	// BEFORE meInAction and marks exactly the window in which the server waits for
	// my action. Otherwise there was a window in which the buttons were already
	// active (canAct) but m_myTurn was still false → clicks/preselections
	// were discarded as "no turn" (fold/call/raise check m_myTurn) and
	// ran into the timeout (the server auto check).
	if (playerNum == 0 && !m_myTurn && humanCanAct()) {
		m_myTurn = true;
		emit myTurnChanged();
	}
	updateAwaitingMyAction();
	if (playerNum == 0)
		qDebug() << "[ACTDBG] startTimeout seat0 myTurn=" << m_myTurn
				 << "humanCanAct=" << humanCanAct() << "tSeat=" << m_timeoutSeatId
				 << "awaiting=" << m_awaitingMyAction;

	// As in the widgets client: the sound only after 3 seconds of lead time – only for me
	// and only if I am still to act (a noted action may already have been
	// executed synchronously above → then no beep).
	if (playerNum == 0 && m_myTurn && timeoutSec >= 4)
		m_timeoutBeepTimer->start((timeoutSec - 3) * 1000);
}

void GameHandler::onStopTimeoutAnimation(int playerNum)
{
	if (m_timeoutSeatId == playerNum) {
		m_timeoutSeatId = -1;
		emit timeoutChanged();
	}
	m_timeoutBeepTimer->stop();

	// My action window is over (I acted or the turn expired) → end the turn,
	// matching the setting in onStartTimeoutAnimation.
	if (playerNum == 0 && m_myTurn) {
		m_myTurn = false;
		emit myTurnChanged();
	}
	// The timer on my seat was stopped = the window is over (acted or expired).
	if (playerNum == 0)
		closeMyTurnWindow();
	updateAwaitingMyAction();
}

void GameHandler::onNetworkGameEnded()
{
	// Removed from the network game (the end of the game, it was closed, a kick …):
	// reset our own state cleanly. Without this m_myTurn and
	// m_game stay stale; a later action (e.g. the auto mode of the ComboBox on the
	// still visible GamePage) would call fold()/call() with a guard that looks
	// valid and would dereference a null game shared_ptr on the engine
	// side.
	m_game.reset();
	if (m_myTurn) {
		m_myTurn = false;
		emit myTurnChanged();
	}
	if (m_timeoutSeatId != -1) {
		m_timeoutSeatId = -1;
		emit timeoutChanged();
	}
	closeMyTurnWindow();   // no game any more → no open turn window
	m_timeoutBeepTimer->stop();
	if (m_pingState != 0 || m_pingAvg != -1) {
		m_pingState = 0;  // no network connection any more → reset the status light
		m_pingAvg = m_pingMin = m_pingMax = -1;
		emit pingStateChanged();
	}
}

void GameHandler::onPingUpdate(int minPing, int avgPing, int maxPing)
{
	// The status light thresholds as in the Qt widgets client (MyAvatarLabel::refreshPing):
	// ≤1000 ms green, ≤2000 ms yellow, >2000 ms red; negative values = no data.
	int state;
	if (avgPing < 0)            state = 0;
	else if (avgPing <= 1000)   state = 1;
	else if (avgPing <= 2000)   state = 2;
	else                        state = 3;
	bool changed = false;
	if (state != m_pingState)   {
		m_pingState = state;
		changed = true;
	}
	if (avgPing != m_pingAvg)   {
		m_pingAvg   = avgPing;
		changed = true;
	}
	if (minPing != m_pingMin)   {
		m_pingMin   = minPing;
		changed = true;
	}
	if (maxPing != m_pingMax)   {
		m_pingMax   = maxPing;
		changed = true;
	}
	if (changed)
		emit pingStateChanged();
}

void GameHandler::onNetClientPlayerLeft(unsigned uniquePlayerId, const QString &playerName, int removeReason)
{
	// Note in the game history whether the player left voluntarily, was kicked
	// or lost the connection (removeReason from clientstate.cpp).
	if (!playerName.isEmpty()) {
		QString line;
		switch (removeReason) {
		case NTF_NET_REMOVED_KICKED:
			line = playerName + QStringLiteral(" was kicked from the game");
			break;
		case NTF_NET_INTERNAL:
			line = playerName + QStringLiteral(" was disconnected");
			break;
		default: // NTF_NET_REMOVED_ON_REQUEST
			line = playerName + QStringLiteral(" has left the game");
			break;
		}
		appendGameLog(line, LogSitOut);
	}

	m_leftPlayers.insert(uniquePlayerId);
	refreshPlayerData();
	emit playersChanged();
}

void GameHandler::refreshSpectators()
{
	// Read the spectator list of the running game from the session – analogous to the
	// Qt widgets client (gameTableImpl::refreshSpectatorsDisplay).
	QStringList names;
	if (m_session && m_session->isNetworkClientRunning()) {
		const unsigned gameId = m_session->getClientCurrentGameId();
		if (gameId != 0) {
			const GameInfo info = m_session->getClientGameInfo(gameId);
			for (unsigned id : info.spectatorsDuringGame) {
				const PlayerInfo pi = m_session->getClientPlayerInfo(id);
				names << QString::fromUtf8(pi.playerName.c_str());
			}
			// The server sends a spectator only the OTHER
			// spectators (AcceptNewSession sends their own SpectatorJoined to
			// all the other sessions). Without this addition the eye icon showed
			// "0" to the only spectator of a table.
			if (m_session->isClientSpectating()) {
				const PlayerInfo me = m_session->getClientPlayerInfo(
										  m_session->getClientUniquePlayerId());
				const QString myName = QString::fromUtf8(me.playerName.c_str());
				if (!myName.isEmpty())
					names << myName;
			}
		}
	}
	if (names != m_spectatorNames) {
		m_spectatorNames = names;
		emit spectatorsChanged();
	}
}

void GameHandler::checkBustedLocalPlayers()
{
	// Only in a local game (no network client).
	if (!m_session || m_session->isNetworkClientRunning()) return;
	if (!m_game) return;

	PlayerList seats = m_game->getSeatsList();
	for (auto it = seats->begin(); it != seats->end(); ++it) {
		int id = (*it)->getMyID();
		// Seat 0 = the human player, never hide it automatically.
		if (id <= 0) continue;
		// Skip empty seats.
		if ((*it)->getMyName().empty()) continue;

		unsigned uid = (*it)->getMyUniqueID();
		// Already left → no timer needed.
		if (m_leftPlayers.contains(uid)) continue;

		// Only hide players who really dropped out: cash==0 AND no longer
		// active in the tournament. An all-in player has cash==0 in the middle of a hand
		// as well (their whole stack lies in the pot as a bet), but keeps
		// active==true and still plays for the pot – otherwise they would be removed from the table
		// wrongly after 10 s although the game is still running. Only at the
		// next hand start does the engine (game.cpp) set cash==0 players inactive.
		if ((*it)->getMyCash() == 0 && !(*it)->getMyActiveStatus()) {
			// No timer for this player yet: start the 10 second delay.
			if (!m_bustedLocalTimers.contains(uid)) {
				QTimer *t = new QTimer(this);
				t->setSingleShot(true);
				connect(t, &QTimer::timeout, this, [this, uid]() {
					m_bustedLocalTimers.remove(uid);
					m_leftPlayers.insert(uid);
					refreshPlayerData();
				});
				m_bustedLocalTimers.insert(uid, t);
				t->start(10000);
			}
		} else {
			// The player has chips again (e.g. a rebuy) → discard the running timer.
			if (m_bustedLocalTimers.contains(uid)) {
				delete m_bustedLocalTimers.take(uid);
			}
		}
	}
}

bool GameHandler::checkLocalGameOver()
{
	// Only in a local game: in a network game the server decides about
	// the end of the game and the hand start.
	if (!m_session || m_session->isNetworkClientRunning()) return false;
	if (m_localGameExitRequested) return true;   // The game has already been left
	if (m_localGameOver) return true;            // The winner has already been reported

	boost::shared_ptr<Game> game = m_session->getCurrentGame();
	if (!game) return false;
	boost::shared_ptr<HandInterface> hand = game->getCurrentHand();
	if (!hand) return false;

	// The end of the tournament exactly as in the Qt widgets client (gameTableImpl::postRiverRun-
	// Animation6): if only ONE player has chips after the pot distribution, the
	// game is over. Without this check the client keeps starting new
	// hands – the winner plays against themselves endlessly and pays blinds
	// into a pot that they win back immediately.
	boost::shared_ptr<PlayerInterface> winner;
	int playersWithCash = 0;
	PlayerList activePlayers = hand->getActivePlayerList();
	for (auto it = activePlayers->begin(); it != activePlayers->end(); ++it) {
		if ((*it)->getMyCash() > 0) {
			playersWithCash++;
			winner = *it;
		}
	}
	if (playersWithCash > 1) return false;

	m_localGameOver = true;

	// A log entry as in the widgets client ("<name> wins game <n>!").
	if (winner)
		appendGameLog(QString::fromStdString(winner->getMyName())
					  + QStringLiteral(" wins game ")
					  + QString::number(game->getMyGameID())
					  + QStringLiteral("!"),
					  LogGameWin);

	// No turn is possible any more – shut the action bar down.
	closeMyTurnWindow();
	if (m_myTurn) {
		m_myTurn = false;
		emit myTurnChanged();
	}
	if (m_timeoutSeatId != -1) {
		m_timeoutSeatId = -1;
		emit timeoutChanged();
	}
	// Do not hide players who dropped out any more: the final standings should stay
	// while the winner message is open.
	qDeleteAll(m_bustedLocalTimers);
	m_bustedLocalTimers.clear();

	emit localGameFinished(winner ? QString::fromStdString(winner->getMyName()) : QString(),
						   winner ? winner->getMyID() : -1);
	return true;
}

void GameHandler::onBlindsSet(int smallBlind)
{
	if (localGameCallbacksBlocked()) return;
	if (m_soundEventHandler)
		m_soundEventHandler->blindsWereSet(smallBlind);
}

void GameHandler::refreshBoardCards()
{
	if (!m_game) return;
	auto hand = m_game->getCurrentHand();
	if (!hand) return;
	auto board = hand->getBoard();
	if (!board) return;

	int raw[5] = {-1, -1, -1, -1, -1};
	board->getMyCards(raw);

	QVariantList newCards;
	for (int i = 0; i < 5; ++i)
		newCards.append(i < m_boardCardCount ? raw[i] : -1);

	if (newCards != m_boardCards) {
		m_boardCards = newCards;
		emit boardCardsChanged();
	}
	refreshChanceAndHand();
}

void GameHandler::refreshChanceAndHand()
{
	// Look for your own player (seat 0). As a spectator there is none – then
	// the odds/hand display stays empty (haveCards == false).
	boost::shared_ptr<PlayerInterface> human;
	if (m_game && !m_spectating) {
		PlayerList seats = m_game->getSeatsList();
		if (seats) {
			for (auto it = seats->begin(); it != seats->end(); ++it) {
				if ((*it)->getMyID() == 0) {
					human = *it;
					break;
				}
			}
		}
	}

	int holeCards[2] = {-1, -1};
	int boardCards[5] = {-1, -1, -1, -1, -1};
	bool folded = false;
	bool haveCards = false;

	if (human && human->getMyActiveStatus()) {
		human->getMyCards(holeCards);
		haveCards = (holeCards[0] >= 0 && holeCards[1] >= 0);
		if (haveCards) {
			auto hand = m_game ? m_game->getCurrentHand() : nullptr;
			if (hand && hand->getBoard())
				hand->getBoard()->getMyCards(boardCards);
			folded = (human->getMyAction() == PLAYER_ACTION_FOLD);
		}
	}

	// The input signature: only recompute when the cards/board/fold have changed.
	std::vector<int> inputs;
	inputs.reserve(9);
	inputs.push_back(haveCards ? holeCards[0] : -1);
	inputs.push_back(haveCards ? holeCards[1] : -1);
	inputs.push_back(m_boardCardCount);
	for (int i = 0; i < 5; ++i)
		inputs.push_back(haveCards ? boardCards[i] : -1);
	inputs.push_back(folded ? 1 : 0);
	if (inputs == m_lastChanceInputs)
		return;
	m_lastChanceInputs = inputs;

	QVariantList newChance;

	if (haveCards) {
		GameState gs = GAME_STATE_PREFLOP;
		if (m_boardCardCount >= 5)      gs = GAME_STATE_RIVER;
		else if (m_boardCardCount == 4) gs = GAME_STATE_TURN;
		else if (m_boardCardCount == 3) gs = GAME_STATE_FLOP;

		std::vector<std::vector<int>> chance =
									   CardsValue::calcCardsChance(gs, holeCards, boardCards);
		if (chance.size() >= 2 && chance[0].size() >= 10 && chance[1].size() >= 10) {
			for (int cat = 0; cat < 10; ++cat) {
				QVariantMap m;
				m["prob"]     = chance[0][cat];
				m["possible"] = (chance[1][cat] != 0) && !folded;
				newChance.append(m);
			}
		}
	}

	if (newChance != m_cardsChance || folded != m_cardsChanceFolded) {
		m_cardsChance = newChance;
		m_cardsChanceFolded = folded;
		emit cardsChanceChanged();
	}
}

void GameHandler::onNextRoundCleanGui()
{
	if (localGameCallbacksBlocked()) return;
	onDisableMyButtons();
	m_pot = 0;
	m_totalPot = 0;
	emit potChanged();
	emit totalPotChanged();
	m_minRaiseAmount = 0;
	m_maxRaiseAmount = 0;
	emit minRaiseAmountChanged();
	emit maxRaiseAmountChanged();
	m_boardCardCount = 0;
	m_boardCards = QVariantList{-1, -1, -1, -1, -1};
	emit boardCardCountChanged();
	emit boardCardsChanged();
	if (!m_winnerSeatIds.isEmpty()) {
		m_winnerSeatIds.clear();
		emit winnerSeatIdsChanged();
	}
	if (!m_winningHandText.isEmpty()) {
		m_winningHandText.clear();
		emit winningHandTextChanged();
	}
	// Reset the showdown spotlight for the new hand (the hole and board fade).
	m_holeFade0.clear();
	m_holeFade1.clear();
	const QVariantList noBoardFade = {false, false, false, false, false};
	if (m_boardCardFade != noBoardFade) {
		m_boardCardFade = noBoardFade;
		emit boardCardFadeChanged();
	}
	// End the showdown before the player data is rebuilt → the cards close.
	setShowdownActive(false);
	m_allInRevealed = false;
	m_postRiverShownPlayers.clear();
	// Discard the showdown snapshot of the previous hand → the live state applies again from now on.
	m_foldedAtHandEnd.clear();
	m_needToShowAtHandEnd.clear();
	m_showdownSnapshotValid = false;
	if (m_canShowCards) {
		m_canShowCards = false;
		emit canShowCardsChanged();
	}
	refreshPlayerData();
	// Refresh the button state: at the end/start of a hand getMyAction() is still
	// the last action (!= NONE) → the buttons are inactive until the engine resets it to
	// NONE for the new hand.
	computeCallAndRaiseAmounts();
}

void GameHandler::onDealFlopCards()
{
	if (localGameCallbacksBlocked()) return;
	m_boardCardCount = 3;
	emit boardCardCountChanged();
	refreshBoardCards();
}

void GameHandler::onDealTurnCard()
{
	if (localGameCallbacksBlocked()) return;
	m_boardCardCount = 4;
	emit boardCardCountChanged();
	refreshBoardCards();
}

void GameHandler::onDealRiverCard()
{
	if (localGameCallbacksBlocked()) return;
	m_boardCardCount = 5;
	emit boardCardCountChanged();
	refreshBoardCards();
}

// ─── Q_INVOKABLE actions called from QML ────────────────────────────────────

void GameHandler::fold()
{
	qDebug() << "[FOLDDBG] fold() entry"
			 << "myTurn=" << m_myTurn << "tSeat=" << m_timeoutSeatId
			 << "isMyTurnToAct=" << isMyTurnToAct();
	if (!m_game || !m_session || !isMyTurnToAct()) {
		qDebug() << "[FOLDDBG] fold() EARLY-RETURN";
		return;
	}

	auto hand = m_game->getCurrentHand();
	if (!hand) return;
	auto seats = hand->getSeatsList();
	if (!seats || seats->empty()) return;
	auto humanPlayer = seats->front();

	qDebug() << "[FOLDDBG] fold() pre-dispatch"
			 << "myButton=" << humanPlayer->getMyButton()
			 << "round=" << (int)hand->getCurrentRound();

	// [RACEDBG] These mutate the shared Hand on the GUI thread. If a net-thread
	// "[RACEDBG] net: ... BEGIN/END" line appears interleaved with this one in
	// pokerth-debug.log (different thread id), the GUI- and network-thread are
	// touching the same Hand concurrently -> the suspected auto-fold freeze.
	qDebug() << "[RACEDBG] gui: fold() mutating Hand BEGIN";
	humanPlayer->setMyAction(PLAYER_ACTION_FOLD, true);
	humanPlayer->setMyTurn(false);
	hand->setPreviousPlayerID(0);
	qDebug() << "[RACEDBG] gui: fold() mutating Hand END";

	doActionDone();
}

bool GameHandler::call(int expectedAmount)
{
	qDebug() << "[CALLDBG] call() entry expected=" << expectedAmount
			 << "myTurn=" << m_myTurn << "tSeat=" << m_timeoutSeatId
			 << "isMyTurnToAct=" << isMyTurnToAct();
	if (!m_game || !m_session || !isMyTurnToAct()) {
		qDebug() << "[CALLDBG] call() EARLY-RETURN (game=" << (m_game ? 1 : 0)
				 << " session=" << (m_session ? 1 : 0)
				 << " turn=" << isMyTurnToAct() << ")";
		return false;
	}

	auto hand = m_game->getCurrentHand();
	if (!hand) return false;
	auto seats = hand->getSeatsList();
	if (!seats || seats->empty()) return false;
	auto humanPlayer = seats->front();
	auto bero = hand->getCurrentBeRo();
	if (!bero) return false;

	int highestSet = bero->getHighestSet();
	int humanSet = humanPlayer->getMySet();

	// A coverage check against the amount the player saw (0 = check).
	// The live state may have been raised by the network thread since the click/the
	// preselection; then send NOTHING (see the comment at the declaration).
	// Less than expected is harmless (never more chips than intended) and is
	// executed; −1 means "any amount" (auto check/call).
	const int requiredAmount = (humanPlayer->getMyCash() + humanSet <= highestSet)
							   ? humanPlayer->getMyCash()
							   : std::max(0, highestSet - humanSet);
	if (expectedAmount >= 0 && requiredAmount > expectedAmount) {
		qDebug() << "[CALLDBG] call() REJECTED: required=" << requiredAmount
				 << "> expected=" << expectedAmount
				 << "(highestSet=" << highestSet << "mySet=" << humanSet << ")";
		emit actionRejected(requiredAmount, expectedAmount);
		return false;
	}

	qDebug() << "[CALLDBG] call() pre-dispatch"
			 << "humanSet=" << humanSet << "highestSet=" << highestSet
			 << "humanCash=" << humanPlayer->getMyCash()
			 << "myButton=" << humanPlayer->getMyButton()
			 << "(1=D,2=SB,3=BB)"
			 << "round=" << (int)hand->getCurrentRound()
			 << "myAction=" << humanPlayer->getMyAction();

	if (highestSet == 0 || humanSet >= highestSet) {
		// A check – either no bet has been made (highestSet == 0) OR our
		// own bet already corresponds to the highest one (the classic case:
		// the BB option preflop, everybody has limped). The server expects
		// PLAYER_ACTION_CHECK explicitly here; a CALL without an actual chip
		// movement would be discarded → a timeout with the default action.
		humanPlayer->setMyAction(PLAYER_ACTION_CHECK, true);
		qDebug() << "[CALLDBG] call() -> CHECK branch";
	} else if (humanPlayer->getMyCash() + humanSet <= highestSet) {
		// All-in call
		humanPlayer->setMySet(humanPlayer->getMyCash());
		humanPlayer->setMyCash(0);
		humanPlayer->setMyAction(PLAYER_ACTION_ALLIN, true);
		qDebug() << "[CALLDBG] call() -> ALLIN branch lastRelSet=" << humanPlayer->getMyLastRelativeSet();
	} else {
		// Regular call
		humanPlayer->setMySet(highestSet - humanSet);
		humanPlayer->setMyAction(PLAYER_ACTION_CALL, true);
		qDebug() << "[CALLDBG] call() -> CALL branch lastRelSet=" << humanPlayer->getMyLastRelativeSet()
				 << "newMySet=" << humanPlayer->getMySet();
	}

	humanPlayer->setMyTurn(false);
	hand->getBoard()->collectSets();
	hand->setPreviousPlayerID(0);

	doActionDone();
	onRefreshPot();
	return true;
}

void GameHandler::raise(int amount)
{
	qDebug() << "[RAISEDBG] raise() entry amount=" << amount
			 << "myTurn=" << m_myTurn << "tSeat=" << m_timeoutSeatId
			 << "isMyTurnToAct=" << isMyTurnToAct();
	if (!m_game || !m_session || !isMyTurnToAct()) {
		qDebug() << "[RAISEDBG] raise() EARLY-RETURN";
		return;
	}

	auto hand = m_game->getCurrentHand();
	if (!hand) return;
	auto seats = hand->getSeatsList();
	if (!seats || seats->empty()) return;
	auto humanPlayer = seats->front();
	auto bero = hand->getCurrentBeRo();
	if (!bero) return;

	// Default to minimum raise if no amount specified
	if (amount <= 0) {
		amount = m_minRaiseAmount;
	}
	if (amount <= 0) {
		qDebug() << "[RAISEDBG] raise() amount<=0 after minRaise fallback (m_minRaiseAmount=" << m_minRaiseAmount << ") ABORT";
		return;
	}
	qDebug() << "[RAISEDBG] raise() pre-dispatch amount=" << amount
			 << "humanSet=" << humanPlayer->getMySet()
			 << "humanCash=" << humanPlayer->getMyCash()
			 << "highestSet=" << bero->getHighestSet()
			 << "minRaiseEngine=" << bero->getMinimumRaise()
			 << "myButton=" << humanPlayer->getMyButton()
			 << "round=" << (int)hand->getCurrentRound();

	int tempCash = humanPlayer->getMyCash();

	humanPlayer->setMySet(amount); // adds to set, deducts from cash

	if (amount >= tempCash) {
		// All-in
		humanPlayer->setMyCash(0);
		humanPlayer->setMyAction(PLAYER_ACTION_ALLIN, true);
		if (bero->getHighestSet() + bero->getMinimumRaise() > humanPlayer->getMySet()) {
			bero->setFullBetRule(true);
		}
		if (humanPlayer->getMySet() > bero->getHighestSet()) {
			bero->setMinimumRaise(humanPlayer->getMySet() - bero->getHighestSet());
			bero->setHighestSet(humanPlayer->getMySet());
			hand->setLastActionPlayerID(humanPlayer->getMyUniqueID());
		}
	} else {
		const bool firstBet = (bero->getHighestSet() == 0);
		humanPlayer->setMyAction(firstBet ? PLAYER_ACTION_BET : PLAYER_ACTION_RAISE, true);
		bero->setMinimumRaise(humanPlayer->getMySet() - bero->getHighestSet());
		bero->setHighestSet(humanPlayer->getMySet());
		hand->setLastActionPlayerID(humanPlayer->getMyUniqueID());
	}

	humanPlayer->setMyTurn(false);
	hand->getBoard()->collectSets();
	hand->setPreviousPlayerID(0);

	doActionDone();
	onRefreshPot();
}

void GameHandler::allIn()
{
	if (!m_game || !m_session || !isMyTurnToAct()) return;

	auto hand = m_game->getCurrentHand();
	if (!hand) return;
	auto seats = hand->getSeatsList();
	if (!seats || seats->empty()) return;
	auto humanPlayer = seats->front();
	auto bero = hand->getCurrentBeRo();
	if (!bero) return;

	humanPlayer->setMySet(humanPlayer->getMyCash());
	humanPlayer->setMyCash(0);
	humanPlayer->setMyAction(PLAYER_ACTION_ALLIN, true);

	if (bero->getHighestSet() + bero->getMinimumRaise() > humanPlayer->getMySet()) {
		bero->setFullBetRule(true);
	}
	if (humanPlayer->getMySet() > bero->getHighestSet()) {
		bero->setMinimumRaise(humanPlayer->getMySet() - bero->getHighestSet());
		bero->setHighestSet(humanPlayer->getMySet());
		hand->setLastActionPlayerID(humanPlayer->getMyUniqueID());
	}

	humanPlayer->setMyTurn(false);
	hand->getBoard()->collectSets();
	hand->setPreviousPlayerID(0);

	doActionDone();
	onRefreshPot();
}

void GameHandler::showMyCards()
{
	if (!m_canShowCards) return;
	if (m_session) m_session->showMyCards();
	m_canShowCards = false;
	emit canShowCardsChanged();
	// A visual confirmation in the self box: "turn" your own cards (like the
	// widget client via showHoleCards), so that you see that you really are showing.
	emit myCardsShown();
}

// ─── Local game startup ──────────────────────────────────────────────────────

void GameHandler::startLocalGame()
{
	if (!m_session) return;
	m_localGameExitRequested = false;
	m_localGameOver = false;

	// All values come from the configuration (settings → local game),
	// exactly as in the widget client (startWindowImpl::startNewLocalGame without a dialog).
	GameData gameData;
	if (m_config) {
		gameData.maxNumberOfPlayers = m_config->readConfigInt("NumberOfPlayers");
		gameData.startMoney         = m_config->readConfigInt("StartCash");
		gameData.firstSmallBlind    = m_config->readConfigInt("FirstSmallBlind");

		if (m_config->readConfigInt("RaiseBlindsAtHands")) {
			gameData.raiseIntervalMode              = RAISE_ON_HANDNUMBER;
			gameData.raiseSmallBlindEveryHandsValue = m_config->readConfigInt("RaiseSmallBlindEveryHands");
		} else {
			gameData.raiseIntervalMode                = RAISE_ON_MINUTES;
			gameData.raiseSmallBlindEveryMinutesValue = m_config->readConfigInt("RaiseSmallBlindEveryMinutes");
		}

		if (m_config->readConfigInt("AlwaysDoubleBlinds")) {
			gameData.raiseMode = DOUBLE_BLINDS;
		} else {
			gameData.raiseMode        = MANUAL_BLINDS_ORDER;
			gameData.manualBlindsList = m_config->readConfigIntList("ManualBlindsList");

			if (m_config->readConfigInt("AfterMBAlwaysDoubleBlinds")) {
				gameData.afterManualBlindsMode = AFTERMB_DOUBLE_BLINDS;
			} else if (m_config->readConfigInt("AfterMBAlwaysRaiseAbout")) {
				gameData.afterManualBlindsMode = AFTERMB_RAISE_ABOUT;
				gameData.afterMBAlwaysRaiseValue = m_config->readConfigInt("AfterMBAlwaysRaiseValue");
			} else {
				gameData.afterManualBlindsMode = AFTERMB_STAY_AT_LAST_BLIND;
			}
		}

		gameData.guiSpeed = m_config->readConfigInt("GameSpeed");
	}
	if (gameData.maxNumberOfPlayers < 2) gameData.maxNumberOfPlayers = 6;
	if (gameData.startMoney <= 0)        gameData.startMoney         = 1500;
	if (gameData.firstSmallBlind <= 0)   gameData.firstSmallBlind    = 10;
	if (gameData.raiseSmallBlindEveryHandsValue   < 1) gameData.raiseSmallBlindEveryHandsValue   = 8;
	if (gameData.raiseSmallBlindEveryMinutesValue < 1) gameData.raiseSmallBlindEveryMinutesValue = 5;
	if (gameData.guiSpeed < 1 || gameData.guiSpeed > 11) gameData.guiSpeed = 4;

	gameData.delayBetweenHandsSec           = 7;
	gameData.playerActionTimeoutSec         = 20;

	StartData startData;
	startData.numberOfPlayers     = gameData.maxNumberOfPlayers;
	startData.startDealerPlayerId = 0;

	m_session->startLocalGame(gameData, startData);

	// Sync m_game so action methods and refreshes work immediately
	auto game = m_session->getCurrentGame();
	if (game) setGame(game);
}

void GameHandler::endLocalGame()
{
	if (!m_session) return;
	if (m_session->isNetworkClientRunning()) return;

	m_localGameExitRequested = true;
	m_game.reset();

	// Ausstehende Busted-Player-Timer abbrechen.
	qDeleteAll(m_bustedLocalTimers);
	m_bustedLocalTimers.clear();

	if (m_myTurn) {
		m_myTurn = false;
		emit myTurnChanged();
	}

	refreshPlayerData();
}

bool GameHandler::isLocalGameRunning() const
{
	if (m_localGameExitRequested) return false;
	if (!m_session) return false;
	if (m_session->isNetworkClientRunning()) return false;
	if (m_game) return true;
	return static_cast<bool>(m_session->getCurrentGame());
}

bool GameHandler::isInternetGameRunning() const
{
	return m_session
		   && m_session->isNetworkClientRunning()
		   && m_session->getGameType() == Session::GAME_TYPE_INTERNET;
}

void GameHandler::reportAvatar(int seatId)
{
	// Only meaningful in an internet game (as in the Qt widgets client, MyAvatarLabel).
	if (!isInternetGameRunning() || !m_game)
		return;

	PlayerList seats = m_game->getSeatsList();
	for (auto it = seats->begin(); it != seats->end(); ++it) {
		if ((*it)->getMyID() != seatId)
			continue;
		// The avatar hash = the base name of the avatar file (without the path/extension), exactly as
		// MyAvatarLabel::reportBadAvatar passes the value to the server.
		const std::string avatar = (*it)->getMyAvatar();
		if (avatar.empty())
			return;
		const QFileInfo fi(QString::fromStdString(avatar));
		m_session->reportBadAvatar((*it)->getMyUniqueID(),
								   fi.baseName().toStdString());
		return;
	}
}

// ─── Game-loop advance slots (called via QMetaObject from QmlGuiInterface) ───

void GameHandler::onRunBeRo()
{
	if (localGameCallbacksBlocked()) return;
	if (!m_game) return;
	auto hand = m_game->getCurrentHand();
	if (hand && hand->getCurrentBeRo())
		hand->getCurrentBeRo()->run();
}

void GameHandler::onNextPlayerBeRo()
{
	if (localGameCallbacksBlocked()) return;
	if (!m_game) return;
	auto hand = m_game->getCurrentHand();
	if (hand && hand->getCurrentBeRo())
		hand->getCurrentBeRo()->nextPlayer();
}

// Called after the board cards of a street have been dealt. In an all-in
// condition there is no more betting (the running-player list is empty), so
// calling BeRo::run() would throw ERR_RUNNING_PLAYER_NOT_FOUND. Instead we
// advance straight to the next street/showdown via switchRounds(). In a normal
// (non-all-in) round we start the betting via BeRo::run().
void GameHandler::onAfterDealCards()
{
	if (localGameCallbacksBlocked()) return;
	if (!m_game) return;
	auto hand = m_game->getCurrentHand();
	if (!hand) return;

	if (hand->getAllInCondition()) {
		hand->switchRounds();
	} else if (hand->getCurrentBeRo()) {
		hand->getCurrentBeRo()->run();
	}
}

void GameHandler::onSwitchRounds()
{
	if (localGameCallbacksBlocked()) return;
	if (!m_game) return;
	auto hand = m_game->getCurrentHand();
	if (hand) hand->switchRounds();
}

void GameHandler::onPostRiverRunBeRo()
{
	if (localGameCallbacksBlocked()) return;
	if (!m_game) return;
	auto hand = m_game->getCurrentHand();
	if (hand && hand->getCurrentBeRo())
		hand->getCurrentBeRo()->postRiverRun();
}

// ─────────────────────────────────────────────────────────────────────────────
// The showdown snapshot
//
// The showdown must NOT read the fold state live from the player objects:
//
//   * Game::initHand() sets the action of ALL players to
//     PLAYER_ACTION_NONE at the beginning of EVERY hand (game.cpp) – including
//     PLAYER_ACTION_FOLD. (The network client protects FOLD from the round reset
//     deliberately in ClientStateRunHand::ResetPlayerActions(); initHand() deliberately does not.)
//   * The widgets client can rely on getMyAction() nevertheless, because it
//     blocks the network thread: waitForGuiUpdateDone() waits there for a
//     semaphore that prepareForNewHand() only releases after the end of the post-river animation
//     (gametableimpl.cpp). So initHand() runs later for sure.
//   * In the QML client waitForGuiUpdateDone() is a no-op (qmlguiinterface.h) and
//     onShowdown() runs via a QueuedConnection. In a network game the
//     server starts the next hand – so initHand() may long since have cleared
//     the FOLD flags before onShowdown() gets its turn.
//
// The consequence without a snapshot: in the showdown players who folded count as active. Whoever
// knows their cards (in a network game at least you yourself) ends up in the game history
// with a rated hand name – even if that hand beats the real
// winner. To the player that looks like a wrong winner.
//
// Therefore: freeze the state at the end of the hand (synchronously on the network thread, see
// QmlGuiInterface::postRiverRunAnimation1) and read only the snapshot from then on.
void GameHandler::captureShowdownSnapshot()
{
	m_foldedAtHandEnd.clear();
	m_needToShowAtHandEnd.clear();
	m_showdownSnapshotValid = false;
	if (!m_game) return;
	auto hand = m_game->getCurrentHand();
	if (!hand) return;
	auto activeList = hand->getActivePlayerList();
	if (!activeList) return;

	for (auto it = activeList->begin(); it != activeList->end(); ++it) {
		const unsigned uid = (*it)->getMyUniqueID();
		if ((*it)->getMyAction() == PLAYER_ACTION_FOLD)
			m_foldedAtHandEnd.insert(uid);
		// checkIfINeedToShowCards() reads board->getPlayerNeedToShowCards() –
		// the next hand overwrites the same list as well.
		if ((*it)->checkIfINeedToShowCards())
			m_needToShowAtHandEnd.insert(uid);
	}
	m_showdownSnapshotValid = true;
}

bool GameHandler::showdownFolded(unsigned uniqueId, bool liveFolded) const
{
	return m_showdownSnapshotValid ? m_foldedAtHandEnd.contains(uniqueId) : liveFolded;
}

bool GameHandler::showdownNeedsToShow(unsigned uniqueId, bool liveNeedsToShow) const
{
	return m_showdownSnapshotValid ? m_needToShowAtHandEnd.contains(uniqueId) : liveNeedsToShow;
}

void GameHandler::onShowdown()
{
	if (localGameCallbacksBlocked()) return;
	if (!m_game) return;
	auto hand = m_game->getCurrentHand();
	if (!hand) return;
	auto board = hand->getBoard();
	if (!board) return;

	// The showdown is active now → the opponents' cards may be revealed
	// (determinePlayerNeedToShowCards() was already called in postRiverRun()).
	setShowdownActive(true);
	// During the showdown nobody is to act. An m_myTurn that may still be set
	// (e.g. when the turn was active via the action timer) would otherwise keep
	// the action buttons in the QML action bar "armed"
	// (armed = myTurn || …) – the buttons would stay clickable in the showdown.
	if (m_myTurn) {
		m_myTurn = false;
		emit myTurnChanged();
	}
	closeMyTurnWindow();   // in the showdown nobody waits for me any more
	refreshPlayerData();
	computeCallAndRaiseAmounts(); // Buttons sofort deaktivieren

	// The pot is already distributed. Determine the winners and separate the main/side pot exactly
	// like the widgets client (gameTableImpl::postRiverRunAnimation3).
	std::list<unsigned> winners = board->getWinners();
	auto activeList = hand->getActivePlayerList();
	auto bero = hand->getCurrentBeRo();

	// The side pot criterion exactly as the engine itself (Log::logHandWinner): a main pot
	// winner = a money winner with the best showdown hand (cardsValueInt >=
	// highestCardsValue, which is determined over all players who have not folded). Money
	// winners with a worse hand have only won a side pot and get
	// NO winner badge. Deliberately WITHOUT a getAllInCondition() gate: on the network client
	// allInCondition is only set with a complete all-in showdown (via an
	// AllInShowCardsMessage) and would not recognise a side pot without a global all-in – a short
	// stack all-in, the others keep betting normally until the showdown.
	const int highestWinnerCardsValue = bero ? bero->getHighestCardsValue() : 0;
	auto isMainPotWinner = [&](const auto &p) -> bool {
		const bool isW = std::find(winners.begin(), winners.end(), p->getMyUniqueID()) != winners.end();
		const bool hasActuallyWon = isW && p->getLastMoneyWon() > 0;
		if (showdownFolded(p->getMyUniqueID(), p->getMyAction() == PLAYER_ACTION_FOLD) || !hasActuallyWon)
			return false;
		if (highestWinnerCardsValue > 0 && p->getMyCardsValueInt() < highestWinnerCardsValue)
			return false; // Side-Pot-Gewinner
		return true;
	};

	// The winner badge: every MAIN POT winner who has not folded (several with a split
	// pot). Side pot winners get NO badge – as in the widgets client, which
	// sets the "winner" label only for isMainPot winners.
	QVariantList newWinners;
	if (activeList) {
		for (auto it = activeList->begin(); it != activeList->end(); ++it)
			if (isMainPotWinner(*it))
				newWinners.append((*it)->getMyID());
	}
	if (newWinners != m_winnerSeatIds) {
		m_winnerSeatIds = newWinners;
		emit winnerSeatIdsChanged();
	}

	// Determine the name of the winning hand (like label_WinningCombination in the widgets
	// client). Only meaningful if there is a real showdown (more than one
	// player who has not folded) – otherwise the text stays empty.
	QString newHandText;
	int nonFold = 0;
	if (activeList) {
		for (auto it = activeList->begin(); it != activeList->end(); ++it)
			if (!showdownFolded((*it)->getMyUniqueID(), (*it)->getMyAction() == PLAYER_ACTION_FOLD)) ++nonFold;
	}
	if (activeList && bero && nonFold > 1) {
		std::string name = CardsValue::determineHandName(bero->getHighestCardsValue(), activeList);
		newHandText = QString::fromStdString(name);
	}

	if (newHandText != m_winningHandText) {
		m_winningHandText = newHandText;
		emit winningHandTextChanged();
	}

	// ── The showdown spotlight: dim the cards aside from the winning hand ──────────
	// As in the widgets client (gameTableImpl::postRiverRunAnimation3): for every
	// main pot winner getMyBestHandPosition delivers the 5 positions of their
	// best hand – 0/1 are their hole cards, 2..6 the board cards 0..4.
	// All the other cards are dimmed to 25 % opacity in the showdown
	// (the actual fading is done by QML, depending on ShowFadeOutCardsAnimation).
	// A board card stays bright as soon as it counts towards ANY winning hand
	// (cleaner with split pots than the per-winner fading of the widgets client).
	QSet<int> newHoleFade0, newHoleFade1;
	QVariantList newBoardFade = {false, false, false, false, false};
	if (nonFold > 1) {
		bool boardUsed[5] = {false, false, false, false, false};
		bool anyWinner = false;
		for (auto it = activeList->begin(); it != activeList->end(); ++it) {
			if (!isMainPotWinner(*it))
				continue;
			anyWinner = true;
			int pos[5];
			(*it)->getMyBestHandPosition(pos);
			bool useHole0 = false, useHole1 = false;
			for (int j = 0; j < 5; ++j) {
				const int p = pos[j];
				if (p == 0)               useHole0 = true;
				else if (p == 1)          useHole1 = true;
				else if (p >= 2 && p <= 6) boardUsed[p - 2] = true;
			}
			const int id = (*it)->getMyID();
			if (!useHole0) newHoleFade0.insert(id);
			if (!useHole1) newHoleFade1.insert(id);
		}
		if (anyWinner) {
			for (int i = 0; i < 5; ++i)
				newBoardFade[i] = !boardUsed[i];
		}
	}
	if (newHoleFade0 != m_holeFade0 || newHoleFade1 != m_holeFade1) {
		m_holeFade0 = newHoleFade0;
		m_holeFade1 = newHoleFade1;
		refreshPlayerData(); // it pushes fade0/fade1 into the seat data
	}
	if (newBoardFade != m_boardCardFade) {
		m_boardCardFade = newBoardFade;
		emit boardCardFadeChanged();
	}

	// ── Log the showdown in the game history (the logic 1:1 from the widgets client) ──
	// In the PokerTH client the engine calls neither logFlipHoleCardsMsg nor
	// logPlayerWinsMsg by itself – in the Qt widgets client the GUI does that
	// (gameTableImpl::postRiverRunAnimation2/3). So it is reproduced here, otherwise
	// the revealed cards and the winner are missing from the "game history" overlay.
	if (!activeList) return;

	// 1) The revealed hole cards of the players who have to show according to the engine
	//    (like showHoleCards → setMyCardsFlip(1,1) for the post-river round:
	//    "name shows [c0, c1] - \"hand name\"").
	//    ONLY with a real showdown (nonFold > 1). If all but one have folded,
	//    there is nothing to reveal – the server then sends EndOfHandHideCards
	//    (without a cardsvalue) and does not call determinePlayerNeedToShowCards() at all,
	//    i.e. board->getPlayerNeedToShowCards() still holds the list of the
	//    last showdown hand (the board lives per game, not per hand). Without
	//    this gate the winner logged a "shows [...]" although they showed
	//    nothing – followed by the real second "shows" when they press the show
	//    button. The widgets client gates it the same way
	//    (gameTableImpl::postRiverRunAnimation2: nonfoldPlayersCounter != 1).
	if (nonFold > 1) {
		for (auto it = activeList->begin(); it != activeList->end(); ++it) {
			const unsigned uid = (*it)->getMyUniqueID();
			if (showdownFolded(uid, (*it)->getMyAction() == PLAYER_ACTION_FOLD)
					|| !showdownNeedsToShow(uid, (*it)->checkIfINeedToShowCards()))
				continue;
			int cards[2] = {-1, -1};
			(*it)->getMyCards(cards);
			if (cards[0] < 0 || cards[1] < 0)
				continue;
			QString line = QString::fromStdString((*it)->getMyName())
						   + " shows [" + logCard(cards[0]) + ", " + logCard(cards[1]) + "]";
			// 0 = "no hand value" (clientstate.cpp/ClientHand set it to 0 at the start of a
			// hand, the server only sends it along in the showdown). Otherwise determineHandName(0)
			// invents a hand ("High Card, Deuces") – the engine checks for > 0 at
			// the same place (Log::logHoleCardsHandName).
			const int cardsValueInt = (*it)->getMyCardsValueInt();
			if (cardsValueInt > 0) {
				std::string handName = CardsValue::determineHandName(cardsValueInt, activeList);
				if (!handName.empty())
					line += " - \"" + QString::fromStdString(handName) + "\"";
			}
			appendGameLog(line);
		}
	}

	// 2) The winners – the main/side pot via isMainPotWinner (highestWinnerCardsValue
	//    computed above). Real winners are in the winners list AND have
	//    actually won money.
	for (auto it = activeList->begin(); it != activeList->end(); ++it) {
		const bool isWinner = std::find(winners.begin(), winners.end(), (*it)->getMyUniqueID()) != winners.end();
		const bool hasActuallyWon = isWinner && (*it)->getLastMoneyWon() > 0;
		if (showdownFolded((*it)->getMyUniqueID(), (*it)->getMyAction() == PLAYER_ACTION_FOLD) || !hasActuallyWon)
			continue;
		// With an all-in with several winners: the best hand = the main pot, the rest a side pot.
		const bool isMainPot = isMainPotWinner(*it);
		QString msg = QString::fromStdString((*it)->getMyName())
					  + " wins $" + QString::number((*it)->getLastMoneyWon());
		if (!isMainPot)
			msg += QStringLiteral(" (side pot)");
		appendGameLog(msg, isMainPot ? LogWinnerMain : LogWinnerSide);
	}

	// 3) A sit-out for players without cash (as gameTableImpl does after the pot distribution).
	for (auto it = activeList->begin(); it != activeList->end(); ++it) {
		if ((*it)->getMyCash() == 0)
			appendGameLog(QString::fromStdString((*it)->getMyName()) + " sits out", LogSitOut);
	}

	// 4) The "show" button: the human player (seat 0) can show their cards voluntarily
	//    if they have not folded and do not HAVE to show (the logic 1:1 from the
	//    Qt widgets client, gameTableImpl::postRiverRunAnimation2).
	// Spectators have no seat and thus no cards to show.
	bool newCanShow = false;
	auto seatsList = hand->getSeatsList();
	if (!m_spectating && seatsList && !seatsList->empty()) {
		auto humanPlayer = seatsList->front(); // seat 0
		const unsigned humanUid = humanPlayer->getMyUniqueID();
		if (humanPlayer->getMyActiveStatus()
				&& !showdownFolded(humanUid, humanPlayer->getMyAction() == PLAYER_ACTION_FOLD)) {
			if (nonFold == 1) {
				// Won without a showdown – they can show
				newCanShow = true;
			} else if (nonFold > 1
					   && !showdownNeedsToShow(humanUid, humanPlayer->checkIfINeedToShowCards())) {
				// Several active players, but the human does not have to show – they can show voluntarily
				newCanShow = true;
			}
		}
	}
	if (newCanShow != m_canShowCards) {
		m_canShowCards = newCanShow;
		emit canShowCardsChanged();
	}
}

void GameHandler::onFlipHolecardsAllIn()
{
	// Reveal the cards of all players who have not folded (the all-in runout).
	// The engine has already called setMyCards() for all all-in players
	// (clientstate.cpp: the AllInShowCardsMessage handler).
	qDebug() << "[ALLIN] onFlipHolecardsAllIn() blocked=" << localGameCallbacksBlocked()
			 << "hasGame=" << (bool)m_game;
	if (localGameCallbacksBlocked()) return;
	if (!m_game) return;
	auto hand = m_game->getCurrentHand();
	if (!hand) return;

	// As in the Qt widgets client: only reveal if >= 2 players have not
	// folded (otherwise somebody has already won and there is nothing to show).
	auto active = hand->getActivePlayerList();
	if (!active) return;
	int nonFolded = 0;
	for (auto it = active->begin(); it != active->end(); ++it) {
		int c[2] = {-1, -1};
		(*it)->getMyCards(c);
		qDebug() << "[ALLIN]   active seatId=" << (*it)->getMyID()
				 << "action=" << (int)(*it)->getMyAction()
				 << "cards=" << c[0] << "/" << c[1];
		if ((*it)->getMyAction() != PLAYER_ACTION_FOLD) ++nonFolded;
	}
	qDebug() << "[ALLIN]   nonFolded=" << nonFolded;
	if (nonFolded < 2) {
		qDebug() << "[ALLIN]   GUARD: nonFolded<2 - aborting";
		return;
	}

	m_allInRevealed = true;
	qDebug() << "[ALLIN]   m_allInRevealed set to true, calling refreshPlayerData";
	refreshPlayerData();
}

void GameHandler::onPlayerShowCards(unsigned playerId)
{
	// A player has shown their cards voluntarily after the hand. The engine
	// has already updated setMyCards()/setMyCardsValueInt() in the AfterHandShowCardsMessage
	// handler (clientstate.cpp). In the widgets client
	// gameTableImpl::showHoleCards reveals the cards and logs the action via
	// setMyCardsFlip(1,1) → logFlipHoleCardsMsg. Both were missing in the QML client, because
	// SignalNetClientPostRiverShowCards was a no-op there.
	if (localGameCallbacksBlocked()) return;
	if (!m_game) return;
	auto hand = m_game->getCurrentHand();
	if (!hand) return;
	auto activeList = hand->getActivePlayerList();
	if (!activeList) return;

	for (auto it = activeList->begin(); it != activeList->end(); ++it) {
		if ((*it)->getMyUniqueID() != playerId)
			continue;
		// Players who have folded do not belong in the showdown – not even when
		// an AfterHandShowCardsMessage arrives for them. Otherwise their
		// hand would appear rated in the game history and could "beat" the real winner.
		// (The engine SQL log filters at the same place: Log::logHoleCardsHandName.)
		if (showdownFolded(playerId, (*it)->getMyAction() == PLAYER_ACTION_FOLD))
			return;
		int cards[2] = {-1, -1};
		(*it)->getMyCards(cards);
		if (cards[0] < 0 || cards[1] < 0)
			return;
		// Leave the cards revealed (until the next hand).
		m_postRiverShownPlayers.insert(playerId);
		// Log it in the game history: "name shows [c0, c1] - \"hand name\""
		// (like logFlipHoleCardsMsg with state=1 in the widgets client).
		QString line = QString::fromStdString((*it)->getMyName())
					   + " shows [" + logCard(cards[0]) + ", " + logCard(cards[1]) + "]";
		// Only append the hand name if the server has delivered a value
		// (AfterHandShowCardsMessage: "if (r.cardsvalue())"). 0 means "no value" –
		// determineHandName(0) would invent a hand ("High Card, Deuces").
		// AND only if all five board cards really lie: the engine
		// rolls the board and the hole cards completely at the hand start and computes
		// the 7 card value immediately (localhand.cpp); the server always sends it along since
		// the roundBeforePostRiver gate was dropped. If the winner shows their cards
		// voluntarily after a fold-out (preflop/flop/turn), the
		// hand name would be built from board cards that were never dealt – JJ would appear as
		// "Four of a Kind, Jacks". From GAME_STATE_RIVER on all board cards lie;
		// the widgets client gates it the same way at the same place
		// (gameTableImpl::showHoleCards: currentRound < GAME_STATE_RIVER ⇒
		// setMyCardsFlip(1,2), i.e. cards without a hand value).
		const int cardsValueInt = (*it)->getMyCardsValueInt();
		if (cardsValueInt > 0 && hand->getCurrentRound() >= GAME_STATE_RIVER) {
			std::string handName = CardsValue::determineHandName(cardsValueInt, activeList);
			if (!handName.empty())
				line += " - \"" + QString::fromStdString(handName) + "\"";
		}
		appendGameLog(line);
		refreshPlayerData();
		return;
	}
}
