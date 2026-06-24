/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2025 Felix Hammer, Florian Thauer, Lothar May          *
 *****************************************************************************/

#include "qmlguiinterface.h"
#include "serverconnectionhandler.h"
#include "lobbyhandler.h"
#include "gamehandler.h"
#include "configfile.h"
#include <session.h>
#include <game.h>
#include <gamedata.h>
#include <game_defs.h>
#include <cardsvalue.h>
#include <QString>
#include <QChar>
#include <QMetaObject>
#include <QTimer>

namespace {
// Karten-Code (0-51) → Kurzform mit Unicode-Farbsymbol, z. B. "K♥".
//   0-12 Karo(♦), 13-25 Herz(♥), 26-38 Pik(♠), 39-51 Kreuz(♣); Rang 2..A.
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
}

void QmlGuiInterface::SignalNetClientError(int errorID, int osErrorID)
{
    if (m_handler) {
        QMetaObject::invokeMethod(m_handler, [this, errorID, osErrorID]() {
            m_handler->onNetClientError(errorID, osErrorID);
        }, Qt::QueuedConnection);
    }
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

void QmlGuiInterface::SignalNetClientPingUpdate(unsigned /*minPing*/, unsigned avgPing, unsigned /*maxPing*/)
{
    // Eigener Client-Ping → GameHandler, der daraus den Netzwerkstatus (Ampel)
    // für die eigene Avatar-Ecke ableitet (Einstellung ShowPingStateInAvatar).
    // Aufruf kommt aus dem Netzwerk-Thread → QueuedConnection.
    if (m_gameHandler) {
        QMetaObject::invokeMethod(m_gameHandler, "onPingUpdate", Qt::QueuedConnection,
                                  Q_ARG(int, static_cast<int>(avgPing)));
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

void QmlGuiInterface::SignalNetClientPlayerJoined(unsigned playerId, const std::string &playerName, bool isGameAdmin)
{
    if (m_lobbyHandler) {
        const QString qPlayerName = QString::fromStdString(playerName);
        QMetaObject::invokeMethod(m_lobbyHandler, "updatePlayerName", Qt::QueuedConnection,
                                  Q_ARG(unsigned, playerId), Q_ARG(QString, qPlayerName), Q_ARG(bool, isGameAdmin));
        // Benachrichtigungs-Sound (playerconnected / onlinegameready) – nach
        // updatePlayerName eingereiht, damit die Spielerliste aktuell ist.
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
}

void QmlGuiInterface::SignalNetClientSelfJoined(unsigned playerId, const std::string &playerName, bool isGameAdmin)
{
    if (m_lobbyHandler) {
        const QString qPlayerName = QString::fromStdString(playerName);
        QMetaObject::invokeMethod(m_lobbyHandler, [this, playerId, qPlayerName, isGameAdmin]() {
            m_lobbyHandler->setMyPlayerInfo(playerId, qPlayerName);
            // Beim Selbst-Beitritt (z. B. als Host des eigenen Spiels) den
            // Spiel-Admin-Status übernehmen → Start-Button im Warteraum sichtbar.
            // Strikt getrennt vom Server-Admin (kickban / Spiel schließen).
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
    // GameHandler-Zustand zurücksetzen (m_myTurn/m_game), damit keine späte
    // Aktion ins beendete Spiel läuft.
    if (m_gameHandler) {
        QMetaObject::invokeMethod(m_gameHandler, "onNetworkGameEnded", Qt::QueuedConnection);
    }
}

void QmlGuiInterface::SignalNetClientWaitDialog()
{
    // Das Netzwerk-Spiel ist beendet (Type_EndOfGameMessage) bzw. wir wurden
    // entfernt – die Engine fordert hier (wie showClientDialog() im Widgets-
    // Client) das Schließen des Gametables und die Rückkehr in den Warteraum
    // bzw. die Lobby an. Ohne dies blieb der Gametable nach Spielende offen.
    // Bei aktivem Auto-Leave folgt unmittelbar SignalNetClientRemovedFromGame,
    // das von dort aus bis in die Lobbyliste weiterpoppt.
    if (m_lobbyHandler) {
        QMetaObject::invokeMethod(m_lobbyHandler, "onWaitGameDialog", Qt::QueuedConnection);
    }
}

void QmlGuiInterface::SignalNetClientPlayerLeft(unsigned playerId, const std::string & /*playerName*/, int /*removeReason*/)
{
    // Sitz des Spielers in der Spielansicht leeren.
    if (m_gameHandler) {
        QMetaObject::invokeMethod(m_gameHandler, "onNetClientPlayerLeft", Qt::QueuedConnection,
                                  Q_ARG(unsigned, playerId));
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

// Aktiver Spieler / Spieler-Aktion hat sich geändert → Spielerdaten aktualisieren,
// damit der gelbe "am Zug"-Rahmen und die Aktions-Anzeige (Fold/Call/…) live folgen.
void QmlGuiInterface::refreshGroupbox(int /*playerId*/, int /*state*/) const
{
    if (m_gameHandler) {
        QMetaObject::invokeMethod(m_gameHandler, "onRefreshSet", Qt::QueuedConnection);
    }
}

void QmlGuiInterface::refreshAction(int playerId, int action) const
{
    // WICHTIG: hier nur die GUI auffrischen, KEINEN Aktions-Sound abspielen.
    // Der Aktions-Sound kommt ausschließlich über logPlayerActionMsg() (feuert
    // genau einmal pro Aktion, in lokalen UND Netzwerk-Spielen). Im Netzwerk
    // ruft die Engine pro Aktion sowohl refreshAction(id, action) als auch
    // logPlayerActionMsg() auf – würden beide den Sound spielen, hörte man ihn
    // doppelt (leicht versetzt). Lokal spielt refreshAction ohnehin keinen Sound
    // (Aktion = PLAYER_ACTION_NONE). onRefreshSet = gleiche Auffrischung wie
    // onRefreshAction, nur ohne Sound.
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
        case 1: msg += QStringLiteral(" folds."); break;
        case 2: msg += QStringLiteral(" checks."); break;
        case 3: msg += QStringLiteral(" calls $") + QString::number(setValue) + "."; break;
        case 4: msg += QStringLiteral(" bets $") + QString::number(setValue) + "."; break;
        case 5: msg += QStringLiteral(" bets $") + QString::number(setValue) + "."; break;
        case 6: msg += QStringLiteral(" is all in with $") + QString::number(setValue) + "."; break;
        default: msg.clear();
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
        // Wortlaut 1:1 wie guiLog::logNewGameHandMsg im Qt-Widgets-Client.
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
    // Kartenzahl wird – wie im Widgets-Client (guiLog::logDealBoardCardsMsg) –
    // ausschließlich über die Runden-ID bestimmt (Flop=3, Turn=4, River=5).
    // Nicht ausgeteilte Karten kommen als 0 herein (nicht -1), ein Wert-Check
    // würde sie fälschlich als 2♦ einfügen.
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
        // Andere Runden-IDs (Post-River beim All-In-Runout) protokollieren das
        // volle Board erneut – redundant zur River-Zeile → nicht anzeigen.
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
        // Handname anhängen (wie guiLog::logFlipHoleCardsMsg), z. B. - "Straight, six high".
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

    QMetaObject::invokeMethod(gh, "onShowdown", Qt::QueuedConnection);

    // Start the next hand after a pause so the user can see the result
    // (Gewinner + aufgedeckte Karten + Gewinner-Hand etwas länger zeigen).
    //
    // ACHTUNG: Im NETZWERK-Spiel startet der SERVER die nächste Hand über
    // `Type_HandStartMessage` (siehe clientstate.cpp:initHand/startHand).
    // Wenn wir hier nach 5.5s zusätzlich `onNextRoundCleanGui` + initHand +
    // startHand aufrufen, doppeln wir den Hand-Setup. Schlimmer:
    // `onNextRoundCleanGui` ruft synchron `onDisableMyButtons()` auf
    // (gamehandler.cpp:1040). Wenn der Server die neue Hand schneller
    // startet als der Timer abläuft (UTG-Spot → eigene Action liegt fast
    // immer innerhalb 5.5s nach Showdown), feuert dieser Timer MITTEN in
    // den eigenen Zug, setzt `m_myTurn = false` und legt die Aktions-
    // Buttons tot. Resultat: User kann nicht klicken, Server-Timeout
    // führt zum Auto-Fold („die Hand vor meinem BB reagiert nicht auf
    // actions"). Daher: bei Netzwerkspielen überlässt der Client die
    // Hand-Transition komplett dem Server.
    boost::shared_ptr<Session> sessionForTimer = session;
    const bool isNetwork = sessionForTimer && sessionForTimer->isNetworkClientRunning();
    if (isNetwork) {
        return;
    }
    QTimer::singleShot(5500, gh, [gh, sessionForTimer]() {
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
