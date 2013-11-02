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

#include <net/serverlobbythread.h>
#include <net/servergame.h>
#include <net/serverbanmanager.h>
#include <net/serverexception.h>
#include <net/receivebuffer.h>
#include <net/senderhelper.h>
#include <net/serverircbotcallback.h>
#include <net/socket_msg.h>
#include <net/chatcleanermanager.h>
#include <net/net_helper.h>
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
#include <playerinterface.h>

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <boost/lambda/lambda.hpp>
#include <boost/filesystem.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/algorithm/string.hpp>
#include <gsasl.h>

#define SERVER_MAX_NUM_LOBBY_SESSIONS				512		// Maximum number of idle users in lobby.
#define SERVER_MAX_NUM_TOTAL_SESSIONS				2000	// Total maximum of sessions, fitting a 2048 handle limit

#define SERVER_SAVE_STATISTICS_INTERVAL_SEC			60
#define SERVER_CHECK_SESSION_TIMEOUTS_INTERVAL_MSEC	500
#define SERVER_REMOVE_GAME_INTERVAL_MSEC			500
#define SERVER_REMOVE_PLAYER_INTERVAL_MSEC			100
#define SERVER_UPDATE_LOGIN_LOCK_INTERVAL_MSEC		1000
#define SERVER_PROCESS_SEND_INTERVAL_MSEC			10

#define SERVER_INIT_LOGIN_CLIENT_LOCK_SEC			NetHelper::GetLoginLockSec()

#define SERVER_INIT_SESSION_TIMEOUT_SEC				60
#define SERVER_TIMEOUT_WARNING_REMAINING_SEC		60
#define SERVER_SESSION_ACTIVITY_TIMEOUT_SEC			1800	// 30 min, MUST be > SERVER_TIMEOUT_WARNING_REMAINING_SEC
#define SERVER_SESSION_FORCED_TIMEOUT_SEC			86400	// 1 day, should be quite large.

#define SERVER_ADDRESS_LOCALHOST_STR_V4				"127.0.0.1"
#define SERVER_ADDRESS_LOCALHOST_STR_V4V6			"::ffff:127.0.0.1"
#define SERVER_ADDRESS_LOCALHOST_STR				"::1"

#define SERVER_STATISTICS_FILE_NAME					"server_statistics.log"
#define SERVER_STATISTICS_STR_TOTAL_PLAYERS			"TotalNumPlayersLoggedIn"
#define SERVER_STATISTICS_STR_TOTAL_GAMES			"TotalNumGamesCreated"
#define SERVER_STATISTICS_STR_MAX_GAMES				"MaxGamesOpen"
#define SERVER_STATISTICS_STR_MAX_PLAYERS			"MaxPlayersLoggedIn"
#define SERVER_STATISTICS_STR_CUR_GAMES				"CurGamesOpen"
#define SERVER_STATISTICS_STR_CUR_PLAYERS			"CurPlayersLoggedIn"

using namespace std;
using boost::asio::ip::tcp;


class InternalServerCallback : public SessionDataCallback, public ChatCleanerCallback, public ServerDBCallback
{
public:
	InternalServerCallback(ServerLobbyThread &server) : m_server(server) {}
	virtual ~InternalServerCallback() {}

	virtual void CloseSession(boost::shared_ptr<SessionData> session) {
		m_server.CloseSession(session);
	}

	virtual void SessionError(boost::shared_ptr<SessionData> session, int errorCode) {
		m_server.SessionError(session, errorCode);
	}

	virtual void SessionTimeoutWarning(boost::shared_ptr<SessionData> session, unsigned remainingSec) {
		m_server.SessionTimeoutWarning(session, remainingSec);
	}

	virtual void HandlePacket(boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet) {
		m_server.DispatchPacket(session, packet);
	}

	virtual void SignalChatBotMessage(const string &msg) {
		m_server.SendChatBotMsg(msg);
	}

	virtual void SignalChatBotMessage(unsigned gameId, const std::string &msg) {
		m_server.SendChatBotMsg(gameId, msg);
	}

	virtual void SignalKickPlayer(unsigned playerId) {
		m_server.RemovePlayer(playerId, ERR_NET_PLAYER_KICKED);
	}

	virtual void SignalBanPlayer(unsigned playerId) {
		string playerName(m_server.GetPlayerNameFromId(playerId));
		if (!playerName.empty()) {
			m_server.GetBanManager().BanPlayerName(playerName, 1);
			m_server.RemovePlayer(playerId, ERR_NET_PLAYER_KICKED);
		}
	}

	virtual void SignalMutePlayer(unsigned playerId) {
		m_server.MutePlayerInGame(playerId);
	}

	virtual void ConnectSuccess() {
		LOG_MSG("Successfully connected to database.");
	}

	virtual void ConnectFailed(string error) {
		LOG_ERROR("DB connect error: " << error);
	}

	virtual void QueryError(string error) {
		LOG_ERROR("DB query error: " << error);
	}

	virtual void PlayerLoginSuccess(unsigned requestId, boost::shared_ptr<DBPlayerData> dbPlayerData) {
		m_server.UserValid(requestId, *dbPlayerData);
	}

	virtual void PlayerLoginFailed(unsigned requestId) {
		m_server.UserInvalid(requestId);
	}

	virtual void PlayerLoginBlocked(unsigned requestId) {
		m_server.UserBlocked(requestId);
	}

	virtual void AvatarIsBlacklisted(unsigned requestId) {
		m_server.AvatarBlacklisted(requestId);
	}

	virtual void AvatarIsOK(unsigned requestId) {
		m_server.AvatarOK(requestId);
	}

	virtual void CreateGameSuccess(unsigned /*requestId*/) {
		// Nothing to do.
	}

	virtual void CreateGameFailed(unsigned requestId) {
		// TODO maybe handle request id.
		LOG_ERROR("DB create game failed for request " << requestId);
	}

	virtual void ReportAvatarSuccess(unsigned requestId, unsigned replyId) {
		m_server.SendReportAvatarResult(requestId, replyId, true);
	}

	virtual void ReportAvatarFailed(unsigned requestId, unsigned replyId) {
		m_server.SendReportAvatarResult(requestId, replyId, false);
	}

	virtual void ReportGameSuccess(unsigned requestId, unsigned replyId) {
		m_server.SendReportGameResult(requestId, replyId, true);
	}

	virtual void ReportGameFailed(unsigned requestId, unsigned replyId) {
		m_server.SendReportGameResult(requestId, replyId, false);
	}

	virtual void PlayerAdminList(unsigned /*requestId*/, std::list<DB_id> adminList) {
		m_server.GetBanManager().SetAdminPlayerIds(adminList);
	}

	virtual void BlockPlayerSuccess(unsigned requestId, unsigned replyId) {
		m_server.SendAdminBanPlayerResult(requestId, replyId, true);
	}

	virtual void BlockPlayerFailed(unsigned requestId, unsigned replyId) {
		m_server.SendAdminBanPlayerResult(requestId, replyId, false);
	}

private:
	ServerLobbyThread &m_server;
};


ServerLobbyThread::ServerLobbyThread(GuiInterface &gui, ServerMode mode, ServerIrcBotCallback &ircBotCb, ConfigFile &serverConfig,
									 AvatarManager &avatarManager, boost::shared_ptr<boost::asio::io_service> ioService)
	: m_ioService(ioService), m_authContext(NULL), m_gui(gui), m_ircBotCb(ircBotCb), m_avatarManager(avatarManager),
	  m_mode(mode), m_serverConfig(serverConfig), m_curGameId(0), m_curUniquePlayerId(0), m_curSessionId(INVALID_SESSION + 1),
	  m_statDataChanged(false), m_removeGameTimer(*ioService),
	  m_saveStatisticsTimer(*ioService), m_loginLockTimer(*ioService),
	  m_startTime(boost::posix_time::second_clock::local_time())
{
	m_internalServerCallback.reset(new InternalServerCallback(*this));
	m_sender.reset(new SenderHelper(m_ioService));
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
	if (!logDir.empty()) {
		boost::filesystem::path logPath(logDir);
		if (!logDir.empty()) {
			logPath /= SERVER_STATISTICS_FILE_NAME;
			m_statisticsFileName = logPath.directory_string();
			ReadStatisticsFile();
		}
	}
	m_database->Init(
		m_serverConfig.readConfigString("DBServerAddress"),
		m_serverConfig.readConfigString("DBServerUser"),
		m_serverConfig.readConfigString("DBServerPassword"),
		m_serverConfig.readConfigString("DBServerDatabaseName"),
		m_serverConfig.readConfigString("DBServerEncryptionKey"));
	m_database->AsyncQueryAdminPlayers(0);

	GetBanManager().InitGameNameBadWordList(m_serverConfig.readConfigStringList("GameNameBadWordList"));
}

void
ServerLobbyThread::SignalTermination()
{
	Thread::SignalTermination();
	m_ioService->stop();
}

void
ServerLobbyThread::AddConnection(boost::shared_ptr<SessionData> sessionData)
{
	// Create a new session.
	m_sessionManager.AddSession(sessionData);

	LOG_VERBOSE("Accepted connection - session #" << sessionData->GetId() << ".");

	sessionData->StartTimerInitTimeout(SERVER_INIT_SESSION_TIMEOUT_SEC);
	sessionData->StartTimerGlobalTimeout(SERVER_SESSION_FORCED_TIMEOUT_SEC);
	sessionData->StartTimerActivityTimeout(SERVER_SESSION_ACTIVITY_TIMEOUT_SEC, SERVER_TIMEOUT_WARNING_REMAINING_SEC);

	unsigned numLobbySessions = m_sessionManager.GetRawSessionCount();
	unsigned numGameSessions = m_gameSessionManager.GetRawSessionCount();
	if (numLobbySessions <= SERVER_MAX_NUM_LOBBY_SESSIONS
			&& numLobbySessions + numGameSessions <= SERVER_MAX_NUM_TOTAL_SESSIONS) {
		string ipAddress = sessionData->GetRemoteIPAddressFromSocket();
		if (!ipAddress.empty()) {
			sessionData->SetClientAddr(ipAddress);

			boost::shared_ptr<NetPacket> packet(new NetPacket);
			packet->GetMsg()->set_messagetype(PokerTHMessage::Type_AnnounceMessage);
			AnnounceMessage *netAnnounce = packet->GetMsg()->mutable_announcemessage();
			netAnnounce->mutable_protocolversion()->set_majorversion(NET_VERSION_MAJOR);
			netAnnounce->mutable_protocolversion()->set_minorversion(NET_VERSION_MINOR);
			netAnnounce->mutable_latestgameversion()->set_majorversion(POKERTH_VERSION_MAJOR);
			netAnnounce->mutable_latestgameversion()->set_minorversion(POKERTH_VERSION_MINOR);
			netAnnounce->set_latestbetarevision(POKERTH_BETA_REVISION);
			switch (GetServerMode()) {
			case SERVER_MODE_LAN:
				netAnnounce->set_servertype(AnnounceMessage::serverTypeLAN);
				break;
			case SERVER_MODE_INTERNET_NOAUTH:
				netAnnounce->set_servertype(AnnounceMessage::serverTypeInternetNoAuth);
				break;
			case SERVER_MODE_INTERNET_AUTH:
				netAnnounce->set_servertype(AnnounceMessage::serverTypeInternetAuth);
				break;
			}
			{
				boost::mutex::scoped_lock lock(m_statMutex);
				netAnnounce->set_numplayersonserver(m_statData.numberOfPlayersOnServer);
			}
			GetSender().Send(sessionData, packet);
			sessionData->GetReceiveBuffer().StartAsyncRead(sessionData);
		} else {
			// We do not accept sessions if we cannot
			// retrieve the client address.
			SessionError(sessionData, ERR_NET_INVALID_SESSION);
		}
	} else {
		// Server is full.
		// Gracefully close this session.
		SessionError(sessionData, ERR_NET_SERVER_FULL);
	}
}

void
ServerLobbyThread::ReAddSession(boost::shared_ptr<SessionData> session, int reason, unsigned gameId)
{
	if (session && session->GetPlayerData()) {
		boost::shared_ptr<NetPacket> packet(new NetPacket);
		packet->GetMsg()->set_messagetype(PokerTHMessage::Type_RemovedFromGameMessage);
		RemovedFromGameMessage *removed = packet->GetMsg()->mutable_removedfromgamemessage();
		removed->set_gameid(gameId);

		switch (reason) {
		case NTF_NET_REMOVED_GAME_FULL :
			removed->set_removedfromgamereason(RemovedFromGameMessage::gameIsFull);
			break;
		case NTF_NET_REMOVED_ALREADY_RUNNING :
			removed->set_removedfromgamereason(RemovedFromGameMessage::gameIsRunning);
			break;
		case NTF_NET_REMOVED_KICKED :
			removed->set_removedfromgamereason(RemovedFromGameMessage::kickedFromGame);
			break;
		case NTF_NET_REMOVED_TIMEOUT :
			removed->set_removedfromgamereason(RemovedFromGameMessage::gameTimeout);
			break;
		case NTF_NET_REMOVED_START_FAILED :
			removed->set_removedfromgamereason(RemovedFromGameMessage::removedStartFailed);
			break;
		case NTF_NET_REMOVED_GAME_CLOSED :
			removed->set_removedfromgamereason(RemovedFromGameMessage::gameClosed);
			break;
		default :
			removed->set_removedfromgamereason(RemovedFromGameMessage::removedOnRequest);
			break;
		}
		GetSender().Send(session, packet);

		HandleReAddedSession(session);
	}
}

void
ServerLobbyThread::MoveSessionToGame(boost::shared_ptr<ServerGame> game, boost::shared_ptr<SessionData> session, bool autoLeave, bool spectateOnly)
{
	// Remove session from the lobby.
	m_sessionManager.RemoveSession(session->GetId());
	// Session is now in game state.
	session->SetState(spectateOnly ? SessionData::Spectating : SessionData::Game);
	// Store it in the list of game sessions.
	m_gameSessionManager.AddSession(session);
	// Set the game id of the session.
	session->SetGame(game);
	// Add session to the game.
	game->AddSession(session, spectateOnly);
	// Optionally enable auto leave after game finish.
	if (autoLeave)
		game->SetPlayerAutoLeaveOnFinish(session->GetPlayerData()->GetUniqueId());
}

void
ServerLobbyThread::CloseSession(boost::shared_ptr<SessionData> session)
{
	if (session && session->GetState() != SessionData::Closed) { // Make this call reentrant.
		LOG_VERBOSE("Closing session #" << session->GetId() << ".");

		boost::shared_ptr<ServerGame> tmpGame = session->GetGame();
		if (tmpGame) {
			tmpGame->RemoveSession(session, NTF_NET_INTERNAL);
		}
		session->SetGame(boost::shared_ptr<ServerGame>());
		session->SetState(SessionData::Closed);

		m_sessionManager.RemoveSession(session->GetId());
		m_gameSessionManager.RemoveSession(session->GetId());

		if (session->GetPlayerData())
			NotifyPlayerLeftLobby(session->GetPlayerData()->GetUniqueId());
		// Update stats (if needed).
		UpdateStatisticsNumberOfPlayers();

		// Ignore error when shutting down the socket.
		boost::shared_ptr<boost::asio::ip::tcp::socket> sock = session->GetAsioSocket();
		if (sock) {
			boost::system::error_code ec;
			session->GetAsioSocket()->shutdown(boost::asio::ip::tcp::socket::shutdown_receive, ec);
		}
		// Close this session after send.
		GetSender().SetCloseAfterSend(session);
		// Cancel all timers of the session.
		session->CancelTimers();
	}
}

void
ServerLobbyThread::ResubscribeLobbyMsg(boost::shared_ptr<SessionData> session)
{
	InternalResubscribeMsg(session);
}

void
ServerLobbyThread::NotifyPlayerJoinedLobby(unsigned playerId)
{
	boost::shared_ptr<NetPacket> notify = CreateNetPacketPlayerListNew(playerId);
	m_sessionManager.SendLobbyMsgToAllSessions(GetSender(), notify, SessionData::Established);
	m_gameSessionManager.SendLobbyMsgToAllSessions(GetSender(), notify, SessionData::Game | SessionData::Spectating | SessionData::SpectatorWaiting);
}

void
ServerLobbyThread::NotifyPlayerLeftLobby(unsigned playerId)
{
	boost::shared_ptr<NetPacket> notify = CreateNetPacketPlayerListLeft(playerId);
	m_sessionManager.SendLobbyMsgToAllSessions(GetSender(), notify, SessionData::Established);
	m_gameSessionManager.SendLobbyMsgToAllSessions(GetSender(), notify, SessionData::Game | SessionData::Spectating | SessionData::SpectatorWaiting);
}

void
ServerLobbyThread::NotifyPlayerJoinedGame(unsigned gameId, unsigned playerId)
{
	// Send notification to players in lobby.
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_GameListPlayerJoinedMessage);
	GameListPlayerJoinedMessage *netListMsg = packet->GetMsg()->mutable_gamelistplayerjoinedmessage();
	netListMsg->set_gameid(gameId);
	netListMsg->set_playerid(playerId);

	m_sessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Established);
	m_gameSessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Game | SessionData::Spectating | SessionData::SpectatorWaiting);
}

void
ServerLobbyThread::NotifyPlayerLeftGame(unsigned gameId, unsigned playerId)
{
	// Send notification to players in lobby.
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_GameListPlayerLeftMessage);
	GameListPlayerLeftMessage *netListMsg = packet->GetMsg()->mutable_gamelistplayerleftmessage();
	netListMsg->set_gameid(gameId);
	netListMsg->set_playerid(playerId);

	m_sessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Established);
	m_gameSessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Game | SessionData::Spectating | SessionData::SpectatorWaiting);
}

void
ServerLobbyThread::NotifySpectatorJoinedGame(unsigned gameId, unsigned playerId)
{
	// Send notification to players in lobby.
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_GameListSpectatorJoinedMessage);
	GameListSpectatorJoinedMessage *netListMsg = packet->GetMsg()->mutable_gamelistspectatorjoinedmessage();
	netListMsg->set_gameid(gameId);
	netListMsg->set_playerid(playerId);

	m_sessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Established);
	m_gameSessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Game | SessionData::Spectating | SessionData::SpectatorWaiting);
}

void
ServerLobbyThread::NotifySpectatorLeftGame(unsigned gameId, unsigned playerId)
{
	// Send notification to players in lobby.
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_GameListSpectatorLeftMessage);
	GameListSpectatorLeftMessage *netListMsg = packet->GetMsg()->mutable_gamelistspectatorleftmessage();
	netListMsg->set_gameid(gameId);
	netListMsg->set_playerid(playerId);

	m_sessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Established);
	m_gameSessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Game | SessionData::Spectating | SessionData::SpectatorWaiting);
}

void
ServerLobbyThread::NotifyGameAdminChanged(unsigned gameId, unsigned newAdminPlayerId)
{
	// Send notification to players in lobby.
	// Send notification to players in lobby.
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_GameListAdminChangedMessage);
	GameListAdminChangedMessage *netListMsg = packet->GetMsg()->mutable_gamelistadminchangedmessage();
	netListMsg->set_gameid(gameId);
	netListMsg->set_newadminplayerid(newAdminPlayerId);

	m_sessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Established);
	m_gameSessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Game | SessionData::Spectating | SessionData::SpectatorWaiting);
}

void
ServerLobbyThread::NotifyStartingGame(unsigned gameId)
{
	boost::shared_ptr<NetPacket> packet = CreateNetPacketGameListUpdate(gameId, GAME_MODE_STARTED);
	m_sessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Established);
	m_gameSessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Game | SessionData::Spectating | SessionData::SpectatorWaiting);
}

void
ServerLobbyThread::NotifyReopeningGame(unsigned gameId)
{
	boost::shared_ptr<NetPacket> packet = CreateNetPacketGameListUpdate(gameId, GAME_MODE_CREATED);
	m_sessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Established);
	m_gameSessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Game | SessionData::Spectating | SessionData::SpectatorWaiting);
}

void
ServerLobbyThread::HandleGameRetrievePlayerInfo(boost::shared_ptr<SessionData> session, const PlayerInfoRequestMessage &playerInfoRequest)
{
	// Someone within a game requested player info.
	HandleNetPacketRetrievePlayerInfo(session, playerInfoRequest);
}

void
ServerLobbyThread::HandleGameRetrieveAvatar(boost::shared_ptr<SessionData> session, const AvatarRequestMessage &retrieveAvatar)
{
	// Someone within a game requested an avatar.
	HandleNetPacketRetrieveAvatar(session, retrieveAvatar);
}

void
ServerLobbyThread::HandleGameReportGame(boost::shared_ptr<SessionData> session, const ReportGameMessage &reportGame)
{
	// Someone within a game reportet a game name.
	HandleNetPacketReportGame(session, reportGame);
}

void
ServerLobbyThread::HandleChatRequest(boost::shared_ptr<SessionData> session, const ChatRequestMessage &chatRequest)
{
	// Someone within a game sent a lobby message.
	HandleNetPacketChatRequest(session, chatRequest);
}

void
ServerLobbyThread::HandleAdminRemoveGame(boost::shared_ptr<SessionData> session, const AdminRemoveGameMessage &removeGame)
{
	// An admin within a game sent a remove game message.
	HandleNetPacketAdminRemoveGame(session, removeGame);
}

void
ServerLobbyThread::HandleAdminBanPlayer(boost::shared_ptr<SessionData> session, const AdminBanPlayerMessage &banPlayer)
{
	// An admin within a game sent a ban player message.
	HandleNetPacketAdminBanPlayer(session, banPlayer);
}

bool
ServerLobbyThread::KickPlayerByName(const std::string &playerName)
{
	bool retVal = false;
	boost::shared_ptr<SessionData> session = m_sessionManager.GetSessionByPlayerName(playerName);
	if (!session)
		session = m_gameSessionManager.GetSessionByPlayerName(playerName);

	if (session && session->GetPlayerData()) {
		RemovePlayer(session->GetPlayerData()->GetUniqueId(), ERR_NET_PLAYER_KICKED);
		retVal = true;
	}

	return retVal;
}

bool
ServerLobbyThread::RemoveGameByPlayerName(const std::string &playerName)
{
	bool retVal = false;
	boost::shared_ptr<SessionData> session = m_gameSessionManager.GetSessionByPlayerName(playerName);

	if (session) {
		boost::shared_ptr<ServerGame> game = session->GetGame();
		if (game) {
			m_ioService->post(boost::bind(&ServerLobbyThread::InternalRemoveGame, shared_from_this(), game));
			retVal = true;
		}
	}

	return retVal;
}

string
ServerLobbyThread::GetPlayerIPAddress(const std::string &playerName) const
{
	string ipAddress;
	boost::shared_ptr<SessionData> session = m_sessionManager.GetSessionByPlayerName(playerName);
	if (!session)
		session = m_gameSessionManager.GetSessionByPlayerName(playerName);

	if (session)
		ipAddress = session->GetClientAddr();

	return ipAddress;
}

std::string
ServerLobbyThread::GetPlayerNameFromId(unsigned playerId) const
{
	string name;
	boost::shared_ptr<SessionData> session = m_sessionManager.GetSessionByUniquePlayerId(playerId);
	if (!session)
		session = m_gameSessionManager.GetSessionByUniquePlayerId(playerId);

	if (session && session->GetPlayerData())
		name = session->GetPlayerData()->GetName();

	return name;
}

void
ServerLobbyThread::RemovePlayer(unsigned playerId, unsigned errorCode)
{
	m_ioService->post(boost::bind(&ServerLobbyThread::InternalRemovePlayer, shared_from_this(), playerId, errorCode));
}

void
ServerLobbyThread::MutePlayerInGame(unsigned playerId)
{
	m_ioService->post(boost::bind(&ServerLobbyThread::InternalMutePlayerInGame, shared_from_this(), playerId));
}

void
ServerLobbyThread::SendGlobalChat(const string &message)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_ChatMessage);
	ChatMessage *netChat = packet->GetMsg()->mutable_chatmessage();
	netChat->set_chattype(ChatMessage::chatTypeBroadcast);
	netChat->set_chattext(message);

	m_sessionManager.SendToAllSessions(GetSender(), packet, SessionData::Established);
	m_gameSessionManager.SendToAllSessions(GetSender(), packet, SessionData::Game | SessionData::Spectating | SessionData::SpectatorWaiting);
}

void
ServerLobbyThread::SendGlobalMsgBox(const string &message)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_DialogMessage);
	DialogMessage *netDialog = packet->GetMsg()->mutable_dialogmessage();
	netDialog->set_notificationtext(message);

	m_sessionManager.SendToAllSessions(GetSender(), packet, SessionData::Established);
	m_gameSessionManager.SendToAllSessions(GetSender(), packet, SessionData::Game | SessionData::Spectating | SessionData::SpectatorWaiting);
}

void
ServerLobbyThread::SendChatBotMsg(const std::string &message)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_ChatMessage);
	ChatMessage *netChat = packet->GetMsg()->mutable_chatmessage();
	netChat->set_chattype(ChatMessage::chatTypeBot);
	netChat->set_chattext(message);

	m_sessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Established);
	m_gameSessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Game | SessionData::Spectating | SessionData::SpectatorWaiting);

	GetIrcBotCallback().SignalLobbyMessage(
		0,
		"(chat bot)",
		message);
}

void
ServerLobbyThread::SendChatBotMsg(unsigned gameId, const std::string &message)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_ChatMessage);
	ChatMessage *netChat = packet->GetMsg()->mutable_chatmessage();
	netChat->set_chattype(ChatMessage::chatTypeBot);
	netChat->set_gameid(gameId);
	netChat->set_chattext(message);

	GameMap::const_iterator pos = m_gameMap.find(gameId);
	if (pos != m_gameMap.end()) {
		pos->second->SendToAllPlayers(packet, SessionData::Game | SessionData::Spectating | SessionData::SpectatorWaiting);
	}
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

bool
ServerLobbyThread::SendToLobbyPlayer(unsigned playerId, boost::shared_ptr<NetPacket> packet)
{
	bool retVal = false;
	boost::shared_ptr<SessionData> tmpSession = m_sessionManager.GetSessionByUniquePlayerId(playerId);
	if (tmpSession) {
		GetSender().Send(tmpSession, packet);
		retVal = true;
	}
	return retVal;
}

AvatarManager &
ServerLobbyThread::GetAvatarManager()
{
	return m_avatarManager;
}

ChatCleanerManager &
ServerLobbyThread::GetChatCleaner()
{
	assert(m_chatCleanerManager);
	return *m_chatCleanerManager;
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

ServerMode
ServerLobbyThread::GetServerMode() const
{
	return m_mode;
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

boost::shared_ptr<ServerDBInterface>
ServerLobbyThread::GetDatabase()
{
	return m_database;
}

ServerBanManager &
ServerLobbyThread::GetBanManager()
{
	assert(m_banManager);
	return *m_banManager;
}

SessionDataCallback &
ServerLobbyThread::GetSessionDataCallback()
{
	return *m_internalServerCallback;
}

u_int32_t
ServerLobbyThread::GetNextSessionId()
{
	return m_curSessionId++;
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
	try {
		InitAuthContext();

		InitChatCleaner();
		// Start database engine.
		m_database->Start();
		// Register all timers.
		RegisterTimers();

		boost::asio::io_service::work ioWork(*m_ioService);
		m_ioService->run(); // Will only be aborted asynchronously.

	} catch (const PokerTHException &e) {
		GetCallback().SignalNetServerError(e.GetErrorId(), e.GetOsErrorCode());
		LOG_ERROR("Lobby exception: " << e.what());
	}
	// Clear all sessions and games.
	m_sessionManager.Clear();
	m_gameSessionManager.Clear();
	BOOST_FOREACH(const GameMap::value_type& tmpGame, m_gameMap) {
		tmpGame.second->Exit();
	}
	m_gameMap.clear();
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
	m_saveStatisticsTimer.cancel();
	m_loginLockTimer.cancel();
}

void
ServerLobbyThread::InitAuthContext()
{
	int res = gsasl_init(&m_authContext);
	if (res != GSASL_OK)
		throw ServerException(__FILE__, __LINE__, ERR_NET_GSASL_INIT_FAILED, 0);

	if (!gsasl_server_support_p(m_authContext, "SCRAM-SHA-1")) {
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
	if (m_serverConfig.readConfigInt("UseChatCleaner") != 0) {
		m_chatCleanerManager->Init(
			m_serverConfig.readConfigString("ChatCleanerHostAddress"),
			m_serverConfig.readConfigInt("ChatCleanerPort"),
			m_serverConfig.readConfigInt("ChatCleanerUseIpv6") != 0,
			m_serverConfig.readConfigString("ChatCleanerClientAuth"),
			m_serverConfig.readConfigString("ChatCleanerServerAuth"));
	}
}

void
ServerLobbyThread::DispatchPacket(boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet)
{
	if (session) {
		// Retrieve current game, if applicable.
		boost::shared_ptr<ServerGame> game = session->GetGame();
		if (game) {
			// We need to catch game-specific exceptions, so that they do not affect the server.
			try {
				game->HandlePacket(session, packet);
			} catch (const PokerTHException &e) {
				LOG_ERROR("Game " << game->GetId() << " - Read handler exception: " << e.what());
				game->RemoveAllSessions();
			}
		} else
			HandlePacket(session, packet);
	}
}

void
ServerLobbyThread::HandlePacket(boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet)
{
	if (session && packet) {
		if (packet->IsClientActivity())
			session->ResetActivityTimer();

		if (session->GetState() == SessionData::Init) {
			if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_InitMessage) {
				HandleNetPacketInit(session, packet->GetMsg()->initmessage());
			} else if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_AuthClientResponseMessage) {
				HandleNetPacketAuthClientResponse(session, packet->GetMsg()->authclientresponsemessage());
			} else if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_AvatarHeaderMessage) {
				HandleNetPacketAvatarHeader(session, packet->GetMsg()->avatarheadermessage());
			} else if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_UnknownAvatarMessage) {
				HandleNetPacketUnknownAvatar(session, packet->GetMsg()->unknownavatarmessage());
			} else {
				SessionError(session, ERR_SOCK_INVALID_STATE);
			}
		} else if (session->GetState() == SessionData::ReceivingAvatar) {
			if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_AvatarDataMessage) {
				HandleNetPacketAvatarFile(session, packet->GetMsg()->avatardatamessage());
			} else if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_AvatarEndMessage) {
				HandleNetPacketAvatarEnd(session, packet->GetMsg()->avatarendmessage());
			} else {
				SessionError(session, ERR_SOCK_INVALID_STATE);
			}
		} else {
			if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_PlayerInfoRequestMessage)
				HandleNetPacketRetrievePlayerInfo(session, packet->GetMsg()->playerinforequestmessage());
			else if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_AvatarRequestMessage)
				HandleNetPacketRetrieveAvatar(session, packet->GetMsg()->avatarrequestmessage());
			else if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_ResetTimeoutMessage)
			{}
			else if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_SubscriptionRequestMessage) {
				const SubscriptionRequestMessage &subscriptionRequest = packet->GetMsg()->subscriptionrequestmessage();
				if (subscriptionRequest.subscriptionaction() == SubscriptionRequestMessage::resubscribeGameList)
					InternalResubscribeMsg(session);
				else
					session->ResetWantsLobbyMsg();
			} else if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_JoinNewGameMessage) {
				HandleNetPacketCreateGame(session, packet->GetMsg()->joinnewgamemessage());
			} else if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_JoinExistingGameMessage) {
				HandleNetPacketJoinGame(session, packet->GetMsg()->joinexistinggamemessage());
			} else if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_RejoinExistingGameMessage) {
				HandleNetPacketRejoinGame(session, packet->GetMsg()->rejoinexistinggamemessage());
			} else if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_ChatRequestMessage) {
				HandleNetPacketChatRequest(session, packet->GetMsg()->chatrequestmessage());
			} else if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_RejectGameInvitationMessage) {
				HandleNetPacketRejectGameInvitation(session, packet->GetMsg()->rejectgameinvitationmessage());
			} else if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_ReportGameMessage) {
				HandleNetPacketReportGame(session, packet->GetMsg()->reportgamemessage());
			} else if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_LeaveGameRequestMessage) {
				// Ignore "leave game" in this state.
			} else if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_AdminRemoveGameMessage) {
				HandleNetPacketAdminRemoveGame(session, packet->GetMsg()->adminremovegamemessage());
			} else if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_AdminBanPlayerMessage) {
				HandleNetPacketAdminBanPlayer(session, packet->GetMsg()->adminbanplayermessage());
			} else {
				SessionError(session, ERR_SOCK_INVALID_STATE);
			}
		}
	}
}

void
ServerLobbyThread::HandleNetPacketInit(boost::shared_ptr<SessionData> session, const InitMessage &initMessage)
{
	LOG_VERBOSE("Received init for session #" << session->GetId() << ".");

	// Before any other processing, perform some denial of service and
	// brute force attack prevention by checking whether the user recently sent an
	// Init packet.
	if (m_serverConfig.readConfigInt("ServerBruteForceProtection") != 0) {
		bool recentlySentInit = false;
		{
			boost::mutex::scoped_lock lock(m_timerClientAddressMapMutex);
			if (m_timerClientAddressMap.find(session->GetClientAddr()) != m_timerClientAddressMap.end())
				recentlySentInit = true;
			else
				m_timerClientAddressMap[session->GetClientAddr()] = boost::timers::portable::microsec_timer();
		}
		if (recentlySentInit) {
			SessionError(session, ERR_NET_INIT_BLOCKED);
			return;
		}
	}

	// Check the protocol version.
	if (initMessage.requestedversion().majorversion() != NET_VERSION_MAJOR
			|| session->GetPlayerData()) { // Has this session already sent an init?
		SessionError(session, ERR_NET_VERSION_NOT_SUPPORTED);
		return;
	}
#ifndef POKERTH_OFFICIAL_SERVER
	// Check (clear text) server password (skip for official server, they are open to everyone).
	string serverPassword;
	if (initMessage.has_authserverpassword()) {
		serverPassword = initMessage.authserverpassword();
	}
	if (serverPassword != m_serverConfig.readConfigString("ServerPassword")) {
		SessionError(session, ERR_NET_INVALID_PASSWORD);
		return;
	}
#endif

	string playerName;
	MD5Buf avatarMD5;
	bool noAuth = false;
	bool validGuest = false;
	if (initMessage.login() == InitMessage::guestLogin) {
		playerName = initMessage.nickname();
		// Verify guest player name.
		if (playerName.length() > sizeof(SERVER_GUEST_PLAYER_NAME - 1)
				&& playerName.substr(0, sizeof(SERVER_GUEST_PLAYER_NAME) - 1) == SERVER_GUEST_PLAYER_NAME) {
			string guestId(playerName.substr(sizeof(SERVER_GUEST_PLAYER_NAME)));
			if ((size_t)count_if(guestId.begin(), guestId.end(), ::isdigit) == guestId.size()) {
				validGuest = true;
				noAuth = true;
			}
		}
		if (!validGuest) {
			SessionError(session, ERR_NET_INVALID_PLAYER_NAME);
			return;
		}
	}
#ifdef POKERTH_OFFICIAL_SERVER
	else if (initMessage.login() == InitMessage::authenticatedLogin) {
		string inAuthData(initMessage.clientuserdata());
		if (initMessage.has_avatarhash()) {
			memcpy(avatarMD5.GetData(), initMessage.avatarhash().data(), MD5_DATA_SIZE);
		}
		session->CreateServerAuthSession(m_authContext);
		if (session->AuthStep(1, inAuthData))
			playerName = session->AuthGetUser();
	}
#else
	else if (initMessage.login() == InitMessage::unauthenticatedLogin) {
		playerName = initMessage.nickname();
		if (initMessage.has_avatarhash()) {
			memcpy(avatarMD5.GetData(), initMessage.avatarhash().data(), MD5_DATA_SIZE);
		}
		noAuth = true;
	}
#endif
	else {
		SessionError(session, ERR_NET_INVALID_PASSWORD);
		return;
	}

	// Check whether the player name is correct.
	// Partly, this is also done in netpacket.
	// However, some disallowed names are checked only here.
	if (playerName.empty()
			|| playerName[0] == '#'
			|| playerName[0] == ' '
			|| playerName.substr(0, sizeof(SERVER_COMPUTER_PLAYER_NAME) - 1) == SERVER_COMPUTER_PLAYER_NAME) {
		SessionError(session, ERR_NET_INVALID_PLAYER_NAME);
		return;
	}

	// Check whether the player name is banned.
	if (GetBanManager().IsPlayerBanned(playerName)) {
		SessionError(session, ERR_NET_PLAYER_BANNED);
		return;
	}
	// Check whether the peer IP address is banned.
	if (GetBanManager().IsIPAddressBanned(session->GetClientAddr())) {
		SessionError(session, ERR_NET_PLAYER_BANNED);
		return;
	}

	// Create player data object.
	boost::shared_ptr<PlayerData> tmpPlayerData(
		new PlayerData(GetNextUniquePlayerId(), 0, PLAYER_TYPE_HUMAN, validGuest ? PLAYER_RIGHTS_GUEST : PLAYER_RIGHTS_NORMAL, false));
	tmpPlayerData->SetName(playerName);
	tmpPlayerData->SetAvatarMD5(avatarMD5);
	if (initMessage.has_mylastsessionid()) {
		tmpPlayerData->SetOldGuid(initMessage.mylastsessionid());
	}

	// Set player data for session.
	m_sessionManager.SetSessionPlayerData(session->GetId(), tmpPlayerData);
	session->SetPlayerData(tmpPlayerData);

	if (noAuth)
		InitAfterLogin(session);
	else
		AuthenticatePlayer(session);
}

void
ServerLobbyThread::HandleNetPacketAuthClientResponse(boost::shared_ptr<SessionData> session, const AuthClientResponseMessage &clientResponse)
{
	if (session && session->GetPlayerData() && session->AuthGetCurStepNum() == 1) {
		string authData = clientResponse.clientresponse();
		if (session->AuthStep(2, authData)) {
			string outVerification(session->AuthGetNextOutMsg());

			boost::shared_ptr<NetPacket> packet(new NetPacket);
			packet->GetMsg()->set_messagetype(PokerTHMessage::Type_AuthServerVerificationMessage);
			AuthServerVerificationMessage *verification = packet->GetMsg()->mutable_authserververificationmessage();
			verification->set_serververification(outVerification);
			GetSender().Send(session, packet);
			// The last message is only for server verification.
			// We are done now, the user has logged in.
			boost::shared_ptr<PlayerData> tmpPlayerData = session->GetPlayerData();
			if (GetBanManager().IsAdminPlayer(tmpPlayerData->GetDBId())) {
				session->GetPlayerData()->SetRights(PLAYER_RIGHTS_ADMIN);
			}
			CheckAvatarBlacklist(session);
		} else
			SessionError(session, ERR_NET_INVALID_PASSWORD);
	}
}

void
ServerLobbyThread::HandleNetPacketAvatarHeader(boost::shared_ptr<SessionData> session, const AvatarHeaderMessage &avatarHeader)
{
	if (session->GetPlayerData()) {
		if (avatarHeader.avatarsize() >= MIN_AVATAR_FILE_SIZE && avatarHeader.avatarsize() <= MAX_AVATAR_FILE_SIZE) {
			boost::shared_ptr<AvatarFile> tmpAvatarFile(new AvatarFile);
			tmpAvatarFile->fileData.reserve(avatarHeader.avatarsize());
			tmpAvatarFile->fileType = static_cast<AvatarFileType>(avatarHeader.avatartype());
			tmpAvatarFile->reportedSize = avatarHeader.avatarsize();
			// Ignore request id for now.

			session->GetPlayerData()->SetNetAvatarFile(tmpAvatarFile);

			// Session is now receiving an avatar.
			session->SetState(SessionData::ReceivingAvatar);
		} else
			SessionError(session, ERR_NET_AVATAR_TOO_LARGE);
	}
}

void
ServerLobbyThread::HandleNetPacketUnknownAvatar(boost::shared_ptr<SessionData> session, const UnknownAvatarMessage &/*unknownAvatar*/)
{
	if (session->GetPlayerData()) {
		// Free memory (just in case).
		session->GetPlayerData()->SetNetAvatarFile(boost::shared_ptr<AvatarFile>());
		session->GetPlayerData()->SetAvatarMD5(MD5Buf());
		// Start session.
		EstablishSession(session);
	}
}

void
ServerLobbyThread::HandleNetPacketAvatarFile(boost::shared_ptr<SessionData> session, const AvatarDataMessage &avatarData)
{
	if (session->GetPlayerData()) {
		boost::shared_ptr<AvatarFile> tmpAvatar = session->GetPlayerData()->GetNetAvatarFile();
		const string &avatarBlock = avatarData.avatarblock();
		if (tmpAvatar && tmpAvatar->fileData.size() + avatarBlock.size() <= tmpAvatar->reportedSize) {
			std::copy(&avatarBlock[0], &avatarBlock[avatarBlock.size()], back_inserter(tmpAvatar->fileData));
		}
	}
}

void
ServerLobbyThread::HandleNetPacketAvatarEnd(boost::shared_ptr<SessionData> session, const AvatarEndMessage &/*avatarEnd*/)
{
	if (session->GetPlayerData()) {
		boost::shared_ptr<AvatarFile> tmpAvatar = session->GetPlayerData()->GetNetAvatarFile();
		MD5Buf avatarMD5 = session->GetPlayerData()->GetAvatarMD5();
		if (!avatarMD5.IsZero() && tmpAvatar.get()) {
			unsigned avatarSize = (unsigned)tmpAvatar->fileData.size();
			if (avatarSize == tmpAvatar->reportedSize) {
				if (!GetAvatarManager().StoreAvatarInCache(avatarMD5, tmpAvatar->fileType, &tmpAvatar->fileData[0], avatarSize, true)) {
					session->GetPlayerData()->SetAvatarMD5(MD5Buf());
					LOG_ERROR("Failed to store avatar in cache directory.");
				}

				// Free memory.
				session->GetPlayerData()->SetNetAvatarFile(boost::shared_ptr<AvatarFile>());
				// Set avatar file name.
				string avatarFileName;
				if (GetAvatarManager().GetAvatarFileName(avatarMD5, avatarFileName))
					session->GetPlayerData()->SetAvatarFile(avatarFileName);
				// Init finished - start session.
				EstablishSession(session);
				LOG_MSG("Client \"" << session->GetClientAddr() << "\" uploaded avatar \""
						<< boost::filesystem::path(avatarFileName).file_string() << "\".");
			} else
				SessionError(session, ERR_NET_WRONG_AVATAR_SIZE);
		}
	}
}

void
ServerLobbyThread::HandleNetPacketRetrievePlayerInfo(boost::shared_ptr<SessionData> session, const PlayerInfoRequestMessage &playerInfoRequest)
{
	BOOST_FOREACH(unsigned playerId, playerInfoRequest.playerid()) {
		// Find player in lobby or in a game.
		boost::shared_ptr<SessionData> tmpSession = m_sessionManager.GetSessionByUniquePlayerId(playerId);
		if (!tmpSession) {
			tmpSession = m_gameSessionManager.GetSessionByUniquePlayerId(playerId);
		}
		boost::shared_ptr<PlayerData> tmpPlayer;
		if (tmpSession) {
			tmpPlayer = tmpSession->GetPlayerData();
		}

		if (!tmpPlayer) {
			boost::mutex::scoped_lock lock(m_computerPlayersMutex);
			PlayerDataMap::const_iterator pos = m_computerPlayers.find(playerId);
			if (pos != m_computerPlayers.end())
				tmpPlayer = pos->second;
		}

		boost::shared_ptr<NetPacket> packet(new NetPacket);
		packet->GetMsg()->set_messagetype(PokerTHMessage::Type_PlayerInfoReplyMessage);
		PlayerInfoReplyMessage *netPlayerInfoReply = packet->GetMsg()->mutable_playerinforeplymessage();
		netPlayerInfoReply->set_playerid(playerId);

		if (tmpPlayer) {
			// Send player info to client.
			PlayerInfoReplyMessage::PlayerInfoData *data = netPlayerInfoReply->mutable_playerinfodata();

			data->set_playername(tmpPlayer->GetName());
			data->set_ishuman(tmpPlayer->GetType() == PLAYER_TYPE_HUMAN);
			data->set_playerrights(static_cast<NetPlayerInfoRights>(tmpPlayer->GetRights()));
			if (!tmpPlayer->GetCountry().empty()) {
				data->set_countrycode(tmpPlayer->GetCountry());
			}
			if (!tmpPlayer->GetAvatarMD5().IsZero()) {
				PlayerInfoReplyMessage::PlayerInfoData::AvatarData *avatarData = data->mutable_avatardata();
				avatarData->set_avatartype(static_cast<NetAvatarType>(AvatarManager::GetAvatarFileType(tmpPlayer->GetAvatarFile())));
				avatarData->set_avatarhash(tmpPlayer->GetAvatarMD5().GetData(), MD5_DATA_SIZE);
			}
		} else {
			// Unknown player id - do not set any data.
		}
		GetSender().Send(session, packet);
	}
}

void
ServerLobbyThread::HandleNetPacketRetrieveAvatar(boost::shared_ptr<SessionData> session, const AvatarRequestMessage &retrieveAvatar)
{
	bool avatarFound = false;

	string tmpFile;
	MD5Buf tmpMD5;
	memcpy(tmpMD5.GetData(), retrieveAvatar.avatarhash().data(), MD5_DATA_SIZE);
	if (GetAvatarManager().GetAvatarFileName(tmpMD5, tmpFile)) {
		NetPacketList tmpPackets;
		if (GetAvatarManager().AvatarFileToNetPackets(tmpFile, retrieveAvatar.requestid(), tmpPackets) == 0) {
			avatarFound = true;
			GetSender().Send(session, tmpPackets);
		} else
			LOG_ERROR("Failed to read avatar file for network transmission.");
	}

	if (!avatarFound) {
		// Notify client we didn't find the avatar.
		boost::shared_ptr<NetPacket> unknownAvatar(new NetPacket);
		unknownAvatar->GetMsg()->set_messagetype(PokerTHMessage::Type_UnknownAvatarMessage);
		UnknownAvatarMessage *netAvatarReply = unknownAvatar->GetMsg()->mutable_unknownavatarmessage();
		netAvatarReply->set_requestid(retrieveAvatar.requestid());

		GetSender().Send(session, unknownAvatar);
	}
}

void
ServerLobbyThread::HandleNetPacketCreateGame(boost::shared_ptr<SessionData> session, const JoinNewGameMessage &newGame)
{
	LOG_VERBOSE("Creating new game, initiated by session #" << session->GetId() << ".");

	string password;
	if (newGame.has_password())
		password = newGame.password();

	// Create a new game.
	GameData tmpData;
	NetPacket::GetGameData(newGame.gameinfo(), tmpData);
	string gameName(newGame.gameinfo().gamename());
	// Always trim the game name.
	boost::trim(gameName);
	boost::replace_all(gameName, "\n", " ");
	boost::replace_all(gameName, "\r", " ");
	boost::replace_all(gameName, "\t", " ");
	boost::replace_all(gameName, "\v", " ");
	boost::replace_all(gameName, "\f", " ");
	unsigned gameId = GetNextGameId();

	if (gameName.empty() || !isprint(gameName[0])) {
		SendJoinGameFailed(session, gameId, NTF_NET_JOIN_GAME_BAD_NAME);
	} else if (IsGameNameInUse(gameName)) {
		SendJoinGameFailed(session, gameId, NTF_NET_JOIN_GAME_NAME_IN_USE);
	} else if (GetBanManager().IsBadGameName(gameName)) {
		SendJoinGameFailed(session, gameId, NTF_NET_JOIN_GAME_BAD_NAME);
	} else if (session->GetPlayerData()->GetRights() == PLAYER_RIGHTS_GUEST
			   && tmpData.gameType != GAME_TYPE_NORMAL) {
		SendJoinGameFailed(session, gameId, NTF_NET_JOIN_GUEST_FORBIDDEN);
	} else if (!ServerGame::CheckSettings(tmpData, password, GetServerMode())) {
		SendJoinGameFailed(session, gameId, NTF_NET_JOIN_INVALID_SETTINGS);
	} else {
		boost::shared_ptr<ServerGame> game(
			new ServerGame(
				shared_from_this(),
				gameId,
				gameName,
				password,
				tmpData,
				session->GetPlayerData()->GetUniqueId(),
				session->GetPlayerData()->GetDBId(),
				GetGui(),
				m_serverConfig));
		game->Init();

		// Add game to list of games.
		InternalAddGame(game);

		MoveSessionToGame(game, session, newGame.autoleave(), false);
	}
}

void
ServerLobbyThread::HandleNetPacketJoinGame(boost::shared_ptr<SessionData> session, const JoinExistingGameMessage &joinGame)
{
	string password;
	if (joinGame.has_password())
		password = joinGame.password();

	// Join an existing game.
	GameMap::iterator pos = m_gameMap.find(joinGame.gameid());

	if (pos != m_gameMap.end()) {
		boost::shared_ptr<ServerGame> game = pos->second;
		const GameData &tmpData = game->GetGameData();
		if (joinGame.spectateonly()) {
			if (!tmpData.allowSpectators) {
				SendJoinGameFailed(session, joinGame.gameid(), NTF_NET_JOIN_NO_SPECTATORS);
			} else {
				MoveSessionToGame(game, session, joinGame.autoleave(), true);
			}
		} else {
			// As guest, you are only allowed to join normal games.
			if (session->GetPlayerData()->GetRights() == PLAYER_RIGHTS_GUEST
					&& tmpData.gameType != GAME_TYPE_NORMAL) {
				SendJoinGameFailed(session, joinGame.gameid(), NTF_NET_JOIN_GUEST_FORBIDDEN);
			} else if (tmpData.gameType == GAME_TYPE_INVITE_ONLY
					   && !game->IsPlayerInvited(session->GetPlayerData()->GetUniqueId())) {
				SendJoinGameFailed(session, joinGame.gameid(), NTF_NET_JOIN_NOT_INVITED);
			} else if (!game->CheckPassword(password)) {
				SendJoinGameFailed(session, joinGame.gameid(), NTF_NET_JOIN_INVALID_PASSWORD);
			} else if (tmpData.gameType == GAME_TYPE_RANKING
					   && !joinGame.spectateonly()
					   && session->GetClientAddr() != SERVER_ADDRESS_LOCALHOST_STR
					   && session->GetClientAddr() != SERVER_ADDRESS_LOCALHOST_STR_V4V6
					   && session->GetClientAddr() != SERVER_ADDRESS_LOCALHOST_STR_V4
					   && game->IsClientAddressConnected(session->GetClientAddr())) {
				SendJoinGameFailed(session, joinGame.gameid(), NTF_NET_JOIN_IP_BLOCKED);
			} else {
				MoveSessionToGame(game, session, joinGame.autoleave(), false);
			}
		}
	} else {
		SendJoinGameFailed(session, joinGame.gameid(), NTF_NET_JOIN_GAME_INVALID);
	}
}

void
ServerLobbyThread::HandleNetPacketRejoinGame(boost::shared_ptr<SessionData> session, const RejoinExistingGameMessage &rejoinGame)
{
	// Rejoin a running game.
	GameMap::iterator pos = m_gameMap.find(rejoinGame.gameid());

	if (pos != m_gameMap.end()) {
		boost::shared_ptr<ServerGame> game = pos->second;
		MoveSessionToGame(game, session, rejoinGame.autoleave(), false);
	} else {
		SendJoinGameFailed(session, rejoinGame.gameid(), NTF_NET_JOIN_GAME_INVALID);
	}
}

void
ServerLobbyThread::HandleNetPacketChatRequest(boost::shared_ptr<SessionData> session, const ChatRequestMessage &chatRequest)
{
	bool chatSent = false;
	// Guests are not allowed to chat.
	if (session->GetPlayerData() && session->GetPlayerData()->GetRights() != PLAYER_RIGHTS_GUEST) {
		if (!chatRequest.has_targetgameid() && !chatRequest.has_targetplayerid()) {
			string chatMsg(chatRequest.chattext());

			boost::shared_ptr<NetPacket> packet(new NetPacket);
			packet->GetMsg()->set_messagetype(PokerTHMessage::Type_ChatMessage);
			ChatMessage *netChat = packet->GetMsg()->mutable_chatmessage();
			netChat->set_chattype(ChatMessage::chatTypeLobby);
			netChat->set_playerid(session->GetPlayerData()->GetUniqueId());
			netChat->set_chattext(chatMsg);

			m_sessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Established);
			m_gameSessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Game | SessionData::Spectating | SessionData::SpectatorWaiting);

			// Send the message to the chat cleaner bot.
			m_chatCleanerManager->HandleLobbyChatText(
				session->GetPlayerData()->GetUniqueId(),
				session->GetPlayerData()->GetName(),
				chatMsg);
			// Send the message to the irc bot.
			GetIrcBotCallback().SignalLobbyMessage(
				session->GetPlayerData()->GetUniqueId(),
				session->GetPlayerData()->GetName(),
				chatMsg);
			chatSent = true;
		} else if (!chatRequest.has_targetgameid() && chatRequest.has_targetplayerid()) {
			boost::shared_ptr<SessionData> targetSession = m_sessionManager.GetSessionByUniquePlayerId(chatRequest.targetplayerid());
			if (!targetSession)
				targetSession = m_gameSessionManager.GetSessionByUniquePlayerId(chatRequest.targetplayerid());

			if (targetSession && targetSession->GetPlayerData()) {
				// Only allow private messages to players which are not in running games.
				boost::shared_ptr<ServerGame> tmpGame = targetSession->GetGame();
				if (!tmpGame || !tmpGame->IsRunning()) {
					boost::shared_ptr<NetPacket> packet(new NetPacket);
					packet->GetMsg()->set_messagetype(PokerTHMessage::Type_ChatMessage);
					ChatMessage *netChat = packet->GetMsg()->mutable_chatmessage();
					netChat->set_chattype(ChatMessage::chatTypePrivate);
					netChat->set_playerid(session->GetPlayerData()->GetUniqueId());
					netChat->set_chattext(chatRequest.chattext());

					GetSender().Send(targetSession, packet);
					chatSent = true;
				}
			}
		}
	}
	// Other chat types are not allowed in the lobby.
	if (!chatSent) {
		boost::shared_ptr<NetPacket> packet(new NetPacket);
		packet->GetMsg()->set_messagetype(PokerTHMessage::Type_ChatRejectMessage);
		ChatRejectMessage *netReject = packet->GetMsg()->mutable_chatrejectmessage();
		netReject->set_chattext(chatRequest.chattext());
		GetSender().Send(session, packet);
	}
}

void
ServerLobbyThread::HandleNetPacketRejectGameInvitation(boost::shared_ptr<SessionData> session, const RejectGameInvitationMessage &reject)
{
	GameMap::iterator pos = m_gameMap.find(reject.gameid());

	if (pos != m_gameMap.end() && session->GetPlayerData()) {
		ServerGame &game = *pos->second;
		unsigned tmpPlayerId = session->GetPlayerData()->GetUniqueId();
		if (game.IsPlayerInvited(tmpPlayerId)) {
			// If he actively rejects, he is no longer invited.
			if (reject.myrejectreason() == RejectGameInvitationMessage::rejectReasonNo) {
				game.RemovePlayerInvitation(tmpPlayerId);
			}
			// Send reject notification.
			boost::shared_ptr<NetPacket> packet(new NetPacket);
			packet->GetMsg()->set_messagetype(PokerTHMessage::Type_RejectInvNotifyMessage);
			RejectInvNotifyMessage *netReject = packet->GetMsg()->mutable_rejectinvnotifymessage();
			netReject->set_gameid(reject.gameid());
			netReject->set_playerid(tmpPlayerId);
			netReject->set_playerrejectreason(reject.myrejectreason());

			game.SendToAllPlayers(packet, SessionData::Game | SessionData::Spectating | SessionData::SpectatorWaiting);
		}
	}
}

void
ServerLobbyThread::HandleNetPacketReportGame(boost::shared_ptr<SessionData> session, const ReportGameMessage &report)
{
	GameMap::iterator pos = m_gameMap.find(report.reportedgameid());

	if (pos != m_gameMap.end() && session->GetPlayerData()) {
		boost::shared_ptr<ServerGame> tmpGame(pos->second);
		if (!tmpGame->IsNameReported()) {
			// Temporarily note that this game was reported.
			// This prevents spamming of the game report.
			tmpGame->SetNameReported();
			unsigned creatorDBId = tmpGame->GetCreatorDBId();
			unsigned reporterDBId = session->GetPlayerData()->GetDBId();
			GetDatabase()->AsyncReportGame(
				session->GetPlayerData()->GetUniqueId(),
				tmpGame->GetId(),
				creatorDBId != 0 ? &creatorDBId : NULL,
				tmpGame->GetId(),
				tmpGame->GetName(),
				reporterDBId != 0 ? &reporterDBId : NULL
			);
		} else {
			boost::shared_ptr<NetPacket> packet(new NetPacket);
			packet->GetMsg()->set_messagetype(PokerTHMessage::Type_ReportGameAckMessage);
			ReportGameAckMessage *netReportAck = packet->GetMsg()->mutable_reportgameackmessage();
			netReportAck->set_reportedgameid(report.reportedgameid());
			netReportAck->set_reportgameresult(ReportGameAckMessage::gameReportDuplicate);
			GetSender().Send(session, packet);
		}
	} else {
		boost::shared_ptr<NetPacket> packet(new NetPacket);
		packet->GetMsg()->set_messagetype(PokerTHMessage::Type_ReportGameAckMessage);
		ReportGameAckMessage *netReportAck = packet->GetMsg()->mutable_reportgameackmessage();
		netReportAck->set_reportedgameid(report.reportedgameid());
		netReportAck->set_reportgameresult(ReportGameAckMessage::gameReportInvalid);
		GetSender().Send(session, packet);
	}
}

void
ServerLobbyThread::HandleNetPacketAdminRemoveGame(boost::shared_ptr<SessionData> session, const AdminRemoveGameMessage &removeGame)
{
	GameMap::iterator pos = m_gameMap.find(removeGame.removegameid());

	// Create Ack-Packet.
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_AdminRemoveGameAckMessage);
	AdminRemoveGameAckMessage *netRemoveAck = packet->GetMsg()->mutable_adminremovegameackmessage();
	netRemoveAck->set_removegameid(removeGame.removegameid());

	// Check whether game id is valid and whether the player is an admin.
	if (pos != m_gameMap.end() && session->GetPlayerData() && GetBanManager().IsAdminPlayer(session->GetPlayerData()->GetDBId())) {
		LOG_ERROR("Player " << session->GetPlayerData()->GetName() << "(" << session->GetPlayerData()->GetDBId() << ") removes game '" << pos->second->GetName() << "'");
		InternalRemoveGame(pos->second);
		netRemoveAck->set_removegameresult(AdminRemoveGameAckMessage::gameRemoveAccepted);
	} else {
		netRemoveAck->set_removegameresult(AdminRemoveGameAckMessage::gameRemoveInvalid);
	}
	GetSender().Send(session, packet);
}

void
ServerLobbyThread::HandleNetPacketAdminBanPlayer(boost::shared_ptr<SessionData> session, const AdminBanPlayerMessage &banPlayer)
{
	// Create Ack-Packet.
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_AdminBanPlayerAckMessage);
	AdminBanPlayerAckMessage *netBanAck = packet->GetMsg()->mutable_adminbanplayerackmessage();
	netBanAck->set_banplayerid(banPlayer.banplayerid());

	if (session && session->GetPlayerData() && GetBanManager().IsAdminPlayer(session->GetPlayerData()->GetDBId())) {

		boost::shared_ptr<SessionData> tmpSession = m_sessionManager.GetSessionByUniquePlayerId(banPlayer.banplayerid());
		if (!tmpSession) {
			tmpSession = m_gameSessionManager.GetSessionByUniquePlayerId(banPlayer.banplayerid());
		}
		boost::shared_ptr<PlayerData> tmpPlayer;
		if (tmpSession) {
			tmpPlayer = tmpSession->GetPlayerData();
		}
		if (tmpPlayer && !GetBanManager().IsAdminPlayer(tmpPlayer->GetDBId())) {
			// Ban the player's IP address for 24 hours.
			GetBanManager().BanIPAddress(tmpSession->GetClientAddr(), 24);
			// Kick the player.
			RemovePlayer(tmpPlayer->GetUniqueId(), ERR_NET_PLAYER_KICKED);
			// Permanently ban the player in the database.
			if (tmpPlayer->GetDBId() != DB_ID_INVALID) {
				LOG_ERROR("Player " << session->GetPlayerData()->GetName() << "(" << session->GetPlayerData()->GetDBId()
						  << ") bans player " << tmpPlayer->GetName() << "(" << tmpPlayer->GetDBId()
						  << ") who has IP " << tmpSession->GetClientAddr());
				GetDatabase()->AsyncBlockPlayer(session->GetPlayerData()->GetUniqueId(), tmpPlayer->GetUniqueId(), tmpPlayer->GetDBId(), 0, 4);
				netBanAck->set_banplayerresult(AdminBanPlayerAckMessage::banPlayerPending);
			} else {
				netBanAck->set_banplayerresult(AdminBanPlayerAckMessage::banPlayerNoDB);
			}
		} else {
			netBanAck->set_banplayerresult(AdminBanPlayerAckMessage::banPlayerInvalid);
		}
	} else {
		netBanAck->set_banplayerresult(AdminBanPlayerAckMessage::banPlayerInvalid);
	}
	GetSender().Send(session, packet);
}

void
ServerLobbyThread::AuthChallenge(boost::shared_ptr<SessionData> session, const string &secret)
{
	if (session && session->GetPlayerData() && session->AuthGetCurStepNum() == 1) {
		session->AuthSetPassword(secret); // For this auth session.
		string outChallenge(session->AuthGetNextOutMsg());

		boost::shared_ptr<NetPacket> packet(new NetPacket);
		packet->GetMsg()->set_messagetype(PokerTHMessage::Type_AuthServerChallengeMessage);
		AuthServerChallengeMessage *challenge = packet->GetMsg()->mutable_authserverchallengemessage();
		challenge->set_serverchallenge(outChallenge);
		GetSender().Send(session, packet);
	}
}

void
ServerLobbyThread::CheckAvatarBlacklist(boost::shared_ptr<SessionData> session)
{
	if (session && session->GetPlayerData()) {
		const MD5Buf &avatarMD5 = session->GetPlayerData()->GetAvatarMD5();
		if (!avatarMD5.IsZero())
			m_database->AsyncCheckAvatarBlacklist(session->GetPlayerData()->GetUniqueId(), avatarMD5.ToString());
		else
			InitAfterLogin(session);
	}
}

void
ServerLobbyThread::AvatarBlacklisted(unsigned playerId)
{
	boost::shared_ptr<SessionData> tmpSession = m_sessionManager.GetSessionByUniquePlayerId(playerId, true);
	if (tmpSession && tmpSession->GetPlayerData()) {
		tmpSession->GetPlayerData()->SetAvatarMD5(MD5Buf()); // Reset avatar if blacklisted.
		InitAfterLogin(tmpSession);
	}
}

void
ServerLobbyThread::AvatarOK(unsigned playerId)
{
	boost::shared_ptr<SessionData> tmpSession = m_sessionManager.GetSessionByUniquePlayerId(playerId, true);
	InitAfterLogin(tmpSession);
}

void
ServerLobbyThread::InitAfterLogin(boost::shared_ptr<SessionData> session)
{
	if (session && session->GetPlayerData()) {
		const MD5Buf &avatarMD5 = session->GetPlayerData()->GetAvatarMD5();
		string avatarFileName;
		if (!avatarMD5.IsZero()
				&& !GetAvatarManager().GetAvatarFileName(avatarMD5, avatarFileName)) {
			RequestPlayerAvatar(session);
		} else {
			if (!avatarFileName.empty())
				session->GetPlayerData()->SetAvatarFile(avatarFileName);
			EstablishSession(session);
		}
	}
}

void
ServerLobbyThread::EstablishSession(boost::shared_ptr<SessionData> session)
{
	if (!session->GetPlayerData())
		throw ServerException(__FILE__, __LINE__, ERR_NET_INVALID_SESSION, 0);

	unsigned rejoinPlayerId = 0;
	u_int32_t rejoinGameId = GetRejoinGameIdForPlayer(session->GetPlayerData()->GetName(), session->GetPlayerData()->GetOldGuid(), rejoinPlayerId);
	if (rejoinGameId != 0) {
		// Offer rejoin, and disconnect current player with the same name.
		InternalRemovePlayer(rejoinPlayerId, ERR_NET_PLAYER_NAME_IN_USE);
	} else {
		// Check whether this player is already connected.
		unsigned previousPlayerId = GetPlayerId(session->GetPlayerData()->GetName());
		if (previousPlayerId != 0 && previousPlayerId != session->GetPlayerData()->GetUniqueId()) {
#ifdef POKERTH_OFFICIAL_SERVER
			// If so, and this is a login server, disconnect the already connected player.
			InternalRemovePlayer(previousPlayerId, ERR_NET_PLAYER_NAME_IN_USE);
#else
			// If this is a server without password protection, close this new session and return.
			SessionError(session, ERR_NET_PLAYER_NAME_IN_USE);
			return;
#endif
		}
	}

	// Run postlogin for DB
	string tmpAvatarHash;
	string tmpAvatarType;
	if (!session->GetPlayerData()->GetAvatarMD5().IsZero()) {
		tmpAvatarHash = session->GetPlayerData()->GetAvatarMD5().ToString();
		tmpAvatarType = AvatarManager::GetAvatarFileExtension(AvatarManager::GetAvatarFileType(session->GetPlayerData()->GetAvatarFile()));
		if (!tmpAvatarType.empty())
			tmpAvatarType.erase(0, 1); // Only store extension without the "."
	}
	m_database->PlayerPostLogin(session->GetPlayerData()->GetDBId(), tmpAvatarHash, tmpAvatarType);

	// Generate a new GUID.
	boost::uuids::uuid sessionGuid(m_sessionIdGenerator());
	session->GetPlayerData()->SetGuid(string((char *)&sessionGuid, boost::uuids::uuid::static_size()));

	// Send ACK to client.
	boost::shared_ptr<NetPacket> ack(new NetPacket);
	ack->GetMsg()->set_messagetype(PokerTHMessage::Type_InitAckMessage);
	InitAckMessage *netInitAck = ack->GetMsg()->mutable_initackmessage();
	netInitAck->set_yoursessionid(session->GetPlayerData()->GetGuid());
	netInitAck->set_yourplayerid(session->GetPlayerData()->GetUniqueId());
	if (rejoinGameId != 0) {
		netInitAck->set_rejoingameid(rejoinGameId);
	}
	GetSender().Send(session, ack);

	// Send the connected players list to the client.
	SendPlayerList(session);
	// Send the game list to the client.
	SendGameList(session);

	// Session is now established.
	session->SetState(SessionData::Established);

	{
		boost::mutex::scoped_lock lock(m_statMutex);
		++m_statData.totalPlayersEverLoggedIn;
		m_statDataChanged = true;
	}
	// Notify all players.
	NotifyPlayerJoinedLobby(session->GetPlayerData()->GetUniqueId());

	UpdateStatisticsNumberOfPlayers();
}

void
ServerLobbyThread::AuthenticatePlayer(boost::shared_ptr<SessionData> session)
{
	if(session->GetPlayerData()) {
		m_database->AsyncPlayerLogin(session->GetPlayerData()->GetUniqueId(), session->GetPlayerData()->GetName());
	}
}

void
ServerLobbyThread::UserValid(unsigned playerId, const DBPlayerData &dbPlayerData)
{
	boost::shared_ptr<SessionData> tmpSession = m_sessionManager.GetSessionByUniquePlayerId(playerId, true);
	if (tmpSession && tmpSession->GetPlayerData()) {
		tmpSession->GetPlayerData()->SetDBId(dbPlayerData.id);
		tmpSession->GetPlayerData()->SetCountry(dbPlayerData.country);
		this->AuthChallenge(tmpSession, dbPlayerData.secret);
	}
}

void
ServerLobbyThread::UserInvalid(unsigned playerId)
{
	SessionError(m_sessionManager.GetSessionByUniquePlayerId(playerId, true), ERR_NET_INVALID_PASSWORD);
}

void
ServerLobbyThread::SendReportAvatarResult(unsigned byPlayerId, unsigned reportedPlayerId, bool success)
{
	boost::shared_ptr<SessionData> session = m_sessionManager.GetSessionByUniquePlayerId(byPlayerId);
	if (!session)
		session = m_gameSessionManager.GetSessionByUniquePlayerId(byPlayerId);
	if (session) {
		boost::shared_ptr<NetPacket> packet(new NetPacket);
		packet->GetMsg()->set_messagetype(PokerTHMessage::Type_ReportAvatarAckMessage);
		ReportAvatarAckMessage *netReportAck = packet->GetMsg()->mutable_reportavatarackmessage();
		netReportAck->set_reportedplayerid(reportedPlayerId);
		netReportAck->set_reportavatarresult(success ? ReportAvatarAckMessage::avatarReportAccepted : ReportAvatarAckMessage::avatarReportInvalid);
		GetSender().Send(session, packet);
	}
}

void
ServerLobbyThread::SendReportGameResult(unsigned byPlayerId, unsigned reportedGameId, bool success)
{
	boost::shared_ptr<SessionData> session = m_sessionManager.GetSessionByUniquePlayerId(byPlayerId);
	if (!session)
		session = m_gameSessionManager.GetSessionByUniquePlayerId(byPlayerId);
	if (session) {
		boost::shared_ptr<NetPacket> packet(new NetPacket);
		packet->GetMsg()->set_messagetype(PokerTHMessage::Type_ReportGameAckMessage);
		ReportGameAckMessage *netReportAck = packet->GetMsg()->mutable_reportgameackmessage();
		netReportAck->set_reportedgameid(reportedGameId);
		netReportAck->set_reportgameresult(success ? ReportGameAckMessage::gameReportAccepted : ReportGameAckMessage::gameReportInvalid);
		GetSender().Send(session, packet);
	}
}

void
ServerLobbyThread::SendAdminBanPlayerResult(unsigned byPlayerId, unsigned reportedPlayerId, bool success)
{
	boost::shared_ptr<SessionData> session = m_sessionManager.GetSessionByUniquePlayerId(byPlayerId);
	if (!session)
		session = m_gameSessionManager.GetSessionByUniquePlayerId(byPlayerId);
	if (session) {
		boost::shared_ptr<NetPacket> packet(new NetPacket);
		packet->GetMsg()->set_messagetype(PokerTHMessage::Type_AdminBanPlayerAckMessage);
		AdminBanPlayerAckMessage *netBanAck = packet->GetMsg()->mutable_adminbanplayerackmessage();
		netBanAck->set_banplayerid(reportedPlayerId);
		netBanAck->set_banplayerresult(success ? AdminBanPlayerAckMessage::banPlayerAccepted : AdminBanPlayerAckMessage::banPlayerDBError);
		GetSender().Send(session, packet);
	}
}

void
ServerLobbyThread::UserBlocked(unsigned playerId)
{
	SessionError(m_sessionManager.GetSessionByUniquePlayerId(playerId, true), ERR_NET_PLAYER_BLOCKED);
}

void
ServerLobbyThread::RequestPlayerAvatar(boost::shared_ptr<SessionData> session)
{
	if (!session->GetPlayerData())
		throw ServerException(__FILE__, __LINE__, ERR_NET_INVALID_SESSION, 0);
	// Ask the client to send its avatar.
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_AvatarRequestMessage);
	AvatarRequestMessage *netAvatarRequest = packet->GetMsg()->mutable_avatarrequestmessage();
	netAvatarRequest->set_requestid(session->GetPlayerData()->GetUniqueId());
	netAvatarRequest->set_avatarhash(session->GetPlayerData()->GetAvatarMD5().GetData(), MD5_DATA_SIZE);

	GetSender().Send(session, packet);
}

void
ServerLobbyThread::TimerRemoveGame(const boost::system::error_code &ec)
{
	if (!ec) {
		// Synchronously remove games which have been closed.
		GameMap::iterator i = m_gameMap.begin();
		GameMap::iterator end = m_gameMap.end();
		while (i != end) {
			GameMap::iterator next = i;
			++next;
			boost::shared_ptr<ServerGame> tmpGame = i->second;
			if (!tmpGame->GetSessionManager().HasSessionWithState(SessionData::Game)) {
				tmpGame->MoveSpectatorsToLobby();
				InternalRemoveGame(tmpGame); // This will delete the game.
			}
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
ServerLobbyThread::TimerUpdateClientLoginLock(const boost::system::error_code &ec)
{
	if (!ec) {
		boost::mutex::scoped_lock lock(m_timerClientAddressMapMutex);

		TimerClientAddressMap::iterator i = m_timerClientAddressMap.begin();
		TimerClientAddressMap::iterator end = m_timerClientAddressMap.end();

		while (i != end) {
			TimerClientAddressMap::iterator next = i;
			++next;
			if (i->second.elapsed().total_seconds() > (int)SERVER_INIT_LOGIN_CLIENT_LOCK_SEC)
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

bool
ServerLobbyThread::IsGameNameInUse(const std::string &gameName) const
{
	bool found = false;
	GameMap::const_iterator i = m_gameMap.begin();
	GameMap::const_iterator end = m_gameMap.end();

	while (i != end) {
		if ((*i).second->GetName() == gameName) {
			found = true;
			break;
		}
		++i;
	}
	return found;
}

boost::shared_ptr<ServerGame>
ServerLobbyThread::InternalGetGameFromId(unsigned gameId)
{
	boost::shared_ptr<ServerGame> game;
	if (gameId) {
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
	m_gameSessionManager.SendLobbyMsgToAllSessions(GetSender(), CreateNetPacketGameListNew(*game), SessionData::Game | SessionData::Spectating | SessionData::SpectatorWaiting);

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
		if (m_statData.numberOfGamesOpen) {
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
	m_gameSessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Game | SessionData::Spectating | SessionData::SpectatorWaiting);
}

void
ServerLobbyThread::InternalRemovePlayer(unsigned playerId, unsigned errorCode)
{
	boost::shared_ptr<SessionData> session = m_sessionManager.GetSessionByUniquePlayerId(playerId, true);
	if (session)
		SessionError(session, errorCode);
	else {
		// Remove player from game.
		boost::shared_ptr<SessionData> session = m_gameSessionManager.GetSessionByUniquePlayerId(playerId);
		if (session) {
			boost::shared_ptr<ServerGame> tmpGame = session->GetGame();
			if (tmpGame) {
				tmpGame->RemovePlayer(playerId, errorCode);
			}
		}
	}
}

void
ServerLobbyThread::InternalMutePlayerInGame(unsigned playerId)
{
	boost::shared_ptr<SessionData> session = m_gameSessionManager.GetSessionByUniquePlayerId(playerId);
	if (session) {
		boost::shared_ptr<ServerGame> tmpGame = session->GetGame();
		if (tmpGame) {
			tmpGame->MutePlayer(playerId, true);
		}
	}
}

void
ServerLobbyThread::InternalResubscribeMsg(boost::shared_ptr<SessionData> session)
{
	if (!session->WantsLobbyMsg()) {
		session->SetWantsLobbyMsg();
		SendPlayerList(session);
		SendGameList(session);
		// Send new statistics information.
		/*		boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
				packet->GetMsg()->present = PokerTHMessage_PR_statisticsMessage;
				StatisticsMessage_t *netStatistics = &packet->GetMsg()->choice.statisticsMessage;

				StatisticsData_t *data = (StatisticsData_t *)calloc(1, sizeof(struct StatisticsData));
				data->statisticsType = statisticsType_statNumberOfPlayers;
				data->statisticsValue = m_sessionManager.GetRawSessionCount() + m_gameSessionManager.GetRawSessionCount();
				ASN_SEQUENCE_ADD(&netStatistics->statisticsData.list, data);

				GetSender().Send(session, packet);*/
	}
}

void
ServerLobbyThread::HandleReAddedSession(boost::shared_ptr<SessionData> session)
{
	// Remove session from game session list.
	m_gameSessionManager.RemoveSession(session->GetId());

	if (m_sessionManager.GetRawSessionCount() <= SERVER_MAX_NUM_LOBBY_SESSIONS) {
		// Set state (back) to established.
		session->SetState(SessionData::Established);
		session->SetGame(boost::shared_ptr<ServerGame>());
		// Add session to lobby list.
		m_sessionManager.AddSession(session);
	} else {
		// Gracefully close this session.
		SessionError(session, ERR_NET_SERVER_FULL);
	}
}

void
ServerLobbyThread::SessionTimeoutWarning(boost::shared_ptr<SessionData> session, unsigned remainingSec)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_TimeoutWarningMessage);
	TimeoutWarningMessage *netWarning = packet->GetMsg()->mutable_timeoutwarningmessage();
	netWarning->set_timeoutreason(TimeoutWarningMessage::timeoutNoDataReceived);
	netWarning->set_remainingseconds(remainingSec);
	GetSender().Send(session, packet);

	if (session->GetGame() && session->GetPlayerData()) {
		session->GetGame()->MarkPlayerAsInactive(session->GetPlayerData()->GetUniqueId());
	}
}

void
ServerLobbyThread::SessionError(boost::shared_ptr<SessionData> session, int errorCode)
{
	if (session) {
		if (errorCode == ERR_NET_PLAYER_KICKED || errorCode == ERR_NET_SESSION_TIMED_OUT) {
			if (session->GetGame() && session->GetPlayerData()) {
				session->GetGame()->MarkPlayerAsKicked(session->GetPlayerData()->GetUniqueId());
			}
		}

		SendError(session, errorCode);
		CloseSession(session);
	}
}

void
ServerLobbyThread::SendError(boost::shared_ptr<SessionData> s, int errorCode)
{
	LOG_VERBOSE("Sending error code " << errorCode << " to session #" << s->GetId() << ".");
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_ErrorMessage);
	ErrorMessage *netError = packet->GetMsg()->mutable_errormessage();
	netError->set_errorreason(NetPacket::GameErrorToNetError(errorCode));
	GetSender().Send(s, packet);
}

void
ServerLobbyThread::SendJoinGameFailed(boost::shared_ptr<SessionData> s, unsigned gameId, int reason)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_JoinGameFailedMessage);
	JoinGameFailedMessage *netJoinFailed = packet->GetMsg()->mutable_joingamefailedmessage();
	netJoinFailed->set_gameid(gameId);

	switch (reason) {
	case NTF_NET_JOIN_GAME_FULL :
		netJoinFailed->set_joingamefailurereason(JoinGameFailedMessage::gameIsFull);
		break;
	case NTF_NET_JOIN_ALREADY_RUNNING :
		netJoinFailed->set_joingamefailurereason(JoinGameFailedMessage::gameIsRunning);
		break;
	case NTF_NET_JOIN_INVALID_PASSWORD :
		netJoinFailed->set_joingamefailurereason(JoinGameFailedMessage::invalidPassword);
		break;
	case NTF_NET_JOIN_GUEST_FORBIDDEN :
		netJoinFailed->set_joingamefailurereason(JoinGameFailedMessage::notAllowedAsGuest);
		break;
	case NTF_NET_JOIN_NOT_INVITED :
		netJoinFailed->set_joingamefailurereason(JoinGameFailedMessage::notInvited);
		break;
	case NTF_NET_JOIN_GAME_NAME_IN_USE :
		netJoinFailed->set_joingamefailurereason(JoinGameFailedMessage::gameNameInUse);
		break;
	case NTF_NET_JOIN_GAME_BAD_NAME :
		netJoinFailed->set_joingamefailurereason(JoinGameFailedMessage::badGameName);
		break;
	case NTF_NET_JOIN_INVALID_SETTINGS :
		netJoinFailed->set_joingamefailurereason(JoinGameFailedMessage::invalidSettings);
		break;
	case NTF_NET_JOIN_IP_BLOCKED :
		netJoinFailed->set_joingamefailurereason(JoinGameFailedMessage::ipAddressBlocked);
		break;
	case NTF_NET_JOIN_REJOIN_FAILED :
		netJoinFailed->set_joingamefailurereason(JoinGameFailedMessage::rejoinFailed);
		break;
	case NTF_NET_JOIN_NO_SPECTATORS :
		netJoinFailed->set_joingamefailurereason(JoinGameFailedMessage::noSpectatorsAllowed);
		break;
	default :
		netJoinFailed->set_joingamefailurereason(JoinGameFailedMessage::invalidGame);
		break;
	}
	GetSender().Send(s, packet);
}

void
ServerLobbyThread::SendPlayerList(boost::shared_ptr<SessionData> s)
{
	// Retrieve all player ids.
	PlayerIdList idList(m_sessionManager.GetPlayerIdList(SessionData::Established));
	PlayerIdList gameIdList(m_gameSessionManager.GetPlayerIdList(SessionData::Game | SessionData::Spectating | SessionData::SpectatorWaiting));
	idList.splice(idList.begin(), gameIdList);
	// Send all player ids to client.
	PlayerIdList::const_iterator i = idList.begin();
	PlayerIdList::const_iterator end = idList.end();
	while (i != end) {
		GetSender().Send(s, CreateNetPacketPlayerListNew(*i));
		++i;
	}
}

void
ServerLobbyThread::SendGameList(boost::shared_ptr<SessionData> s)
{
	GameMap::const_iterator game_i = m_gameMap.begin();
	GameMap::const_iterator game_end = m_gameMap.end();
	while (game_i != game_end) {
		GetSender().Send(s, CreateNetPacketGameListNew(*game_i->second));
		++game_i;
	}
}

void
ServerLobbyThread::UpdateStatisticsNumberOfPlayers()
{
	ServerStats stats;
	// Get all logged-in sessions and all sessions within a game.
	unsigned curNumberOfPlayersOnServer = m_sessionManager.GetSessionCountWithState(SessionData::Established) + m_gameSessionManager.GetRawSessionCount();
	{
		boost::mutex::scoped_lock lock(m_statMutex);
		if (curNumberOfPlayersOnServer != m_statData.numberOfPlayersOnServer) {
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
	if (stats.numberOfPlayersOnServer) {
		boost::shared_ptr<NetPacket> packet(new NetPacket);
		packet->GetMsg()->set_messagetype(PokerTHMessage::Type_StatisticsMessage);
		StatisticsMessage *netStatistics = packet->GetMsg()->mutable_statisticsmessage();

		StatisticsMessage::StatisticsData *data = netStatistics->add_statisticsdata();
		data->set_statisticstype(StatisticsMessage::StatisticsData::statNumberOfPlayers);
		data->set_statisticsvalue(m_sessionManager.GetRawSessionCount() + m_gameSessionManager.GetRawSessionCount());

		m_sessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Established);
		m_gameSessionManager.SendLobbyMsgToAllSessions(GetSender(), packet, SessionData::Game | SessionData::Spectating | SessionData::SpectatorWaiting);
	}
}

void
ServerLobbyThread::ReadStatisticsFile()
{
	ifstream i(m_statisticsFileName.c_str(), ios_base::in);

	if (!i.fail() && !i.eof()) {
		boost::mutex::scoped_lock lock(m_statMutex);
		do {
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
	if (!ec) {
		LOG_VERBOSE("Saving statistics.");
		boost::mutex::scoped_lock lock(m_statMutex);
		if (m_statDataChanged) {
			ofstream o(m_statisticsFileName.c_str(), ios_base::out | ios_base::trunc);
			if (!o.fail()) {
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

ServerIrcBotCallback &
ServerLobbyThread::GetIrcBotCallback()
{
	return m_ircBotCb;
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

unsigned
ServerLobbyThread::GetPlayerId(const string &name) const
{
	unsigned playerId = 0;

	boost::shared_ptr<SessionData> tmpSession(m_sessionManager.GetSessionByPlayerName(name));

	if (!tmpSession)
		tmpSession = m_gameSessionManager.GetSessionByPlayerName(name);

	if (tmpSession && tmpSession->GetPlayerData())
		playerId = tmpSession->GetPlayerData()->GetUniqueId();

	return playerId;
}

boost::shared_ptr<NetPacket>
ServerLobbyThread::CreateNetPacketPlayerListNew(unsigned playerId)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_PlayerListMessage);
	PlayerListMessage *netPlayerList = packet->GetMsg()->mutable_playerlistmessage();
	netPlayerList->set_playerid(playerId);
	netPlayerList->set_playerlistnotification(PlayerListMessage::playerListNew);
	return packet;
}

boost::shared_ptr<NetPacket>
ServerLobbyThread::CreateNetPacketPlayerListLeft(unsigned playerId)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_PlayerListMessage);
	PlayerListMessage *netPlayerList = packet->GetMsg()->mutable_playerlistmessage();
	netPlayerList->set_playerid(playerId);
	netPlayerList->set_playerlistnotification(PlayerListMessage::playerListLeft);
	return packet;
}

boost::shared_ptr<NetPacket>
ServerLobbyThread::CreateNetPacketGameListNew(const ServerGame &game)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_GameListNewMessage);
	GameListNewMessage *netGameList = packet->GetMsg()->mutable_gamelistnewmessage();
	netGameList->set_gameid(game.GetId());
	netGameList->set_adminplayerid(game.GetAdminPlayerId());
	netGameList->set_gamemode(game.IsRunning() ? netGameStarted : netGameCreated);
	NetPacket::SetGameData(game.GetGameData(), *netGameList->mutable_gameinfo());
	netGameList->mutable_gameinfo()->set_gamename(game.GetName());
	netGameList->set_isprivate(game.IsPasswordProtected());

	PlayerIdList tmpList = game.GetPlayerIdList();
	PlayerIdList::const_iterator i = tmpList.begin();
	PlayerIdList::const_iterator end = tmpList.end();
	while (i != end) {
		netGameList->add_playerids(*i);
		++i;
	}

	tmpList = game.GetSpectatorIdList();
	i = tmpList.begin();
	end = tmpList.end();
	while (i != end) {
		netGameList->add_spectatorids(*i);
		++i;
	}

	return packet;
}

boost::shared_ptr<NetPacket>
ServerLobbyThread::CreateNetPacketGameListUpdate(unsigned gameId, GameMode mode)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_GameListUpdateMessage);
	GameListUpdateMessage *netGameList = packet->GetMsg()->mutable_gamelistupdatemessage();
	netGameList->set_gameid(gameId);
	netGameList->set_gamemode(static_cast<NetGameMode>(mode));
	return packet;
}

u_int32_t
ServerLobbyThread::GetRejoinGameIdForPlayer(const std::string &playerName, const std::string &guid, unsigned &outPlayerUniqueId)
{
	u_int32_t retGameId = 0;
	if (!guid.empty()) {
		GameMap::iterator i = m_gameMap.begin();
		GameMap::iterator end = m_gameMap.end();
		while (i != end) {
			boost::shared_ptr<ServerGame> tmpGame = i->second;
			boost::shared_ptr<PlayerInterface> tmpPlayer = tmpGame->GetPlayerInterfaceFromGame(playerName);
			if (tmpPlayer && tmpPlayer->getMyGuid() == guid && tmpPlayer->getMyCash() > 0) {
				retGameId = tmpGame->GetId();
				outPlayerUniqueId = tmpPlayer->getMyUniqueID();
				break;
			}
			++i;
		}
	}
	return retGameId;
}

