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

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <net/servergame.h>
#include <net/servergamestate.h>
#include <net/serverlobbythread.h>
#include <net/serverexception.h>
#include <net/senderhelper.h>
#include <net/receiverhelper.h>
#include <net/socket_msg.h>
#include <core/loghelper.h>
#include <db/serverdbinterface.h>
#include <game.h>
#include <localenginefactory.h>
#include <tools.h>


#define SERVER_CHECK_VOTE_KICK_INTERVAL_MSEC	500
#define SERVER_KICK_TIMEOUT_ADD_DELAY_SEC		2

using namespace std;


ServerGame::ServerGame(boost::shared_ptr<ServerLobbyThread> lobbyThread, u_int32_t id, const string &name, const string &pwd, const GameData &gameData, unsigned adminPlayerId, GuiInterface &gui, ConfigFile *playerConfig)
: m_adminPlayerId(adminPlayerId), m_lobbyThread(lobbyThread), m_gui(gui),
  m_gameData(gameData), m_curState(NULL), m_id(id), m_dbId(DB_ID_INVALID), m_name(name),
  m_password(pwd), m_playerConfig(playerConfig), m_gameNum(1), m_curPetitionId(1),
  m_voteKickTimer(lobbyThread->GetIOService()), m_stateTimer(lobbyThread->GetIOService())
{
	LOG_VERBOSE("Game object " << GetId() << " created.");

	m_receiver.reset(new ReceiverHelper);
}

ServerGame::~ServerGame()
{
	LOG_VERBOSE("Game object " << GetId() << " destructed.");
}

void
ServerGame::Init()
{
	m_voteKickTimer.expires_from_now(
		boost::posix_time::milliseconds(SERVER_CHECK_VOTE_KICK_INTERVAL_MSEC));
	m_voteKickTimer.async_wait(
		boost::bind(
			&ServerGame::TimerVoteKick, shared_from_this(), boost::asio::placeholders::error));

	SetState(SERVER_INITIAL_STATE::Instance());
}

void
ServerGame::Exit()
{
	m_voteKickTimer.cancel();
	SetState(ServerGameStateFinal::Instance());
}

u_int32_t
ServerGame::GetId() const
{
	return m_id;
}

const std::string &
ServerGame::GetName() const
{
	return m_name;
}

DB_id
ServerGame::GetDBId() const
{
	return m_dbId;
}

void
ServerGame::SetDBId(DB_id newId)
{
	m_dbId = newId;
}

void
ServerGame::AddSession(SessionWrapper session)
{
	if (session.sessionData)
		GetState().HandleNewSession(shared_from_this(), session);
}

void
ServerGame::RemovePlayer(unsigned playerId, unsigned errorCode)
{
	SessionWrapper tmpSession = GetSessionManager().GetSessionByUniquePlayerId(playerId);
	// Only kick if the player was found.
	if (tmpSession.sessionData.get())
		SessionError(tmpSession, errorCode);
}

void
ServerGame::HandlePacket(SessionWrapper session, boost::shared_ptr<NetPacket> packet)
{
	if (session.sessionData && packet)
		GetState().ProcessPacket(shared_from_this(), session, packet);
}

GameState
ServerGame::GetCurRound() const
{
	return static_cast<GameState>(GetGame().getCurrentHand()->getCurrentRound());
}

void
ServerGame::SendToAllPlayers(boost::shared_ptr<NetPacket> packet, SessionData::State state)
{
	GetSessionManager().SendToAllSessions(GetLobbyThread().GetSender(), packet, state);
}

void
ServerGame::RemoveAllSessions()
{
	// Clean up ALL sessions which are left.
	GetSessionManager().ForEach(boost::bind(&ServerLobbyThread::RemoveSessionFromGame, boost::ref(*m_lobbyThread), _1));
	GetSessionManager().Clear();
	SetState(ServerGameStateFinal::Instance());
}

void
ServerGame::TimerVoteKick(const boost::system::error_code &ec)
{
	if (!ec && m_curState != &ServerGameStateFinal::Instance())
	{
		// Check whether someone should be kicked, or whether a vote kick should be aborted.
		// Only one vote kick can be active at a time.
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
			else if (!IsValidPlayer(m_voteKickData->kickPlayerId))
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
				boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
				packet->GetMsg()->present = PokerTHMessage_PR_endKickPetitionMessage;
				EndKickPetitionMessage_t *netEndPetition = &packet->GetMsg()->choice.endKickPetitionMessage;
				netEndPetition->gameId = GetId();
				netEndPetition->petitionId = m_voteKickData->petitionId;
				netEndPetition->numVotesAgainstKicking = m_voteKickData->numVotesAgainstKicking;
				netEndPetition->numVotesInFavourOfKicking = m_voteKickData->numVotesInFavourOfKicking;
				netEndPetition->resultPlayerKicked = doKick;
				netEndPetition->petitionEndReason = reason;
				SendToAllPlayers(packet, SessionData::Game);

				// Perform kick.
				if (doKick)
					InternalKickPlayer(m_voteKickData->kickPlayerId);
				// This petition has ended.
				m_voteKickData.reset();
			}
		}
		m_voteKickTimer.expires_from_now(
			boost::posix_time::milliseconds(SERVER_CHECK_VOTE_KICK_INTERVAL_MSEC));
		m_voteKickTimer.async_wait(
			boost::bind(
				&ServerGame::TimerVoteKick, shared_from_this(), boost::asio::placeholders::error));
	}
}

void
ServerGame::InternalStartGame()
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
	GetLobbyThread().GetDatabase().AsyncCreateGame(GetId(), GetName());
}

void
ServerGame::ResetGame()
{
	m_game.reset();
}

void
ServerGame::InternalKickPlayer(unsigned playerId)
{
	SessionWrapper tmpSession = GetSessionManager().GetSessionByUniquePlayerId(playerId);
	// Only kick if the player was found.
	if (tmpSession.sessionData.get())
		MoveSessionToLobby(tmpSession, NTF_NET_REMOVED_KICKED);
	// KICKING COMPUTER PLAYERS IS BUGGY AND OCCASIONALLY CAUSES A CRASH
	// Disabled for now.
	//else
	//{
	//	boost::shared_ptr<PlayerData> tmpData(RemoveComputerPlayer(playerId));
	//	if (tmpData)
	//		RemovePlayerData(tmpData, NTF_NET_REMOVED_KICKED);
	//}
}

void
ServerGame::InternalAskVoteKick(SessionWrapper byWhom, unsigned playerIdWho, unsigned timeoutSec)
{
	if (IsRunning() && byWhom.playerData)
	{
		// Retrieve only the number of human players.
		size_t numPlayers = GetSessionManager().GetPlayerIdList(SessionData::Game).size();
		if (numPlayers > 2)
		{
			// Check whether the player to be kicked exists.
			if (IsValidPlayer(playerIdWho))
			{
				// Lock the vote kick data.
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

					boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
					packet->GetMsg()->present = PokerTHMessage_PR_startKickPetitionMessage;
					StartKickPetitionMessage_t *netStartPetition = &packet->GetMsg()->choice.startKickPetitionMessage;
					netStartPetition->gameId = GetId();
					netStartPetition->petitionId = m_voteKickData->petitionId;
					netStartPetition->proposingPlayerId = playerIdByWhom;
					netStartPetition->kickPlayerId = m_voteKickData->kickPlayerId;
					netStartPetition->kickTimeoutSec = timeoutSec;
					netStartPetition->numVotesNeededToKick = m_voteKickData->numVotesToKick;
					SendToAllPlayers(packet, SessionData::Game);
				}
				else
					InternalDenyAskVoteKick(byWhom, playerIdWho, KICK_DENIED_OTHER_IN_PROGRESS);
			}
			else
				InternalDenyAskVoteKick(byWhom, playerIdWho, KICK_DENIED_INVALID_PLAYER_ID);
		}
		else
			InternalDenyAskVoteKick(byWhom, playerIdWho, KICK_DENIED_TOO_FEW_PLAYERS);
	}
	else
		InternalDenyAskVoteKick(byWhom, playerIdWho, KICK_DENIED_INVALID_STATE);
}

void
ServerGame::InternalDenyAskVoteKick(SessionWrapper byWhom, unsigned playerIdWho, DenyKickPlayerReason reason)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
	packet->GetMsg()->present = PokerTHMessage_PR_askKickDeniedMessage;
	AskKickDeniedMessage_t *netKickDenied = &packet->GetMsg()->choice.askKickDeniedMessage;
	netKickDenied->gameId = GetId();
	netKickDenied->playerId = playerIdWho;
	netKickDenied->kickDeniedReason = reason;
	GetLobbyThread().GetSender().Send(byWhom.sessionData, packet);
}

void
ServerGame::InternalVoteKick(SessionWrapper byWhom, unsigned petitionId, KickVote vote)
{
	if (IsRunning() && byWhom.playerData)
	{
		// Check whether this is the valid petition id.
		if (m_voteKickData && m_voteKickData->petitionId == petitionId)
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
				boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
				packet->GetMsg()->present = PokerTHMessage_PR_kickPetitionUpdateMessage;
				KickPetitionUpdateMessage_t *netKickUpdate = &packet->GetMsg()->choice.kickPetitionUpdateMessage;
				netKickUpdate->gameId = GetId();
				netKickUpdate->petitionId = m_voteKickData->petitionId;
				netKickUpdate->numVotesAgainstKicking = m_voteKickData->numVotesAgainstKicking;
				netKickUpdate->numVotesInFavourOfKicking = m_voteKickData->numVotesInFavourOfKicking;
				netKickUpdate->numVotesNeededToKick = m_voteKickData->numVotesToKick;
				SendToAllPlayers(packet, SessionData::Game);
			}
			else
				InternalDenyVoteKick(byWhom, petitionId, VOTE_DENIED_ALREADY_VOTED);
		}
		else
			InternalDenyVoteKick(byWhom, petitionId, VOTE_DENIED_INVALID_PETITION);
	}
	else
		InternalDenyVoteKick(byWhom, petitionId, VOTE_DENIED_INVALID_PETITION);
}

void
ServerGame::InternalDenyVoteKick(SessionWrapper byWhom, unsigned petitionId, DenyVoteReason reason)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
	packet->GetMsg()->present = PokerTHMessage_PR_voteKickReplyMessage;
	VoteKickReplyMessage_t *netVoteReply = &packet->GetMsg()->choice.voteKickReplyMessage;
	netVoteReply->gameId = GetId();
	netVoteReply->petitionId = petitionId;
	netVoteReply->voteKickReplyType.present = voteKickReplyType_PR_voteKickDenied;

	VoteKickDenied_t *kickDenied = &netVoteReply->voteKickReplyType.choice.voteKickDenied;
	kickDenied->voteKickDeniedReason = reason;
	GetLobbyThread().GetSender().Send(byWhom.sessionData, packet);
}

PlayerDataList
ServerGame::GetFullPlayerDataList() const
{
	PlayerDataList playerList(GetSessionManager().GetPlayerDataList());
	boost::mutex::scoped_lock lock(m_computerPlayerListMutex);
	copy(m_computerPlayerList.begin(), m_computerPlayerList.end(), back_inserter(playerList));

	return playerList;
}

boost::shared_ptr<PlayerData>
ServerGame::GetPlayerDataByUniqueId(unsigned playerId) const
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
ServerGame::GetPlayerIdList() const
{
	PlayerIdList idList(GetSessionManager().GetPlayerIdList(SessionData::Game));
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
ServerGame::IsPlayerConnected(const std::string &name) const
{
	return GetSessionManager().IsPlayerConnected(name);
}

bool
ServerGame::IsRunning() const
{
	return m_game.get() != NULL;
}

unsigned
ServerGame::GetAdminPlayerId() const
{
	return m_adminPlayerId;
}

void
ServerGame::SetAdminPlayerId(unsigned playerId)
{
	m_adminPlayerId = playerId;
}

void
ServerGame::AddComputerPlayer(boost::shared_ptr<PlayerData> player)
{
	{
		boost::mutex::scoped_lock lock(m_computerPlayerListMutex);
		m_computerPlayerList.push_back(player);
	}
	GetLobbyThread().AddComputerPlayer(player);
}

boost::shared_ptr<PlayerData>
ServerGame::RemoveComputerPlayer(unsigned playerId)
{
	boost::shared_ptr<PlayerData> tmpPlayer;
	{
		boost::mutex::scoped_lock lock(m_computerPlayerListMutex);
		PlayerDataList::iterator i = m_computerPlayerList.begin();
		PlayerDataList::iterator end = m_computerPlayerList.end();
		while (i != end)
		{
			if ((*i)->GetUniqueId() == playerId)
			{
				tmpPlayer = *i;
				m_computerPlayerList.erase(i);
				break;
			}
			++i;
		}
	}
	GetLobbyThread().RemoveComputerPlayer(tmpPlayer);
	return tmpPlayer;
}

bool
ServerGame::IsComputerPlayerActive(unsigned playerId) const
{
	bool retVal = false;
	boost::mutex::scoped_lock lock(m_computerPlayerListMutex);
	PlayerDataList::const_iterator i = m_computerPlayerList.begin();
	PlayerDataList::const_iterator end = m_computerPlayerList.end();
	while (i != end)
	{
		if ((*i)->GetUniqueId() == playerId)
			retVal = true;
		++i;
	}
	return retVal;
}

void
ServerGame::ResetComputerPlayerList()
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
ServerGame::GracefulRemoveSession(SessionWrapper session, int reason)
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
ServerGame::RemovePlayerData(boost::shared_ptr<PlayerData> player, int reason)
{
	if (player->IsGameAdmin())
	{
		// Find new admin for the game
		PlayerDataList playerList(GetSessionManager().GetPlayerDataList());
		if (!playerList.empty())
		{
			boost::shared_ptr<PlayerData> newAdmin = playerList.front();
			SetAdminPlayerId(newAdmin->GetUniqueId());
			newAdmin->SetGameAdmin(true);
			// Notify game state on admin change
			GetState().NotifyGameAdminChanged(shared_from_this());
			// Send "Game Admin Changed" to clients.
			boost::shared_ptr<NetPacket> adminChanged(new NetPacket(NetPacket::Alloc));
			adminChanged->GetMsg()->present = PokerTHMessage_PR_gamePlayerMessage;
			GamePlayerMessage_t *netGameAdmin = &adminChanged->GetMsg()->choice.gamePlayerMessage;
			netGameAdmin->gameId = GetId();

			netGameAdmin->gamePlayerNotification.present = gamePlayerNotification_PR_gameAdminChanged;
			GameAdminChanged_t *gameAdmin = &netGameAdmin->gamePlayerNotification.choice.gameAdminChanged;
			gameAdmin->newAdminPlayerId = newAdmin->GetUniqueId(); // Choose next player as admin.
			GetSessionManager().SendToAllSessions(GetLobbyThread().GetSender(), adminChanged, SessionData::Game);

			GetLobbyThread().NotifyGameAdminChanged(GetId(), newAdmin->GetUniqueId());
		}
	}
	// Reset player rights.
	player->SetGameAdmin(false);

	// Send "Player Left" to clients.
	boost::shared_ptr<NetPacket> thisPlayerLeft(new NetPacket(NetPacket::Alloc));
	thisPlayerLeft->GetMsg()->present = PokerTHMessage_PR_gamePlayerMessage;
	GamePlayerMessage_t *netPlayerLeft = &thisPlayerLeft->GetMsg()->choice.gamePlayerMessage;
	netPlayerLeft->gameId = GetId();

	netPlayerLeft->gamePlayerNotification.present = gamePlayerNotification_PR_gamePlayerLeft;
	GamePlayerLeft_t *gamePlayer = &netPlayerLeft->gamePlayerNotification.choice.gamePlayerLeft;
	gamePlayer->playerId = player->GetUniqueId();
	switch (reason)
	{
		case NTF_NET_REMOVED_ON_REQUEST :
			gamePlayer->gamePlayerLeftReason = gamePlayerLeftReason_leftOnRequest;
			break;
		case NTF_NET_REMOVED_KICKED :
			gamePlayer->gamePlayerLeftReason = gamePlayerLeftReason_leftKicked;
			break;
		default :
			gamePlayer->gamePlayerLeftReason = gamePlayerLeftReason_leftError;
			break;
	}
	GetSessionManager().SendToAllSessions(GetLobbyThread().GetSender(), thisPlayerLeft, SessionData::Game);

	GetLobbyThread().NotifyPlayerLeftGame(GetId(), player->GetUniqueId());
}

void
ServerGame::ErrorRemoveSession(SessionWrapper session)
{
	GetLobbyThread().RemoveSessionFromGame(session);
	GracefulRemoveSession(session, NTF_NET_INTERNAL);
}

void
ServerGame::SessionError(SessionWrapper session, int errorCode)
{
	if (!session.sessionData.get())
		throw ServerException(__FILE__, __LINE__, ERR_NET_INVALID_SESSION, 0);
	ErrorRemoveSession(session);
	GetLobbyThread().SessionError(session, errorCode);
}

void
ServerGame::MoveSessionToLobby(SessionWrapper session, int reason)
{
	GracefulRemoveSession(session, reason);
	// Reset ready flag - just in case it is set, player may leave at any time.
	session.sessionData->ResetReadyFlag();
	GetLobbyThread().ReAddSession(session, reason);
}

void
ServerGame::RemoveDisconnectedPlayers()
{
	// This should only be called between hands.
	if (m_game.get())
	{
		PlayerListIterator i = m_game->getSeatsList()->begin();
		PlayerListIterator end = m_game->getSeatsList()->end();
		while (i != end)
		{
			boost::shared_ptr<PlayerInterface> tmpPlayer = *i;
			if ((tmpPlayer->getMyType() == PLAYER_TYPE_HUMAN && !GetSessionManager().IsPlayerConnected(tmpPlayer->getMyUniqueID()))
				|| (tmpPlayer->getMyType() == PLAYER_TYPE_COMPUTER && !IsComputerPlayerActive(tmpPlayer->getMyUniqueID())))
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
ServerGame::GetCurNumberOfPlayers() const
{
	return GetFullPlayerDataList().size();
}

void
ServerGame::AssignPlayerNumbers()
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

bool
ServerGame::IsValidPlayer(unsigned playerId) const
{
	bool retVal = false;
	const PlayerIdList list(GetPlayerIdList());
	if (find(list.begin(), list.end(), playerId) != list.end())
		retVal = true;
	return retVal;
}

SessionManager &
ServerGame::GetSessionManager()
{
	return m_sessionManager;
}

const SessionManager &
ServerGame::GetSessionManager() const
{
	return m_sessionManager;
}

ServerLobbyThread &
ServerGame::GetLobbyThread()
{
	assert(m_lobbyThread);
	return *m_lobbyThread;
}

ServerCallback &
ServerGame::GetCallback()
{
	return m_gui;
}

ServerGameState &
ServerGame::GetState()
{
	assert(m_curState);
	return *m_curState;
}

void
ServerGame::SetState(ServerGameState &newState)
{
	if (m_curState)
		m_curState->Exit(shared_from_this());
	m_curState = &newState;
	m_curState->Enter(shared_from_this());
}

boost::asio::deadline_timer &
ServerGame::GetStateTimer()
{
	return m_stateTimer;
}

ReceiverHelper &
ServerGame::GetReceiver()
{
	assert(m_receiver.get());
	return *m_receiver;
}

Game &
ServerGame::GetGame()
{
	assert(m_game.get());
	return *m_game;
}

const Game &
ServerGame::GetGame() const
{
	assert(m_game.get());
	return *m_game;
}

const GameData &
ServerGame::GetGameData() const
{
	return m_gameData;
}

const StartData &
ServerGame::GetStartData() const
{
	return m_startData;
}

void
ServerGame::SetStartData(const StartData &startData)
{
	m_startData = startData;
}

bool
ServerGame::IsPasswordProtected() const
{
	return !m_password.empty();
}

bool
ServerGame::CheckPassword(const string &password) const
{
	return (password == m_password);
}

GuiInterface &
ServerGame::GetGui()
{
	return m_gui;
}

unsigned
ServerGame::GetNextGameNum()
{
	return m_gameNum++;
}

