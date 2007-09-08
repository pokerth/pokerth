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
#ifndef STDSESSION_H
#define STDSESSION_H
#include "gamedata.h"
#include "playerdata.h"
#include "game_defs.h"
#include <string>
#include <boost/shared_ptr.hpp>

class GuiInterface;
class Game;
class ConfigFile;
class ClientThread;
class ServerAcceptThread;
class AvatarManager;

class Session{
public:

	Session(GuiInterface*, ConfigFile*);

	~Session();

	void startLocalGame(const GameData &gameData, const StartData &startData);
	void startClientGame(boost::shared_ptr<Game> game);

	Game *getCurrentGame();

	GuiInterface *getGui();

	void startInternetClient();
	void startNetworkClient(const std::string &serverAddress, unsigned serverPort, bool ipv6, bool sctp, const std::string &pwd);
	void startNetworkClientForLocalServer(const GameData &gameData);
	void terminateNetworkClient();
	void clientCreateGame(const GameData &gameData, const std::string &name, const std::string &password);
	void clientJoinGame(unsigned gameId, const std::string &password);

	void startNetworkServer();
	void sendLeaveCurrentGame();
	void sendStartEvent(bool fillUpWithCpuPlayers);
	void terminateNetworkServer();
	void waitForNetworkServer(unsigned timeoutMsec);

	void sendClientPlayerAction();

	void setCurrentGameID(int theValue) { currentGameID = theValue; }
	int getCurrentGameID() const { return currentGameID; }

	void sendChatMessage(const std::string &message);
	void kickPlayer(unsigned playerId);

	bool isNetworkClientRunning() const; // TODO hack
	bool isNetworkServerRunning() const; // TODO hack

	GameInfo getClientGameInfo(unsigned gameId);
	PlayerInfo getClientPlayerInfo(unsigned playerId);

private:

	int currentGameID;

	ClientThread *myNetClient;
	ServerAcceptThread *myNetServer;
	boost::shared_ptr<AvatarManager> myAvatarManager;

	boost::shared_ptr<Game> currentGame;
	GuiInterface *myGui;
	ConfigFile *myConfig;
};



#endif
