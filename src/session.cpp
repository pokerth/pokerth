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
#include "session.h"
#include "game.h"
#include "guiinterface.h"
#include "configfile.h"
#include <localenginefactory.h>
#include <clientenginefactory.h>
#include <net/clientthread.h>
#include <net/serverthread.h>

#include <sstream>

#define NET_CLIENT_TERMINATE_TIMEOUT_MSEC	1000
#define NET_SERVER_TERMINATE_TIMEOUT_MSEC	1000

using namespace std;

Session::Session(GuiInterface *g, ConfigFile *c)
: currentGameID(0), myNetClient(0), myNetServer(0), myGui(g), myConfig(c)
{	
	
}


Session::~Session()
{
	terminateNetworkClient();
	terminateNetworkServer();
	delete myConfig;
	myConfig = 0;
}


void Session::startLocalGame(const GameData &gameData, const StartData &startData) {

	currentGame.reset();
	currentGameID++;

	myGui->initGui(gameData.guiSpeed);

	PlayerDataList playerDataList;
	for(int i=0; i<gameData.numberOfPlayers; i++) {

		//Namen und Avatarpfad abfragen 
		ostringstream myName;
		if (i==0) { myName << "MyName";	}
		else { myName << "Opponent" << i << "Name"; }
		ostringstream myAvatar;
		if (i==0) { myAvatar << "MyAvatar";	}
		else { myAvatar << "Opponent" << i << "Avatar"; }

		//PlayerData erzeugen
		// TODO: PlayerType setzen
		// UniqueId = PlayerNumber for local games.
		boost::shared_ptr<PlayerData> playerData(new PlayerData(i, i, i == 0 ? PLAYER_TYPE_HUMAN : PLAYER_TYPE_COMPUTER));
		playerData->SetName(myConfig->readConfigString(myName.str()));
		playerData->SetAvatarFile(myConfig->readConfigString(myAvatar.str()));

		playerDataList.push_back(playerData);
	}
	// EngineFactory erstellen
	boost::shared_ptr<EngineFactory> factory(new LocalEngineFactory(myConfig)); // LocalEngine erstellen

	currentGame.reset(new Game(myGui, factory, playerDataList, gameData, startData, currentGameID));

	//// SPIEL-SCHLEIFE
	currentGame->initHand();
	currentGame->startHand();
	// SPIEL-SCHLEIFE
}

void Session::startClientGame(boost::shared_ptr<Game> game)
{
	currentGameID++;

// TODO: use server gui speed.
	myGui->initGui(5);
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

void Session::startNetworkClient(const string &serverAddress, unsigned serverPort, bool ipv6, const string &pwd)
{
	if (myNetClient || !myGui)
		return; // TODO: throw exception
	myNetClient = new ClientThread(*myGui);
	myNetClient->Init(
		serverAddress,
		serverPort,
		ipv6,
		pwd,
		myConfig->readConfigString("MyName"));
	myNetClient->Run();
}

void Session::startNetworkClientForLocalServer()
{
	if (myNetClient || !myGui)
		return; // TODO: throw exception
	myNetClient = new ClientThread(*myGui);
	myNetClient->Init(
		"localhost",
		myConfig->readConfigInt("ServerPort"),
		myConfig->readConfigInt("ServerUseIpv6") == 1,
		myConfig->readConfigString("ServerPassword"),
		myConfig->readConfigString("MyName"));
	myNetClient->Run();
}

void Session::terminateNetworkClient()
{
	if (!myNetClient)
		return; // already terminated
	myNetClient->SignalTermination();
	// Give the thread some time to terminate.
	if (myNetClient->Join(NET_CLIENT_TERMINATE_TIMEOUT_MSEC))
	{
		delete myNetClient;
	}
	// If termination fails, leave a memory leak to prevent a crash.
	myNetClient = 0;
}

void Session::startNetworkServer(const GameData &gameData)
{
	if (myNetServer)
		return; // TODO: throw exception
	myNetServer = new ServerThread(*myGui, myConfig);
	myNetServer->Init(
		myConfig->readConfigInt("ServerPort"),
		myConfig->readConfigInt("ServerUseIpv6") == 1,
		myConfig->readConfigString("ServerPassword"),
		gameData);
	myNetServer->Run();
}

void Session::initiateNetworkServerGame()
{
	if (!myNetServer)
		return; // TODO: throw exception
	myNetServer->StartGame();
}

void Session::terminateNetworkServer()
{
	if (!myNetServer)
		return; // already terminated
	myNetServer->SignalTermination();
	// Give the thread some time to terminate.
	if (myNetServer->Join(NET_SERVER_TERMINATE_TIMEOUT_MSEC))
	{
		delete myNetServer;
	}
	// If termination fails, leave a memory leak to prevent a crash.
	myNetServer = 0;
}

void Session::sendClientPlayerAction()
{
	if (!myNetClient)
		return;
	myNetClient->SendPlayerAction();
}

