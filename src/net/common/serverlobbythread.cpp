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

#include <net/serverlobbythread.h>
#include <net/servergamethread.h>
#include <net/serverexception.h>
#include <net/senderthread.h>
#include <net/sendercallback.h>
#include <net/receiverhelper.h>
#include <net/socket_msg.h>
#include <core/avatarmanager.h>
#include <openssl/rand.h>

#include <boost/lambda/lambda.hpp>

#define SERVER_CLOSE_SESSION_DELAY_SEC	1
#define SERVER_MAX_NUM_SESSIONS			512 // Maximum number of idle users in lobby.

#define SERVER_COMPUTER_PLAYER_NAME				"Computer"

using namespace std;


class ServerSenderCallback : public SenderCallback
{
public:
	ServerSenderCallback(ServerLobbyThread &server) : m_server(server) {}
	virtual ~ServerSenderCallback() {}

	virtual void SignalNetError(SOCKET /*sock*/, int /*errorID*/, int /*osErrorID*/)
	{
		// We just ignore send errors for now, on server side.
		// A serious send error should trigger a read error or a read
		// returning 0 afterwards, and we will handle this error.
	}

private:
	ServerLobbyThread &m_server;
};


ServerLobbyThread::ServerLobbyThread(GuiInterface &gui, ConfigFile *playerConfig, AvatarManager &avatarManager)
: m_gui(gui), m_avatarManager(avatarManager), m_playerConfig(playerConfig),
  m_curGameId(0), m_curUniquePlayerId(0)
{
	m_senderCallback.reset(new ServerSenderCallback(*this));
	m_sender.reset(new SenderThread(GetSenderCallback()));
	m_receiver.reset(new ReceiverHelper);
}

ServerLobbyThread::~ServerLobbyThread()
{
	CleanupConnectQueue();
}

void
ServerLobbyThread::Init(const string &pwd)
{
	m_password = pwd;
}

void
ServerLobbyThread::AddConnection(boost::shared_ptr<ConnectData> data)
{
	boost::mutex::scoped_lock lock(m_connectQueueMutex);
	m_connectQueue.push_back(data);
}

void
ServerLobbyThread::ReAddSession(SessionWrapper session, int reason)
{
	boost::shared_ptr<NetPacket> packet(new NetPacketRemovedFromGame);
	NetPacketRemovedFromGame::Data removedData;
	removedData.removeReason = reason;
	static_cast<NetPacketRemovedFromGame *>(packet.get())->SetData(removedData);
	GetSender().Send(session.sessionData->GetSocket(), packet);

	boost::mutex::scoped_lock lock(m_sessionQueueMutex);
	m_sessionQueue.push_back(session);
}

void
ServerLobbyThread::MoveSessionToGame(ServerGameThread &game, SessionWrapper session)
{
	// Remove session from the lobby.
	m_sessionManager.RemoveSession(session.sessionData->GetSocket());
	// Session is now in game state.
	session.sessionData->SetState(SessionData::Game);
	// Store it in the list of game sessions.
	m_gameSessionManager.AddSession(session);
	// Add session to the game.
	game.AddSession(session);
}

void
ServerLobbyThread::RemoveSessionFromGame(SessionWrapper session)
{
	// Just remove the session. Only for fatal errors.
	m_gameSessionManager.RemoveSession(session.sessionData->GetSocket());
}

void
ServerLobbyThread::CloseSessionDelayed(SessionWrapper session)
{
	m_sessionManager.RemoveSession(session.sessionData->GetSocket());
	m_gameSessionManager.RemoveSession(session.sessionData->GetSocket());

	boost::timers::portable::microsec_timer closeTimer;
	CloseSessionList::value_type closeSessionData(closeTimer, session.sessionData);

	boost::mutex::scoped_lock lock(m_closeSessionListMutex);
	m_closeSessionList.push_back(closeSessionData);
}

void
ServerLobbyThread::NotifyPlayerJoinedGame(unsigned gameId, unsigned playerId)
{
	// Send notification to players in lobby.
	boost::shared_ptr<NetPacket> packet(new NetPacketGameListPlayerJoined);
	NetPacketGameListPlayerJoined::Data packetData;
	packetData.gameId = gameId;
	packetData.playerId = playerId;
	static_cast<NetPacketGameListPlayerJoined *>(packet.get())->SetData(packetData);
	m_sessionManager.SendToAllSessions(GetSender(), packet, SessionData::Established);
	m_gameSessionManager.SendToAllSessions(GetSender(), packet, SessionData::Game);
}

void
ServerLobbyThread::NotifyPlayerLeftGame(unsigned gameId, unsigned playerId)
{
	// Send notification to players in lobby.
	boost::shared_ptr<NetPacket> packet(new NetPacketGameListPlayerLeft);
	NetPacketGameListPlayerLeft::Data packetData;
	packetData.gameId = gameId;
	packetData.playerId = playerId;
	static_cast<NetPacketGameListPlayerLeft *>(packet.get())->SetData(packetData);
	m_sessionManager.SendToAllSessions(GetSender(), packet, SessionData::Established);
	m_gameSessionManager.SendToAllSessions(GetSender(), packet, SessionData::Game);
}

void
ServerLobbyThread::NotifyStartingGame(unsigned gameId)
{
	boost::shared_ptr<NetPacket> packet = CreateNetPacketGameListUpdate(gameId, GAME_MODE_STARTED);
	m_sessionManager.SendToAllSessions(GetSender(), packet, SessionData::Established);
	m_gameSessionManager.SendToAllSessions(GetSender(), packet, SessionData::Game);
}

void
ServerLobbyThread::NotifyReopeningGame(unsigned gameId)
{
	boost::shared_ptr<NetPacket> packet = CreateNetPacketGameListUpdate(gameId, GAME_MODE_CREATED);
	m_sessionManager.SendToAllSessions(GetSender(), packet, SessionData::Established);
	m_gameSessionManager.SendToAllSessions(GetSender(), packet, SessionData::Game);
}

void
ServerLobbyThread::HandleGameRetrievePlayerInfo(SessionWrapper session, const NetPacketRetrievePlayerInfo &tmpPacket)
{
	// Someone within a game requested player info.
	HandleNetPacketRetrievePlayerInfo(session, tmpPacket);
}

void
ServerLobbyThread::HandleGameRetrieveAvatar(SessionWrapper session, const NetPacketRetrieveAvatar &tmpPacket)
{
	// Someone within a game requested an avatar.
	HandleNetPacketRetrieveAvatar(session, tmpPacket);
}

void
ServerLobbyThread::AddComputerPlayer(boost::shared_ptr<PlayerData> player)
{
	boost::mutex::scoped_lock lock(m_computerPlayersMutex);
	m_computerPlayers.insert(PlayerDataMap::value_type(player->GetUniqueId(), player));
}

void
ServerLobbyThread::RemoveComputerPlayer(boost::shared_ptr<PlayerData> player)
{
	boost::mutex::scoped_lock lock(m_computerPlayersMutex);
	m_computerPlayers.erase(player->GetUniqueId());
}

void
ServerLobbyThread::RemoveGame(unsigned id)
{
	boost::mutex::scoped_lock lock(m_removeGameListMutex);
	m_removeGameList.push_back(id);
}

AvatarManager &
ServerLobbyThread::GetAvatarManager()
{
	return m_avatarManager;
}

u_int32_t
ServerLobbyThread::GetNextUniquePlayerId()
{
	boost::mutex::scoped_lock lock(m_curUniquePlayerIdMutex);
	return m_curUniquePlayerId++;
}

u_int32_t
ServerLobbyThread::GetNextGameId()
{
	return m_curGameId++;
}

void
ServerLobbyThread::Main()
{
	GetSender().Run();

	try
	{
		while (!ShouldTerminate())
		{
			{
				// Handle one incoming connection at a time.
				boost::shared_ptr<ConnectData> tmpData;
				{
					boost::mutex::scoped_lock lock(m_connectQueueMutex);
					if (!m_connectQueue.empty())
					{
						tmpData = m_connectQueue.front();
						m_connectQueue.pop_front();
					}
				}
				if (tmpData.get())
					HandleNewConnection(tmpData);
			}
			{
				// Handle one incoming session at a time.
				SessionWrapper tmpSession;
				{
					boost::mutex::scoped_lock lock(m_sessionQueueMutex);
					if (!m_sessionQueue.empty())
					{
						tmpSession = m_sessionQueue.front();
						m_sessionQueue.pop_front();
					}
				}
				if (tmpSession.sessionData.get() && tmpSession.playerData.get())
					HandleReAddedSession(tmpSession);
			}
			// Process loop.
			ProcessLoop();
			// Close sessions.
			CloseSessionLoop();
			// Remove games.
			RemoveGameLoop();
		}
	} catch (const NetException &e)
	{
		GetCallback().SignalNetServerError(e.GetErrorId(), e.GetOsErrorCode());
	}

	TerminateGames();

	GetSender().SignalTermination();
	GetSender().Join(SENDER_THREAD_TERMINATE_TIMEOUT);

	CleanupConnectQueue();
}

void
ServerLobbyThread::ProcessLoop()
{
	// Wait for data.
	SessionWrapper session = m_sessionManager.Select(RECV_TIMEOUT_MSEC);

	if (session.sessionData.get())
	{
		boost::shared_ptr<NetPacket> packet;
		try
		{
			// Receive the next packet.
			packet = GetReceiver().Recv(session.sessionData->GetSocket(), session.sessionData->GetReceiveBuffer());
		} catch (const NetException &)
		{
			// On error: Close this session.
			m_sessionManager.RemoveSession(session.sessionData->GetSocket());
			return;
		}
		if (packet.get())
		{
			if (session.sessionData->GetState() == SessionData::Init)
			{
				if (packet->ToNetPacketInit())
					HandleNetPacketInit(session, *packet->ToNetPacketInit());
				else if (packet->ToNetPacketAvatarHeader())
					HandleNetPacketAvatarHeader(session, *packet->ToNetPacketAvatarHeader());
				else
					SessionError(session, ERR_SOCK_INVALID_STATE);
			}
			else if (session.sessionData->GetState() == SessionData::ReceivingAvatar)
			{
				if (packet->ToNetPacketAvatarFile())
					HandleNetPacketAvatarFile(session, *packet->ToNetPacketAvatarFile());
				else if (packet->ToNetPacketAvatarEnd())
					HandleNetPacketAvatarEnd(session, *packet->ToNetPacketAvatarEnd());
				else
					SessionError(session, ERR_SOCK_INVALID_STATE);
			}
			else
			{
				if (packet->ToNetPacketRetrievePlayerInfo())
					HandleNetPacketRetrievePlayerInfo(session, *packet->ToNetPacketRetrievePlayerInfo());
				else if (packet->ToNetPacketRetrieveAvatar())
					HandleNetPacketRetrieveAvatar(session, *packet->ToNetPacketRetrieveAvatar());
				else if (packet->ToNetPacketCreateGame())
					HandleNetPacketCreateGame(session, *packet->ToNetPacketCreateGame());
				else if (packet->ToNetPacketJoinGame())
					HandleNetPacketJoinGame(session, *packet->ToNetPacketJoinGame());
				else
					SessionError(session, ERR_SOCK_INVALID_STATE);
			}
		}
	}
}

void
ServerLobbyThread::HandleNetPacketInit(SessionWrapper session, const NetPacketInit &tmpPacket)
{
	NetPacketInit::Data initData;
	tmpPacket.GetData(initData);

	// Check the protocol version.
	if (initData.versionMajor != NET_VERSION_MAJOR)
	{
		SessionError(session, ERR_NET_VERSION_NOT_SUPPORTED);
		return;
	}

	// Check the server password.
	if (!CheckPassword(initData.password))
	{
		SessionError(session, ERR_NET_INVALID_PASSWORD);
		return;
	}

	// Check whether the player name is correct.
	// Partly, this is also done in netpacket.
	// However, some disallowed names are checked only here.
	if (initData.playerName.empty() || initData.playerName.size() > MAX_NAME_SIZE
		|| initData.playerName[0] == '#'
		|| initData.playerName.substr(0, sizeof(SERVER_COMPUTER_PLAYER_NAME) - 1) == SERVER_COMPUTER_PLAYER_NAME)
	{
		SessionError(session, ERR_NET_INVALID_PLAYER_NAME);
		return;
	}

	// Check whether this player is already connected.
	if (IsPlayerConnected(initData.playerName))
	{
		SessionError(session, ERR_NET_PLAYER_NAME_IN_USE);
		return;
	}

	// Create player data object.
	boost::shared_ptr<PlayerData> tmpPlayerData(
		new PlayerData(GetNextUniquePlayerId(), 0, PLAYER_TYPE_HUMAN, PLAYER_RIGHTS_NORMAL));
	tmpPlayerData->SetName(initData.playerName);
	tmpPlayerData->SetNetSessionData(session.sessionData);
	if (initData.showAvatar)
		tmpPlayerData->SetAvatarMD5(initData.avatar);

	// Set player data for session.
	m_sessionManager.SetSessionPlayerData(session.sessionData->GetSocket(), tmpPlayerData);
	session.playerData = tmpPlayerData;

	if (initData.showAvatar && !GetAvatarManager().HasAvatar(initData.avatar))
		RequestPlayerAvatar(session);
	else
		EstablishSession(session);
}

void
ServerLobbyThread::HandleNetPacketAvatarHeader(SessionWrapper session, const NetPacketAvatarHeader &tmpPacket)
{
	if (session.playerData.get())
	{
		NetPacketAvatarHeader::Data headerData;
		tmpPacket.GetData(headerData);

		if (headerData.avatarFileSize && headerData.avatarFileSize <= MAX_AVATAR_FILE_SIZE)
		{
			boost::shared_ptr<AvatarData> tmpAvatarData(new AvatarData);
			tmpAvatarData->fileData.reserve(headerData.avatarFileSize);
			tmpAvatarData->fileType = headerData.avatarFileType;
			tmpAvatarData->reportedSize = headerData.avatarFileSize;
			// Ignore request id for now.

			session.playerData->SetNetAvatarData(tmpAvatarData);

			// Session is now receiving an avatar.
			session.sessionData->SetState(SessionData::ReceivingAvatar);
		}
		else
			SessionError(session, ERR_NET_AVATAR_TOO_LARGE);
	}
}

void
ServerLobbyThread::HandleNetPacketAvatarFile(SessionWrapper session, const NetPacketAvatarFile &tmpPacket)
{
	if (session.playerData.get())
	{
		NetPacketAvatarFile::Data data;
		tmpPacket.GetData(data);

		boost::shared_ptr<AvatarData> tmpAvatar = session.playerData->GetNetAvatarData();
		if (tmpAvatar.get() && tmpAvatar->fileData.size() + data.fileData.size() <= tmpAvatar->reportedSize)
		{
			std::copy(data.fileData.begin(), data.fileData.end(), back_inserter(tmpAvatar->fileData));
		}
	}
}

void
ServerLobbyThread::HandleNetPacketAvatarEnd(SessionWrapper session, const NetPacketAvatarEnd &/*tmpPacket*/)
{
	if (session.playerData.get())
	{
		boost::shared_ptr<AvatarData> tmpAvatar = session.playerData->GetNetAvatarData();
		MD5Buf avatarMD5 = session.playerData->GetAvatarMD5();
		if (!avatarMD5.IsZero() && tmpAvatar.get())
		{
			unsigned avatarSize = (unsigned)tmpAvatar->fileData.size();
			if (avatarSize == tmpAvatar->reportedSize)
			{
				GetAvatarManager().StoreAvatarInCache(avatarMD5, tmpAvatar->fileType, &tmpAvatar->fileData[0], avatarSize);
				// TODO log error
				// Free memory.
				session.playerData->SetNetAvatarData(boost::shared_ptr<AvatarData>());
				// Init finished - start session.
				EstablishSession(session);
			}
			else
				SessionError(session, ERR_NET_WRONG_AVATAR_SIZE);
		}
	}
}

void
ServerLobbyThread::HandleNetPacketRetrievePlayerInfo(SessionWrapper session, const NetPacketRetrievePlayerInfo &tmpPacket)
{
	NetPacketRetrievePlayerInfo::Data request;
	tmpPacket.GetData(request);

	// Find player in lobby or in a game.
	boost::shared_ptr<PlayerData> tmpPlayer = m_sessionManager.GetSessionByUniquePlayerId(request.playerId).playerData;
	if (!tmpPlayer.get())
		tmpPlayer = m_gameSessionManager.GetSessionByUniquePlayerId(request.playerId).playerData;
	if (!tmpPlayer.get())
	{
		boost::mutex::scoped_lock lock(m_computerPlayersMutex);
		PlayerDataMap::const_iterator pos = m_computerPlayers.find(request.playerId);
		if (pos != m_computerPlayers.end())
			tmpPlayer = pos->second;
	}

	if (tmpPlayer.get())
	{
		// Send player info to client.
		boost::shared_ptr<NetPacket> info(new NetPacketPlayerInfo);
		NetPacketPlayerInfo::Data infoData;
		infoData.playerId = tmpPlayer->GetUniqueId();
		infoData.playerInfo.ptype = tmpPlayer->GetType();
		infoData.playerInfo.playerName = tmpPlayer->GetName();
		infoData.playerInfo.hasAvatar = !tmpPlayer->GetAvatarMD5().IsZero();
		if (infoData.playerInfo.hasAvatar)
			infoData.playerInfo.avatar = tmpPlayer->GetAvatarMD5();
		static_cast<NetPacketPlayerInfo *>(info.get())->SetData(infoData);
		GetSender().Send(session.sessionData->GetSocket(), info);
	}
	// TODO: handle error
}

void
ServerLobbyThread::HandleNetPacketRetrieveAvatar(SessionWrapper session, const NetPacketRetrieveAvatar &tmpPacket)
{
	NetPacketRetrieveAvatar::Data request;
	tmpPacket.GetData(request);

	string tmpFile;
	if (GetAvatarManager().GetAvatarFileName(request.avatar, tmpFile))
	{
		NetPacketList tmpPackets;
		if (GetAvatarManager().AvatarFileToNetPackets(tmpFile, request.requestId, tmpPackets))
			GetSender().SendLowPrio(session.sessionData->GetSocket(), tmpPackets);
		// TODO handle error
	}
}

void
ServerLobbyThread::HandleNetPacketCreateGame(SessionWrapper session, const NetPacketCreateGame &tmpPacket)
{
	// Create a new game.
	NetPacketCreateGame::Data createGameData;
	tmpPacket.GetData(createGameData);

	boost::shared_ptr<ServerGameThread> game(
		new ServerGameThread(
			*this, GetNextGameId(),
			createGameData.gameName,
			createGameData.password,
			GetGui(),
			m_playerConfig));
	game->Init(createGameData.gameData);

	MoveSessionToGame(*game, session);

	// Add game to list of games.
	InternalAddGame(game);

	// Start the game.
	game->Run();
}

void
ServerLobbyThread::HandleNetPacketJoinGame(SessionWrapper session, const NetPacketJoinGame &tmpPacket)
{
	// Join an existing game.
	NetPacketJoinGame::Data joinGameData;
	tmpPacket.GetData(joinGameData);

	GameMap::iterator pos = m_gameMap.find(joinGameData.gameId);

	if (pos != m_gameMap.end())
	{
		ServerGameThread &game = *pos->second;
		if (game.CheckPassword(joinGameData.password))
		{
			MoveSessionToGame(game, session);
		}
		else
		{
			SendJoinGameFailed(session.sessionData->GetSocket(), NTF_NET_JOIN_INVALID_PASSWORD);
		}
	}
	else
	{
		SessionError(session, ERR_NET_UNKNOWN_GAME);
	}
}

void
ServerLobbyThread::EstablishSession(SessionWrapper session)
{
	assert(session.playerData.get());
	// Send ACK to client.
	boost::shared_ptr<NetPacket> initAck(new NetPacketInitAck);
	NetPacketInitAck::Data initAckData;
	initAckData.sessionId = session.sessionData->GetId(); // TODO: currently unused.
	initAckData.playerId = session.playerData->GetUniqueId();
	static_cast<NetPacketInitAck *>(initAck.get())->SetData(initAckData);
	GetSender().Send(session.sessionData->GetSocket(), initAck);

	// Send the game list to the client.
	SendGameList(session.sessionData->GetSocket());

	// Session is now established.
	session.sessionData->SetState(SessionData::Established);
}

void
ServerLobbyThread::RequestPlayerAvatar(SessionWrapper session)
{
	assert(session.playerData.get());
	// Ask the client to send its avatar.
	boost::shared_ptr<NetPacket> retrieveAvatar(new NetPacketRetrieveAvatar);
	NetPacketRetrieveAvatar::Data retrieveAvatarData;
	retrieveAvatarData.requestId = session.playerData->GetUniqueId();
	retrieveAvatarData.avatar = session.playerData->GetAvatarMD5();
	static_cast<NetPacketRetrieveAvatar *>(retrieveAvatar.get())->SetData(retrieveAvatarData);
	GetSender().Send(session.sessionData->GetSocket(), retrieveAvatar);
}

void
ServerLobbyThread::CloseSessionLoop()
{
	boost::mutex::scoped_lock lock(m_closeSessionListMutex);

	CloseSessionList::iterator i = m_closeSessionList.begin();
	CloseSessionList::iterator end = m_closeSessionList.end();

	while (i != end)
	{
		CloseSessionList::iterator cur = i++;

		if (cur->first.elapsed().total_seconds() >= SERVER_CLOSE_SESSION_DELAY_SEC)
			m_closeSessionList.erase(cur);
	}
}

void
ServerLobbyThread::RemoveGameLoop()
{
	boost::mutex::scoped_lock lock(m_removeGameListMutex);

	RemoveGameList::iterator i = m_removeGameList.begin();
	RemoveGameList::iterator end = m_removeGameList.end();

	// Synchronously remove games which have been closed.
	while (i != end)
	{
		GameMap::iterator pos = m_gameMap.find(*i);
		if (pos != m_gameMap.end())
		{
			boost::shared_ptr<ServerGameThread> tmpGame = pos->second;
			tmpGame->SignalTermination();
			tmpGame->Join(GAME_THREAD_TERMINATE_TIMEOUT);
			InternalRemoveGame(tmpGame);
		}
		++i;
	}
	m_removeGameList.clear();
}

void
ServerLobbyThread::InternalAddGame(boost::shared_ptr<ServerGameThread> game)
{
	// Add game to list.
	m_gameMap.insert(GameMap::value_type(game->GetId(), game));
	// Notify all players.
	m_sessionManager.SendToAllSessions(GetSender(), CreateNetPacketGameListNew(*game), SessionData::Established);
	m_gameSessionManager.SendToAllSessions(GetSender(), CreateNetPacketGameListNew(*game), SessionData::Game);
}

void
ServerLobbyThread::InternalRemoveGame(boost::shared_ptr<ServerGameThread> game)
{
	// Remove game from list.
	m_gameMap.erase(game->GetId());
	// Remove all sessions left in the game.
	game->RemoveAllSessions();
	// Notify all players.
	boost::shared_ptr<NetPacket> packet = CreateNetPacketGameListUpdate(game->GetId(), GAME_MODE_CLOSED);
	m_sessionManager.SendToAllSessions(GetSender(), packet, SessionData::Established);
	m_gameSessionManager.SendToAllSessions(GetSender(), packet, SessionData::Game);
}

void
ServerLobbyThread::TerminateGames()
{
	GameMap::iterator i = m_gameMap.begin();
	GameMap::iterator end = m_gameMap.end();

	while (i != end)
	{
		i->second->SignalTermination();
		i->second->Join(GAME_THREAD_TERMINATE_TIMEOUT);
		++i;
	}
	m_gameMap.clear();
}

void
ServerLobbyThread::HandleNewConnection(boost::shared_ptr<ConnectData> connData)
{
	if (m_sessionManager.GetRawSessionCount() <= SERVER_MAX_NUM_SESSIONS)
	{
		// Create a random session id.
		// This id can be used to reconnect to the server if the connection was lost.
		unsigned sessionId;

		 // TODO: check for collisions.
		if(!RAND_bytes((unsigned char *)&sessionId, sizeof(sessionId)))
		{
			RAND_pseudo_bytes((unsigned char *)&sessionId, sizeof(sessionId)); 
		}

		// Create a new session.
		boost::shared_ptr<SessionData> sessionData(new SessionData(connData->ReleaseSocket(), sessionId));
		m_sessionManager.AddSession(sessionData);
	}
	else
	{
		// Server is full.
		// Create a generic session with Id 0.
		boost::shared_ptr<SessionData> sessionData(new SessionData(connData->ReleaseSocket(), 0));
		// Gracefully close this session.
		SessionError(SessionWrapper(sessionData, boost::shared_ptr<PlayerData>()), ERR_NET_SERVER_FULL);
	}
}

void
ServerLobbyThread::HandleReAddedSession(SessionWrapper session)
{
	// Remove session from game session list.
	m_gameSessionManager.RemoveSession(session.sessionData->GetSocket());

	if (m_sessionManager.GetRawSessionCount() <= SERVER_MAX_NUM_SESSIONS)
	{
		// Set state (back) to established.
		session.sessionData->SetState(SessionData::Established);
		// Add session to lobby list.
		m_sessionManager.AddSession(session);
	}
	else
	{
		// Gracefully close this session.
		SessionError(session, ERR_NET_SERVER_FULL);
	}
}

void
ServerLobbyThread::CleanupConnectQueue()
{
	boost::mutex::scoped_lock lock(m_connectQueueMutex);

	// Sockets will be closed automatically.
	m_connectQueue.clear();
}

void
ServerLobbyThread::SessionError(SessionWrapper session, int errorCode)
{
	if (session.sessionData.get())
	{
		SendError(session.sessionData->GetSocket(), errorCode);
		CloseSessionDelayed(session);
	}
}

void
ServerLobbyThread::SendError(SOCKET s, int errorCode)
{
	boost::shared_ptr<NetPacket> packet(new NetPacketError);
	NetPacketError::Data errorData;
	errorData.errorCode = errorCode;
	static_cast<NetPacketError *>(packet.get())->SetData(errorData);
	GetSender().Send(s, packet);
}

void
ServerLobbyThread::SendJoinGameFailed(SOCKET s, int reason)
{
	boost::shared_ptr<NetPacket> packet(new NetPacketJoinGameFailed);
	NetPacketJoinGameFailed::Data failedData;
	failedData.failureCode = reason;
	static_cast<NetPacketJoinGameFailed *>(packet.get())->SetData(failedData);
	GetSender().Send(s, packet);
}

void
ServerLobbyThread::SendGameList(SOCKET s)
{
	GameMap::const_iterator game_i = m_gameMap.begin();
	GameMap::const_iterator game_end = m_gameMap.end();
	while (game_i != game_end)
	{
		GetSender().Send(s, CreateNetPacketGameListNew(*game_i->second));
		++game_i;
	}
}

ServerCallback &
ServerLobbyThread::GetCallback()
{
	return m_gui;
}

SenderThread &
ServerLobbyThread::GetSender()
{
	assert(m_sender.get());
	return *m_sender;
}

ReceiverHelper &
ServerLobbyThread::GetReceiver()
{
	assert(m_receiver.get());
	return *m_receiver;
}

bool
ServerLobbyThread::CheckPassword(const string &password) const
{
	return (password == m_password);
}

ServerSenderCallback &
ServerLobbyThread::GetSenderCallback()
{
	assert(m_senderCallback.get());
	return *m_senderCallback;
}

GuiInterface &
ServerLobbyThread::GetGui()
{
	return m_gui;
}

bool
ServerLobbyThread::IsPlayerConnected(const string &name)
{
	bool retVal = false;

	retVal = m_sessionManager.IsPlayerConnected(name);

	if (!retVal)
		retVal = m_gameSessionManager.IsPlayerConnected(name);

	return retVal;
}

boost::shared_ptr<NetPacket>
ServerLobbyThread::CreateNetPacketGameListNew(const ServerGameThread &game)
{
	boost::shared_ptr<NetPacket> packet(new NetPacketGameListNew);
	NetPacketGameListNew::Data packetData;
	packetData.gameId = game.GetId();
	packetData.gameInfo.mode = game.IsRunning() ? GAME_MODE_STARTED : GAME_MODE_CREATED;
	packetData.gameInfo.name = game.GetName();
	packetData.gameInfo.data = game.GetGameData();
	packetData.gameInfo.players = game.GetPlayerIdList();
	packetData.gameInfo.isPasswordProtected = game.IsPasswordProtected();
	static_cast<NetPacketGameListNew *>(packet.get())->SetData(packetData);
	return packet;
}

boost::shared_ptr<NetPacket>
ServerLobbyThread::CreateNetPacketGameListUpdate(unsigned gameId, GameMode mode)
{
	boost::shared_ptr<NetPacket> packet(new NetPacketGameListUpdate);
	NetPacketGameListUpdate::Data packetData;
	packetData.gameId = gameId;
	packetData.gameMode = mode;
	static_cast<NetPacketGameListUpdate *>(packet.get())->SetData(packetData);
	return packet;
}

