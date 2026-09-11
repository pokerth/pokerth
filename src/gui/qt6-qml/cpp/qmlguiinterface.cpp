/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2025 Felix Hammer, Florian Thauer, Lothar May          *
 *****************************************************************************/

#include "qmlguiinterface.h"
#include "serverconnectionhandler.h"
#include "lobbyhandler.h"
#include "gamehandler.h"
#include "androidconnectionservice.h"
#include "iosbackgroundsession.h"
#include "configfile.h"
#include <session.h>
#include <game.h>
#include <gamedata.h>
#include <game_defs.h>
#include <cardsvalue.h>
#include "net/socket_msg.h"
#include <QString>
#include <QChar>
#include <QMetaObject>
#include <QTimer>

namespace
{
// Card code (0-51) → a short form with a Unicode suit symbol, e.g. "K♥".
//   0-12 diamonds(♦), 13-25 hearts(♥), 26-38 spades(♠), 39-51 clubs(♣); rank 2..A.
QString fmtCard(int code)
{
	if (code < 0 || code > 51)
		return QStringLiteral("?");
	static const char *ranks[] = {"2","3","4","5","6","7","8","9","10","J","Q","K","A"};
	static const QChar suits[] = { QChar(0x2666), QChar(0x2665), QChar(0x2660), QChar(0x2663) };
	return QString::fromLatin1(ranks[code % 13]) + QString(suits[code / 13]);
}
} // namespace

QmlGuiInterface::QmlGuiInterface(ConfigFile *config, ServerConnectionHandler *handler, LobbyHandler *lobbyHandler)
	: m_config(config), m_session(), m_handler(handler), m_lobbyHandler(lobbyHandler)
{
}

QmlGuiInterface::~QmlGuiInterface()
{
}

void QmlGuiInterface::SignalNetClientConnect(int actionID)
{
	if (m_handler) {
		QMetaObject::invokeMethod(m_handler, [this, actionID]() {
			m_handler->onNetClientConnect(actionID);
		}, Qt::QueuedConnection);
	}

	if (m_lobbyHandler && m_session && actionID == 1) {
		QMetaObject::invokeMethod(m_lobbyHandler, [this]() {
			// New client initialization: force lobby model reset/sync.
			m_lobbyHandler->setSession(m_session);
		}, Qt::QueuedConnection);
	}

	// Client start (actionID 1 = MSG_SOCK_INIT_DONE, all kinds of connection:
	// internet, LAN join, own host): start the Android foreground service,
	// so that the connection survives background phases (doze/app freezer).
	// It is stopped on a network error (SignalNetClientError) and when
	// leaving the server (LobbyHandler::leaveServer).
	if (actionID == 1) {
		AndroidConnectionService::start();
		// The iOS counterpart: it marks the active session, so that a background
		// grace period is requested when switching apps (see iosbackgroundsession.h).
		IosBackgroundSession::start();
	}
}

void QmlGuiInterface::SignalNetClientGameInfo(int actionID)
{
	// MSG_NET_GAME_CLIENT_SYNCREJOIN: we have rejoined a running game;
	// the server, however, only puts us at the table at the beginning of the next hand.
	// Until then the waiting room stays - without a notice that
	// would be indistinguishable from normally waiting for fellow players.
	// The counterpart to the widgets client (waitRejoinStartGameMsgBox).
	if (m_lobbyHandler && actionID == MSG_NET_GAME_CLIENT_SYNCREJOIN) {
		QMetaObject::invokeMethod(m_lobbyHandler, [this]() {
			m_lobbyHandler->onRejoinSyncWait();
		}, Qt::QueuedConnection);
	}
}

void QmlGuiInterface::SignalNetServerError(int errorID, int osErrorID)
{
	// Errors of the embedded server (hosting your own network game), e.g. an
	// occupied port. Report it the same way as in the widgets client (signalNetServerError -> the same
	// networkError slot) - it was swallowed before.
	if (m_handler) {
		QMetaObject::invokeMethod(m_handler, [this, errorID, osErrorID]() {
			m_handler->onNetServerError(errorID, osErrorID);
		}, Qt::QueuedConnection);
	}
}

void QmlGuiInterface::SignalNetClientError(int errorID, int osErrorID)
{
	if (m_handler) {
		QMetaObject::invokeMethod(m_handler, [this, errorID, osErrorID]() {
			m_handler->onNetClientError(errorID, osErrorID);
		}, Qt::QueuedConnection);
	}
	// A network error also ends a running network game → reset the game state
	// (m_myTurn/m_game), as with SignalNetClientRemovedFromGame,
	// so that no late action runs into the dead game.
	if (m_gameHandler) {
		QMetaObject::invokeMethod(m_gameHandler, "onNetworkGameEnded", Qt::QueuedConnection);
	}
	// The connection is gone → the foreground service has nothing left to protect.
	AndroidConnectionService::stop();
	IosBackgroundSession::stop();
}

void QmlGuiInterface::SignalNetClientLoginShow()
{
	if (m_handler) {
		QMetaObject::invokeMethod(m_handler, [this]() {
			m_handler->onNetClientLoginShow();
		}, Qt::QueuedConnection);
	}
}

void QmlGuiInterface::SignalNetClientGameListNew(unsigned gameId)
{
	if (m_lobbyHandler && m_session) {
		GameInfo gameInfo = m_session->getClientGameInfo(gameId);
		QString gameName = QString::fromStdString(gameInfo.name);
		QMetaObject::invokeMethod(m_lobbyHandler, "onGameListNew", Qt::QueuedConnection,
								  Q_ARG(unsigned, gameId), Q_ARG(QString, gameName));
	}
}

void QmlGuiInterface::SignalNetClientGameListRemove(unsigned gameId)
{
	if (m_lobbyHandler) {
		QMetaObject::invokeMethod(m_lobbyHandler, "onGameListRemove", Qt::QueuedConnection,
								  Q_ARG(unsigned, gameId));
	}
}

void QmlGuiInterface::SignalNetClientGameListUpdateMode(unsigned gameId, GameMode mode)
{
	if (m_lobbyHandler) {
		QMetaObject::invokeMethod(m_lobbyHandler, "onGameListUpdateMode", Qt::QueuedConnection,
								  Q_ARG(unsigned, gameId), Q_ARG(int, static_cast<int>(mode)));
	}
}

void QmlGuiInterface::SignalNetClientGameListUpdateAdmin(unsigned gameId, unsigned adminPlayerId)
{
	Q_UNUSED(adminPlayerId)
	if (m_lobbyHandler) {
		QMetaObject::invokeMethod(m_lobbyHandler, "onGameListChanged", Qt::QueuedConnection,
								  Q_ARG(unsigned, gameId));
	}
}

void QmlGuiInterface::SignalNetClientGameListPlayerJoined(unsigned gameId, unsigned playerId)
{
	Q_UNUSED(playerId)
	if (m_lobbyHandler) {
		QMetaObject::invokeMethod(m_lobbyHandler, "onGameListChanged", Qt::QueuedConnection,
								  Q_ARG(unsigned, gameId));
	}
}

void QmlGuiInterface::SignalNetClientGameListPlayerLeft(unsigned gameId, unsigned playerId)
{
	Q_UNUSED(playerId)
	if (m_lobbyHandler) {
		QMetaObject::invokeMethod(m_lobbyHandler, "onGameListChanged", Qt::QueuedConnection,
								  Q_ARG(unsigned, gameId));
	}
}

void QmlGuiInterface::SignalNetClientGameListSpectatorJoined(unsigned gameId, unsigned playerId)
{
	Q_UNUSED(playerId)
	if (m_lobbyHandler) {
		QMetaObject::invokeMethod(m_lobbyHandler, "onGameListChanged", Qt::QueuedConnection,
								  Q_ARG(unsigned, gameId));
	}
}

void QmlGuiInterface::SignalNetClientGameListSpectatorLeft(unsigned gameId, unsigned playerId)
{
	Q_UNUSED(playerId)
	if (m_lobbyHandler) {
		QMetaObject::invokeMethod(m_lobbyHandler, "onGameListChanged", Qt::QueuedConnection,
								  Q_ARG(unsigned, gameId));
	}
}

void QmlGuiInterface::SignalNetClientLobbyChatMsg(const std::string &playerName, const std::string &msg)
{
	if (m_lobbyHandler) {
		const QString qPlayerName = QString::fromStdString(playerName);
		const QString qMsg = QString::fromStdString(msg);
		QMetaObject::invokeMethod(m_lobbyHandler, "onLobbyChatMessage", Qt::QueuedConnection,
								  Q_ARG(QString, qPlayerName), Q_ARG(QString, qMsg));
	}
}

void QmlGuiInterface::SignalNetClientGameChatMsg(const std::string &playerName, const std::string &msg)
{
	if (m_gameHandler) {
		QMetaObject::invokeMethod(m_gameHandler, "appendChat", Qt::QueuedConnection,
								  Q_ARG(QString, QString::fromStdString(playerName)),
								  Q_ARG(QString, QString::fromStdString(msg)));
	}
}

void QmlGuiInterface::SignalNetClientPingUpdate(unsigned minPing, unsigned avgPing, unsigned maxPing)
{
	// Our own client ping → to the GameHandler, which derives the network status (the light)
	// for our own avatar corner from it (the setting ShowPingStateInAvatar) and
	// provides the raw values (min/avg/max ms) for the mouseover overlay.
	// The call comes from the network thread → QueuedConnection.
	if (m_gameHandler) {
		QMetaObject::invokeMethod(m_gameHandler, "onPingUpdate", Qt::QueuedConnection,
								  Q_ARG(int, static_cast<int>(minPing)),
								  Q_ARG(int, static_cast<int>(avgPing)),
								  Q_ARG(int, static_cast<int>(maxPing)));
	}
}

void QmlGuiInterface::SignalNetClientPrivateChatMsg(const std::string &playerName, const std::string &msg)
{
	if (m_lobbyHandler) {
		QMetaObject::invokeMethod(m_lobbyHandler, "onPrivateChatMessage", Qt::QueuedConnection,
								  Q_ARG(QString, QString::fromStdString(playerName)),
								  Q_ARG(QString, QString::fromStdString(msg)));
	}
}

void QmlGuiInterface::SignalLobbyPlayerJoined(unsigned playerId, const std::string &nickName)
{
	if (m_lobbyHandler) {
		const QString qNickName = QString::fromStdString(nickName);
		QMetaObject::invokeMethod(m_lobbyHandler, "onLobbyPlayerJoined", Qt::QueuedConnection,
								  Q_ARG(unsigned, playerId), Q_ARG(QString, qNickName));
	}
}

void QmlGuiInterface::SignalLobbyPlayerLeft(unsigned playerId)
{
	if (m_lobbyHandler) {
		QMetaObject::invokeMethod(m_lobbyHandler, "onLobbyPlayerLeft", Qt::QueuedConnection,
								  Q_ARG(unsigned, playerId));
	}
}

void QmlGuiInterface::SignalNetClientShowTimeoutDialog(NetTimeoutReason reason, unsigned remainingSec)
{
	if (m_lobbyHandler) {
		QMetaObject::invokeMethod(m_lobbyHandler, "onTimeoutWarning", Qt::QueuedConnection,
								  Q_ARG(int, static_cast<int>(reason)),
								  Q_ARG(int, static_cast<int>(remainingSec)));
	}
}

void QmlGuiInterface::SignalNetClientMsgBox(const std::string &msg)
{
	if (m_lobbyHandler) {
		QMetaObject::invokeMethod(m_lobbyHandler, "onNetworkMessage", Qt::QueuedConnection,
								  Q_ARG(QString, QString::fromUtf8(msg.c_str())));
	}
}

void QmlGuiInterface::SignalNetClientMsgBox(unsigned msgId)
{
	if (m_lobbyHandler) {
		QMetaObject::invokeMethod(m_lobbyHandler, "onNetworkMessageId", Qt::QueuedConnection,
								  Q_ARG(unsigned, msgId));
	}
}

void QmlGuiInterface::SignalNetClientNotification(int notificationId)
{
	if (m_lobbyHandler) {
		QMetaObject::invokeMethod(m_lobbyHandler, "onNetworkNotification", Qt::QueuedConnection,
								  Q_ARG(int, notificationId));
	}
}

void QmlGuiInterface::SignalNetClientPlayerJoined(unsigned playerId, const std::string &playerName, bool isGameAdmin)
{
	if (m_lobbyHandler) {
		const QString qPlayerName = QString::fromStdString(playerName);
		QMetaObject::invokeMethod(m_lobbyHandler, "updatePlayerName", Qt::QueuedConnection,
								  Q_ARG(unsigned, playerId), Q_ARG(QString, qPlayerName), Q_ARG(bool, isGameAdmin));
		// Notification sound (playerconnected / onlinegameready) – queued after
		// updatePlayerName, so that the player list is up to date.
		QMetaObject::invokeMethod(m_lobbyHandler, "onGamePlayerJoined", Qt::QueuedConnection);
	}
}

void QmlGuiInterface::SignalNetClientPlayerChanged(unsigned playerId, const std::string &newPlayerName)
{
	if (m_lobbyHandler) {
		const QString qPlayerName = QString::fromStdString(newPlayerName);
		// Read isAdmin from session — same as Qt widgets GUI does on demand
		const bool isAdmin = m_session ? m_session->getClientPlayerInfo(playerId).isAdmin : false;
		QMetaObject::invokeMethod(m_lobbyHandler, "updatePlayerName", Qt::QueuedConnection,
								  Q_ARG(unsigned, playerId), Q_ARG(QString, qPlayerName), Q_ARG(bool, isAdmin));
	}
	// Update the spectator display (a renamed player can be a spectator
	// – as in the widgets client, which triggers refreshSpectatorsDisplay here).
	if (m_gameHandler) {
		QMetaObject::invokeMethod(m_gameHandler, "refreshSpectators", Qt::QueuedConnection);
		// The PlayerInfo arrives asynchronously (it is requested when joining the
		// running game). Rebuild the seat data, so that the name, the country flag and
		// the guest status at the table are correct immediately instead of only at the next action.
		QMetaObject::invokeMethod(m_gameHandler, "onRefreshPlayerName", Qt::QueuedConnection);
	}
}

void QmlGuiInterface::SignalNetClientSpectatorJoined(unsigned /*playerId*/, const std::string & /*playerName*/)
{
	if (m_gameHandler)
		QMetaObject::invokeMethod(m_gameHandler, "refreshSpectators", Qt::QueuedConnection);
}

void QmlGuiInterface::SignalNetClientSpectatorLeft(unsigned /*playerId*/, const std::string & /*playerName*/, int /*removeReason*/)
{
	if (m_gameHandler)
		QMetaObject::invokeMethod(m_gameHandler, "refreshSpectators", Qt::QueuedConnection);
}

void QmlGuiInterface::SignalNetClientSelfJoined(unsigned playerId, const std::string &playerName, bool isGameAdmin)
{
	if (m_lobbyHandler) {
		const QString qPlayerName = QString::fromStdString(playerName);
		QMetaObject::invokeMethod(m_lobbyHandler, [this, playerId, qPlayerName, isGameAdmin]() {
			m_lobbyHandler->setMyPlayerInfo(playerId, qPlayerName);
			// On a self join (e.g. as the host of your own game) adopt the
			// game admin status → the start button in the waiting room becomes visible.
			// Strictly separate from the server admin (kickban / close the game).
			m_lobbyHandler->setCurrentGameAdmin(isGameAdmin);
			m_lobbyHandler->onSelfJoinedGame();
		}, Qt::QueuedConnection);
	}
}

void QmlGuiInterface::SignalNetClientRemovedFromGame(int notificationId)
{
	if (m_lobbyHandler) {
		QMetaObject::invokeMethod(m_lobbyHandler, [this, notificationId]() {
			m_lobbyHandler->onRemovedFromGame(notificationId);
		}, Qt::QueuedConnection);
	}
	// Reset the GameHandler state (m_myTurn/m_game), so that no late
	// action runs into the finished game.
	if (m_gameHandler) {
		QMetaObject::invokeMethod(m_gameHandler, "onNetworkGameEnded", Qt::QueuedConnection);
	}
}

void QmlGuiInterface::SignalNetClientWaitDialog()
{
	// The network game is finished (Type_EndOfGameMessage) or we have been
	// removed – the engine requests here (like showClientDialog() in the widgets
	// client) that the game table be closed and that we return to the waiting room
	// or the lobby. Without this the game table stayed open after the end of the game.
	// With auto-leave active, SignalNetClientRemovedFromGame follows immediately,
	// which pops on from there into the lobby list.
	if (m_lobbyHandler) {
		QMetaObject::invokeMethod(m_lobbyHandler, "onWaitGameDialog", Qt::QueuedConnection);
	}
}

void QmlGuiInterface::SignalNetClientPlayerLeft(unsigned playerId, const std::string &playerName, int removeReason)
{
	// Clear the seat of the player in the game view and note it in the log
	// (left / kicked / disconnected – removeReason).
	if (m_gameHandler) {
		QMetaObject::invokeMethod(m_gameHandler, "onNetClientPlayerLeft", Qt::QueuedConnection,
								  Q_ARG(unsigned, playerId),
								  Q_ARG(QString, QString::fromStdString(playerName)),
								  Q_ARG(int, removeReason));
	}
}

void QmlGuiInterface::SignalSelfGameInvitation(unsigned gameId, unsigned playerIdFrom)
{
	qDebug() << "[INVITE] SignalSelfGameInvitation received: gameId=" << gameId << "fromPlayerId=" << playerIdFrom
			 << "lobbyHandler=" << (m_lobbyHandler ? "ok" : "NULL");
	if (m_lobbyHandler) {
		QMetaObject::invokeMethod(m_lobbyHandler, [this, gameId, playerIdFrom]() {
			m_lobbyHandler->onSelfGameInvitation(gameId, playerIdFrom);
		}, Qt::QueuedConnection);
	}
}

void QmlGuiInterface::SignalPlayerGameInvitation(unsigned gameId, unsigned playerIdWho, unsigned playerIdFrom)
{
	if (m_lobbyHandler) {
		QMetaObject::invokeMethod(m_lobbyHandler, [this, gameId, playerIdWho, playerIdFrom]() {
			m_lobbyHandler->onPlayerGameInvitation(gameId, playerIdWho, playerIdFrom);
		}, Qt::QueuedConnection);
	}
}

void QmlGuiInterface::SignalRejectedGameInvitation(unsigned gameId, unsigned playerIdWho, DenyGameInvitationReason reason)
{
	if (m_lobbyHandler) {
		const int r = static_cast<int>(reason);
		QMetaObject::invokeMethod(m_lobbyHandler, [this, gameId, playerIdWho, r]() {
			m_lobbyHandler->onRejectedGameInvitation(gameId, playerIdWho, r);
		}, Qt::QueuedConnection);
	}
}

void QmlGuiInterface::SignalNetClientRejoinPossible(unsigned gameId)
{
	// After a connection loss the server offers to continue the old
	// game session (InitAck.rejoinGameId). It comes from the network thread.
	if (m_lobbyHandler) {
		QMetaObject::invokeMethod(m_lobbyHandler, [this, gameId]() {
			m_lobbyHandler->onRejoinPossible(gameId);
		}, Qt::QueuedConnection);
	}
}

void QmlGuiInterface::SignalNetClientGameStart(boost::shared_ptr<Game> game)
{
	if (m_lobbyHandler) {
		QMetaObject::invokeMethod(m_lobbyHandler, [this]() {
			m_lobbyHandler->onGameStarted();
		}, Qt::QueuedConnection);
	}
	if (m_gameHandler && game) {
		QMetaObject::invokeMethod(m_gameHandler, [this, game]() {
			m_gameHandler->setGame(game);
		}, Qt::QueuedConnection);
		// Adopt players who are already spectating into the eye display initially.
		QMetaObject::invokeMethod(m_gameHandler, "refreshSpectators", Qt::QueuedConnection);
	}
}

void QmlGuiInterface::refreshSet() const
{
	if (m_gameHandler) {
		QMetaObject::invokeMethod(m_gameHandler, "onRefreshSet", Qt::QueuedConnection);
	}
}

void QmlGuiInterface::refreshCash() const
{
	if (m_gameHandler) {
		QMetaObject::invokeMethod(m_gameHandler, "onRefreshCash", Qt::QueuedConnection);
	}
}

// The active player / the player action has changed → update the player data,
// so that the yellow "to act" frame and the action display (fold/call/…) follow live.
void QmlGuiInterface::refreshGroupbox(int /*playerId*/, int /*state*/) const
{
	if (m_gameHandler) {
		QMetaObject::invokeMethod(m_gameHandler, "onRefreshSet", Qt::QueuedConnection);
	}
}

void QmlGuiInterface::refreshAction(int playerId, int action) const
{
	// IMPORTANT: only refresh the GUI here, do NOT play an action sound.
	// The action sound comes exclusively via logPlayerActionMsg() (which fires
	// exactly once per action, in local AND network games). In a network game
	// the engine calls both refreshAction(id, action) and
	// logPlayerActionMsg() per action – if both played the sound, you would hear it
	// twice (slightly offset). Locally refreshAction plays no sound anyway
	// (the action = PLAYER_ACTION_NONE). onRefreshSet = the same refresh as
	// onRefreshAction, only without the sound.
	Q_UNUSED(playerId);
	Q_UNUSED(action);
	if (m_gameHandler) {
		QMetaObject::invokeMethod(m_gameHandler, "onRefreshSet", Qt::QueuedConnection);
	}
}

void QmlGuiInterface::refreshChangePlayer() const
{
	if (m_gameHandler) {
		QMetaObject::invokeMethod(m_gameHandler, "onRefreshSet", Qt::QueuedConnection);
	}
}

void QmlGuiInterface::refreshPlayerName() const
{
	if (m_gameHandler) {
		QMetaObject::invokeMethod(m_gameHandler, "onRefreshPlayerName", Qt::QueuedConnection);
	}
}

void QmlGuiInterface::refreshPot() const
{
	if (m_gameHandler) {
		QMetaObject::invokeMethod(m_gameHandler, "onRefreshPot", Qt::QueuedConnection);
	}
}

void QmlGuiInterface::refreshGameLabels(GameState state) const
{
	if (m_gameHandler) {
		int stateInt = static_cast<int>(state);
		QMetaObject::invokeMethod(m_gameHandler, "onRefreshGameLabels", Qt::QueuedConnection,
								  Q_ARG(int, stateInt));
	}
}

void QmlGuiInterface::meInAction()
{
	if (m_gameHandler) {
		QMetaObject::invokeMethod(m_gameHandler, "onMeInAction", Qt::QueuedConnection);
	}
}

void QmlGuiInterface::disableMyButtons()
{
	if (m_gameHandler) {
		QMetaObject::invokeMethod(m_gameHandler, "onDisableMyButtons", Qt::QueuedConnection);
	}
}

void QmlGuiInterface::startTimeoutAnimation(int playerNum, int timeoutSec)
{
	if (m_gameHandler) {
		QMetaObject::invokeMethod(m_gameHandler, "onStartTimeoutAnimation", Qt::QueuedConnection,
								  Q_ARG(int, playerNum), Q_ARG(int, timeoutSec));
	}
}

void QmlGuiInterface::stopTimeoutAnimation(int playerNum)
{
	if (m_gameHandler) {
		QMetaObject::invokeMethod(m_gameHandler, "onStopTimeoutAnimation", Qt::QueuedConnection,
								  Q_ARG(int, playerNum));
	}
}

void QmlGuiInterface::logPlayerActionMsg(std::string playName, int action, int setValue)
{
	if (m_gameHandler && action > 0) {
		QMetaObject::invokeMethod(m_gameHandler, "onRefreshAction", Qt::QueuedConnection,
								  Q_ARG(int, 0), Q_ARG(int, action));

		// Spielverlauf-Text (analog guiLog::logPlayerActionMsg).
		const QString name = QString::fromStdString(playName);
		QString msg = name;
		switch (action) {
		case 1:
			msg += QStringLiteral(" folds.");
			break;
		case 2:
			msg += QStringLiteral(" checks.");
			break;
		case 3:
			msg += QStringLiteral(" calls $") + QString::number(setValue) + ".";
			break;
		case 4:
			msg += QStringLiteral(" bets $") + QString::number(setValue) + ".";
			break;
		case 5:
			msg += QStringLiteral(" bets $") + QString::number(setValue) + ".";
			break;
		case 6:
			msg += QStringLiteral(" is all in with $") + QString::number(setValue) + ".";
			break;
		default:
			msg.clear();
		}
		if (!msg.isEmpty())
			QMetaObject::invokeMethod(m_gameHandler, "appendGameLog", Qt::QueuedConnection,
									  Q_ARG(QString, msg), Q_ARG(int, GameHandler::LogNormal));
	}
}

void QmlGuiInterface::logNewBlindsSetsMsg(int sbSet, int bbSet, std::string sbName, std::string bbName)
{
	if (m_gameHandler) {
		QMetaObject::invokeMethod(m_gameHandler, "onBlindsSet", Qt::QueuedConnection,
								  Q_ARG(int, sbSet));

		const QString sb = QString::fromStdString(sbName) + " posts small blind ($" + QString::number(sbSet) + ")";
		const QString bb = QString::fromStdString(bbName) + " posts big blind ($" + QString::number(bbSet) + ")";
		QMetaObject::invokeMethod(m_gameHandler, "appendGameLog", Qt::QueuedConnection,
								  Q_ARG(QString, sb), Q_ARG(int, GameHandler::LogNormal));
		QMetaObject::invokeMethod(m_gameHandler, "appendGameLog", Qt::QueuedConnection,
								  Q_ARG(QString, bb), Q_ARG(int, GameHandler::LogNormal));
	}
}

void QmlGuiInterface::logNewGameHandMsg(int gameID, int handID)
{
	if (m_gameHandler) {
		// The wording 1:1 as in guiLog::logNewGameHandMsg in the Qt widgets client.
		const QString msg = QStringLiteral("## Game: ") + QString::number(gameID)
							+ QStringLiteral(" | Hand: ") + QString::number(handID) + QStringLiteral(" ##");
		QMetaObject::invokeMethod(m_gameHandler, "appendGameLog", Qt::QueuedConnection,
								  Q_ARG(QString, msg), Q_ARG(int, GameHandler::LogHeader));
	}
}

void QmlGuiInterface::logPlayerWinsMsg(std::string playerName, int pot, bool main)
{
	if (m_gameHandler) {
		QString msg = QString::fromStdString(playerName) + " wins $" + QString::number(pot);
		if (!main)
			msg += QStringLiteral(" (side pot)");
		QMetaObject::invokeMethod(m_gameHandler, "appendGameLog", Qt::QueuedConnection,
								  Q_ARG(QString, msg),
								  Q_ARG(int, main ? GameHandler::LogWinnerMain : GameHandler::LogWinnerSide));
	}
}

void QmlGuiInterface::logPlayerSitsOut(std::string playerName)
{
	if (m_gameHandler) {
		const QString msg = QString::fromStdString(playerName) + " sits out";
		QMetaObject::invokeMethod(m_gameHandler, "appendGameLog", Qt::QueuedConnection,
								  Q_ARG(QString, msg), Q_ARG(int, GameHandler::LogSitOut));
	}
}

void QmlGuiInterface::logDealBoardCardsMsg(int roundID, int card1, int card2, int card3, int card4, int card5)
{
	if (!m_gameHandler)
		return;
	// The number of cards is – as in the widgets client (guiLog::logDealBoardCardsMsg) –
	// determined exclusively by the round ID (flop=3, turn=4, river=5).
	// Cards that have not been dealt come in as 0 (not -1), a value check
	// would wrongly insert them as 2♦.
	QString round;
	QStringList cards;
	switch (roundID) {
	case 1:
		round = QStringLiteral("Flop");
		cards << fmtCard(card1) << fmtCard(card2) << fmtCard(card3);
		break;
	case 2:
		round = QStringLiteral("Turn");
		cards << fmtCard(card1) << fmtCard(card2) << fmtCard(card3) << fmtCard(card4);
		break;
	case 3:
		round = QStringLiteral("River");
		cards << fmtCard(card1) << fmtCard(card2) << fmtCard(card3) << fmtCard(card4) << fmtCard(card5);
		break;
	default:
		// Other round IDs (post-river during an all-in runout) log the
		// full board again – redundant with the river line → do not show it.
		return;
	}
	const QString msg = "--- " + round + " --- [" + cards.join(", ") + "]";
	QMetaObject::invokeMethod(m_gameHandler, "appendGameLog", Qt::QueuedConnection,
							  Q_ARG(QString, msg), Q_ARG(int, GameHandler::LogBoard));
}

void QmlGuiInterface::logFlipHoleCardsMsg(std::string playerName, int card1, int card2, int cardsValueInt, std::string showHas)
{
	if (m_gameHandler) {
		QString msg = QString::fromStdString(playerName) + " " + QString::fromStdString(showHas)
					  + " [" + fmtCard(card1) + ", " + fmtCard(card2) + "]";
		// Append the hand name (as in guiLog::logFlipHoleCardsMsg), e.g. - "Straight, six high".
		if (cardsValueInt != -1 && m_session && m_session->getCurrentGame()) {
			const std::string handName =
				CardsValue::determineHandName(cardsValueInt, m_session->getCurrentGame()->getActivePlayerList());
			if (!handName.empty())
				msg += " - \"" + QString::fromStdString(handName) + "\"";
		}
		QMetaObject::invokeMethod(m_gameHandler, "appendGameLog", Qt::QueuedConnection,
								  Q_ARG(QString, msg), Q_ARG(int, GameHandler::LogNormal));
	}
}

void QmlGuiInterface::logPlayerWinGame(std::string playerName, int gameID)
{
	if (m_gameHandler) {
		const QString msg = QString::fromStdString(playerName) + " wins game " + QString::number(gameID) + "!";
		QMetaObject::invokeMethod(m_gameHandler, "appendGameLog", Qt::QueuedConnection,
								  Q_ARG(QString, msg), Q_ARG(int, GameHandler::LogGameWin));
	}
}

void QmlGuiInterface::nextRoundCleanGui()
{
	if (m_gameHandler) {
		QMetaObject::invokeMethod(m_gameHandler, "onNextRoundCleanGui", Qt::QueuedConnection);
	}
}

void QmlGuiInterface::refreshAll() const
{
	if (m_gameHandler) {
		QMetaObject::invokeMethod(m_gameHandler, "onRefreshPlayerName", Qt::QueuedConnection);
		QMetaObject::invokeMethod(m_gameHandler, "onRefreshCash",       Qt::QueuedConnection);
		QMetaObject::invokeMethod(m_gameHandler, "onRefreshSet",        Qt::QueuedConnection);
		QMetaObject::invokeMethod(m_gameHandler, "onRefreshPot",        Qt::QueuedConnection);
	}
}

void QmlGuiInterface::dealFlopCards()
{
	if (m_gameHandler) QMetaObject::invokeMethod(m_gameHandler, "onDealFlopCards", Qt::QueuedConnection);
}

void QmlGuiInterface::dealTurnCard()
{
	if (m_gameHandler) QMetaObject::invokeMethod(m_gameHandler, "onDealTurnCard", Qt::QueuedConnection);
}

void QmlGuiInterface::dealRiverCard()
{
	if (m_gameHandler) QMetaObject::invokeMethod(m_gameHandler, "onDealRiverCard", Qt::QueuedConnection);
}

// ─── Local game-loop animation callbacks ─────────────────────────────────────
// These replicate the timer-driven animation chain in the Qt5 gametableimpl.
// Each "Animation1" starts a new betting round (calls BeRo::run()).
// "beRoAnimation2" advances to the next CPU player (calls BeRo::nextPlayer()).
// "nextPlayerAnimation" processes the end of an action (calls switchRounds()).
// "postRiverAnimation1" distributes the pot; "postRiverRunAnimation1" starts the next hand.

void QmlGuiInterface::nextPlayerAnimation()
{
	// After a player acts: trigger switchRounds() with a short delay
	if (!m_gameHandler) return;
	GameHandler *gh = m_gameHandler;
	QTimer::singleShot(300, gh, [gh]() {
		QMetaObject::invokeMethod(gh, "onSwitchRounds", Qt::DirectConnection);
	});
}

void QmlGuiInterface::beRoAnimation2(int /*myBeRoID*/)
{
	// CPU player's turn: advance to nextPlayer() with a short delay
	if (!m_gameHandler) return;
	GameHandler *gh = m_gameHandler;
	QTimer::singleShot(300, gh, [gh]() {
		QMetaObject::invokeMethod(gh, "onNextPlayerBeRo", Qt::DirectConnection);
	});
}

void QmlGuiInterface::preflopAnimation1()
{
	// Start of preflop betting: call BeRo::run()
	if (!m_gameHandler) return;
	GameHandler *gh = m_gameHandler;
	QTimer::singleShot(300, gh, [gh]() {
		QMetaObject::invokeMethod(gh, "onRunBeRo", Qt::DirectConnection);
	});
}

void QmlGuiInterface::flopAnimation1()
{
	if (!m_gameHandler) return;
	GameHandler *gh = m_gameHandler;
	QTimer::singleShot(300, gh, [gh]() {
		QMetaObject::invokeMethod(gh, "onRunBeRo", Qt::DirectConnection);
	});
}

void QmlGuiInterface::turnAnimation1()
{
	if (!m_gameHandler) return;
	GameHandler *gh = m_gameHandler;
	QTimer::singleShot(300, gh, [gh]() {
		QMetaObject::invokeMethod(gh, "onRunBeRo", Qt::DirectConnection);
	});
}

void QmlGuiInterface::riverAnimation1()
{
	if (!m_gameHandler) return;
	GameHandler *gh = m_gameHandler;
	QTimer::singleShot(300, gh, [gh]() {
		QMetaObject::invokeMethod(gh, "onRunBeRo", Qt::DirectConnection);
	});
}

void QmlGuiInterface::flipHolecardsAllIn()
{
	if (!m_gameHandler) return;
	QMetaObject::invokeMethod(m_gameHandler, "onFlipHolecardsAllIn", Qt::QueuedConnection);
}

void QmlGuiInterface::SignalNetClientPostRiverShowCards(unsigned playerId)
{
	// Another player voluntarily shows their cards after the hand. In the
	// widgets client that triggers gameTableImpl::showHoleCards (reveal the cards +
	// log it). Pass it on to the GameHandler here instead of swallowing it.
	if (!m_gameHandler) return;
	QMetaObject::invokeMethod(m_gameHandler, "onPlayerShowCards", Qt::QueuedConnection,
							  Q_ARG(unsigned, playerId));
}

void QmlGuiInterface::postRiverAnimation1()
{
	// Show-down: call BeRo::postRiverRun() which distributes the pot
	if (!m_gameHandler) return;
	GameHandler *gh = m_gameHandler;
	QTimer::singleShot(500, gh, [gh]() {
		QMetaObject::invokeMethod(gh, "onPostRiverRunBeRo", Qt::DirectConnection);
	});
}

void QmlGuiInterface::postRiverRunAnimation1()
{
	// Pot already distributed. Show showdown: reveal cards + mark winner.
	if (!m_gameHandler) return;
	GameHandler *gh = m_gameHandler;
	boost::shared_ptr<Session> session = m_session;

	// Freeze the fold/reveal state IMMEDIATELY, while the engine data of the
	// hand that has just finished is still valid. We run here synchronously on the
	// network thread (clientstate.cpp calls us directly from the
	// EndOfHandShowCards handler – the SQL log relies there on the
	// FOLD flags still being intact as well). onShowdown() below, by contrast, runs QUEUED, and in a
	// network game the SERVER starts the next hand: its initHand() sets
	// all actions to NONE and would clear away the fold state before that. The
	// widgets client does not have the problem, because it blocks the network thread here with a
	// semaphore (waitForGuiUpdateDone) – in the QML client that is a
	// no-op. Without this snapshot, folded players appear with a rated
	// hand in the showdown/game history.
	QMetaObject::invokeMethod(gh, "captureShowdownSnapshot", Qt::DirectConnection);

	QMetaObject::invokeMethod(gh, "onShowdown", Qt::QueuedConnection);

	// Start the next hand after a pause so the user can see the result
	// (show the winner + the revealed cards + the winning hand a bit longer).
	//
	// ATTENTION: in a NETWORK game the SERVER starts the next hand via
	// `Type_HandStartMessage` (see clientstate.cpp:initHand/startHand).
	// If we additionally call `onNextRoundCleanGui` + initHand +
	// startHand here after 5.5s, we duplicate the hand setup. Worse:
	// `onNextRoundCleanGui` calls `onDisableMyButtons()` synchronously
	// (gamehandler.cpp:1040). If the server starts the new hand faster
	// than the timer expires (a UTG spot → our own action lies almost
	// always within 5.5s after the showdown), this timer fires IN THE MIDDLE of
	// our own turn, sets `m_myTurn = false` and kills the action
	// buttons. The result: the user cannot click, the server timeout
	// leads to an auto-fold ("the hand before my BB does not react to
	// actions"). Therefore: in network games the client leaves the
	// hand transition entirely to the server.
	boost::shared_ptr<Session> sessionForTimer = session;
	const bool isNetwork = sessionForTimer && sessionForTimer->isNetworkClientRunning();
	if (isNetwork) {
		return;
	}
	QTimer::singleShot(5500, gh, [gh, sessionForTimer]() {
		// The end of the tournament? As in the widgets client (gameTableImpl::postRiverRun-
		// Animation6) NO further hand may follow the last one if only
		// one player has chips left – otherwise the winner plays on alone endlessly
		// and pays blinds in the process. checkLocalGameOver() reports the end of the
		// game to the QML side (the winner popup).
		if (gh->checkLocalGameOver())
			return;
		QMetaObject::invokeMethod(gh, "onNextRoundCleanGui", Qt::DirectConnection);
		if (sessionForTimer) {
			auto game = sessionForTimer->getCurrentGame();
			if (game) {
				game->initHand();
				game->startHand();
			}
		}
	});
}

void QmlGuiInterface::dealBeRoCards(int beRoID)
{
	// Called by BeRo::run() on its first invocation to "show" dealing.
	// Reveal board cards for Flop/Turn/River, then trigger the second BeRo::run().
	if (!m_gameHandler) return;
	GameHandler *gh = m_gameHandler;

	// Reveal the appropriate board cards
	if (beRoID == GAME_STATE_FLOP) {
		QMetaObject::invokeMethod(gh, "onDealFlopCards", Qt::QueuedConnection);
	} else if (beRoID == GAME_STATE_TURN) {
		QMetaObject::invokeMethod(gh, "onDealTurnCard", Qt::QueuedConnection);
	} else if (beRoID == GAME_STATE_RIVER) {
		QMetaObject::invokeMethod(gh, "onDealRiverCard", Qt::QueuedConnection);
	}

	// After the reveal, continue the round. In an all-in condition this advances
	// to the next street/showdown (no betting); otherwise it starts the betting.
	QTimer::singleShot(300, gh, [gh]() {
		QMetaObject::invokeMethod(gh, "onAfterDealCards", Qt::DirectConnection);
	});
}
