/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/
#include "serverguiwrapper.h"
#include <session.h>


using namespace std;


ServerGuiWrapper::ServerGuiWrapper(ConfigFile *config, ClientCallback *clientcb, ServerCallback *servercb, IrcCallback *irccb)
	: myConfig(config), myClientcb(clientcb), myServercb(servercb), myIrccb(irccb)
{
}

ServerGuiWrapper::~ServerGuiWrapper()
{
}

void ServerGuiWrapper::initGui(int /*speed*/) {}

boost::shared_ptr<Session> ServerGuiWrapper::getSession()
{
	assert(mySession.get());
	return mySession;
}

void ServerGuiWrapper::setSession(boost::shared_ptr<Session> session)
{
	mySession = session;
}

void ServerGuiWrapper::refreshSet() const {}
void ServerGuiWrapper::refreshCash() const {}
void ServerGuiWrapper::refreshAction(int /*playerID*/, int /*playerAction*/) const {}
void ServerGuiWrapper::refreshChangePlayer() const {}
void ServerGuiWrapper::refreshAll() const {}
void ServerGuiWrapper::refreshPot() const {}
void ServerGuiWrapper::refreshGroupbox(int /*playerID*/, int /*status*/) const {}
void ServerGuiWrapper::refreshPlayerName() const {}
void ServerGuiWrapper::refreshButton() const {}
void ServerGuiWrapper::refreshGameLabels(GameState /*state*/) const {}
void ServerGuiWrapper::setPlayerAvatar(int /*myUniqueID*/, const std::string &/*myAvatar*/) const {};
void ServerGuiWrapper::waitForGuiUpdateDone() const {}
void ServerGuiWrapper::dealBeRoCards(int /*myBeRoID*/) {}
void ServerGuiWrapper::dealHoleCards() {}
void ServerGuiWrapper::dealFlopCards() {}
void ServerGuiWrapper::dealTurnCard() {}
void ServerGuiWrapper::dealRiverCard() {}
void ServerGuiWrapper::nextPlayerAnimation() {}
void ServerGuiWrapper::beRoAnimation2(int /*myBeRoID*/) {}
void ServerGuiWrapper::preflopAnimation1() {}
void ServerGuiWrapper::preflopAnimation2() {}
void ServerGuiWrapper::flopAnimation1() {}
void ServerGuiWrapper::flopAnimation2() {}
void ServerGuiWrapper::turnAnimation1() {}
void ServerGuiWrapper::turnAnimation2() {}
void ServerGuiWrapper::riverAnimation1() {}
void ServerGuiWrapper::riverAnimation2() {}
void ServerGuiWrapper::postRiverAnimation1() {}
void ServerGuiWrapper::postRiverRunAnimation1() {}
void ServerGuiWrapper::flipHolecardsAllIn() {}
void ServerGuiWrapper::nextRoundCleanGui() {}
void ServerGuiWrapper::meInAction() {}
void ServerGuiWrapper::disableMyButtons() {}
void ServerGuiWrapper::updateMyButtonsState() {}
void ServerGuiWrapper::startTimeoutAnimation(int /*playerNum*/, int /*timeoutSec*/) {}
void ServerGuiWrapper::stopTimeoutAnimation(int /*playerNum*/) {}
void ServerGuiWrapper::startVoteOnKick(unsigned /*playerId*/, unsigned /*voteStarterPlayerId*/, int /*timeoutSec*/, int /*numVotesNeededToKick*/) {}
void ServerGuiWrapper::changeVoteOnKickButtonsState(bool /*showHide*/) {}
void ServerGuiWrapper::refreshVotesMonitor(int /*currentVotes*/, int /*numVotesNeededToKick*/) {}
void ServerGuiWrapper::endVoteOnKick() {}
void ServerGuiWrapper::logPlayerActionMsg(string /*playerName*/, int /*action*/, int /*setValue*/) {}
void ServerGuiWrapper::logNewGameHandMsg(int /*gameID*/, int /*handID*/) {}
void ServerGuiWrapper::logPlayerWinsMsg(std::string /*playerName*/, int /*pot*/, bool /*main*/) {}
void ServerGuiWrapper::logPlayerSitsOut(std::string /*playerName*/) {}
void ServerGuiWrapper::logNewBlindsSetsMsg(int /*sbSet*/, int /*bbSet*/, std::string /*sbName*/, std::string /*bbName*/) {}
void ServerGuiWrapper::logDealBoardCardsMsg(int /*roundID*/, int /*card1*/, int /*card2*/, int /*card3*/, int /*card4*/, int /*card5*/) {}
void ServerGuiWrapper::logFlipHoleCardsMsg(std::string /*playerName*/, int /*card1*/, int /*card2*/, int /*cardsValueInt*/, std::string /*showHas*/) {}
void ServerGuiWrapper::logPlayerWinGame(std::string /*playerName*/, int /*gameID*/) {}
void ServerGuiWrapper::flushLogAtGame(int /*gameID*/) {}
void ServerGuiWrapper::flushLogAtHand() {}

void ServerGuiWrapper::SignalNetClientServerListAdd(unsigned serverId)
{
	if (myClientcb) myClientcb->SignalNetClientServerListAdd(serverId);
}
void ServerGuiWrapper::SignalNetClientServerListClear()
{
	if (myClientcb) myClientcb->SignalNetClientServerListClear();
}
void ServerGuiWrapper::SignalNetClientServerListShow()
{
	if (myClientcb) myClientcb->SignalNetClientServerListShow();
}

void ServerGuiWrapper::SignalNetClientLoginShow()
{
	if (myClientcb) myClientcb->SignalNetClientLoginShow();
}

void ServerGuiWrapper::SignalNetClientRejoinPossible(unsigned gameId)
{
	if (myClientcb) myClientcb->SignalNetClientRejoinPossible(gameId);
}

void ServerGuiWrapper::SignalNetClientPostRiverShowCards(unsigned playerId)
{
	if (myClientcb) myClientcb->SignalNetClientPostRiverShowCards(playerId);
}

void ServerGuiWrapper::SignalNetClientConnect(int actionID)
{
	if (myClientcb) myClientcb->SignalNetClientConnect(actionID);
}
void ServerGuiWrapper::SignalNetClientGameInfo(int actionID)
{
	if (myClientcb) myClientcb->SignalNetClientGameInfo(actionID);
}
void ServerGuiWrapper::SignalNetClientError(int errorID, int osErrorID)
{
	if (myClientcb) myClientcb->SignalNetClientError(errorID, osErrorID);
}
void ServerGuiWrapper::SignalNetClientNotification(int notificationId)
{
	if (myClientcb) myClientcb->SignalNetClientNotification(notificationId);
}
void ServerGuiWrapper::SignalNetClientStatsUpdate(const ServerStats &stats)
{
	if (myClientcb) myClientcb->SignalNetClientStatsUpdate(stats);
}
void ServerGuiWrapper::SignalNetClientPingUpdate(unsigned minPing, unsigned avgPing, unsigned maxPing)
{
	if (myClientcb) myClientcb->SignalNetClientPingUpdate(minPing, avgPing, maxPing);
}
void ServerGuiWrapper::SignalNetClientShowTimeoutDialog(NetTimeoutReason reason, unsigned remainingSec)
{
	if (myClientcb) myClientcb->SignalNetClientShowTimeoutDialog(reason, remainingSec);
}
void ServerGuiWrapper::SignalNetClientRemovedFromGame(int notificationId)
{
	if (myClientcb) myClientcb->SignalNetClientRemovedFromGame(notificationId);
}
void ServerGuiWrapper::SignalNetClientSelfJoined(unsigned playerId, const string &playerName, bool isGameAdmin)
{
	if (myClientcb) myClientcb->SignalNetClientSelfJoined(playerId, playerName, isGameAdmin);
}
void ServerGuiWrapper::SignalNetClientPlayerJoined(unsigned playerId, const string &playerName, bool isGameAdmin)
{
	if (myClientcb) myClientcb->SignalNetClientPlayerJoined(playerId, playerName, isGameAdmin);
}
void ServerGuiWrapper::SignalNetClientPlayerChanged(unsigned playerId, const string &newPlayerName)
{
	if (myClientcb) myClientcb->SignalNetClientPlayerChanged(playerId, newPlayerName);
}
void ServerGuiWrapper::SignalNetClientPlayerLeft(unsigned playerId, const string &playerName, int removeReason)
{
	if (myClientcb) myClientcb->SignalNetClientPlayerLeft(playerId, playerName, removeReason);
}
void ServerGuiWrapper::SignalNetClientSpectatorJoined(unsigned playerId, const string &playerName)
{
	if (myClientcb) myClientcb->SignalNetClientSpectatorJoined(playerId, playerName);
}
void ServerGuiWrapper::SignalNetClientSpectatorLeft(unsigned playerId, const string &playerName, int removeReason)
{
	if (myClientcb) myClientcb->SignalNetClientSpectatorLeft(playerId, playerName, removeReason);
}
void ServerGuiWrapper::SignalNetClientNewGameAdmin(unsigned playerId, const string &playerName)
{
	if (myClientcb) myClientcb->SignalNetClientNewGameAdmin(playerId, playerName);
}
void ServerGuiWrapper::SignalNetClientGameListNew(unsigned gameId)
{
	if (myClientcb) myClientcb->SignalNetClientGameListNew(gameId);
}
void ServerGuiWrapper::SignalNetClientGameListRemove(unsigned gameId)
{
	if (myClientcb) myClientcb->SignalNetClientGameListRemove(gameId);
}
void ServerGuiWrapper::SignalNetClientGameListUpdateMode(unsigned gameId, GameMode mode)
{
	if (myClientcb) myClientcb->SignalNetClientGameListUpdateMode(gameId, mode);
}
void ServerGuiWrapper::SignalNetClientGameListUpdateAdmin(unsigned gameId, unsigned adminPlayerId)
{
	if (myClientcb) myClientcb->SignalNetClientGameListUpdateAdmin(gameId, adminPlayerId);
}
void ServerGuiWrapper::SignalNetClientGameListPlayerJoined(unsigned gameId, unsigned playerId)
{
	if (myClientcb) myClientcb->SignalNetClientGameListPlayerJoined(gameId, playerId);
}
void ServerGuiWrapper::SignalNetClientGameListPlayerLeft(unsigned gameId, unsigned playerId)
{
	if (myClientcb) myClientcb->SignalNetClientGameListPlayerLeft(gameId, playerId);
}
void ServerGuiWrapper::SignalNetClientGameListSpectatorJoined(unsigned gameId, unsigned playerId)
{
	if (myClientcb) myClientcb->SignalNetClientGameListSpectatorJoined(gameId, playerId);
}
void ServerGuiWrapper::SignalNetClientGameListSpectatorLeft(unsigned gameId, unsigned playerId)
{
	if (myClientcb) myClientcb->SignalNetClientGameListSpectatorLeft(gameId, playerId);
}
void ServerGuiWrapper::SignalNetClientGameStart(boost::shared_ptr<Game> game)
{
	if (myClientcb) myClientcb->SignalNetClientGameStart(game);
}
void ServerGuiWrapper::SignalNetClientGameChatMsg(const string &playerName, const string &msg)
{
	if (myClientcb) myClientcb->SignalNetClientGameChatMsg(playerName, msg);
}
void ServerGuiWrapper::SignalNetClientLobbyChatMsg(const string &playerName, const string &msg)
{
	if (myClientcb) myClientcb->SignalNetClientLobbyChatMsg(playerName, msg);
}
void ServerGuiWrapper::SignalNetClientPrivateChatMsg(const string &playerName, const string &msg)
{
	if (myClientcb) myClientcb->SignalNetClientPrivateChatMsg(playerName, msg);
}
void ServerGuiWrapper::SignalNetClientMsgBox(const string &msg)
{
	if (myClientcb) myClientcb->SignalNetClientMsgBox(msg);
}
void ServerGuiWrapper::SignalNetClientMsgBox(unsigned msgId)
{
	if (myClientcb) myClientcb->SignalNetClientMsgBox(msgId);
}
void ServerGuiWrapper::SignalNetClientWaitDialog()
{
	if (myClientcb) myClientcb->SignalNetClientWaitDialog();
}
void ServerGuiWrapper::SignalNetServerSuccess(int actionID)
{
	if (myServercb) myServercb->SignalNetServerSuccess(actionID);
}
void ServerGuiWrapper::SignalNetServerError(int errorID, int osErrorID)
{
	if (myServercb) myServercb->SignalNetServerError(errorID, osErrorID);
}

void ServerGuiWrapper::SignalIrcConnect(const string &server)
{
	if (myIrccb) myIrccb->SignalIrcConnect(server);
}
void ServerGuiWrapper::SignalIrcSelfJoined(const string &nickName, const std::string &channel)
{
	if (myIrccb) myIrccb->SignalIrcSelfJoined(nickName, channel);
}
void ServerGuiWrapper::SignalIrcPlayerJoined(const string &nickName)
{
	if (myIrccb) myIrccb->SignalIrcPlayerJoined(nickName);
}
void ServerGuiWrapper::SignalIrcPlayerChanged(const string &oldNick, const string &newNick)
{
	if (myIrccb) myIrccb->SignalIrcPlayerChanged(oldNick, newNick);
}
void ServerGuiWrapper::SignalIrcPlayerKicked(const string &nickName, const string &byWhom, const string &reason)
{
	if (myIrccb) myIrccb->SignalIrcPlayerKicked(nickName, byWhom, reason);
}
void ServerGuiWrapper::SignalIrcPlayerLeft(const string &nickName)
{
	if (myIrccb) myIrccb->SignalIrcPlayerLeft(nickName);
}
void ServerGuiWrapper::SignalIrcChatMsg(const string &nickName, const string &msg)
{
	if (myIrccb) myIrccb->SignalIrcChatMsg(nickName, msg);
}
void ServerGuiWrapper::SignalIrcError(int errorCode)
{
	if (myIrccb) myIrccb->SignalIrcError(errorCode);
}
void ServerGuiWrapper::SignalIrcServerError(int errorCode)
{
	if (myIrccb) myIrccb->SignalIrcServerError(errorCode);
}

void ServerGuiWrapper::SignalLobbyPlayerJoined(unsigned playerId, const std::string &nickName)
{
	if (myClientcb) myClientcb->SignalLobbyPlayerJoined(playerId, nickName);
}
void ServerGuiWrapper::SignalLobbyPlayerKicked(const std::string &nickName, const std::string &byWhom, const std::string &reason)
{
	if (myClientcb) myClientcb->SignalLobbyPlayerKicked(nickName, byWhom, reason);
}
void ServerGuiWrapper::SignalLobbyPlayerLeft(unsigned playerId)
{
	if (myClientcb) myClientcb->SignalLobbyPlayerLeft(playerId);
}

void ServerGuiWrapper::SignalSelfGameInvitation(unsigned gameId, unsigned playerIdFrom)
{
	if (myClientcb) myClientcb->SignalSelfGameInvitation(gameId, playerIdFrom);
}
void ServerGuiWrapper::SignalPlayerGameInvitation(unsigned gameId, unsigned playerIdWho, unsigned playerIdFrom)
{
	if (myClientcb) myClientcb->SignalPlayerGameInvitation(gameId, playerIdWho, playerIdFrom);
}
void ServerGuiWrapper::SignalRejectedGameInvitation(unsigned gameId, unsigned playerIdWho, DenyGameInvitationReason reason)
{
	if (myClientcb) myClientcb->SignalRejectedGameInvitation(gameId, playerIdWho, reason);
}
