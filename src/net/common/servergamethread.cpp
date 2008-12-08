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

#include <net/servergamethread.h>
#include <net/servergamestate.h>
#include <net/serverlobbythread.h>
#include <net/serverexception.h>
#include <net/senderthread.h>
#include <net/receiverhelper.h>
#include <net/socket_msg.h>
#include <core/loghelper.h>
#include <game.h>
#include <localenginefactory.h>
#include <tools.h>

#include <boost/bind.hpp>


#define SERVER_CHECK_VOTE_KICK_INTERVAL_MSEC	500
#define SERVER_KICK_TIMEOUT_ADD_DELAY_SEC		2

using namespace std;


ServerGameThread::ServerGameThread(ServerLobbyThread &lobbyThread, u_int32_t id, const string &name, const string &pwd, const GameData &gameData, unsigned adminPlayerId, GuiInterface &gui, ConfigFile *playerConfig)
: m_adminPlayerId(adminPlayerId), m_lobbyThread(lobbyThread), m_gui(gui),
  m_gameData(gameData), m_id(id), m_name(name), m_password(pwd), m_playerConfig(playerConfig),
  m_curState(NULL), m_gameNum(1), m_curPetitionId(1),
  m_stateTimer(boost::posix_time::time_duration(0, 0, 0), boost::timers::portable::microsec_timer::manual_start),
  m_stateTimerFlag(0)
{
	LOG_VERBOSE("Game object " << GetId() << " created.");

	m_receiver.reset(new ReceiverHelper);
}

ServerGameThread::~ServerGameThread()
{
	LOG_VERBOSE("Game object " << GetId() << " destructed.");
}

u_int32_t
ServerGameThread::GetId() const
{
	return m_id;
}

const std::string &
ServerGameThread::GetName() const
{
	return m_name;
}

void
ServerGameThread::AddSession(SessionWrapper session)
{
	// Must be thread safe.
	boost::mutex::scoped_lock lock(m_sessionQueueMutex);
	m_sessionQueue.push_back(session);
}

void
ServerGameThread::RemovePlayer(unsigned playerId, unsigned errorCode)
{
	boost::mutex::scoped_lock lock(m_removePlayerListMutex);
	m_removePlayerList.push_back(RemovePlayerList::value_type(playerId, errorCode));
}

GameState
ServerGameThread::GetCurRound() const
{
	return static_cast<GameState>(GetGame().getCurrentHand()->getCurrentRound());
}

void
ServerGameThread::SendToAllPlayers(boost::shared_ptr<NetPacket> packet, SessionData::State state)
{
	GetSessionManager().SendToAllSessions(GetSender(), packet, state);
}

void
ServerGameThread::RemoveAllSessions()
{
	// Called from lobby thread.
	// Clean up ALL sessions which are left.
	ServerLobbyThread &lobbyThread = GetLobbyThread();
	boost::mutex::scoped_lock lock(m_sessionQueueMutex);
	while (!m_sessionQueue.empty())
	{
		SessionWrapper tmpSession = m_sessionQueue.front();
		m_sessionQueue.pop_front();
		LOG_VERBOSE("Game closing, forcing removal of session #" << tmpSession.sessionData->GetId() << ".");
		lobbyThread.RemoveSessionFromGame(tmpSession);
	}
	GetSessionManager().ForEach(boost::bind(&ServerLobbyThread::RemoveSessionFromGame, boost::ref(lobbyThread), _1));
}

void
ServerGameThread::Main()
{
	LOG_VERBOSE("Game thread " << GetId() << " started.");

	SetState(SERVER_INITIAL_STATE::Instance());

	try
	{
		do
		{
			{
				// Handle one new session at a time.
				SessionWrapper tmpSession;
				{
					boost::mutex::scoped_lock lock(m_sessionQueueMutex);
					if (!m_sessionQueue.empty())
					{
						tmpSession = m_sessionQueue.front();
						m_sessionQueue.pop_front();
					}
				}
				if (tmpSession.sessionData.get())
					GetState().HandleNewSession(*this, tmpSession);
			}
			// Process current state.
			GetState().Process(*this);
			RemovePlayerLoop();
			VoteKickLoop();
		} while (!ShouldTerminate() && GetSessionManager().HasSessions());
	} catch (const PokerTHException &e)
	{
		GetCallback().SignalNetServerError(e.GetErrorId(), e.GetOsErrorCode());
		LOG_ERROR(e.what());
	}

	ResetComputerPlayerList();
	GetLobbyThread().RemoveGame(GetId());

	LOG_VERBOSE("Game thread " << GetId() << " terminating.");
}

void
ServerGameThread::RemovePlayerLoop()
{
	boost::mutex::scoped_lock lock(m_removePlayerListMutex);

	RemovePlayerList::iterator i = m_removePlayerList.begin();
	RemovePlayerList::iterator end = m_removePlayerList.end();

	while (i != end)
	{
		SessionWrapper tmpSession = GetSessionManager().GetSessionByUniquePlayerId(i->first);
		// Only kick if the player was found.
		if (tmpSession.sessionData.get())
			SessionError(tmpSession, i->second);
		++i;
	}
	m_removePlayerList.clear();
}

void
ServerGameThread::VoteKickLoop()
{
	// Do not call the vote kick action all the time.
	// Check the timer.
	if (m_voteKickActionTimer.elapsed().total_milliseconds() >= SERVER_CHECK_VOTE_KICK_INTERVAL_MSEC)
		VoteKickAction();
}

void
ServerGameThread::VoteKickAction()
{
	// Check whether someone should be kicked, or whether a vote kick should be aborted.
	// Only one vote kick can be active at a time.
	boost::mutex::scoped_lock lock(m_voteKickDataMutex);
	if (m_voteKickData)
	{
		// Prepare some values.
		const PlayerIdList playerIds(GetPlayerIdList());
		int votesRequiredToKick = m_voteKickData->numVotesToKick - m_voteKickData->numVotesInFavourOfKicking;
		int playersAllowedToVote = 0;
		// We need to count the number of players which are still allowed to vote.
		PlayerIdList::const_iterator player_i = playerIds.begin();
		PlayerIdList::const_iterator player_end = playerIds.end();
		while (player_i != player_end)
		{
			if (find(m_voteKickData->votedPlayerIds.begin(), m_voteKickData->votedPlayerIds.end(), *player_i) == m_voteKickData->votedPlayerIds.end())
				playersAllowedToVote++;
			++player_i;
		}
		bool abortPetition = false;
		bool doKick = false;
		EndPetitionReason reason;

		// 1. Enough votes to kick the player.
		if (m_voteKickData->numVotesInFavourOfKicking >= m_voteKickData->numVotesToKick)
		{
			reason = PETITION_END_ENOUGH_VOTES;
			abortPetition = true;
			doKick = true;
		}
		// 2. Several players left the game, so a kick is no longer possible.
		else if (votesRequiredToKick > playersAllowedToVote)
		{
			reason = PETITION_END_NOT_ENOUGH_PLAYERS;
			abortPetition = true;
		}
		// 3. The kick has become invalid because the player to be kicked left.
		else if (find(playerIds.begin(), playerIds.end(), m_voteKickData->kickPlayerId) == playerIds.end())
		{
			reason = PETITION_END_PLAYER_LEFT;
			abortPetition = true;
		}
		// 4. A kick request timed out (because not everyone voted).
		else if (m_voteKickData->voteTimer.elapsed().total_seconds() >= m_voteKickData->timeLimitSec)
		{
			reason = PETITION_END_TIMEOUT;
			abortPetition = true;
		}
		if (abortPetition)
		{
			boost::shared_ptr<NetPacket> endPetition(new NetPacketEndKickPlayerPetition);
			NetPacketEndKickPlayerPetition::Data endPetitionData;
			endPetitionData.petitionId = m_voteKickData->petitionId;
			endPetitionData.numVotesAgainstKicking = m_voteKickData->numVotesAgainstKicking;
			endPetitionData.numVotesInFavourOfKicking = m_voteKickData->numVotesInFavourOfKicking;
			endPetitionData.playerKicked = doKick;
			endPetitionData.endReason = reason;

			static_cast<NetPacketEndKickPlayerPetition *>(endPetition.get())->SetData(endPetitionData);
			SendToAllPlayers(endPetition, SessionData::Game);

			// Perform kick.
			if (doKick)
				InternalKickPlayer(m_voteKickData->kickPlayerId);
			// This petition has ended.
			m_voteKickData.reset();
		}
	}
}

void
ServerGameThread::InternalStartGame()
{
	// Set order of players.
	AssignPlayerNumbers();

	// Initialize the game.
	GuiInterface &gui = GetGui();
	PlayerDataList playerData = GetFullPlayerDataList();

	// Create EngineFactory
	boost::shared_ptr<EngineFactory> factory(new LocalEngineFactory(m_playerConfig)); // LocalEngine erstellen

	// Set start data.
	StartData startData;
	startData.numberOfPlayers = playerData.size();

	int tmpDealerPos = 0;
	Tools::getRandNumber(0, startData.numberOfPlayers-1, 1, &tmpDealerPos, 0);
	// The Player Id is not continuous. Therefore, the start dealer position
	// needs to be converted to a player Id, and cannot be directly generated
	// as player Id.
	PlayerDataList::const_iterator player_i = playerData.begin();
	PlayerDataList::const_iterator player_end = playerData.end();

	int tmpPos = 0;
	while (player_i != player_end)
	{
		startData.startDealerPlayerId = static_cast<unsigned>((*player_i)->GetUniqueId());
		if (tmpPos == tmpDealerPos)
			break;
		++tmpPos;
		++player_i;
	}
	if (player_i == player_end)
		throw ServerException(__FILE__, __LINE__, ERR_NET_DEALER_NOT_FOUND, 0);

	SetStartData(startData);

	m_game.reset(new Game(&gui, factory, playerData, GetGameData(), GetStartData(), GetNextGameNum()));

	GetLobbyThread().NotifyStartingGame(GetId());
}

void
ServerGameThread::ResetGame()
{
	m_game.reset();
}

void
ServerGameThread::InternalKickPlayer(unsigned playerId)
{
	SessionWrapper tmpSession = GetSessionManager().GetSessionByUniquePlayerId(playerId);
	// Only kick if the player was found.
	if (tmpSession.sessionData.get())
		MoveSessionToLobby(tmpSession, NTF_NET_REMOVED_KICKED);
}

void
ServerGameThread::InternalAskVoteKick(SessionWrapper byWhom, unsigned playerIdWho, unsigned timeoutSec)
{
	if (IsRunning() && byWhom.playerData)
	{
		// Retrieve only the number of human players.
		size_t numPlayers = GetSessionManager().GetPlayerIdList().size();
		if (numPlayers > 2)
		{
			// Lock the vote kick data.
			boost::mutex::scoped_lock lock(m_voteKickDataMutex);
			if (!m_voteKickData)
			{
				// Initiate a vote kick.
				unsigned playerIdByWhom = byWhom.playerData->GetUniqueId();
				m_voteKickData.reset(new VoteKickData);
				m_voteKickData->petitionId = m_curPetitionId++;
				m_voteKickData->kickPlayerId = playerIdWho;
				m_voteKickData->numVotesToKick = static_cast<int>(ceil(numPlayers / 3. * 2.));
				m_voteKickData->timeLimitSec = timeoutSec + SERVER_KICK_TIMEOUT_ADD_DELAY_SEC;
				// Consider first vote.
				m_voteKickData->numVotesInFavourOfKicking = 1;
				m_voteKickData->votedPlayerIds.push_back(playerIdByWhom);

				boost::shared_ptr<NetPacket> startPetition(new NetPacketStartKickPlayerPetition);
				NetPacketStartKickPlayerPetition::Data startPetitionData;
				startPetitionData.petitionId = m_voteKickData->petitionId;
				startPetitionData.proposingPlayerId = playerIdByWhom;
				startPetitionData.kickPlayerId = m_voteKickData->kickPlayerId;
				startPetitionData.kickTimeoutSec = timeoutSec;
				startPetitionData.numVotesNeededToKick = m_voteKickData->numVotesToKick;

				static_cast<NetPacketStartKickPlayerPetition *>(startPetition.get())->SetData(startPetitionData);
				SendToAllPlayers(startPetition, SessionData::Game);
			}
			else
				InternalDenyAskVoteKick(byWhom, playerIdWho, KICK_DENIED_OTHER_IN_PROGRESS);
		}
		else
			InternalDenyAskVoteKick(byWhom, playerIdWho, KICK_DENIED_TOO_FEW_PLAYERS);
	}
	else
		InternalDenyAskVoteKick(byWhom, playerIdWho, KICK_DENIED_INVALID_STATE);
}

void
ServerGameThread::InternalDenyAskVoteKick(SessionWrapper byWhom, unsigned playerIdWho, DenyKickPlayerReason reason)
{
	boost::shared_ptr<NetPacket> denyPetition(new NetPacketAskKickPlayerDenied);
	NetPacketAskKickPlayerDenied::Data denyPetitionData;
	denyPetitionData.playerId = playerIdWho;
	denyPetitionData.denyReason = reason;
	static_cast<NetPacketAskKickPlayerDenied *>(denyPetition.get())->SetData(denyPetitionData);
	GetSender().Send(byWhom.sessionData, denyPetition);
}

void
ServerGameThread::InternalVoteKick(SessionWrapper byWhom, unsigned petitionId, KickVote vote)
{
	if (IsRunning() && byWhom.playerData)
	{
		boost::mutex::scoped_lock lock(m_voteKickDataMutex);
		// Check whether this is the valid petition id.
		if (m_voteKickData->petitionId == petitionId)
		{
			// Check whether the player already voted.
			unsigned playerId = byWhom.playerData->GetUniqueId();
			if (find(m_voteKickData->votedPlayerIds.begin(), m_voteKickData->votedPlayerIds.end(), playerId) == m_voteKickData->votedPlayerIds.end())
			{
				m_voteKickData->votedPlayerIds.push_back(playerId);
				if (vote == KICK_VOTE_IN_FAVOUR)
					m_voteKickData->numVotesInFavourOfKicking++;
				else
					m_voteKickData->numVotesAgainstKicking++;
				// Send update notification.
				boost::shared_ptr<NetPacket> updatePetition(new NetPacketKickPlayerPetitionUpdate);
				NetPacketKickPlayerPetitionUpdate::Data updatePetitionData;
				updatePetitionData.petitionId = m_voteKickData->petitionId;
				updatePetitionData.numVotesAgainstKicking = m_voteKickData->numVotesAgainstKicking;
				updatePetitionData.numVotesInFavourOfKicking = m_voteKickData->numVotesInFavourOfKicking;
				updatePetitionData.numVotesNeededToKick = m_voteKickData->numVotesToKick;

				static_cast<NetPacketKickPlayerPetitionUpdate *>(updatePetition.get())->SetData(updatePetitionData);
				SendToAllPlayers(updatePetition, SessionData::Game);
			}
			else
				InternalDenyVoteKick(byWhom, petitionId, VOTE_DENIED_ALREADY_VOTED);
		}
		else
			InternalDenyVoteKick(byWhom, petitionId, VOTE_DENIED_INVALID_PETITION);
	}
	else
		InternalDenyVoteKick(byWhom, petitionId, VOTE_DENIED_IMPOSSIBLE);
}

void
ServerGameThread::InternalDenyVoteKick(SessionWrapper byWhom, unsigned petitionId, DenyVoteReason reason)
{
	boost::shared_ptr<NetPacket> denyVote(new NetPacketVoteKickPlayerDenied);
	NetPacketVoteKickPlayerDenied::Data denyVoteData;
	denyVoteData.petitionId = petitionId;
	denyVoteData.denyReason = reason;
	static_cast<NetPacketVoteKickPlayerDenied *>(denyVote.get())->SetData(denyVoteData);
	GetSender().Send(byWhom.sessionData, denyVote);
}

PlayerDataList
ServerGameThread::GetFullPlayerDataList() const
{
	PlayerDataList playerList(GetSessionManager().GetPlayerDataList());
	boost::mutex::scoped_lock lock(m_computerPlayerListMutex);
	copy(m_computerPlayerList.begin(), m_computerPlayerList.end(), back_inserter(playerList));

	return playerList;
}

boost::shared_ptr<PlayerData>
ServerGameThread::GetPlayerDataByUniqueId(unsigned playerId) const
{
	boost::shared_ptr<PlayerData> tmpPlayer;
	SessionWrapper session = GetSessionManager().GetSessionByUniquePlayerId(playerId);
	if (session.playerData.get())
	{
		tmpPlayer = session.playerData;
	}
	else
	{
		boost::mutex::scoped_lock lock(m_computerPlayerListMutex);
		PlayerDataList::const_iterator i = m_computerPlayerList.begin();
		PlayerDataList::const_iterator end = m_computerPlayerList.end();
		while (i != end)
		{
			if ((*i)->GetUniqueId() == playerId)
			{
				tmpPlayer = *i;
				break;
			}
			++i;
		}
	}
	return tmpPlayer;
}

PlayerIdList
ServerGameThread::GetPlayerIdList() const
{
	PlayerIdList idList(GetSessionManager().GetPlayerIdList());
	boost::mutex::scoped_lock lock(m_computerPlayerListMutex);
	PlayerDataList::const_iterator i = m_computerPlayerList.begin();
	PlayerDataList::const_iterator end = m_computerPlayerList.end();
	while (i != end)
	{
		idList.push_back((*i)->GetUniqueId());
		++i;
	}

	return idList;
}

bool
ServerGameThread::IsPlayerConnected(const std::string &name) const
{
	return GetSessionManager().IsPlayerConnected(name);
}

bool
ServerGameThread::IsRunning() const
{
	return m_game.get() != NULL;
}

unsigned
ServerGameThread::GetAdminPlayerId() const
{
	boost::mutex::scoped_lock lock(m_adminPlayerIdMutex);
	return m_adminPlayerId;
}

void
ServerGameThread::SetAdminPlayerId(unsigned playerId)
{
	boost::mutex::scoped_lock lock(m_adminPlayerIdMutex);
	m_adminPlayerId = playerId;
}

void
ServerGameThread::AddComputerPlayer(boost::shared_ptr<PlayerData> player)
{
	{
		boost::mutex::scoped_lock lock(m_computerPlayerListMutex);
		m_computerPlayerList.push_back(player);
	}
	GetLobbyThread().AddComputerPlayer(player);
}

void
ServerGameThread::ResetComputerPlayerList()
{
	boost::mutex::scoped_lock lock(m_computerPlayerListMutex);

	PlayerDataList::iterator i = m_computerPlayerList.begin();
	PlayerDataList::iterator end = m_computerPlayerList.end();

	while (i != end)
	{
		GetLobbyThread().RemoveComputerPlayer(*i);
		RemovePlayerData(*i, NTF_NET_REMOVED_ON_REQUEST);
		++i;
	}

	m_computerPlayerList.clear();
}

void
ServerGameThread::GracefulRemoveSession(SessionWrapper session, int reason)
{
	if (!session.sessionData.get())
		throw ServerException(__FILE__, __LINE__, ERR_NET_INVALID_SESSION, 0);
	GetSessionManager().RemoveSession(session.sessionData->GetId());

	boost::shared_ptr<PlayerData> tmpPlayerData = session.playerData;
	if (tmpPlayerData.get() && !tmpPlayerData->GetName().empty())
	{
		RemovePlayerData(tmpPlayerData, reason);
	}
}

void
ServerGameThread::RemovePlayerData(boost::shared_ptr<PlayerData> player, int reason)
{
	if (player->GetRights() == PLAYER_RIGHTS_ADMIN)
	{
		// Find new admin for the game
		PlayerDataList playerList(GetSessionManager().GetPlayerDataList());
		if (!playerList.empty())
		{
			boost::shared_ptr<PlayerData> newAdmin = playerList.front();
			SetAdminPlayerId(newAdmin->GetUniqueId());
			newAdmin->SetRights(PLAYER_RIGHTS_ADMIN);
			// Notify game state on admin change
			GetState().NotifyGameAdminChanged(*this);
			// Send "Game Admin Changed" to clients.
			boost::shared_ptr<NetPacket> adminChanged(new NetPacketGameAdminChanged);
			NetPacketGameAdminChanged::Data adminChangedData;
			adminChangedData.playerId = newAdmin->GetUniqueId(); // Choose next player as admin.
			static_cast<NetPacketGameAdminChanged *>(adminChanged.get())->SetData(adminChangedData);
			GetSessionManager().SendToAllSessions(GetSender(), adminChanged, SessionData::Game);

			GetLobbyThread().NotifyGameAdminChanged(GetId(), newAdmin->GetUniqueId());
		}
	}
	// Reset player rights.
	player->SetRights(PLAYER_RIGHTS_NORMAL);

	// Send "Player Left" to clients.
	boost::shared_ptr<NetPacket> thisPlayerLeft(new NetPacketPlayerLeft);
	NetPacketPlayerLeft::Data thisPlayerLeftData;
	thisPlayerLeftData.playerId = player->GetUniqueId();
	thisPlayerLeftData.removeReason = reason;
	static_cast<NetPacketPlayerLeft *>(thisPlayerLeft.get())->SetData(thisPlayerLeftData);
	GetSessionManager().SendToAllSessions(GetSender(), thisPlayerLeft, SessionData::Game);

	GetLobbyThread().NotifyPlayerLeftGame(GetId(), player->GetUniqueId());
}

void
ServerGameThread::ErrorRemoveSession(SessionWrapper session)
{
	GetLobbyThread().RemoveSessionFromGame(session);
	GracefulRemoveSession(session, NTF_NET_INTERNAL);
}

void
ServerGameThread::SessionError(SessionWrapper session, int errorCode)
{
	if (!session.sessionData.get())
		throw ServerException(__FILE__, __LINE__, ERR_NET_INVALID_SESSION, 0);
	ErrorRemoveSession(session);
	GetLobbyThread().SessionError(session, errorCode);
}

void
ServerGameThread::MoveSessionToLobby(SessionWrapper session, int reason)
{
	GracefulRemoveSession(session, reason);
	// Reset ready flag - just in case it is set, player may leave at any time.
	session.sessionData->ResetReadyFlag();
	GetLobbyThread().ReAddSession(session, reason);
}

void
ServerGameThread::RemoveDisconnectedPlayers()
{
	// This should only be called between hands.
	if (m_game.get())
	{
		PlayerListIterator i = m_game->getSeatsList()->begin();
		PlayerListIterator end = m_game->getSeatsList()->end();
		while (i != end)
		{
			boost::shared_ptr<PlayerInterface> tmpPlayer = *i;
			if (!GetSessionManager().IsPlayerConnected(tmpPlayer->getMyUniqueID()) && tmpPlayer->getMyType() == PLAYER_TYPE_HUMAN)
			{
				// Setting player cash to 0 will deactivate the player.
				tmpPlayer->setMyCash(0);
				tmpPlayer->setNetSessionData(boost::shared_ptr<SessionData>());
			}
			++i;
		}
	}
}

size_t
ServerGameThread::GetCurNumberOfPlayers() const
{
	return GetFullPlayerDataList().size();
}

void
ServerGameThread::AssignPlayerNumbers()
{
	int playerNumber = 0;

	PlayerDataList playerList = GetFullPlayerDataList();
	PlayerDataList::iterator player_i = playerList.begin();
	PlayerDataList::iterator player_end = playerList.end();

	while (player_i != player_end)
	{
		(*player_i)->SetNumber(playerNumber);
		++playerNumber;
		++player_i;
	}
}

SessionManager &
ServerGameThread::GetSessionManager()
{
	return m_sessionManager;
}

const SessionManager &
ServerGameThread::GetSessionManager() const
{
	return m_sessionManager;
}

ServerLobbyThread &
ServerGameThread::GetLobbyThread()
{
	return m_lobbyThread;
}

ServerCallback &
ServerGameThread::GetCallback()
{
	return m_gui;
}

ServerGameState &
ServerGameThread::GetState()
{
	assert(m_curState);
	return *m_curState;
}

void
ServerGameThread::SetState(ServerGameState &newState)
{
	newState.Init(*this);
	m_curState = &newState;
}

const boost::timers::portable::microsec_timer &
ServerGameThread::GetStateTimer() const
{
	return m_stateTimer;
}

boost::timers::portable::microsec_timer &
ServerGameThread::GetStateTimer()
{
	return m_stateTimer;
}

unsigned
ServerGameThread::GetStateTimerFlag() const
{
	return m_stateTimerFlag;
}
void
ServerGameThread::SetStateTimerFlag(unsigned flag)
{
	m_stateTimerFlag = flag;
}

SenderThread &
ServerGameThread::GetSender()
{
	return GetLobbyThread().GetSender();
}

ReceiverHelper &
ServerGameThread::GetReceiver()
{
	assert(m_receiver.get());
	return *m_receiver;
}

Game &
ServerGameThread::GetGame()
{
	assert(m_game.get());
	return *m_game;
}

const Game &
ServerGameThread::GetGame() const
{
	assert(m_game.get());
	return *m_game;
}

const GameData &
ServerGameThread::GetGameData() const
{
	return m_gameData;
}

const StartData &
ServerGameThread::GetStartData() const
{
	return m_startData;
}

void
ServerGameThread::SetStartData(const StartData &startData)
{
	m_startData = startData;
}

bool
ServerGameThread::IsPasswordProtected() const
{
	return !m_password.empty();
}

bool
ServerGameThread::CheckPassword(const string &password) const
{
	return (password == m_password);
}

GuiInterface &
ServerGameThread::GetGui()
{
	return m_gui;
}

unsigned
ServerGameThread::GetNextGameNum()
{
	return m_gameNum++;
}

