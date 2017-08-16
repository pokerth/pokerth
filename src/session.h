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
#ifndef STDSESSION_H
#define STDSESSION_H

#include <boost/shared_ptr.hpp>
#include <string>

#include "serverdata.h"
#include "gamedata.h"
#include "playerdata.h"
#include "game_defs.h"
#include <core/crypthelper.h>


class GuiInterface;
class Game;
class ConfigFile;
class Log;
class ClientThread;
class ServerManager;
class IrcThread;
class AvatarManager;
class QtToolsInterface;

class Session
{
public:

	Session(GuiInterface*, ConfigFile*, Log*);

	~Session();

	enum GameType { GAME_TYPE_NONE, GAME_TYPE_LOCAL, GAME_TYPE_NETWORK, GAME_TYPE_INTERNET };

	// Only one of the two inits should be called.
	bool init();
	void init(boost::shared_ptr<AvatarManager> manager);

	void addOwnAvatar(const std::string &avatarFile);

	void startLocalGame(const GameData &gameData, const StartData &startData);
	void startClientGame(boost::shared_ptr<Game> game);

	boost::shared_ptr<Game> getCurrentGame();

	GuiInterface *getGui();
	Log* getMyLog()
	{
		return myLog;
	}
	GameType getGameType();

	boost::shared_ptr<AvatarManager> getAvatarManager();

	void startInternetClient();
	void startNetworkClient(const std::string &serverAddress, unsigned serverPort, bool ipv6, bool sctp);
	void startNetworkClientForLocalServer(const GameData &gameData);
	void terminateNetworkClient();
	void clientCreateGame(const GameData &gameData, const std::string &name, const std::string &password);
	void clientJoinGame(unsigned gameId, const std::string &password);
	void clientRejoinGame(unsigned gameId);

	void startNetworkServer(bool dedicated);
	void sendLeaveCurrentGame();
	void sendStartEvent(bool fillUpWithCpuPlayers);
	void terminateNetworkServer();
	bool pollNetworkServerTerminated();

	void sendClientPlayerAction();

	void sendGameChatMessage(const std::string &message);
	void sendLobbyChatMessage(const std::string &message);
	void sendPrivateChatMessage(unsigned targetPlayerId, const std::string &message);
	void kickPlayer(unsigned playerId);
	void kickPlayer(const std::string &playerName);
	void startVoteKickPlayer(unsigned playerId);
	void voteKick(bool doKick);
	void showMyCards();
	void selectServer(unsigned serverId);
	void setLogin(const std::string &userName, const std::string &password, bool isGuest);

	void invitePlayerToCurrentGame(unsigned playerId);
	void acceptGameInvitation(unsigned gameId);
	void rejectGameInvitation(unsigned gameId, DenyGameInvitationReason reason);

	void reportBadAvatar(unsigned reportedPlayerId, const std::string &avatarHash);
	void reportBadGameName(unsigned gameId);
	void adminActionBanPlayer(unsigned playerId);

	void resetNetworkTimeout();

	void adminActionCloseGame(unsigned gameId);

	bool isNetworkClientRunning() const; // TODO hack
	bool isNetworkServerRunning() const; // TODO hack

	ServerInfo getClientServerInfo(unsigned serverId) const;
	GameInfo getClientGameInfo(unsigned gameId) const;
	PlayerInfo getClientPlayerInfo(unsigned playerId) const;
	unsigned getGameIdOfPlayer(unsigned playerId) const;
	ServerStats getClientStats() const;
	unsigned getClientCurrentGameId() const;
	unsigned getClientUniquePlayerId() const;

	bool getAvatarFile(const MD5Buf &avatarMD5, std::string &fileName);

private:

	int currentGameNum;

	std::string myIrcNick;

	boost::shared_ptr<ClientThread> myNetClient;
	boost::shared_ptr<ServerManager> myNetServer;

	boost::shared_ptr<AvatarManager> myAvatarManager;

	boost::shared_ptr<Game> currentGame;
	GuiInterface *myGui;
	ConfigFile *myConfig;
	Log *myLog;
	GameType myGameType;
	QtToolsInterface *myQtToolsInterface;
};

#endif
