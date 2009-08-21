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
#include <boost/asio.hpp>
#include <net/socket_helper.h>
#include "session.h"
#include "game.h"
#include "guiinterface.h"
#include "configfile.h"
#include <qttoolsinterface.h>
#include <localenginefactory.h>
#include <clientenginefactory.h>
#include <net/clientthread.h>
#include <net/servermanager.h>
#include <net/ircthread.h>
#include <core/avatarmanager.h>

#include <sstream>

#define NET_CLIENT_TERMINATE_TIMEOUT_MSEC	2000
#define NET_IRC_TERMINATE_TIMEOUT_MSEC		2000

#define NET_DEFAULT_GAME					"default"

//uncomment for 0.7 beta
//#define POKERTH_IS_07BETA

using namespace std;

Session::Session(GuiInterface *g, ConfigFile *c)
: currentGameNum(0), myGui(g), myConfig(c), myGameType(GAME_TYPE_NONE)
{
	myQtToolsInterface = CreateQtToolsWrapper();
}


Session::~Session()
{
	terminateNetworkClient();
	terminateNetworkServer();
	delete myConfig;
	myConfig = 0;
	delete myQtToolsInterface;
	myQtToolsInterface = 0;
}

bool Session::init()
{
	myAvatarManager.reset(new AvatarManager(
		myConfig->readConfigInt("ServerUsePutAvatars") == 1,
		myConfig->readConfigString("ServerPutAvatarsAddress"),
		myConfig->readConfigString("ServerPutAvatarsUser"),
		myConfig->readConfigString("ServerPutAvatarsPassword")
		));
	bool retVal = myAvatarManager->Init(
		myQtToolsInterface->stringFromUtf8(myConfig->readConfigString("AppDataDir")),
		myQtToolsInterface->stringFromUtf8(myConfig->readConfigString("CacheDir")));
	addOwnAvatar(myQtToolsInterface->stringFromUtf8(myConfig->readConfigString("MyAvatar")));
	myAvatarManager->RemoveOldAvatarCacheEntries();
	return retVal;
}

void Session::init(boost::shared_ptr<AvatarManager> manager)
{
	myAvatarManager = manager;
}

void Session::addOwnAvatar(const std::string &avatarFile)
{
	myAvatarManager->AddSingleAvatar(avatarFile);
}

void Session::startLocalGame(const GameData &gameData, const StartData &startData) {

	myGameType = GAME_TYPE_LOCAL;

	currentGame.reset();
	currentGameNum++;

	myGui->initGui(gameData.guiSpeed);

	PlayerDataList playerDataList;
	for(int i = 0; i < startData.numberOfPlayers; i++) {

		//Namen und Avatarpfad abfragen
		ostringstream myName;
		if (i==0) { myName << "MyName";	}
		else { myName << "Opponent" << i << "Name"; }
		ostringstream myAvatar;
		if (i==0) { myAvatar << "MyAvatar";	}
		else { myAvatar << "Opponent" << i << "Avatar"; }

		//PlayerData erzeugen
		// UniqueId = PlayerNumber for local games.
		boost::shared_ptr<PlayerData> playerData(new PlayerData(i, i,
			i == 0 ? PLAYER_TYPE_HUMAN : PLAYER_TYPE_COMPUTER,
			i == 0 ? PLAYER_RIGHTS_ADMIN : PLAYER_RIGHTS_NORMAL));
		playerData->SetName(myConfig->readConfigString(myName.str()));
		playerData->SetAvatarFile(myConfig->readConfigString(myAvatar.str()));

		playerDataList.push_back(playerData);
	}
	// EngineFactory erstellen
	boost::shared_ptr<EngineFactory> factory(new LocalEngineFactory(myConfig)); // LocalEngine erstellen

	currentGame.reset(new Game(myGui, factory, playerDataList, gameData, startData, currentGameNum));

	//// SPIEL-SCHLEIFE
	currentGame->initHand();
	currentGame->startHand();
	// SPIEL-SCHLEIFE
}

void Session::startClientGame(boost::shared_ptr<Game> game)
{
	currentGameNum++;

	currentGame = game;
}

Game *Session::getCurrentGame()
{
	return currentGame.get();
}

GuiInterface *Session::getGui()
{
	return myGui;
}

Session::GameType Session::getGameType()
{
	return myGameType;
}

boost::shared_ptr<AvatarManager> Session::getAvatarManager()
{
	return myAvatarManager;
}

void Session::startInternetClient()
{
	if (myNetClient || !myGui)
	{
		assert(false);
		return;
	}
	myGameType = GAME_TYPE_INTERNET;

	myNetClient.reset(new ClientThread(*myGui, *myAvatarManager));
	bool useAvatarServer = myConfig->readConfigInt("UseAvatarServer") != 0;

	myNetClient->Init(
		myConfig->readConfigString("InternetServerAddress"), 
#ifdef POKERTH_IS_07BETA
		"pokerth.net/serverlist07.xml.z", 1,
#else
		myConfig->readConfigString("InternetServerListAddress"),
		myConfig->readConfigInt("InternetServerConfigMode") == 0,
#endif
		myConfig->readConfigInt("InternetServerPort"),
		myConfig->readConfigInt("InternetServerUseIpv6") == 1,
		myConfig->readConfigInt("InternetServerUseSctp") == 1,
		useAvatarServer ? myConfig->readConfigString("AvatarServerAddress") : "",
		myConfig->readConfigString("InternetServerPassword"),
		myConfig->readConfigString("MyName"),
		myConfig->readConfigString("MyAvatar"),
		myQtToolsInterface->stringFromUtf8(myConfig->readConfigString("CacheDir")));
	myNetClient->Run();
}

void Session::startNetworkClient(const string &serverAddress, unsigned serverPort, bool ipv6, bool sctp, const string &pwd)
{
	if (myNetClient || !myGui)
	{
		assert(false);
		return;
	}
	myGameType = GAME_TYPE_NETWORK;

	myNetClient.reset(new ClientThread(*myGui, *myAvatarManager));
	myNetClient->Init(
		serverAddress,
		"",
		false,
		serverPort,
		ipv6,
		sctp,
		"", // no avatar server
		pwd,
		myConfig->readConfigString("MyName"),
		myConfig->readConfigString("MyAvatar"),
		myQtToolsInterface->stringFromUtf8(myConfig->readConfigString("CacheDir")));
	myNetClient->Run();
	myNetClient->SendJoinFirstGame("");
}

void Session::startNetworkClientForLocalServer(const GameData &gameData)
{
	if (myNetClient || !myGui)
	{
		assert(false);
		return;
	}
	myGameType = GAME_TYPE_NETWORK;

	myNetClient.reset(new ClientThread(*myGui, *myAvatarManager));
	bool useIpv6 = myConfig->readConfigInt("ServerUseIpv6") == 1;
	const char *loopbackAddr = useIpv6 ? "::1" : "127.0.0.1";
	myNetClient->Init(
		loopbackAddr,
		"",
		false,
		myConfig->readConfigInt("ServerPort"),
		useIpv6,
		myConfig->readConfigInt("ServerUseSctp") == 1,
		"", // no avatar server
		myConfig->readConfigString("ServerPassword"),
		myConfig->readConfigString("MyName"),
		myConfig->readConfigString("MyAvatar"),
		myQtToolsInterface->stringFromUtf8(myConfig->readConfigString("CacheDir")));
	myNetClient->Run();
	myNetClient->SendCreateGame(gameData, NET_DEFAULT_GAME, "");
}

void Session::terminateNetworkClient()
{
	if (!myNetClient)
		return; // already terminated
	myNetClient->SignalTermination();
	// Give the threads some time to terminate.
	if (myNetClient->Join(NET_CLIENT_TERMINATE_TIMEOUT_MSEC))
		myNetClient.reset();

	// If termination fails, leave a memory leak to prevent a crash.
	myGameType = GAME_TYPE_NONE;
}

void Session::clientCreateGame(const GameData &gameData, const string &name, const string &password)
{
	if (!myNetClient)
		return; // only act if client is running.
	myNetClient->SendCreateGame(gameData, name, password);
}

void Session::clientJoinGame(unsigned gameId, const std::string &password)
{
	if (!myNetClient)
		return; // only act if client is running.
	myNetClient->SendJoinGame(gameId, password);
}

void Session::startNetworkServer()
{
	if (myNetServer)
	{
		assert(false);
		return;
	}

	myNetServer.reset(new ServerManager(*myGui, myConfig, *myAvatarManager));

	boost::shared_ptr<IrcThread> tmpIrcAdminThread;
	boost::shared_ptr<IrcThread> tmpIrcLobbyThread;
	if (myConfig->readConfigInt("UseAdminIRC"))
	{
		tmpIrcAdminThread = boost::shared_ptr<IrcThread>(new IrcThread(&myNetServer->GetAdminBot()));

		tmpIrcAdminThread->Init(
			myConfig->readConfigString("AdminIRCServerAddress"),
			myConfig->readConfigInt("AdminIRCServerPort"),
			myConfig->readConfigInt("AdminIRCServerUseIpv6") == 1,
			myConfig->readConfigString("AdminIRCServerNick"),
			myConfig->readConfigString("AdminIRCChannel"),
			myConfig->readConfigString("AdminIRCChannelPassword"));
	}
	
	if (myConfig->readConfigInt("UseLobbyIRC"))
	{	
		tmpIrcLobbyThread = boost::shared_ptr<IrcThread>(new IrcThread(&myNetServer->GetLobbyBot()));

		tmpIrcLobbyThread->Init(
			myConfig->readConfigString("LobbyIRCServerAddress"),
			myConfig->readConfigInt("LobbyIRCServerPort"),
			myConfig->readConfigInt("LobbyIRCServerUseIpv6") == 1,
			myConfig->readConfigString("LobbyIRCServerNick"),
			myConfig->readConfigString("LobbyIRCChannel"),
			myConfig->readConfigString("LobbyIRCChannelPassword"));
	}

	myNetServer->Init(
		myConfig->readConfigInt("ServerPort"),
		myConfig->readConfigInt("ServerUseIpv6") == 1,
		myConfig->readConfigInt("ServerUseSctp") == 1 ? NETWORK_MODE_TCP_SCTP : NETWORK_MODE_TCP,
		myConfig->readConfigString("ServerPassword"),
		myQtToolsInterface->stringFromUtf8(myConfig->readConfigString("LogDir")),
		tmpIrcAdminThread,
		tmpIrcLobbyThread
		);

	myNetServer->RunAll();
}

void Session::terminateNetworkServer()
{
	if (!myNetServer)
		return; // already terminated
	myNetServer->SignalTerminationAll();
	// Give the thread some time to terminate.
	if (myNetServer->JoinAll(true))
		myNetServer.reset();
	// If termination fails, leave a memory leak to prevent a crash.
}

bool Session::pollNetworkServerTerminated()
{
	bool retVal = false;
	if (!myNetServer)
		retVal = true; // already terminated
	else
	{
		myNetServer->Process();
		if (myNetServer->JoinAll(false))
			retVal = true;
	}
	return retVal;
}

void Session::sendLeaveCurrentGame()
{
	if (!myNetClient)
		return; // only act if client is running.
	myNetClient->SendLeaveCurrentGame();
}

void Session::sendStartEvent(bool fillUpWithCpuPlayers)
{
	if (!myNetClient)
		return; // only act if client is running.
	myNetClient->SendStartEvent(fillUpWithCpuPlayers);
}

void Session::sendClientPlayerAction()
{
	if (!myNetClient)
		return; // only act if client is running.
	myNetClient->SendPlayerAction();
}

void Session::sendGameChatMessage(const std::string &message)
{
	if (!myNetClient)
		return; // only act if client is running.
	myNetClient->SendGameChatMessage(message);
}

void Session::sendLobbyChatMessage(const std::string &message)
{
	if (!myNetClient)
		return;
	myNetClient->SendLobbyChatMessage(message);
}

void Session::kickPlayer(unsigned playerId)
{
	if (!myNetClient)
		return; // only act if client is running.
	myNetClient->SendKickPlayer(playerId);
}

void Session::resetNetworkTimeout()
{
	if (!myNetClient)
		return; // only act if client is running.
	myNetClient->SendResetTimeout();
}

void Session::kickPlayer(const string &playerName)
{
	if (!myNetClient)
		return; // only act if client is running.
	unsigned playerId;
	if (myNetClient->GetPlayerIdFromName(playerName, playerId))
		kickPlayer(playerId);
}

void Session::startVoteKickPlayer(unsigned playerId)
{
	if (!myNetClient)
		return; // only act if client is running.
	myNetClient->SendAskKickPlayer(playerId);
}

void Session::selectServer(unsigned serverId)
{
	if (!myNetClient)
		return; // only act if client is running.
	myNetClient->SelectServer(serverId);
}

void Session::voteKick(bool doKick)
{
	if (!myNetClient)
		return; // only act if client is running.
	myNetClient->SendVoteKick(doKick);
}

bool Session::isNetworkClientRunning() const
{
	// This, and every place which calls this, is a HACK.
	return myNetClient.get() != NULL;
}

bool Session::isNetworkServerRunning() const
{
	// This, and every place which calls this, is a HACK.
	return myNetServer.get() != NULL;
}

ServerInfo Session::getClientServerInfo(unsigned serverId) const
{
	ServerInfo info;
	if (myNetClient)
		info = myNetClient->GetServerInfo(serverId);
	return info;
}

GameInfo Session::getClientGameInfo(unsigned playerId) const
{
	GameInfo info;
	if (myNetClient)
		info = myNetClient->GetGameInfo(playerId);
	return info;
}

PlayerInfo Session::getClientPlayerInfo(unsigned playerId) const
{
	PlayerInfo info;
	if (myNetClient)
		info = myNetClient->GetPlayerInfo(playerId);
	return info;
}

ServerStats Session::getClientStats() const
{
	ServerStats stats;
	if (myNetClient)
		stats = myNetClient->GetStatData();
	return stats;
}

unsigned Session::getClientCurrentGameId() const
{
	unsigned id = 0;
	if (myNetClient)
		id = myNetClient->GetGameId();
	return id;
}

bool Session::getAvatarFile(const MD5Buf &avatarMD5, std::string &fileName)
{
	bool retVal = false;
	if (myAvatarManager.get())
	{
		string tmpFileName;
		retVal = myAvatarManager->GetAvatarFileName(avatarMD5, tmpFileName);
		if (retVal)
			fileName = myQtToolsInterface->stringToUtf8(tmpFileName);
	}
	return retVal;
}

