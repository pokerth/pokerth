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
#include <db/serverdbinterface.h>
#include <core/loghelper.h>
#include <gamedata.h>
#include <game.h>
#include <playerinterface.h>
#include <handinterface.h>

#include <boost/bind.hpp>

#include <sstream>

using namespace std;

//#define SERVER_TEST

#ifdef SERVER_TEST
	#define SERVER_DELAY_NEXT_GAME_SEC				0
	#define SERVER_DEAL_FLOP_CARDS_DELAY_SEC		0
	#define SERVER_DEAL_TURN_CARD_DELAY_SEC			0
	#define SERVER_DEAL_RIVER_CARD_DELAY_SEC		0
	#define SERVER_DEAL_ADD_ALL_IN_DELAY_SEC		0
	#define SERVER_SHOW_CARDS_DELAY_SEC				0
	#define SERVER_PLAYER_TIMEOUT_ADD_DELAY_SEC		0
	#define SERVER_COMPUTER_ACTION_DELAY_SEC		0
#else
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
#define SERVER_LOOP_DELAY_MSEC						50

// Helper functions

static void SendPlayerAction(ServerGame &server, boost::shared_ptr<PlayerInterface> player)
{
	if (!player.get())
		throw ServerException(__FILE__, __LINE__, ERR_NET_NO_CURRENT_PLAYER, 0);

	boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
	packet->GetMsg()->present = PokerTHMessage_PR_playersActionDoneMessage;
	PlayersActionDoneMessage_t *netActionDone = &packet->GetMsg()->choice.playersActionDoneMessage;

	netActionDone->gameId = server.GetId();
	netActionDone->gameState = server.GetCurRound();
	netActionDone->highestSet = server.GetGame().getCurrentHand()->getCurrentBeRo()->getHighestSet();
	netActionDone->minimumRaise = server.GetGame().getCurrentHand()->getCurrentBeRo()->getMinimumRaise();
	netActionDone->playerAction = static_cast<PlayerAction>(player->getMyAction());
	netActionDone->playerId = player->getMyUniqueID();
	netActionDone->playerMoney = player->getMyCash();
	netActionDone->totalPlayerBet = player->getMySet();
	server.SendToAllPlayers(packet, SessionData::Game);
}

static void SendNewRoundCards(ServerGame &server, Game &curGame, int state)
{
	int cards[5];
	curGame.getCurrentHand()->getBoard()->getMyCards(cards);
	switch(state) {
		case GAME_STATE_PREFLOP: {
			// nothing to do
		} break;
		case GAME_STATE_FLOP: {
			// deal flop cards
			boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
			packet->GetMsg()->present = PokerTHMessage_PR_dealFlopCardsMessage;
			DealFlopCardsMessage_t *netDealFlop = &packet->GetMsg()->choice.dealFlopCardsMessage;
			netDealFlop->gameId = server.GetId();
			netDealFlop->flopCard1 = cards[0];
			netDealFlop->flopCard2 = cards[1];
			netDealFlop->flopCard3 = cards[2];
			server.SendToAllPlayers(packet, SessionData::Game);
		} break;
		case GAME_STATE_TURN: {
			// deal turn card
			boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
			packet->GetMsg()->present = PokerTHMessage_PR_dealTurnCardMessage;
			DealTurnCardMessage_t *netDealTurn = &packet->GetMsg()->choice.dealTurnCardMessage;
			netDealTurn->gameId = server.GetId();
			netDealTurn->turnCard = cards[3];
			server.SendToAllPlayers(packet, SessionData::Game);
		} break;
		case GAME_STATE_RIVER: {
			// deal river card
			boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
			packet->GetMsg()->present = PokerTHMessage_PR_dealRiverCardMessage;
			DealRiverCardMessage_t *netDealRiver = &packet->GetMsg()->choice.dealRiverCardMessage;
			netDealRiver->gameId = server.GetId();
			netDealRiver->riverCard = cards[4];
			server.SendToAllPlayers(packet, SessionData::Game);
		} break;
		default: {
			// 
		}
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

static void
SetPlayerResult(PlayerResult_t &playerResult, boost::shared_ptr<PlayerInterface> tmpPlayer)
{
	playerResult.playerId = tmpPlayer->getMyUniqueID();
	int tmpCards[2];
	int bestHandPos[5];
	tmpPlayer->getMyCards(tmpCards);
	playerResult.resultCard1 = tmpCards[0];
	playerResult.resultCard2 = tmpCards[1];
	tmpPlayer->getMyBestHandPosition(bestHandPos);
	for (int num = 0; num < 5; num++)
	{
		long *handPos = (long *)calloc(1, sizeof(long));
		*handPos = bestHandPos[num];
		ASN_SEQUENCE_ADD(&playerResult.bestHandPosition.list, handPos);
	}

	playerResult.cardsValue = tmpPlayer->getMyCardsValueInt();
	playerResult.moneyWon = tmpPlayer->getLastMoneyWon();
	playerResult.playerMoney = tmpPlayer->getMyCash();
}

//-----------------------------------------------------------------------------

ServerGameState::~ServerGameState()
{
}

//-----------------------------------------------------------------------------

AbstractServerGameStateReceiving::~AbstractServerGameStateReceiving()
{
}

void
AbstractServerGameStateReceiving::ProcessPacket(boost::shared_ptr<ServerGame> server, SessionWrapper session, boost::shared_ptr<NetPacket> packet)
{
	if (packet->IsClientActivity())
		session.sessionData->ResetActivityTimer();
	if (packet->GetMsg()->present == PokerTHMessage_PR_playerInfoRequestMessage)
	{
		// Delegate to Lobby.
		server->GetLobbyThread().HandleGameRetrievePlayerInfo(session, packet->GetMsg()->choice.playerInfoRequestMessage);
	}
	else if (packet->GetMsg()->present == PokerTHMessage_PR_avatarRequestMessage)
	{
		// Delegate to Lobby.
		server->GetLobbyThread().HandleGameRetrieveAvatar(session, packet->GetMsg()->choice.avatarRequestMessage);
	}
	else if (packet->GetMsg()->present == PokerTHMessage_PR_leaveGameRequestMessage)
	{
		server->MoveSessionToLobby(session, NTF_NET_REMOVED_ON_REQUEST);
	}
	else if (packet->GetMsg()->present == PokerTHMessage_PR_kickPlayerRequestMessage)
	{
		// Only admins are allowed to kick, and only in the lobby.
		// After leaving the lobby, a vote needs to be initiated to kick.
		KickPlayerRequestMessage_t *netKickRequest = &packet->GetMsg()->choice.kickPlayerRequestMessage;
		if (session.playerData->IsGameAdmin() && !server->IsRunning() && netKickRequest->gameId == server->GetId())
		{
			server->InternalKickPlayer(netKickRequest->playerId);
		}
	}
	else if (packet->GetMsg()->present == PokerTHMessage_PR_askKickPlayerMessage)
	{
		if (session.playerData)
		{
			AskKickPlayerMessage_t *netAskKick = &packet->GetMsg()->choice.askKickPlayerMessage;
			server->InternalAskVoteKick(session, netAskKick->playerId, SERVER_VOTE_KICK_TIMEOUT_SEC);
		}
	}
	else if (packet->GetMsg()->present == PokerTHMessage_PR_voteKickRequestMessage)
	{
		if (session.playerData)
		{
			VoteKickRequestMessage_t *netVoteKick = &packet->GetMsg()->choice.voteKickRequestMessage;
			server->InternalVoteKick(session, netVoteKick->petitionId, netVoteKick->voteKick ? KICK_VOTE_IN_FAVOUR : KICK_VOTE_AGAINST);
		}
	}
	// Chat text is always allowed.
	else if (packet->GetMsg()->present == PokerTHMessage_PR_chatRequestMessage)
	{
		// Only forward if this player is known and not a guest.
		if (session.playerData && session.playerData->GetRights() != PLAYER_RIGHTS_GUEST)
		{
			// Forward chat text to all players.
			// TODO: Some limitation needed.
			ChatRequestMessage_t *netChatRequest = &packet->GetMsg()->choice.chatRequestMessage;
			if (netChatRequest->chatRequestType.present == chatRequestType_PR_chatRequestTypeLobby)
			{
				if (!server->IsRunning())
					server->GetLobbyThread().HandleChatRequest(session, *netChatRequest);
			}
			else if (netChatRequest->chatRequestType.present == chatRequestType_PR_chatRequestTypeGame)
			{
				boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
				packet->GetMsg()->present = PokerTHMessage_PR_chatMessage;
				ChatMessage_t *netChat = &packet->GetMsg()->choice.chatMessage;
				netChat->chatType.present = chatType_PR_chatTypeGame;
				ChatTypeGame_t *netGameChat = &netChat->chatType.choice.chatTypeGame;
				netGameChat->gameId = server->GetId();
				netGameChat->playerId = session.playerData->GetUniqueId();
				OCTET_STRING_fromBuf(
					&netChat->chatText,
					(char *)netChatRequest->chatText.buf,
					netChatRequest->chatText.size);
				server->SendToAllPlayers(packet, SessionData::Game);
			}
		}
	}
	else if (packet->GetMsg()->present == PokerTHMessage_PR_subscriptionRequestMessage)
	{
		SubscriptionRequestMessage_t *netSubscription = &packet->GetMsg()->choice.subscriptionRequestMessage;
		if (netSubscription->subscriptionAction == subscriptionAction_resubscribeGameList)
		{
			if (!session.sessionData->WantsLobbyMsg())
				server->GetLobbyThread().ResubscribeLobbyMsg(session);
		}
		else
			session.sessionData->ResetWantsLobbyMsg();
	}
	else
	{
		// Packet processing in subclass.
		InternalProcessPacket(server, session, packet);
	}
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
ServerGameStateInit::Enter(boost::shared_ptr<ServerGame> server)
{
	RegisterAdminTimer(server);
}

void
ServerGameStateInit::Exit(boost::shared_ptr<ServerGame> server)
{
	UnregisterAdminTimer(server);
}

void
ServerGameStateInit::NotifyGameAdminChanged(boost::shared_ptr<ServerGame> server)
{
	UnregisterAdminTimer(server);
	RegisterAdminTimer(server);
}

void
ServerGameStateInit::HandleNewSession(boost::shared_ptr<ServerGame> server, SessionWrapper session)
{
	if (session.sessionData && session.playerData)
	{
		const GameData &tmpGameData = server->GetGameData();
		// Check the number of players.
		if (server->GetCurNumberOfPlayers() >= (size_t)tmpGameData.maxNumberOfPlayers)
		{
			server->MoveSessionToLobby(session, NTF_NET_REMOVED_GAME_FULL);
		}
		else
		{
			session.playerData->SetGameAdmin(session.playerData->GetUniqueId() == server->GetAdminPlayerId());

			// Send ack to client.
			boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
			packet->GetMsg()->present = PokerTHMessage_PR_joinGameReplyMessage;
			JoinGameReplyMessage_t *netJoinReply = &packet->GetMsg()->choice.joinGameReplyMessage;
			netJoinReply->gameId = server->GetId();
			netJoinReply->joinGameResult.present = joinGameResult_PR_joinGameAck;
			JoinGameAck_t *joinAck = &netJoinReply->joinGameResult.choice.joinGameAck;
			joinAck->areYouGameAdmin = static_cast<PlayerInfoRights>(session.playerData->IsGameAdmin());

			NetPacket::SetGameData(server->GetGameData(), &joinAck->gameInfo);
			OCTET_STRING_fromBuf(
				&joinAck->gameInfo.gameName,
				session.playerData->GetName().c_str(),
				session.playerData->GetName().length());
			server->GetLobbyThread().GetSender().Send(session.sessionData, packet);

			// Send notifications for connected players to client.
			PlayerDataList tmpPlayerList = server->GetFullPlayerDataList();
			PlayerDataList::iterator player_i = tmpPlayerList.begin();
			PlayerDataList::iterator player_end = tmpPlayerList.end();
			while (player_i != player_end)
			{
				server->GetLobbyThread().GetSender().Send(session.sessionData, CreateNetPacketPlayerJoined(server->GetId(), *(*player_i)));
				++player_i;
			}

			// Send "Player Joined" to other fully connected clients.
			server->SendToAllPlayers(CreateNetPacketPlayerJoined(server->GetId(), *session.playerData), SessionData::Game);

			// Accept session.
			server->GetSessionManager().AddSession(session);

			// Notify lobby.
			server->GetLobbyThread().NotifyPlayerJoinedGame(server->GetId(), session.playerData->GetUniqueId());

			if (server->GetCurNumberOfPlayers() == (size_t)tmpGameData.maxNumberOfPlayers)
			{
				// Automatically start the game if it is full.
				SendStartEvent(*server, false);
			}
		}
	}
}

void
ServerGameStateInit::RegisterAdminTimer(boost::shared_ptr<ServerGame> server)
{
	server->GetStateTimer().expires_from_now(
		boost::posix_time::seconds(SERVER_GAME_ADMIN_TIMEOUT_SEC - SERVER_GAME_ADMIN_WARNING_REMAINING_SEC));
	server->GetStateTimer().async_wait(
		boost::bind(
			&ServerGameStateInit::TimerAdminWarning, this, boost::asio::placeholders::error, server));
}

void
ServerGameStateInit::UnregisterAdminTimer(boost::shared_ptr<ServerGame> server)
{
	server->GetStateTimer().cancel();
}

void
ServerGameStateInit::TimerAdminWarning(const boost::system::error_code &ec, boost::shared_ptr<ServerGame> server)
{
	if (!ec && &server->GetState() == this)
	{
		// Find game admin.
		SessionWrapper session = server->GetSessionManager().GetSessionByUniquePlayerId(server->GetAdminPlayerId());
		if (session.sessionData.get())
		{
			// Send him a warning.
			boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
			packet->GetMsg()->present = PokerTHMessage_PR_timeoutWarningMessage;
			TimeoutWarningMessage_t *netWarning = &packet->GetMsg()->choice.timeoutWarningMessage;
			netWarning->timeoutReason = NETWORK_TIMEOUT_GAME_ADMIN_IDLE;
			netWarning->remainingSeconds = SERVER_GAME_ADMIN_WARNING_REMAINING_SEC;
			server->GetLobbyThread().GetSender().Send(session.sessionData, packet);
		}
		// Start timeout timer.
		server->GetStateTimer().expires_from_now(
			boost::posix_time::seconds(SERVER_GAME_ADMIN_WARNING_REMAINING_SEC));
		server->GetStateTimer().async_wait(
			boost::bind(
				&ServerGameStateInit::TimerAdminTimeout, this, boost::asio::placeholders::error, server));
	}
}

void
ServerGameStateInit::TimerAdminTimeout(const boost::system::error_code &ec, boost::shared_ptr<ServerGame> server)
{
	if (!ec && &server->GetState() == this)
	{
		// Find game admin.
		SessionWrapper session = server->GetSessionManager().GetSessionByUniquePlayerId(server->GetAdminPlayerId());
		if (session.sessionData.get())
		{
			// Remove him from the game.
			server->MoveSessionToLobby(session, NTF_NET_REMOVED_TIMEOUT);
		}
	}
}

void
ServerGameStateInit::SendStartEvent(ServerGame &server, bool fillWithComputerPlayers)
{
	// Fill up with computer players.
	server.ResetComputerPlayerList();

	if (fillWithComputerPlayers)
	{
		int remainingSlots = server.GetGameData().maxNumberOfPlayers - server.GetCurNumberOfPlayers();
		for (int i = 1; i <= remainingSlots; i++)
		{
			boost::shared_ptr<PlayerData> tmpPlayerData(
				new PlayerData(server.GetLobbyThread().GetNextUniquePlayerId(), 0, PLAYER_TYPE_COMPUTER, PLAYER_RIGHTS_NORMAL, false));

			ostringstream name;
			name << SERVER_COMPUTER_PLAYER_NAME << i;
			tmpPlayerData->SetName(name.str());
			server.AddComputerPlayer(tmpPlayerData);

			// Send "Player Joined" to other fully connected clients.
			server.SendToAllPlayers(CreateNetPacketPlayerJoined(server.GetId(), *tmpPlayerData), SessionData::Game);

			// Notify lobby.
			server.GetLobbyThread().NotifyPlayerJoinedGame(server.GetId(), tmpPlayerData->GetUniqueId());
		}
	}
	boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
	packet->GetMsg()->present = PokerTHMessage_PR_startEventMessage;
	StartEventMessage_t *netStartEvent = &packet->GetMsg()->choice.startEventMessage;
	netStartEvent->gameId = server.GetId();
	netStartEvent->fillWithComputerPlayers = fillWithComputerPlayers;

	// Wait for all players to confirm start of game.
	server.SendToAllPlayers(packet, SessionData::Game);

	server.SetState(ServerGameStateStartGame::Instance());
}

void
ServerGameStateInit::InternalProcessPacket(boost::shared_ptr<ServerGame> server, SessionWrapper session, boost::shared_ptr<NetPacket> packet)
{
	if (packet->GetMsg()->present == PokerTHMessage_PR_startEventMessage)
	{
		StartEventMessage_t *netStartEvent = &packet->GetMsg()->choice.startEventMessage;
		// Only admins are allowed to start the game.
		if (session.playerData->IsGameAdmin() && netStartEvent->gameId == server->GetId())
		{
			SendStartEvent(*server, netStartEvent->fillWithComputerPlayers);
		}
	}
	else if (packet->GetMsg()->present == PokerTHMessage_PR_invitePlayerToGameMessage)
	{
		InvitePlayerToGameMessage_t *netInvite = &packet->GetMsg()->choice.invitePlayerToGameMessage;

		// Only invite players which are not already within the group.
		if (netInvite->gameId == server->GetId() && !server->IsPlayerConnected(netInvite->playerId))
		{
			boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
			packet->GetMsg()->present = PokerTHMessage_PR_inviteNotifyMessage;
			InviteNotifyMessage_t *netInvNotif = &packet->GetMsg()->choice.inviteNotifyMessage;
			netInvNotif->gameId = netInvite->gameId;
			netInvNotif->playerIdByWhom = session.playerData->GetUniqueId();
			netInvNotif->playerIdWho = netInvite->playerId;

			bool requestSent = server->GetLobbyThread().SendToLobbyPlayer(netInvite->playerId, packet);
			server->SendToAllPlayers(packet, SessionData::Game);
			if (requestSent)
			{
				// This player has been invited.
				server->AddPlayerInvitation(netInvite->playerId);
			}
			else
			{
				// Player is not in lobby - send reject message.
				boost::shared_ptr<NetPacket> p2(new NetPacket(NetPacket::Alloc));
				p2->GetMsg()->present = PokerTHMessage_PR_rejectInvNotifyMessage;
				RejectInvNotifyMessage_t *netRejNotif = &p2->GetMsg()->choice.rejectInvNotifyMessage;
				netRejNotif->gameId = netInvite->gameId;
				netRejNotif->playerId = netInvite->playerId;
				netRejNotif->playerRejectReason = RejectGameInvReason_busy;

				server->SendToAllPlayers(p2, SessionData::Game);
			}
		}
	}
	else if (packet->GetMsg()->present == PokerTHMessage_PR_resetTimeoutMessage)
	{
		if (session.playerData->IsGameAdmin())
		{
			RegisterAdminTimer(server);
		}
	}
	else
	{
		server->SessionError(session, ERR_SOCK_INVALID_PACKET);
	}
}

boost::shared_ptr<NetPacket>
ServerGameStateInit::CreateNetPacketPlayerJoined(unsigned gameId, const PlayerData &playerData)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
	packet->GetMsg()->present = PokerTHMessage_PR_gamePlayerMessage;
	GamePlayerMessage_t *netGamePlayer = &packet->GetMsg()->choice.gamePlayerMessage;
	netGamePlayer->gameId = gameId;
	netGamePlayer->gamePlayerNotification.present = gamePlayerNotification_PR_gamePlayerJoined;
	GamePlayerJoined_t *playerJoined = &netGamePlayer->gamePlayerNotification.choice.gamePlayerJoined;
	playerJoined->playerId = playerData.GetUniqueId();
	playerJoined->isGameAdmin = playerData.IsGameAdmin();
	return packet;
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
ServerGameStateStartGame::Enter(boost::shared_ptr<ServerGame> server)
{
	server->GetStateTimer().expires_from_now(
		boost::posix_time::seconds(SERVER_START_GAME_TIMEOUT_SEC));
	server->GetStateTimer().async_wait(
		boost::bind(
			&ServerGameStateStartGame::TimerTimeout, this, boost::asio::placeholders::error, server));
}

void
ServerGameStateStartGame::Exit(boost::shared_ptr<ServerGame> server)
{
	server->GetStateTimer().cancel();
}

void
ServerGameStateStartGame::HandleNewSession(boost::shared_ptr<ServerGame> server, SessionWrapper session)
{
	// Do not accept new sessions in this state.
	server->MoveSessionToLobby(session, NTF_NET_REMOVED_ALREADY_RUNNING);
}

void
ServerGameStateStartGame::InternalProcessPacket(boost::shared_ptr<ServerGame> server, SessionWrapper session, boost::shared_ptr<NetPacket> packet)
{
	if (packet->GetMsg()->present == PokerTHMessage_PR_startEventAckMessage)
	{
		session.sessionData->SetReadyFlag();
		if (server->GetSessionManager().CountReadySessions() == server->GetSessionManager().GetRawSessionCount())
		{
			// Everyone is ready.
			server->GetSessionManager().ResetAllReadyFlags();
			DoStart(server);
		}
	}
}

void
ServerGameStateStartGame::TimerTimeout(const boost::system::error_code &ec, boost::shared_ptr<ServerGame> server)
{
	if (!ec && &server->GetState() == this)
	{
		// On timeout: start anyway.
		server->GetSessionManager().ResetAllReadyFlags();
		// TODO report successful start! -> new callback?!
		//retVal = MSG_SOCK_INIT_DONE;
		DoStart(server);
	}
}

void
ServerGameStateStartGame::DoStart(boost::shared_ptr<ServerGame> server)
{
	PlayerDataList tmpPlayerList = server->GetFullPlayerDataList();
	if (tmpPlayerList.size() <= 1)
	{
		if (!tmpPlayerList.empty())
		{
			boost::shared_ptr<PlayerData> tmpPlayer(tmpPlayerList.front());
			SessionWrapper tmpSession = server->GetSessionManager().GetSessionByUniquePlayerId(tmpPlayer->GetUniqueId());
			if (tmpSession.sessionData.get())
				server->MoveSessionToLobby(tmpSession, NTF_NET_REMOVED_START_FAILED);
		}
	}
	else
	{
		server->InternalStartGame();

		boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
		packet->GetMsg()->present = PokerTHMessage_PR_gameStartMessage;
		GameStartMessage_t *netGameStart = &packet->GetMsg()->choice.gameStartMessage;
		netGameStart->gameId = server->GetId();
		netGameStart->startDealerPlayerId = server->GetStartData().startDealerPlayerId;

		// Send player order to clients.
		// Assume player list is sorted by number.
		PlayerDataList::iterator player_i = tmpPlayerList.begin();
		PlayerDataList::iterator player_end = tmpPlayerList.end();
		while (player_i != player_end)
		{
			NonZeroId_t *playerSlot = (NonZeroId_t *)calloc(1, sizeof(NonZeroId_t));
			*playerSlot = (*player_i)->GetUniqueId();
			ASN_SEQUENCE_ADD(&netGameStart->playerSeats.list, playerSlot);
	
			++player_i;
		}

		server->SendToAllPlayers(packet, SessionData::Game);

		// Start the first hand.
		ServerGameStateHand::StartNewHand(server);
		server->SetState(ServerGameStateHand::Instance());
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
ServerGameStateHand::Enter(boost::shared_ptr<ServerGame> server)
{
	server->GetStateTimer().expires_from_now(
		boost::posix_time::milliseconds(SERVER_LOOP_DELAY_MSEC));
	server->GetStateTimer().async_wait(
		boost::bind(
			&ServerGameStateHand::TimerLoop, this, boost::asio::placeholders::error, server));
}

void
ServerGameStateHand::Exit(boost::shared_ptr<ServerGame> server)
{
	server->GetStateTimer().cancel();
}

void
ServerGameStateHand::HandleNewSession(boost::shared_ptr<ServerGame> server, SessionWrapper session)
{
	// Do not accept new sessions in this state.
	server->MoveSessionToLobby(session, NTF_NET_REMOVED_ALREADY_RUNNING);
}

void
ServerGameStateHand::InternalProcessPacket(boost::shared_ptr<ServerGame> /*server*/, SessionWrapper /*session*/, boost::shared_ptr<NetPacket> /*packet*/)
{
	// TODO: maybe reject packet.
}

void
ServerGameStateHand::TimerLoop(const boost::system::error_code &ec, boost::shared_ptr<ServerGame> server)
{
	if (!ec && &server->GetState() == this)
	{
		try
		{
			EngineLoop(server);
		}
		catch (const PokerTHException &e)
		{
			LOG_ERROR("Game " << server->GetId() << " - Engine exception: " << e.what());
			server->RemoveAllSessions(); // Close this game on error.
		}
	}
}

void
ServerGameStateHand::EngineLoop(boost::shared_ptr<ServerGame> server)
{
	Game &curGame = server->GetGame();

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
			boost::shared_ptr<NetPacket> allIn(new NetPacket(NetPacket::Alloc));
			allIn->GetMsg()->present = PokerTHMessage_PR_allInShowCardsMessage;
			AllInShowCardsMessage_t *netAllInShow = &allIn->GetMsg()->choice.allInShowCardsMessage;
			netAllInShow->gameId = server->GetId();

			PlayerListConstIterator i = nonFoldPlayers.begin();
			PlayerListConstIterator end = nonFoldPlayers.end();

			while (i != end)
			{
				PlayerAllIn_t *playerAllIn = (PlayerAllIn_t *)calloc(1, sizeof(PlayerAllIn_t));
				playerAllIn->playerId = (*i)->getMyUniqueID();
				int tmpCards[2];
				(*i)->getMyCards(tmpCards);
				playerAllIn->allInCard1 = tmpCards[0];
				playerAllIn->allInCard2 = tmpCards[1];
				ASN_SEQUENCE_ADD(&netAllInShow->playersAllIn.list, playerAllIn);

				++i;
			}
			server->SendToAllPlayers(allIn, SessionData::Game);
			curGame.getCurrentHand()->setCardsShown(true);

			server->GetStateTimer().expires_from_now(
				boost::posix_time::seconds(SERVER_SHOW_CARDS_DELAY_SEC));
			server->GetStateTimer().async_wait(
				boost::bind(
					&ServerGameStateHand::TimerShowCards, this, boost::asio::placeholders::error, server));
		}
		else
		{
			SendNewRoundCards(*server, curGame, newRound);

			server->GetStateTimer().expires_from_now(
				boost::posix_time::seconds(GetDealCardsDelaySec(*server)));
			server->GetStateTimer().async_wait(
				boost::bind(
					&ServerGameStateHand::TimerLoop, this, boost::asio::placeholders::error, server));
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

			boost::shared_ptr<NetPacket> notification(new NetPacket(NetPacket::Alloc));
			notification->GetMsg()->present = PokerTHMessage_PR_playersTurnMessage;
			PlayersTurnMessage_t *netPlayersTurn = &notification->GetMsg()->choice.playersTurnMessage;
			netPlayersTurn->gameId = server->GetId();
			netPlayersTurn->gameState = curGame.getCurrentHand()->getCurrentRound();
			netPlayersTurn->playerId = curPlayer->getMyUniqueID();
			server->SendToAllPlayers(notification, SessionData::Game);

			// If the player is computer controlled, let the engine act.
			if (curPlayer->getMyType() == PLAYER_TYPE_COMPUTER)
			{
				server->GetStateTimer().expires_from_now(
					boost::posix_time::seconds(SERVER_COMPUTER_ACTION_DELAY_SEC));
				server->GetStateTimer().async_wait(
					boost::bind(
						&ServerGameStateHand::TimerComputerAction, this, boost::asio::placeholders::error, server));
			}
			// If the player we are waiting for left, continue without him.
			else if (!server->GetSessionManager().IsPlayerConnected(curPlayer->getMyName()))
			{
				PerformPlayerAction(*server, curPlayer, PLAYER_ACTION_FOLD, 0);

				server->GetStateTimer().expires_from_now(
					boost::posix_time::milliseconds(SERVER_LOOP_DELAY_MSEC));
				server->GetStateTimer().async_wait(
					boost::bind(
						&ServerGameStateHand::TimerLoop, this, boost::asio::placeholders::error, server));
			}
			else
			{
				server->SetState(ServerGameStateWaitPlayerAction::Instance());
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
				boost::shared_ptr<NetPacket> endHand(new NetPacket(NetPacket::Alloc));
				endHand->GetMsg()->present = PokerTHMessage_PR_endOfHandMessage;
				EndOfHandMessage_t *netEndHand = &endHand->GetMsg()->choice.endOfHandMessage;
				netEndHand->gameId = server->GetId();

				netEndHand->endOfHandType.present = endOfHandType_PR_endOfHandHideCards;
				EndOfHandHideCards_t *endHandHide = &netEndHand->endOfHandType.choice.endOfHandHideCards;
				endHandHide->playerId = player->getMyUniqueID();
				endHandHide->moneyWon = player->getLastMoneyWon();
				endHandHide->playerMoney = player->getMyCash();
				server->SendToAllPlayers(endHand, SessionData::Game);
			}
			else
			{
				// End of Hand - show cards.
				const PlayerIdList showList(curGame.getCurrentHand()->getBoard()->getPlayerNeedToShowCards());
				boost::shared_ptr<NetPacket> endHand(new NetPacket(NetPacket::Alloc));
				endHand->GetMsg()->present = PokerTHMessage_PR_endOfHandMessage;
				EndOfHandMessage_t *netEndHand = &endHand->GetMsg()->choice.endOfHandMessage;
				netEndHand->gameId = server->GetId();

				netEndHand->endOfHandType.present = endOfHandType_PR_endOfHandShowCards;
				EndOfHandShowCards_t *endHandShow = &netEndHand->endOfHandType.choice.endOfHandShowCards;

				PlayerIdList::const_iterator i = showList.begin();
				PlayerIdList::const_iterator end = showList.end();

				while (i != end)
				{
					boost::shared_ptr<PlayerInterface> tmpPlayer(curGame.getPlayerByUniqueId(*i));
					if (tmpPlayer)
					{
						PlayerResult_t *playerResult = (PlayerResult_t *)calloc(1, sizeof(PlayerResult_t));
						SetPlayerResult(*playerResult, tmpPlayer);

						ASN_SEQUENCE_ADD(&endHandShow->playerResults.list, playerResult);
					}
					++i;
				}
				server->SendToAllPlayers(endHand, SessionData::Game);
			}

			// Remove disconnected players. This is the one and only place to do this.
			server->RemoveDisconnectedPlayers();

			// Start next hand - if enough players are left.
			list<boost::shared_ptr<PlayerInterface> > playersWithCash = *curGame.getActivePlayerList();
			playersWithCash.remove_if(boost::bind(&PlayerInterface::getMyCash, _1) < 1);

			if (playersWithCash.empty())
			{
				// No more players left - restart.
				server->SetState(SERVER_INITIAL_STATE::Instance());
			}
			else if (playersWithCash.size() == 1)
			{
				// Store winner in database.
				boost::shared_ptr<PlayerInterface> winnerPlayer = *(playersWithCash.begin());
				boost::shared_ptr<PlayerData> tmpPlayer = server->GetPlayerDataByUniqueId(winnerPlayer->getMyUniqueID());
				if (tmpPlayer && server->GetDBId())
				{
					cerr << "Setting winner for game " << server->GetDBId() << " player id " << tmpPlayer->GetDBId() << endl;
					server->GetLobbyThread().GetDatabase().SetGamePlayerPlace(server->GetDBId(), tmpPlayer->GetDBId(), 1);
				}

				// View a dialog for a new game - delayed.
				server->GetStateTimer().expires_from_now(
					boost::posix_time::seconds(SERVER_DELAY_NEXT_GAME_SEC));
				server->GetStateTimer().async_wait(
					boost::bind(
						&ServerGameStateHand::TimerNextGame, this, boost::asio::placeholders::error, server, winnerPlayer->getMyUniqueID()));
			}
			else
			{
				server->SetState(ServerGameStateWaitNextHand::Instance());
			}
		}
	}
}

void
ServerGameStateHand::TimerShowCards(const boost::system::error_code &ec, boost::shared_ptr<ServerGame> server)
{
	if (!ec && &server->GetState() == this)
	{
		Game &curGame = server->GetGame();
		SendNewRoundCards(*server, curGame, curGame.getCurrentHand()->getCurrentRound());

		server->GetStateTimer().expires_from_now(
			boost::posix_time::seconds(GetDealCardsDelaySec(*server)));
		server->GetStateTimer().async_wait(
			boost::bind(
				&ServerGameStateHand::TimerLoop, this, boost::asio::placeholders::error, server));
	}
}

void
ServerGameStateHand::TimerComputerAction(const boost::system::error_code &ec, boost::shared_ptr<ServerGame> server)
{
	if (!ec && &server->GetState() == this)
	{
		try
		{
			boost::shared_ptr<PlayerInterface> curPlayer = server->GetGame().getCurrentPlayer();
			if (!curPlayer)
				throw ServerException(__FILE__, __LINE__, ERR_NET_NO_CURRENT_PLAYER, 0);

			curPlayer->action();
			SendPlayerAction(*server, curPlayer);
			EngineLoop(server);
		}
		catch (const PokerTHException &e)
		{
			LOG_ERROR("Game " << server->GetId() << " - Computer timer exception: " << e.what());
			server->RemoveAllSessions(); // Close this game on error.
		}
	}
}

void
ServerGameStateHand::TimerNextHand(const boost::system::error_code &ec, boost::shared_ptr<ServerGame> server)
{
	if (!ec && &server->GetState() == this)
	{
		StartNewHand(server);
		TimerLoop(ec, server);
	}
}

void
ServerGameStateHand::TimerNextGame(const boost::system::error_code &ec, boost::shared_ptr<ServerGame> server, unsigned winnerPlayerId)
{
	if (!ec && &server->GetState() == this)
	{
		boost::shared_ptr<NetPacket> endGame(new NetPacket(NetPacket::Alloc));
		endGame->GetMsg()->present = PokerTHMessage_PR_endOfGameMessage;
		EndOfGameMessage_t *netEndGame = &endGame->GetMsg()->choice.endOfGameMessage;
		netEndGame->gameId = server->GetId();
		netEndGame->winnerPlayerId = winnerPlayerId;
		server->SendToAllPlayers(endGame, SessionData::Game);

		// Wait for the start of a new game.
		server->ResetComputerPlayerList();
		server->ResetGame();
		server->GetLobbyThread().NotifyReopeningGame(server->GetId());
		server->SetState(ServerGameStateInit::Instance());
	}
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

void
ServerGameStateHand::StartNewHand(boost::shared_ptr<ServerGame> server)
{
	// Initialize hand.
	Game &curGame = server->GetGame();
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

			boost::shared_ptr<NetPacket> notifyCards(new NetPacket(NetPacket::Alloc));
			notifyCards->GetMsg()->present = PokerTHMessage_PR_handStartMessage;
			HandStartMessage_t *netHandStart = &notifyCards->GetMsg()->choice.handStartMessage;
			netHandStart->gameId = server->GetId();
			netHandStart->yourCard1 = cards[0];
			netHandStart->yourCard2 = cards[1];
			netHandStart->smallBlind = curGame.getCurrentHand()->getSmallBlind();
			server->GetLobbyThread().GetSender().Send(tmpPlayer->getNetSessionData(), notifyCards);
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
			boost::shared_ptr<NetPacket> notifySmallBlind(new NetPacket(NetPacket::Alloc));
			notifySmallBlind->GetMsg()->present = PokerTHMessage_PR_playersActionDoneMessage;
			PlayersActionDoneMessage_t *netSmallBlind = &notifySmallBlind->GetMsg()->choice.playersActionDoneMessage;
			netSmallBlind->gameId = server->GetId();
			netSmallBlind->gameState = NetGameState_statePreflopSmallBlind;
			netSmallBlind->playerId = tmpPlayer->getMyUniqueID();
			netSmallBlind->playerAction = tmpPlayer->getMyAction();
			netSmallBlind->totalPlayerBet = tmpPlayer->getMySet();
			netSmallBlind->playerMoney = tmpPlayer->getMyCash();
			netSmallBlind->highestSet = server->GetGame().getCurrentHand()->getCurrentBeRo()->getHighestSet();
			netSmallBlind->minimumRaise = server->GetGame().getCurrentHand()->getCurrentBeRo()->getMinimumRaise();
			server->SendToAllPlayers(notifySmallBlind, SessionData::Game);
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
			boost::shared_ptr<NetPacket> notifyBigBlind(new NetPacket(NetPacket::Alloc));
			notifyBigBlind->GetMsg()->present = PokerTHMessage_PR_playersActionDoneMessage;
			PlayersActionDoneMessage_t *netBigBlind = &notifyBigBlind->GetMsg()->choice.playersActionDoneMessage;
			netBigBlind->gameId = server->GetId();
			netBigBlind->gameState = NetGameState_statePreflopBigBlind;
			netBigBlind->playerId = tmpPlayer->getMyUniqueID();
			netBigBlind->playerAction = tmpPlayer->getMyAction();
			netBigBlind->totalPlayerBet = tmpPlayer->getMySet();
			netBigBlind->playerMoney = tmpPlayer->getMyCash();
			netBigBlind->highestSet = server->GetGame().getCurrentHand()->getCurrentBeRo()->getHighestSet();
			netBigBlind->minimumRaise = server->GetGame().getCurrentHand()->getCurrentBeRo()->getMinimumRaise();
			server->SendToAllPlayers(notifyBigBlind, SessionData::Game);
			break;
		}
		++i;
	}
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
ServerGameStateWaitPlayerAction::Enter(boost::shared_ptr<ServerGame> server)
{
#ifdef SERVER_TEST
	int timeoutSec = 0;
#else
	int timeoutSec = server->GetGameData().playerActionTimeoutSec + SERVER_PLAYER_TIMEOUT_ADD_DELAY_SEC;
#endif

	server->GetStateTimer().expires_from_now(boost::posix_time::seconds(timeoutSec));
	server->GetStateTimer().async_wait(
		boost::bind(
			&ServerGameStateWaitPlayerAction::TimerTimeout, this, boost::asio::placeholders::error, server));
}

void
ServerGameStateWaitPlayerAction::Exit(boost::shared_ptr<ServerGame> server)
{
	server->GetStateTimer().cancel();
}

void
ServerGameStateWaitPlayerAction::HandleNewSession(boost::shared_ptr<ServerGame> server, SessionWrapper session)
{
	// Do not accept new sessions in this state.
	server->MoveSessionToLobby(session, NTF_NET_REMOVED_ALREADY_RUNNING);
}

void
ServerGameStateWaitPlayerAction::InternalProcessPacket(boost::shared_ptr<ServerGame> server, SessionWrapper session, boost::shared_ptr<NetPacket> packet)
{
	if (packet->GetMsg()->present == PokerTHMessage_PR_myActionRequestMessage)
	{
		MyActionRequestMessage_t *netMyAction = &packet->GetMsg()->choice.myActionRequestMessage;

		// TODO consider game id.
		Game &curGame = server->GetGame();
		boost::shared_ptr<PlayerInterface> tmpPlayer = curGame.getPlayerByUniqueId(session.playerData->GetUniqueId());
		if (!tmpPlayer.get())
			throw ServerException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);

		// Check whether this is the correct round.
		PlayerActionCode code = ACTION_CODE_VALID;
		if (curGame.getCurrentHand()->getCurrentRound() != netMyAction->gameState)
			code = ACTION_CODE_INVALID_STATE;

		// Check whether this is the correct player.
		boost::shared_ptr<PlayerInterface> curPlayer = server->GetGame().getCurrentPlayer();
		if (code == ACTION_CODE_VALID
			&& (curPlayer->getMyUniqueID() != tmpPlayer->getMyUniqueID()))
		{
			code = ACTION_CODE_NOT_YOUR_TURN;
		}

		// If the client omitted some values, fill them in.
		if (netMyAction->myAction == PLAYER_ACTION_CALL && netMyAction->myRelativeBet == 0)
		{
			if (curGame.getCurrentHand()->getCurrentBeRo()->getHighestSet() >= tmpPlayer->getMySet() + tmpPlayer->getMyCash())
				netMyAction->myAction = PLAYER_ACTION_ALLIN;
			else
				netMyAction->myRelativeBet = curGame.getCurrentHand()->getCurrentBeRo()->getHighestSet() - tmpPlayer->getMySet();
		}
		if (netMyAction->myAction == PLAYER_ACTION_ALLIN && netMyAction->myRelativeBet == 0)
			netMyAction->myRelativeBet = tmpPlayer->getMyCash();

		// Check whether the action is valid.
		if (code == ACTION_CODE_VALID
			&& (tmpPlayer->checkMyAction(
					netMyAction->myAction,
					netMyAction->myRelativeBet,
					curGame.getCurrentHand()->getCurrentBeRo()->getHighestSet(),
					curGame.getCurrentHand()->getCurrentBeRo()->getMinimumRaise(),
					curGame.getCurrentHand()->getSmallBlind()) != 0))
		{
			code = ACTION_CODE_NOT_ALLOWED;
		}

		if (code == ACTION_CODE_VALID)
		{
			PerformPlayerAction(*server, tmpPlayer, static_cast<PlayerAction>(netMyAction->myAction), netMyAction->myRelativeBet);
			server->SetState(ServerGameStateHand::Instance());
		}
		else
		{
			// Send reject message.
			boost::shared_ptr<NetPacket> reject(new NetPacket(NetPacket::Alloc));
			reject->GetMsg()->present = PokerTHMessage_PR_yourActionRejectedMessage;
			YourActionRejectedMessage_t *netActionRejected = &reject->GetMsg()->choice.yourActionRejectedMessage;
			netActionRejected->gameId = server->GetId();
			netActionRejected->gameState = netMyAction->gameState;
			netActionRejected->yourAction = netMyAction->myAction;
			netActionRejected->yourRelativeBet = netMyAction->myRelativeBet;
			netActionRejected->rejectionReason = code;
			server->GetLobbyThread().GetSender().Send(session.sessionData, reject);
		}
	}
}

void
ServerGameStateWaitPlayerAction::TimerTimeout(const boost::system::error_code &ec, boost::shared_ptr<ServerGame> server)
{
	if (!ec && &server->GetState() == this)
	{
		try
		{
			Game &curGame = server->GetGame();
			// Retrieve current player.
			boost::shared_ptr<PlayerInterface> curPlayer = curGame.getCurrentPlayer();
			if (!curPlayer.get())
				throw ServerException(__FILE__, __LINE__, ERR_NET_NO_CURRENT_PLAYER, 0);

			// Player did not act fast enough. Act for him.
			if (curGame.getCurrentHand()->getCurrentBeRo()->getHighestSet() == curPlayer->getMySet())
				PerformPlayerAction(*server, curPlayer, PLAYER_ACTION_CHECK, 0);
			else
				PerformPlayerAction(*server, curPlayer, PLAYER_ACTION_FOLD, 0);

			server->SetState(ServerGameStateHand::Instance());
		}
		catch (const PokerTHException &e)
		{
			LOG_ERROR("Game " << server->GetId() << " - Player timer exception: " << e.what());
			server->RemoveAllSessions(); // Close this game on error.
		}
	}
}

//-----------------------------------------------------------------------------


ServerGameStateWaitNextHand ServerGameStateWaitNextHand::s_state;

ServerGameStateWaitNextHand &
ServerGameStateWaitNextHand::Instance()
{
	return s_state;
}

ServerGameStateWaitNextHand::ServerGameStateWaitNextHand()
{
}

ServerGameStateWaitNextHand::~ServerGameStateWaitNextHand()
{
}

void
ServerGameStateWaitNextHand::Enter(boost::shared_ptr<ServerGame> server)
{
#ifdef SERVER_TEST
	int timeoutSec = 0;
#else
	int timeoutSec = server->GetGameData().delayBetweenHandsSec;
#endif

	server->GetStateTimer().expires_from_now(
		boost::posix_time::seconds(timeoutSec));

	server->GetStateTimer().async_wait(
		boost::bind(
			&ServerGameStateWaitNextHand::TimerTimeout, this, boost::asio::placeholders::error, server));
}

void
ServerGameStateWaitNextHand::Exit(boost::shared_ptr<ServerGame> server)
{
	server->GetStateTimer().cancel();
}

void
ServerGameStateWaitNextHand::HandleNewSession(boost::shared_ptr<ServerGame> server, SessionWrapper session)
{
	// Do not accept new sessions in this state.
	server->MoveSessionToLobby(session, NTF_NET_REMOVED_ALREADY_RUNNING);
}

void
ServerGameStateWaitNextHand::InternalProcessPacket(boost::shared_ptr<ServerGame> server, SessionWrapper session, boost::shared_ptr<NetPacket> packet)
{
	if (packet->GetMsg()->present == PokerTHMessage_PR_showMyCardsRequestMessage)
	{
		Game &curGame = server->GetGame();
		boost::shared_ptr<NetPacket> show(new NetPacket(NetPacket::Alloc));
		show->GetMsg()->present = PokerTHMessage_PR_afterHandShowCardsMessage;

		AfterHandShowCardsMessage_t *netShowCards = &show->GetMsg()->choice.afterHandShowCardsMessage;
		boost::shared_ptr<PlayerInterface> tmpPlayer(curGame.getPlayerByUniqueId(session.playerData->GetUniqueId()));
		if (tmpPlayer)
		{
			SetPlayerResult(netShowCards->playerResult, tmpPlayer);
			server->SendToAllPlayers(show, SessionData::Game);
		}
	}
}

void
ServerGameStateWaitNextHand::TimerTimeout(const boost::system::error_code &ec, boost::shared_ptr<ServerGame> server)
{
	if (!ec && &server->GetState() == this)
	{
		ServerGameStateHand::StartNewHand(server);
		server->SetState(ServerGameStateHand::Instance());
	}
}

//-----------------------------------------------------------------------------

ServerGameStateFinal ServerGameStateFinal::s_state;

ServerGameStateFinal &
ServerGameStateFinal::Instance()
{
	return s_state;
}

//-----------------------------------------------------------------------------

