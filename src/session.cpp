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
#include <net/clientthread.h>
#include <net/serverthread.h>

#define NET_CLIENT_TERMINATE_TIMEOUT_MSEC	1000
#define NET_SERVER_TERMINATE_TIMEOUT_MSEC	1000

using namespace std;

Session::Session(GuiInterface *g)
: actualGameID(0), myNetClient(0), myNetServer(0), actualGame(0), myGui(g)
{	
	myConfig = new ConfigFile;
}


Session::~Session()
{
	terminateNetworkClient();
	terminateNetworkServer();
	deleteGame();
	delete myConfig;
}


void Session::startGame(int qP, int sC, int sB) {

	actualGameID++;

	actualGame = new Game(myConfig, myGui, qP, sC, sB, actualGameID);
}

void Session::deleteGame() {

	delete actualGame;
	actualGame = 0;

}

void Session::startNetworkClient(const string &serverAddress, unsigned serverPort, bool ipv6, const string &pwd)
{
	if (myNetClient || !myGui)
		return; // TODO: throw exception
	myNetClient = new ClientThread(*myGui);
	myNetClient->Init(serverAddress, serverPort, ipv6, pwd);
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
		myConfig->readConfigString("ServerPassword"));
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

void Session::startNetworkServer()
{
	if (myNetServer)
		return; // TODO: throw exception
	myNetServer = new ServerThread(*myGui);
	myNetServer->Init(
		myConfig->readConfigInt("ServerPort"),
		myConfig->readConfigInt("ServerUseIpv6") == 1,
		myConfig->readConfigString("ServerPassword"));
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

