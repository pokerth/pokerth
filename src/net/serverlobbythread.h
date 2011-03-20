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
/* Network server lobby thread. */

#ifndef _SERVERLOBBYTHREAD_H_
#define _SERVERLOBBYTHREAD_H_

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <net/sessionmanager.h>
#include <net/netpacket.h>
#include <db/serverdbcallback.h>
#include <gui/guiinterface.h>
#include <gamedata.h>

class Log;

#define NET_LOBBY_THREAD_TERMINATE_TIMEOUT_MSEC		20000
#define NET_ADMIN_IRC_TERMINATE_TIMEOUT_MSEC		4000


class SenderHelper;
class InternalServerCallback;
class ServerIrcBotCallback;
class ServerGame;
class ServerBanManager;
class ConfigFile;
class AvatarManager;
class ChatCleanerManager;
class ServerDBInterface;
struct GameData;
class Game;
struct Gsasl;

class ServerLobbyThread : public Thread, public boost::enable_shared_from_this<ServerLobbyThread>
{
public:
	ServerLobbyThread(GuiInterface &gui, ServerMode mode, ServerIrcBotCallback &ircBotCb, ConfigFile &serverConfig, AvatarManager &avatarManager,
					  boost::shared_ptr<boost::asio::io_service> ioService);
	virtual ~ServerLobbyThread();

	void Init(const std::string &logDir);
	virtual void SignalTermination();

	void AddConnection(boost::shared_ptr<boost::asio::ip::tcp::socket> sock);
	void ReAddSession(boost::shared_ptr<SessionData> session, int reason, unsigned gameId);
	void MoveSessionToGame(boost::shared_ptr<ServerGame> game, boost::shared_ptr<SessionData> session, bool autoLeave);
	void RemoveSessionFromGame(boost::shared_ptr<SessionData> session);
	void SessionError(boost::shared_ptr<SessionData> session, int errorCode);
	void ResubscribeLobbyMsg(boost::shared_ptr<SessionData> session);
	void NotifyPlayerJoinedLobby(unsigned playerId);
	void NotifyPlayerLeftLobby(unsigned playerId);
	void NotifyPlayerJoinedGame(unsigned gameId, unsigned playerId);
	void NotifyPlayerLeftGame(unsigned gameId, unsigned playerId);
	void NotifyGameAdminChanged(unsigned gameId, unsigned newAdminPlayerId);
	void NotifyStartingGame(unsigned gameId);
	void NotifyReopeningGame(unsigned gameId);

	void DispatchPacket(boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet);
	void HandleGameRetrievePlayerInfo(boost::shared_ptr<SessionData> session, const PlayerInfoRequestMessage_t &playerInfoRequest);
	void HandleGameRetrieveAvatar(boost::shared_ptr<SessionData> session, const AvatarRequestMessage_t &retrieveAvatar);
	void HandleChatRequest(boost::shared_ptr<SessionData> session, const ChatRequestMessage_t &chatRequest);

	bool KickPlayerByName(const std::string &playerName);
	std::string GetPlayerIPAddress(const std::string &playerName) const;
	std::string GetPlayerNameFromId(unsigned playerId) const;
	void RemovePlayer(unsigned playerId, unsigned errorCode);

	void SendGlobalChat(const std::string &message);
	void SendGlobalMsgBox(const std::string &message);
	void SendChatBotMsg(const std::string &message);
	void SendChatBotMsg(unsigned gameId, const std::string &message);
	void ReconnectChatBot();

	void AddComputerPlayer(boost::shared_ptr<PlayerData> player);
	void RemoveComputerPlayer(boost::shared_ptr<PlayerData> player);

	bool SendToLobbyPlayer(unsigned playerId, boost::shared_ptr<NetPacket> packet);

	u_int32_t GetNextUniquePlayerId();
	u_int32_t GetNextGameId();
	ServerCallback &GetCallback();

	AvatarManager &GetAvatarManager();
	ChatCleanerManager &GetChatCleaner();

	ServerStats GetStats() const;
	boost::posix_time::ptime GetStartTime() const;
	ServerMode GetServerMode() const;

	SenderHelper &GetSender();
	boost::asio::io_service &GetIOService();
	boost::shared_ptr<ServerDBInterface> GetDatabase();
	ServerBanManager &GetBanManager();

protected:

	typedef std::deque<boost::shared_ptr<boost::asio::ip::tcp::socket> > ConnectQueue;
	typedef std::list<boost::shared_ptr<SessionData> > SessionList;
	typedef std::list<SessionId> SessionIdList;
	typedef std::map<SessionId, boost::timers::portable::microsec_timer> TimerSessionMap;
	typedef std::map<unsigned, boost::shared_ptr<ServerGame> > GameMap;
	typedef std::map<std::string, boost::timers::portable::microsec_timer> TimerClientAddressMap;
	typedef std::list<unsigned> RemoveGameList;

	// Main function of the thread.
	virtual void Main();
	void RegisterTimers();
	void CancelTimers();
	void InitAuthContext();
	void ClearAuthContext();
	void InitChatCleaner();

	void HandlePacket(boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet);
	void HandleNetPacketInit(boost::shared_ptr<SessionData> session, const InitMessage_t &initMessage);
	void HandleNetPacketAuthClientResponse(boost::shared_ptr<SessionData> session, const AuthClientResponse_t &clientResponse);
	void HandleNetPacketAvatarHeader(boost::shared_ptr<SessionData> session, unsigned requestId, const AvatarHeader_t &avatarHeader);
	void HandleNetPacketUnknownAvatar(boost::shared_ptr<SessionData> session, unsigned requestId, const UnknownAvatar_t &unknownAvatar);
	void HandleNetPacketAvatarFile(boost::shared_ptr<SessionData> session, unsigned requestId, const AvatarData_t &avatarData);
	void HandleNetPacketAvatarEnd(boost::shared_ptr<SessionData> session, unsigned requestId, const AvatarEnd_t &avatarEnd);
	void HandleNetPacketRetrievePlayerInfo(boost::shared_ptr<SessionData> session, const PlayerInfoRequestMessage_t &playerInfoRequest);
	void HandleNetPacketRetrieveAvatar(boost::shared_ptr<SessionData> session, const AvatarRequestMessage_t &retrieveAvatar);
	void HandleNetPacketCreateGame(boost::shared_ptr<SessionData> session, const std::string &password, bool autoLeave, const JoinNewGame_t &newGame);
	void HandleNetPacketJoinGame(boost::shared_ptr<SessionData> session, const std::string &password, bool autoLeave, const JoinExistingGame_t &joinGame);
	void HandleNetPacketChatRequest(boost::shared_ptr<SessionData> session, const ChatRequestMessage_t &chatRequest);
	void HandleNetPacketRejectGameInvitation(boost::shared_ptr<SessionData> session, const RejectGameInvitationMessage_t &reject);
	// TODO would be better to use state pattern here.
	void AuthChallenge(boost::shared_ptr<SessionData> session, const std::string &secret);
	void CheckAvatarBlacklist(boost::shared_ptr<SessionData> session);
	void AvatarBlacklisted(unsigned playerId);
	void AvatarOK(unsigned playerId);
	void InitAfterLogin(boost::shared_ptr<SessionData> session);
	void EstablishSession(boost::shared_ptr<SessionData> session);
	void AuthenticatePlayer(boost::shared_ptr<SessionData> session);
	void UserValid(unsigned playerId, const DBPlayerData &dbPlayerData);
	void UserInvalid(unsigned playerId);
	void UserBlocked(unsigned playerId);

	void SendReportAvatarResult(unsigned byPlayerId, unsigned reportedPlayerId, bool success);
	void RequestPlayerAvatar(boost::shared_ptr<SessionData> session);
	void TimerRemoveGame(const boost::system::error_code &ec);
	void TimerRemovePlayer(const boost::system::error_code &ec);
	void TimerUpdateClientLoginLock(const boost::system::error_code &ec);
	void TimerCheckSessionTimeouts(const boost::system::error_code &ec);
	void TimerCleanupAvatarCache(const boost::system::error_code &ec);

	bool IsGameNameInUse(const std::string &gameName) const;
	boost::shared_ptr<ServerGame> InternalGetGameFromId(unsigned gameId);
	void InternalAddGame(boost::shared_ptr<ServerGame> game);
	void InternalRemoveGame(boost::shared_ptr<ServerGame> game);
	void InternalRemovePlayer(unsigned playerId, unsigned errorCode);
	void InternalResubscribeMsg(boost::shared_ptr<SessionData> session);

	void HandleReAddedSession(boost::shared_ptr<SessionData> session);

	void InternalCheckSessionTimeouts(boost::shared_ptr<SessionData> session);

	void CleanupSessionMap();

	void CloseSession(SessionId sessionId);
	void CloseSession(boost::shared_ptr<SessionData> session);
	void SendError(boost::shared_ptr<SessionData> s, int errorCode);
	void SendJoinGameFailed(boost::shared_ptr<SessionData> s, unsigned gameId, int reason);
	void SendPlayerList(boost::shared_ptr<SessionData> s);
	void SendGameList(boost::shared_ptr<SessionData> s);
	void UpdateStatisticsNumberOfPlayers();
	void BroadcastStatisticsUpdate(const ServerStats &stats);

	void ReadStatisticsFile();
	void TimerSaveStatisticsFile(const boost::system::error_code &ec);

	InternalServerCallback &GetSenderCallback();
	GuiInterface &GetGui();
	ServerIrcBotCallback &GetIrcBotCallback();

	bool IsPlayerConnected(const std::string &name) const;

	static boost::shared_ptr<NetPacket> CreateNetPacketPlayerListNew(unsigned playerId);
	static boost::shared_ptr<NetPacket> CreateNetPacketPlayerListLeft(unsigned playerId);
	static boost::shared_ptr<NetPacket> CreateNetPacketGameListNew(const ServerGame &game);
	static boost::shared_ptr<NetPacket> CreateNetPacketGameListUpdate(unsigned gameId, GameMode mode);

private:

	boost::shared_ptr<boost::asio::io_service> m_ioService;

	boost::shared_ptr<InternalServerCallback> m_internalServerCallback;
	boost::shared_ptr<SenderHelper> m_sender;

	SessionManager m_sessionManager;
	SessionManager m_gameSessionManager;

	Gsasl *m_authContext;

	TimerClientAddressMap m_timerClientAddressMap;
	mutable boost::mutex m_timerClientAddressMapMutex;

	RemoveGameList m_removeGameList;
	mutable boost::mutex m_removeGameListMutex;

	RemovePlayerList m_removePlayerList;
	mutable boost::mutex m_removePlayerListMutex;

	PlayerDataMap m_computerPlayers;
	mutable boost::mutex m_computerPlayersMutex;

	GameMap m_gameMap;

	GuiInterface &m_gui;
	ServerIrcBotCallback &m_ircBotCb;
	AvatarManager &m_avatarManager;

	const ServerMode m_mode;
	std::string m_statisticsFileName;
	ConfigFile &m_serverConfig;
	u_int32_t m_curGameId;

	u_int32_t m_curUniquePlayerId;
	u_int32_t m_curSessionId;
	mutable boost::mutex m_curUniquePlayerIdMutex;

	boost::shared_ptr<Log> m_serverLog;

	ServerStats m_statData;
	bool m_statDataChanged;
	mutable boost::mutex m_statMutex;

	boost::shared_ptr<ServerBanManager> m_banManager;
	boost::shared_ptr<ChatCleanerManager> m_chatCleanerManager;
	boost::shared_ptr<ServerDBInterface> m_database;

	boost::asio::deadline_timer m_removeGameTimer;
	boost::asio::deadline_timer m_removePlayerTimer;
	boost::asio::deadline_timer m_sessionTimeoutTimer;
	boost::asio::deadline_timer m_avatarCleanupTimer;
	boost::asio::deadline_timer m_saveStatisticsTimer;
	boost::asio::deadline_timer m_loginLockTimer;

	boost::uuids::random_generator m_sessionIdGenerator;

	const boost::posix_time::ptime m_startTime;

	friend class InternalServerCallback;
};

#endif
