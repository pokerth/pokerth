/***************************************************************************
 *   Copyright (C) 2007-2009 by Lothar May                                 *
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
#include <net/servergame.h>
#include <net/serverbanmanager.h>
#include <net/serverexception.h>
#include <net/senderhelper.h>
#include <net/sendercallback.h>
#include <net/serverircbotcallback.h>
#include <net/receiverhelper.h>
#include <net/socket_msg.h>
#include <net/chatcleanermanager.h>
#include <db/serverdbinterface.h>
#ifdef POKERTH_OFFICIAL_SERVER
	#include <dbclosed/serverdbfactoryinternal.h>
#else
	#include <db/serverdbfactorygeneric.h>
#endif
#include <core/avatarmanager.h>
#include <core/loghelper.h>
#include <core/openssl_wrapper.h>
#include <configfile.h>

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <boost/lambda/lambda.hpp>
#include <boost/filesystem.hpp>
#include <boost/bind.hpp>
#include <gsasl.h>

#define SERVER_MAX_NUM_SESSIONS						512		// Maximum number of idle users in lobby.

#define SERVER_CACHE_CLEANUP_INTERVAL_SEC			86400	// 1 day
#define SERVER_SAVE_STATISTICS_INTERVAL_SEC			60
#define SERVER_CHECK_SESSION_TIMEOUTS_INTERVAL_MSEC	500
#define SERVER_REMOVE_GAME_INTERVAL_MSEC			500
#define SERVER_REMOVE_PLAYER_INTERVAL_MSEC			100
#define SERVER_UPDATE_LOGIN_LOCK_INTERVAL_MSEC		1000
#define SERVER_PROCESS_SEND_INTERVAL_MSEC			10

#define SERVER_INIT_LOGIN_CLIENT_LOCK_SEC			30      // Forbid a client to send an additional avatar.

#define SERVER_INIT_SESSION_TIMEOUT_SEC				60
#define SERVER_TIMEOUT_WARNING_REMAINING_SEC		60
#define SERVER_SESSION_ACTIVITY_TIMEOUT_SEC			1800	// 30 min, MUST be > SERVER_TIMEOUT_WARNING_REMAINING_SEC
#define SERVER_SESSION_FORCED_TIMEOUT_SEC			86400	// 1 day, should be quite large.


#define SERVER_STATISTICS_FILE_NAME					"server_statistics.log"
#define SERVER_STATISTICS_STR_TOTAL_PLAYERS			"TotalNumPlayersLoggedIn"
#define SERVER_STATISTICS_STR_TOTAL_GAMES			"TotalNumGamesCreated"
#define SERVER_STATISTICS_STR_MAX_GAMES				"MaxGamesOpen"
#define SERVER_STATISTICS_STR_MAX_PLAYERS			"MaxPlayersLoggedIn"
#define SERVER_STATISTICS_STR_CUR_GAMES				"CurGamesOpen"
#define SERVER_STATISTICS_STR_CUR_PLAYERS			"CurPlayersLoggedIn"

using namespace std;
using boost::asio::ip::tcp;


class InternalServerCallback : public SenderCallback, public SessionDataCallback, public ChatCleanerCallback, public ServerDBCallback
{
public:
	InternalServerCallback(ServerLobbyThread &server) : m_server(server) {}
	virtual ~InternalServerCallback() {}

	virtual void SignalNetError(SessionId /*session*/, int /*errorID*/, int /*osErrorID*/)
	{
		// We just ignore send errors for now, on server side.
		// A serious send error should trigger a read error or a read
		// returning 0 afterwards, and we will handle this error.
	}
	virtual void SignalSessionTerminated(unsigned session)
	{
		m_server.GetSender().SignalSessionTerminated(session);
	}
	virtual void SignalChatBotMessage(const string &msg)
	{
		m_server.SendChatBotMsg(msg);
	}

	virtual void ConnectSuccess()
	{
		LOG_MSG("Successfully connected to database.");
	}

	virtual void ConnectFailed(const string &error)
	{
		LOG_ERROR("DB connect error: " << error);
	}

	virtual void QueryError(const std::string &error)
	{
		LOG_ERROR("DB query error: " << error);
	}

	virtual void PlayerLoginSuccess(unsigned requestId, DB_id dbPlayerId, const std::string &secret)
	{
		m_server.UserValid(requestId, dbPlayerId, secret);
	}

	virtual void PlayerLoginFailed(unsigned requestId)
	{
		m_server.UserInvalid(requestId);
	}

	virtual void CreateGameSuccess(unsigned requestId, DB_id gameId)
	{
		m_server.SetGameDBId((u_int32_t)requestId, gameId);
	}

	virtual void CreateGameFailed(unsigned requestId)
	{
		// TODO maybe handle request id.
		LOG_ERROR("DB create game failed for request " << requestId);
	}

private:
	ServerLobbyThread &m_server;
};


ServerLobbyThread::ServerLobbyThread(GuiInterface &gui, ServerMode mode, ServerIrcBotCallback &ircBotCb, ConfigFile *playerConfig,
									 AvatarManager &avatarManager, boost::shared_ptr<boost::asio::io_service> ioService)
: m_ioService(ioService), m_authContext(NULL), m_gui(gui), m_ircBotCb(ircBotCb), m_avatarManager(avatarManager),
  m_mode(mode), m_playerConfig(playerConfig), m_curGameId(0), m_curUniquePlayerId(0), m_curSessionId(INVALID_SESSION + 1),
  m_statDataChanged(false), m_removeGameTimer(*ioService), m_removePlayerTimer(*ioService),
  m_sessionTimeoutTimer(*ioService), m_avatarCleanupTimer(*ioService),
  m_saveStatisticsTimer(*ioService), m_loginLockTimer(*ioService),
  m_startTime(boost::posix_time::second_clock::local_time())
{
	m_internalServerCallback.reset(new InternalServerCallback(*this));
	m_sender.reset(new SenderHelper(*m_internalServerCallback, m_ioService));
	m_receiver.reset(new ReceiverHelper);
	m_banManager.reset(new ServerBanManager(m_ioService));
	m_chatCleanerManager.reset(new ChatCleanerManager(*m_internalServerCallback, m_ioService));
	DBFactory dbFactory;
	m_database = dbFactory.CreateServerDBObject(*m_internalServerCallback, m_ioService);
}

ServerLobbyThread::~ServerLobbyThread()
{
}

void
ServerLobbyThread::Init(const string &logDir)
{
	// Read previous server statistics.
	if (!logDir.empty())
	{
		boost::filesystem::path logPath(logDir);
		if (!logDir.empty())
		{
			logPath /= SERVER_STATISTICS_FILE_NAME;
			m_statisticsFileName = logPath.directory_string();
			ReadStatisticsFile();
		}
	}
	m_database->Init("127.0.0.1", "user", "password", "database", "key");
}

void
ServerLobbyThread::SignalTermination()
{
	Thread::SignalTermination();
	m_ioService->stop();
}

void
ServerLobbyThread::AddConnection(boost::shared_ptr<tcp::socket> sock)
{
	// Create a random session id.
	// This id can be used to reconnect to the server if the connection was lost.
	//unsigned sessionId;

	// TODO: use randomized method.
	//if(!RAND_bytes((unsigned char *)&sessionId, sizeof(sessionId)))
	//{
	//	RAND_pseudo_bytes((unsigned char *)&sessionId, sizeof(sessionId));
	//}

	// Create a new session.
	boost::shared_ptr<SessionData> sessionData(new SessionData(sock, m_curSessionId++, *m_internalServerCallback));
	m_sessionManager.AddSession(sessionData);

	LOG_VERBOSE("Accepted connection - session #" << sessionData->GetId() << ".");

	bool hasClientIp = false;
	if (m_sessionManager.GetRawSessionCount() <= SERVER_MAX_NUM_SESSIONS)
	{
		boost::system::error_code errCode;
		tcp::endpoint clientEndpoint = sock->remote_endpoint(errCode);
		if (!errCode)
		{
			string ipAddress = clientEndpoint.address().to_string(errCode);
			if (!errCode && !ipAddress.empty())
			{
				sessionData->SetClientAddr(ipAddress);
				hasClientIp = true;

				boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
				packet->GetMsg()->present = PokerTHMessage_PR_announceMessage;
				AnnounceMessage_t *netAnnounce = &packet->GetMsg()->choice.announceMessage;
				netAnnounce->protocolVersion.major = NET_VERSION_MAJOR;
				netAnnounce->protocolVersion.minor = NET_VERSION_MINOR;
				netAnnounce->latestGameVersion.major = POKERTH_VERSION_MAJOR;
				netAnnounce->latestGameVersion.minor = POKERTH_VERSION_MINOR;
				netAnnounce->latestBetaRevision = POKERTH_BETA_REVISION;
				switch (m_mode)
				{
					case SERVER_MODE_LAN:
						netAnnounce->serverType = serverType_serverTypeLAN;
						break;
					case SERVER_MODE_INTERNET_NOAUTH:
						netAnnounce->serverType = serverType_serverTypeInternetNoAuth;
						break;
					case SERVER_MODE_INTERNET_AUTH:
						netAnnounce->serverType = serverType_serverTypeInternetAuth;
						break;
				}
				GetSender().Send(sessionData, packet);

				sock->async_read_some(
					boost::asio::buffer(sessionData->GetReceiveBuffer().recvBuf, RECV_BUF_SIZE),
					boost::bind(
						&ServerLobbyThread::HandleRead,
						this,
						boost::asio::placeholders::error,
						sessionData->GetId(),
						boost::asio::placeholders::bytes_transferred));
			}
		}
		if (!hasClientIp)
		{
			// We do not accept sessions if we cannot
			// retrieve the client address.
			SessionError(SessionWrapper(sessionData, boost::shared_ptr<PlayerData>()), ERR_NET_INVALID_SESSION);
		}
	}
	else
	{
		// Server is full.
		// Gracefully close this session.
		SessionError(SessionWrapper(sessionData, boost::shared_ptr<PlayerData>()), ERR_NET_SERVER_FULL);
	}
}

void
ServerLobbyThread::ReAddSession(SessionWrapper session, int reason)
{
	if (session.sessionData.get() && session.playerData.get())
	{
		boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
		packet->GetMsg()->present = PokerTHMessage_PR_gamePlayerMessage;
		GamePlayerMessage_t *netPlayerMsg = &packet->GetMsg()->choice.gamePlayerMessage;
		netPlayerMsg->gameId = session.sessionData->GetGameId();
		netPlayerMsg->gamePlayerNotification.present = gamePlayerNotification_PR_removedFromGame;
		RemovedFromGame_t *removed = &netPlayerMsg->gamePlayerNotification.choice.removedFromGame;

		switch (reason)
		{
			case NTF_NET_REMOVED_GAME_FULL :
				removed->removedFromGameReason = removedFromGameReason_gameIsFull;
				break;
			case NTF_NET_REMOVED_ALREADY_RUNNING :
				removed->removedFromGameReason = removedFromGameReason_gameIsRunning;
				break;
			case NTF_NET_REMOVED_KICKED :
				removed->removedFromGameReason = removedFromGameReason_kickedFromGame;
				break;
			case NTF_NET_REMOVED_TIMEOUT :
				removed->removedFromGameReason = removedFromGameReason_gameTimeout;
				break;
			case NTF_NET_REMOVED_START_FAILED :
				removed->removedFromGameReason = removedFromGameReason_removedStartFailed;
				break;
			default :
				removed->removedFromGameReason = removedFromGameReason_removedOnRequest;
				break;
		}
		GetSender().Send(session.sessionData, packet);

		HandleReAddedSession(session);
	}
}

void
ServerLobbyThread::MoveSessionToGame(ServerGame &game, SessionWrapper session)
{
	// Remove session from the lobby.
	m_sessionManager.RemoveSession(session.sessionData->GetId());
	// Session is now in game state.
	session.sessionData->SetState(SessionData::Game);
	// Store it in the list of game sessions.
	m_gameSessionManager.AddSession(session);
	// Set the game id of the session.
	session.sessionData->SetGameId(game.GetId());
	// Add session to the game.
	game.AddSession(session);
}

void
ServerLobbyThread::RemoveSessionFromGame(SessionWrapper session)
{
	// Just remove the session. Only for fatal errors.
	CloseSession(session);
	session.sessionData->SetGameId(0);
}

void
ServerLobbyThread::CloseSession(SessionWrapper session)
{
	if (session.sessionData && session.sessionData->GetState() != SessionData::Closed) // Make this call reentrant.
	{
		LOG_VERBOSE("Closing session #" << session.sessionData->GetId() << ".");
		session.sessionData->SetState(SessionData::Closed);

		m_sessionManager.RemoveSession(session.sessionData->GetId());
		m_gameSessionManager.RemoveSession(session.sessionData->GetId());

		if (session.playerData)
			NotifyPlayerLeftLobby(session.playerData->GetUniqueId());
		// Update stats (if needed).
		UpdateStatisticsNumberOfPlayers();
	}
}

void
ServerLobbyThread::ResubscribeLobbyMsg(SessionWrapper session)
{
	InternalResubscribeMsg(session);
}

void
ServerLobbyThread::NotifyPlayerJoinedLobby(unsigned playerId)
{
	boost::shared_ptr<NetPacket> notify = CreateNetPacketPlayerListNew(playerId);
	m_sessionManager.SendLobbyMsgToAllSessions(GetSender(), notify, SessionData::Established);
	m_gameSessionManager.SendLobbyMsgToAllSessions(GetSender(), notify, SessionData::Game);
}

void
ServerLobbyThread::NotifyPlayerLeftLobby(unsigned playerId)
{
	boost::shared_ptr<NetPacket> notify = CreateNetPacketPlayerListLeft(playerId);
	m_sessionManager.SendLobbyMsgToAllSessions(GetSender(), notify, SessionData::Established);
	m_gameSessionManager.SendLobbyMsgToAllSessions(GetSender(), notify, SessionData::Game);
}

void
ServerLobbyThread::NotifyPlayerJoinedGame(unsigned gameId, unsigned playerId)
{
	// Send notification to players in lobby.
	boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
	packet->GetMsg()->present = PokerTHMessage_PR_gameListMessage;
	GameListMessage_t *netListMsg = &packet->GetMsg()->choice.gameListMessage;
	netListMsg->gameId = gameId;

	netListMsg->gameListNotification.present = gameListNotification_PR_gameListPlayerJoined;
	GameListPlayerJoined_t *playerJoined = &netListMsg->gameListNotification.choice.gameListPlayerJoined;
	playerJoined->playerId = playerId;

	m_sessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Established);
	m_gameSessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Game);
}

void
ServerLobbyThread::NotifyPlayerLeftGame(unsigned gameId, unsigned playerId)
{
	// Send notification to players in lobby.
	boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
	packet->GetMsg()->present = PokerTHMessage_PR_gameListMessage;
	GameListMessage_t *netListMsg = &packet->GetMsg()->choice.gameListMessage;
	netListMsg->gameId = gameId;

	netListMsg->gameListNotification.present = gameListNotification_PR_gameListPlayerLeft;
	GameListPlayerLeft_t *playerLeft = &netListMsg->gameListNotification.choice.gameListPlayerLeft;
	playerLeft->playerId = playerId;

	m_sessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Established);
	m_gameSessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Game);
}

void
ServerLobbyThread::NotifyGameAdminChanged(unsigned gameId, unsigned newAdminPlayerId)
{
	// Send notification to players in lobby.
	boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
	packet->GetMsg()->present = PokerTHMessage_PR_gameListMessage;
	GameListMessage_t *netListMsg = &packet->GetMsg()->choice.gameListMessage;
	netListMsg->gameId = gameId;

	netListMsg->gameListNotification.present = gameListNotification_PR_gameListAdminChanged;
	GameListAdminChanged_t *adminChanged = &netListMsg->gameListNotification.choice.gameListAdminChanged;
	adminChanged->newAdminPlayerId = newAdminPlayerId;

	m_sessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Established);
	m_gameSessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Game);
}

void
ServerLobbyThread::NotifyStartingGame(unsigned gameId)
{
	boost::shared_ptr<NetPacket> packet = CreateNetPacketGameListUpdate(gameId, GAME_MODE_STARTED);
	m_sessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Established);
	m_gameSessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Game);
}

void
ServerLobbyThread::NotifyReopeningGame(unsigned gameId)
{
	boost::shared_ptr<NetPacket> packet = CreateNetPacketGameListUpdate(gameId, GAME_MODE_CREATED);
	m_sessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Established);
	m_gameSessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Game);
}

void
ServerLobbyThread::HandleGameRetrievePlayerInfo(SessionWrapper session, const PlayerInfoRequestMessage_t &playerInfoRequest)
{
	// Someone within a game requested player info.
	HandleNetPacketRetrievePlayerInfo(session, playerInfoRequest);
}

void
ServerLobbyThread::HandleGameRetrieveAvatar(SessionWrapper session, const AvatarRequestMessage_t &retrieveAvatar)
{
	// Someone within a game requested an avatar.
	HandleNetPacketRetrieveAvatar(session, retrieveAvatar);
}

void
ServerLobbyThread::HandleChatRequest(SessionWrapper session, const ChatRequestMessage_t &chatRequest)
{
	// Someone within a game sent a lobby message.
	HandleNetPacketChatRequest(session, chatRequest);
}

bool
ServerLobbyThread::KickPlayerByName(const std::string &playerName)
{
	bool retVal = false;
	SessionWrapper session = m_sessionManager.GetSessionByPlayerName(playerName);
	if (!session.sessionData.get())
		session = m_gameSessionManager.GetSessionByPlayerName(playerName);

	if (session.sessionData.get() && session.playerData.get())
	{
		RemovePlayer(session.playerData->GetUniqueId(), ERR_NET_PLAYER_KICKED);
		retVal = true;
	}

	return retVal;
}

string
ServerLobbyThread::GetPlayerIPAddress(const std::string &playerName) const
{
	string ipAddress;
	SessionWrapper session = m_sessionManager.GetSessionByPlayerName(playerName);
	if (!session.sessionData.get())
		session = m_gameSessionManager.GetSessionByPlayerName(playerName);

	if (session.sessionData.get() && session.playerData.get())
		ipAddress = session.sessionData->GetClientAddr();

	return ipAddress;
}

void
ServerLobbyThread::RemovePlayer(unsigned playerId, unsigned errorCode)
{
	boost::mutex::scoped_lock lock(m_removePlayerListMutex);
	m_removePlayerList.push_back(RemovePlayerList::value_type(playerId, errorCode));
}

void
ServerLobbyThread::SendGlobalChat(const string &message)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
	packet->GetMsg()->present = PokerTHMessage_PR_chatMessage;
	ChatMessage_t *netChat = &packet->GetMsg()->choice.chatMessage;
	netChat->chatType.present = chatType_PR_chatTypeBroadcast;
	OCTET_STRING_fromBuf(
		&netChat->chatText,
		message.c_str(),
		message.length());

	m_sessionManager.SendToAllSessions(GetSender(), packet, SessionData::Established);
	m_gameSessionManager.SendToAllSessions(GetSender(), packet, SessionData::Game);
}

void
ServerLobbyThread::SendGlobalMsgBox(const string &message)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
	packet->GetMsg()->present = PokerTHMessage_PR_dialogMessage;
	DialogMessage_t *netDialog = &packet->GetMsg()->choice.dialogMessage;

	OCTET_STRING_fromBuf(
		&netDialog->notificationText,
		message.c_str(),
		message.length());
	m_sessionManager.SendToAllSessions(GetSender(), packet, SessionData::Established);
	m_gameSessionManager.SendToAllSessions(GetSender(), packet, SessionData::Game);
}

void
ServerLobbyThread::SendChatBotMsg(const std::string &message)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
	packet->GetMsg()->present = PokerTHMessage_PR_chatMessage;
	ChatMessage_t *netChat = &packet->GetMsg()->choice.chatMessage;
	netChat->chatType.present = chatType_PR_chatTypeBot;
	OCTET_STRING_fromBuf(
		&netChat->chatText,
		message.c_str(),
		message.length());

	m_sessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Established);
	m_gameSessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Game);

	GetIrcBotCallback().SignalLobbyMessage(
		0,
		"(chat bot)", 
		message);
}

void
ServerLobbyThread::ReconnectChatBot()
{
	m_chatCleanerManager->ReInit();
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

AvatarManager &
ServerLobbyThread::GetAvatarManager()
{
	return m_avatarManager;
}

ServerStats
ServerLobbyThread::GetStats() const
{
	boost::mutex::scoped_lock lock(m_statMutex);
	return m_statData;
}

boost::posix_time::ptime
ServerLobbyThread::GetStartTime() const
{
	return m_startTime;
}

SenderHelper &
ServerLobbyThread::GetSender()
{
	assert(m_sender);
	return *m_sender;
}

boost::asio::io_service &
ServerLobbyThread::GetIOService()
{
	assert(m_ioService);
	return *m_ioService;
}

ServerDBInterface &
ServerLobbyThread::GetDatabase()
{
	assert(m_database);
	return *m_database;
}

ServerBanManager &
ServerLobbyThread::GetBanManager()
{
	assert(m_banManager);
	return *m_banManager;
}

u_int32_t
ServerLobbyThread::GetNextUniquePlayerId()
{
	boost::mutex::scoped_lock lock(m_curUniquePlayerIdMutex);
	m_curUniquePlayerId++;
	if (m_curUniquePlayerId == 0) // 0 is an invalid id.
		m_curUniquePlayerId++;

	return m_curUniquePlayerId;
}

u_int32_t
ServerLobbyThread::GetNextGameId()
{
	m_curGameId++;
	if (m_curGameId == 0) // 0 is an invalid id.
		m_curGameId++;

	return m_curGameId;
}

void
ServerLobbyThread::Main()
{
	try
	{
		InitAuthContext();

		InitChatCleaner();
		// Start database engine.
		m_database->Start();
		// Register all timers.
		RegisterTimers();

		boost::asio::io_service::work ioWork(*m_ioService);
		m_ioService->run(); // Will only be aborted asynchronously.

	} catch (const PokerTHException &e)
	{
		GetCallback().SignalNetServerError(e.GetErrorId(), e.GetOsErrorCode());
		LOG_ERROR("Lobby exception: " << e.what());
	}
	// Clear all sessions.
	m_sessionManager.Clear();
	m_gameSessionManager.Clear();
	// Cancel pending timer callbacks.
	CancelTimers();
	// Stop database engine.
	m_database->Stop();

	ClearAuthContext();
}

void
ServerLobbyThread::RegisterTimers()
{
	// Remove closed games.
	m_removeGameTimer.expires_from_now(
		boost::posix_time::milliseconds(SERVER_REMOVE_GAME_INTERVAL_MSEC));
	m_removeGameTimer.async_wait(
		boost::bind(
			&ServerLobbyThread::TimerRemoveGame, shared_from_this(), boost::asio::placeholders::error));
	// Remove inactive/kicked players.
	m_removePlayerTimer.expires_from_now(
		boost::posix_time::milliseconds(SERVER_REMOVE_PLAYER_INTERVAL_MSEC));
	m_removePlayerTimer.async_wait(
		boost::bind(
			&ServerLobbyThread::TimerRemovePlayer, shared_from_this(), boost::asio::placeholders::error));
	// Check the timeout of sessions which have not been initialised.
	m_sessionTimeoutTimer.expires_from_now(
		boost::posix_time::milliseconds(SERVER_CHECK_SESSION_TIMEOUTS_INTERVAL_MSEC));
	m_sessionTimeoutTimer.async_wait(
		boost::bind(
			&ServerLobbyThread::TimerCheckSessionTimeouts, shared_from_this(), boost::asio::placeholders::error));
	// Cleanup the avatar cache. Note: Only works if there are no users on the server.
	m_avatarCleanupTimer.expires_from_now(
		boost::posix_time::seconds(SERVER_CACHE_CLEANUP_INTERVAL_SEC));
	m_avatarCleanupTimer.async_wait(
		boost::bind(
			&ServerLobbyThread::TimerCleanupAvatarCache, shared_from_this(), boost::asio::placeholders::error));
	// Update the statistics file.
	m_saveStatisticsTimer.expires_from_now(
		boost::posix_time::seconds(SERVER_SAVE_STATISTICS_INTERVAL_SEC));
	m_saveStatisticsTimer.async_wait(
		boost::bind(
			&ServerLobbyThread::TimerSaveStatisticsFile, shared_from_this(), boost::asio::placeholders::error));
	// Update the avatar upload locks.
	m_loginLockTimer.expires_from_now(
		boost::posix_time::milliseconds(SERVER_UPDATE_LOGIN_LOCK_INTERVAL_MSEC));
	m_loginLockTimer.async_wait(
		boost::bind(
			&ServerLobbyThread::TimerUpdateClientLoginLock, shared_from_this(), boost::asio::placeholders::error));
}

void
ServerLobbyThread::CancelTimers()
{
	m_removeGameTimer.cancel();
	m_removePlayerTimer.cancel();
	m_sessionTimeoutTimer.cancel();
	m_avatarCleanupTimer.cancel();
	m_saveStatisticsTimer.cancel();
	m_loginLockTimer.cancel();
}

void
ServerLobbyThread::InitAuthContext()
{
	int res = gsasl_init(&m_authContext);
	if (res != GSASL_OK)
		throw ServerException(__FILE__, __LINE__, ERR_NET_GSASL_INIT_FAILED, 0);

	if (!gsasl_server_support_p(m_authContext, "SCRAM-SHA-1"))
	{
		gsasl_done(m_authContext);
		throw ServerException(__FILE__, __LINE__, ERR_NET_GSASL_NO_SCRAM, 0);
	}
}

void
ServerLobbyThread::ClearAuthContext()
{
	gsasl_done(m_authContext);
	m_authContext = NULL;
}

void
ServerLobbyThread::InitChatCleaner()
{
	if (m_playerConfig->readConfigInt("UseChatCleaner") != 0)
	{
		m_chatCleanerManager->Init(
				m_playerConfig->readConfigString("ChatCleanerHostAddress"),
				m_playerConfig->readConfigInt("ChatCleanerPort"),
				m_playerConfig->readConfigInt("ChatCleanerUseIpv6") != 0,
				m_playerConfig->readConfigString("ChatCleanerClientAuth"),
				m_playerConfig->readConfigString("ChatCleanerServerAuth"));
	}
}

void
ServerLobbyThread::HandleRead(const boost::system::error_code &ec, SessionId sessionId, size_t bytesRead)
{
	if (ec != boost::asio::error::operation_aborted)
	{
		try
		{
			// Find the session.
			SessionWrapper session = m_sessionManager.GetSessionById(sessionId);
			if (!session.sessionData)
				session = m_gameSessionManager.GetSessionById(sessionId);
			if (session.sessionData)
			{
				ReceiveBuffer &buf = session.sessionData->GetReceiveBuffer();
				if (!ec)
				{
					if (buf.recvBufUsed + bytesRead > RECV_BUF_SIZE)
						LOG_ERROR("Session " << session.sessionData->GetId() << " - Internal error: Receive buffer overflow!");
					buf.recvBufUsed += bytesRead;
					GetReceiver().ScanPackets(buf);
					bool errorFlag = false;

					while (!buf.receivedPackets.empty())
					{
						boost::shared_ptr<NetPacket> packet = buf.receivedPackets.front();
						buf.receivedPackets.pop_front();
						// Retrieve current game, if applicable.
						boost::shared_ptr<ServerGame> game = InternalGetGameFromId(session.sessionData->GetGameId());
						if (game)
						{
							// We need to catch game-specific exceptions, so that they do not affect the server.
							try
							{
								game->HandlePacket(session, packet);
							} catch (const PokerTHException &e)
							{
								LOG_ERROR("Game " << game->GetId() << " - Read handler exception: " << e.what());
								game->RemoveAllSessions();
								errorFlag = true;
								break;
							}
						}
						else
							HandlePacket(session, packet);
					}
					if (buf.recvBufUsed >= RECV_BUF_SIZE)
					{
						LOG_ERROR("Session " << session.sessionData->GetId() << " - Full receive buf but no valid packet.");
						buf.recvBufUsed = 0;
					}
					if (!errorFlag)
					{
						session.sessionData->GetAsioSocket()->async_read_some(
							boost::asio::buffer(buf.recvBuf + buf.recvBufUsed, RECV_BUF_SIZE - buf.recvBufUsed),
							boost::bind(
								&ServerLobbyThread::HandleRead,
								this,
								boost::asio::placeholders::error,
								sessionId,
								boost::asio::placeholders::bytes_transferred));
					}
				}
				else if (ec == boost::asio::error::interrupted || ec == boost::asio::error::try_again)
				{
					LOG_ERROR("Session " << sessionId << " - recv interrupted: " << ec);
					session.sessionData->GetAsioSocket()->async_read_some(
						boost::asio::buffer(buf.recvBuf + buf.recvBufUsed, RECV_BUF_SIZE - buf.recvBufUsed),
						boost::bind(
							&ServerLobbyThread::HandleRead,
							this,
							boost::asio::placeholders::error,
							sessionId,
							boost::asio::placeholders::bytes_transferred));
				}
				else
				{
					LOG_ERROR("Session " << sessionId << " - Connection closed: " << ec);
					// On error: Close this session.
					boost::shared_ptr<ServerGame> game = InternalGetGameFromId(session.sessionData->GetGameId());
					if (game)
						game->ErrorRemoveSession(session);
					else
						CloseSession(session);
				}
			}
		} catch (const exception &e)
		{
			LOG_ERROR("Session " << sessionId << " - unhandled exception in HandleRead: " << e.what());
		}
	}
}

void
ServerLobbyThread::HandlePacket(SessionWrapper session, boost::shared_ptr<NetPacket> packet)
{
	if (session.sessionData && packet)
	{
		if (packet->IsClientActivity())
			session.sessionData->ResetActivityTimer();

		if (session.sessionData->GetState() == SessionData::Init)
		{
			if (packet->GetMsg()->present == PokerTHMessage_PR_initMessage)
				HandleNetPacketInit(session, packet->GetMsg()->choice.initMessage);
			else if (packet->GetMsg()->present == PokerTHMessage_PR_authMessage)
			{
				AuthMessage_t *authMsg = &packet->GetMsg()->choice.authMessage;
				if (authMsg->present == AuthMessage_PR_authClientResponse)
					HandleNetPacketAuthClientResponse(session, authMsg->choice.authClientResponse);
				else
					SessionError(session, ERR_SOCK_INVALID_STATE);
			}
			else if (packet->GetMsg()->present == PokerTHMessage_PR_avatarReplyMessage)
			{
				AvatarReplyMessage_t *avatarReply = &packet->GetMsg()->choice.avatarReplyMessage;
				if (avatarReply->avatarResult.present == avatarResult_PR_avatarHeader)
					HandleNetPacketAvatarHeader(session, avatarReply->requestId, avatarReply->avatarResult.choice.avatarHeader);
				else if (avatarReply->avatarResult.present == avatarResult_PR_unknownAvatar)
					HandleNetPacketUnknownAvatar(session, avatarReply->requestId, avatarReply->avatarResult.choice.unknownAvatar);
				else
					SessionError(session, ERR_SOCK_INVALID_STATE);
			}
			else
				SessionError(session, ERR_SOCK_INVALID_STATE);
		}
		else if (session.sessionData->GetState() == SessionData::ReceivingAvatar)
		{
			if (packet->GetMsg()->present == PokerTHMessage_PR_avatarReplyMessage)
			{
				AvatarReplyMessage_t *avatarReply = &packet->GetMsg()->choice.avatarReplyMessage;
				if (avatarReply->avatarResult.present == avatarResult_PR_avatarData)
					HandleNetPacketAvatarFile(session, avatarReply->requestId, avatarReply->avatarResult.choice.avatarData);
				else if (avatarReply->avatarResult.present == avatarResult_PR_avatarEnd)
					HandleNetPacketAvatarEnd(session, avatarReply->requestId, avatarReply->avatarResult.choice.avatarEnd);
				else
					SessionError(session, ERR_SOCK_INVALID_STATE);
			}
			else
				SessionError(session, ERR_SOCK_INVALID_STATE);
		}
		else
		{
			if (packet->GetMsg()->present == PokerTHMessage_PR_playerInfoRequestMessage)
				HandleNetPacketRetrievePlayerInfo(session, packet->GetMsg()->choice.playerInfoRequestMessage);
			else if (packet->GetMsg()->present == PokerTHMessage_PR_avatarRequestMessage)
				HandleNetPacketRetrieveAvatar(session, packet->GetMsg()->choice.avatarRequestMessage);
			else if (packet->GetMsg()->present == PokerTHMessage_PR_resetTimeoutMessage)
			{}
			else if (packet->GetMsg()->present == PokerTHMessage_PR_subscriptionRequestMessage)
			{
				SubscriptionRequestMessage_t *subscriptionRequest = &packet->GetMsg()->choice.subscriptionRequestMessage;
				if (subscriptionRequest->subscriptionAction == subscriptionAction_resubscribeGameList)
					InternalResubscribeMsg(session);
				else
					session.sessionData->ResetWantsLobbyMsg();
			}
			else if (packet->GetMsg()->present == PokerTHMessage_PR_joinGameRequestMessage)
			{
				JoinGameRequestMessage_t *joinRequest = &packet->GetMsg()->choice.joinGameRequestMessage;
				string password;
				if (joinRequest->password)
					password = string((char *)joinRequest->password->buf, joinRequest->password->size);
				if (joinRequest->joinGameAction.present == joinGameAction_PR_joinNewGame)
					HandleNetPacketCreateGame(session, password, joinRequest->joinGameAction.choice.joinNewGame);
				else if (joinRequest->joinGameAction.present == joinGameAction_PR_joinExistingGame)
					HandleNetPacketJoinGame(session, password, joinRequest->joinGameAction.choice.joinExistingGame);
			}
			else if (packet->GetMsg()->present == PokerTHMessage_PR_chatRequestMessage)
				HandleNetPacketChatRequest(session, packet->GetMsg()->choice.chatRequestMessage);
			else
				SessionError(session, ERR_SOCK_INVALID_STATE);
		}
	}
}

void
ServerLobbyThread::HandleNetPacketInit(SessionWrapper session, const InitMessage_t &initMessage)
{
	LOG_VERBOSE("Received init for session #" << session.sessionData->GetId() << ".");

	// Before any other processing, perform some denial of service and
	// brute force attack prevention by checking whether the user recently sent an
	// Init packet.
	bool recentlySentInit = false;
	{
		boost::mutex::scoped_lock lock(m_timerClientAddressMapMutex);
		if (m_timerClientAddressMap.find(session.sessionData->GetClientAddr()) != m_timerClientAddressMap.end())
			recentlySentInit = true;
		else
			m_timerClientAddressMap[session.sessionData->GetClientAddr()] = boost::timers::portable::microsec_timer();
	}
	if (recentlySentInit)
	{
		SessionError(session, ERR_NET_INIT_BLOCKED);
		return;
	}

	// Check the protocol version.
	if (initMessage.requestedVersion.major != NET_VERSION_MAJOR
		|| session.playerData) // Has this session already sent an init?
	{
		SessionError(session, ERR_NET_VERSION_NOT_SUPPORTED);
		return;
	}

	string playerName;
	MD5Buf avatarMD5;
	bool noAuth = false;
	bool validGuest = false;
	if (initMessage.login.present == login_PR_guestLogin)
	{
		const GuestLogin_t *guestLogin = &initMessage.login.choice.guestLogin;
		playerName = STL_STRING_FROM_OCTET_STRING(guestLogin->nickName);
		// Verify guest player name.
		if (playerName.length() > sizeof(SERVER_GUEST_PLAYER_NAME - 1)
			&& playerName.substr(0, sizeof(SERVER_GUEST_PLAYER_NAME) - 1) == SERVER_GUEST_PLAYER_NAME)
		{
			string guestId(playerName.substr(sizeof(SERVER_GUEST_PLAYER_NAME)));
			if (count_if(guestId.begin(), guestId.end(), ::isdigit) == guestId.size())
			{
				validGuest = true;
				noAuth = true;
			}
		}
		if (!validGuest)
		{
			SessionError(session, ERR_NET_INVALID_PLAYER_NAME);
			return;
		}
	}
#ifdef POKERTH_OFFICIAL_SERVER
	else if (initMessage.login.present == login_PR_authenticatedLogin)
	{
		const AuthenticatedLogin_t *authLogin = &initMessage.login.choice.authenticatedLogin;
		string inAuthData((const char *)authLogin->clientUserData.buf, authLogin->clientUserData.size);
		if (authLogin->avatar)
			memcpy(avatarMD5.data, authLogin->avatar->buf, MD5_DATA_SIZE);
		session.sessionData->CreateServerAuthSession(m_authContext);
		if (session.sessionData->AuthStep(1, inAuthData))
			playerName = session.sessionData->AuthGetUser();
	}
#else
	else if (initMessage.login.present == login_PR_unauthenticatedLogin)
	{
		const UnauthenticatedLogin_t *noauthLogin = &initMessage.login.choice.unauthenticatedLogin;
		playerName = STL_STRING_FROM_OCTET_STRING(noauthLogin->nickName);
		if (noauthLogin->avatar)
			memcpy(avatarMD5.data, noauthLogin->avatar->buf, MD5_DATA_SIZE);
		noAuth = true;
	}
#endif
	else
	{
		SessionError(session, ERR_NET_INVALID_PASSWORD);
		return;
	}

	// Check whether the player name is correct.
	// Partly, this is also done in netpacket.
	// However, some disallowed names are checked only here.
	if (playerName.empty()
		|| playerName[0] == '#'
		|| playerName[0] == ' '
		|| playerName.substr(0, sizeof(SERVER_COMPUTER_PLAYER_NAME) - 1) == SERVER_COMPUTER_PLAYER_NAME)
	{
		SessionError(session, ERR_NET_INVALID_PLAYER_NAME);
		return;
	}

	// Check whether this player is already connected.
	if (IsPlayerConnected(playerName))
	{
		SessionError(session, ERR_NET_PLAYER_NAME_IN_USE);
		return;
	}

	// Check whether the player name is banned.
	if (GetBanManager().IsPlayerBanned(playerName))
	{
		SessionError(session, ERR_NET_PLAYER_BANNED);
		return;
	}
	// Check whether the peer IP address is banned.
	if (GetBanManager().IsIPAddressBanned(session.sessionData->GetClientAddr()))
	{
		SessionError(session, ERR_NET_PLAYER_BANNED);
		return;
	}

	// Create player data object.
	boost::shared_ptr<PlayerData> tmpPlayerData(
		new PlayerData(GetNextUniquePlayerId(), 0, PLAYER_TYPE_HUMAN, PLAYER_RIGHTS_NORMAL));
	tmpPlayerData->SetName(playerName);
	tmpPlayerData->SetNetSessionData(session.sessionData);
	tmpPlayerData->SetAvatarMD5(avatarMD5);

	// Set player data for session.
	m_sessionManager.SetSessionPlayerData(session.sessionData->GetId(), tmpPlayerData);
	session.playerData = tmpPlayerData;

	if (noAuth)
		InitAfterLogin(session);
	else
		AuthenticatePlayer(session);
}

void
ServerLobbyThread::HandleNetPacketAuthClientResponse(SessionWrapper session, const AuthClientResponse_t &clientResponse)
{
	if (session.sessionData && session.playerData && session.sessionData->AuthGetCurStepNum() == 1)
	{
		string authData = STL_STRING_FROM_OCTET_STRING(clientResponse.clientResponse);
		if (session.sessionData->AuthStep(2, authData))
		{
			string outVerification(session.sessionData->AuthGetNextOutMsg());

			boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
			packet->GetMsg()->present = PokerTHMessage_PR_authMessage;
			AuthMessage_t *netAuth = &packet->GetMsg()->choice.authMessage;
			netAuth->present = AuthMessage_PR_authServerVerification;
			AuthServerVerification_t *verification = &netAuth->choice.authServerVerification;
			OCTET_STRING_fromBuf(
				&verification->serverVerification,
				(char *)outVerification.c_str(),
				outVerification.size());
			GetSender().Send(session.sessionData, packet);
			// The last message is only for server verification.
			// We are done now, the user has logged in.
			InitAfterLogin(session);
		}
		else
			SessionError(session, ERR_NET_INVALID_PASSWORD);
	}
}

void
ServerLobbyThread::HandleNetPacketAvatarHeader(SessionWrapper session, unsigned /*requestId*/, const AvatarHeader_t &avatarHeader)
{
	if (session.playerData)
	{
		if (avatarHeader.avatarSize >= MIN_AVATAR_FILE_SIZE && avatarHeader.avatarSize <= MAX_AVATAR_FILE_SIZE)
		{
			boost::shared_ptr<AvatarFile> tmpAvatarFile(new AvatarFile);
			tmpAvatarFile->fileData.reserve(avatarHeader.avatarSize);
			tmpAvatarFile->fileType = static_cast<AvatarFileType>(avatarHeader.avatarType);
			tmpAvatarFile->reportedSize = avatarHeader.avatarSize;
			// Ignore request id for now.

			session.playerData->SetNetAvatarFile(tmpAvatarFile);

			// Session is now receiving an avatar.
			session.sessionData->SetState(SessionData::ReceivingAvatar);
		}
		else
			SessionError(session, ERR_NET_AVATAR_TOO_LARGE);
	}
}

void
ServerLobbyThread::HandleNetPacketUnknownAvatar(SessionWrapper session, unsigned /*requestId*/, const UnknownAvatar_t &/*unknownAvatar*/)
{
	if (session.playerData.get())
	{
		// Free memory (just in case).
		session.playerData->SetNetAvatarFile(boost::shared_ptr<AvatarFile>());
		session.playerData->SetAvatarMD5(MD5Buf());
		// Start session.
		EstablishSession(session);
	}
}

void
ServerLobbyThread::HandleNetPacketAvatarFile(SessionWrapper session, unsigned /*requestId*/, const AvatarData_t &avatarData)
{
	if (session.playerData.get())
	{
		boost::shared_ptr<AvatarFile> tmpAvatar = session.playerData->GetNetAvatarFile();
		if (tmpAvatar.get() && tmpAvatar->fileData.size() + avatarData.avatarBlock.size <= tmpAvatar->reportedSize)
		{
			std::copy(&avatarData.avatarBlock.buf[0], &avatarData.avatarBlock.buf[avatarData.avatarBlock.size], back_inserter(tmpAvatar->fileData));
		}
	}
}

void
ServerLobbyThread::HandleNetPacketAvatarEnd(SessionWrapper session, unsigned /*requestId*/, const AvatarEnd_t &/*avatarEnd*/)
{
	if (session.playerData.get())
	{
		boost::shared_ptr<AvatarFile> tmpAvatar = session.playerData->GetNetAvatarFile();
		MD5Buf avatarMD5 = session.playerData->GetAvatarMD5();
		if (!avatarMD5.IsZero() && tmpAvatar.get())
		{
			unsigned avatarSize = (unsigned)tmpAvatar->fileData.size();
			if (avatarSize == tmpAvatar->reportedSize)
			{
				if (!GetAvatarManager().StoreAvatarInCache(avatarMD5, tmpAvatar->fileType, &tmpAvatar->fileData[0], avatarSize, true))
				{
					session.playerData->SetAvatarMD5(MD5Buf());
					LOG_ERROR("Failed to store avatar in cache directory.");
				}

				// Free memory.
				session.playerData->SetNetAvatarFile(boost::shared_ptr<AvatarFile>());
				// Set avatar file name.
				string avatarFileName;
				if (GetAvatarManager().GetAvatarFileName(avatarMD5, avatarFileName))
					session.playerData->SetAvatarFile(avatarFileName);
				// Init finished - start session.
				EstablishSession(session);
				LOG_MSG("Client \"" << session.sessionData->GetClientAddr() << "\" uploaded avatar \""
					<< boost::filesystem::path(avatarFileName).file_string() << "\".");
			}
			else
				SessionError(session, ERR_NET_WRONG_AVATAR_SIZE);
		}
	}
}

void
ServerLobbyThread::HandleNetPacketRetrievePlayerInfo(SessionWrapper session, const PlayerInfoRequestMessage_t &playerInfoRequest)
{
	// Find player in lobby or in a game.
	boost::shared_ptr<PlayerData> tmpPlayer = m_sessionManager.GetSessionByUniquePlayerId(playerInfoRequest.playerId).playerData;
	if (!tmpPlayer.get())
		tmpPlayer = m_gameSessionManager.GetSessionByUniquePlayerId(playerInfoRequest.playerId).playerData;
	if (!tmpPlayer.get())
	{
		boost::mutex::scoped_lock lock(m_computerPlayersMutex);
		PlayerDataMap::const_iterator pos = m_computerPlayers.find(playerInfoRequest.playerId);
		if (pos != m_computerPlayers.end())
			tmpPlayer = pos->second;
	}

	boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
	packet->GetMsg()->present = PokerTHMessage_PR_playerInfoReplyMessage;
	PlayerInfoReplyMessage_t *netPlayerInfoReply = &packet->GetMsg()->choice.playerInfoReplyMessage;
	netPlayerInfoReply->playerId = tmpPlayer->GetUniqueId();

	if (tmpPlayer.get())
	{
		// Send player info to client.
		netPlayerInfoReply->playerInfoResult.present = playerInfoResult_PR_playerInfoData;
		PlayerInfoData *data = &netPlayerInfoReply->playerInfoResult.choice.playerInfoData;

		data->isHuman = tmpPlayer->GetType() == PLAYER_TYPE_HUMAN;
		OCTET_STRING_fromBuf(
			&data->playerName,
			tmpPlayer->GetName().c_str(),
			tmpPlayer->GetName().length());
		if (!tmpPlayer->GetAvatarMD5().IsZero())
		{
			data->avatarData = (struct PlayerInfoData::avatarData *)calloc(1, sizeof(struct PlayerInfoData::avatarData));
			data->avatarData->avatarType = static_cast<NetAvatarType_t>(AvatarManager::GetAvatarFileType(tmpPlayer->GetAvatarFile()));
			OCTET_STRING_fromBuf(
				&data->avatarData->avatar,
				(char *)tmpPlayer->GetAvatarMD5().data,
				MD5_DATA_SIZE);
		}
	}
	else
	{
		// Unknown player id - notify client.
		netPlayerInfoReply->playerInfoResult.present = playerInfoResult_PR_unknownPlayerInfo;
	}
	GetSender().Send(session.sessionData, packet);
}

void
ServerLobbyThread::HandleNetPacketRetrieveAvatar(SessionWrapper session, const AvatarRequestMessage_t &retrieveAvatar)
{
	bool avatarFound = false;

	string tmpFile;
	MD5Buf tmpMD5;
	memcpy(tmpMD5.data, retrieveAvatar.avatar.buf, MD5_DATA_SIZE);
	if (GetAvatarManager().GetAvatarFileName(tmpMD5, tmpFile))
	{
		NetPacketList tmpPackets;
		if (GetAvatarManager().AvatarFileToNetPackets(tmpFile, retrieveAvatar.requestId, tmpPackets) == 0)
		{
			avatarFound = true;
			GetSender().Send(session.sessionData, tmpPackets);
		}
		else
			LOG_ERROR("Failed to read avatar file for network transmission.");
	}

	if (!avatarFound)
	{
		// Notify client we didn't find the avatar.
		boost::shared_ptr<NetPacket> unknownAvatar(new NetPacket(NetPacket::Alloc));
		unknownAvatar->GetMsg()->present = PokerTHMessage_PR_avatarReplyMessage;
		AvatarReplyMessage_t *netAvatarReply = &unknownAvatar->GetMsg()->choice.avatarReplyMessage;
		netAvatarReply->requestId = retrieveAvatar.requestId;
		netAvatarReply->avatarResult.present = avatarResult_PR_unknownAvatar;

		GetSender().Send(session.sessionData, unknownAvatar);
	}
}

void
ServerLobbyThread::HandleNetPacketCreateGame(SessionWrapper session, const std::string &password, const JoinNewGame_t &newGame)
{
	LOG_VERBOSE("Creating new game, initiated by session #" << session.sessionData->GetId() << ".");

	// Create a new game.
	GameData tmpData;
	NetPacket::GetGameData(&newGame.gameInfo, tmpData);
	boost::shared_ptr<ServerGame> game(
		new ServerGame(
			shared_from_this(),
			GetNextGameId(),
			STL_STRING_FROM_OCTET_STRING(newGame.gameInfo.gameName),
			password,
			tmpData,
			session.playerData->GetUniqueId(),
			GetGui(),
			m_playerConfig));
	game->Init();

	// Add game to list of games.
	InternalAddGame(game);

	MoveSessionToGame(*game, session);
}

void
ServerLobbyThread::HandleNetPacketJoinGame(SessionWrapper session, const std::string &password, const JoinExistingGame_t &joinGame)
{
	// Join an existing game.
	GameMap::iterator pos = m_gameMap.find(joinGame.gameId);

	if (pos != m_gameMap.end())
	{
		ServerGame &game = *pos->second;
		if (game.CheckPassword(password))
		{
			MoveSessionToGame(game, session);
		}
		else
		{
			SendJoinGameFailed(session.sessionData, NTF_NET_JOIN_INVALID_PASSWORD);
		}
	}
	else
	{
		// TODO do not remove session
		SessionError(session, ERR_NET_UNKNOWN_GAME);
	}
}

void
ServerLobbyThread::HandleNetPacketChatRequest(SessionWrapper session, const ChatRequestMessage_t &chatRequest)
{
	if (chatRequest.chatRequestType.present == chatRequestType_PR_chatRequestTypeLobby && session.playerData)
	{
		boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
		packet->GetMsg()->present = PokerTHMessage_PR_chatMessage;
		ChatMessage_t *netChat = &packet->GetMsg()->choice.chatMessage;
		netChat->chatType.present = chatType_PR_chatTypeLobby;
		ChatTypeLobby_t *netLobbyChat = &netChat->chatType.choice.chatTypeLobby;
		netLobbyChat->playerId = session.playerData->GetUniqueId();
		OCTET_STRING_fromBuf(
			&netChat->chatText,
			(char *)chatRequest.chatText.buf,
			chatRequest.chatText.size);

		m_sessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Established);
		m_gameSessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Game);

		string chatMsg = STL_STRING_FROM_OCTET_STRING(chatRequest.chatText);
		// Send the message to the chat cleaner bot.
		m_chatCleanerManager->HandleChatText(
			session.playerData->GetUniqueId(),
			session.playerData->GetName(),
			chatMsg);
		// Send the message to the irc bot.
		GetIrcBotCallback().SignalLobbyMessage(
			session.playerData->GetUniqueId(),
			session.playerData->GetName(), 
			chatMsg);
	}
}

void
ServerLobbyThread::AuthChallenge(SessionWrapper session, const string &secret)
{
	if (session.sessionData && session.playerData && session.sessionData->AuthGetCurStepNum() == 1)
	{
		session.playerData->SetPassword(secret); // For later encryption of data.
		session.sessionData->AuthSetPassword(secret); // For this auth session.
		string outChallenge(session.sessionData->AuthGetNextOutMsg());

		boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
		packet->GetMsg()->present = PokerTHMessage_PR_authMessage;
		AuthMessage_t *netAuth = &packet->GetMsg()->choice.authMessage;
		netAuth->present = AuthMessage_PR_authServerChallenge;
		AuthServerChallenge_t *challenge = &netAuth->choice.authServerChallenge;
		OCTET_STRING_fromBuf(
			&challenge->serverChallenge,
			(char *)outChallenge.c_str(),
			outChallenge.size());
		GetSender().Send(session.sessionData, packet);
	}
}

void
ServerLobbyThread::InitAfterLogin(SessionWrapper session)
{
	if (session.sessionData && session.playerData)
	{
		const MD5Buf &avatarMD5 = session.playerData->GetAvatarMD5();
		string avatarFileName;
		if (!avatarMD5.IsZero()
			&& !GetAvatarManager().GetAvatarFileName(avatarMD5, avatarFileName))
		{
			RequestPlayerAvatar(session);
		}
		else
		{
			if (!avatarFileName.empty())
				session.playerData->SetAvatarFile(avatarFileName);
			EstablishSession(session);
		}
	}
}

void
ServerLobbyThread::EstablishSession(SessionWrapper session)
{
	if (!session.playerData.get())
		throw ServerException(__FILE__, __LINE__, ERR_NET_INVALID_SESSION, 0);
	// Send ACK to client.
	boost::shared_ptr<NetPacket> ack(new NetPacket(NetPacket::Alloc));
	ack->GetMsg()->present = PokerTHMessage_PR_initAckMessage;
	InitAckMessage_t *netInitAck = &ack->GetMsg()->choice.initAckMessage;
//	initAckData.sessionId = session.sessionData->GetId(); // TODO: currently unused.
	netInitAck->yourPlayerId = session.playerData->GetUniqueId();
	GetSender().Send(session.sessionData, ack);

	// Send the connected players list to the client.
	SendPlayerList(session.sessionData);
	// Send the game list to the client.
	SendGameList(session.sessionData);

	// Session is now established.
	session.sessionData->SetState(SessionData::Established);

	{
		boost::mutex::scoped_lock lock(m_statMutex);
		++m_statData.totalPlayersEverLoggedIn;
		m_statDataChanged = true;
	}
	// Notify all players.
	NotifyPlayerJoinedLobby(session.playerData->GetUniqueId());

	UpdateStatisticsNumberOfPlayers();
}

void
ServerLobbyThread::AuthenticatePlayer(SessionWrapper session)
{
	if(session.playerData)
		m_database->AsyncPlayerLogin(session.playerData->GetUniqueId(), session.playerData->GetName());
}

void
ServerLobbyThread::UserValid(unsigned playerId, DB_id dbPlayerId, const string &dbSecret)
{
	SessionWrapper tmpSession = m_sessionManager.GetSessionByUniquePlayerId(playerId, true);
	tmpSession.playerData->SetDBId(dbPlayerId);
	this->AuthChallenge(tmpSession, dbSecret);
}

void
ServerLobbyThread::UserInvalid(unsigned playerId)
{
	SessionError(m_sessionManager.GetSessionByUniquePlayerId(playerId, true), ERR_NET_INVALID_PASSWORD);
}

void
ServerLobbyThread::RequestPlayerAvatar(SessionWrapper session)
{
	if (!session.playerData.get())
		throw ServerException(__FILE__, __LINE__, ERR_NET_INVALID_SESSION, 0);
	// Ask the client to send its avatar.
	boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
	packet->GetMsg()->present = PokerTHMessage_PR_avatarRequestMessage;
	AvatarRequestMessage_t *netAvatarRequest = &packet->GetMsg()->choice.avatarRequestMessage;
	netAvatarRequest->requestId = session.playerData->GetUniqueId();
	OCTET_STRING_fromBuf(
		&netAvatarRequest->avatar,
		(char *)session.playerData->GetAvatarMD5().data,
		MD5_DATA_SIZE);
	GetSender().Send(session.sessionData, packet);
}

void
ServerLobbyThread::TimerRemoveGame(const boost::system::error_code &ec)
{
	if (!ec)
	{
		// Synchronously remove games which have been closed.
		GameMap::iterator i = m_gameMap.begin();
		GameMap::iterator end = m_gameMap.end();
		while (i != end)
		{
			GameMap::iterator next = i;
			++next;
			boost::shared_ptr<ServerGame> tmpGame = i->second;
			if (!tmpGame->GetSessionManager().HasSessions())
				InternalRemoveGame(tmpGame); // This will delete the game.
			i = next;
		}
		// Restart timer
		m_removeGameTimer.expires_from_now(
			boost::posix_time::milliseconds(SERVER_REMOVE_GAME_INTERVAL_MSEC));
		m_removeGameTimer.async_wait(
			boost::bind(
				&ServerLobbyThread::TimerRemoveGame, shared_from_this(), boost::asio::placeholders::error));
	}
}

void
ServerLobbyThread::TimerRemovePlayer(const boost::system::error_code &ec)
{
	if (!ec)
	{
		boost::mutex::scoped_lock lock(m_removePlayerListMutex);

		if (!m_removePlayerList.empty())
		{
			RemovePlayerList::iterator i = m_removePlayerList.begin();
			RemovePlayerList::iterator end = m_removePlayerList.end();

			while (i != end)
			{
				InternalRemovePlayer(i->first, i->second);
				++i;
			}
			m_removePlayerList.clear();
		}
		// Restart timer
		m_removePlayerTimer.expires_from_now(
			boost::posix_time::milliseconds(SERVER_REMOVE_PLAYER_INTERVAL_MSEC));
		m_removePlayerTimer.async_wait(
			boost::bind(
				&ServerLobbyThread::TimerRemovePlayer, shared_from_this(), boost::asio::placeholders::error));
	}
}

void
ServerLobbyThread::TimerUpdateClientLoginLock(const boost::system::error_code &ec)
{
	if (!ec)
	{
		boost::mutex::scoped_lock lock(m_timerClientAddressMapMutex);

		TimerClientAddressMap::iterator i = m_timerClientAddressMap.begin();
		TimerClientAddressMap::iterator end = m_timerClientAddressMap.end();

		while (i != end)
		{
			TimerClientAddressMap::iterator next = i;
			++next;
			if (i->second.elapsed().total_seconds() > SERVER_INIT_LOGIN_CLIENT_LOCK_SEC)
				m_timerClientAddressMap.erase(i);
			i = next;
		}
		// Restart timer
		m_loginLockTimer.expires_from_now(
			boost::posix_time::milliseconds(SERVER_UPDATE_LOGIN_LOCK_INTERVAL_MSEC));
		m_loginLockTimer.async_wait(
			boost::bind(
				&ServerLobbyThread::TimerUpdateClientLoginLock, shared_from_this(), boost::asio::placeholders::error));
	}
}

void
ServerLobbyThread::TimerCheckSessionTimeouts(const boost::system::error_code &ec)
{
	if (!ec)
	{
		m_sessionManager.ForEach(boost::bind(&ServerLobbyThread::InternalCheckSessionTimeouts, boost::ref(*this), _1));
		m_gameSessionManager.ForEach(boost::bind(&ServerLobbyThread::InternalCheckSessionTimeouts, boost::ref(*this), _1));
		// Restart timer
		m_sessionTimeoutTimer.expires_from_now(
			boost::posix_time::milliseconds(SERVER_CHECK_SESSION_TIMEOUTS_INTERVAL_MSEC));
		m_sessionTimeoutTimer.async_wait(
			boost::bind(
				&ServerLobbyThread::TimerCheckSessionTimeouts, shared_from_this(), boost::asio::placeholders::error));
	}
}

void
ServerLobbyThread::TimerCleanupAvatarCache(const boost::system::error_code &ec)
{
	if (!ec)
	{
		// Only act if there are no sessions.
		if (!m_sessionManager.HasSessions() && !m_gameSessionManager.HasSessions())
		{
			LOG_VERBOSE("Cleaning up avatar cache.");

			m_avatarManager.RemoveOldAvatarCacheEntries();
		}
		// Restart timer
		m_avatarCleanupTimer.expires_from_now(
			boost::posix_time::seconds(SERVER_CACHE_CLEANUP_INTERVAL_SEC));
		m_avatarCleanupTimer.async_wait(
			boost::bind(
				&ServerLobbyThread::TimerCleanupAvatarCache, shared_from_this(), boost::asio::placeholders::error));
	}
}

boost::shared_ptr<ServerGame>
ServerLobbyThread::InternalGetGameFromId(unsigned gameId)
{
	boost::shared_ptr<ServerGame> game;
	if (gameId)
	{
		GameMap::iterator pos = m_gameMap.find(gameId);

		if (pos != m_gameMap.end())
			game = pos->second;
	}
	return game;
}

void
ServerLobbyThread::InternalAddGame(boost::shared_ptr<ServerGame> game)
{
	// Add game to list.
	m_gameMap.insert(GameMap::value_type(game->GetId(), game));
	// Notify all players.
	m_sessionManager.SendLobbyMsgToAllSessions(GetSender(), CreateNetPacketGameListNew(*game), SessionData::Established);
	m_gameSessionManager.SendLobbyMsgToAllSessions(GetSender(), CreateNetPacketGameListNew(*game), SessionData::Game);

	{
		boost::mutex::scoped_lock lock(m_statMutex);
		++m_statData.totalGamesEverCreated;
		++m_statData.numberOfGamesOpen;
		unsigned numGames = static_cast<unsigned>(m_gameMap.size());
		if (numGames > m_statData.maxGamesOpen)
			m_statData.maxGamesOpen = numGames;
		m_statDataChanged = true;
	}
}

void
ServerLobbyThread::InternalRemoveGame(boost::shared_ptr<ServerGame> game)
{
	{
		boost::mutex::scoped_lock lock(m_statMutex);
		if (m_statData.numberOfGamesOpen)
		{
			--m_statData.numberOfGamesOpen;
			m_statDataChanged = true;
		}
	}
	// Remove game from list.
	m_gameMap.erase(game->GetId());
	// Remove all sessions left in the game.
	game->ResetComputerPlayerList();
	game->RemoveAllSessions();
	game->Exit();
	// Notify all players.
	boost::shared_ptr<NetPacket> packet = CreateNetPacketGameListUpdate(game->GetId(), GAME_MODE_CLOSED);
	m_sessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Established);
	m_gameSessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Game);
}

void
ServerLobbyThread::InternalRemovePlayer(unsigned playerId, unsigned errorCode)
{
	SessionWrapper session = m_sessionManager.GetSessionByUniquePlayerId(playerId, true);
	if (session.sessionData.get())
		SessionError(session, errorCode);
	else
	{
		// Scan games for the player.
		GameMap::iterator i = m_gameMap.begin();
		GameMap::iterator end = m_gameMap.end();

		while (i != end)
		{
			boost::shared_ptr<ServerGame> tmpGame = i->second;
			if (tmpGame->GetPlayerDataByUniqueId(playerId).get())
			{
				tmpGame->RemovePlayer(playerId, errorCode);
				break;
			}
			++i;
		}
	}
}

void
ServerLobbyThread::InternalResubscribeMsg(SessionWrapper session)
{
	if (!session.sessionData->WantsLobbyMsg())
	{
		session.sessionData->SetWantsLobbyMsg();
		SendPlayerList(session.sessionData);
		SendGameList(session.sessionData);
		// Send new statistics information.
/*		boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
		packet->GetMsg()->present = PokerTHMessage_PR_statisticsMessage;
		StatisticsMessage_t *netStatistics = &packet->GetMsg()->choice.statisticsMessage;

		StatisticsData_t *data = (StatisticsData_t *)calloc(1, sizeof(struct StatisticsData));
		data->statisticsType = statisticsType_statNumberOfPlayers;
		data->statisticsValue = m_sessionManager.GetRawSessionCount() + m_gameSessionManager.GetRawSessionCount();
		ASN_SEQUENCE_ADD(&netStatistics->statisticsData.list, data);

		GetSender().Send(session.sessionData, packet);*/
	}
}

void
ServerLobbyThread::HandleReAddedSession(SessionWrapper session)
{
	// Remove session from game session list.
	m_gameSessionManager.RemoveSession(session.sessionData->GetId());

	if (m_sessionManager.GetRawSessionCount() <= SERVER_MAX_NUM_SESSIONS)
	{
		// Set state (back) to established.
		session.sessionData->SetState(SessionData::Established);
		session.sessionData->SetGameId(0);
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
ServerLobbyThread::InternalCheckSessionTimeouts(SessionWrapper session)
{
	bool closeSession = false;
	if (session.sessionData.get())
	{
		if (session.sessionData->GetState() == SessionData::Init && session.sessionData->GetAutoDisconnectTimerElapsedSec() >= SERVER_INIT_SESSION_TIMEOUT_SEC)
		{
			LOG_VERBOSE("Session init timeout, removing session #" << session.sessionData->GetId() << ".");
			closeSession = true;
		}
		else if (session.sessionData->GetActivityTimerElapsedSec() >= SERVER_SESSION_ACTIVITY_TIMEOUT_SEC - SERVER_TIMEOUT_WARNING_REMAINING_SEC
				&& !session.sessionData->HasActivityNoticeBeenSent())
		{
			session.sessionData->MarkActivityNotice();

			boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
			packet->GetMsg()->present = PokerTHMessage_PR_timeoutWarningMessage;
			TimeoutWarningMessage_t *netWarning = &packet->GetMsg()->choice.timeoutWarningMessage;
			netWarning->timeoutReason = timeoutReason_timeoutNoDataReceived;
			netWarning->remainingSeconds = SERVER_TIMEOUT_WARNING_REMAINING_SEC;
			GetSender().Send(session.sessionData, packet);
		}
		else if (session.sessionData->GetActivityTimerElapsedSec() >= SERVER_SESSION_ACTIVITY_TIMEOUT_SEC)
		{
			LOG_VERBOSE("Activity timeout, removing session #" << session.sessionData->GetId() << ".");
			closeSession = true;
		}
		else if (session.sessionData->GetAutoDisconnectTimerElapsedSec() >= SERVER_SESSION_FORCED_TIMEOUT_SEC)
		{
			LOG_VERBOSE("Auto disconnect timeout, removing session #" << session.sessionData->GetId() << ".");
			closeSession = true;
		}
	}
	if (closeSession)
	{
		if (session.playerData.get())
			RemovePlayer(session.playerData->GetUniqueId(), ERR_NET_SESSION_TIMED_OUT);
		else
			m_sessionManager.RemoveSession(session.sessionData->GetId());
	}
}

void
ServerLobbyThread::SessionError(SessionWrapper session, int errorCode)
{
	if (session.sessionData)
	{
		SendError(session.sessionData, errorCode);
		CloseSession(session);
	}
}

void
ServerLobbyThread::SendError(boost::shared_ptr<SessionData> s, int errorCode)
{
	LOG_VERBOSE("Sending error code " << errorCode << " to session #" << s->GetId() << ".");
	boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
	packet->GetMsg()->present = PokerTHMessage_PR_errorMessage;
	ErrorMessage_t *netError = &packet->GetMsg()->choice.errorMessage;
	netError->errorReason = NetPacket::GameErrorToNetError(errorCode);
	GetSender().Send(s, packet);
}

void
ServerLobbyThread::SendJoinGameFailed(boost::shared_ptr<SessionData> s, int reason)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
	packet->GetMsg()->present = PokerTHMessage_PR_joinGameReplyMessage;
	JoinGameReplyMessage_t *netJoinReply = &packet->GetMsg()->choice.joinGameReplyMessage;
	netJoinReply->gameId = s->GetGameId();

	netJoinReply->joinGameResult.present = joinGameResult_PR_joinGameFailed;
	JoinGameFailed_t *joinFailed = &netJoinReply->joinGameResult.choice.joinGameFailed;

	switch (reason)
	{
		case NTF_NET_JOIN_GAME_FULL :
			joinFailed->joinGameFailureReason = joinGameFailureReason_gameIsFull;
			break;
		case NTF_NET_JOIN_ALREADY_RUNNING :
			joinFailed->joinGameFailureReason = joinGameFailureReason_gameIsRunning;
			break;
		case NTF_NET_JOIN_INVALID_PASSWORD :
			joinFailed->joinGameFailureReason = joinGameFailureReason_invalidPassword;
			break;
		default :
			joinFailed->joinGameFailureReason = joinGameFailureReason_invalidGame;
			break;
	}
	GetSender().Send(s, packet);
}

void
ServerLobbyThread::SendPlayerList(boost::shared_ptr<SessionData> s)
{
	// Retrieve all player ids.
	PlayerIdList idList(m_sessionManager.GetPlayerIdList(SessionData::Established));
	PlayerIdList gameIdList(m_gameSessionManager.GetPlayerIdList(SessionData::Game));
	idList.splice(idList.begin(), gameIdList);
	// Send all player ids to client.
	PlayerIdList::const_iterator i = idList.begin();
	PlayerIdList::const_iterator end = idList.end();
	while (i != end)
	{
		GetSender().Send(s, CreateNetPacketPlayerListNew(*i));
		++i;
	}
}

void
ServerLobbyThread::SendGameList(boost::shared_ptr<SessionData> s)
{
	GameMap::const_iterator game_i = m_gameMap.begin();
	GameMap::const_iterator game_end = m_gameMap.end();
	while (game_i != game_end)
	{
		GetSender().Send(s, CreateNetPacketGameListNew(*game_i->second));
		++game_i;
	}
}

void
ServerLobbyThread::UpdateStatisticsNumberOfPlayers()
{
	ServerStats stats;
	unsigned curNumberOfPlayersOnServer = m_sessionManager.GetRawSessionCount() + m_gameSessionManager.GetRawSessionCount();
	{
		boost::mutex::scoped_lock lock(m_statMutex);
		if (curNumberOfPlayersOnServer != m_statData.numberOfPlayersOnServer)
		{
			m_statData.numberOfPlayersOnServer = stats.numberOfPlayersOnServer = curNumberOfPlayersOnServer;
			if (curNumberOfPlayersOnServer > m_statData.maxPlayersLoggedIn)
				m_statData.maxPlayersLoggedIn = curNumberOfPlayersOnServer;
			m_statDataChanged = true;
		}
	}
	// Do not send other stats than number of players for now.
	//BroadcastStatisticsUpdate(stats);
}

void
ServerLobbyThread::BroadcastStatisticsUpdate(const ServerStats &stats)
{
	if (stats.numberOfPlayersOnServer)
	{
		boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
		packet->GetMsg()->present = PokerTHMessage_PR_statisticsMessage;
		StatisticsMessage_t *netStatistics = &packet->GetMsg()->choice.statisticsMessage;

		StatisticsData_t *data = (StatisticsData_t *)calloc(1, sizeof(struct StatisticsData));
		data->statisticsType = statisticsType_statNumberOfPlayers;
		data->statisticsValue = m_sessionManager.GetRawSessionCount() + m_gameSessionManager.GetRawSessionCount();
		ASN_SEQUENCE_ADD(&netStatistics->statisticsData.list, data);

		m_sessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Established);
		m_gameSessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Game);
	}
}

void
ServerLobbyThread::ReadStatisticsFile()
{
	ifstream i(m_statisticsFileName.c_str(), ios_base::in);

	if (!i.fail() && !i.eof())
	{
		boost::mutex::scoped_lock lock(m_statMutex);
		do
		{
			string statisticsType;
			unsigned statisticsValue;
			i >> statisticsType;
			i >> statisticsValue;
			if (statisticsType == SERVER_STATISTICS_STR_TOTAL_PLAYERS)
				m_statData.totalPlayersEverLoggedIn = statisticsValue;
			else if (statisticsType == SERVER_STATISTICS_STR_TOTAL_GAMES)
				m_statData.totalGamesEverCreated = statisticsValue;
			else if (statisticsType == SERVER_STATISTICS_STR_MAX_PLAYERS)
				m_statData.maxPlayersLoggedIn = statisticsValue;
			else if (statisticsType == SERVER_STATISTICS_STR_MAX_GAMES)
				m_statData.maxGamesOpen = statisticsValue;
			// other statistics are non-persistant and not read.
		} while (!i.fail() && !i.eof());
		m_statDataChanged = false;
	}
}

void
ServerLobbyThread::TimerSaveStatisticsFile(const boost::system::error_code &ec)
{
	if (!ec)
	{
		LOG_VERBOSE("Saving statistics.");
		boost::mutex::scoped_lock lock(m_statMutex);
		if (m_statDataChanged)
		{
			ofstream o(m_statisticsFileName.c_str(), ios_base::out | ios_base::trunc);
			if (!o.fail())
			{
				o << SERVER_STATISTICS_STR_TOTAL_PLAYERS " " << m_statData.totalPlayersEverLoggedIn << endl;
				o << SERVER_STATISTICS_STR_TOTAL_GAMES " " << m_statData.totalGamesEverCreated << endl;
				o << SERVER_STATISTICS_STR_MAX_PLAYERS " " << m_statData.maxPlayersLoggedIn << endl;
				o << SERVER_STATISTICS_STR_MAX_GAMES " " << m_statData.maxGamesOpen << endl;
				o << SERVER_STATISTICS_STR_CUR_PLAYERS " " << m_statData.numberOfPlayersOnServer << endl;
				o << SERVER_STATISTICS_STR_CUR_GAMES " " << m_statData.numberOfGamesOpen << endl;
				m_statDataChanged = false;
			}
		}
		// Restart timer
		m_saveStatisticsTimer.expires_from_now(
			boost::posix_time::seconds(SERVER_SAVE_STATISTICS_INTERVAL_SEC));
		m_saveStatisticsTimer.async_wait(
			boost::bind(
				&ServerLobbyThread::TimerSaveStatisticsFile, shared_from_this(), boost::asio::placeholders::error));
	}
}

ServerCallback &
ServerLobbyThread::GetCallback()
{
	return m_gui;
}

void
ServerLobbyThread::SetGameDBId(u_int32_t gameId, DB_id gameDBId)
{
	boost::shared_ptr<ServerGame> game = InternalGetGameFromId(gameId);
	if (game)
		game->SetDBId(gameDBId);
}

ServerIrcBotCallback &
ServerLobbyThread::GetIrcBotCallback()
{
	return m_ircBotCb;
}

ReceiverHelper &
ServerLobbyThread::GetReceiver()
{
	assert(m_receiver.get());
	return *m_receiver;
}

InternalServerCallback &
ServerLobbyThread::GetSenderCallback()
{
	assert(m_internalServerCallback.get());
	return *m_internalServerCallback;
}

GuiInterface &
ServerLobbyThread::GetGui()
{
	return m_gui;
}

bool
ServerLobbyThread::IsPlayerConnected(const string &name) const
{
	bool retVal = false;

	retVal = m_sessionManager.IsPlayerConnected(name);

	if (!retVal)
		retVal = m_gameSessionManager.IsPlayerConnected(name);

	return retVal;
}

boost::shared_ptr<NetPacket>
ServerLobbyThread::CreateNetPacketPlayerListNew(unsigned playerId)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
	packet->GetMsg()->present = PokerTHMessage_PR_playerListMessage;
	PlayerListMessage_t *netPlayerList = &packet->GetMsg()->choice.playerListMessage;
	netPlayerList->playerId = playerId;
	netPlayerList->playerListNotification = playerListNotification_playerListNew;
	return packet;
}

boost::shared_ptr<NetPacket>
ServerLobbyThread::CreateNetPacketPlayerListLeft(unsigned playerId)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
	packet->GetMsg()->present = PokerTHMessage_PR_playerListMessage;
	PlayerListMessage_t *netPlayerList = &packet->GetMsg()->choice.playerListMessage;
	netPlayerList->playerId = playerId;
	netPlayerList->playerListNotification = playerListNotification_playerListLeft;
	return packet;
}

boost::shared_ptr<NetPacket>
ServerLobbyThread::CreateNetPacketGameListNew(const ServerGame &game)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
	packet->GetMsg()->present = PokerTHMessage_PR_gameListMessage;
	GameListMessage_t *netGameList = &packet->GetMsg()->choice.gameListMessage;
	netGameList->gameId = game.GetId();

	netGameList->gameListNotification.present = gameListNotification_PR_gameListNew;
	GameListNew_t *gameNew = &netGameList->gameListNotification.choice.gameListNew;
	gameNew->adminPlayerId = game.GetAdminPlayerId();
	gameNew->gameMode = game.IsRunning() ? NetGameMode_gameStarted : NetGameMode_gameCreated;
	NetPacket::SetGameData(game.GetGameData(), &gameNew->gameInfo);
	OCTET_STRING_fromBuf(
		&gameNew->gameInfo.gameName,
		game.GetName().c_str(),
		game.GetName().length());
	gameNew->isPrivate = game.IsPasswordProtected();

	PlayerIdList tmpList = game.GetPlayerIdList();
	PlayerIdList::const_iterator i = tmpList.begin();
	PlayerIdList::const_iterator end = tmpList.end();
	while (i != end)
	{
		NonZeroId_t *playerId = (NonZeroId_t *)calloc(1, sizeof(NonZeroId_t));
		*playerId = *i;
		ASN_SEQUENCE_ADD(&gameNew->playerIds.list, playerId);
		++i;
	}

	return packet;
}

boost::shared_ptr<NetPacket>
ServerLobbyThread::CreateNetPacketGameListUpdate(unsigned gameId, GameMode mode)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
	packet->GetMsg()->present = PokerTHMessage_PR_gameListMessage;
	GameListMessage_t *netGameList = &packet->GetMsg()->choice.gameListMessage;
	netGameList->gameId = gameId;

	netGameList->gameListNotification.present = gameListNotification_PR_gameListUpdate;
	GameListUpdate_t *gameUpdate = &netGameList->gameListNotification.choice.gameListUpdate;

	gameUpdate->gameMode = mode;
	return packet;
}

