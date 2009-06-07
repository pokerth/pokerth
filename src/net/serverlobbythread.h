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

#include <boost/asio.hpp>
#include <deque>
#include <boost/regex.hpp>

#include <core/timermanager.h>
#include <net/sessionmanager.h>
#include <net/netpacket.h>
#include <gui/guiinterface.h>
#include <gamedata.h>


#define NET_LOBBY_THREAD_TERMINATE_TIMEOUT_MSEC		20000
#define NET_ADMIN_IRC_TERMINATE_TIMEOUT_MSEC		4000


class SenderHelper;
class ReceiverHelper;
class ServerSenderCallback;
class ServerGame;
class ConfigFile;
class AvatarManager;
struct GameData;
class Game;

class ServerLobbyThread : public Thread
{
public:
	ServerLobbyThread(GuiInterface &gui, ConfigFile *playerConfig, AvatarManager &avatarManager,
		boost::shared_ptr<boost::asio::io_service> ioService);
	virtual ~ServerLobbyThread();

	void Init(const std::string &pwd, const std::string &logDir);
	virtual void SignalTermination();

	void AddConnection(boost::shared_ptr<boost::asio::ip::tcp::socket> sock);
	void ReAddSession(SessionWrapper session, int reason);
	void MoveSessionToGame(ServerGame &game, SessionWrapper session);
	void RemoveSessionFromGame(SessionWrapper session);
	void SessionError(SessionWrapper session, int errorCode);
	void ResubscribeLobbyMsg(SessionWrapper session);
	void NotifyPlayerJoinedGame(unsigned gameId, unsigned playerId);
	void NotifyPlayerLeftGame(unsigned gameId, unsigned playerId);
	void NotifyGameAdminChanged(unsigned gameId, unsigned newAdminPlayerId);
	void NotifyStartingGame(unsigned gameId);
	void NotifyReopeningGame(unsigned gameId);

	void HandleGameRetrievePlayerInfo(SessionWrapper session, const NetPacketRetrievePlayerInfo &tmpPacket);
	void HandleGameRetrieveAvatar(SessionWrapper session, const NetPacketRetrieveAvatar &tmpPacket);

	bool KickPlayerByName(const std::string &playerName);
	void BanPlayerRegex(const std::string &playerRegex);
	void BanIPAddress(const std::string &ipAddress);
	bool UnBan(unsigned banId);
	void GetBanList(std::list<std::string> &list) const;
	void ClearBanList();
	std::string GetPlayerIPAddress(const std::string &playerName) const;
	void RemovePlayer(unsigned playerId, unsigned errorCode);

	void SendGlobalChat(const std::string &message);
	void SendGlobalMsgBox(const std::string &message);

	void AddComputerPlayer(boost::shared_ptr<PlayerData> player);
	void RemoveComputerPlayer(boost::shared_ptr<PlayerData> player);

	u_int32_t GetNextUniquePlayerId();
	u_int32_t GetNextGameId();
	ServerCallback &GetCallback();

	TimerManager &GetTimerManager();
	AvatarManager &GetAvatarManager();

	ServerStats GetStats() const;
	boost::posix_time::ptime GetStartTime() const;

	SenderHelper &GetSender();

protected:

	typedef std::deque<boost::shared_ptr<boost::asio::ip::tcp::socket> > ConnectQueue;
	typedef std::deque<SessionWrapper> SessionQueue;
	typedef std::list<SessionWrapper> SessionList;
	typedef std::list<SessionId> SessionIdList;
	typedef std::map<SessionId, boost::timers::portable::microsec_timer> TimerSessionMap;
	typedef std::map<unsigned, boost::shared_ptr<ServerGame> > GameMap;
	typedef std::map<std::string, boost::timers::portable::microsec_timer> TimerClientAddressMap;
	typedef std::list<unsigned> RemoveGameList;
	typedef std::map<unsigned, boost::regex> RegexMap;
	typedef std::map<unsigned, std::string> IPAddressMap;

	// Main function of the thread.
	virtual void Main();
	void RegisterTimers();

	void HandleRead(SessionId sessionId, const boost::system::error_code& error, size_t bytesRead);
	void HandlePacket(SessionWrapper session, boost::shared_ptr<NetPacket> packet);
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
	void TimerRemoveGame();
	void TimerRemovePlayer();
	void TimerUpdateClientAvatarLock();
	void TimerCheckSessionTimeouts();
	void TimerCleanupAvatarCache();

	boost::shared_ptr<ServerGame> InternalGetGameFromId(unsigned gameId);
	void InternalAddGame(boost::shared_ptr<ServerGame> game);
	void InternalRemoveGame(boost::shared_ptr<ServerGame> game);
	void InternalRemovePlayer(unsigned playerId, unsigned errorCode);
	void InternalResubscribeMsg(SessionWrapper session);

	void HandleReAddedSession(SessionWrapper session);

	void InternalCheckSessionTimeouts(SessionWrapper session);

	void CleanupSessionMap();

	void CloseSession(SessionWrapper session);
	void SendError(boost::shared_ptr<SessionData> s, int errorCode);
	void SendJoinGameFailed(boost::shared_ptr<SessionData> s, int reason);
	void SendGameList(boost::shared_ptr<SessionData> s);
	void UpdateStatisticsNumberOfPlayers();
	void BroadcastStatisticsUpdate(const ServerStats &stats);

	void ReadStatisticsFile();
	void TimerSaveStatisticsFile();

	ReceiverHelper &GetReceiver();

	bool CheckPassword(const std::string &password) const;

	ServerSenderCallback &GetSenderCallback();
	GuiInterface &GetGui();

	bool IsPlayerConnected(const std::string &name) const;
	bool IsPlayerBanned(const std::string &name) const;
	bool IsIPAddressBanned(const std::string &ipAddress) const;

	static boost::shared_ptr<NetPacket> CreateNetPacketGameListNew(const ServerGame &game);
	static boost::shared_ptr<NetPacket> CreateNetPacketGameListUpdate(unsigned gameId, GameMode mode);

private:

	boost::shared_ptr<boost::asio::io_service> m_ioService;
	boost::shared_ptr<boost::asio::io_service::work> m_work;

	boost::shared_ptr<ServerSenderCallback> m_senderCallback;
	boost::shared_ptr<SenderHelper> m_sender;
	boost::shared_ptr<ReceiverHelper> m_receiver;

	SessionManager m_sessionManager;
	SessionManager m_gameSessionManager;
	TimerManager m_timerManager;

	TimerClientAddressMap m_timerAvatarClientAddressMap;
	mutable boost::mutex m_timerAvatarClientAddressMapMutex;

	RemoveGameList m_removeGameList;
	mutable boost::mutex m_removeGameListMutex;

	RemovePlayerList m_removePlayerList;
	mutable boost::mutex m_removePlayerListMutex;

	PlayerDataMap m_computerPlayers;
	mutable boost::mutex m_computerPlayersMutex;

	RegexMap m_banPlayerNameMap;
	IPAddressMap m_banIPAddressMap;
	unsigned m_curBanId;
	mutable boost::mutex m_banMutex;

	GameMap m_gameMap;

	GuiInterface &m_gui;
	AvatarManager &m_avatarManager;

	std::string m_password;
	std::string m_statisticsFileName;
	ConfigFile *m_playerConfig;
	u_int32_t m_curGameId;

	u_int32_t m_curUniquePlayerId;
	u_int32_t m_curSessionId;
	mutable boost::mutex m_curUniquePlayerIdMutex;

	ServerStats m_statData;
	bool m_statDataChanged;
	mutable boost::mutex m_statMutex;

	const boost::posix_time::ptime m_startTime;
};

#endif
