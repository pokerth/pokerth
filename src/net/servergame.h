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
/* Network server game. */

#ifndef _SERVERGAME_H_
#define _SERVERGAME_H_

#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/steady_timer.hpp>
#include <third_party/boost/timers.hpp>
#include <map>

#include <net/sessionmanager.h>
#include <db/serverdbcallback.h>
#include <gui/guiinterface.h>
#include <gamedata.h>


class ServerLobbyThread;
class ServerGameState;
class ServerDBInterface;
class PlayerInterface;
class ConfigFile;
struct GameData;
class Game;

class ServerGame : public boost::enable_shared_from_this<ServerGame>
{
public:
	ServerGame(
		boost::shared_ptr<ServerLobbyThread> lobbyThread, u_int32_t id, const std::string &name, const std::string &pwd, const GameData &gameData,
		unsigned adminPlayerId, unsigned creatorPlayerDBId, GuiInterface &gui, ConfigFile &playerConfig);
	virtual ~ServerGame();

	void Init();
	void Exit();

	u_int32_t GetId() const;
	const std::string &GetName() const;
	unsigned GetCreatorDBId() const;

	void AddSession(boost::shared_ptr<SessionData> session, bool spectateOnly);
	void RemovePlayer(unsigned playerId, unsigned errorCode);
	void MutePlayer(unsigned playerId, bool mute);
	void MarkPlayerAsInactive(unsigned playerId);
	void MarkPlayerAsKicked(unsigned playerId);

	void HandlePacket(boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet);

	ServerCallback &GetCallback();
	GameState GetCurRound() const;

	void SendToAllPlayers(boost::shared_ptr<NetPacket> packet, int state);
	void SendToAllButOnePlayers(boost::shared_ptr<NetPacket> packet, SessionId except, int state);
	void RemoveAllSessions();
	void MoveSpectatorsToLobby();

	bool IsPasswordProtected() const;
	bool CheckPassword(const std::string &password) const;
	static bool CheckSettings(const GameData &data, const std::string &password, ServerMode mode);
	const GameData &GetGameData() const;

	boost::shared_ptr<PlayerData> GetPlayerDataByUniqueId(unsigned playerId) const;
	PlayerIdList GetPlayerIdList() const;
	PlayerIdList GetSpectatorIdList() const;
	bool IsPlayerConnected(const std::string &name) const;
	bool IsPlayerConnected(unsigned playerId) const;
	bool IsClientAddressConnected(const std::string &clientAddress) const;
	boost::shared_ptr<PlayerInterface> GetPlayerInterfaceFromGame(const std::string &playerName);
	boost::shared_ptr<PlayerInterface> GetPlayerInterfaceFromGame(unsigned playerId);

	bool IsRunning() const;

	unsigned GetAdminPlayerId() const;
	void SetAdminPlayerId(unsigned playerId);

	void AddPlayerInvitation(unsigned playerId);
	void RemovePlayerInvitation(unsigned playerId);
	bool IsPlayerInvited(unsigned playerId) const;

	void SetPlayerAutoLeaveOnFinish(unsigned playerId);

	void AddRejoinPlayer(unsigned playerId);
	PlayerIdList GetAndResetRejoinPlayers();

	void AddReactivatePlayer(unsigned playerId);
	PlayerIdList GetAndResetReactivatePlayers();

	void AddNewSpectator(unsigned playerId);
	PlayerIdList GetAndResetNewSpectators();

	void SetNameReported();
	bool IsNameReported() const;

	// should be protected, but is needed in function.
	const Game &GetGame() const;
	Game &GetGame();

	void KickPlayer(unsigned playerId);

	void AddPlayerToNumJoinsPerPlayer(const std::string &playerName);
	int GetNumJoinsPerPlayer(const std::string &playerName);

protected:

	struct RankingData {
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
	void ReplaceRankingPlayer(unsigned oldPlayerId, unsigned newPlayerId);
	void StoreAndResetRanking();
	void RemoveAutoLeavePlayers();
	void InternalEndGame();

	void InternalAskVoteKick(boost::shared_ptr<SessionData> byWhom, unsigned playerIdWho, unsigned timeoutSec);
	void InternalDenyAskVoteKick(boost::shared_ptr<SessionData> byWhom, unsigned playerIdWho, DenyKickPlayerReason reason);
	void InternalVoteKick(boost::shared_ptr<SessionData> byWhom, unsigned petitionId, KickVote vote);
	void InternalDenyVoteKick(boost::shared_ptr<SessionData> byWhom, unsigned petitionId, DenyVoteReason reason);

	PlayerDataList GetFullPlayerDataList() const;

	void AddComputerPlayer(boost::shared_ptr<PlayerData> player);
	boost::shared_ptr<PlayerData> RemoveComputerPlayer(unsigned playerId);
	bool IsComputerPlayerActive(unsigned playerId) const;
	void ResetComputerPlayerList();

	void RemoveSession(boost::shared_ptr<SessionData> session, int reason);
	void RemovePlayerData(boost::shared_ptr<PlayerData> player, int reason, bool spectateOnly);
	void SessionError(boost::shared_ptr<SessionData> session, int errorCode);
	void MoveSessionToLobby(boost::shared_ptr<SessionData> session, int reason);

	void RemoveDisconnectedPlayers();
	int GetCurNumberOfPlayers() const;
	void AssignPlayerNumbers(PlayerDataList &playerList);
	bool IsValidPlayer(unsigned playerId) const;

	void AddReportedAvatar(unsigned playerId);
	bool IsAvatarReported(unsigned playerId) const;

	ServerLobbyThread &GetLobbyThread();

	ServerGameState &GetState();
	void SetState(ServerGameState &newState);

	boost::asio::steady_timer &GetStateTimer1();
	boost::asio::steady_timer &GetStateTimer2();

	const StartData &GetStartData() const;
	void SetStartData(const StartData &startData);

	GuiInterface &GetGui();

	unsigned GetNextGameNum();

	const SessionManager &GetSessionManager() const;
	SessionManager &GetSessionManager();
	ServerDBInterface &GetDatabase();

	typedef std::map<std::string, int> NumJoinsPerPlayerMap;

private:
	ServerGame(const ServerGame &other);

	SessionManager m_sessionManager;
	PlayerDataList m_computerPlayerList;
	mutable boost::mutex m_computerPlayerListMutex;

	PlayerIdList m_playerInvitationList;
	mutable boost::mutex m_playerInvitationListMutex;

	PlayerIdList m_autoLeavePlayerList;
	mutable boost::mutex m_autoLeavePlayerListMutex;

	PlayerIdList m_rejoinPlayerList;
	mutable boost::mutex m_rejoinPlayerListMutex;

	PlayerIdList m_reactivatePlayerList;
	mutable boost::mutex m_reactivatePlayerListMutex;

	PlayerIdList m_newSpectatorList;
	mutable boost::mutex m_newSpectatorListMutex;

	PlayerIdList m_reportedAvatarList;

	RankingMap m_rankingMap;

	unsigned m_adminPlayerId;

	boost::shared_ptr<VoteKickData> m_voteKickData;

	boost::shared_ptr<ServerLobbyThread> m_lobbyThread;
	boost::shared_ptr<ServerDBInterface> m_database;
	GuiInterface &m_gui;

	const GameData		m_gameData;
	StartData			m_startData;
	boost::shared_ptr<Game>	 m_game;
	ServerGameState			*m_curState;

	const u_int32_t		m_id;
	const std::string	m_name;
	const std::string	m_password;
	const unsigned		m_creatorPlayerDBId;
	ConfigFile		   &m_playerConfig;
	unsigned			m_gameNum;
	unsigned			m_curPetitionId;
	boost::asio::steady_timer m_voteKickTimer;
	boost::asio::steady_timer m_stateTimer1;
	boost::asio::steady_timer m_stateTimer2;
	bool				m_isNameReported;

	friend class ServerLobbyThread;
	friend class AbstractServerGameStateReceiving;
	friend class AbstractServerGameStateRunning;
	friend class ServerGameStateInit;
	friend class ServerGameStateWaitAck;
	friend class ServerGameStateStartGame;
	friend class ServerGameStateHand;
	friend class ServerGameStateWaitPlayerAction;
	friend class ServerGameStateWaitNextHand;

	NumJoinsPerPlayerMap m_numJoinsPerPlayer;
};

#endif
