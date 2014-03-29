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

#ifdef BOOST_ASIO_HAS_STD_CHRONO
using namespace std::chrono;
#else
using namespace boost::chrono;
#endif

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
#define SERVER_MAX_NUM_SPECTATORS_PER_GAME			100

#define GAME_MAX_NUM_JOINS_PER_PLAYER				6

// Helper functions

static void SendPlayerAction(ServerGame &server, boost::shared_ptr<PlayerInterface> player)
{
	if (!player.get())
		throw ServerException(__FILE__, __LINE__, ERR_NET_NO_CURRENT_PLAYER, 0);

	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_PlayersActionDoneMessage);
	PlayersActionDoneMessage *netActionDone = packet->GetMsg()->mutable_playersactiondonemessage();

	netActionDone->set_gameid(server.GetId());
	netActionDone->set_gamestate(static_cast<NetGameState>(server.GetCurRound()));
	netActionDone->set_highestset(server.GetGame().getCurrentHand()->getCurrentBeRo()->getHighestSet());
	netActionDone->set_minimumraise(server.GetGame().getCurrentHand()->getCurrentBeRo()->getMinimumRaise());
	netActionDone->set_playeraction(static_cast<NetPlayerAction>(player->getMyAction()));
	netActionDone->set_playerid(player->getMyUniqueID());
	netActionDone->set_playermoney(player->getMyCash());
	netActionDone->set_totalplayerbet(player->getMySet());
	server.SendToAllPlayers(packet, SessionData::Game | SessionData::Spectating);
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
		boost::shared_ptr<NetPacket> packet(new NetPacket);
		packet->GetMsg()->set_messagetype(PokerTHMessage::Type_DealFlopCardsMessage);
		DealFlopCardsMessage *netDealFlop = packet->GetMsg()->mutable_dealflopcardsmessage();
		netDealFlop->set_gameid(server.GetId());
		netDealFlop->set_flopcard1(cards[0]);
		netDealFlop->set_flopcard2(cards[1]);
		netDealFlop->set_flopcard3(cards[2]);
		server.SendToAllPlayers(packet, SessionData::Game | SessionData::Spectating);
	}
	break;
	case GAME_STATE_TURN: {
		// deal turn card
		boost::shared_ptr<NetPacket> packet(new NetPacket);
		packet->GetMsg()->set_messagetype(PokerTHMessage::Type_DealTurnCardMessage);
		DealTurnCardMessage *netDealTurn = packet->GetMsg()->mutable_dealturncardmessage();
		netDealTurn->set_gameid(server.GetId());
		netDealTurn->set_turncard(cards[3]);
		server.SendToAllPlayers(packet, SessionData::Game | SessionData::Spectating);
	}
	break;
	case GAME_STATE_RIVER: {
		// deal river card
		boost::shared_ptr<NetPacket> packet(new NetPacket);
		packet->GetMsg()->set_messagetype(PokerTHMessage::Type_DealRiverCardMessage);
		DealRiverCardMessage *netDealRiver = packet->GetMsg()->mutable_dealrivercardmessage();
		netDealRiver->set_gameid(server.GetId());
		netDealRiver->set_rivercard(cards[4]);
		server.SendToAllPlayers(packet, SessionData::Game | SessionData::Spectating);
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
SetPlayerResult(PlayerResult &playerResult, boost::shared_ptr<PlayerInterface> tmpPlayer, GameState roundBeforePostRiver)
{
	playerResult.set_playerid(tmpPlayer->getMyUniqueID());
	int tmpCards[2];
	int bestHandPos[5];
	tmpPlayer->getMyCards(tmpCards);
	playerResult.set_resultcard1(tmpCards[0]);
	playerResult.set_resultcard2(tmpCards[1]);
	tmpPlayer->getMyBestHandPosition(bestHandPos);
	for (int num = 0; num < 5; num++) {
		playerResult.add_besthandposition(bestHandPos[num]);
	}

	if (roundBeforePostRiver == GAME_STATE_RIVER) {
		playerResult.set_cardsvalue(tmpPlayer->getMyCardsValueInt());
	}
	playerResult.set_moneywon(tmpPlayer->getLastMoneyWon());
	playerResult.set_playermoney(tmpPlayer->getMyCash());
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
	if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_PlayerInfoRequestMessage) {
		// Delegate to Lobby.
		server->GetLobbyThread().HandleGameRetrievePlayerInfo(session, packet->GetMsg()->playerinforequestmessage());
	} else if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_AvatarRequestMessage) {
		// Delegate to Lobby.
		server->GetLobbyThread().HandleGameRetrieveAvatar(session, packet->GetMsg()->avatarrequestmessage());
	} else if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_LeaveGameRequestMessage) {
		server->MoveSessionToLobby(session, NTF_NET_REMOVED_ON_REQUEST);
	} else if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_KickPlayerRequestMessage) {
		// Only admins are allowed to kick, and only in the lobby.
		// After leaving the lobby, a vote needs to be initiated to kick.
		const KickPlayerRequestMessage &netKickRequest = packet->GetMsg()->kickplayerrequestmessage();
		if (session->GetPlayerData()->IsGameAdmin() && !server->IsRunning()
				&& netKickRequest.gameid() == server->GetId() && server->GetGameData().gameType != GAME_TYPE_RANKING) {
			server->KickPlayer(netKickRequest.playerid());
		}
	} else if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_AskKickPlayerMessage) {
		if (server->GetGameData().gameType != GAME_TYPE_RANKING) {
			const AskKickPlayerMessage &netAskKick = packet->GetMsg()->askkickplayermessage();
			server->InternalAskVoteKick(session, netAskKick.playerid(), SERVER_VOTE_KICK_TIMEOUT_SEC);
		}
	} else if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_VoteKickRequestMessage) {
		const VoteKickRequestMessage &netVoteKick = packet->GetMsg()->votekickrequestmessage();
		server->InternalVoteKick(session, netVoteKick.petitionid(), netVoteKick.votekick() ? KICK_VOTE_IN_FAVOUR : KICK_VOTE_AGAINST);
	}
	// Chat text is always allowed.
	else if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_ChatRequestMessage) {
		bool chatSent = false;
		const ChatRequestMessage &netChatRequest = packet->GetMsg()->chatrequestmessage();
		// Only forward if this player is known and not a guest.
		if (session->GetPlayerData()->GetRights() != PLAYER_RIGHTS_GUEST) {
			// Forward chat text to all players.
			// TODO: Some limitation needed.
			if (!netChatRequest.has_targetgameid()) {
				if (!server->IsRunning()) {
					server->GetLobbyThread().HandleChatRequest(session, netChatRequest);
					chatSent = true;
				}
			} else {
				boost::shared_ptr<PlayerInterface> tmpPlayer (server->GetPlayerInterfaceFromGame(session->GetPlayerData()->GetUniqueId()));
				// If we did not find the player, then the game did not start yet. Allow chat for now.
				// Otherwise, check whether the player is muted.
				if (!tmpPlayer || !tmpPlayer->isMuted()) {
					boost::shared_ptr<NetPacket> packet(new NetPacket);
					packet->GetMsg()->set_messagetype(PokerTHMessage::Type_ChatMessage);
					ChatMessage *netChat = packet->GetMsg()->mutable_chatmessage();
					netChat->set_gameid(server->GetId());
					netChat->set_playerid(session->GetPlayerData()->GetUniqueId());
					netChat->set_chattype(ChatMessage::chatTypeGame);
					netChat->set_chattext(netChatRequest.chattext());
					server->SendToAllPlayers(packet, SessionData::Game | SessionData::Spectating | SessionData::SpectatorWaiting);
					chatSent = true;

					// Send the message to the chat cleaner bot for ranking games.
					//if (server->GetGameData().gameType == GAME_TYPE_RANKING)
					//{
					server->GetLobbyThread().GetChatCleaner().HandleGameChatText(
						server->GetId(),
						session->GetPlayerData()->GetUniqueId(),
						session->GetPlayerData()->GetName(),
						netChatRequest.chattext());
					//}
				}
			}
		}
		// Reject chat otherwise.
		if (!chatSent) {
			boost::shared_ptr<NetPacket> packet(new NetPacket);
			packet->GetMsg()->set_messagetype(PokerTHMessage::Type_ChatRejectMessage);
			ChatRejectMessage *netReject = packet->GetMsg()->mutable_chatrejectmessage();
			netReject->set_chattext(netChatRequest.chattext());
			server->GetLobbyThread().GetSender().Send(session, packet);
		}
	} else if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_SubscriptionRequestMessage) {
		const SubscriptionRequestMessage &netSubscription = packet->GetMsg()->subscriptionrequestmessage();
		if (netSubscription.subscriptionaction() == SubscriptionRequestMessage::resubscribeGameList) {
			if (!session->WantsLobbyMsg())
				server->GetLobbyThread().ResubscribeLobbyMsg(session);
		} else
			session->ResetWantsLobbyMsg();
	} else if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_ReportAvatarMessage) {
		const ReportAvatarMessage &netReport = packet->GetMsg()->reportavatarmessage();
		boost::shared_ptr<PlayerData> tmpPlayer = server->GetPlayerDataByUniqueId(netReport.reportedplayerid());
		MD5Buf tmpMD5;
		memcpy(tmpMD5.GetData(), netReport.reportedavatarhash().data(), MD5_DATA_SIZE);
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
				boost::shared_ptr<NetPacket> packet(new NetPacket);
				packet->GetMsg()->set_messagetype(PokerTHMessage::Type_ReportAvatarAckMessage);
				ReportAvatarAckMessage *netReportAck = packet->GetMsg()->mutable_reportavatarackmessage();
				netReportAck->set_reportedplayerid(netReport.reportedplayerid());
				netReportAck->set_reportavatarresult(ReportAvatarAckMessage::avatarReportDuplicate);
				server->GetLobbyThread().GetSender().Send(session, packet);
			}
		} else {
			boost::shared_ptr<NetPacket> packet(new NetPacket);
			packet->GetMsg()->set_messagetype(PokerTHMessage::Type_ReportAvatarAckMessage);
			ReportAvatarAckMessage *netReportAck = packet->GetMsg()->mutable_reportavatarackmessage();
			netReportAck->set_reportedplayerid(netReport.reportedplayerid());
			netReportAck->set_reportavatarresult(ReportAvatarAckMessage::avatarReportInvalid);
			server->GetLobbyThread().GetSender().Send(session, packet);
		}
	} else if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_ReportGameMessage) {
		// Delegate to Lobby.
		server->GetLobbyThread().HandleGameReportGame(session, packet->GetMsg()->reportgamemessage());
	} else if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_JoinExistingGameMessage) {
		// Ignore join game requests in this state (may be multiple clicks from the lobby).
	} else {
		// Packet processing in subclass.
		InternalProcessPacket(server, session, packet);
	}
}

boost::shared_ptr<NetPacket>
AbstractServerGameStateReceiving::CreateNetPacketPlayerJoined(unsigned gameId, const PlayerData &playerData)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_GamePlayerJoinedMessage);
	GamePlayerJoinedMessage *netGamePlayer = packet->GetMsg()->mutable_gameplayerjoinedmessage();
	netGamePlayer->set_gameid(gameId);
	netGamePlayer->set_playerid(playerData.GetUniqueId());
	netGamePlayer->set_isgameadmin(playerData.IsGameAdmin());
	return packet;
}

boost::shared_ptr<NetPacket>
AbstractServerGameStateReceiving::CreateNetPacketSpectatorJoined(unsigned gameId, const PlayerData &playerData)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_GameSpectatorJoinedMessage);
	GameSpectatorJoinedMessage *netGameSpectator = packet->GetMsg()->mutable_gamespectatorjoinedmessage();
	netGameSpectator->set_gameid(gameId);
	netGameSpectator->set_playerid(playerData.GetUniqueId());
	return packet;
}

boost::shared_ptr<NetPacket>
AbstractServerGameStateReceiving::CreateNetPacketJoinGameAck(const ServerGame &server, const PlayerData &playerData, bool spectateOnly)
{
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_JoinGameAckMessage);
	JoinGameAckMessage *netJoinReply = packet->GetMsg()->mutable_joingameackmessage();
	netJoinReply->set_gameid(server.GetId());
	netJoinReply->set_areyougameadmin(playerData.IsGameAdmin());
	netJoinReply->set_spectateonly(spectateOnly);

	NetGameInfo *gameInfo = netJoinReply->mutable_gameinfo();
	NetPacket::SetGameData(server.GetGameData(), *gameInfo);
	gameInfo->set_gamename(server.GetName());
	return packet;
}

boost::shared_ptr<NetPacket>
AbstractServerGameStateReceiving::CreateNetPacketHandStart(const ServerGame &server)
{
	const Game &curGame = server.GetGame();

	boost::shared_ptr<NetPacket> notifyCards(new NetPacket);
	notifyCards->GetMsg()->set_messagetype(PokerTHMessage::Type_HandStartMessage);
	HandStartMessage *netHandStart = notifyCards->GetMsg()->mutable_handstartmessage();
	netHandStart->set_gameid(server.GetId());

	PlayerListIterator player_i = curGame.getSeatsList()->begin();
	PlayerListIterator player_end = curGame.getSeatsList()->end();
	int playerCounter = 0;
	while (player_i != player_end && playerCounter < server.GetStartData().numberOfPlayers) {
		NetPlayerState seatState;
		if (!(*player_i)->getMyActiveStatus()) {
			seatState = netPlayerStateNoMoney;
		} else if (!(*player_i)->isSessionActive()) {
			seatState = netPlayerStateSessionInactive;
		} else {
			seatState = netPlayerStateNormal;
		}
		netHandStart->add_seatstates(seatState);
		++player_i;
		++playerCounter;
	}

	netHandStart->set_smallblind(curGame.getCurrentHand()->getSmallBlind());
	netHandStart->set_dealerplayerid(curGame.getCurrentHand()->getDealerPosition());
	return notifyCards;
}

void
AbstractServerGameStateReceiving::AcceptNewSession(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session, bool spectateOnly)
{
	// Set game admin, if applicable.
	session->GetPlayerData()->SetGameAdmin(session->GetPlayerData()->GetUniqueId() == server->GetAdminPlayerId());

	// Send ack to client.
	server->GetLobbyThread().GetSender().Send(session, CreateNetPacketJoinGameAck(*server, *session->GetPlayerData(), spectateOnly));

	// Send notifications for connected players to client.
	PlayerDataList tmpPlayerList(server->GetFullPlayerDataList());
	PlayerDataList::iterator player_i = tmpPlayerList.begin();
	PlayerDataList::iterator player_end = tmpPlayerList.end();
	while (player_i != player_end) {
		server->GetLobbyThread().GetSender().Send(session, CreateNetPacketPlayerJoined(server->GetId(), *(*player_i)));
		++player_i;
	}

	// Send notifications for connected spectators to client.
	PlayerDataList tmpSpectatorList(server->GetSessionManager().GetSpectatorDataList());
	PlayerDataList::iterator spectator_i = tmpSpectatorList.begin();
	PlayerDataList::iterator spectator_end = tmpSpectatorList.end();
	while (spectator_i != spectator_end) {
		server->GetLobbyThread().GetSender().Send(session, CreateNetPacketSpectatorJoined(server->GetId(), *(*spectator_i)));
		++spectator_i;
	}

	// Send "Player Joined"/"Spectator Joined" to other fully connected clients.
	if (spectateOnly) {
		server->SendToAllPlayers(CreateNetPacketSpectatorJoined(server->GetId(), *session->GetPlayerData()), SessionData::Game | SessionData::Spectating | SessionData::SpectatorWaiting);
	} else {
		server->SendToAllPlayers(CreateNetPacketPlayerJoined(server->GetId(), *session->GetPlayerData()), SessionData::Game | SessionData::Spectating | SessionData::SpectatorWaiting);
	}

	// Accept session.
	server->GetSessionManager().AddSession(session);

	// Notify lobby.
	if (spectateOnly) {
		server->GetLobbyThread().NotifySpectatorJoinedGame(server->GetId(), session->GetPlayerData()->GetUniqueId());
	} else {
		server->GetLobbyThread().NotifyPlayerJoinedGame(server->GetId(), session->GetPlayerData()->GetUniqueId());
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
ServerGameStateInit::HandleNewPlayer(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session)
{
	if (session && session->GetPlayerData()) {
		const GameData &tmpGameData = server->GetGameData();

		// Check the number of players and number of joins per player
		if (server->GetCurNumberOfPlayers() >= tmpGameData.maxNumberOfPlayers || server->GetNumJoinsPerPlayer(session->GetPlayerData()->GetName()) > GAME_MAX_NUM_JOINS_PER_PLAYER) {
			server->MoveSessionToLobby(session, NTF_NET_REMOVED_GAME_FULL);
		} else {
			// add player to NumJoinsPerPlayerMap
			server->AddPlayerToNumJoinsPerPlayer(session->GetPlayerData()->GetName());

			AcceptNewSession(server, session, false);

			if (server->GetCurNumberOfPlayers() == tmpGameData.maxNumberOfPlayers) {
				// Automatically start the game if it is full.
				RegisterAutoStartTimer(server);
			}
		}
	}
}

void
ServerGameStateInit::HandleNewSpectator(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session)
{
	if (session && session->GetPlayerData()) {
		if (server->GetSpectatorIdList().size() >= SERVER_MAX_NUM_SPECTATORS_PER_GAME) {
			server->MoveSessionToLobby(session, NTF_NET_REMOVED_GAME_FULL);
		} else {
			AcceptNewSession(server, session, true);
		}
	}
}

void
ServerGameStateInit::RegisterAdminTimer(boost::shared_ptr<ServerGame> server)
{
	// No admin timeout in LAN or ranking games.
	if (server->GetLobbyThread().GetServerMode() != SERVER_MODE_LAN && server->GetGameData().gameType != GAME_TYPE_RANKING) {
		server->GetStateTimer1().expires_from_now(
			seconds(SERVER_GAME_ADMIN_TIMEOUT_SEC - SERVER_GAME_ADMIN_WARNING_REMAINING_SEC));
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
			seconds(SERVER_AUTOSTART_GAME_DELAY_SEC));
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
			boost::shared_ptr<NetPacket> packet(new NetPacket);
			packet->GetMsg()->set_messagetype(PokerTHMessage::Type_TimeoutWarningMessage);
			TimeoutWarningMessage *netWarning = packet->GetMsg()->mutable_timeoutwarningmessage();
			netWarning->set_timeoutreason(TimeoutWarningMessage::timeoutInactiveGame);
			netWarning->set_remainingseconds(SERVER_GAME_ADMIN_WARNING_REMAINING_SEC);
			server->GetLobbyThread().GetSender().Send(session, packet);
		}
		// Start timeout timer.
		server->GetStateTimer1().expires_from_now(
			seconds(SERVER_GAME_ADMIN_WARNING_REMAINING_SEC));
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
			server.SendToAllPlayers(CreateNetPacketPlayerJoined(server.GetId(), *tmpPlayerData), SessionData::Game | SessionData::Spectating | SessionData::SpectatorWaiting);

			// Notify lobby.
			server.GetLobbyThread().NotifyPlayerJoinedGame(server.GetId(), tmpPlayerData->GetUniqueId());
		}
	}
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_StartEventMessage);
	StartEventMessage *netStartEvent = packet->GetMsg()->mutable_starteventmessage();
	netStartEvent->set_starteventtype(StartEventMessage::startEvent);
	netStartEvent->set_gameid(server.GetId());
	netStartEvent->set_fillwithcomputerplayers(fillWithComputerPlayers);

	// Wait for all players to confirm start of game.
	server.SendToAllPlayers(packet, SessionData::Game);
	// Notify lobby that this game is running.
	server.GetLobbyThread().NotifyStartingGame(server.GetId());

	server.SetState(ServerGameStateStartGame::Instance());
}

void
ServerGameStateInit::InternalProcessPacket(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet)
{
	if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_StartEventMessage) {
		const StartEventMessage &netStartEvent = packet->GetMsg()->starteventmessage();
		// Only admins are allowed to start the game.
		if (session->GetPlayerData()->IsGameAdmin()
				&& netStartEvent.gameid() == server->GetId()
				&& netStartEvent.starteventtype() == StartEventMessage::startEvent
				&& (server->GetGameData().gameType != GAME_TYPE_RANKING // ranking games need to be full
					|| server->GetGameData().maxNumberOfPlayers == server->GetCurNumberOfPlayers())) {
			SendStartEvent(*server, netStartEvent.fillwithcomputerplayers());
		} else { // kick players who try to start but are not allowed to
			server->MoveSessionToLobby(session, NTF_NET_REMOVED_START_FAILED);
		}
	} else if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_InvitePlayerToGameMessage) {
		const InvitePlayerToGameMessage &netInvite = packet->GetMsg()->inviteplayertogamemessage();

		// Only invite players which are not already within the group.
		if (netInvite.gameid() == server->GetId() && !server->IsPlayerConnected(netInvite.playerid())) {
			boost::shared_ptr<NetPacket> packet(new NetPacket);
			packet->GetMsg()->set_messagetype(PokerTHMessage::Type_InviteNotifyMessage);
			InviteNotifyMessage *netInvNotif = packet->GetMsg()->mutable_invitenotifymessage();
			netInvNotif->set_gameid(netInvite.gameid());
			netInvNotif->set_playeridbywhom(session->GetPlayerData()->GetUniqueId());
			netInvNotif->set_playeridwho(netInvite.playerid());

			bool requestSent = server->GetLobbyThread().SendToLobbyPlayer(netInvite.playerid(), packet);
			server->SendToAllPlayers(packet, SessionData::Game);
			if (requestSent) {
				// This player has been invited.
				server->AddPlayerInvitation(netInvite.playerid());
			} else {
				// Player is not in lobby - send reject message.
				boost::shared_ptr<NetPacket> p2(new NetPacket);
				p2->GetMsg()->set_messagetype(PokerTHMessage::Type_RejectInvNotifyMessage);
				RejectInvNotifyMessage *netRejNotif = p2->GetMsg()->mutable_rejectinvnotifymessage();
				netRejNotif->set_gameid(netInvite.gameid());
				netRejNotif->set_playerid(netInvite.playerid());
				netRejNotif->set_playerrejectreason(RejectGameInvitationMessage::rejectReasonBusy);

				server->SendToAllPlayers(p2, SessionData::Game);
			}
		}
	} else if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_ResetTimeoutMessage) {
		if (session->GetPlayerData()->IsGameAdmin()) {
			RegisterAdminTimer(server);
		}
	} else if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_AdminRemoveGameMessage) {
		server->GetLobbyThread().HandleAdminRemoveGame(session, packet->GetMsg()->adminremovegamemessage());
	} else if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_AdminBanPlayerMessage) {
		server->GetLobbyThread().HandleAdminBanPlayer(session, packet->GetMsg()->adminbanplayermessage());
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
		seconds(SERVER_START_GAME_TIMEOUT_SEC));
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
ServerGameStateStartGame::HandleNewPlayer(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session)
{
	// Do not accept new sessions in this state.
	server->MoveSessionToLobby(session, NTF_NET_REMOVED_ALREADY_RUNNING);
}

void
ServerGameStateStartGame::HandleNewSpectator(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session)
{
	if (session && session->GetPlayerData()) {
		AcceptNewSession(server, session, true);
	}
}

void
ServerGameStateStartGame::InternalProcessPacket(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet)
{
	if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_StartEventAckMessage) {
		session->SetReadyFlag();
		if (server->GetSessionManager().CountReadySessions() == server->GetSessionManager().GetSessionCountWithState(SessionData::Game)) {
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
		boost::shared_ptr<NetPacket> packet(new NetPacket);
		packet->GetMsg()->set_messagetype(PokerTHMessage::Type_GameStartInitialMessage);
		GameStartInitialMessage *netGameStart = packet->GetMsg()->mutable_gamestartinitialmessage();
		netGameStart->set_gameid(server->GetId());
		netGameStart->set_startdealerplayerid(server->GetStartData().startDealerPlayerId);

		// Send player order to clients.
		// Assume player list is sorted by number.
		PlayerDataList::iterator player_i = tmpPlayerList.begin();
		PlayerDataList::iterator player_end = tmpPlayerList.end();
		while (player_i != player_end) {
			netGameStart->add_playerseats((*player_i)->GetUniqueId());
			++player_i;
		}

		server->SendToAllPlayers(packet, SessionData::Game | SessionData::Spectating);

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
AbstractServerGameStateRunning::HandleNewPlayer(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session)
{

	// Verify that the user is allowed to rejoin.
	if (session && session->GetPlayerData()) {
		boost::shared_ptr<PlayerInterface> tmpPlayer = server->GetPlayerInterfaceFromGame(session->GetPlayerData()->GetName());
		if (tmpPlayer && tmpPlayer->getMyGuid() == session->GetPlayerData()->GetOldGuid()) {
			// The player wants to rejoin.
			AcceptNewSession(server, session, false);
			// Remember: We need to initiate a rejoin when starting the next hand.
			server->AddRejoinPlayer(session->GetPlayerData()->GetUniqueId());

			// Send start event right away.
			boost::shared_ptr<NetPacket> packet(new NetPacket);
			packet->GetMsg()->set_messagetype(PokerTHMessage::Type_StartEventMessage);
			StartEventMessage *netStartEvent = packet->GetMsg()->mutable_starteventmessage();
			netStartEvent->set_starteventtype(StartEventMessage::rejoinEvent);
			netStartEvent->set_gameid(server->GetId());

			// Wait for rejoining player to confirm start of game.
			server->GetLobbyThread().GetSender().Send(session, packet);
		} else {
			// Do not accept "new" sessions in this state, only rejoin is allowed.
			server->MoveSessionToLobby(session, NTF_NET_REMOVED_ALREADY_RUNNING);
		}
	}
}

void
AbstractServerGameStateRunning::HandleNewSpectator(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session)
{
	if (session && session->GetPlayerData()) {
		AcceptNewSession(server, session, true);
		session->SetState(SessionData::SpectatorWaiting);
		server->AddNewSpectator(session->GetPlayerData()->GetUniqueId());
	}
}

void
AbstractServerGameStateRunning::InternalProcessPacket(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet)
{
	if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_ResetTimeoutMessage) {
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
		milliseconds(SERVER_LOOP_DELAY_MSEC));
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
			boost::shared_ptr<NetPacket> allIn(new NetPacket);
			allIn->GetMsg()->set_messagetype(PokerTHMessage::Type_AllInShowCardsMessage);
			AllInShowCardsMessage *netAllInShow = allIn->GetMsg()->mutable_allinshowcardsmessage();
			netAllInShow->set_gameid(server->GetId());

			PlayerListConstIterator i = nonFoldPlayers.begin();
			PlayerListConstIterator end = nonFoldPlayers.end();

			while (i != end) {
				AllInShowCardsMessage::PlayerAllIn *playerAllIn = netAllInShow->add_playersallin();
				playerAllIn->set_playerid((*i)->getMyUniqueID());
				int tmpCards[2];
				(*i)->getMyCards(tmpCards);
				playerAllIn->set_allincard1(tmpCards[0]);
				playerAllIn->set_allincard2(tmpCards[1]);
				++i;
			}
			server->SendToAllPlayers(allIn, SessionData::Game | SessionData::Spectating);
			curGame.getCurrentHand()->setCardsShown(true);

			server->GetStateTimer1().expires_from_now(
				seconds(SERVER_SHOW_CARDS_DELAY_SEC));
			server->GetStateTimer1().async_wait(
				boost::bind(
					&ServerGameStateHand::TimerShowCards, this, boost::asio::placeholders::error, server));
		} else {
			SendNewRoundCards(*server, curGame, newRound);

			server->GetStateTimer1().expires_from_now(
				seconds(GetDealCardsDelaySec(*server)));
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

			boost::shared_ptr<NetPacket> notification(new NetPacket);
			notification->GetMsg()->set_messagetype(PokerTHMessage::Type_PlayersTurnMessage);
			PlayersTurnMessage *netPlayersTurn = notification->GetMsg()->mutable_playersturnmessage();
			netPlayersTurn->set_gameid(server->GetId());
			netPlayersTurn->set_gamestate(static_cast<NetGameState>(curGame.getCurrentHand()->getCurrentRound()));
			netPlayersTurn->set_playerid(curPlayer->getMyUniqueID());
			server->SendToAllPlayers(notification, SessionData::Game | SessionData::Spectating);

			// If the player is computer controlled, let the engine act.
			if (curPlayer->getMyType() == PLAYER_TYPE_COMPUTER) {
				server->GetStateTimer1().expires_from_now(
					seconds(SERVER_COMPUTER_ACTION_DELAY_SEC));
				server->GetStateTimer1().async_wait(
					boost::bind(
						&ServerGameStateHand::TimerComputerAction, this, boost::asio::placeholders::error, server));
			} else {
				// If the player we are waiting for left, continue without him.
				if (!server->GetSessionManager().IsPlayerConnected(curPlayer->getMyUniqueID())
						|| !curPlayer->isSessionActive()) {
					PerformPlayerAction(*server, curPlayer, PLAYER_ACTION_FOLD, 0);

					server->GetStateTimer1().expires_from_now(
						milliseconds(SERVER_LOOP_DELAY_MSEC));
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
				boost::shared_ptr<NetPacket> endHand(new NetPacket);
				endHand->GetMsg()->set_messagetype(PokerTHMessage::Type_EndOfHandHideCardsMessage);
				EndOfHandHideCardsMessage *netEndHand = endHand->GetMsg()->mutable_endofhandhidecardsmessage();
				netEndHand->set_gameid(server->GetId());
				netEndHand->set_playerid(player->getMyUniqueID());
				netEndHand->set_moneywon(player->getLastMoneyWon());
				netEndHand->set_playermoney(player->getMyCash());
				server->SendToAllPlayers(endHand, SessionData::Game | SessionData::Spectating);
			} else {
				// End of Hand - show cards.
				const PlayerIdList showList(curGame.getCurrentHand()->getBoard()->getPlayerNeedToShowCards());
				boost::shared_ptr<NetPacket> endHand(new NetPacket);
				endHand->GetMsg()->set_messagetype(PokerTHMessage::Type_EndOfHandShowCardsMessage);
				EndOfHandShowCardsMessage *netEndHand = endHand->GetMsg()->mutable_endofhandshowcardsmessage();
				netEndHand->set_gameid(server->GetId());

				PlayerIdList::const_iterator i = showList.begin();
				PlayerIdList::const_iterator end = showList.end();

				while (i != end) {
					boost::shared_ptr<PlayerInterface> tmpPlayer(curGame.getPlayerByUniqueId(*i));
					if (tmpPlayer) {
						PlayerResult *playerResult = netEndHand->add_playerresults();
						SetPlayerResult(*playerResult, tmpPlayer, GAME_STATE_RIVER);
					}
					++i;
				}
				server->SendToAllPlayers(endHand, SessionData::Game | SessionData::Spectating);
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
					seconds(SERVER_DELAY_NEXT_GAME_SEC));
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
			seconds(GetDealCardsDelaySec(*server)));
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
		boost::shared_ptr<NetPacket> endGame(new NetPacket);
		endGame->GetMsg()->set_messagetype(PokerTHMessage::Type_EndOfGameMessage);
		EndOfGameMessage *netEndGame = endGame->GetMsg()->mutable_endofgamemessage();
		netEndGame->set_gameid(server->GetId());
		netEndGame->set_winnerplayerid(winnerPlayerId);
		server->SendToAllPlayers(endGame, SessionData::Game | SessionData::Spectating);

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

	// Initialize new spectators.
	InitNewSpectators(server);

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

			boost::shared_ptr<NetPacket> notifyCards = CreateNetPacketHandStart(*server);
			HandStartMessage *netHandStart = notifyCards->GetMsg()->mutable_handstartmessage();
			string tmpPassword(tmpSession->AuthGetPassword());
			if (tmpPassword.empty()) { // encrypt only if password is present
				HandStartMessage::PlainCards *plainCards = netHandStart->mutable_plaincards();
				plainCards->set_plaincard1(cards[0]);
				plainCards->set_plaincard2(cards[1]);
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
					netHandStart->set_encryptedcards((const char *)&tmpCipher[0], tmpCipher.size());
				} else {
					server->RemovePlayer(tmpPlayer->getMyUniqueID(), ERR_SOCK_INVALID_STATE);
					errorFlag = true;
				}
			}

			if (!errorFlag) {
				server->GetLobbyThread().GetSender().Send(tmpSession, notifyCards);
			}
		}
		++i;
	}
	server->SendToAllPlayers(CreateNetPacketHandStart(*server), SessionData::Spectating);

	// Start hand.
	curGame.startHand();

	// Auto small blind / big blind at the beginning of hand.
	i = curGame.getActivePlayerList()->begin();
	end = curGame.getActivePlayerList()->end();

	while (i != end) {
		boost::shared_ptr<PlayerInterface> tmpPlayer = *i;
		if (tmpPlayer->getMyButton() == BUTTON_SMALL_BLIND) {
			boost::shared_ptr<NetPacket> notifySmallBlind(new NetPacket);
			notifySmallBlind->GetMsg()->set_messagetype(PokerTHMessage::Type_PlayersActionDoneMessage);
			PlayersActionDoneMessage *netSmallBlind = notifySmallBlind->GetMsg()->mutable_playersactiondonemessage();
			netSmallBlind->set_gameid(server->GetId());
			netSmallBlind->set_gamestate(netStatePreflopSmallBlind);
			netSmallBlind->set_playerid(tmpPlayer->getMyUniqueID());
			netSmallBlind->set_playeraction(static_cast<NetPlayerAction>(tmpPlayer->getMyAction()));
			netSmallBlind->set_totalplayerbet(tmpPlayer->getMySet());
			netSmallBlind->set_playermoney(tmpPlayer->getMyCash());
			netSmallBlind->set_highestset(server->GetGame().getCurrentHand()->getCurrentBeRo()->getHighestSet());
			netSmallBlind->set_minimumraise(server->GetGame().getCurrentHand()->getCurrentBeRo()->getMinimumRaise());
			server->SendToAllPlayers(notifySmallBlind, SessionData::Game | SessionData::Spectating);
			break;
		}
		++i;
	}

	i = curGame.getActivePlayerList()->begin();
	end = curGame.getActivePlayerList()->end();
	while (i != end) {
		boost::shared_ptr<PlayerInterface> tmpPlayer = *i;
		if (tmpPlayer->getMyButton() == BUTTON_BIG_BLIND) {
			boost::shared_ptr<NetPacket> notifyBigBlind(new NetPacket);
			notifyBigBlind->GetMsg()->set_messagetype(PokerTHMessage::Type_PlayersActionDoneMessage);
			PlayersActionDoneMessage *netBigBlind = notifyBigBlind->GetMsg()->mutable_playersactiondonemessage();
			netBigBlind->set_gameid(server->GetId());
			netBigBlind->set_gamestate(netStatePreflopBigBlind);
			netBigBlind->set_playerid(tmpPlayer->getMyUniqueID());
			netBigBlind->set_playeraction(static_cast<NetPlayerAction>(tmpPlayer->getMyAction()));
			netBigBlind->set_totalplayerbet(tmpPlayer->getMySet());
			netBigBlind->set_playermoney(tmpPlayer->getMyCash());
			netBigBlind->set_highestset(server->GetGame().getCurrentHand()->getCurrentBeRo()->getHighestSet());
			netBigBlind->set_minimumraise(server->GetGame().getCurrentHand()->getCurrentBeRo()->getMinimumRaise());
			server->SendToAllPlayers(notifyBigBlind, SessionData::Game | SessionData::Spectating);
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
						boost::shared_ptr<NetPacket> packet(new NetPacket);
						packet->GetMsg()->set_messagetype(PokerTHMessage::Type_TimeoutWarningMessage);
						TimeoutWarningMessage *netWarning = packet->GetMsg()->mutable_timeoutwarningmessage();
						netWarning->set_timeoutreason(TimeoutWarningMessage::timeoutKickAfterAutofold);
						netWarning->set_remainingseconds(actionTimeout * SERVER_GAME_FORCED_TIMEOUT_FACTOR - tmpPlayer->getTimeSecSinceLastRemoteAction());
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
ServerGameStateHand::InitNewSpectators(boost::shared_ptr<ServerGame> server)
{
	PlayerIdList spectatorIdList(server->GetAndResetNewSpectators());
	PlayerIdList::iterator i = spectatorIdList.begin();
	PlayerIdList::iterator end = spectatorIdList.end();
	while (i != end) {
		boost::shared_ptr<SessionData> session(server->GetSessionManager().GetSessionByUniquePlayerId(*i));
		if (session && session->GetPlayerData()) {
			session->SetState(SessionData::Spectating);
			SendGameData(server, session);
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
		boost::shared_ptr<NetPacket> packet(new NetPacket);
		packet->GetMsg()->set_messagetype(PokerTHMessage::Type_PlayerIdChangedMessage);
		PlayerIdChangedMessage *netIdChanged = packet->GetMsg()->mutable_playeridchangedmessage();
		netIdChanged->set_oldplayerid(rejoinPlayer->getMyUniqueID());
		netIdChanged->set_newplayerid(session->GetPlayerData()->GetUniqueId());
		server->SendToAllButOnePlayers(packet, session->GetId(), SessionData::Game | SessionData::Spectating | SessionData::SpectatorWaiting);

		// Update the dealer, if necessary.
		curGame.replaceDealer(rejoinPlayer->getMyUniqueID(), session->GetPlayerData()->GetUniqueId());
		// Update the ranking map.
		server->ReplaceRankingPlayer(rejoinPlayer->getMyUniqueID(), session->GetPlayerData()->GetUniqueId());
		// Change the Id in the poker engine.
		rejoinPlayer->setMyUniqueID(session->GetPlayerData()->GetUniqueId());
		rejoinPlayer->setMyGuid(session->GetPlayerData()->GetGuid());
		rejoinPlayer->markRemoteAction();
		rejoinPlayer->setIsSessionActive(true);
		SendGameData(server, session);
	} else {
		server->SessionError(session, ERR_SOCK_INVALID_STATE);
	}
}

void
ServerGameStateHand::SendGameData(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session)
{
	Game &curGame = server->GetGame();
	// Send game start notification to rejoining client.
	boost::shared_ptr<NetPacket> packet(new NetPacket);
	packet->GetMsg()->set_messagetype(PokerTHMessage::Type_GameStartRejoinMessage);
	GameStartRejoinMessage *netGameStart = packet->GetMsg()->mutable_gamestartrejoinmessage();
	netGameStart->set_gameid(server->GetId());
	netGameStart->set_startdealerplayerid(curGame.getDealerPosition());
	netGameStart->set_handnum(curGame.getCurrentHandID());
	PlayerListIterator player_i = curGame.getSeatsList()->begin();
	PlayerListIterator player_end = curGame.getSeatsList()->end();
	int player_count = 0;
	while (player_i != player_end && player_count < server->GetStartData().numberOfPlayers) {
		boost::shared_ptr<PlayerInterface> tmpPlayer = *player_i;
		GameStartRejoinMessage::RejoinPlayerData *playerSlot = netGameStart->add_rejoinplayerdata();
		playerSlot->set_playerid(tmpPlayer->getMyUniqueID());
		playerSlot->set_playermoney(tmpPlayer->getMyCash());
		++player_i;
		++player_count;
	}

	server->GetLobbyThread().GetSender().Send(session, packet);
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

		server->GetStateTimer1().expires_from_now(seconds(timeoutSec));
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

	if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_MyActionRequestMessage) {
		MyActionRequestMessage *netMyAction = packet->GetMsg()->mutable_myactionrequestmessage();

		// TODO consider game id.
		Game &curGame = server->GetGame();
		boost::shared_ptr<PlayerInterface> tmpPlayer = curGame.getPlayerByUniqueId(session->GetPlayerData()->GetUniqueId());
		if (!tmpPlayer)
			throw ServerException(__FILE__, __LINE__, ERR_NET_UNKNOWN_PLAYER_ID, 0);

		// Check whether this is the correct round.
		PlayerActionCode code = ACTION_CODE_VALID;
		if (curGame.getCurrentHand()->getCurrentRound() != static_cast<GameState>(netMyAction->gamestate()))
			code = ACTION_CODE_INVALID_STATE;

		// Check whether this is the correct player.
		boost::shared_ptr<PlayerInterface> curPlayer = server->GetGame().getCurrentPlayer();
		if (code == ACTION_CODE_VALID
				&& (curPlayer->getMyUniqueID() != tmpPlayer->getMyUniqueID())) {
			code = ACTION_CODE_NOT_YOUR_TURN;
		}

		// If the client omitted some values, fill them in.
		if (netMyAction->myaction() == netActionCall && netMyAction->myrelativebet() == 0) {
			if (curGame.getCurrentHand()->getCurrentBeRo()->getHighestSet() >= tmpPlayer->getMySet() + tmpPlayer->getMyCash())
				netMyAction->set_myaction(netActionAllIn);
			else
				netMyAction->set_myrelativebet(curGame.getCurrentHand()->getCurrentBeRo()->getHighestSet() - tmpPlayer->getMySet());
		}
		if (netMyAction->myaction() == netActionAllIn && netMyAction->myrelativebet() == 0)
			netMyAction->set_myrelativebet(tmpPlayer->getMyCash());

		// Check whether the action is valid.
		if (code == ACTION_CODE_VALID
				&& (tmpPlayer->checkMyAction(
						netMyAction->myaction(),
						netMyAction->myrelativebet(),
						curGame.getCurrentHand()->getCurrentBeRo()->getHighestSet(),
						curGame.getCurrentHand()->getCurrentBeRo()->getMinimumRaise(),
						curGame.getCurrentHand()->getSmallBlind()) != 0)) {
			code = ACTION_CODE_NOT_ALLOWED;
		}

		if (code == ACTION_CODE_VALID) {
			tmpPlayer->setIsSessionActive(true);
			tmpPlayer->markRemoteAction();
			PerformPlayerAction(*server, tmpPlayer, static_cast<PlayerAction>(netMyAction->myaction()), netMyAction->myrelativebet());
			server->SetState(ServerGameStateHand::Instance());
		} else {
			// Send reject message.
			boost::shared_ptr<NetPacket> reject(new NetPacket);
			reject->GetMsg()->set_messagetype(PokerTHMessage::Type_YourActionRejectedMessage);
			YourActionRejectedMessage *netActionRejected = reject->GetMsg()->mutable_youractionrejectedmessage();
			netActionRejected->set_gameid(server->GetId());
			netActionRejected->set_gamestate(netMyAction->gamestate());
			netActionRejected->set_youraction(netMyAction->myaction());
			netActionRejected->set_yourrelativebet(netMyAction->myrelativebet());
			netActionRejected->set_rejectionreason(static_cast<YourActionRejectedMessage::RejectionReason>(code));
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
		seconds(timeoutSec));

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

	if (packet->GetMsg()->messagetype() == PokerTHMessage::Type_ShowMyCardsRequestMessage) {
		Game &curGame = server->GetGame();
		boost::shared_ptr<NetPacket> show(new NetPacket);
		show->GetMsg()->set_messagetype(PokerTHMessage::Type_AfterHandShowCardsMessage);

		AfterHandShowCardsMessage *netShowCards = show->GetMsg()->mutable_afterhandshowcardsmessage();
		boost::shared_ptr<PlayerInterface> tmpPlayer(curGame.getPlayerByUniqueId(session->GetPlayerData()->GetUniqueId()));
		if (tmpPlayer) {
			SetPlayerResult(*netShowCards->mutable_playerresult(), tmpPlayer, curGame.getCurrentHand()->getRoundBeforePostRiver());
			server->SendToAllPlayers(show, SessionData::Game | SessionData::Spectating);
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

