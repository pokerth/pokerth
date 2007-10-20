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
class IrcThread;
class AvatarManager;

class Session{
public:

	Session(GuiInterface*, ConfigFile*);

	~Session();

	enum GameType { GAME_TYPE_NONE, GAME_TYPE_LOCAL, GAME_TYPE_NETWORK, GAME_TYPE_INTERNET };

	// Only one of the two inits should be called.
	bool init();
	void init(boost::shared_ptr<AvatarManager> manager);

	void startLocalGame(const GameData &gameData, const StartData &startData);
	void startClientGame(boost::shared_ptr<Game> game);

	Game *getCurrentGame();
	GuiInterface *getGui();
	GameType getGameType();

	boost::shared_ptr<AvatarManager> getAvatarManager();

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
	bool waitForNetworkServer(unsigned timeoutMsec);

	void sendIrcChatMessage(const std::string &message);

	void sendClientPlayerAction();

	void setCurrentGameID(int theValue) { currentGameID = theValue; }
	int getCurrentGameID() const { return currentGameID; }

	void sendChatMessage(const std::string &message);
	void kickPlayer(unsigned playerId);

	bool isNetworkClientRunning() const; // TODO hack
	bool isNetworkServerRunning() const; // TODO hack

	GameInfo getClientGameInfo(unsigned gameId);
	PlayerInfo getClientPlayerInfo(unsigned playerId);
	ServerStats getClientStats();

private:

	int currentGameID;

	ClientThread *myNetClient;
	ServerAcceptThread *myNetServer;
	IrcThread *myIrcThread;
	boost::shared_ptr<AvatarManager> myAvatarManager;

	boost::shared_ptr<Game> currentGame;
	GuiInterface *myGui;
	ConfigFile *myConfig;
	GameType myGameType;
};



#endif
