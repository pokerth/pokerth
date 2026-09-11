/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2025 Felix Hammer, Florian Thauer, Lothar May          *
 *****************************************************************************/

#ifndef GAMEHANDLER_H
#define GAMEHANDLER_H

#include <QObject>
#include <QAbstractListModel>
#include <QVariantList>
#include <QStringList>
#include <QElapsedTimer>
#include <QSet>
#include <QPointer>
#include <vector>

#include "chatcolors.h"
#include <boost/shared_ptr.hpp>

class ConfigFile;
class Session;
class Game;
class SoundEvents;
class QTimer;
class ChatTranslator;
class StyleProvider;

// An incremental list model for the game history (log). Deliberately NOT a
// QStringList property: for QML a QStringList is a value type that is read
// completely anew on every new line → the bound ListView resets itself
// completely and rebuilds all (rich text) delegates. Since the history
// grows several times per second during a hand, that builds up into noticeable
// stuttering as the list gets longer. With beginInsertRows/beginRemoveRows
// the view only inserts the one new line (O(1)) instead of rebuilding.
class GameLogModel : public QAbstractListModel
{
	Q_OBJECT

	// The whole history as ONE rich text document (the lines concatenated with <br>) –
	// for the TextEdit based display (continuous selection + copying,
	// analogous to the ChatBox). It changes on every append()/clear().
	Q_PROPERTY(QString html READ html NOTIFY htmlChanged)

public:
	enum Roles { LineRole = Qt::UserRole + 1 };

	explicit GameLogModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}

	// Instead of finished hex values the stored lines carry only colour roles
	// (chatcolors.h). Only here – when delivering – are they filled with the colours
	// of the current table theme; a theme change thereby recolours the
	// history that is already there instead of leaving white text on a light
	// ground.
	QString html() const
	{
		return TableChatColors::expand(m_lines.join(QStringLiteral("<br>")), m_palette);
	}

	// Set the colour palette of the table theme; if it changes, the whole
	// document counts as new (htmlChanged + dataChanged over all lines).
	void setPalette(const TableChatColors::Palette &palette)
	{
		m_palette = palette;
		if (m_lines.isEmpty())
			return;
		emit dataChanged(index(0), index(m_lines.size() - 1), { LineRole });
		emit htmlChanged();
	}

	int rowCount(const QModelIndex &parent = QModelIndex()) const override
	{
		return parent.isValid() ? 0 : m_lines.size();
	}
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
	{
		if (index.row() < 0 || index.row() >= m_lines.size())
			return QVariant();
		if (role == LineRole || role == Qt::DisplayRole)
			return TableChatColors::expand(m_lines.at(index.row()), m_palette);
		return QVariant();
	}
	QHash<int, QByteArray> roleNames() const override
	{
		return { { LineRole, QByteArrayLiteral("line") } };
	}

	// Append a line and – as before – limit it to maxLines.
	void append(const QString &line, int maxLines)
	{
		beginInsertRows(QModelIndex(), m_lines.size(), m_lines.size());
		m_lines.append(line);
		endInsertRows();
		if (m_lines.size() > maxLines) {
			const int n = m_lines.size() - maxLines;
			beginRemoveRows(QModelIndex(), 0, n - 1);
			m_lines.erase(m_lines.begin(), m_lines.begin() + n);
			endRemoveRows();
		}
		emit htmlChanged();
	}
	void clear()
	{
		if (m_lines.isEmpty())
			return;
		beginResetModel();
		m_lines.clear();
		endResetModel();
		emit htmlChanged();
	}

signals:
	void htmlChanged();

private:
	QStringList m_lines;                 // raw, with colour role placeholders
	TableChatColors::Palette m_palette;  // the colours of the current table theme
};

class GameHandler : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QVariantList players READ players NOTIFY playersChanged)
	Q_PROPERTY(int pot READ pot NOTIFY potChanged)
	Q_PROPERTY(int gameId READ gameId NOTIFY gameIdChanged)
	Q_PROPERTY(QString phaseText READ phaseText NOTIFY phaseTextChanged)
	Q_PROPERTY(int handNumber READ handNumber NOTIFY handNumberChanged)
	Q_PROPERTY(bool myTurn READ myTurn NOTIFY myTurnChanged)
	Q_PROPERTY(bool canAct READ canAct NOTIFY canActChanged)
	// The authoritative "the server is waiting for my action NOW" (a network game).
	// The source is the engine itself: currentPlayersTurnId is set exclusively when
	// a PlayersTurnMessage arrives and only changes when another
	// player is to act. Unlike myTurn/timeoutSeatId, this
	// state can be cleared by NO trailing refresh callback (disableMyButtons,
	// stopTimeoutAnimation, a phase change, the showdown …) – that is exactly
	// what clicks used to die of, which silently degraded to a preselection and then
	// ran into the server timeout. After your own action it is false immediately
	// (m_myTurnWindowClosed), so that the buttons become inactive right away as before.
	Q_PROPERTY(bool awaitingMyAction READ awaitingMyAction NOTIFY awaitingMyActionChanged)
	Q_PROPERTY(int callAmount READ callAmount NOTIFY callAmountChanged)
	Q_PROPERTY(int minRaiseAmount READ minRaiseAmount NOTIFY minRaiseAmountChanged)
	Q_PROPERTY(int maxRaiseAmount READ maxRaiseAmount NOTIFY maxRaiseAmountChanged)
	Q_PROPERTY(int totalPot READ totalPot NOTIFY totalPotChanged)
	Q_PROPERTY(int boardCardCount READ boardCardCount NOTIFY boardCardCountChanged)
	Q_PROPERTY(QVariantList boardCards READ boardCards NOTIFY boardCardsChanged)
	// A list of all main pot winner seats (several with a split pot). The widgets
	// client shows the winner label on every player who has not folded and who has won
	// the main pot – side pot winners get NO badge.
	Q_PROPERTY(QVariantList winnerSeatIds READ winnerSeatIds NOTIFY winnerSeatIdsChanged)
	Q_PROPERTY(QString winningHandText READ winningHandText NOTIFY winningHandTextChanged)
	// Showdown spotlight: 5 bool values (one per board card). true = this board card
	// does NOT belong to the best hand of the winner and is dimmed at the
	// showdown (the setting "fade out animation for loser cards",
	// the config key ShowFadeOutCardsAnimation – like the widgets client, which fades the
	// cards not counting towards the winning hand to 25 % opacity). The
	// corresponding hole cards of the winners come via p["fade0"]/p["fade1"].
	Q_PROPERTY(QVariantList boardCardFade READ boardCardFade NOTIFY boardCardFadeChanged)
	// The active action timeout: the seat that is currently to act (−1 = none) and the
	// timeout duration in seconds. For it the player box shows a small
	// progress bar instead of the action badge.
	Q_PROPERTY(int timeoutSeatId READ timeoutSeatId NOTIFY timeoutChanged)
	Q_PROPERTY(int timeoutSec READ timeoutSec NOTIFY timeoutChanged)
	// The game history as an incremental model (see GameLogModel). CONSTANT, because
	// the model pointer is fixed – updates run via the model signals.
	Q_PROPERTY(GameLogModel* gameLog READ gameLog CONSTANT)
	Q_PROPERTY(QStringList chatLog READ chatLog NOTIFY chatLogChanged)
	// The translator for the game chat. The ChatBox routes taps on the globe
	// symbol to chatTranslator.requestTranslation(id).
	Q_PROPERTY(QObject* chatTranslator READ chatTranslator CONSTANT)
	// true as soon as there is (at least) one other human player in the game besides me
	Q_PROPERTY(bool hasHumanOpponents READ hasHumanOpponents NOTIFY hasHumanOpponentsChanged)
	// true post-river, when the human player can show their cards voluntarily
	Q_PROPERTY(bool canShowCards READ canShowCards NOTIFY canShowCardsChanged)
	// true during the showdown/result display (post-river, until the next hand).
	// In this phase QML disables the action buttons (fold/call/raise), so that
	// no premature click for the next round goes nowhere.
	Q_PROPERTY(bool showdownActive READ showdownActive NOTIFY showdownActiveChanged)
	// The card odds of your own hand (seat 0) – analogous to the CardsChanceMonitor
	// of the widgets client. A list with 10 entries, index 0 = high card …
	// 9 = royal flush; every entry is a map {"prob": int %, "possible": bool}.
	Q_PROPERTY(QVariantList cardsChance READ cardsChance NOTIFY cardsChanceChanged)
	// true if your own player has folded (the odds are dimmed).
	Q_PROPERTY(bool cardsChanceFolded READ cardsChanceFolded NOTIFY cardsChanceChanged)
	// The network status light of your own client from the average ping
	// (the setting ShowPingStateInAvatar): 0 = unknown/no data,
	// 1 = green (≤1000 ms), 2 = yellow (≤2000 ms), 3 = red (>2000 ms). As in the
	// Qt widgets client (MyAvatarLabel::refreshPing), only at your own avatar.
	Q_PROPERTY(int pingState READ pingState NOTIFY pingStateChanged)
	// The raw values of the last server response times (ms): average/min/max.
	// −1 = no data yet. They feed the overlay at the network status dot
	// (mouseover) – like the tooltip of the Qt widgets client (refreshPing).
	Q_PROPERTY(int pingAvg READ pingAvg NOTIFY pingStateChanged)
	Q_PROPERTY(int pingMin READ pingMin NOTIFY pingStateChanged)
	Q_PROPERTY(int pingMax READ pingMax NOTIFY pingStateChanged)
	// The spectators of the running game (spectatorsDuringGame) – as in the
	// Qt widgets client (gameTableImpl::refreshSpectatorsDisplay): the count for
	// an eye icon with a badge, the names for the tooltip.
	Q_PROPERTY(int spectatorCount READ spectatorCount NOTIFY spectatorsChanged)
	Q_PROPERTY(QStringList spectatorNames READ spectatorNames NOTIFY spectatorsChanged)
	// true if I am only spectating this table. Then there is no seat of your
	// own: the GamePage draws all seats as a ring (without a self box) and
	// hides the action bar.
	Q_PROPERTY(bool spectating READ spectating NOTIFY spectatingChanged)

public:
	explicit GameHandler(QObject *parent = nullptr);
	~GameHandler() override;

	void setSession(boost::shared_ptr<Session> session);
	void setGame(boost::shared_ptr<Game> game);
	void setConfig(ConfigFile *config);
	// The sound handler is created centrally in main() and used by the lobby and
	// the game handler together (not owned): every SoundEvents
	// instance holds an audio stream of its own.
	void setSoundEvents(SoundEvents *soundEvents);
	// Register the table theme: the chat and the game history take their text colours
	// from there and recolour themselves on every style change.
	void setStyleProvider(StyleProvider *styleProvider);

	// Called from QML to start a local game
	Q_INVOKABLE void startLocalGame();
	Q_INVOKABLE void endLocalGame();
	Q_INVOKABLE bool isLocalGameRunning() const;
	// Is an internet game running (a network client, the game type internet)? Only then
	// does reporting an avatar make sense – as in the Qt widgets client, which shows
	// "Report inappropriate avatar" exclusively for internet games.
	Q_INVOKABLE bool isInternetGameRunning() const;
	// Reports the avatar of the player at the given seat as inappropriate to
	// the server (a port of MyAvatarLabel::reportBadAvatar). The avatar hash
	// follows – as in the widgets client – from the base name of the avatar file.
	Q_INVOKABLE void reportAvatar(int seatId);
	// The URL of the table statistics overview (tableview=1 + the nicks of the active players
	// at the table) – 1:1 like the Qt widgets client (MyNameLabel). It is built from the
	// live seats of the running game; empty if no game is running.
	Q_INVOKABLE QString tableStatsUrl() const;
	// The nicks of the active players at the running network table (seat order,
	// without players who have already dropped out) – the data basis for tableStatsUrl()
	// and the native table ranking page (GameTableStatsPage).
	Q_INVOKABLE QStringList tableStatsNicks() const;
	QVariantList players() const
	{
		return m_players;
	}
	int pot() const
	{
		return m_pot;
	}
	int gameId() const
	{
		return m_gameId;
	}
	QString phaseText() const
	{
		return m_phaseText;
	}
	int handNumber() const
	{
		return m_handNumber;
	}
	bool myTurn() const
	{
		return m_myTurn;
	}
	bool canAct() const
	{
		return m_canAct;
	}
	bool awaitingMyAction() const
	{
		return m_awaitingMyAction;
	}
	int callAmount() const
	{
		return m_callAmount;
	}
	int minRaiseAmount() const
	{
		return m_minRaiseAmount;
	}
	int maxRaiseAmount() const
	{
		return m_maxRaiseAmount;
	}
	int totalPot() const
	{
		return m_totalPot;
	}

	int boardCardCount() const
	{
		return m_boardCardCount;
	}
	QVariantList boardCards() const
	{
		return m_boardCards;
	}
	QVariantList winnerSeatIds() const
	{
		return m_winnerSeatIds;
	}
	QString winningHandText() const
	{
		return m_winningHandText;
	}
	QVariantList boardCardFade() const
	{
		return m_boardCardFade;
	}
	int timeoutSeatId() const
	{
		return m_timeoutSeatId;
	}
	int timeoutSec() const
	{
		return m_timeoutSec;
	}
	GameLogModel* gameLog()
	{
		return &m_gameLogModel;
	}
	// As with the history, m_chatLog contains only colour roles; the table theme
	// colours are only added when delivering (see chatcolors.h).
	QStringList chatLog() const
	{
		QStringList out;
		out.reserve(m_chatLog.size());
		for (const QString &line : m_chatLog)
			out.append(TableChatColors::expand(line, m_tableChatPalette));
		return out;
	}
	QObject* chatTranslator() const;
	bool hasHumanOpponents() const
	{
		return m_hasHumanOpponents;
	}
	bool canShowCards() const
	{
		return m_canShowCards;
	}
	bool showdownActive() const
	{
		return m_showdownActive;
	}
	QVariantList cardsChance() const
	{
		return m_cardsChance;
	}
	bool cardsChanceFolded() const
	{
		return m_cardsChanceFolded;
	}
	int pingState() const
	{
		return m_pingState;
	}
	int pingAvg() const
	{
		return m_pingAvg;
	}
	int pingMin() const
	{
		return m_pingMin;
	}
	int pingMax() const
	{
		return m_pingMax;
	}
	int spectatorCount() const
	{
		return static_cast<int>(m_spectatorNames.size());
	}
	QStringList spectatorNames() const
	{
		return m_spectatorNames;
	}
	bool spectating() const
	{
		return m_spectating;
	}

	// The line type for colouring the game history – colours/style 1:1 like the
	// Qt widgets client (the default table style).
	enum LogLineType {
		LogNormal = 0,   // actions, blinds, revealed cards (#F0F0F0)
		LogHeader,       // "## Game | Hand ##" (fett)
		LogWinnerMain,   // Gewinner Hauptpot (#FFFF00)
		LogWinnerSide,   // Gewinner Side-Pot (#FFFFCC)
		LogSitOut,       // "… sits out" (kursiv, #FF6633)
		LogBoard,        // "--- Flop/Turn/River ---" (#FF6633)
		LogGameWin       // "… wins game X!" (fett+kursiv)
	};
	Q_ENUM(LogLineType)

	// Append a line to the in-game action log (called from QmlGuiInterface).
	Q_INVOKABLE void appendGameLog(const QString &message, int type = LogNormal);
	// In-game chat: append a received message / send one to the table.
	Q_INVOKABLE void appendChat(const QString &playerName, const QString &message);
	Q_INVOKABLE void sendChat(const QString &message);

	// Called from QmlGuiInterface callbacks (must be Q_INVOKABLE for invokeMethod)
	Q_INVOKABLE void onRefreshSet();
	Q_INVOKABLE void onRefreshAction(int playerId, int playerAction);
	Q_INVOKABLE void onRefreshCash();
	Q_INVOKABLE void onRefreshPlayerName();
	Q_INVOKABLE void onRefreshPot();
	Q_INVOKABLE void onRefreshGameLabels(int gameState);
	Q_INVOKABLE void onMeInAction();
	Q_INVOKABLE void onDisableMyButtons();
	Q_INVOKABLE void onStartTimeoutAnimation(int playerNum, int timeoutSec);
	Q_INVOKABLE void onStopTimeoutAnimation(int playerNum);
	// The network game is finished / we were removed from the game: reset the
	// GameHandler state, so that no stale m_myTurn/m_game is left behind (otherwise
	// a late action can run into the dead game → see ClientThread::SendPlayerAction).
	Q_INVOKABLE void onNetworkGameEnded();
	// Network: a player has left the game → clear the seat and note it in the
	// log (left / kicked / disconnected – removeReason from socket_msg.h).
	Q_INVOKABLE void onNetClientPlayerLeft(unsigned uniquePlayerId,
										   const QString &playerName = QString(),
										   int removeReason = 0);
	// Network: re-read the spectator list of the running game from the session
	// (called on a spectator join/leave/rename).
	Q_INVOKABLE void refreshSpectators();
	// Netzwerk: eigener Client-Ping aktualisiert → Netzwerkstatus-Ampel ableiten.
	Q_INVOKABLE void onPingUpdate(int minPing, int avgPing, int maxPing);
	Q_INVOKABLE void onBlindsSet(int smallBlind);
	// Local game: check for the end of the tournament (only ONE player with chips left). It is
	// called at the end of a hand, BEFORE the next hand is started. It returns
	// true if the game is over – then no further hand may follow.
	Q_INVOKABLE bool checkLocalGameOver();
	Q_INVOKABLE void onNextRoundCleanGui();
	Q_INVOKABLE void onDealFlopCards();
	Q_INVOKABLE void onDealTurnCard();
	Q_INVOKABLE void onDealRiverCard();
	// Game-loop advance callbacks (called via QMetaObject from QmlGuiInterface)
	Q_INVOKABLE void onRunBeRo();
	Q_INVOKABLE void onAfterDealCards();
	Q_INVOKABLE void onNextPlayerBeRo();
	Q_INVOKABLE void onSwitchRounds();
	Q_INVOKABLE void onPostRiverRunBeRo();
	Q_INVOKABLE void onShowdown();
	Q_INVOKABLE void onFlipHolecardsAllIn();
	// A player shows their cards voluntarily after the hand (AfterHandShowCards).
	Q_INVOKABLE void onPlayerShowCards(unsigned playerId);
	// Freeze the fold/show state at the end of the hand. It MUST run synchronously on the
	// network thread (a DirectConnection from postRiverRunAnimation1) while the
	// engine data is still valid – see showdownFolded() in the .cpp.
	Q_INVOKABLE void captureShowdownSnapshot();

	// Called from QML
	Q_INVOKABLE void fold();
	// Check/call. expectedAmount is the amount the player SAW on the button
	// when they triggered/preselected it (0 = "check"), −1 = any
	// amount is fine (deliberately only "auto check/call").
	//
	// The comparison MUST happen here: the engine data (highestSet) is changed by the
	// network thread immediately when the opponent's action arrives, while the
	// QML side only catches up with its values via the queued signals. Between the two
	// lies a window in which the button still shows "check" (or a preselection
	// means "check") while the engine already knows about an all-in/raise. If the
	// action is – as it used to be – only derived here from the live state, the
	// free check that was meant becomes a call over the full new amount (a player report:
	// the BB checks, the opponent goes all-in, the client calls $4226). If the engine demands more
	// than the player saw, NOTHING is therefore sent and false is returned;
	// the turn window stays open, the player decides anew.
	Q_INVOKABLE bool call(int expectedAmount = -1);
	Q_INVOKABLE void raise(int amount = 0);
	Q_INVOKABLE void allIn();
	Q_INVOKABLE void showMyCards();

signals:
	void playersChanged();
	void potChanged();
	void gameIdChanged();
	void phaseTextChanged();
	void handNumberChanged();
	void myTurnChanged();
	// It is triggered on EVERY "I am to act" callback of the engine (meInAction),
	// independently of whether m_myTurn changes in the process. On it the QML side executes
	// the preselected/automatic action – like the widgets client, which executes the
	// remembered action directly in meInAction() (a button click). That way the
	// execution NO longer depends on the myTurn edge change (which fails to appear, for instance, when the
	// turn was already marked active via the action timer).
	void meInActionTriggered();
	void refreshActionTriggered();   // a real player action (not a global refresh)
	// A check/call was discarded because the engine demanded more at the moment of
	// execution than the player saw on the button (an opponent raised/went
	// all-in in between). NOTHING was sent – I am
	// still to act. QML thereupon discards the preselection and locks the
	// call button briefly (AccidentallyCallBlocker), so that the next click does not
	// accidentally call the new, higher amount.
	void actionRejected(int requiredAmount, int expectedAmount);
	void roundValuesReady();          // after a round change: fresh values are available
	// The betting round has just been decided (the last action of the round has happened, the next
	// round/hand has not started yet). On it QML locks the action buttons
	// immediately and discards stale preselections/amounts until roundValuesReady() or
	// the next turn of your own delivers fresh values again.
	void bettingRoundEnded();
	void canActChanged();
	void awaitingMyActionChanged();
	void callAmountChanged();
	void minRaiseAmountChanged();
	void maxRaiseAmountChanged();
	void totalPotChanged();
	void boardCardCountChanged();
	void boardCardsChanged();
	void winnerSeatIdsChanged();
	void winningHandTextChanged();
	void boardCardFadeChanged();
	void timeoutChanged();
	void chatLogChanged();
	void hasHumanOpponentsChanged();
	void canShowCardsChanged();
	void showdownActiveChanged();
	void cardsChanceChanged();
	void pingStateChanged();
	void spectatorsChanged();
	void spectatingChanged();
	// An emoji reaction was received (the chat convention "/emoji 🎉" of the web client) –
	// it is not shown in the chat but played as an animation at the seat.
	void reactionReceived(const QString &playerName, const QString &emoji);
	// Your own cards were revealed (on a click) → the self box plays a flip
	// confirmation like the widget client (showHoleCards), although your own
	// cards already lie open.
	void myCardsShown();
	// The local game is over: only one player has chips left. On it QML shows
	// the winner message (a new game / back to the menu). winnerSeatId == 0
	// means: the human player has won.
	void localGameFinished(const QString &winnerName, int winnerSeatId);

protected:
	// An app-wide filter: real user activity (mouse/keyboard) → a ResetTimeout
	// to the server, so that the in-game AFK timeout (21 min) does not strike.
	bool eventFilter(QObject *watched, QEvent *event) override;

private:
	// The only place that reports changes to the chat history. It bundles several
	// changes of the same event loop pass into ONE chatLogChanged():
	// every notification delivers the complete history anew to QML, where
	// every ChatBox rebuilds its complete rich text document from it (moving
	// the translate symbol triggered that twice per line the mouse passed
	// over, for instance). The counterpart to LobbyHandler::notifyChatLogChanged.
	void notifyChatLogChanged();
	bool localGameCallbacksBlocked() const;
	void playYourTurnTimeoutSound();
	// Adopt the colours from the table theme into m_tableChatPalette and report the chat
	// as well as the history as changed.
	void refreshTableChatPalette();
	void refreshPlayerData();
	void refreshBoardCards();
	// Recompute the odds (CardsValue::calcCardsChance) + the currently best hand of your own
	// player and – only on a change – notify the QML side.
	void refreshChanceAndHand();
	void refreshPotData();
	void computeCallAndRaiseAmounts();
	// Local game: remove players with 0 coins from the display after 10 seconds
	// (analogous to onNetClientPlayerLeft in online games).
	void checkBustedLocalPlayers();
	// True if the human player (seat 0) can currently act (in the
	// hand, not all-in/folded, cash > 0, active). Engine based.
	bool humanCanAct() const;
	// True if the server is currently waiting for MY action. What counts is the
	// action timer on my seat (m_timeoutSeatId == 0), which is already set from
	// startTimeoutAnimation on (before meInAction). In addition m_myTurn,
	// in case the timer path does not apply at some point. It prevents discarded actions.
	//
	// As a spectator NEVER: there seat 0 is a foreign player whose action
	// timer sets m_timeoutSeatId to 0. Without this guard fold()/
	// call()/raise()/showMyCards() (keyboard shortcuts!) could be triggered for them.
	//
	// A third source: engineAwaitsMyAction() – the engine knows it authoritatively,
	// even when both flags were cleared by a trailing callback.
	// At the same time the window flag locks reliably against a second action in the
	// same turn window (previously clearing the flags in doActionDone() did that
	// implicitly).
	// The definition is in the .cpp: it needs the complete session type, because the
	// turn window flag ONLY applies in a network game (locally there is no
	// PlayersTurnMessage that could open it again).
	bool isMyTurnToAct() const;
	// Does the turn pointer of the engine point at me? A pure comparison, without further
	// conditions – its edge opens/closes the turn window.
	// Only meaningful in a network game – in a local game your own
	// unique ID is 0 and thus indistinguishable from the initial value of a fresh BeRo
	// (0 as well); there it stays at the previous myTurn/timer path.
	bool engineTurnPointsAtMe() const;
	// The authoritative query: is the server waiting for my action NOW?
	bool engineAwaitsMyAction() const;
	// Open the turn window (a new PlayersTurnMessage for me) or close it
	// (our own action was sent, the server ended the window, the showdown …).
	void openMyTurnWindow();
	void closeMyTurnWindow();
	// Recompute m_awaitingMyAction and – only on a change – inform QML.
	void updateAwaitingMyAction();
	void doActionDone();
	// Set the showdown flag and – only on a real change – notify the QML
	// side (showdownActive gates the action buttons among other things).
	void setShowdownActive(bool active);

	boost::shared_ptr<Session> m_session;
	boost::shared_ptr<Game> m_game;
	ConfigFile *m_config = nullptr;
	SoundEvents *m_soundEventHandler = nullptr;   // not owned
	QTimer *m_timeoutBeepTimer = nullptr;
	// The rate limit for the AFK reset (ResetTimeoutMessage). As in the widgets
	// client: send it at most every few minutes, on real user activity.
	QElapsedTimer m_afkResetTimer;
	static constexpr qint64 kAfkResetIntervalMs = 3 * 60 * 1000; // 3 min

	QVariantList m_players;
	int m_pot = 0;
	int m_gameId = 0;
	QString m_phaseText;
	int m_handNumber = 0;
	bool m_myTurn = false;
	bool m_canAct = false;
	// The cached value of engineAwaitsMyAction() minus an action that has already been
	// sent – the QML side binds to it (awaitingMyAction).
	bool m_awaitingMyAction = false;
	// The last seen state of engineTurnPointsAtMe() – edge detection.
	bool m_engineTurnPointedAtMe = false;
	// The turn window is closed: either an action has already been sent or
	// the server has ended the window (PlayersActionDone for my seat,
	// a timeout, the showdown, the end of the game). Only a rising edge of the turn pointer –
	// i.e. a real new PlayersTurnMessage for me – opens it again.
	bool m_myTurnWindowClosed = true;
	// The edge detection for bettingRoundEnded(): true while computeCallAndRaise-
	// Amounts() recognises the betting round as finished (roundClosed). The signal
	// fires only on the rising edge (the round has just been decided).
	bool m_roundClosed = false;
	int m_callAmount = 0;
	int m_minRaiseAmount = 0;
	int m_maxRaiseAmount = 0;
	int m_totalPot = 0;
	int m_boardCardCount = 0;
	QVariantList m_boardCards;  // 5 slots: card index (0-51) or -1 if not dealt
	QVariantList m_winnerSeatIds;
	QString m_winningHandText;  // The name of the winning hand (only during the showdown)
	// The showdown spotlight (see the boardCardFade property). m_boardCardFade has 5
	// bool entries; m_holeFade0/1 contain the seats whose hole card 0/1
	// is dimmed. They are reset at the beginning of every hand.
	QVariantList m_boardCardFade = {false, false, false, false, false};
	QSet<int> m_holeFade0;
	QSet<int> m_holeFade1;
	int m_timeoutSeatId = -1;   // The seat with a running action timeout (−1 = none)
	int m_timeoutSec = 0;       // The duration of the action timeout in seconds
	GameLogModel m_gameLogModel; // The live action log (game history) for the overlay
	QStringList m_chatLog;      // The in-game chat history (raw, with colour roles)
	// The table theme that delivers the colours of the chat and the history. The StyleProvider
	// lives in main() and can die before this handler → QPointer.
	QPointer<StyleProvider> m_styleProvider;
	TableChatColors::Palette m_tableChatPalette;
	// Is a bundled chatLogChanged notification already running? (notifyChatLogChanged)
	bool m_chatLogNotifyPending = false;
	ChatTranslator *m_chatTranslator = nullptr; // it appends the translate symbols and translates them
	bool m_hasHumanOpponents = false;
	bool m_canShowCards = false;
	// The showdown is active: only then may opponent cards be revealed. It prevents the
	// (still stale) playerNeedToShowCards list from wrongly revealing cards during the river
	// betting round of the next hand.
	bool m_showdownActive = false;
	// The card odds (10 maps {prob, possible}) + the fold state + the current hand
	// of your own player. They are updated in refreshChanceAndHand().
	QVariantList m_cardsChance;
	bool m_cardsChanceFolded = false;
	int m_pingState = 0;  // The network status light (0 unknown,1 green,2 yellow,3 red)
	int m_pingAvg = -1;   // the last server response times in ms (−1 = no data)
	int m_pingMin = -1;
	int m_pingMax = -1;
	QStringList m_spectatorNames;  // The names of the spectators of the running game
	bool m_spectating = false;     // I am only spectating this table
	// The input signature of the last odds computation (hole cards, board, fold
	// state). If it stays the same, the expensive calcCardsChance loop is
	// skipped – refreshChanceAndHand() would otherwise run on every micro refresh.
	std::vector<int> m_lastChanceInputs;
	// The all-in reveal: all cards of players who have not folded are visible
	// (AllInShowCardsMessage), reset until the next hand.
	bool m_allInRevealed = false;
	// Players (unique ID) who have shown their cards voluntarily after the hand
	// (AfterHandShowCardsMessage → SignalNetClientPostRiverShowCards). Their cards
	// stay revealed until the next hand. In the widgets client
	// gameTableImpl::showHoleCards does that; the QML showdown reveal does not apply here,
	// because the pointer is not in playerNeedToShowCards (a win without a showdown).
	QSet<unsigned> m_postRiverShownPlayers;
	// ── The showdown snapshot (the fold and reveal state at the end of the hand) ───────────
	// The showdown code runs deferred (onShowdown via a QueuedConnection), and the
	// next hand may long since have overwritten the engine data by then.
	// So freeze it at the end of the hand and afterwards read ONLY these copies.
	// Details and the reasoning: showdownFolded() in gamehandler.cpp.
	QSet<unsigned> m_foldedAtHandEnd;
	QSet<unsigned> m_needToShowAtHandEnd;
	bool m_showdownSnapshotValid = false;
	// The fold/reveal state for the showdown. While the snapshot is valid,
	// it wins against the (possibly already reset) live state.
	bool showdownFolded(unsigned uniqueId, bool liveFolded) const;
	bool showdownNeedsToShow(unsigned uniqueId, bool liveNeedsToShow) const;
	// The action display: per seat the action seen last + the round token
	// in which it was set. That way the action is only shown in its own round
	// and is removed everywhere automatically at the beginning of a round.
	int m_lastSeenAction[10] = {};
	// Besides the action type, remember the bet per seat: calling again
	// after a raise stays type CALL but increases the bet → it counts as a
	// fresh action, so that the (previously cleared) badge appears again.
	int m_lastSeenSet[10] = {};
	int m_actionToken[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
	// If a player bets/raises, the action badges of all the other (not yet
	// folded) players have to disappear – they are to act again. For that every
	// action gets a consecutive sequence number; it is only shown if it is
	// at least as new as the last aggression (bet/raise) of the round.
	int m_actionSeq[10] = {};
	int m_actionCounter = 0;
	int m_lastAggressorSeq = 0;
	int m_aggressorToken = -1;
	bool m_localGameExitRequested = false;
	// The unique IDs of players who have left the network game.
	// Their seat is displayed as empty in refreshPlayerData().
	QSet<unsigned> m_leftPlayers;
	// Local game: the running 10 second timers for players with 0 coins.
	// The key = the unique player ID; after it expires the player is treated like a
	// player who left an online game (the seat is hidden).
	QMap<unsigned, QTimer*> m_bustedLocalTimers;
	// The local game is finished (only one player with chips left). It prevents
	// checkLocalGameOver() from reporting the end of the game several times.
	bool m_localGameOver = false;
};

#endif // GAMEHANDLER_H
