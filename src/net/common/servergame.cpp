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

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <algorithm>

#include <net/servergame.h>
#include <net/servergamestate.h>
#include <net/serverlobbythread.h>
#include <net/serverexception.h>
#include <net/senderhelper.h>
#include <net/socket_msg.h>
#include <core/loghelper.h>
#include <db/serverdbinterface.h>
#include <db/serverdbnoaction.h>
#include <game.h>
#include <localenginefactory.h>
#include <tools.h>
#include <configfile.h>


#define SERVER_CHECK_VOTE_KICK_INTERVAL_MSEC	500
#define SERVER_KICK_TIMEOUT_ADD_DELAY_SEC		2

using namespace std;

#ifdef BOOST_ASIO_HAS_STD_CHRONO
using namespace std::chrono;
#else
using namespace boost::chrono;
#endif

static bool LessThanPlayerHandStartMoney(const boost::shared_ptr<PlayerInterface> p1, const boost::shared_ptr<PlayerInterface> p2)
{
	return p1->getMyRoundStartCash() < p2->getMyRoundStartCash();
}


ServerGame::ServerGame(boost::shared_ptr<ServerLobbyThread> lobbyThread, u_int32_t id, const string &name, const string &pwd, const GameData &gameData,
					   unsigned adminPlayerId, unsigned creatorPlayerDBId, GuiInterface &gui, ConfigFile &playerConfig)
	: m_adminPlayerId(adminPlayerId), m_lobbyThread(lobbyThread), m_gui(gui),
	  m_gameData(gameData), m_curState(NULL), m_id(id), m_name(name),
	  m_password(pwd), m_creatorPlayerDBId(creatorPlayerDBId), m_playerConfig(playerConfig),
	  m_gameNum(1), m_curPetitionId(1), m_voteKickTimer(lobbyThread->GetIOService()),
	  m_stateTimer1(lobbyThread->GetIOService()), m_stateTimer2(lobbyThread->GetIOService()),
	  m_isNameReported(false)
{
	LOG_VERBOSE("Game object " << GetId() << " created.");
}

ServerGame::~ServerGame()
{
	LOG_VERBOSE("Game object " << GetId() << " destructed.");
}

void
ServerGame::Init()
{
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

unsigned
ServerGame::GetCreatorDBId() const
{
	return m_creatorPlayerDBId;
}

void
ServerGame::AddSession(boost::shared_ptr<SessionData> session, bool spectateOnly)
{
	if (session) {
		if (spectateOnly) {
			GetState().HandleNewSpectator(shared_from_this(), session);
		} else {
			GetState().HandleNewPlayer(shared_from_this(), session);
		}
	}
}

void
ServerGame::RemovePlayer(unsigned playerId, unsigned errorCode)
{
	boost::shared_ptr<SessionData> tmpSession = GetSessionManager().GetSessionByUniquePlayerId(playerId);
	// Only kick if the player was found.
	if (tmpSession)
		SessionError(tmpSession, errorCode);
}

void
ServerGame::MutePlayer(unsigned playerId, bool mute)
{
	if (m_game) {
		boost::shared_ptr<PlayerInterface> tmpPlayer(m_game->getPlayerByUniqueId(playerId));
		if (tmpPlayer) {
			tmpPlayer->setIsMuted(mute);
		}
	}
}

void
ServerGame::MarkPlayerAsInactive(unsigned playerId)
{
	if (m_game) {
		boost::shared_ptr<PlayerInterface> tmpPlayer(m_game->getPlayerByUniqueId(playerId));
		if (tmpPlayer) {
			tmpPlayer->setIsSessionActive(false);
		}
	}
}

void
ServerGame::MarkPlayerAsKicked(unsigned playerId)
{
	// Mark the player as kicked in the engine.
	if (m_game) {
		boost::shared_ptr<PlayerInterface> tmpPlayer(m_game->getPlayerByUniqueId(playerId));
		if (tmpPlayer) {
			// Player was kicked, so he is not allowed to rejoin.
			tmpPlayer->setIsKicked(true);
			tmpPlayer->setMyGuid("");
		}
	}
}

void
ServerGame::HandlePacket(boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet)
{
	if (session && packet)
		GetState().ProcessPacket(shared_from_this(), session, packet);
}

GameState
ServerGame::GetCurRound() const
{
	return static_cast<GameState>(GetGame().getCurrentHand()->getCurrentRound());
}

void
ServerGame::SendToAllPlayers(boost::shared_ptr<NetPacket> packet, int state)
{
	GetSessionManager().SendToAllSessions(GetLobbyThread().GetSender(), packet, state);
}

void
ServerGame::SendToAllButOnePlayers(boost::shared_ptr<NetPacket> packet, SessionId except, int state)
{
	GetSessionManager().SendToAllButOneSessions(GetLobbyThread().GetSender(), packet, except, state);
}

void
ServerGame::RemoveAllSessions()
{
	// Clean up ALL sessions which are left.
	GetSessionManager().ForEach(&SessionData::Close);
	GetSessionManager().Clear();
	SetState(ServerGameStateFinal::Instance());
}

void
ServerGame::MoveSpectatorsToLobby()
{
	PlayerIdList spectatorList = GetSpectatorIdList();
	PlayerIdList::const_iterator i = spectatorList.begin();
	PlayerIdList::const_iterator end = spectatorList.end();
	while (i != end) {
		boost::shared_ptr<SessionData> tmpSession = GetSessionManager().GetSessionByUniquePlayerId(*i);
		// Only remove if the spectator was found.
		if (tmpSession)
			MoveSessionToLobby(tmpSession, NTF_NET_REMOVED_GAME_CLOSED);
		++i;
	}
}

void
ServerGame::TimerVoteKick(const boost::system::error_code &ec)
{
	if (!ec && m_curState != &ServerGameStateFinal::Instance()) {
		// Check whether someone should be kicked, or whether a vote kick should be aborted.
		// Only one vote kick can be active at a time.
		if (m_voteKickData) {
			// Prepare some values.
			const PlayerIdList playerIds(GetPlayerIdList());
			int votesRequiredToKick = m_voteKickData->numVotesToKick - m_voteKickData->numVotesInFavourOfKicking;
			int playersAllowedToVote = 0;
			// We need to count the number of players which are still allowed to vote.
			PlayerIdList::const_iterator player_i = playerIds.begin();
			PlayerIdList::const_iterator player_end = playerIds.end();
			while (player_i != player_end) {
				if (find(m_voteKickData->votedPlayerIds.begin(), m_voteKickData->votedPlayerIds.end(), *player_i) == m_voteKickData->votedPlayerIds.end())
					playersAllowedToVote++;
				++player_i;
			}
			bool abortPetition = false;
			bool doKick = false;
			EndPetitionReason reason;

			// 1. Enough votes to kick the player.
			if (m_voteKickData->numVotesInFavourOfKicking >= m_voteKickData->numVotesToKick) {
				reason = PETITION_END_ENOUGH_VOTES;
				abortPetition = true;
				doKick = true;
			}
			// 2. Several players left the game, so a kick is no longer possible.
			else if (votesRequiredToKick > playersAllowedToVote) {
				reason = PETITION_END_NOT_ENOUGH_PLAYERS;
				abortPetition = true;
			}
			// 3. The kick has become invalid because the player to be kicked left.
			else if (!IsValidPlayer(m_voteKickData->kickPlayerId)) {
				reason = PETITION_END_PLAYER_LEFT;
				abortPetition = true;
			}
			// 4. A kick request timed out (because not everyone voted).
			else if (m_voteKickData->voteTimer.elapsed().total_seconds() >= m_voteKickData->timeLimitSec) {
				reason = PETITION_END_TIMEOUT;
				abortPetition = true;
			}
			if (abortPetition) {
				boost::shared_ptr<NetPacket> packet(new NetPacket);
				packet->GetMsg()->set_messagetype(PokerTHMessage::Type_EndKickPetitionMessage);
				EndKickPetitionMessage *netEndPetition = packet->GetMsg()->mutable_endkickpetitionmessage();
				netEndPetition->set_gameid(GetId());
				netEndPetition->set_petitionid(m_voteKickData->petitionId);
				netEndPetition->set_numvotesagainstkicking(m_voteKickData->numVotesAgainstKicking);
				netEndPetition->set_numvotesinfavourofkicking(m_voteKickData->numVotesInFavourOfKicking);
				netEndPetition->set_resultplayerkicked(doKick);
				netEndPetition->set_petitionendreason(static_cast<EndKickPetitionMessage::PetitionEndReason>(reason));
				SendToAllPlayers(packet, SessionData::Game);

				// Perform kick.
				if (doKick)
					KickPlayer(m_voteKickData->kickPlayerId);
				// This petition has ended.
				m_voteKickData.reset();
			}
			m_voteKickTimer.expires_from_now(
				milliseconds(SERVER_CHECK_VOTE_KICK_INTERVAL_MSEC));
			m_voteKickTimer.async_wait(
				boost::bind(
					&ServerGame::TimerVoteKick, shared_from_this(), boost::asio::placeholders::error));
		}
	}
}

PlayerDataList
ServerGame::InternalStartGame()
{
	// Initialize the game.
	PlayerDataList playerData(GetFullPlayerDataList());

	if (playerData.size() >= 2) {
		// Set DB Backend.
		if (GetGameData().gameType == GAME_TYPE_RANKING)
			m_database = GetLobbyThread().GetDatabase();
		else
			m_database.reset(new ServerDBNoAction);

		// Randomize player list.
		// Note: This does not use a cryptographically strong
		// random number generator.
		vector<boost::shared_ptr<PlayerData> > tmpData(playerData.begin(), playerData.end());
		random_shuffle(tmpData.begin(), tmpData.end());
		copy(tmpData.begin(), tmpData.end(), playerData.begin());

		// Set order of players.
		AssignPlayerNumbers(playerData);

		// Create EngineFactory
		boost::shared_ptr<EngineFactory> factory(new LocalEngineFactory(&m_playerConfig)); // LocalEngine erstellen

		// Set start data.
		StartData startData;
		startData.numberOfPlayers = (int)playerData.size();

		int tmpDealerPos = 0;
		Tools::GetRand(0, startData.numberOfPlayers-1, 1, &tmpDealerPos);
		// The Player Id is not continuous. Therefore, the start dealer position
		// needs to be converted to a player Id, and cannot be directly generated
		// as player Id.
		PlayerDataList::const_iterator player_i = playerData.begin();
		PlayerDataList::const_iterator player_end = playerData.end();

		int tmpPos = 0;
		while (player_i != player_end) {
			startData.startDealerPlayerId = static_cast<unsigned>((*player_i)->GetUniqueId());
			if (tmpPos == tmpDealerPos)
				break;
			++tmpPos;
			++player_i;
		}
		if (player_i == player_end)
			throw ServerException(__FILE__, __LINE__, ERR_NET_DEALER_NOT_FOUND, 0);

		SetStartData(startData);

		GuiInterface &gui = GetGui();
		m_game.reset(new Game(&gui, factory, playerData, GetGameData(), GetStartData(), GetNextGameNum(), NULL));

		GetDatabase().AsyncCreateGame(GetId(), GetName());
		InitRankingMap(playerData);
	}
	return playerData;
}

void
ServerGame::InitRankingMap(const PlayerDataList &playerDataList)
{
	PlayerDataList::const_iterator i = playerDataList.begin();
	PlayerDataList::const_iterator end = playerDataList.end();
	while (i != end) {
		boost::shared_ptr<PlayerData> tmpPlayer(*i);
		RankingData tmpData(tmpPlayer->GetDBId());
		m_rankingMap[tmpPlayer->GetUniqueId()] = tmpData;
		++i;
	}
}

void
ServerGame::UpdateRankingMap()
{
	list<boost::shared_ptr<PlayerInterface> > activePlayers = *m_game->getActivePlayerList();
	int currentRank = static_cast<int>(activePlayers.size());
	list<boost::shared_ptr<PlayerInterface> > tmpRemovedPlayers;
	PlayerListIterator active_i = activePlayers.begin();
	PlayerListIterator active_end = activePlayers.end();
	PlayerListIterator next_active_i = active_i;
	while(active_i != active_end) {
		++next_active_i;
		if ((*active_i)->getMyCash() < 1) {
			tmpRemovedPlayers.push_back(*active_i);
			activePlayers.erase(active_i);
		}
		active_i = next_active_i;
	}

	if (!tmpRemovedPlayers.empty()) {
		tmpRemovedPlayers.sort(LessThanPlayerHandStartMoney);
		PlayerListConstIterator removed_i = tmpRemovedPlayers.begin();
		PlayerListConstIterator removed_end = tmpRemovedPlayers.end();
		PlayerListConstIterator next_removed_i = removed_i;
		int currentRankCounter = 0;
		while (removed_i != removed_end) {
			++next_removed_i;

			SetPlayerPlace((*removed_i)->getMyUniqueID(), currentRank);
			++currentRankCounter;
			if (next_removed_i != removed_end && (*removed_i)->getMyRoundStartCash() < (*next_removed_i)->getMyRoundStartCash()) {
				currentRank -= currentRankCounter;
				currentRankCounter = 0;
			}

			removed_i = next_removed_i;
		}
	}
	// Last player is winner.
	if (activePlayers.size() == 1) {
		SetPlayerPlace((*(activePlayers.begin()))->getMyUniqueID(), 1);
	}
}

void
ServerGame::SetPlayerPlace(unsigned playerId, int place)
{
	RankingMap::iterator pos = m_rankingMap.find(playerId);
	if (pos != m_rankingMap.end()) {
		(*pos).second.place = place;
	}
}

void
ServerGame::ReplaceRankingPlayer(unsigned oldPlayerId, unsigned newPlayerId)
{
	RankingMap::iterator pos = m_rankingMap.find(oldPlayerId);
	if (pos != m_rankingMap.end()) {
		RankingData tmpData((*pos).second);
		m_rankingMap[newPlayerId] = tmpData;
		m_rankingMap.erase(pos);
	}
}

void
ServerGame::StoreAndResetRanking()
{
	// Store players in database.
	RankingMap::const_iterator i = m_rankingMap.begin();
	RankingMap::const_iterator end = m_rankingMap.end();
	while (i != end) {
		if ((*i).second.dbid != DB_ID_INVALID) {
			GetDatabase().SetGamePlayerPlace(GetId(), (*i).second.dbid, (*i).second.place);
		}
		++i;
	}
	GetDatabase().EndGame(GetId());
	m_rankingMap.clear();
}

void
ServerGame::RemoveAutoLeavePlayers()
{
	boost::mutex::scoped_lock lock(m_autoLeavePlayerListMutex);
	PlayerIdList::const_iterator i = m_autoLeavePlayerList.begin();
	PlayerIdList::const_iterator end = m_autoLeavePlayerList.end();
	while (i != end) {
		boost::shared_ptr<SessionData> tmpSession = GetSessionManager().GetSessionByUniquePlayerId(*i);
		// Only remove if the player was found.
		if (tmpSession)
			MoveSessionToLobby(tmpSession, NTF_NET_REMOVED_ON_REQUEST);
		++i;
	}
	m_autoLeavePlayerList.clear();
}

void
ServerGame::InternalEndGame()
{
	StoreAndResetRanking();
	m_game.reset();
	m_numJoinsPerPlayer.clear();
}

void
ServerGame::KickPlayer(unsigned playerId)
{
	MarkPlayerAsKicked(playerId);

	// Kick the network session from this game.
	boost::shared_ptr<SessionData> tmpSession(GetSessionManager().GetSessionByUniquePlayerId(playerId));
	// Only kick if the player was found.
	if (tmpSession) {
		MoveSessionToLobby(tmpSession, NTF_NET_REMOVED_KICKED);
	}
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
ServerGame::InternalAskVoteKick(boost::shared_ptr<SessionData> byWhom, unsigned playerIdWho, unsigned timeoutSec)
{
	if (IsRunning() && byWhom->GetPlayerData()) {
		// Retrieve only the number of human players.
		size_t numPlayers = GetSessionManager().GetPlayerIdList(SessionData::Game).size();
		if (numPlayers > 2) {
			// Check whether the player to be kicked exists.
			if (IsValidPlayer(playerIdWho)) {
				// Lock the vote kick data.
				if (!m_voteKickData) {
					// Initiate a vote kick.
					unsigned playerIdByWhom = byWhom->GetPlayerData()->GetUniqueId();
					m_voteKickData.reset(new VoteKickData);
					m_voteKickData->petitionId = m_curPetitionId++;
					m_voteKickData->kickPlayerId = playerIdWho;
					m_voteKickData->numVotesToKick = static_cast<int>(ceil(numPlayers / 3. * 2.));
					m_voteKickData->timeLimitSec = timeoutSec + SERVER_KICK_TIMEOUT_ADD_DELAY_SEC;
					// Consider first vote.
					m_voteKickData->numVotesInFavourOfKicking = 1;
					m_voteKickData->votedPlayerIds.push_back(playerIdByWhom);

					boost::shared_ptr<NetPacket> packet(new NetPacket);
					packet->GetMsg()->set_messagetype(PokerTHMessage::Type_StartKickPetitionMessage);
					StartKickPetitionMessage *netStartPetition = packet->GetMsg()->mutable_startkickpetitionmessage();
					netStartPetition->set_gameid(GetId());
					netStartPetition->set_petitionid(m_voteKickData->petitionId);
					netStartPetition->set_proposingplayerid(playerIdByWhom);
					netStartPetition->set_kickplayerid(m_voteKickData->kickPlayerId);
					netStartPetition->set_kicktimeoutsec(timeoutSec);
					netStartPetition->set_numvotesneededtokick(m_voteKickData->numVotesToKick);
					SendToAllPlayers(packet, SessionData::Game);

					m_voteKickTimer.expires_from_now(
						milliseconds(SERVER_CHECK_VOTE_KICK_INTERVAL_MSEC));
					m_voteKickTimer.async_wait(
						boost::bind(
							&ServerGame::TimerVoteKick, shared_from_this(), boost::asio::placeholders::error));

				} else
					InternalDenyAskVoteKick(byWhom, playerIdWho, KICK_DENIED_OTHER_IN_PROGRESS);
			} else
				InternalDenyAskVoteKick(byWhom, playerIdWho, KICK_DENIED_INVALID_PLAYER_ID);
		} else
			InternalDenyAskVoteKick(byWhom, playerIdWho, KICK_DENIED_TOO_FEW_PLAYERS);
	} else
		InternalDenyAskVoteKick(byWhom, playerIdWho, KICK_DENIED_INVALID_STATE);
}

void
ServerGame::InternalDenyAskVoteKick(boost::shared_ptr<SessionData> byWhom, unsigned playerIdWho, DenyKickPlayerReason reason)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_AskKickDeniedMessage);
	AskKickDeniedMessage *netKickDenied = packet->GetMsg()->mutable_askkickdeniedmessage();
	netKickDenied->set_gameid(GetId());
	netKickDenied->set_playerid(playerIdWho);
	netKickDenied->set_kickdeniedreason(static_cast<AskKickDeniedMessage::KickDeniedReason>(reason));
	GetLobbyThread().GetSender().Send(byWhom, packet);
}

void
ServerGame::InternalVoteKick(boost::shared_ptr<SessionData> byWhom, unsigned petitionId, KickVote vote)
{
	if (IsRunning() && byWhom->GetPlayerData()) {
		// Check whether this is the valid petition id.
		if (m_voteKickData && m_voteKickData->petitionId == petitionId) {
			// Check whether the player already voted.
			unsigned playerId = byWhom->GetPlayerData()->GetUniqueId();
			if (find(m_voteKickData->votedPlayerIds.begin(), m_voteKickData->votedPlayerIds.end(), playerId) == m_voteKickData->votedPlayerIds.end()) {
				m_voteKickData->votedPlayerIds.push_back(playerId);
				if (vote == KICK_VOTE_IN_FAVOUR)
					m_voteKickData->numVotesInFavourOfKicking++;
				else
					m_voteKickData->numVotesAgainstKicking++;
				// Send update notification.
				boost::shared_ptr<NetPacket> packet(new NetPacket);
				packet->GetMsg()->set_messagetype(PokerTHMessage::Type_KickPetitionUpdateMessage);
				KickPetitionUpdateMessage *netKickUpdate = packet->GetMsg()->mutable_kickpetitionupdatemessage();
				netKickUpdate->set_gameid(GetId());
				netKickUpdate->set_petitionid(m_voteKickData->petitionId);
				netKickUpdate->set_numvotesagainstkicking(m_voteKickData->numVotesAgainstKicking);
				netKickUpdate->set_numvotesinfavourofkicking(m_voteKickData->numVotesInFavourOfKicking);
				netKickUpdate->set_numvotesneededtokick(m_voteKickData->numVotesToKick);
				SendToAllPlayers(packet, SessionData::Game);
			} else
				InternalDenyVoteKick(byWhom, petitionId, VOTE_DENIED_ALREADY_VOTED);
		} else
			InternalDenyVoteKick(byWhom, petitionId, VOTE_DENIED_INVALID_PETITION);
	} else
		InternalDenyVoteKick(byWhom, petitionId, VOTE_DENIED_INVALID_PETITION);
}

void
ServerGame::InternalDenyVoteKick(boost::shared_ptr<SessionData> byWhom, unsigned petitionId, DenyVoteReason reason)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_VoteKickReplyMessage);
	VoteKickReplyMessage *netVoteReply = packet->GetMsg()->mutable_votekickreplymessage();
	netVoteReply->set_gameid(GetId());
	netVoteReply->set_petitionid(petitionId);
	switch(reason) {
	case VOTE_DENIED_ALREADY_VOTED :
		netVoteReply->set_votekickreplytype(VoteKickReplyMessage::voteKickDeniedAlreadyVoted);
		break;
	default:
		netVoteReply->set_votekickreplytype(VoteKickReplyMessage::voteKickDeniedInvalid);
		break;
	}
	GetLobbyThread().GetSender().Send(byWhom, packet);
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
	boost::shared_ptr<SessionData> session = GetSessionManager().GetSessionByUniquePlayerId(playerId);
	if (session) {
		tmpPlayer = session->GetPlayerData();
	}
	if (!tmpPlayer) {
		boost::mutex::scoped_lock lock(m_computerPlayerListMutex);
		PlayerDataList::const_iterator i = m_computerPlayerList.begin();
		PlayerDataList::const_iterator end = m_computerPlayerList.end();
		while (i != end) {
			if ((*i)->GetUniqueId() == playerId) {
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
	while (i != end) {
		idList.push_back((*i)->GetUniqueId());
		++i;
	}

	return idList;
}

PlayerIdList
ServerGame::GetSpectatorIdList() const
{
	return GetSessionManager().GetPlayerIdList(SessionData::Spectating | SessionData::SpectatorWaiting);
}

bool
ServerGame::IsPlayerConnected(const std::string &name) const
{
	return GetSessionManager().IsPlayerConnected(name);
}

bool
ServerGame::IsPlayerConnected(unsigned playerId) const
{
	return GetSessionManager().IsPlayerConnected(playerId);
}

bool
ServerGame::IsClientAddressConnected(const std::string &clientAddress) const
{
	return GetSessionManager().IsClientAddressConnected(clientAddress);
}

boost::shared_ptr<PlayerInterface>
ServerGame::GetPlayerInterfaceFromGame(const std::string &playerName)
{
	boost::shared_ptr<PlayerInterface> tmpPlayer;
	if (m_game) {
		tmpPlayer = m_game->getPlayerByName(playerName);
	}
	return tmpPlayer;
}

boost::shared_ptr<PlayerInterface>
ServerGame::GetPlayerInterfaceFromGame(unsigned playerId)
{
	boost::shared_ptr<PlayerInterface> tmpPlayer;
	if (m_game) {
		tmpPlayer = m_game->getPlayerByUniqueId(playerId);
	}
	return tmpPlayer;
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
ServerGame::AddPlayerInvitation(unsigned playerId)
{
	boost::mutex::scoped_lock lock(m_playerInvitationListMutex);
	m_playerInvitationList.push_back(playerId);
	m_playerInvitationList.sort();
	m_playerInvitationList.unique();
}

void
ServerGame::RemovePlayerInvitation(unsigned playerId)
{
	boost::mutex::scoped_lock lock(m_playerInvitationListMutex);
	m_playerInvitationList.remove(playerId);
}

bool
ServerGame::IsPlayerInvited(unsigned playerId) const
{
	bool retVal = false;
	boost::mutex::scoped_lock lock(m_playerInvitationListMutex);
	PlayerIdList::const_iterator pos = find(m_playerInvitationList.begin(), m_playerInvitationList.end(), playerId);
	if (pos != m_playerInvitationList.end())
		retVal = true;
	return retVal;
}

void
ServerGame::SetPlayerAutoLeaveOnFinish(unsigned playerId)
{
	boost::mutex::scoped_lock lock(m_autoLeavePlayerListMutex);
	m_autoLeavePlayerList.push_back(playerId);
}

void
ServerGame::AddRejoinPlayer(unsigned playerId)
{
	boost::mutex::scoped_lock lock(m_rejoinPlayerListMutex);
	m_rejoinPlayerList.push_back(playerId);
}

PlayerIdList
ServerGame::GetAndResetRejoinPlayers()
{
	boost::mutex::scoped_lock lock(m_rejoinPlayerListMutex);
	PlayerIdList tmpList(m_rejoinPlayerList);
	m_rejoinPlayerList.clear();
	return tmpList;
}

void
ServerGame::AddReactivatePlayer(unsigned playerId)
{
	boost::mutex::scoped_lock lock(m_reactivatePlayerListMutex);
	m_reactivatePlayerList.push_back(playerId);
}

PlayerIdList
ServerGame::GetAndResetReactivatePlayers()
{
	boost::mutex::scoped_lock lock(m_reactivatePlayerListMutex);
	PlayerIdList tmpList(m_reactivatePlayerList);
	m_reactivatePlayerList.clear();
	return tmpList;
}

void
ServerGame::AddNewSpectator(unsigned playerId)
{
	boost::mutex::scoped_lock lock(m_newSpectatorListMutex);
	m_newSpectatorList.push_back(playerId);
}

PlayerIdList
ServerGame::GetAndResetNewSpectators()
{
	boost::mutex::scoped_lock lock(m_newSpectatorListMutex);
	PlayerIdList tmpList(m_newSpectatorList);
	m_newSpectatorList.clear();
	return tmpList;
}

void
ServerGame::SetNameReported()
{
	m_isNameReported = true;
}

bool
ServerGame::IsNameReported() const
{
	return m_isNameReported;
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
		while (i != end) {
			if ((*i)->GetUniqueId() == playerId) {
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
	while (i != end) {
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

	while (i != end) {
		GetLobbyThread().RemoveComputerPlayer(*i);
		RemovePlayerData(*i, NTF_NET_REMOVED_ON_REQUEST, false);
		++i;
	}

	m_computerPlayerList.clear();
}

void
ServerGame::RemoveSession(boost::shared_ptr<SessionData> session, int reason)
{
	if (!session)
		throw ServerException(__FILE__, __LINE__, ERR_NET_INVALID_SESSION, 0);

	if (GetSessionManager().RemoveSession(session->GetId())) {
		boost::shared_ptr<PlayerData> tmpPlayerData = session->GetPlayerData();
		if (tmpPlayerData && !tmpPlayerData->GetName().empty()) {
			RemovePlayerData(tmpPlayerData, reason, session->GetState() == SessionData::Spectating || session->GetState() == SessionData::SpectatorWaiting);
		}
	}
}

void
ServerGame::RemovePlayerData(boost::shared_ptr<PlayerData> player, int reason, bool spectateOnly)
{
	if (player->IsGameAdmin()) {
		// Find new admin for the game
		PlayerDataList playerList(GetSessionManager().GetPlayerDataList());
		if (!playerList.empty()) {
			boost::shared_ptr<PlayerData> newAdmin = playerList.front();
			SetAdminPlayerId(newAdmin->GetUniqueId());
			newAdmin->SetGameAdmin(true);
			// Notify game state on admin change
			GetState().NotifyGameAdminChanged(shared_from_this());
			// Send "Game Admin Changed" to clients.
			boost::shared_ptr<NetPacket> adminChanged(new NetPacket);
			adminChanged->GetMsg()->set_messagetype(PokerTHMessage::Type_GameAdminChangedMessage);
			GameAdminChangedMessage *netGameAdmin = adminChanged->GetMsg()->mutable_gameadminchangedmessage();
			netGameAdmin->set_gameid(GetId());
			netGameAdmin->set_newadminplayerid(newAdmin->GetUniqueId()); // Choose next player as admin.
			GetSessionManager().SendToAllSessions(GetLobbyThread().GetSender(), adminChanged, SessionData::Game);

			GetLobbyThread().NotifyGameAdminChanged(GetId(), newAdmin->GetUniqueId());
		}
	}
	// Reset player rights.
	player->SetGameAdmin(false);

	// Send "Player Left" to clients.
	boost::shared_ptr<NetPacket> thisPlayerLeft(new NetPacket);
	GamePlayerLeftMessage::GamePlayerLeftReason netReason = GamePlayerLeftMessage::leftError;
	switch (reason) {
	case NTF_NET_REMOVED_ON_REQUEST :
		netReason = GamePlayerLeftMessage::leftOnRequest;
		break;
	case NTF_NET_REMOVED_KICKED :
		netReason = GamePlayerLeftMessage::leftKicked;
		break;
	}

	if (spectateOnly) {
		thisPlayerLeft->GetMsg()->set_messagetype(PokerTHMessage::Type_GameSpectatorLeftMessage);
		GameSpectatorLeftMessage *netPlayerLeft = thisPlayerLeft->GetMsg()->mutable_gamespectatorleftmessage();
		netPlayerLeft->set_gameid(GetId());
		netPlayerLeft->set_playerid(player->GetUniqueId());
		netPlayerLeft->set_gamespectatorleftreason(netReason);
	} else {
		thisPlayerLeft->GetMsg()->set_messagetype(PokerTHMessage::Type_GamePlayerLeftMessage);
		GamePlayerLeftMessage *netPlayerLeft = thisPlayerLeft->GetMsg()->mutable_gameplayerleftmessage();
		netPlayerLeft->set_gameid(GetId());
		netPlayerLeft->set_playerid(player->GetUniqueId());
		netPlayerLeft->set_gameplayerleftreason(netReason);
	}
	GetSessionManager().SendToAllSessions(GetLobbyThread().GetSender(), thisPlayerLeft, SessionData::Game | SessionData::Spectating | SessionData::SpectatorWaiting);

	GetState().NotifySessionRemoved(shared_from_this());
	if (spectateOnly) {
		GetLobbyThread().NotifySpectatorLeftGame(GetId(), player->GetUniqueId());
	} else {
		GetLobbyThread().NotifyPlayerLeftGame(GetId(), player->GetUniqueId());
	}
}

void
ServerGame::SessionError(boost::shared_ptr<SessionData> session, int errorCode)
{
	if (!session)
		throw ServerException(__FILE__, __LINE__, ERR_NET_INVALID_SESSION, 0);
	RemoveSession(session, NTF_NET_INTERNAL);
	GetLobbyThread().SessionError(session, errorCode);
}

void
ServerGame::MoveSessionToLobby(boost::shared_ptr<SessionData> session, int reason)
{
	RemoveSession(session, reason);
	// Reset ready flag - just in case it is set, player may leave at any time.
	session->ResetReadyFlag();
	GetLobbyThread().ReAddSession(session, reason, GetId());
}

void
ServerGame::RemoveDisconnectedPlayers()
{
	// This should only be called between hands.
	if (m_game) {
		PlayerList tmpList(m_game->getSeatsList());
		PlayerListIterator i = tmpList->begin();
		PlayerListIterator end = tmpList->end();
		while (i != end) {
			boost::shared_ptr<PlayerInterface> tmpPlayer = *i;
			if ((tmpPlayer->getMyType() == PLAYER_TYPE_HUMAN && !GetSessionManager().IsPlayerConnected(tmpPlayer->getMyUniqueID()))
					|| (tmpPlayer->getMyType() == PLAYER_TYPE_COMPUTER && !IsComputerPlayerActive(tmpPlayer->getMyUniqueID()))) {
				// Setting player cash to 0 will deactivate the player.
				// The player should only be deactivated if rejoin is not possible.
				if (tmpPlayer->isKicked() || tmpPlayer->getMyGuid().empty()) {
					tmpPlayer->setMyCash(0);
					tmpPlayer->setMyGuid("");
				}
				tmpPlayer->setIsSessionActive(false);
			}
			++i;
		}
	}
}

int
ServerGame::GetCurNumberOfPlayers() const
{
	return (int)GetFullPlayerDataList().size();
}

void
ServerGame::AssignPlayerNumbers(PlayerDataList &playerList)
{
	int playerNumber = 0;

	PlayerDataList::iterator player_i = playerList.begin();
	PlayerDataList::iterator player_end = playerList.end();

	while (player_i != player_end) {
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

void
ServerGame::AddReportedAvatar(unsigned playerId)
{
	m_reportedAvatarList.push_back(playerId);
}

bool
ServerGame::IsAvatarReported(unsigned playerId) const
{
	bool retVal = false;
	PlayerIdList::const_iterator pos = find(m_reportedAvatarList.begin(), m_reportedAvatarList.end(), playerId);
	if (pos != m_reportedAvatarList.end())
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

ServerDBInterface &
ServerGame::GetDatabase()
{
	assert(m_database);
	return *m_database;
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

boost::asio::steady_timer &
ServerGame::GetStateTimer1()
{
	return m_stateTimer1;
}

boost::asio::steady_timer &
ServerGame::GetStateTimer2()
{
	return m_stateTimer2;
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

bool
ServerGame::CheckSettings(const GameData &data, const string &password, ServerMode mode)
{
	bool retVal = true;
	if (mode != SERVER_MODE_LAN) {
		if (data.playerActionTimeoutSec < 5) {
			retVal = false;
		}
	}
	if (data.gameType == GAME_TYPE_RANKING) {
		if ((data.startMoney != RANKING_GAME_START_CASH)
				|| (data.maxNumberOfPlayers != RANKING_GAME_NUMBER_OF_PLAYERS)
				|| (data.firstSmallBlind != RANKING_GAME_START_SBLIND)
				|| (data.raiseIntervalMode != RAISE_ON_HANDNUMBER)
				|| (data.raiseMode != DOUBLE_BLINDS)
				|| (data.raiseSmallBlindEveryHandsValue != RANKING_GAME_RAISE_EVERY_HAND)
				|| (!password.empty())
				|| (!data.allowSpectators)) {
			retVal = false;
		}
	}
	return retVal;
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

void
ServerGame::AddPlayerToNumJoinsPerPlayer(const std::string &playerName)
{
	NumJoinsPerPlayerMap::iterator pos = m_numJoinsPerPlayer.find(playerName);
	if (pos != m_numJoinsPerPlayer.end()) {
		pos->second++;
	} else {
		m_numJoinsPerPlayer[playerName] = 1;
	}
}

int
ServerGame::GetNumJoinsPerPlayer(const std::string &playerName)
{
	int num = 0;
	NumJoinsPerPlayerMap::const_iterator pos = m_numJoinsPerPlayer.find(playerName);
	if (pos != m_numJoinsPerPlayer.end()) {
		num = pos->second;
	}
	return num;
}
