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

#include <net/servergamestate.h>
#include <net/servergame.h>
#include <net/serverlobbythread.h>
#include <net/receiverhelper.h>
#include <net/senderhelper.h>
#include <net/netpacket.h>
#include <net/socket_msg.h>
#include <net/serverexception.h>
#include <gamedata.h>
#include <game.h>
#include <playerinterface.h>
#include <handinterface.h>

#include <boost/bind.hpp>

#include <sstream>

using namespace std;

//#define SERVER_TEST

#ifdef SERVER_TEST
	#define SERVER_DELAY_NEXT_HAND_SEC				0
	#define SERVER_DELAY_NEXT_GAME_SEC				0
	#define SERVER_DEAL_FLOP_CARDS_DELAY_SEC		0
	#define SERVER_DEAL_TURN_CARD_DELAY_SEC			0
	#define SERVER_DEAL_RIVER_CARD_DELAY_SEC		0
	#define SERVER_DEAL_ADD_ALL_IN_DELAY_SEC		0
	#define SERVER_SHOW_CARDS_DELAY_SEC				0
	#define SERVER_PLAYER_TIMEOUT_ADD_DELAY_SEC		0
	#define SERVER_COMPUTER_ACTION_DELAY_SEC		0
#else
	#define SERVER_DELAY_NEXT_HAND_SEC				10
	#define SERVER_DELAY_NEXT_GAME_SEC				10
	#define SERVER_DEAL_FLOP_CARDS_DELAY_SEC		5
	#define SERVER_DEAL_TURN_CARD_DELAY_SEC			2
	#define SERVER_DEAL_RIVER_CARD_DELAY_SEC		2
	#define SERVER_DEAL_ADD_ALL_IN_DELAY_SEC		2
	#define SERVER_SHOW_CARDS_DELAY_SEC				2
	#define SERVER_PLAYER_TIMEOUT_ADD_DELAY_SEC		2
	#define SERVER_COMPUTER_ACTION_DELAY_SEC		2
#endif

#define SERVER_START_GAME_TIMEOUT_SEC				10
#define SERVER_GAME_ADMIN_WARNING_REMAINING_SEC		60
#define SERVER_GAME_ADMIN_TIMEOUT_SEC				300		// 5 min, MUST be > SERVER_GAME_ADMIN_WARNING_REMAINING_SEC
#define SERVER_VOTE_KICK_TIMEOUT_SEC				30
#define SERVER_LOOP_DELAY_MSEC						20

// Helper functions
// TODO: these are hacks.

static void SendPlayerAction(ServerGame &server, boost::shared_ptr<PlayerInterface> player)
{
	if (!player.get())
		throw ServerException(__FILE__, __LINE__, ERR_NET_NO_CURRENT_PLAYER, 0);
	boost::shared_ptr<NetPacket> notifyActionDone(new NetPacketPlayersActionDone);
	NetPacketPlayersActionDone::Data actionDoneData;
	actionDoneData.gameState = server.GetCurRound();
	actionDoneData.playerId = player->getMyUniqueID();
	actionDoneData.playerAction = static_cast<PlayerAction>(player->getMyAction());
	actionDoneData.totalPlayerBet = player->getMySet();
	actionDoneData.playerMoney = player->getMyCash();
	actionDoneData.highestSet = server.GetGame().getCurrentHand()->getCurrentBeRo()->getHighestSet();
	actionDoneData.minimumRaise = server.GetGame().getCurrentHand()->getCurrentBeRo()->getMinimumRaise();
	static_cast<NetPacketPlayersActionDone *>(notifyActionDone.get())->SetData(actionDoneData);
	server.SendToAllPlayers(notifyActionDone, SessionData::Game);
}

static void SendNewRoundCards(ServerGame &server, Game &curGame, int state)
{
	// TODO: no switch needed here if game states are polymorphic
	switch(state) {
		case GAME_STATE_PREFLOP: {
			// nothing to do
		} break;
		case GAME_STATE_FLOP: {
			// deal flop cards
			int cards[5];
			curGame.getCurrentHand()->getBoard()->getMyCards(cards);
			boost::shared_ptr<NetPacket> notifyCards(new NetPacketDealFlopCards);
			NetPacketDealFlopCards::Data notifyCardsData;
			for (int num = 0; num < 3; num++)
				notifyCardsData.flopCards[num] = static_cast<u_int16_t>(cards[num]);
			static_cast<NetPacketDealFlopCards *>(notifyCards.get())->SetData(notifyCardsData);
			server.SendToAllPlayers(notifyCards, SessionData::Game);
		} break;
		case GAME_STATE_TURN: {
			// deal turn card
			int cards[5];
			curGame.getCurrentHand()->getBoard()->getMyCards(cards);
			boost::shared_ptr<NetPacket> notifyCards(new NetPacketDealTurnCard);
			NetPacketDealTurnCard::Data notifyCardsData;
			notifyCardsData.turnCard = static_cast<u_int16_t>(cards[3]);
			static_cast<NetPacketDealTurnCard *>(notifyCards.get())->SetData(notifyCardsData);
			server.SendToAllPlayers(notifyCards, SessionData::Game);
		} break;
		case GAME_STATE_RIVER: {
			// deal river card
			int cards[5];
			curGame.getCurrentHand()->getBoard()->getMyCards(cards);
			boost::shared_ptr<NetPacket> notifyCards(new NetPacketDealRiverCard);
			NetPacketDealRiverCard::Data notifyCardsData;
			notifyCardsData.riverCard = static_cast<u_int16_t>(cards[4]);
			static_cast<NetPacketDealRiverCard *>(notifyCards.get())->SetData(notifyCardsData);
			server.SendToAllPlayers(notifyCards, SessionData::Game);
		} break;
		default: {
			// 
		}
	}
}

static void StartNewHand(ServerGame &server)
{
	// Initialize hand.
	Game &curGame = server.GetGame();
	curGame.initHand();

	// HACK: Skip GUI notification run
	curGame.getCurrentHand()->getFlop()->skipFirstRunGui();
	curGame.getCurrentHand()->getTurn()->skipFirstRunGui();
	curGame.getCurrentHand()->getRiver()->skipFirstRunGui();

	// Consider all players, even inactive.
	PlayerListIterator i = curGame.getSeatsList()->begin();
	PlayerListIterator end = curGame.getSeatsList()->end();

	// Send cards to all players.
	while (i != end)
	{
		// also send to inactive players, but not to disconnected players.
		boost::shared_ptr<PlayerInterface> tmpPlayer = *i;
		if (tmpPlayer->getNetSessionData().get())
		{
			int cards[2];
			tmpPlayer->getMyCards(cards);
			boost::shared_ptr<NetPacket> notifyCards(new NetPacketHandStart);
			NetPacketHandStart::Data handStartData;
			handStartData.yourCards[0] = static_cast<unsigned>(cards[0]);
			handStartData.yourCards[1] = static_cast<unsigned>(cards[1]);
			handStartData.smallBlind = curGame.getCurrentHand()->getSmallBlind();
			static_cast<NetPacketHandStart *>(notifyCards.get())->SetData(handStartData);

			tmpPlayer->getNetSessionData()->GetSender().Send(tmpPlayer->getNetSessionData(), notifyCards);
		}
		++i;
	}

	// Start hand.
	curGame.startHand();

	// Auto small blind / big blind at the beginning of hand.
	i = curGame.getActivePlayerList()->begin();
	end = curGame.getActivePlayerList()->end();

	while (i != end)
	{
		boost::shared_ptr<PlayerInterface> tmpPlayer = *i;
		if (tmpPlayer->getMyButton() == BUTTON_SMALL_BLIND)
		{
			boost::shared_ptr<NetPacket> notifySmallBlind(new NetPacketPlayersActionDone);
			NetPacketPlayersActionDone::Data actionDoneData;
			actionDoneData.gameState = GAME_STATE_PREFLOP_SMALL_BLIND;
			actionDoneData.playerId = tmpPlayer->getMyUniqueID();
			actionDoneData.playerAction = (PlayerAction)tmpPlayer->getMyAction();
			actionDoneData.totalPlayerBet = tmpPlayer->getMySet();
			actionDoneData.playerMoney = tmpPlayer->getMyCash();
			actionDoneData.highestSet = server.GetGame().getCurrentHand()->getCurrentBeRo()->getHighestSet();
			actionDoneData.minimumRaise = server.GetGame().getCurrentHand()->getCurrentBeRo()->getMinimumRaise();
			static_cast<NetPacketPlayersActionDone *>(notifySmallBlind.get())->SetData(actionDoneData);
			server.SendToAllPlayers(notifySmallBlind, SessionData::Game);
			break;
		}
		++i;
	}

	i = curGame.getActivePlayerList()->begin();
	end = curGame.getActivePlayerList()->end();
	while (i != end)
	{
		boost::shared_ptr<PlayerInterface> tmpPlayer = *i;
		if (tmpPlayer->getMyButton() == BUTTON_BIG_BLIND)
		{
			boost::shared_ptr<NetPacket> notifyBigBlind(new NetPacketPlayersActionDone);
			NetPacketPlayersActionDone::Data actionDoneData;
			actionDoneData.gameState = GAME_STATE_PREFLOP_BIG_BLIND;
			actionDoneData.playerId = tmpPlayer->getMyUniqueID();
			actionDoneData.playerAction = (PlayerAction)tmpPlayer->getMyAction();
			actionDoneData.totalPlayerBet = tmpPlayer->getMySet();
			actionDoneData.playerMoney = tmpPlayer->getMyCash();
			actionDoneData.highestSet = server.GetGame().getCurrentHand()->getCurrentBeRo()->getHighestSet();
			actionDoneData.minimumRaise = server.GetGame().getCurrentHand()->getCurrentBeRo()->getMinimumRaise();
			static_cast<NetPacketPlayersActionDone *>(notifyBigBlind.get())->SetData(actionDoneData);
			server.SendToAllPlayers(notifyBigBlind, SessionData::Game);
			break;
		}
		++i;
	}
}

static void PerformPlayerAction(ServerGame &server, boost::shared_ptr<PlayerInterface> player, PlayerAction action, int bet)
{
	Game &curGame = server.GetGame();
	if (!player.get())
		throw ServerException(__FILE__, __LINE__, ERR_NET_NO_CURRENT_PLAYER, 0);
	player->setMyAction(action);
	// Only change the player bet if action is not fold/check
	if (action != PLAYER_ACTION_FOLD && action != PLAYER_ACTION_CHECK)
	{

		player->setMySet(bet);

		// update minimumRaise
		switch(action) {
			case PLAYER_ACTION_BET: {
				curGame.getCurrentHand()->getCurrentBeRo()->setMinimumRaise(bet);
			} break;
			case PLAYER_ACTION_RAISE: {
				curGame.getCurrentHand()->getCurrentBeRo()->setMinimumRaise(player->getMySet() - curGame.getCurrentHand()->getCurrentBeRo()->getHighestSet());
			} break;
			case PLAYER_ACTION_ALLIN: {
				if(player->getMySet() - curGame.getCurrentHand()->getCurrentBeRo()->getHighestSet() > curGame.getCurrentHand()->getCurrentBeRo()->getMinimumRaise()) {
					curGame.getCurrentHand()->getCurrentBeRo()->setMinimumRaise(player->getMySet() - curGame.getCurrentHand()->getCurrentBeRo()->getHighestSet());
				}
			} break;
			default: {
			}
		}

		// update highestSet
		if (player->getMySet() > curGame.getCurrentHand()->getCurrentBeRo()->getHighestSet())
			curGame.getCurrentHand()->getCurrentBeRo()->setHighestSet(player->getMySet());
		// Update total sets.
		curGame.getCurrentHand()->getBoard()->collectSets();
	}


	SendPlayerAction(server, player);
}

//-----------------------------------------------------------------------------

ServerGameState::~ServerGameState()
{
}

//-----------------------------------------------------------------------------

AbstractServerGameStateReceiving::~AbstractServerGameStateReceiving()
{
}

int
AbstractServerGameStateReceiving::ProcessPacket(ServerGame &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet)
{
	// This is the receive loop for the server.
	int retVal = MSG_SOCK_INTERNAL_PENDING;

	if (packet->IsClientActivity())
		session.sessionData->ResetActivityTimer();
	if (packet->ToNetPacketRetrievePlayerInfo())
	{
		// Delegate to Lobby.
		server.GetLobbyThread().HandleGameRetrievePlayerInfo(session, *packet->ToNetPacketRetrievePlayerInfo());
	}
	else if (packet->ToNetPacketRetrieveAvatar())
	{
		// Delegate to Lobby.
		server.GetLobbyThread().HandleGameRetrieveAvatar(session, *packet->ToNetPacketRetrieveAvatar());
	}
	else if (packet->ToNetPacketLeaveCurrentGame())
	{
		server.MoveSessionToLobby(session, NTF_NET_REMOVED_ON_REQUEST);
	}
	else if (packet->ToNetPacketKickPlayer())
	{
		// Only admins are allowed to kick, and only in the lobby.
		// After leaving the lobby, a vote needs to be initiated to kick.
		if (session.playerData->GetRights() == PLAYER_RIGHTS_ADMIN && !server.IsRunning())
		{
			NetPacketKickPlayer::Data kickPlayerData;
			packet->ToNetPacketKickPlayer()->GetData(kickPlayerData);

			server.InternalKickPlayer(kickPlayerData.playerId);
		}
	}
	else if (packet->ToNetPacketAskKickPlayer())
	{
		if (session.playerData)
		{
			NetPacketAskKickPlayer::Data askKickData;
			packet->ToNetPacketAskKickPlayer()->GetData(askKickData);

			server.InternalAskVoteKick(session, askKickData.playerId, SERVER_VOTE_KICK_TIMEOUT_SEC);
		}
	}
	else if (packet->ToNetPacketVoteKickPlayer())
	{
		if (session.playerData)
		{
			NetPacketVoteKickPlayer::Data voteData;
			packet->ToNetPacketVoteKickPlayer()->GetData(voteData);

			server.InternalVoteKick(session, voteData.petitionId, voteData.vote);
		}
	}
	// Chat text is always allowed.
	else if (packet->ToNetPacketSendChatText())
	{
		if (session.playerData) // Only forward if this player is known.
		{
			// Forward chat text to all players.
			// TODO: Some limitation needed.
			NetPacketSendChatText::Data inChatData;
			packet->ToNetPacketSendChatText()->GetData(inChatData);

			boost::shared_ptr<NetPacket> outChat(new NetPacketChatText);
			NetPacketChatText::Data outChatData;
			outChatData.playerId = session.playerData->GetUniqueId();
			outChatData.text = inChatData.text;
			static_cast<NetPacketChatText *>(outChat.get())->SetData(outChatData);
			server.SendToAllPlayers(outChat, SessionData::Game);
		}
	}
	else if (packet->ToNetPacketUnsubscribeGameList())
	{
		// We can do this directly in this thread.
		session.sessionData->ResetWantsLobbyMsg();
	}
	else if (packet->ToNetPacketResubscribeGameList())
	{
		// This needs to be performed in the lobby thread,
		// because a new game list needs to be sent.
		if (!session.sessionData->WantsLobbyMsg())
			server.GetLobbyThread().ResubscribeLobbyMsg(session);
	}
	else
	{
		// Packet processing in subclass.
		retVal = InternalProcessPacket(server, session, packet);
	}

	return retVal;
}

//-----------------------------------------------------------------------------

ServerGameStateInit ServerGameStateInit::s_state;

ServerGameStateInit &
ServerGameStateInit::Instance()
{
	return s_state;
}

ServerGameStateInit::ServerGameStateInit()
{
}

ServerGameStateInit::~ServerGameStateInit()
{
}

void
ServerGameStateInit::Enter(ServerGame &server)
{
	RegisterAdminTimer(server);
}

void
ServerGameStateInit::Exit(ServerGame &server)
{
	UnregisterAdminTimer(server);
}

void
ServerGameStateInit::NotifyGameAdminChanged(ServerGame &server)
{
	UnregisterAdminTimer(server);
	RegisterAdminTimer(server);
}

void
ServerGameStateInit::HandleNewSession(ServerGame &server, SessionWrapper session)
{
	if (session.sessionData.get() && session.playerData.get())
	{
		size_t curNumPlayers = server.GetCurNumberOfPlayers();

		// Check the number of players.
		if (curNumPlayers >= (size_t)server.GetGameData().maxNumberOfPlayers)
		{
			server.MoveSessionToLobby(session, NTF_NET_REMOVED_GAME_FULL);
		}
		// Check whether the client supports the current game.
		else if ((size_t)server.GetGameData().maxNumberOfPlayers > session.sessionData->GetMaxNumPlayers())
		{
			server.MoveSessionToLobby(session, NTF_NET_REMOVED_GAME_FULL);
		}
		else
		{
			if (session.playerData->GetUniqueId() == server.GetAdminPlayerId())
			{
				// This is the admin player.
				session.playerData->SetRights(PLAYER_RIGHTS_ADMIN);
			}

			// Send ack to client.
			boost::shared_ptr<NetPacket> joinGameAck(new NetPacketJoinGameAck);
			NetPacketJoinGameAck::Data joinGameAckData;
			joinGameAckData.gameId = server.GetId();
			joinGameAckData.prights = session.playerData->GetRights();
			joinGameAckData.gameData = server.GetGameData();
			static_cast<NetPacketJoinGameAck *>(joinGameAck.get())->SetData(joinGameAckData);
			session.sessionData->GetSender().Send(session.sessionData, joinGameAck);

			// Send notifications for connected players to client.
			PlayerDataList tmpPlayerList = server.GetFullPlayerDataList();
			PlayerDataList::iterator player_i = tmpPlayerList.begin();
			PlayerDataList::iterator player_end = tmpPlayerList.end();
			while (player_i != player_end)
			{
				session.sessionData->GetSender().Send(session.sessionData, CreateNetPacketPlayerJoined(*(*player_i)));
				++player_i;
			}

			// Send "Player Joined" to other fully connected clients.
			server.SendToAllPlayers(CreateNetPacketPlayerJoined(*session.playerData), SessionData::Game);

			// Accept session.
			server.GetSessionManager().AddSession(session);

			// Notify lobby.
			server.GetLobbyThread().NotifyPlayerJoinedGame(server.GetId(), session.playerData->GetUniqueId());
		}
	}
}

void
ServerGameStateInit::RegisterAdminTimer(ServerGame &server)
{
	server.SetStateTimerId(
		server.GetLobbyThread().GetTimerManager().RegisterTimer(
			(SERVER_GAME_ADMIN_TIMEOUT_SEC - SERVER_GAME_ADMIN_WARNING_REMAINING_SEC) * 1000,
			boost::bind(&ServerGameStateInit::TimerAdminWarning, this, boost::ref(server))));
}

void
ServerGameStateInit::UnregisterAdminTimer(ServerGame &server)
{
	server.GetLobbyThread().GetTimerManager().UnregisterTimer(server.GetStateTimerId());
	server.SetStateTimerId(0);
}

void
ServerGameStateInit::TimerAdminWarning(ServerGame &server)
{
	// Find game admin.
	SessionWrapper session = server.GetSessionManager().GetSessionByUniquePlayerId(server.GetAdminPlayerId());
	if (session.sessionData.get())
	{
		// Send him a warning.
		boost::shared_ptr<NetPacket> warning(new NetPacketTimeoutWarning);
		NetPacketTimeoutWarning::Data warningData;
		warningData.timeoutReason = NETWORK_TIMEOUT_GAME_ADMIN_IDLE;
		warningData.remainingSeconds = SERVER_GAME_ADMIN_WARNING_REMAINING_SEC;
		static_cast<NetPacketTimeoutWarning *>(warning.get())->SetData(warningData);
		session.sessionData->GetSender().Send(session.sessionData, warning);
	}
	// Start timeout timer.
	server.GetLobbyThread().GetTimerManager().RestartTimer(
		server.GetStateTimerId(),
		SERVER_GAME_ADMIN_WARNING_REMAINING_SEC * 1000,
		boost::bind(&ServerGameStateInit::TimerAdminTimeout, this, boost::ref(server)));
}

void
ServerGameStateInit::TimerAdminTimeout(ServerGame &server)
{
	// Find game admin.
	SessionWrapper session = server.GetSessionManager().GetSessionByUniquePlayerId(server.GetAdminPlayerId());
	if (session.sessionData.get())
	{
		// Remove him from the game.
		server.MoveSessionToLobby(session, NTF_NET_REMOVED_TIMEOUT);
	}
}

int
ServerGameStateInit::InternalProcessPacket(ServerGame &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet)
{
	int retVal = MSG_SOCK_INTERNAL_PENDING;

	if (packet->ToNetPacketStartEvent())
	{
		// Only admins are allowed to start the game.
		if (session.playerData->GetRights() == PLAYER_RIGHTS_ADMIN)
		{
			NetPacketStartEvent::Data startData;
			packet->ToNetPacketStartEvent()->GetData(startData);

			// Fill up with computer players.
			server.ResetComputerPlayerList();

			if (startData.fillUpWithCpuPlayers)
			{
				int remainingSlots = server.GetGameData().maxNumberOfPlayers - server.GetCurNumberOfPlayers();
				for (int i = 1; i <= remainingSlots; i++)
				{
					boost::shared_ptr<PlayerData> tmpPlayerData(
						new PlayerData(server.GetLobbyThread().GetNextUniquePlayerId(), 0, PLAYER_TYPE_COMPUTER, PLAYER_RIGHTS_NORMAL));

					ostringstream name;
					name << SERVER_COMPUTER_PLAYER_NAME << i;
					tmpPlayerData->SetName(name.str());
					server.AddComputerPlayer(tmpPlayerData);

					// Send "Player Joined" to other fully connected clients.
					server.SendToAllPlayers(CreateNetPacketPlayerJoined(*tmpPlayerData), SessionData::Game);

					// Notify lobby.
					server.GetLobbyThread().NotifyPlayerJoinedGame(server.GetId(), tmpPlayerData->GetUniqueId());
				}
			}
			// Wait for all players to confirm start of game.
			server.SendToAllPlayers(boost::shared_ptr<NetPacket>(packet->Clone()), SessionData::Game);

			server.SetState(ServerGameStateStartGame::Instance());
		}
	}
	else if (packet->ToNetPacketResetTimeout())
	{
		if (session.playerData->GetRights() == PLAYER_RIGHTS_ADMIN)
		{
			UnregisterAdminTimer(server);
			RegisterAdminTimer(server);
		}
	}
	else
	{
		server.SessionError(session, ERR_SOCK_INVALID_PACKET);
	}

	return retVal;
}

boost::shared_ptr<NetPacket>
ServerGameStateInit::CreateNetPacketPlayerJoined(const PlayerData &playerData)
{
	boost::shared_ptr<NetPacket> thisPlayerJoined(new NetPacketPlayerJoined);
	NetPacketPlayerJoined::Data thisPlayerJoinedData;
	thisPlayerJoinedData.playerId = playerData.GetUniqueId();
	thisPlayerJoinedData.prights = playerData.GetRights();
	static_cast<NetPacketPlayerJoined *>(thisPlayerJoined.get())->SetData(thisPlayerJoinedData);
	return thisPlayerJoined;
}

//-----------------------------------------------------------------------------

ServerGameStateStartGame ServerGameStateStartGame::s_state;

ServerGameStateStartGame &
ServerGameStateStartGame::Instance()
{
	return s_state;
}

ServerGameStateStartGame::ServerGameStateStartGame()
{
}

ServerGameStateStartGame::~ServerGameStateStartGame()
{
}

void
ServerGameStateStartGame::Enter(ServerGame &server)
{
	server.SetStateTimerId(
		server.GetLobbyThread().GetTimerManager().RegisterTimer(
			SERVER_START_GAME_TIMEOUT_SEC * 1000,
			boost::bind(&ServerGameStateStartGame::TimerTimeout, this, boost::ref(server))));
}

void
ServerGameStateStartGame::Exit(ServerGame &server)
{
	server.GetLobbyThread().GetTimerManager().UnregisterTimer(server.GetStateTimerId());
	server.SetStateTimerId(0);
}

void
ServerGameStateStartGame::HandleNewSession(ServerGame &server, SessionWrapper session)
{
	// Do not accept new sessions in this state.
	server.MoveSessionToLobby(session, NTF_NET_REMOVED_ALREADY_RUNNING);
}

int
ServerGameStateStartGame::InternalProcessPacket(ServerGame &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet)
{
	int retVal = MSG_SOCK_INTERNAL_PENDING;

	if (packet->ToNetPacketStartEventAck())
	{
		session.sessionData->SetReadyFlag();
		if (server.GetSessionManager().CountReadySessions() == server.GetSessionManager().GetRawSessionCount())
		{
			// Everyone is ready.
			server.GetSessionManager().ResetAllReadyFlags();
			DoStart(server);
			retVal = MSG_SOCK_INIT_DONE;
		}
	}

	return retVal;
}

void
ServerGameStateStartGame::TimerTimeout(ServerGame &server)
{
	// On timeout: start anyway.
	server.GetSessionManager().ResetAllReadyFlags();
	// TODO report successful start! -> new callback?!
	//retVal = MSG_SOCK_INIT_DONE;
	DoStart(server);
}

void
ServerGameStateStartGame::DoStart(ServerGame &server)
{
	PlayerDataList tmpPlayerList = server.GetFullPlayerDataList();
	if (tmpPlayerList.size() <= 1)
	{
		if (!tmpPlayerList.empty())
		{
			boost::shared_ptr<PlayerData> tmpPlayer(tmpPlayerList.front());
			SessionWrapper tmpSession = server.GetSessionManager().GetSessionByUniquePlayerId(tmpPlayer->GetUniqueId());
			if (tmpSession.sessionData.get())
				server.MoveSessionToLobby(tmpSession, NTF_NET_REMOVED_START_FAILED);
		}
	}
	else
	{
		server.InternalStartGame();

		boost::shared_ptr<NetPacket> answer(new NetPacketGameStart);

		NetPacketGameStart::Data gameStartData;
		gameStartData.startData = server.GetStartData();

		// Send player order to clients.
		// Assume player list is sorted by number.
		PlayerDataList::iterator player_i = tmpPlayerList.begin();
		PlayerDataList::iterator player_end = tmpPlayerList.end();
		while (player_i != player_end)
		{
			NetPacketGameStart::PlayerSlot tmpPlayerSlot;
			tmpPlayerSlot.playerId = (*player_i)->GetUniqueId();
			gameStartData.playerSlots.push_back(tmpPlayerSlot);
	
			++player_i;
		}

		static_cast<NetPacketGameStart *>(answer.get())->SetData(gameStartData);
		server.SendToAllPlayers(answer, SessionData::Game);

		// Start the first hand.
		StartNewHand(server);
		server.SetState(ServerGameStateHand::Instance());
	}
}

//-----------------------------------------------------------------------------

ServerGameStateHand ServerGameStateHand::s_state;

ServerGameStateHand &
ServerGameStateHand::Instance()
{
	return s_state;
}

ServerGameStateHand::ServerGameStateHand()
{
}

ServerGameStateHand::~ServerGameStateHand()
{
}

void
ServerGameStateHand::Enter(ServerGame &server)
{
	server.SetStateTimerId(
		server.GetLobbyThread().GetTimerManager().RegisterTimer(
			SERVER_LOOP_DELAY_MSEC,
			boost::bind(&ServerGameStateHand::TimerLoop, this, boost::ref(server))));
}

void
ServerGameStateHand::Exit(ServerGame &server)
{
	server.GetLobbyThread().GetTimerManager().UnregisterTimer(server.GetStateTimerId());
	server.SetStateTimerId(0);
}

void
ServerGameStateHand::HandleNewSession(ServerGame &server, SessionWrapper session)
{
	// Do not accept new sessions in this state.
	server.MoveSessionToLobby(session, NTF_NET_REMOVED_ALREADY_RUNNING);
}

int
ServerGameStateHand::InternalProcessPacket(ServerGame &/*server*/, SessionWrapper /*session*/, boost::shared_ptr<NetPacket> /*packet*/)
{
	// TODO: maybe reject packet.
	return MSG_SOCK_INTERNAL_PENDING;
}

void
ServerGameStateHand::TimerLoop(ServerGame &server)
{
	Game &curGame = server.GetGame();

	// Main game loop.
	int curRound = curGame.getCurrentHand()->getCurrentRound();
	curGame.getCurrentHand()->switchRounds();
	if (!curGame.getCurrentHand()->getAllInCondition())
		curGame.getCurrentHand()->getCurrentBeRo()->run();
	int newRound = curGame.getCurrentHand()->getCurrentRound();

	// If round changes, deal cards if needed.
	if (newRound != curRound && newRound != GAME_STATE_POST_RIVER)
	{
		if (newRound <= curRound)
			throw ServerException(__FILE__, __LINE__, ERR_NET_INVALID_GAME_ROUND, 0);

		// Retrieve non-fold players. If only one player is left, no cards are shown.
		list<boost::shared_ptr<PlayerInterface> > nonFoldPlayers = *curGame.getActivePlayerList();
		nonFoldPlayers.remove_if(boost::bind(&PlayerInterface::getMyAction, _1) == PLAYER_ACTION_FOLD);

		if (curGame.getCurrentHand()->getAllInCondition()
			&& !curGame.getCurrentHand()->getCardsShown()
			&& nonFoldPlayers.size() > 1)
		{
			// Send cards of all active players to all players (all in).
			boost::shared_ptr<NetPacket> allIn(new NetPacketAllInShowCards);
			NetPacketAllInShowCards::Data allInData;

			PlayerListConstIterator i = nonFoldPlayers.begin();
			PlayerListConstIterator end = nonFoldPlayers.end();

			while (i != end)
			{
				NetPacketAllInShowCards::PlayerCards tmpPlayerCards;
				tmpPlayerCards.playerId = (*i)->getMyUniqueID();

				int tmpCards[2];
				(*i)->getMyCards(tmpCards);
				tmpPlayerCards.cards[0] = static_cast<u_int16_t>(tmpCards[0]);
				tmpPlayerCards.cards[1] = static_cast<u_int16_t>(tmpCards[1]);

				allInData.playerCards.push_back(tmpPlayerCards);
				++i;
			}
			static_cast<NetPacketAllInShowCards *>(allIn.get())->SetData(allInData);
			server.SendToAllPlayers(allIn, SessionData::Game);
			curGame.getCurrentHand()->setCardsShown(true);

			server.GetLobbyThread().GetTimerManager().RestartTimer(
				server.GetStateTimerId(),
				SERVER_SHOW_CARDS_DELAY_SEC * 1000,
				boost::bind(&ServerGameStateHand::TimerLoop, this, boost::ref(server)));
		}
		else
		{
			SendNewRoundCards(server, curGame, newRound);

			server.GetLobbyThread().GetTimerManager().RestartTimer(
				server.GetStateTimerId(),
				GetDealCardsDelaySec(server) * 1000,
				boost::bind(&ServerGameStateHand::TimerLoop, this, boost::ref(server)));
		}
	}
	else
	{
		if (newRound != GAME_STATE_POST_RIVER) // continue hand
		{
			if (curGame.getCurrentHand()->getAllInCondition())
				throw ServerException(__FILE__, __LINE__, ERR_NET_INTERNAL_GAME_ERROR, 0);

			// Retrieve current player.
			boost::shared_ptr<PlayerInterface> curPlayer = curGame.getCurrentPlayer();
			if (!curPlayer.get())
				throw ServerException(__FILE__, __LINE__, ERR_NET_NO_CURRENT_PLAYER, 0);
			if (!curPlayer->getMyActiveStatus())
				throw ServerException(__FILE__, __LINE__, ERR_NET_PLAYER_NOT_ACTIVE, 0);

			boost::shared_ptr<NetPacket> notification(new NetPacketPlayersTurn);
			NetPacketPlayersTurn::Data playersTurnData;
			playersTurnData.gameState = (GameState)curGame.getCurrentHand()->getCurrentRound();
			playersTurnData.playerId = curPlayer->getMyUniqueID();
			static_cast<NetPacketPlayersTurn *>(notification.get())->SetData(playersTurnData);

			server.SendToAllPlayers(notification, SessionData::Game);

			// If the player is computer controlled, let the engine act.
			if (curPlayer->getMyType() == PLAYER_TYPE_COMPUTER)
			{
				server.GetLobbyThread().GetTimerManager().RestartTimer(
					server.GetStateTimerId(),
					SERVER_COMPUTER_ACTION_DELAY_SEC * 1000,
					boost::bind(&ServerGameStateHand::TimerComputerAction, this, boost::ref(server)));
			}
			// If the player we are waiting for left, continue without him.
			else if (!server.GetSessionManager().IsPlayerConnected(curPlayer->getMyName()))
			{
				PerformPlayerAction(server, curPlayer, PLAYER_ACTION_FOLD, 0);
				server.GetLobbyThread().GetTimerManager().RestartTimer(
					server.GetStateTimerId(),
					SERVER_LOOP_DELAY_MSEC,
					boost::bind(&ServerGameStateHand::TimerLoop, this, boost::ref(server)));
			}
			else
			{
				server.SetState(ServerGameStateWaitPlayerAction::Instance());
			}
		}
		else // hand is over
		{
			// Engine will find out who won.
			curGame.getCurrentHand()->getCurrentBeRo()->postRiverRun();

			// Retrieve non-fold players. If only one player is left, no cards are shown.
			list<boost::shared_ptr<PlayerInterface> > nonFoldPlayers = *curGame.getActivePlayerList();
			nonFoldPlayers.remove_if(boost::bind(&PlayerInterface::getMyAction, _1) == PLAYER_ACTION_FOLD);

			if (nonFoldPlayers.size() == 1)
			{
				// End of Hand, but keep cards hidden.
				boost::shared_ptr<PlayerInterface> player = nonFoldPlayers.front();
				boost::shared_ptr<NetPacket> endHand(new NetPacketEndOfHandHideCards);
				NetPacketEndOfHandHideCards::Data endHandData;
				endHandData.playerId = player->getMyUniqueID();
				endHandData.moneyWon = player->getLastMoneyWon();
				endHandData.playerMoney = player->getMyCash();
				static_cast<NetPacketEndOfHandHideCards *>(endHand.get())->SetData(endHandData);

				server.SendToAllPlayers(endHand, SessionData::Game);
			}
			else
			{
				// End of Hand - show cards of active players.
				boost::shared_ptr<NetPacket> endHand(new NetPacketEndOfHandShowCards);
				NetPacketEndOfHandShowCards::Data endHandData;

				PlayerListConstIterator i = nonFoldPlayers.begin();
				PlayerListConstIterator end = nonFoldPlayers.end();

				while (i != end)
				{
					NetPacketEndOfHandShowCards::PlayerResult tmpPlayerResult;
					tmpPlayerResult.playerId = (*i)->getMyUniqueID();

					int tmpCards[2];
					int bestHandPos[5];
					(*i)->getMyCards(tmpCards);
					tmpPlayerResult.cards[0] = static_cast<u_int16_t>(tmpCards[0]);
					tmpPlayerResult.cards[1] = static_cast<u_int16_t>(tmpCards[1]);

					(*i)->getMyBestHandPosition(bestHandPos);
					for (int num = 0; num < 5; num++)
						tmpPlayerResult.bestHandPos[num] = bestHandPos[num];

					tmpPlayerResult.valueOfCards = (*i)->getMyCardsValueInt();
					tmpPlayerResult.moneyWon = (*i)->getLastMoneyWon();
					tmpPlayerResult.playerMoney = (*i)->getMyCash();

					endHandData.playerResults.push_back(tmpPlayerResult);
					++i;
				}
				static_cast<NetPacketEndOfHandShowCards *>(endHand.get())->SetData(endHandData);

				server.SendToAllPlayers(endHand, SessionData::Game);
			}

			// Remove disconnected players. This is the one and only place to do this.
			server.RemoveDisconnectedPlayers();

			// Start next hand - if enough players are left.
			list<boost::shared_ptr<PlayerInterface> > playersWithCash = *curGame.getActivePlayerList();
			playersWithCash.remove_if(boost::bind(&PlayerInterface::getMyCash, _1) < 1);

			if (playersWithCash.empty())
			{
				// No more players left - restart.
				server.SetState(SERVER_INITIAL_STATE::Instance());
			}
			else if (playersWithCash.size() == 1)
			{
				// View a dialog for a new game - delayed.
				server.GetLobbyThread().GetTimerManager().RestartTimer(
					server.GetStateTimerId(),
					SERVER_DELAY_NEXT_GAME_SEC * 1000,
					boost::bind(&ServerGameStateHand::TimerNextGame, this, boost::ref(server)));
			}
			else
			{
				server.GetLobbyThread().GetTimerManager().RestartTimer(
					server.GetStateTimerId(),
					SERVER_DELAY_NEXT_HAND_SEC * 1000,
					boost::bind(&ServerGameStateHand::TimerNextHand, this, boost::ref(server)));
			}
		}
	}
}

void
ServerGameStateHand::TimerShowCards(ServerGame &server)
{
	Game &curGame = server.GetGame();
	SendNewRoundCards(server, curGame, curGame.getCurrentHand()->getCurrentRound());

	server.GetLobbyThread().GetTimerManager().RestartTimer(
		server.GetStateTimerId(),
		GetDealCardsDelaySec(server) * 1000,
		boost::bind(&ServerGameStateHand::TimerLoop, this, boost::ref(server)));
}

void
ServerGameStateHand::TimerComputerAction(ServerGame &server)
{
	boost::shared_ptr<PlayerInterface> tmpPlayer = server.GetGame().getCurrentPlayer();

	tmpPlayer->action();
	SendPlayerAction(server, tmpPlayer);
	TimerLoop(server);
}

void
ServerGameStateHand::TimerNextHand(ServerGame &server)
{
	StartNewHand(server);
	TimerLoop(server);
}

void
ServerGameStateHand::TimerNextGame(ServerGame &server)
{
	Game &curGame = server.GetGame();
	// The game has ended. Notify all clients.
	boost::shared_ptr<PlayerInterface> winnerPlayer;
	PlayerListIterator i = curGame.getActivePlayerList()->begin();
	PlayerListIterator end = curGame.getActivePlayerList()->end();
	while (i != end)
	{
		winnerPlayer = *i;
		if (winnerPlayer->getMyCash() > 0)
			break;
		++i;
	}

	boost::shared_ptr<NetPacket> endGame(new NetPacketEndOfGame);
	NetPacketEndOfGame::Data endGameData;
	endGameData.winnerPlayerId = winnerPlayer->getMyUniqueID();
	static_cast<NetPacketEndOfGame *>(endGame.get())->SetData(endGameData);

	server.SendToAllPlayers(endGame, SessionData::Game);

	// Wait for the start of a new game.
	server.ResetComputerPlayerList();
	server.ResetGame();
	server.GetLobbyThread().NotifyReopeningGame(server.GetId());
	server.SetState(ServerGameStateInit::Instance());
}

int
ServerGameStateHand::GetDealCardsDelaySec(ServerGame &server)
{
	Game &curGame = server.GetGame();
	int allInDelay = curGame.getCurrentHand()->getAllInCondition() ? SERVER_DEAL_ADD_ALL_IN_DELAY_SEC : 0;
	int delay = 0;

	switch(curGame.getCurrentHand()->getCurrentRound())
	{
		case GAME_STATE_FLOP:
			delay = SERVER_DEAL_FLOP_CARDS_DELAY_SEC;
			break;
		case GAME_STATE_TURN:
			delay = SERVER_DEAL_TURN_CARD_DELAY_SEC + allInDelay;
			break;
		case GAME_STATE_RIVER:
			delay = SERVER_DEAL_RIVER_CARD_DELAY_SEC;
			break;
	}
	return delay;
}

//-----------------------------------------------------------------------------


ServerGameStateWaitPlayerAction ServerGameStateWaitPlayerAction::s_state;

ServerGameStateWaitPlayerAction &
ServerGameStateWaitPlayerAction::Instance()
{
	return s_state;
}

ServerGameStateWaitPlayerAction::ServerGameStateWaitPlayerAction()
{
}

ServerGameStateWaitPlayerAction::~ServerGameStateWaitPlayerAction()
{
}

void
ServerGameStateWaitPlayerAction::Enter(ServerGame &server)
{
#ifdef SERVER_TEST
	int timeoutSec = 0;
#else
	int timeoutSec = server.GetGameData().playerActionTimeoutSec + SERVER_PLAYER_TIMEOUT_ADD_DELAY_SEC;
#endif

	server.SetStateTimerId(
		server.GetLobbyThread().GetTimerManager().RegisterTimer(
			timeoutSec * 1000,
			boost::bind(&ServerGameStateWaitPlayerAction::TimerTimeout, this, boost::ref(server))));
}

void
ServerGameStateWaitPlayerAction::Exit(ServerGame &server)
{
	server.GetLobbyThread().GetTimerManager().UnregisterTimer(server.GetStateTimerId());
	server.SetStateTimerId(0);
}

void
ServerGameStateWaitPlayerAction::HandleNewSession(ServerGame &server, SessionWrapper session)
{
	// Do not accept new sessions in this state.
	server.MoveSessionToLobby(session, NTF_NET_REMOVED_ALREADY_RUNNING);
}

int
ServerGameStateWaitPlayerAction::InternalProcessPacket(ServerGame &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet)
{
	int retVal = MSG_SOCK_INTERNAL_PENDING;

	if (packet->ToNetPacketPlayersAction())
	{
		NetPacketPlayersAction::Data actionData;
		packet->ToNetPacketPlayersAction()->GetData(actionData);

		Game &curGame = server.GetGame();
		boost::shared_ptr<PlayerInterface> tmpPlayer = curGame.getPlayerByUniqueId(session.playerData->GetUniqueId());
		if (!tmpPlayer.get())
			throw ServerException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);

		// Check whether this is the correct round.
		PlayerActionCode code = ACTION_CODE_VALID;
		if (curGame.getCurrentHand()->getCurrentRound() != actionData.gameState)
			code = ACTION_CODE_INVALID_STATE;

		// Check whether this is the correct player.
		boost::shared_ptr<PlayerInterface> curPlayer = server.GetGame().getCurrentPlayer();
		if (code == ACTION_CODE_VALID
			&& (curPlayer->getMyUniqueID() != tmpPlayer->getMyUniqueID()))
		{
			code = ACTION_CODE_NOT_YOUR_TURN;
		}

		// If the client omitted some values, fill them in.
		if (actionData.playerAction == PLAYER_ACTION_CALL && actionData.playerBet == 0)
		{
			if (curGame.getCurrentHand()->getCurrentBeRo()->getHighestSet() >= tmpPlayer->getMySet() + tmpPlayer->getMyCash())
				actionData.playerAction = PLAYER_ACTION_ALLIN;
			else
				actionData.playerBet = curGame.getCurrentHand()->getCurrentBeRo()->getHighestSet() - tmpPlayer->getMySet();
		}
		if (actionData.playerAction == PLAYER_ACTION_ALLIN && actionData.playerBet == 0)
			actionData.playerBet = tmpPlayer->getMyCash();

		// Check whether the action is valid.
		if (code == ACTION_CODE_VALID
			&& (tmpPlayer->checkMyAction(
					actionData.playerAction,
					actionData.playerBet,
					curGame.getCurrentHand()->getCurrentBeRo()->getHighestSet(),
					curGame.getCurrentHand()->getCurrentBeRo()->getMinimumRaise(),
					curGame.getCurrentHand()->getSmallBlind()) != 0))
		{
			code = ACTION_CODE_NOT_ALLOWED;
		}

		if (code == ACTION_CODE_VALID)
		{
			PerformPlayerAction(server, tmpPlayer, actionData.playerAction, actionData.playerBet);
			server.SetState(ServerGameStateHand::Instance());
			retVal = MSG_NET_GAME_SERVER_ACTION;
		}
		else
		{
			// Send reject message.
			boost::shared_ptr<NetPacket> reject(new NetPacketPlayersActionRejected);
			NetPacketPlayersActionRejected::Data rejectData;
			rejectData.gameState = actionData.gameState;
			rejectData.playerAction = actionData.playerAction;
			rejectData.playerBet = actionData.playerBet;
			rejectData.rejectionReason = code;
			static_cast<NetPacketPlayersActionRejected *>(reject.get())->SetData(rejectData);
			session.sessionData->GetSender().Send(session.sessionData, reject);
		}
	}

	return retVal;
}

void
ServerGameStateWaitPlayerAction::TimerTimeout(ServerGame &server)
{
	Game &curGame = server.GetGame();
	// Retrieve current player.
	boost::shared_ptr<PlayerInterface> curPlayer = curGame.getCurrentPlayer();
	if (!curPlayer.get())
		throw ServerException(__FILE__, __LINE__, ERR_NET_NO_CURRENT_PLAYER, 0);

	// Player did not act fast enough. Act for him.
	if (curGame.getCurrentHand()->getCurrentBeRo()->getHighestSet() == curPlayer->getMySet())
		PerformPlayerAction(server, curPlayer, PLAYER_ACTION_CHECK, 0);
	else
		PerformPlayerAction(server, curPlayer, PLAYER_ACTION_FOLD, 0);

	server.SetState(ServerGameStateHand::Instance());
}

//-----------------------------------------------------------------------------

