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
/* Network server game. */

#ifndef _SERVERGAME_H_
#define _SERVERGAME_H_

#include <boost/enable_shared_from_this.hpp>
#include <third_party/boost/timers.hpp>
#include <map>

#include <net/sessionmanager.h>
#include <db/serverdbcallback.h>
#include <gui/guiinterface.h>
#include <gamedata.h>


class ReceiverHelper;
class ServerLobbyThread;
class ServerGameState;
class ServerDBInterface;
class ConfigFile;
struct GameData;
class Game;

class ServerGame : public boost::enable_shared_from_this<ServerGame>
{
public:
	ServerGame(
		boost::shared_ptr<ServerLobbyThread> lobbyThread, u_int32_t id, const std::string &name, const std::string &pwd, const GameData &gameData, unsigned adminPlayerId, GuiInterface &gui, ConfigFile *playerConfig);
	virtual ~ServerGame();

	void Init();
	void Exit();

	u_int32_t GetId() const;
	const std::string &GetName() const;

	DB_id GetDBId() const;
	void SetDBId(DB_id newId);

	void AddSession(SessionWrapper session);
	void RemovePlayer(unsigned playerId, unsigned errorCode);

	void HandlePacket(SessionWrapper session, boost::shared_ptr<NetPacket> packet);

	ServerCallback &GetCallback();
	GameState GetCurRound() const;

	void SendToAllPlayers(boost::shared_ptr<NetPacket> packet, SessionData::State state);
	void SendToAllButOnePlayers(boost::shared_ptr<NetPacket> packet, SessionId except, SessionData::State state);
	void RemoveAllSessions();

	bool IsPasswordProtected() const;
	bool CheckPassword(const std::string &password) const;
	static bool CheckSettings(const GameData &data, const std::string &password);
	const GameData &GetGameData() const;

	boost::shared_ptr<PlayerData> GetPlayerDataByUniqueId(unsigned playerId) const;
	PlayerIdList GetPlayerIdList() const;
	bool IsPlayerConnected(const std::string &name) const;
	bool IsPlayerConnected(unsigned playerId) const;
	bool IsClientAddressConnected(const std::string &clientAddress) const;

	bool IsRunning() const;

	unsigned GetAdminPlayerId() const;
	void SetAdminPlayerId(unsigned playerId);

	void AddPlayerInvitation(unsigned playerId);
	void RemovePlayerInvitation(unsigned playerId);
	bool IsPlayerInvited(unsigned playerId) const;

	unsigned GetSmallDelaySec() const;

	// should be protected, but is needed in function.
	const Game &GetGame() const;
	Game &GetGame();

protected:

	struct RankingData
	{
		RankingData() : dbid(DB_ID_INVALID), place(0) {}
		RankingData(DB_id i, int p = 0) : dbid(i), place(p) {}
		DB_id dbid;
		int place;
	};

	typedef std::map<unsigned, RankingData> RankingMap;

	void TimerVoteKick(const boost::system::error_code &ec);

	PlayerDataList InternalStartGame();
	void InitRankingMap(const PlayerDataList &playerDataList);
	void UpdateRankingMap();
	void SetPlayerPlace(unsigned playerId, int place);
	void InternalEndGame();

	void InternalKickPlayer(unsigned playerId);
	void InternalAskVoteKick(SessionWrapper byWhom, unsigned playerIdWho, unsigned timeoutSec);
	void InternalDenyAskVoteKick(SessionWrapper byWhom, unsigned playerIdWho, DenyKickPlayerReason reason);
	void InternalVoteKick(SessionWrapper byWhom, unsigned petitionId, KickVote vote);
	void InternalDenyVoteKick(SessionWrapper byWhom, unsigned petitionId, DenyVoteReason reason);

	PlayerDataList GetFullPlayerDataList() const;

	void AddComputerPlayer(boost::shared_ptr<PlayerData> player);
	boost::shared_ptr<PlayerData> RemoveComputerPlayer(unsigned playerId);
	bool IsComputerPlayerActive(unsigned playerId) const;
	void ResetComputerPlayerList();

	void GracefulRemoveSession(SessionWrapper session, int reason);
	void RemovePlayerData(boost::shared_ptr<PlayerData> player, int reason);
	void ErrorRemoveSession(SessionWrapper session);
	void SessionError(SessionWrapper session, int errorCode);
	void MoveSessionToLobby(SessionWrapper session, int reason);

	void RemoveDisconnectedPlayers();
	size_t GetCurNumberOfPlayers() const;
	void AssignPlayerNumbers(PlayerDataList &playerList);
	bool IsValidPlayer(unsigned playerId) const;

	ServerLobbyThread &GetLobbyThread();

	ServerGameState &GetState();
	void SetState(ServerGameState &newState);

	boost::asio::deadline_timer &GetStateTimer1();
	boost::asio::deadline_timer &GetStateTimer2();

	ReceiverHelper &GetReceiver();

	const StartData &GetStartData() const;
	void SetStartData(const StartData &startData);

	GuiInterface &GetGui();

	unsigned GetNextGameNum();

	const SessionManager &GetSessionManager() const;
	SessionManager &GetSessionManager();
	ServerDBInterface &GetDatabase();

private:
	ServerGame(const ServerGame &other);

	SessionManager m_sessionManager;
	PlayerDataList m_computerPlayerList;
	mutable boost::mutex m_computerPlayerListMutex;

	PlayerIdList m_playerInvitationList;
	mutable boost::mutex m_playerInvitationListMutex;

	RankingMap m_rankingMap;

	unsigned m_adminPlayerId;

	boost::shared_ptr<VoteKickData> m_voteKickData;

	boost::shared_ptr<ServerLobbyThread> m_lobbyThread;
	boost::shared_ptr<ReceiverHelper> m_receiver;
	boost::shared_ptr<ServerDBInterface> m_database;
	GuiInterface &m_gui;

	const GameData		m_gameData;
	StartData			m_startData;
	boost::shared_ptr<Game>	 m_game;
	ServerGameState			*m_curState;

	const u_int32_t		m_id;
	DB_id				m_dbId;
	const std::string	m_name;
	const std::string	m_password;
	ConfigFile		   *m_playerConfig;
	unsigned			m_gameNum;
	unsigned			m_curPetitionId;
	unsigned			m_doNotAutoKickSmallDelaySec;
	boost::asio::deadline_timer m_voteKickTimer;
	boost::asio::deadline_timer m_stateTimer1;
	boost::asio::deadline_timer m_stateTimer2;

friend class ServerLobbyThread;
friend class AbstractServerGameStateReceiving;
friend class ServerGameStateInit;
friend class ServerGameStateWaitAck;
friend class ServerGameStateStartGame;
friend class ServerGameStateHand;
friend class ServerGameStateWaitPlayerAction;
friend class ServerGameStateWaitNextHand;
};

#endif
