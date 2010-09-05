/***************************************************************************
 *   Copyright (C) 2006 by FThauer FHammer   *
 *   f.thauer@web.de   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "guiwrapper.h"
#include "session.h"
#include "log.h"
#include "gametableimpl.h"
#include "startwindowimpl.h"
#include "configfile.h"
#include <net/socket_msg.h>


using namespace std;


GuiWrapper::GuiWrapper(ConfigFile *c, startWindowImpl *s) : myLog(NULL), myW(NULL), myConfig(c), myStartWindow(s)
{


    myW = new gameTableImpl(myConfig);
    myLog = new Log(myW, myConfig);

    myW->setStartWindow(myStartWindow);
}


GuiWrapper::~GuiWrapper()
{
    delete myLog;

}

void GuiWrapper::initGui(int speed) { myW->signalInitGui(speed); }

boost::shared_ptr<Session> GuiWrapper::getSession() { return myStartWindow->getSession(); }
void GuiWrapper::setSession(boost::shared_ptr<Session> /*session*/) { /*myStartWindow->setSession(session);*/ }

void GuiWrapper::refreshSet() const { myW->signalRefreshSet(); }
void GuiWrapper::refreshCash() const { myW->signalRefreshCash(); }
void GuiWrapper::refreshAction(int playerID, int playerAction) const { myW->signalRefreshAction(playerID, playerAction); }
void GuiWrapper::refreshChangePlayer() const { myW->signalRefreshChangePlayer(); }
void GuiWrapper::refreshAll() const { myW->signalRefreshAll(); }
void GuiWrapper::refreshPot() const { myW->signalRefreshPot(); }
void GuiWrapper::refreshGroupbox(int playerID, int status) const { myW->signalRefreshGroupbox(playerID, status); }
void GuiWrapper::refreshPlayerName() const { myW->signalRefreshPlayerName(); }
void GuiWrapper::refreshButton() const { myW->signalRefreshButton(); }
void GuiWrapper::refreshGameLabels(GameState state) const { myW->signalRefreshGameLabels(state); }

void GuiWrapper::setPlayerAvatar(int myUniqueID, const std::string &myAvatar) const { myW->signalSetPlayerAvatar(myUniqueID, QString::fromUtf8(myAvatar.c_str())); }


void GuiWrapper::waitForGuiUpdateDone() const
{
    myW->signalGuiUpdateDone();
    myW->waitForGuiUpdateDone();
}

void GuiWrapper::dealBeRoCards(int myBeRoID) { myW->signalDealBeRoCards(myBeRoID); }

void GuiWrapper::dealHoleCards() { myW->signalDealHoleCards(); }
void GuiWrapper::dealFlopCards() { myW->signalDealFlopCards0(); }
void GuiWrapper::dealTurnCard() { myW->signalDealTurnCards0(); }
void GuiWrapper::dealRiverCard() { myW->signalDealRiverCards0(); }

void GuiWrapper::nextPlayerAnimation() { myW->signalNextPlayerAnimation(); }

void GuiWrapper::beRoAnimation2(int myBeRoID) { myW->signalBeRoAnimation2(myBeRoID); }

void GuiWrapper::preflopAnimation1() { myW->signalPreflopAnimation1(); }
void GuiWrapper::preflopAnimation2() { myW->signalPreflopAnimation2(); }

void GuiWrapper::flopAnimation1() { myW->signalFlopAnimation1(); }
void GuiWrapper::flopAnimation2() { myW->signalFlopAnimation2(); }

void GuiWrapper::turnAnimation1() { myW->signalTurnAnimation1(); }
void GuiWrapper::turnAnimation2() { myW->signalTurnAnimation2(); }

void GuiWrapper::riverAnimation1() { myW->signalRiverAnimation1(); }
void GuiWrapper::riverAnimation2() { myW->signalRiverAnimation2(); }

void GuiWrapper::postRiverAnimation1() { myW->signalPostRiverAnimation1(); }
void GuiWrapper::postRiverRunAnimation1() { myW->signalPostRiverRunAnimation1(); }

void GuiWrapper::flipHolecardsAllIn() { myW->signalFlipHolecardsAllIn(); }

void GuiWrapper::nextRoundCleanGui() { myW->signalNextRoundCleanGui(); }

void GuiWrapper::meInAction() { myW->signalMeInAction(); }
void GuiWrapper::updateMyButtonsState() { myW->signalUpdateMyButtonsState(); }
void GuiWrapper::disableMyButtons() { myW->signalDisableMyButtons(); }
void GuiWrapper::startTimeoutAnimation(int playerNum, int timeoutSec) { myW->signalStartTimeoutAnimation(playerNum, timeoutSec); }
void GuiWrapper::stopTimeoutAnimation(int playerNum) { myW->signalStopTimeoutAnimation(playerNum); }

void GuiWrapper::startVoteOnKick(unsigned playerId, unsigned voteStarterPlayerId, int timeoutSec, int numVotesNeededToKick) { myW->signalStartVoteOnKick(playerId, voteStarterPlayerId, timeoutSec, numVotesNeededToKick); }
void GuiWrapper::changeVoteOnKickButtonsState(bool showHide) { myW->signalChangeVoteOnKickButtonsState(showHide); }
void GuiWrapper::refreshVotesMonitor(int currentVotes, int numVotesNeededToKick) { myW->refreshVotesMonitor(currentVotes, numVotesNeededToKick); }
void GuiWrapper::endVoteOnKick() { myW->signalEndVoteOnKick(); }

void GuiWrapper::logPlayerActionMsg(string playerName, int playerID, int action, int setValue) { myLog->signalLogPlayerActionMsg(QString::fromUtf8(playerName.c_str()), playerID, action, setValue); }
void GuiWrapper::logNewGameHandMsg(int gameID, int handID) { myLog->signalLogNewGameHandMsg(gameID, handID); }
void GuiWrapper::logNewBlindsSetsMsg(int sbSet, int bbSet, std::string sbName, std::string bbName) { myLog->signalLogNewBlindsSetsMsg(sbSet, bbSet, QString::fromUtf8(sbName.c_str()), QString::fromUtf8(bbName.c_str())); }
void GuiWrapper::logPlayerWinsMsg(std::string playerName, int pot, bool main) { myLog->signalLogPlayerWinsMsg(QString::fromUtf8(playerName.c_str()), pot, main); }
void GuiWrapper::logPlayerSitsOut(std::string playerName) { myLog->signalLogPlayerSitsOut(QString::fromUtf8(playerName.c_str())); }
void GuiWrapper::logDealBoardCardsMsg(int roundID, int card1, int card2, int card3, int card4, int card5) { myLog->signalLogDealBoardCardsMsg(roundID, card1, card2, card3, card4, card5); }
void GuiWrapper::logFlipHoleCardsMsg(string playerName, int playerID, int card1, int card2, int cardsValueInt, string showHas) { myLog->signalLogFlipHoleCardsMsg(QString::fromUtf8(playerName.c_str()), playerID, card1, card2, cardsValueInt, QString::fromUtf8(showHas.c_str())); }
void GuiWrapper::logPlayerWinGame(std::string playerName, int gameID) { myLog->signalLogPlayerWinGame(QString::fromUtf8(playerName.c_str()), gameID); }
void GuiWrapper::flushLogAtGame(int gameID) { myLog->signalFlushLogAtGame(gameID); }
void GuiWrapper::flushLogAtHand() { myLog->signalFlushLogAtHand(); }


void GuiWrapper::SignalNetClientConnect(int actionID) { myStartWindow->signalNetClientConnect(actionID); }
void GuiWrapper::SignalNetClientServerListAdd(unsigned serverId) { myStartWindow->signalNetClientServerListAdd(serverId); }
void GuiWrapper::SignalNetClientServerListShow() { myStartWindow->signalNetClientServerListShow(); }
void GuiWrapper::SignalNetClientLoginShow() { myStartWindow->signalNetClientLoginShow(); }
void GuiWrapper::SignalNetClientPostRiverShowCards(unsigned playerId) { myW->signalPostRiverShowCards(playerId); }
void GuiWrapper::SignalNetClientServerListClear() { myStartWindow->signalNetClientServerListClear(); }
void GuiWrapper::SignalNetClientGameInfo(int actionID) { myStartWindow->signalNetClientGameInfo(actionID); }
void GuiWrapper::SignalNetClientError(int errorID, int osErrorID) { myStartWindow->signalNetClientError(errorID, osErrorID); }
void GuiWrapper::SignalNetClientNotification(int notificationId) { myStartWindow->signalNetClientNotification(notificationId); }
void GuiWrapper::SignalNetClientStatsUpdate(const ServerStats &stats) { myStartWindow->signalNetClientStatsUpdate(stats); }
void GuiWrapper::SignalNetClientShowTimeoutDialog(NetTimeoutReason reason, unsigned remainingSec) { myStartWindow->signalNetClientShowTimeoutDialog(reason, remainingSec); }
void GuiWrapper::SignalNetClientRemovedFromGame(int notificationId) { myStartWindow->signalNetClientRemovedFromGame(notificationId); }
void GuiWrapper::SignalNetClientSelfJoined(unsigned playerId, const string &playerName, bool isGameAdmin) { myStartWindow->signalNetClientSelfJoined(playerId, QString::fromUtf8(playerName.c_str()), isGameAdmin); }
void GuiWrapper::SignalNetClientPlayerJoined(unsigned playerId, const string &playerName, bool isGameAdmin) { myStartWindow->signalNetClientPlayerJoined(playerId, QString::fromUtf8(playerName.c_str()), isGameAdmin); }
void GuiWrapper::SignalNetClientPlayerChanged(unsigned playerId, const string &newPlayerName)
{
    myStartWindow->signalNetClientPlayerChanged(playerId, QString::fromUtf8(newPlayerName.c_str()));
}
void GuiWrapper::SignalNetClientPlayerLeft(unsigned playerId, const string &playerName, int removeReason)
{
    QString tmpName(QString::fromUtf8(playerName.c_str()));
    myStartWindow->signalNetClientPlayerLeft(playerId, tmpName);
    myW->signalNetClientPlayerLeft(playerId);
    if (!playerName.empty() && playerName[0] != '#')
        myLog->signalLogPlayerLeftMsg(tmpName, removeReason == NTF_NET_REMOVED_KICKED);
}
void GuiWrapper::SignalNetClientNewGameAdmin(unsigned playerId, const string &playerName) { 

    myStartWindow->signalNetClientNewGameAdmin(playerId, QString::fromUtf8(playerName.c_str()));
    if (!playerName.empty() && playerName[0] != '#')
        myLog->signalLogNewGameAdminMsg(QString::fromUtf8(playerName.c_str()));

}

void GuiWrapper::SignalNetClientGameListNew(unsigned gameId) { myStartWindow->signalNetClientGameListNew(gameId); }
void GuiWrapper::SignalNetClientGameListRemove(unsigned gameId) { myStartWindow->signalNetClientGameListRemove(gameId); }
void GuiWrapper::SignalNetClientGameListUpdateMode(unsigned gameId, GameMode mode) { myStartWindow->signalNetClientGameListUpdateMode(gameId, mode); }
void GuiWrapper::SignalNetClientGameListUpdateAdmin(unsigned gameId, unsigned adminPlayerId) {myStartWindow->signalNetClientGameListUpdateAdmin(gameId, adminPlayerId); }
void GuiWrapper::SignalNetClientGameListPlayerJoined(unsigned gameId, unsigned playerId) { myStartWindow->signalNetClientGameListPlayerJoined(gameId, playerId); }
void GuiWrapper::SignalNetClientGameListPlayerLeft(unsigned gameId, unsigned playerId) { myStartWindow->signalNetClientGameListPlayerLeft(gameId, playerId); }
void GuiWrapper::SignalNetClientGameStart(boost::shared_ptr<Game> game) { myStartWindow->signalNetClientGameStart(game); }

void GuiWrapper::SignalNetClientWaitDialog() { myStartWindow->signalShowClientDialog(); }
void GuiWrapper::SignalNetClientWarningAutoFoldInRankingGame(unsigned remainingAutoFolds) { myW->signalWarningAutoFoldInRankingGame(remainingAutoFolds); }

void GuiWrapper::SignalNetClientGameChatMsg(const string &playerName, const string &msg) { myStartWindow->signalNetClientGameChatMsg(QString::fromUtf8(playerName.c_str()), QString::fromUtf8(msg.c_str())); }
void GuiWrapper::SignalNetClientLobbyChatMsg(const string &playerName, const string &msg) { myStartWindow->signalNetClientLobbyChatMsg(QString::fromUtf8(playerName.c_str()), QString::fromUtf8(msg.c_str())); }
void GuiWrapper::SignalNetClientMsgBox(const string &msg) { myStartWindow->signalNetClientMsgBox(QString::fromUtf8(msg.c_str())); }

void GuiWrapper::SignalNetServerSuccess(int /*actionID*/) { }
void GuiWrapper::SignalNetServerError(int errorID, int osErrorID) { myStartWindow->signalNetServerError(errorID, osErrorID); }

void GuiWrapper::SignalIrcConnect(const string &/*server*/) { /*myStartWindow->signalIrcConnect(QString::fromUtf8(server.c_str()));*/ }
void GuiWrapper::SignalIrcSelfJoined(const string &/*nickName*/, const string &/*channel*/) { /*myStartWindow->signalIrcSelfJoined(QString::fromUtf8(nickName.c_str()), QString::fromUtf8(channel.c_str()));*/ }
void GuiWrapper::SignalIrcPlayerJoined(const string &/*nickName*/) { /*myStartWindow->signalIrcPlayerJoined(QString::fromUtf8(nickName.c_str()));*/ }
void GuiWrapper::SignalIrcPlayerChanged(const string &/*oldNick*/, const string &/*newNick*/) {/* myStartWindow->signalIrcPlayerChanged(QString::fromUtf8(oldNick.c_str()), QString::fromUtf8(newNick.c_str()));*/ }
void GuiWrapper::SignalIrcPlayerKicked(const std::string &/*nickName*/, const std::string &/*byWhom*/, const std::string &/*reason*/) { /*myStartWindow->signalIrcPlayerKicked(QString::fromUtf8(nickName.c_str()), QString::fromUtf8(byWhom.c_str()), QString::fromUtf8(reason.c_str()));*/ }
void GuiWrapper::SignalIrcPlayerLeft(const std::string &/*nickName*/) { /*myStartWindow->signalIrcPlayerLeft(QString::fromUtf8(nickName.c_str()));*/ }
void GuiWrapper::SignalIrcChatMsg(const std::string &/*nickName*/, const std::string &/*msg*/) { /*myStartWindow->signalIrcChatMessage(QString::fromUtf8(nickName.c_str()), QString::fromUtf8(msg.c_str()));*/ }
void GuiWrapper::SignalIrcError(int /*errorCode*/) { /*myStartWindow->signalIrcError(errorCode);*/ }
void GuiWrapper::SignalIrcServerError(int /*errorCode*/) {/*myStartWindow->signalIrcServerError(errorCode);*/ }

void GuiWrapper::SignalLobbyPlayerJoined(unsigned playerId, const string &nickName) { myStartWindow->signalLobbyPlayerJoined(playerId, QString::fromUtf8(nickName.c_str())); }
void GuiWrapper::SignalLobbyPlayerKicked(const std::string &nickName, const std::string &byWhom, const std::string &reason) { myStartWindow->signalLobbyPlayerKicked(QString::fromUtf8(nickName.c_str()), QString::fromUtf8(byWhom.c_str()), QString::fromUtf8(reason.c_str())); }
void GuiWrapper::SignalLobbyPlayerLeft(unsigned playerId) { myStartWindow->signalLobbyPlayerLeft(playerId); }

void GuiWrapper::SignalSelfGameInvitation(unsigned gameId, unsigned playerIdFrom) { myStartWindow->signalSelfGameInvitation(gameId, playerIdFrom); }
void GuiWrapper::SignalPlayerGameInvitation(unsigned gameId, unsigned playerIdWho, unsigned playerIdFrom) { myStartWindow->signalPlayerGameInvitation(gameId, playerIdWho, playerIdFrom); }
void GuiWrapper::SignalRejectedGameInvitation(unsigned gameId, unsigned playerIdWho, DenyGameInvitationReason reason) { myStartWindow->signalRejectedGameInvitation(gameId, playerIdWho, reason); }
