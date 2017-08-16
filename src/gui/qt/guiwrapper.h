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
#ifndef GUIWRAPPER_H
#define GUIWRAPPER_H

#include <guiinterface.h>

#include <string>

class Session;
class gameTableImpl;
class startWindowImpl;
class guiLog;
class ConfigFile;


class GuiWrapper : public GuiInterface
{
public:
	GuiWrapper(ConfigFile*, startWindowImpl*);

	~GuiWrapper();

	void initGui(int speed);

	boost::shared_ptr<Session> getSession();
	void setSession(boost::shared_ptr<Session> session);

	gameTableImpl* getMyW() const
	{
		return myW;
	}
	guiLog* getMyGuiLog() const
	{
		return myGuiLog;
	}

	void refreshSet() const;
	void refreshCash() const;
	void refreshAction(int =-1, int =-1) const;
	void refreshChangePlayer() const;
	void refreshPot() const;
	void refreshGroupbox(int =-1, int =-1) const;
	void refreshAll() const;
	void refreshPlayerName() const;
	void refreshButton() const;
	void refreshGameLabels(GameState state) const;

	void setPlayerAvatar(int myUniqueID, const std::string &myAvatar) const;

	void waitForGuiUpdateDone() const;

	void dealBeRoCards(int myBeRoID);
	void dealHoleCards();
	void dealFlopCards();
	void dealTurnCard();
	void dealRiverCard();

	void nextPlayerAnimation();

	void beRoAnimation2(int);

	void preflopAnimation1();
	void preflopAnimation2();

	void flopAnimation1();
	void flopAnimation2();

	void turnAnimation1();
	void turnAnimation2();

	void riverAnimation1();
	void riverAnimation2();

	void postRiverAnimation1();
	void postRiverRunAnimation1();

	void flipHolecardsAllIn();

	void nextRoundCleanGui();

	void meInAction();
	void disableMyButtons();
	void updateMyButtonsState();
	void startTimeoutAnimation(int playerNum, int timeoutSec);
	void stopTimeoutAnimation(int playerNum);

	void startVoteOnKick(unsigned playerId, unsigned voteStarterPlayerId, int timeoutSec, int numVotesNeededToKick);
	void changeVoteOnKickButtonsState(bool showHide);
	void refreshVotesMonitor(int currentVotes, int numVotesNeededToKick);
	void endVoteOnKick();

	void logPlayerActionMsg(std::string playerName, int action, int setValue) ;
	void logNewGameHandMsg(int gameID, int handID) ;
	void logNewBlindsSetsMsg(int sbSet, int bbSet, std::string sbName, std::string bbName);
	void logPlayerWinsMsg(std::string playerName, int pot, bool main);
	void logPlayerSitsOut(std::string playerName);
	void logDealBoardCardsMsg(int roundID, int card1, int card2, int card3, int card4 = -1, int card5 = -1);
	void logFlipHoleCardsMsg(std::string playerName, int card1, int card2, int cardsValueInt = -1, std::string showHas = "shows");
	void logPlayerWinGame(std::string playerName, int gameID);
	void flushLogAtGame(int gameID);
	void flushLogAtHand();

	void SignalNetClientConnect(int actionID);
	void SignalNetClientServerListAdd(unsigned serverId);
	void SignalNetClientServerListShow();
	void SignalNetClientServerListClear();
	void SignalNetClientLoginShow();
	void SignalNetClientRejoinPossible(unsigned gameId);
	void SignalNetClientPostRiverShowCards(unsigned playerId);
	void SignalNetClientGameInfo(int actionID);
	void SignalNetClientError(int errorID, int osErrorID);
	void SignalNetClientNotification(int notificationId);
	void SignalNetClientStatsUpdate(const ServerStats &stats);
	void SignalNetClientPingUpdate(unsigned minPing, unsigned avgPing, unsigned maxPing);
	void SignalNetClientShowTimeoutDialog(NetTimeoutReason reason, unsigned remainingSec);
	void SignalNetClientRemovedFromGame(int notificationId);
	void SignalNetClientSelfJoined(unsigned playerId, const std::string &playerName, bool isGameAdmin);
	void SignalNetClientPlayerJoined(unsigned playerId, const std::string &playerName, bool isGameAdmin);
	void SignalNetClientPlayerChanged(unsigned playerId, const std::string &newPlayerName);
	void SignalNetClientPlayerLeft(unsigned playerId, const std::string &playerName, int removeReason);
	void SignalNetClientSpectatorJoined(unsigned playerId, const std::string &playerName);
	void SignalNetClientSpectatorLeft(unsigned playerId, const std::string &playerName, int removeReason);
	void SignalNetClientNewGameAdmin(unsigned playerId, const std::string &playerName);
	void SignalNetClientGameChatMsg(const std::string &playerName, const std::string &msg);
	void SignalNetClientLobbyChatMsg(const std::string &playerName, const std::string &msg);
	void SignalNetClientPrivateChatMsg(const std::string &playerName, const std::string &msg);
	void SignalNetClientMsgBox(const std::string &msg);
	void SignalNetClientMsgBox(unsigned msgId);

	void SignalNetClientWaitDialog();

	void SignalNetClientGameListNew(unsigned gameId);
	void SignalNetClientGameListRemove(unsigned gameId);
	void SignalNetClientGameListUpdateMode(unsigned gameId, GameMode mode);
	void SignalNetClientGameListUpdateAdmin(unsigned gameId, unsigned adminPlayerId);
	void SignalNetClientGameListPlayerJoined(unsigned gameId, unsigned playerId);
	void SignalNetClientGameListPlayerLeft(unsigned gameId, unsigned playerId);
	void SignalNetClientGameListSpectatorJoined(unsigned gameId, unsigned playerId);
	void SignalNetClientGameListSpectatorLeft(unsigned gameId, unsigned playerId);

	void SignalNetClientGameStart(boost::shared_ptr<Game> game);

	void SignalNetServerSuccess(int actionID);
	void SignalNetServerError(int errorID, int osErrorID);

	void SignalIrcConnect(const std::string &server);
	void SignalIrcSelfJoined(const std::string &nickName, const std::string &channel);
	void SignalIrcPlayerJoined(const std::string &nickName);
	void SignalIrcPlayerChanged(const std::string &oldNick, const std::string &newNick);
	void SignalIrcPlayerKicked(const std::string &nickName, const std::string &byWhom, const std::string &reason);
	void SignalIrcPlayerLeft(const std::string &nickName);
	void SignalIrcChatMsg(const std::string &nickName, const std::string &msg);
	void SignalIrcError(int errorCode);
	void SignalIrcServerError(int errorCode);
	void SignalLobbyPlayerJoined(unsigned playerId, const std::string &nickName);
	void SignalLobbyPlayerKicked(const std::string &nickName, const std::string &byWhom, const std::string &reason);
	void SignalLobbyPlayerLeft(unsigned playerId);

	void SignalSelfGameInvitation(unsigned gameId, unsigned playerIdFrom);
	void SignalPlayerGameInvitation(unsigned gameId, unsigned playerIdWho, unsigned playerIdFrom);
	void SignalRejectedGameInvitation(unsigned gameId, unsigned playerIdWho, DenyGameInvitationReason reason);

private:

	guiLog *myGuiLog;
	gameTableImpl *myW;
	ConfigFile *myConfig;
	startWindowImpl *myStartWindow;

};

#endif
