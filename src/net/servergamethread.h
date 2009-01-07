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
/* Network server game thread. */

#ifndef _SERVERGAMETHREAD_H_
#define _SERVERGAMETHREAD_H_

#include <net/sessionmanager.h>
#include <gui/guiinterface.h>
#include <gamedata.h>
#include <third_party/boost/timers.hpp>

#include <deque>

#define GAME_THREAD_TERMINATE_TIMEOUT	THREAD_WAIT_INFINITE


class ReceiverHelper;
class ServerLobbyThread;
class ServerGameState;
class ConfigFile;
struct GameData;
class Game;

class ServerGameThread : public Thread
{
public:
	ServerGameThread(
		ServerLobbyThread &lobbyThread, u_int32_t id, const std::string &name, const std::string &pwd, const GameData &gameData, unsigned adminPlayerId, GuiInterface &gui, ConfigFile *playerConfig);
	virtual ~ServerGameThread();

	u_int32_t GetId() const;
	const std::string &GetName() const;

	void AddSession(SessionWrapper session);
	void RemovePlayer(unsigned playerId, unsigned errorCode);

	ServerCallback &GetCallback();
	GameState GetCurRound() const;

	void SendToAllPlayers(boost::shared_ptr<NetPacket> packet, SessionData::State state);
	void RemoveAllSessions();

	bool IsPasswordProtected() const;
	bool CheckPassword(const std::string &password) const;
	const GameData &GetGameData() const;

	boost::shared_ptr<PlayerData> GetPlayerDataByUniqueId(unsigned playerId) const;
	PlayerIdList GetPlayerIdList() const;
	bool IsPlayerConnected(const std::string &name) const;

	bool IsRunning() const;

	unsigned GetAdminPlayerId() const;
	void SetAdminPlayerId(unsigned playerId);

	// should be protected, but is needed in function.
	const Game &GetGame() const;
	Game &GetGame();

protected:

	typedef std::deque<SessionWrapper> SessionQueue;

	// Main function of the thread.
	virtual void Main();
	void RemovePlayerLoop();
	void VoteKickLoop();
	void VoteKickAction();

	void InternalStartGame();
	void ResetGame();

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
	void AssignPlayerNumbers();
	bool IsValidPlayer(unsigned playerId) const;

	ServerLobbyThread &GetLobbyThread();

	ServerGameState &GetState();
	void SetState(ServerGameState &newState);

	const boost::timers::portable::microsec_timer &GetStateTimer() const;
	boost::timers::portable::microsec_timer &GetStateTimer();
	unsigned GetStateTimerFlag() const;
	void SetStateTimerFlag(unsigned flag);

	ReceiverHelper &GetReceiver();

	const StartData &GetStartData() const;
	void SetStartData(const StartData &startData);

	GuiInterface &GetGui();

	unsigned GetNextGameNum();

	const SessionManager &GetSessionManager() const;
	SessionManager &GetSessionManager();

private:

	SessionQueue m_sessionQueue;
	mutable boost::mutex m_sessionQueueMutex;

	SessionManager m_sessionManager;
	PlayerDataList m_computerPlayerList;
	mutable boost::mutex m_computerPlayerListMutex;

	RemovePlayerList m_removePlayerList;
	mutable boost::mutex m_removePlayerListMutex;

	unsigned m_adminPlayerId;
	mutable boost::mutex m_adminPlayerIdMutex;

	boost::shared_ptr<VoteKickData> m_voteKickData;
	mutable boost::mutex m_voteKickDataMutex;

	ServerLobbyThread &m_lobbyThread;
	boost::shared_ptr<ReceiverHelper> m_receiver;
	GuiInterface &m_gui;

	const GameData		m_gameData;
	StartData			m_startData;
	boost::shared_ptr<Game>	m_game;
	const u_int32_t		m_id;
	const std::string	m_name;
	const std::string	m_password;
	ConfigFile		   *m_playerConfig;
	ServerGameState	   *m_curState;
	unsigned			m_gameNum;
	unsigned			m_curPetitionId;

	boost::timers::portable::microsec_timer m_stateTimer;
	unsigned								m_stateTimerFlag;

	boost::timers::portable::microsec_timer m_voteKickActionTimer;

friend class AbstractServerGameStateReceiving;
friend class AbstractServerGameStateRunning;
friend class AbstractServerGameStateTimer;
friend class ServerGameStateInit;
friend class ServerGameStateWaitAck;
friend class ServerGameStateStartGame;
friend class ServerGameStateStartHand;
friend class ServerGameStateStartRound;
friend class ServerGameStateWaitPlayerAction;
friend class ServerGameStateComputerAction;
friend class ServerGameStateDealCardsDelay;
friend class ServerGameStateShowCardsDelay;
friend class ServerGameStateNextHandDelay;
friend class ServerGameStateNextGameDelay;
};

#endif
