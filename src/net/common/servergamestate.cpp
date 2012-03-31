/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2011 Felix Hammer, Florian Thauer, Lothar May          *
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
 *****************************************************************************/

#include <net/servergamestate.h>
#include <net/servergame.h>
#include <net/serverlobbythread.h>
#include <net/senderhelper.h>
#include <net/netpacket.h>
#include <net/socket_msg.h>
#include <net/serverexception.h>
#include <net/net_helper.h>
#include <net/chatcleanermanager.h>
#include <db/serverdbinterface.h>
#include <core/loghelper.h>
#include <core/avatarmanager.h>
#include <gamedata.h>
#include <game.h>
#include <playerinterface.h>
#include <handinterface.h>

#include <boost/bind.hpp>

#include <sstream>

using namespace std;

//#define POKERTH_SERVER_TEST

#ifdef POKERTH_SERVER_TEST
#define SERVER_DELAY_NEXT_GAME_SEC				0
#define SERVER_DEAL_FLOP_CARDS_DELAY_SEC		0
#define SERVER_DEAL_TURN_CARD_DELAY_SEC			0
#define SERVER_DEAL_RIVER_CARD_DELAY_SEC		0
#define SERVER_DEAL_ADD_ALL_IN_DELAY_SEC		0
#define SERVER_SHOW_CARDS_DELAY_SEC				0
#define SERVER_COMPUTER_ACTION_DELAY_SEC		0
#define SERVER_PLAYER_TIMEOUT_ADD_DELAY_SEC		1
#else
#define SERVER_DELAY_NEXT_GAME_SEC				10
#define SERVER_DEAL_FLOP_CARDS_DELAY_SEC		5
#define SERVER_DEAL_TURN_CARD_DELAY_SEC			2
#define SERVER_DEAL_RIVER_CARD_DELAY_SEC		2
#define SERVER_DEAL_ADD_ALL_IN_DELAY_SEC		2
#define SERVER_SHOW_CARDS_DELAY_SEC				2
#define SERVER_COMPUTER_ACTION_DELAY_SEC		2
#define SERVER_PLAYER_TIMEOUT_ADD_DELAY_SEC		2
#endif

#define SERVER_START_GAME_TIMEOUT_SEC				10
#define SERVER_AUTOSTART_GAME_DELAY_SEC				6
#define SERVER_GAME_ADMIN_WARNING_REMAINING_SEC		60
#define SERVER_GAME_ADMIN_TIMEOUT_SEC				300		// 5 min, MUST be > SERVER_GAME_ADMIN_WARNING_REMAINING_SEC
#define SERVER_GAME_AUTOFOLD_TIMEOUT_FACTOR			30
#define SERVER_GAME_FORCED_TIMEOUT_FACTOR			60
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
	netActionDone->playerAction = player->getMyAction();
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
	}
	break;
	case GAME_STATE_TURN: {
		// deal turn card
		boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
		packet->GetMsg()->present = PokerTHMessage_PR_dealTurnCardMessage;
		DealTurnCardMessage_t *netDealTurn = &packet->GetMsg()->choice.dealTurnCardMessage;
		netDealTurn->gameId = server.GetId();
		netDealTurn->turnCard = cards[3];
		server.SendToAllPlayers(packet, SessionData::Game);
	}
	break;
	case GAME_STATE_RIVER: {
		// deal river card
		boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
		packet->GetMsg()->present = PokerTHMessage_PR_dealRiverCardMessage;
		DealRiverCardMessage_t *netDealRiver = &packet->GetMsg()->choice.dealRiverCardMessage;
		netDealRiver->gameId = server.GetId();
		netDealRiver->riverCard = cards[4];
		server.SendToAllPlayers(packet, SessionData::Game);
	}
	break;
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
	if (action != PLAYER_ACTION_FOLD && action != PLAYER_ACTION_CHECK) {

		player->setMySet(bet);

		// update minimumRaise and lastActionPlayer
		switch(action) {
		case PLAYER_ACTION_BET: {
			curGame.getCurrentHand()->getCurrentBeRo()->setMinimumRaise(bet);
			curGame.getCurrentHand()->setLastActionPlayerID(player->getMyUniqueID());
		}
		break;
		case PLAYER_ACTION_RAISE: {
			curGame.getCurrentHand()->getCurrentBeRo()->setMinimumRaise(player->getMySet() - curGame.getCurrentHand()->getCurrentBeRo()->getHighestSet());
			curGame.getCurrentHand()->setLastActionPlayerID(player->getMyUniqueID());
		}
		break;
		case PLAYER_ACTION_ALLIN: {
			if(player->getMySet() - curGame.getCurrentHand()->getCurrentBeRo()->getHighestSet() > curGame.getCurrentHand()->getCurrentBeRo()->getMinimumRaise()) {
				curGame.getCurrentHand()->getCurrentBeRo()->setMinimumRaise(player->getMySet() - curGame.getCurrentHand()->getCurrentBeRo()->getHighestSet());
			}
			if(player->getMySet() - curGame.getCurrentHand()->getCurrentBeRo()->getHighestSet() > 0) {
				curGame.getCurrentHand()->setLastActionPlayerID(player->getMyUniqueID());
			}
		}
		break;
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
SetPlayerResult(PlayerResult_t &playerResult, boost::shared_ptr<PlayerInterface> tmpPlayer, GameState roundBeforePostRiver)
{
	playerResult.playerId = tmpPlayer->getMyUniqueID();
	int tmpCards[2];
	int bestHandPos[5];
	tmpPlayer->getMyCards(tmpCards);
	playerResult.resultCard1 = tmpCards[0];
	playerResult.resultCard2 = tmpCards[1];
	tmpPlayer->getMyBestHandPosition(bestHandPos);
	for (int num = 0; num < 5; num++) {
		long *handPos = (long *)calloc(1, sizeof(long));
		*handPos = bestHandPos[num];
		ASN_SEQUENCE_ADD(&playerResult.bestHandPosition.list, handPos);
	}

	if (roundBeforePostRiver == GAME_STATE_RIVER) {
		playerResult.cardsValue = (long *)calloc(1, sizeof(long));
		*playerResult.cardsValue = tmpPlayer->getMyCardsValueInt();
	}
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
AbstractServerGameStateReceiving::ProcessPacket(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet)
{
	if (packet->IsClientActivity()) {
		session->ResetActivityTimer();
	}
	if (packet->GetMsg()->present == PokerTHMessage_PR_playerInfoRequestMessage) {
		// Delegate to Lobby.
		server->GetLobbyThread().HandleGameRetrievePlayerInfo(session, packet->GetMsg()->choice.playerInfoRequestMessage);
	} else if (packet->GetMsg()->present == PokerTHMessage_PR_avatarRequestMessage) {
		// Delegate to Lobby.
		server->GetLobbyThread().HandleGameRetrieveAvatar(session, packet->GetMsg()->choice.avatarRequestMessage);
	} else if (packet->GetMsg()->present == PokerTHMessage_PR_leaveGameRequestMessage) {
		server->MoveSessionToLobby(session, NTF_NET_REMOVED_ON_REQUEST);
	} else if (packet->GetMsg()->present == PokerTHMessage_PR_kickPlayerRequestMessage) {
		// Only admins are allowed to kick, and only in the lobby.
		// After leaving the lobby, a vote needs to be initiated to kick.
		KickPlayerRequestMessage_t *netKickRequest = &packet->GetMsg()->choice.kickPlayerRequestMessage;
		if (session->GetPlayerData()->IsGameAdmin() && !server->IsRunning()
				&& netKickRequest->gameId == server->GetId() && server->GetGameData().gameType != GAME_TYPE_RANKING) {
			server->KickPlayer(netKickRequest->playerId);
		}
	} else if (packet->GetMsg()->present == PokerTHMessage_PR_askKickPlayerMessage) {
		if (server->GetGameData().gameType != GAME_TYPE_RANKING) {
			AskKickPlayerMessage_t *netAskKick = &packet->GetMsg()->choice.askKickPlayerMessage;
			server->InternalAskVoteKick(session, netAskKick->playerId, SERVER_VOTE_KICK_TIMEOUT_SEC);
		}
	} else if (packet->GetMsg()->present == PokerTHMessage_PR_voteKickRequestMessage) {
		VoteKickRequestMessage_t *netVoteKick = &packet->GetMsg()->choice.voteKickRequestMessage;
		server->InternalVoteKick(session, netVoteKick->petitionId, netVoteKick->voteKick ? KICK_VOTE_IN_FAVOUR : KICK_VOTE_AGAINST);
	}
	// Chat text is always allowed.
	else if (packet->GetMsg()->present == PokerTHMessage_PR_chatRequestMessage) {
		bool chatSent = false;
		ChatRequestMessage_t *netChatRequest = &packet->GetMsg()->choice.chatRequestMessage;
		// Only forward if this player is known and not a guest.
		if (session->GetPlayerData()->GetRights() != PLAYER_RIGHTS_GUEST) {
			// Forward chat text to all players.
			// TODO: Some limitation needed.
			if (netChatRequest->chatRequestType.present == chatRequestType_PR_chatRequestTypeLobby
					|| netChatRequest->chatRequestType.present == chatRequestType_PR_chatRequestTypePrivate) {
				if (!server->IsRunning()) {
					server->GetLobbyThread().HandleChatRequest(session, *netChatRequest);
					chatSent = true;
				}
			} else if (netChatRequest->chatRequestType.present == chatRequestType_PR_chatRequestTypeGame) {
				boost::shared_ptr<PlayerInterface> tmpPlayer (server->GetPlayerInterfaceFromGame(session->GetPlayerData()->GetUniqueId()));
				// If we did not find the player, then the game did not start yet. Allow chat for now.
				// Otherwise, check whether the player is muted.
				if (!tmpPlayer || !tmpPlayer->isMuted()) {
					boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
					packet->GetMsg()->present = PokerTHMessage_PR_chatMessage;
					ChatMessage_t *netChat = &packet->GetMsg()->choice.chatMessage;
					netChat->chatType.present = chatType_PR_chatTypeGame;
					ChatTypeGame_t *netGameChat = &netChat->chatType.choice.chatTypeGame;
					netGameChat->gameId = server->GetId();
					netGameChat->playerId = session->GetPlayerData()->GetUniqueId();
					OCTET_STRING_fromBuf(
						&netChat->chatText,
						(char *)netChatRequest->chatText.buf,
						netChatRequest->chatText.size);
					server->SendToAllPlayers(packet, SessionData::Game);
					chatSent = true;

					// Send the message to the chat cleaner bot for ranking games.
					//if (server->GetGameData().gameType == GAME_TYPE_RANKING)
					//{
					server->GetLobbyThread().GetChatCleaner().HandleGameChatText(
						server->GetId(),
						session->GetPlayerData()->GetUniqueId(),
						session->GetPlayerData()->GetName(),
						string((char *)netChatRequest->chatText.buf, netChatRequest->chatText.size));
					//}
				}
			}
		}
		// Reject chat otherwise.
		if (!chatSent) {
			boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
			packet->GetMsg()->present = PokerTHMessage_PR_chatRejectMessage;
			ChatRejectMessage_t *netReject = &packet->GetMsg()->choice.chatRejectMessage;
			OCTET_STRING_fromBuf(
				&netReject->chatText,
				(char *)netChatRequest->chatText.buf,
				netChatRequest->chatText.size);
			server->GetLobbyThread().GetSender().Send(session, packet);
		}
	} else if (packet->GetMsg()->present == PokerTHMessage_PR_subscriptionRequestMessage) {
		SubscriptionRequestMessage_t *netSubscription = &packet->GetMsg()->choice.subscriptionRequestMessage;
		if (netSubscription->subscriptionAction == subscriptionAction_resubscribeGameList) {
			if (!session->WantsLobbyMsg())
				server->GetLobbyThread().ResubscribeLobbyMsg(session);
		} else
			session->ResetWantsLobbyMsg();
	} else if (packet->GetMsg()->present == PokerTHMessage_PR_reportAvatarMessage) {
		ReportAvatarMessage_t *netReport = &packet->GetMsg()->choice.reportAvatarMessage;
		boost::shared_ptr<PlayerData> tmpPlayer = server->GetPlayerDataByUniqueId(netReport->reportedPlayerId);
		MD5Buf tmpMD5;
		memcpy(tmpMD5.GetData(), netReport->reportedAvatar.buf, MD5_DATA_SIZE);
		if (tmpPlayer && tmpPlayer->GetDBId() && !tmpMD5.IsZero() && tmpPlayer->GetAvatarMD5() == tmpMD5) {
			if (!server->IsAvatarReported(tmpPlayer->GetUniqueId())) {
				// Temporarily note that this avatar was reported.
				// This prevents spamming of the avatar report.
				server->AddReportedAvatar(tmpPlayer->GetUniqueId());
				DB_id myDBid = session->GetPlayerData()->GetDBId();
				// Do not use the "game" database object, but the global one.
				// The entry should be created even if we are not running a
				// ranking game.

				string tmpAvatarType;
				tmpAvatarType = AvatarManager::GetAvatarFileExtension(AvatarManager::GetAvatarFileType(tmpPlayer->GetAvatarFile()));
				if (!tmpAvatarType.empty())
					tmpAvatarType.erase(0, 1); // Only store extension without the "."

				server->GetLobbyThread().GetDatabase()->AsyncReportAvatar(
					session->GetPlayerData()->GetUniqueId(),
					tmpPlayer->GetUniqueId(),
					tmpPlayer->GetDBId(),
					tmpPlayer->GetAvatarMD5().ToString(),
					tmpAvatarType,
					myDBid != 0 ? &myDBid : NULL
				);
			} else {
				boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
				packet->GetMsg()->present = PokerTHMessage_PR_reportAvatarAckMessage;
				ReportAvatarAckMessage_t *netReportAck = &packet->GetMsg()->choice.reportAvatarAckMessage;
				netReportAck->reportedPlayerId = netReport->reportedPlayerId;
				netReportAck->reportAvatarResult = reportAvatarResult_avatarReportDuplicate;
				server->GetLobbyThread().GetSender().Send(session, packet);
			}
		} else {
			boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
			packet->GetMsg()->present = PokerTHMessage_PR_reportAvatarAckMessage;
			ReportAvatarAckMessage_t *netReportAck = &packet->GetMsg()->choice.reportAvatarAckMessage;
			netReportAck->reportedPlayerId = netReport->reportedPlayerId;
			netReportAck->reportAvatarResult = reportAvatarResult_avatarReportInvalid;
			server->GetLobbyThread().GetSender().Send(session, packet);
		}
	} else {
		// Packet processing in subclass.
		InternalProcessPacket(server, session, packet);
	}
}

boost::shared_ptr<NetPacket>
AbstractServerGameStateReceiving::CreateNetPacketPlayerJoined(unsigned gameId, const PlayerData &playerData)
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

boost::shared_ptr<NetPacket>
AbstractServerGameStateReceiving::CreateNetPacketJoinGameAck(const ServerGame &server, const PlayerData &playerData)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
	packet->GetMsg()->present = PokerTHMessage_PR_joinGameReplyMessage;
	JoinGameReplyMessage_t *netJoinReply = &packet->GetMsg()->choice.joinGameReplyMessage;
	netJoinReply->gameId = server.GetId();
	netJoinReply->joinGameResult.present = joinGameResult_PR_joinGameAck;
	JoinGameAck_t *joinAck = &netJoinReply->joinGameResult.choice.joinGameAck;
	joinAck->areYouGameAdmin = static_cast<PlayerInfoRights>(playerData.IsGameAdmin());

	NetPacket::SetGameData(server.GetGameData(), &joinAck->gameInfo);
	OCTET_STRING_fromBuf(
		&joinAck->gameInfo.gameName,
		server.GetName().c_str(),
		(int)server.GetName().length());
	return packet;
}

void
AbstractServerGameStateReceiving::AcceptNewSession(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session)
{
	// Set game admin, if applicable.
	session->GetPlayerData()->SetGameAdmin(session->GetPlayerData()->GetUniqueId() == server->GetAdminPlayerId());

	// Send ack to client.
	server->GetLobbyThread().GetSender().Send(session, CreateNetPacketJoinGameAck(*server, *session->GetPlayerData()));

	// Send notifications for connected players to client.
	PlayerDataList tmpPlayerList(server->GetFullPlayerDataList());
	PlayerDataList::iterator player_i = tmpPlayerList.begin();
	PlayerDataList::iterator player_end = tmpPlayerList.end();
	while (player_i != player_end) {
		server->GetLobbyThread().GetSender().Send(session, CreateNetPacketPlayerJoined(server->GetId(), *(*player_i)));
		++player_i;
	}

	// Send "Player Joined" to other fully connected clients.
	server->SendToAllPlayers(CreateNetPacketPlayerJoined(server->GetId(), *session->GetPlayerData()), SessionData::Game);

	// Accept session.
	server->GetSessionManager().AddSession(session);

	// Notify lobby.
	server->GetLobbyThread().NotifyPlayerJoinedGame(server->GetId(), session->GetPlayerData()->GetUniqueId());
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
	UnregisterAutoStartTimer(server);
}

void
ServerGameStateInit::NotifyGameAdminChanged(boost::shared_ptr<ServerGame> server)
{
	UnregisterAdminTimer(server);
	RegisterAdminTimer(server);
}

void
ServerGameStateInit::NotifySessionRemoved(boost::shared_ptr<ServerGame> server)
{
	UnregisterAutoStartTimer(server);
}

void
ServerGameStateInit::HandleNewSession(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session)
{
	if (session && session->GetPlayerData()) {
		const GameData &tmpGameData = server->GetGameData();
		// Check the number of players.
		if (server->GetCurNumberOfPlayers() >= tmpGameData.maxNumberOfPlayers) {
			server->MoveSessionToLobby(session, NTF_NET_REMOVED_GAME_FULL);
		} else {

			AcceptNewSession(server, session);

			if (server->GetCurNumberOfPlayers() == tmpGameData.maxNumberOfPlayers) {
				// Automatically start the game if it is full.
				RegisterAutoStartTimer(server);
			}
		}
	}
}

void
ServerGameStateInit::RegisterAdminTimer(boost::shared_ptr<ServerGame> server)
{
	// No admin timeout in LAN or ranking games.
	if (server->GetLobbyThread().GetServerMode() != SERVER_MODE_LAN && server->GetGameData().gameType != GAME_TYPE_RANKING) {
		server->GetStateTimer1().expires_from_now(
			boost::posix_time::seconds(SERVER_GAME_ADMIN_TIMEOUT_SEC - SERVER_GAME_ADMIN_WARNING_REMAINING_SEC));
		server->GetStateTimer1().async_wait(
			boost::bind(
				&ServerGameStateInit::TimerAdminWarning, this, boost::asio::placeholders::error, server));
	}
}

void
ServerGameStateInit::UnregisterAdminTimer(boost::shared_ptr<ServerGame> server)
{
	server->GetStateTimer1().cancel();
}

void
ServerGameStateInit::RegisterAutoStartTimer(boost::shared_ptr<ServerGame> server)
{
	// No autostart in LAN games.
	if (server->GetLobbyThread().GetServerMode() != SERVER_MODE_LAN) {
		server->GetStateTimer2().expires_from_now(
			boost::posix_time::seconds(SERVER_AUTOSTART_GAME_DELAY_SEC));
		server->GetStateTimer2().async_wait(
			boost::bind(
				&ServerGameStateInit::TimerAutoStart, this, boost::asio::placeholders::error, server));
	}
}

void
ServerGameStateInit::UnregisterAutoStartTimer(boost::shared_ptr<ServerGame> server)
{
	server->GetStateTimer2().cancel();
}

void
ServerGameStateInit::TimerAutoStart(const boost::system::error_code &ec, boost::shared_ptr<ServerGame> server)
{
	if (!ec && &server->GetState() == this) {
		SendStartEvent(*server, false);
	}
}

void
ServerGameStateInit::TimerAdminWarning(const boost::system::error_code &ec, boost::shared_ptr<ServerGame> server)
{
	if (!ec && &server->GetState() == this) {
		// Find game admin.
		boost::shared_ptr<SessionData> session = server->GetSessionManager().GetSessionByUniquePlayerId(server->GetAdminPlayerId());
		if (session) {
			// Send him a warning.
			boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
			packet->GetMsg()->present = PokerTHMessage_PR_timeoutWarningMessage;
			TimeoutWarningMessage_t *netWarning = &packet->GetMsg()->choice.timeoutWarningMessage;
			netWarning->timeoutReason = NETWORK_TIMEOUT_GAME_ADMIN_IDLE;
			netWarning->remainingSeconds = SERVER_GAME_ADMIN_WARNING_REMAINING_SEC;
			server->GetLobbyThread().GetSender().Send(session, packet);
		}
		// Start timeout timer.
		server->GetStateTimer1().expires_from_now(
			boost::posix_time::seconds(SERVER_GAME_ADMIN_WARNING_REMAINING_SEC));
		server->GetStateTimer1().async_wait(
			boost::bind(
				&ServerGameStateInit::TimerAdminTimeout, this, boost::asio::placeholders::error, server));
	}
}

void
ServerGameStateInit::TimerAdminTimeout(const boost::system::error_code &ec, boost::shared_ptr<ServerGame> server)
{
	if (!ec && &server->GetState() == this) {
		// Find game admin.
		boost::shared_ptr<SessionData> session = server->GetSessionManager().GetSessionByUniquePlayerId(server->GetAdminPlayerId());
		if (session) {
			// Remove him from the game.
			server->MoveSessionToLobby(session, NTF_NET_REMOVED_TIMEOUT);
		}
	}
}

void
ServerGameStateInit::SendStartEvent(ServerGame &server, bool fillWithComputerPlayers)
{
	if (fillWithComputerPlayers) {
		int remainingSlots = server.GetGameData().maxNumberOfPlayers - server.GetCurNumberOfPlayers();
		for (int i = 1; i <= remainingSlots; i++) {
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

	netStartEvent->startEventType.present = startEventType_PR_startEvent;
	StartEvent_t *start = &netStartEvent->startEventType.choice.startEvent;
	start->fillWithComputerPlayers = fillWithComputerPlayers;

	// Wait for all players to confirm start of game.
	server.SendToAllPlayers(packet, SessionData::Game);
	// Notify lobby that this game is running.
	server.GetLobbyThread().NotifyStartingGame(server.GetId());

	server.SetState(ServerGameStateStartGame::Instance());
}

void
ServerGameStateInit::InternalProcessPacket(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet)
{
	if (packet->GetMsg()->present == PokerTHMessage_PR_startEventMessage) {
		StartEventMessage_t *netStartEvent = &packet->GetMsg()->choice.startEventMessage;
		// Only admins are allowed to start the game.
		if (session->GetPlayerData()->IsGameAdmin()
				&& netStartEvent->gameId == server->GetId()
				&& netStartEvent->startEventType.present == startEventType_PR_startEvent
				&& (server->GetGameData().gameType != GAME_TYPE_RANKING // ranking games need to be full
					|| server->GetGameData().maxNumberOfPlayers == server->GetCurNumberOfPlayers())) {
			SendStartEvent(*server, netStartEvent->startEventType.choice.startEvent.fillWithComputerPlayers != 0);
		} else { // kick players who try to start but are not allowed to
			server->MoveSessionToLobby(session, NTF_NET_REMOVED_START_FAILED);
		}
	} else if (packet->GetMsg()->present == PokerTHMessage_PR_invitePlayerToGameMessage) {
		InvitePlayerToGameMessage_t *netInvite = &packet->GetMsg()->choice.invitePlayerToGameMessage;

		// Only invite players which are not already within the group.
		if (netInvite->gameId == server->GetId() && !server->IsPlayerConnected(netInvite->playerId)) {
			boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
			packet->GetMsg()->present = PokerTHMessage_PR_inviteNotifyMessage;
			InviteNotifyMessage_t *netInvNotif = &packet->GetMsg()->choice.inviteNotifyMessage;
			netInvNotif->gameId = netInvite->gameId;
			netInvNotif->playerIdByWhom = session->GetPlayerData()->GetUniqueId();
			netInvNotif->playerIdWho = netInvite->playerId;

			bool requestSent = server->GetLobbyThread().SendToLobbyPlayer(netInvite->playerId, packet);
			server->SendToAllPlayers(packet, SessionData::Game);
			if (requestSent) {
				// This player has been invited.
				server->AddPlayerInvitation(netInvite->playerId);
			} else {
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
	} else if (packet->GetMsg()->present == PokerTHMessage_PR_resetTimeoutMessage) {
		if (session->GetPlayerData()->IsGameAdmin()) {
			RegisterAdminTimer(server);
		}
	} else {
		server->SessionError(session, ERR_SOCK_INVALID_PACKET);
	}
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
	server->GetStateTimer1().expires_from_now(
		boost::posix_time::seconds(SERVER_START_GAME_TIMEOUT_SEC));
	server->GetStateTimer1().async_wait(
		boost::bind(
			&ServerGameStateStartGame::TimerTimeout, this, boost::asio::placeholders::error, server));
}

void
ServerGameStateStartGame::Exit(boost::shared_ptr<ServerGame> server)
{
	server->GetStateTimer1().cancel();
}

void
ServerGameStateStartGame::HandleNewSession(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session)
{
	// Do not accept new sessions in this state.
	server->MoveSessionToLobby(session, NTF_NET_REMOVED_ALREADY_RUNNING);
}

void
ServerGameStateStartGame::InternalProcessPacket(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet)
{
	if (packet->GetMsg()->present == PokerTHMessage_PR_startEventAckMessage) {
		session->SetReadyFlag();
		if (server->GetSessionManager().CountReadySessions() == server->GetSessionManager().GetRawSessionCount()) {
			// Everyone is ready.
			server->GetSessionManager().ResetAllReadyFlags();
			DoStart(server);
		}
	}
}

void
ServerGameStateStartGame::TimerTimeout(const boost::system::error_code &ec, boost::shared_ptr<ServerGame> server)
{
	if (!ec && &server->GetState() == this) {
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
	PlayerDataList tmpPlayerList(server->InternalStartGame());
	if (tmpPlayerList.size() <= 1) {
		if (!tmpPlayerList.empty()) {
			boost::shared_ptr<PlayerData> tmpPlayer(tmpPlayerList.front());
			boost::shared_ptr<SessionData> tmpSession = server->GetSessionManager().GetSessionByUniquePlayerId(tmpPlayer->GetUniqueId());
			if (tmpSession)
				server->MoveSessionToLobby(tmpSession, NTF_NET_REMOVED_START_FAILED);
		}
	} else {
		boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
		packet->GetMsg()->present = PokerTHMessage_PR_gameStartMessage;
		GameStartMessage_t *netGameStart = &packet->GetMsg()->choice.gameStartMessage;
		netGameStart->gameId = server->GetId();
		netGameStart->startDealerPlayerId = server->GetStartData().startDealerPlayerId;
		netGameStart->gameStartMode.present = gameStartMode_PR_gameStartModeInitial;
		GameStartModeInitial_t *netStartModeInitial = &netGameStart->gameStartMode.choice.gameStartModeInitial;

		// Send player order to clients.
		// Assume player list is sorted by number.
		PlayerDataList::iterator player_i = tmpPlayerList.begin();
		PlayerDataList::iterator player_end = tmpPlayerList.end();
		while (player_i != player_end) {
			NonZeroId_t *playerSlot = (NonZeroId_t *)calloc(1, sizeof(NonZeroId_t));
			*playerSlot = (*player_i)->GetUniqueId();
			ASN_SEQUENCE_ADD(&netStartModeInitial->playerSeats.list, playerSlot);

			++player_i;
		}

		server->SendToAllPlayers(packet, SessionData::Game);

		// Start the first hand.
		ServerGameStateHand::StartNewHand(server);
		server->SetState(ServerGameStateHand::Instance());
	}
}

//-----------------------------------------------------------------------------

AbstractServerGameStateRunning::~AbstractServerGameStateRunning()
{
}

void
AbstractServerGameStateRunning::HandleNewSession(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session)
{

	// Verify that the user is allowed to rejoin.
	if (session && session->GetPlayerData()) {
		boost::shared_ptr<PlayerInterface> tmpPlayer = server->GetPlayerInterfaceFromGame(session->GetPlayerData()->GetName());
		if (tmpPlayer && tmpPlayer->getMyGuid() == session->GetPlayerData()->GetOldGuid()) {
			// The player wants to rejoin.
			AcceptNewSession(server, session);
			// Remember: We need to initiate a rejoin when starting the next hand.
			server->AddRejoinPlayer(session->GetPlayerData()->GetUniqueId());

			// Send start event right away.
			boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
			packet->GetMsg()->present = PokerTHMessage_PR_startEventMessage;
			StartEventMessage_t *netStartEvent = &packet->GetMsg()->choice.startEventMessage;
			netStartEvent->gameId = server->GetId();
			netStartEvent->startEventType.present = startEventType_PR_rejoinEvent;

			// Wait for rejoining player to confirm start of game.
			server->GetLobbyThread().GetSender().Send(session, packet);
		} else {
			// Do not accept "new" sessions in this state, only rejoin is allowed.
			server->MoveSessionToLobby(session, NTF_NET_REMOVED_ALREADY_RUNNING);
		}
	}
}

void
AbstractServerGameStateRunning::InternalProcessPacket(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet)
{
	if (packet->GetMsg()->present == PokerTHMessage_PR_resetTimeoutMessage) {
		// Reactivate session.
		server->AddReactivatePlayer(session->GetPlayerData()->GetUniqueId());
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
	server->GetStateTimer1().expires_from_now(
		boost::posix_time::milliseconds(SERVER_LOOP_DELAY_MSEC));
	server->GetStateTimer1().async_wait(
		boost::bind(
			&ServerGameStateHand::TimerLoop, this, boost::asio::placeholders::error, server));
}

void
ServerGameStateHand::Exit(boost::shared_ptr<ServerGame> server)
{
	server->GetStateTimer1().cancel();
}

void
ServerGameStateHand::InternalProcessPacket(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet)
{
	AbstractServerGameStateRunning::InternalProcessPacket(server, session, packet);
}

void
ServerGameStateHand::TimerLoop(const boost::system::error_code &ec, boost::shared_ptr<ServerGame> server)
{
	if (!ec && &server->GetState() == this) {
		try {
			EngineLoop(server);
		} catch (const PokerTHException &e) {
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
	if (newRound != curRound && newRound != GAME_STATE_POST_RIVER) {
		if (newRound <= curRound)
			throw ServerException(__FILE__, __LINE__, ERR_NET_INVALID_GAME_ROUND, 0);

		// Retrieve non-fold players. If only one player is left, no cards are shown.
		list<boost::shared_ptr<PlayerInterface> > nonFoldPlayers = *curGame.getActivePlayerList();
		nonFoldPlayers.remove_if(boost::bind(&PlayerInterface::getMyAction, _1) == PLAYER_ACTION_FOLD);

		if (curGame.getCurrentHand()->getAllInCondition()
				&& !curGame.getCurrentHand()->getCardsShown()
				&& nonFoldPlayers.size() > 1) {
			// Send cards of all active players to all players (all in).
			boost::shared_ptr<NetPacket> allIn(new NetPacket(NetPacket::Alloc));
			allIn->GetMsg()->present = PokerTHMessage_PR_allInShowCardsMessage;
			AllInShowCardsMessage_t *netAllInShow = &allIn->GetMsg()->choice.allInShowCardsMessage;
			netAllInShow->gameId = server->GetId();

			PlayerListConstIterator i = nonFoldPlayers.begin();
			PlayerListConstIterator end = nonFoldPlayers.end();

			while (i != end) {
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

			server->GetStateTimer1().expires_from_now(
				boost::posix_time::seconds(SERVER_SHOW_CARDS_DELAY_SEC));
			server->GetStateTimer1().async_wait(
				boost::bind(
					&ServerGameStateHand::TimerShowCards, this, boost::asio::placeholders::error, server));
		} else {
			SendNewRoundCards(*server, curGame, newRound);

			server->GetStateTimer1().expires_from_now(
				boost::posix_time::seconds(GetDealCardsDelaySec(*server)));
			server->GetStateTimer1().async_wait(
				boost::bind(
					&ServerGameStateHand::TimerLoop, this, boost::asio::placeholders::error, server));
		}
	} else {
		if (newRound != GAME_STATE_POST_RIVER) { // continue hand
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
			if (curPlayer->getMyType() == PLAYER_TYPE_COMPUTER) {
				server->GetStateTimer1().expires_from_now(
					boost::posix_time::seconds(SERVER_COMPUTER_ACTION_DELAY_SEC));
				server->GetStateTimer1().async_wait(
					boost::bind(
						&ServerGameStateHand::TimerComputerAction, this, boost::asio::placeholders::error, server));
			} else {
				// If the player we are waiting for left, continue without him.
				if (!server->GetSessionManager().IsPlayerConnected(curPlayer->getMyUniqueID())
						|| !curPlayer->isSessionActive()) {
					PerformPlayerAction(*server, curPlayer, PLAYER_ACTION_FOLD, 0);

					server->GetStateTimer1().expires_from_now(
						boost::posix_time::milliseconds(SERVER_LOOP_DELAY_MSEC));
					server->GetStateTimer1().async_wait(
						boost::bind(
							&ServerGameStateHand::TimerLoop, this, boost::asio::placeholders::error, server));
				} else {
					server->SetState(ServerGameStateWaitPlayerAction::Instance());
				}
			}
		} else { // hand is over
			// Engine will find out who won.
			curGame.getCurrentHand()->getCurrentBeRo()->postRiverRun();

			// Retrieve non-fold players. If only one player is left, no cards are shown.
			list<boost::shared_ptr<PlayerInterface> > nonFoldPlayers = *curGame.getActivePlayerList();
			nonFoldPlayers.remove_if(boost::bind(&PlayerInterface::getMyAction, _1) == PLAYER_ACTION_FOLD);

			if (nonFoldPlayers.size() == 1) {
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
			} else {
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

				while (i != end) {
					boost::shared_ptr<PlayerInterface> tmpPlayer(curGame.getPlayerByUniqueId(*i));
					if (tmpPlayer) {
						PlayerResult_t *playerResult = (PlayerResult_t *)calloc(1, sizeof(PlayerResult_t));
						SetPlayerResult(*playerResult, tmpPlayer, GAME_STATE_RIVER);

						ASN_SEQUENCE_ADD(&endHandShow->playerResults.list, playerResult);
					}
					++i;
				}
				server->SendToAllPlayers(endHand, SessionData::Game);
			}

			// Remove disconnected players. This is the one and only place to do this.
			server->RemoveDisconnectedPlayers();

			// Update rankings of all remaining players
			server->UpdateRankingMap();

			// Start next hand - if enough players are left.
			list<boost::shared_ptr<PlayerInterface> > playersWithCash = *curGame.getActivePlayerList();
			playersWithCash.remove_if(boost::bind(&PlayerInterface::getMyCash, _1) < 1);

			if (playersWithCash.empty()) {
				// No more players left - restart.
				server->SetState(SERVER_INITIAL_STATE::Instance());
			} else if (playersWithCash.size() == 1) {
				boost::shared_ptr<PlayerInterface> winnerPlayer = *(playersWithCash.begin());
				server->InternalEndGame();

				// View a dialog for a new game - delayed.
				server->GetStateTimer1().expires_from_now(
					boost::posix_time::seconds(SERVER_DELAY_NEXT_GAME_SEC));
				server->GetStateTimer1().async_wait(
					boost::bind(
						&ServerGameStateHand::TimerNextGame, this, boost::asio::placeholders::error, server, winnerPlayer->getMyUniqueID()));
			} else {
				server->SetState(ServerGameStateWaitNextHand::Instance());
			}
		}
	}
}

void
ServerGameStateHand::TimerShowCards(const boost::system::error_code &ec, boost::shared_ptr<ServerGame> server)
{
	if (!ec && &server->GetState() == this) {
		Game &curGame = server->GetGame();
		SendNewRoundCards(*server, curGame, curGame.getCurrentHand()->getCurrentRound());

		server->GetStateTimer1().expires_from_now(
			boost::posix_time::seconds(GetDealCardsDelaySec(*server)));
		server->GetStateTimer1().async_wait(
			boost::bind(
				&ServerGameStateHand::TimerLoop, this, boost::asio::placeholders::error, server));
	}
}

void
ServerGameStateHand::TimerComputerAction(const boost::system::error_code &ec, boost::shared_ptr<ServerGame> server)
{
	if (!ec && &server->GetState() == this) {
		try {
			boost::shared_ptr<PlayerInterface> curPlayer = server->GetGame().getCurrentPlayer();
			if (!curPlayer)
				throw ServerException(__FILE__, __LINE__, ERR_NET_NO_CURRENT_PLAYER, 0);

			curPlayer->action();
			SendPlayerAction(*server, curPlayer);
			EngineLoop(server);
		} catch (const PokerTHException &e) {
			LOG_ERROR("Game " << server->GetId() << " - Computer timer exception: " << e.what());
			server->RemoveAllSessions(); // Close this game on error.
		}
	}
}

void
ServerGameStateHand::TimerNextHand(const boost::system::error_code &ec, boost::shared_ptr<ServerGame> server)
{
	if (!ec && &server->GetState() == this) {
		StartNewHand(server);
		TimerLoop(ec, server);
	}
}

void
ServerGameStateHand::TimerNextGame(const boost::system::error_code &ec, boost::shared_ptr<ServerGame> server, unsigned winnerPlayerId)
{
	if (!ec && &server->GetState() == this) {
		boost::shared_ptr<NetPacket> endGame(new NetPacket(NetPacket::Alloc));
		endGame->GetMsg()->present = PokerTHMessage_PR_endOfGameMessage;
		EndOfGameMessage_t *netEndGame = &endGame->GetMsg()->choice.endOfGameMessage;
		netEndGame->gameId = server->GetId();
		netEndGame->winnerPlayerId = winnerPlayerId;
		server->SendToAllPlayers(endGame, SessionData::Game);

		// Wait for the start of a new game.
		server->RemoveAutoLeavePlayers();
		server->ResetComputerPlayerList();
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

	switch(curGame.getCurrentHand()->getCurrentRound()) {
	case GAME_STATE_FLOP:
		delay = SERVER_DEAL_FLOP_CARDS_DELAY_SEC;
		break;
	case GAME_STATE_TURN:
		delay = SERVER_DEAL_TURN_CARD_DELAY_SEC + allInDelay;
		break;
	case GAME_STATE_RIVER:
		delay = SERVER_DEAL_RIVER_CARD_DELAY_SEC;
		break;
	default:
		break;
	}
	return delay;
}

void
ServerGameStateHand::StartNewHand(boost::shared_ptr<ServerGame> server)
{
	Game &curGame = server->GetGame();

	// Reactivate players which were previously inactive.
	ReactivatePlayers(server);

	// Initialize rejoining players.
	// This has to be done before initialising the new hand, because there are side effects.
	InitRejoiningPlayers(server);

	// Kick inactive players.
	CheckPlayerTimeouts(server);

	// Initialize hand.
	curGame.initHand();

	// HACK: Skip GUI notification run
	curGame.getCurrentHand()->getFlop()->skipFirstRunGui();
	curGame.getCurrentHand()->getTurn()->skipFirstRunGui();
	curGame.getCurrentHand()->getRiver()->skipFirstRunGui();

	// Consider all players, even inactive.
	PlayerListIterator i = curGame.getSeatsList()->begin();
	PlayerListIterator end = curGame.getSeatsList()->end();

	// Send cards to all players.
	while (i != end) {
		// Also send to inactive players.
		boost::shared_ptr<PlayerInterface> tmpPlayer = *i;
		boost::shared_ptr<SessionData> tmpSession = server->GetSessionManager().GetSessionByUniquePlayerId(tmpPlayer->getMyUniqueID());
		if (tmpSession) {
			int cards[2];
			bool errorFlag = false;
			tmpPlayer->getMyCards(cards);

			boost::shared_ptr<NetPacket> notifyCards(new NetPacket(NetPacket::Alloc));
			notifyCards->GetMsg()->present = PokerTHMessage_PR_handStartMessage;
			HandStartMessage_t *netHandStart = &notifyCards->GetMsg()->choice.handStartMessage;
			netHandStart->gameId = server->GetId();
			string tmpPassword(tmpSession->AuthGetPassword());
			if (tmpPassword.empty()) { // encrypt only if password is present
				netHandStart->yourCards.present = yourCards_PR_plainCards;
				PlainCards_t *plainCards = &netHandStart->yourCards.choice.plainCards;
				plainCards->plainCard1 = cards[0];
				plainCards->plainCard2 = cards[1];
			} else {
				ostringstream cardDataStream;
				vector<unsigned char> tmpCipher;
				cardDataStream
						<< tmpPlayer->getMyUniqueID() << " "
						<< server->GetId() << " "
						<< curGame.getCurrentHandID() << " "
						<< cards[0] << " "
						<< cards[1];
				if (CryptHelper::AES128Encrypt((const unsigned char *)tmpPassword.c_str(),
											   (unsigned)tmpPassword.size(),
											   cardDataStream.str(),
											   tmpCipher)
						&& !tmpCipher.empty()) {
					netHandStart->yourCards.present = yourCards_PR_encryptedCards;
					EncryptedCards_t *encryptedCards = &netHandStart->yourCards.choice.encryptedCards;
					OCTET_STRING_fromBuf(
						&encryptedCards->cardData,
						(const char *)&tmpCipher[0],
						(int)tmpCipher.size());
				} else {
					server->RemovePlayer(tmpPlayer->getMyUniqueID(), ERR_SOCK_INVALID_STATE);
					errorFlag = true;
				}
			}
			PlayerListIterator player_i = curGame.getSeatsList()->begin();
			PlayerListIterator player_end = curGame.getSeatsList()->end();
			int playerCounter = 0;
			while (player_i != player_end && playerCounter < server->GetStartData().numberOfPlayers) {
				NetPlayerState_t *seatState = (NetPlayerState_t *)calloc(1, sizeof(NetPlayerState_t));
				if (!(*player_i)->getMyActiveStatus()) {
					*seatState = NetPlayerState_playerStateNoMoney;
				} else if (!(*player_i)->isSessionActive()) {
					*seatState = NetPlayerState_playerStateSessionInactive;
				} else {
					*seatState = NetPlayerState_playerStateNormal;
				}
				ASN_SEQUENCE_ADD(&netHandStart->seatStates.list, seatState);
				++player_i;
				++playerCounter;
			}

			if (!errorFlag) {
				netHandStart->smallBlind = curGame.getCurrentHand()->getSmallBlind();
				server->GetLobbyThread().GetSender().Send(tmpSession, notifyCards);
			}
		}
		++i;
	}

	// Start hand.
	curGame.startHand();

	// Auto small blind / big blind at the beginning of hand.
	i = curGame.getActivePlayerList()->begin();
	end = curGame.getActivePlayerList()->end();

	while (i != end) {
		boost::shared_ptr<PlayerInterface> tmpPlayer = *i;
		if (tmpPlayer->getMyButton() == BUTTON_SMALL_BLIND) {
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
	while (i != end) {
		boost::shared_ptr<PlayerInterface> tmpPlayer = *i;
		if (tmpPlayer->getMyButton() == BUTTON_BIG_BLIND) {
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

void
ServerGameStateHand::CheckPlayerTimeouts(boost::shared_ptr<ServerGame> server)
{
	// Check timeout.
	int actionTimeout = server->GetGameData().playerActionTimeoutSec;
	if (actionTimeout) {
		// Consider all active players.
		PlayerListIterator i = server->GetGame().getActivePlayerList()->begin();
		PlayerListIterator end = server->GetGame().getActivePlayerList()->end();

		// Check timeouts of players.
		while (i != end) {
			boost::shared_ptr<PlayerInterface> tmpPlayer = *i;
			if (tmpPlayer->getMyType() == PLAYER_TYPE_HUMAN
					&& (int)tmpPlayer->getTimeSecSinceLastRemoteAction() >= actionTimeout * SERVER_GAME_AUTOFOLD_TIMEOUT_FACTOR) {
				if (tmpPlayer->isSessionActive()) {
					tmpPlayer->setIsSessionActive(false);
					boost::shared_ptr<SessionData> session = server->GetSessionManager().GetSessionByUniquePlayerId(tmpPlayer->getMyUniqueID());
					if (session) {
						boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
						packet->GetMsg()->present = PokerTHMessage_PR_timeoutWarningMessage;
						TimeoutWarningMessage_t *netWarning = &packet->GetMsg()->choice.timeoutWarningMessage;
						netWarning->timeoutReason = NETWORK_TIMEOUT_KICK_AFTER_AUTOFOLD;
						netWarning->remainingSeconds = actionTimeout * SERVER_GAME_FORCED_TIMEOUT_FACTOR - tmpPlayer->getTimeSecSinceLastRemoteAction();
						server->GetLobbyThread().GetSender().Send(session, packet);
					}
				}
				if ((int)tmpPlayer->getTimeSecSinceLastRemoteAction() >= actionTimeout * SERVER_GAME_FORCED_TIMEOUT_FACTOR) {
					server->KickPlayer(tmpPlayer->getMyUniqueID());
				}
			}
			++i;
		}
	}
}

void
ServerGameStateHand::ReactivatePlayers(boost::shared_ptr<ServerGame> server)
{
	PlayerIdList reactivateIdList(server->GetAndResetReactivatePlayers());
	PlayerIdList::iterator i = reactivateIdList.begin();
	PlayerIdList::iterator end = reactivateIdList.end();
	while (i != end) {
		boost::shared_ptr<PlayerInterface> tmpPlayer(server->GetGame().getPlayerByUniqueId(*i));
		if (tmpPlayer) {
			tmpPlayer->markRemoteAction();
			tmpPlayer->setIsSessionActive(true);
		}
		++i;
	}
}

void
ServerGameStateHand::InitRejoiningPlayers(boost::shared_ptr<ServerGame> server)
{
	PlayerIdList rejoinIdList(server->GetAndResetRejoinPlayers());
	PlayerIdList::iterator i = rejoinIdList.begin();
	PlayerIdList::iterator end = rejoinIdList.end();
	while (i != end) {
		boost::shared_ptr<SessionData> session(server->GetSessionManager().GetSessionByUniquePlayerId(*i));
		if (session && session->GetPlayerData()) {
			PerformRejoin(server, session);
		}
		++i;
	}
}

void
ServerGameStateHand::PerformRejoin(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session)
{
	Game &curGame = server->GetGame();
	// Set new player id.
	boost::shared_ptr<PlayerInterface> rejoinPlayer = curGame.getPlayerByName(session->GetPlayerData()->GetName());
	if (rejoinPlayer) {
		// Notify other clients about id change.
		boost::shared_ptr<NetPacket> packet(new NetPacket(NetPacket::Alloc));
		packet->GetMsg()->present = PokerTHMessage_PR_playerIdChangedMessage;
		PlayerIdChangedMessage_t *netIdChanged = &packet->GetMsg()->choice.playerIdChangedMessage;
		netIdChanged->oldPlayerId = rejoinPlayer->getMyUniqueID();
		netIdChanged->newPlayerId = session->GetPlayerData()->GetUniqueId();
		server->SendToAllButOnePlayers(packet, session->GetId(), SessionData::Game);

		// Update the dealer, if necessary.
		curGame.replaceDealer(rejoinPlayer->getMyUniqueID(), session->GetPlayerData()->GetUniqueId());
		// Update the ranking map.
		server->ReplaceRankingPlayer(rejoinPlayer->getMyUniqueID(), session->GetPlayerData()->GetUniqueId());
		// Change the Id in the poker engine.
		rejoinPlayer->setMyUniqueID(session->GetPlayerData()->GetUniqueId());
		rejoinPlayer->setMyGuid(session->GetPlayerData()->GetGuid());
		rejoinPlayer->markRemoteAction();
		rejoinPlayer->setIsSessionActive(true);

		// Send game start notification to rejoining client.
		packet.reset(new NetPacket(NetPacket::Alloc));
		packet->GetMsg()->present = PokerTHMessage_PR_gameStartMessage;
		GameStartMessage_t *netGameStart = &packet->GetMsg()->choice.gameStartMessage;
		netGameStart->gameId = server->GetId();
		netGameStart->startDealerPlayerId = curGame.getDealerPosition();
		netGameStart->gameStartMode.present = gameStartMode_PR_gameStartModeRejoin;
		GameStartModeRejoin_t *netStartModeRejoin = &netGameStart->gameStartMode.choice.gameStartModeRejoin;

		netStartModeRejoin->handNum = curGame.getCurrentHandID();
		PlayerListIterator player_i = curGame.getSeatsList()->begin();
		PlayerListIterator player_end = curGame.getSeatsList()->end();
		int player_count = 0;
		while (player_i != player_end && player_count < server->GetStartData().numberOfPlayers) {
			boost::shared_ptr<PlayerInterface> tmpPlayer = *player_i;
			RejoinPlayerData_t *playerSlot = (RejoinPlayerData_t *)calloc(1, sizeof(RejoinPlayerData_t));
			playerSlot->playerId = tmpPlayer->getMyUniqueID();
			playerSlot->playerMoney = tmpPlayer->getMyCash();
			ASN_SEQUENCE_ADD(&netStartModeRejoin->rejoinPlayerData.list, playerSlot);
			++player_i;
			++player_count;
		}

		server->GetLobbyThread().GetSender().Send(session, packet);
	} else {
		server->SessionError(session, ERR_SOCK_INVALID_STATE);
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
	if (server->GetGameData().playerActionTimeoutSec > 0) { // zero means unlimited thinking time
#ifdef POKERTH_SERVER_TEST
		int timeoutSec = SERVER_PLAYER_TIMEOUT_ADD_DELAY_SEC;
#else
		int timeoutSec = server->GetGameData().playerActionTimeoutSec + SERVER_PLAYER_TIMEOUT_ADD_DELAY_SEC;
#endif

		server->GetStateTimer1().expires_from_now(boost::posix_time::seconds(timeoutSec));
		server->GetStateTimer1().async_wait(
			boost::bind(
				&ServerGameStateWaitPlayerAction::TimerTimeout, this, boost::asio::placeholders::error, server));
	}
}

void
ServerGameStateWaitPlayerAction::Exit(boost::shared_ptr<ServerGame> server)
{
	server->GetStateTimer1().cancel();
}

void
ServerGameStateWaitPlayerAction::InternalProcessPacket(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet)
{
	AbstractServerGameStateRunning::InternalProcessPacket(server, session, packet);

	if (packet->GetMsg()->present == PokerTHMessage_PR_myActionRequestMessage) {
		MyActionRequestMessage_t *netMyAction = &packet->GetMsg()->choice.myActionRequestMessage;

		// TODO consider game id.
		Game &curGame = server->GetGame();
		boost::shared_ptr<PlayerInterface> tmpPlayer = curGame.getPlayerByUniqueId(session->GetPlayerData()->GetUniqueId());
		if (!tmpPlayer)
			throw ServerException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);

		// Check whether this is the correct round.
		PlayerActionCode code = ACTION_CODE_VALID;
		if (curGame.getCurrentHand()->getCurrentRound() != netMyAction->gameState)
			code = ACTION_CODE_INVALID_STATE;

		// Check whether this is the correct player.
		boost::shared_ptr<PlayerInterface> curPlayer = server->GetGame().getCurrentPlayer();
		if (code == ACTION_CODE_VALID
				&& (curPlayer->getMyUniqueID() != tmpPlayer->getMyUniqueID())) {
			code = ACTION_CODE_NOT_YOUR_TURN;
		}

		// If the client omitted some values, fill them in.
		if (netMyAction->myAction == PLAYER_ACTION_CALL && netMyAction->myRelativeBet == 0) {
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
						curGame.getCurrentHand()->getSmallBlind()) != 0)) {
			code = ACTION_CODE_NOT_ALLOWED;
		}

		if (code == ACTION_CODE_VALID) {
			tmpPlayer->setIsSessionActive(true);
			tmpPlayer->markRemoteAction();
			PerformPlayerAction(*server, tmpPlayer, static_cast<PlayerAction>(netMyAction->myAction), netMyAction->myRelativeBet);
			server->SetState(ServerGameStateHand::Instance());
		} else {
			// Send reject message.
			boost::shared_ptr<NetPacket> reject(new NetPacket(NetPacket::Alloc));
			reject->GetMsg()->present = PokerTHMessage_PR_yourActionRejectedMessage;
			YourActionRejectedMessage_t *netActionRejected = &reject->GetMsg()->choice.yourActionRejectedMessage;
			netActionRejected->gameId = server->GetId();
			netActionRejected->gameState = netMyAction->gameState;
			netActionRejected->yourAction = netMyAction->myAction;
			netActionRejected->yourRelativeBet = netMyAction->myRelativeBet;
			netActionRejected->rejectionReason = code;
			server->GetLobbyThread().GetSender().Send(session, reject);
		}
	}
}

void
ServerGameStateWaitPlayerAction::TimerTimeout(const boost::system::error_code &ec, boost::shared_ptr<ServerGame> server)
{
	if (!ec && &server->GetState() == this) {
		try {
			Game &curGame = server->GetGame();
			// Retrieve current player.
			boost::shared_ptr<PlayerInterface> curPlayer = curGame.getCurrentPlayer();
			if (!curPlayer)
				throw ServerException(__FILE__, __LINE__, ERR_NET_NO_CURRENT_PLAYER, 0);

			// Player did not act fast enough. Act for him.
			if (curGame.getCurrentHand()->getCurrentBeRo()->getHighestSet() == curPlayer->getMySet())
				PerformPlayerAction(*server, curPlayer, PLAYER_ACTION_CHECK, 0);
			else
				PerformPlayerAction(*server, curPlayer, PLAYER_ACTION_FOLD, 0);

			server->SetState(ServerGameStateHand::Instance());
		} catch (const PokerTHException &e) {
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
#ifdef POKERTH_SERVER_TEST
	int timeoutSec = 0;
#else
	int timeoutSec = server->GetGameData().delayBetweenHandsSec;
#endif

	server->GetStateTimer1().expires_from_now(
		boost::posix_time::seconds(timeoutSec));

	server->GetStateTimer1().async_wait(
		boost::bind(
			&ServerGameStateWaitNextHand::TimerTimeout, this, boost::asio::placeholders::error, server));
}

void
ServerGameStateWaitNextHand::Exit(boost::shared_ptr<ServerGame> server)
{
	server->GetStateTimer1().cancel();
}

void
ServerGameStateWaitNextHand::InternalProcessPacket(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet)
{
	AbstractServerGameStateRunning::InternalProcessPacket(server, session, packet);

	if (packet->GetMsg()->present == PokerTHMessage_PR_showMyCardsRequestMessage) {
		Game &curGame = server->GetGame();
		boost::shared_ptr<NetPacket> show(new NetPacket(NetPacket::Alloc));
		show->GetMsg()->present = PokerTHMessage_PR_afterHandShowCardsMessage;

		AfterHandShowCardsMessage_t *netShowCards = &show->GetMsg()->choice.afterHandShowCardsMessage;
		boost::shared_ptr<PlayerInterface> tmpPlayer(curGame.getPlayerByUniqueId(session->GetPlayerData()->GetUniqueId()));
		if (tmpPlayer) {
			SetPlayerResult(netShowCards->playerResult, tmpPlayer, curGame.getCurrentHand()->getRoundBeforePostRiver());
			server->SendToAllPlayers(show, SessionData::Game);
		}
	}
}

void
ServerGameStateWaitNextHand::TimerTimeout(const boost::system::error_code &ec, boost::shared_ptr<ServerGame> server)
{
	if (!ec && &server->GetState() == this) {
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

