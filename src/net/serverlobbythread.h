/***************************************************************************
 *   Copyright (C) 2007 by Lothar May                                      *
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
/* Network server receive thread. */

#ifndef _SERVERRECVTHREAD_H_
#define _SERVERRECVTHREAD_H_

#include <net/connectdata.h>
#include <net/sessionmanager.h>
#include <net/netpacket.h>
#include <gui/guiinterface.h>
#include <gamedata.h>

#include <deque>
#include <list>
#include <core/boost/timers.hpp>

#define LOBBY_THREAD_TERMINATE_TIMEOUT	200


class SenderThread;
class ReceiverHelper;
class ServerSenderCallback;
class ServerGameThread;
class ConfigFile;
class AvatarManager;
struct GameData;
class Game;

class ServerLobbyThread : public Thread
{
public:
	ServerLobbyThread(GuiInterface &gui, ConfigFile *playerConfig, AvatarManager &avatarManager);
	virtual ~ServerLobbyThread();

	void Init(const std::string &pwd);

	void AddConnection(boost::shared_ptr<ConnectData> data);
	void ReAddSession(SessionWrapper session, int reason);
	void MoveSessionToGame(ServerGameThread &game, SessionWrapper session);
	void RemoveSessionFromGame(SessionWrapper session);
	void SessionError(SessionWrapper session, int errorCode);
	void NotifyPlayerJoinedGame(unsigned gameId, unsigned playerId);
	void NotifyPlayerLeftGame(unsigned gameId, unsigned playerId);
	void NotifyStartingGame(unsigned gameId);
	void NotifyReopeningGame(unsigned gameId);

	void HandleGameRetrievePlayerInfo(SessionWrapper session, const NetPacketRetrievePlayerInfo &tmpPacket);
	void HandleGameRetrieveAvatar(SessionWrapper session, const NetPacketRetrieveAvatar &tmpPacket);

	void AddComputerPlayer(boost::shared_ptr<PlayerData> player);
	void RemoveComputerPlayer(boost::shared_ptr<PlayerData> player);

	void RemoveGame(unsigned id);

	u_int32_t GetNextUniquePlayerId();
	u_int32_t GetNextGameId();
	ServerCallback &GetCallback();

	AvatarManager &GetAvatarManager();

protected:

	typedef std::deque<boost::shared_ptr<ConnectData> > ConnectQueue;
	typedef std::deque<SessionWrapper> SessionQueue;
	typedef std::list<SessionWrapper> SessionList;
	typedef std::list<std::pair<boost::timers::portable::microsec_timer, boost::shared_ptr<SessionData> > > CloseSessionList;
	typedef std::map<unsigned, boost::shared_ptr<ServerGameThread> > GameMap;
	typedef std::list<unsigned> RemoveGameList;

	// Main function of the thread.
	virtual void Main();

	void ProcessLoop();
	void HandleNetPacketInit(SessionWrapper session, const NetPacketInit &tmpPacket);
	void HandleNetPacketAvatarHeader(SessionWrapper session, const NetPacketAvatarHeader &tmpPacket);
	void HandleNetPacketUnknownAvatar(SessionWrapper session, const NetPacketUnknownAvatar &tmpPacket);
	void HandleNetPacketAvatarFile(SessionWrapper session, const NetPacketAvatarFile &tmpPacket);
	void HandleNetPacketAvatarEnd(SessionWrapper session, const NetPacketAvatarEnd &tmpPacket);
	void HandleNetPacketRetrievePlayerInfo(SessionWrapper session, const NetPacketRetrievePlayerInfo &tmpPacket);
	void HandleNetPacketRetrieveAvatar(SessionWrapper session, const NetPacketRetrieveAvatar &tmpPacket);
	void HandleNetPacketCreateGame(SessionWrapper session, const NetPacketCreateGame &tmpPacket);
	void HandleNetPacketJoinGame(SessionWrapper session, const NetPacketJoinGame &tmpPacket);
	void EstablishSession(SessionWrapper session);
	void RequestPlayerAvatar(SessionWrapper session);
	void CloseSessionLoop();
	void RemoveGameLoop();

	void InternalAddGame(boost::shared_ptr<ServerGameThread> game);
	void InternalRemoveGame(boost::shared_ptr<ServerGameThread> game);

	void TerminateGames();

	void HandleNewConnection(boost::shared_ptr<ConnectData> connData);
	void HandleReAddedSession(SessionWrapper session);

	SOCKET Select();

	void CleanupConnectQueue();
	void CleanupSessionMap();

	void CloseSessionDelayed(SessionWrapper session);
	void SendError(SOCKET s, int errorCode);
	void SendJoinGameFailed(SOCKET s, int reason);
	void SendGameList(SOCKET s);

	SenderThread &GetSender();
	ReceiverHelper &GetReceiver();

	bool CheckPassword(const std::string &password) const;

	ServerSenderCallback &GetSenderCallback();
	GuiInterface &GetGui();

	bool IsPlayerConnected(const std::string &name);

	static boost::shared_ptr<NetPacket> CreateNetPacketGameListNew(const ServerGameThread &game);
	static boost::shared_ptr<NetPacket> CreateNetPacketGameListUpdate(unsigned gameId, GameMode mode);

private:

	ConnectQueue m_connectQueue;
	mutable boost::mutex m_connectQueueMutex;

	SessionQueue m_sessionQueue;
	mutable boost::mutex m_sessionQueueMutex;

	SessionManager m_sessionManager;
	SessionManager m_gameSessionManager;

	CloseSessionList m_closeSessionList;
	mutable boost::mutex m_closeSessionListMutex;

	RemoveGameList m_removeGameList;
	mutable boost::mutex m_removeGameListMutex;

	PlayerDataMap m_computerPlayers;
	mutable boost::mutex m_computerPlayersMutex;

	GameMap m_gameMap;

	std::auto_ptr<ReceiverHelper> m_receiver;
	std::auto_ptr<SenderThread> m_sender;
	std::auto_ptr<ServerSenderCallback> m_senderCallback;
	GuiInterface &m_gui;
	AvatarManager &m_avatarManager;

	std::string m_password;
	ConfigFile *m_playerConfig;
	u_int32_t m_curGameId;

	u_int32_t m_curUniquePlayerId;
	mutable boost::mutex m_curUniquePlayerIdMutex;
};

#endif
