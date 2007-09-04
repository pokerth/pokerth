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
#ifndef GUIWRAPPER_H
#define GUIWRAPPER_H

#include <guiinterface.h>

#include <string>

class Session;
class mainWindowImpl;
class Log;
class Chat;
class ConfigFile;

class GuiWrapper : public GuiInterface
{
public:
	GuiWrapper(ConfigFile*);

	~GuiWrapper();

	void initGui(int speed);

	Session &getSession();
	void setSession(boost::shared_ptr<Session> session);

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
	void startTimeoutAnimation(int playerId, int timeoutSec);
	void stopTimeoutAnimation(int playerId);

	void logPlayerActionMsg(std::string playerName, int action, int setValue) ;
	void logNewGameHandMsg(int gameID, int handID) ;
	void logPlayerWinsMsg(int playerID, int pot);
	void logDealBoardCardsMsg(int roundID, int card1, int card2, int card3, int card4 = -1, int card5 = -1);
	void logFlipHoleCardsMsg(std::string playerName, int card1, int card2, int cardsValueInt = -1, std::string showHas = "shows");
	void logPlayerWinGame(std::string playerName, int gameID);

	void SignalNetClientConnect(int actionID);
	void SignalNetClientGameInfo(int actionID);
	void SignalNetClientError(int errorID, int osErrorID);
	void SignalNetClientSelfJoined(unsigned playerId, const std::string &playerName, PlayerRights rights);
	void SignalNetClientPlayerJoined(unsigned playerId, const std::string &playerName, PlayerRights rights);
	void SignalNetClientPlayerChanged(unsigned playerId, const std::string &newPlayerName);
	void SignalNetClientPlayerLeft(unsigned playerId, const std::string &playerName);
	void SignalNetClientChatMsg(const std::string &playerName, const std::string &msg);
	void SignalNetClientWaitDialog();

	void SignalNetClientGameListNew(unsigned gameId);
	void SignalNetClientGameListRemove(unsigned gameId);
	void SignalNetClientGameListPlayerJoined(unsigned gameId, unsigned playerId);
	void SignalNetClientGameListPlayerLeft(unsigned gameId, unsigned playerId);

	void SignalNetClientGameStart(boost::shared_ptr<Game> game);

	void SignalNetServerSuccess(int actionID);
	void SignalNetServerError(int errorID, int osErrorID);

private:

	Log *myLog;
	Chat *myChat;
	mainWindowImpl *myW;
	ConfigFile *myConfig;

};

#endif
