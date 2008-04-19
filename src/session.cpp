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

#define NET_CLIENT_TERMINATE_TIMEOUT_MSEC	1000
#define NET_IRC_TERMINATE_TIMEOUT_MSEC		2000

#define NET_DEFAULT_GAME					"default"

using namespace std;

Session::Session(GuiInterface *g, ConfigFile *c)
: currentGameNum(0), myNetClient(NULL), myNetServer(NULL), myClientIrcThread(NULL),
  myGui(g), myConfig(c), myGameType(GAME_TYPE_NONE)
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
	myAvatarManager.reset(new AvatarManager);
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
	if (myNetClient || myClientIrcThread || !myGui)
	{
		assert(false);
		return;
	}
	myGameType = GAME_TYPE_INTERNET;

	if (myConfig->readConfigInt("UseIRCLobbyChat"))
	{
		myClientIrcThread = new IrcThread(*myGui);
		myClientIrcThread->Init(
			myConfig->readConfigString("IRCServerAddress"),
			myConfig->readConfigInt("IRCServerPort"),
			myConfig->readConfigInt("IRCServerUseIpv6") == 1,
			myIrcNick,
			myConfig->readConfigString("IRCChannel"),
			myConfig->readConfigString("IRCChannelPassword"));
		myClientIrcThread->Run();
	}

	myNetClient = new ClientThread(*myGui, *myAvatarManager);
	string internetServerAddr(myConfig->readConfigString("InternetServerAddress"));
	string alternateServerAddr;
	if (internetServerAddr == "pokerth.6dns.org")
		alternateServerAddr = "pokerth.dyndns.org";
	myNetClient->Init(
		internetServerAddr,
		alternateServerAddr,
		myConfig->readConfigInt("InternetServerPort"),
		myConfig->readConfigInt("InternetServerUseIpv6") == 1,
		myConfig->readConfigInt("InternetServerUseSctp") == 1,
		myConfig->readConfigString("InternetServerPassword"),
		myConfig->readConfigString("MyName"),
		myConfig->readConfigString("MyAvatar"),
		myConfig->readConfigString("CacheDir"));
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

	myNetClient = new ClientThread(*myGui, *myAvatarManager);
	myNetClient->Init(
		serverAddress,
		"",
		serverPort,
		ipv6,
		sctp,
		pwd,
		myConfig->readConfigString("MyName"),
		myConfig->readConfigString("MyAvatar"),
		myConfig->readConfigString("CacheDir"));
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

	myNetClient = new ClientThread(*myGui, *myAvatarManager);
	bool useIpv6 = myConfig->readConfigInt("ServerUseIpv6") == 1;
	const char *loopbackAddr = useIpv6 ? "::1" : "127.0.0.1";
	myNetClient->Init(
		loopbackAddr,
		"",
		myConfig->readConfigInt("ServerPort"),
		useIpv6,
		myConfig->readConfigInt("ServerUseSctp") == 1,
		myConfig->readConfigString("ServerPassword"),
		myConfig->readConfigString("MyName"),
		myConfig->readConfigString("MyAvatar"),
		myConfig->readConfigString("CacheDir"));
	myNetClient->Run();
	myNetClient->SendCreateGame(gameData, NET_DEFAULT_GAME, "");
}

void Session::terminateNetworkClient()
{
	if (!myNetClient)
		return; // already terminated
	myNetClient->SignalTermination();
	if (myClientIrcThread)
		myClientIrcThread->SignalTermination();
	// Give the threads some time to terminate.
	if (myNetClient->Join(NET_CLIENT_TERMINATE_TIMEOUT_MSEC))
		delete myNetClient;
	if (myClientIrcThread && myClientIrcThread->Join(NET_IRC_TERMINATE_TIMEOUT_MSEC))
		delete myClientIrcThread;

	// If termination fails, leave a memory leak to prevent a crash.
	myNetClient = 0;
	myClientIrcThread = 0;
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

	myNetServer = new ServerManager(*myGui, myConfig, *myAvatarManager);

	boost::shared_ptr<IrcThread> tmpIrcThread;
	if (myConfig->readConfigInt("UseAdminIRC"))
	{
		tmpIrcThread = boost::shared_ptr<IrcThread>(new IrcThread(*myNetServer));

		tmpIrcThread->Init(
			myConfig->readConfigString("AdminIRCServerAddress"),
			myConfig->readConfigInt("AdminIRCServerPort"),
			myConfig->readConfigInt("AdminIRCServerUseIpv6") == 1,
			myConfig->readConfigString("AdminIRCServerNick"),
			myConfig->readConfigString("AdminIRCChannel"),
			myConfig->readConfigString("AdminIRCChannelPassword"));
	}

	myNetServer->Init(
		myConfig->readConfigInt("ServerPort"),
		myConfig->readConfigInt("ServerUseIpv6") == 1,
		myConfig->readConfigInt("ServerUseSctp") == 1 ? NETWORK_MODE_TCP_SCTP : NETWORK_MODE_TCP,
		myConfig->readConfigString("ServerPassword"),
		myConfig->readConfigString("LogDir"),
		tmpIrcThread
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
		delete myNetServer;
	// If termination fails, leave a memory leak to prevent a crash.
	myNetServer = 0;
}

bool Session::pollNetworkServerTerminated()
{
	bool retVal = false;
	if (!myNetServer)
		retVal = true; // already terminated
	else
	{
		if (myNetServer->JoinAll(false))
			retVal = true;
	}
	return retVal;
}

void Session::sendIrcChatMessage(const std::string &message)
{
	if (!myClientIrcThread)
		return;
	myClientIrcThread->SendChatMessage(message);
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

void Session::sendChatMessage(const std::string &message)
{
	if (!myNetClient)
		return; // only act if client is running.
	myNetClient->SendChatMessage(message);
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

bool Session::isNetworkClientRunning() const
{
	// This, and every place which calls this, is a HACK.
	// TODO
	return myNetClient != NULL;
}

bool Session::isNetworkServerRunning() const
{
	// This, and every place which calls this, is a HACK.
	// TODO
	return myNetServer != NULL;
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

