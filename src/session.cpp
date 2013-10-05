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
#include <boost/asio.hpp>
#include <net/socket_helper.h>
#include "session.h"
#include "game.h"
#include "log.h"
#include "guiinterface.h"
#include "configfile.h"
#include <qttoolsinterface.h>
#include <localenginefactory.h>
#include <clientenginefactory.h>
#include <net/clientthread.h>
#include <core/avatarmanager.h>
#include <net/servermanagerfactory.h>

#include <sstream>

#define NET_CLIENT_TERMINATE_TIMEOUT_MSEC	2000
#define NET_IRC_TERMINATE_TIMEOUT_MSEC		2000

#define NET_DEFAULT_GAME					"default"


using namespace std;

Session::Session(GuiInterface *g, ConfigFile *c, Log *l)
	: currentGameNum(0), myGui(g), myConfig(c), myLog(l), myGameType(GAME_TYPE_NONE)
{
	myQtToolsInterface = CreateQtToolsWrapper();
}


Session::~Session()
{
	terminateNetworkClient();
	terminateNetworkServer();
	delete myQtToolsInterface;
	myQtToolsInterface = 0;
	delete myLog;
	myLog = 0;
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
#ifndef POKERTH_OFFICIAL_SERVER
	myAvatarManager->RemoveOldAvatarCacheEntries();
#endif
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

void Session::startLocalGame(const GameData &gameData, const StartData &startData)
{

	myGameType = GAME_TYPE_LOCAL;

	currentGame.reset();

	currentGameNum++;

	myGui->initGui(gameData.guiSpeed);

	PlayerDataList playerDataList;
	for(int i = 0; i < startData.numberOfPlayers; i++) {

		//Namen und Avatarpfad abfragen
		ostringstream myName;
		if (i==0) {
			myName << "MyName";
		} else {
			myName << "Opponent" << i << "Name";
		}
		ostringstream myAvatar;
		if (i==0) {
			myAvatar << "MyAvatar";
		} else {
			myAvatar << "Opponent" << i << "Avatar";
		}

		//PlayerData erzeugen
		// UniqueId = PlayerNumber for local games.
		boost::shared_ptr<PlayerData> playerData(new PlayerData(
					i,
					i,
					i == 0 ? PLAYER_TYPE_HUMAN : PLAYER_TYPE_COMPUTER,
					PLAYER_RIGHTS_NORMAL,
					i == 0));
		playerData->SetName(myConfig->readConfigString(myName.str()));
		playerData->SetAvatarFile(myConfig->readConfigString(myAvatar.str()));

		playerDataList.push_back(playerData);
	}
	// EngineFactory erstellen
	boost::shared_ptr<EngineFactory> factory(new LocalEngineFactory(myConfig)); // LocalEngine erstellen

	currentGame.reset(new Game(myGui, factory, playerDataList, gameData, startData, currentGameNum, myLog));

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

boost::shared_ptr<Game> Session::getCurrentGame()
{
	return currentGame;
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
	if (myNetClient || !myGui) {
		assert(false);
		return;
	}
	myGameType = GAME_TYPE_INTERNET;

	myNetClient.reset(new ClientThread(*myGui, *myAvatarManager, myLog));
	bool useAvatarServer = myConfig->readConfigInt("UseAvatarServer") != 0;

	myNetClient->Init(
		myConfig->readConfigString("InternetServerAddress"),
		myConfig->readConfigString("InternetServerListAddress"),
//		"pokerth.net/serverlist_testing.xml.z",
		myConfig->readConfigString("ServerPassword"),
		myConfig->readConfigInt("InternetServerConfigMode") == 0,
//		true,
		myConfig->readConfigInt("InternetServerPort"),
		myConfig->readConfigInt("InternetServerUseIpv6") == 1,
		myConfig->readConfigInt("InternetServerUseSctp") == 1,
		useAvatarServer ? myConfig->readConfigString("AvatarServerAddress") : "",
		myConfig->readConfigString("MyName"),
		myConfig->readConfigString("MyAvatar"),
		myQtToolsInterface->stringFromUtf8(myConfig->readConfigString("CacheDir")));
	myNetClient->Run();
}

void Session::startNetworkClient(const string &serverAddress, unsigned serverPort, bool ipv6, bool sctp)
{
	if (myNetClient || !myGui) {
		assert(false);
		return;
	}
	myGameType = GAME_TYPE_NETWORK;

	myNetClient.reset(new ClientThread(*myGui, *myAvatarManager, myLog));
	myNetClient->Init(
		serverAddress,
		"",
		myConfig->readConfigString("ServerPassword"),
		false,
		serverPort,
		ipv6,
		sctp,
		"", // no avatar server
		myConfig->readConfigString("MyName"),
		myConfig->readConfigString("MyAvatar"),
		myQtToolsInterface->stringFromUtf8(myConfig->readConfigString("CacheDir")));
	myNetClient->Run();
	myNetClient->SendJoinFirstGame(
		"",
		myConfig->readConfigInt("NetAutoLeaveGameAfterFinish") == 1
	);
}

void Session::startNetworkClientForLocalServer(const GameData &gameData)
{
	if (myNetClient || !myGui) {
		assert(false);
		return;
	}
	myGameType = GAME_TYPE_NETWORK;

	myNetClient.reset(new ClientThread(*myGui, *myAvatarManager, myLog));
	bool useIpv6 = myConfig->readConfigInt("ServerUseIpv6") == 1;
	const char *loopbackAddr = useIpv6 ? "::1" : "127.0.0.1";
	myNetClient->Init(
		loopbackAddr,
		"",
		myConfig->readConfigString("ServerPassword"),
		false,
		myConfig->readConfigInt("ServerPort"),
		useIpv6,
		myConfig->readConfigInt("ServerUseSctp") == 1,
		"", // no avatar server
		myConfig->readConfigString("MyName"),
		myConfig->readConfigString("MyAvatar"),
		myQtToolsInterface->stringFromUtf8(myConfig->readConfigString("CacheDir")));
	myNetClient->Run();
	myNetClient->SendCreateGame(
		gameData,
		NET_DEFAULT_GAME,
		"",
		myConfig->readConfigInt("NetAutoLeaveGameAfterFinish") == 1
	);
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
	myNetClient->SendCreateGame(
		gameData,
		name,
		password,
		myConfig->readConfigInt("NetAutoLeaveGameAfterFinish") == 1
	);
}

void Session::clientJoinGame(unsigned gameId, const std::string &password)
{
	if (!myNetClient)
		return; // only act if client is running.
	myNetClient->SendJoinGame(
		gameId,
		password,
		myConfig->readConfigInt("NetAutoLeaveGameAfterFinish") == 1
	);
}

void Session::clientRejoinGame(unsigned gameId)
{
	if (!myNetClient)
		return; // only act if client is running.
	myNetClient->SendRejoinGame(
		gameId,
		myConfig->readConfigInt("NetAutoLeaveGameAfterFinish") == 1
	);
}

void Session::startNetworkServer(bool dedicated)
{
	if (myNetServer) {
		assert(false);
		return;
	}

	ServerMode mode;
	if (dedicated) {
#ifdef POKERTH_OFFICIAL_SERVER
		mode = SERVER_MODE_INTERNET_AUTH;
#else
		mode = SERVER_MODE_INTERNET_NOAUTH;
#endif
	} else {
		mode = SERVER_MODE_LAN;
	}

	myNetServer = ServerManagerFactory::CreateServerManager(*myConfig, *myGui, mode, *myAvatarManager);

	int protocol = TRANSPORT_PROTOCOL_TCP;
	if (dedicated && myConfig->readConfigInt("ServerUseWebSocket") == 1) {
		protocol |= TRANSPORT_PROTOCOL_WEBSOCKET;
	}
	if (myConfig->readConfigInt("ServerUseSctp") == 1) {
		protocol |= TRANSPORT_PROTOCOL_SCTP;
	}
	myNetServer->Init(
		myConfig->readConfigInt("ServerPort"),
		myConfig->readConfigInt("ServerWebSocketPort"),
		myConfig->readConfigInt("ServerUseIpv6") == 1,
		protocol,
		myQtToolsInterface->stringFromUtf8(myConfig->readConfigString("LogDir")),
		myQtToolsInterface->stringFromUtf8(myConfig->readConfigString("ServerWebSocketResource")),
		myQtToolsInterface->stringFromUtf8(myConfig->readConfigString("ServerWebSocketOrigin"))
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
	else {
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

void Session::sendPrivateChatMessage(unsigned targetPlayerId, const std::string &message)
{
	if (!myNetClient)
		return;
	myNetClient->SendPrivateChatMessage(targetPlayerId, message);
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

void Session::adminActionCloseGame(unsigned gameId)
{
	if (!myNetClient)
		return; // only act if client is running.
	myNetClient->SendAdminRemoveGame(gameId);
}

void Session::adminActionBanPlayer(unsigned playerId)
{
	if (!myNetClient)
		return; // only act if client is running.
	myNetClient->SendAdminBanPlayer(playerId);
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

void
Session::setLogin(const std::string &userName, const std::string &password, bool isGuest)
{
	if (!myNetClient)
		return; // only act if client is running.
	myNetClient->SetLogin(userName, password, isGuest);
}

void
Session::invitePlayerToCurrentGame(unsigned playerId)
{
	if (!myNetClient)
		return; // only act if client is running.
	myNetClient->SendInvitePlayerToCurrentGame(playerId);
}

void
Session::acceptGameInvitation(unsigned gameId)
{
	if (!myNetClient)
		return; // only act if client is running.
	myNetClient->SendJoinGame(
		gameId,
		"",
		myConfig->readConfigInt("NetAutoLeaveGameAfterFinish") == 1
	);
}

void
Session::rejectGameInvitation(unsigned gameId, DenyGameInvitationReason reason)
{
	if (!myNetClient)
		return; // only act if client is running.
	myNetClient->SendRejectGameInvitation(gameId, reason);
}

void Session::reportBadAvatar(unsigned reportedPlayerId, const std::string &avatarHash)
{
	if (!myNetClient)
		return; // only act if client is running.
	myNetClient->SendReportAvatar(reportedPlayerId, avatarHash);
}

void Session::reportBadGameName(unsigned gameId)
{
	if (!myNetClient)
		return; // only act if client is running.
	myNetClient->SendReportGameName(gameId);
}

void Session::voteKick(bool doKick)
{
	if (!myNetClient)
		return; // only act if client is running.
	myNetClient->SendVoteKick(doKick);
}

void Session::showMyCards()
{
	if (!myNetClient)
		return; // only act if client is running.
	myNetClient->SendShowMyCards();
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

unsigned Session::getGameIdOfPlayer(unsigned playerId) const
{
	unsigned gameId = 0;
	if (myNetClient)
		gameId = myNetClient->GetGameIdOfPlayer(playerId);
	return gameId;
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

unsigned Session::getClientUniquePlayerId() const
{
	unsigned id = 0;
	if (myNetClient)
		id = myNetClient->GetGuiPlayerId();
	return id;
}

bool Session::getAvatarFile(const MD5Buf &avatarMD5, std::string &fileName)
{
	bool retVal = false;
	if (myAvatarManager.get()) {
		string tmpFileName;
		retVal = myAvatarManager->GetAvatarFileName(avatarMD5, tmpFileName);
		if (retVal)
			fileName = myQtToolsInterface->stringToUtf8(tmpFileName);
	}
	return retVal;
}

