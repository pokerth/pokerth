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

#include <net/servergamestate.h>
#include <net/servergamethread.h>
#include <net/serverlobbythread.h>
#include <net/receiverhelper.h>
#include <net/senderthread.h>
#include <net/netpacket.h>
#include <net/socket_msg.h>
#include <net/netexception.h>
#include <gamedata.h>
#include <game.h>
#include <playerinterface.h>
#include <handinterface.h>

#include <boost/bind.hpp>

#include <sstream>

using namespace std;

#define SERVER_WAIT_TIMEOUT_MSEC				50
#define SERVER_DELAY_NEXT_HAND_SEC				10
#define SERVER_DELAY_NEXT_GAME_SEC				10
#define SERVER_DEAL_FLOP_CARDS_DELAY_SEC		5
#define SERVER_DEAL_TURN_CARD_DELAY_SEC			2
#define SERVER_DEAL_RIVER_CARD_DELAY_SEC		2
#define SERVER_DEAL_ADD_ALL_IN_DELAY_SEC		2
#define SERVER_SHOW_CARDS_DELAY_SEC				2
#define SERVER_PLAYER_TIMEOUT_ADD_DELAY_SEC		2
#define SERVER_COMPUTER_ACTION_DELAY_SEC		2
#define SERVER_START_GAME_TIMEOUT_SEC			10

#define SERVER_COMPUTER_PLAYER_NAME				"Computer"

// Helper functions
// TODO: these are hacks.

static void SendPlayerAction(ServerGameThread &server, boost::shared_ptr<PlayerInterface> player)
{
	assert(player);

	boost::shared_ptr<NetPacket> notifyActionDone(new NetPacketPlayersActionDone);
	NetPacketPlayersActionDone::Data actionDoneData;
	actionDoneData.gameState = server.GetCurRound();
	actionDoneData.playerId = player->getMyUniqueID();
	actionDoneData.playerAction = static_cast<PlayerAction>(player->getMyAction());
	actionDoneData.totalPlayerBet = player->getMySet();
	actionDoneData.playerMoney = player->getMyCash();
	actionDoneData.highestSet = server.GetGame().getCurrentHand()->getCurrentBeRo()->getHighestSet();
	static_cast<NetPacketPlayersActionDone *>(notifyActionDone.get())->SetData(actionDoneData);
	server.SendToAllPlayers(notifyActionDone, SessionData::Game);
}

static void SendNewRoundCards(ServerGameThread &server, Game &curGame, int state)
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

//-----------------------------------------------------------------------------

ServerGameState::~ServerGameState()
{
}

//-----------------------------------------------------------------------------

AbstractServerGameStateReceiving::AbstractServerGameStateReceiving()
{
}

AbstractServerGameStateReceiving::~AbstractServerGameStateReceiving()
{
}

int
AbstractServerGameStateReceiving::Process(ServerGameThread &server)
{
	// This is the receive loop for the server.
	int retVal = MSG_SOCK_INTERNAL_PENDING;
	SessionWrapper session = server.GetSessionManager().Select(RECV_TIMEOUT_MSEC);

	if (session.sessionData.get())
	{
		boost::shared_ptr<NetPacket> packet;
		try
		{
			// Receive the packet.
			packet = server.GetReceiver().Recv(session.sessionData->GetSocket(), session.sessionData->GetReceiveBuffer());
		} catch (const NetException &)
		{
			server.ErrorRemoveSession(session);
			return retVal;
		}

		// Process packet if one was received.
		if (packet.get())
		{
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
			// Chat text is always allowed.
			else if (packet->ToNetPacketSendChatText())
			{
				if (session.playerData.get()) // Only forward if this player is known.
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
			else
			{
				// Packet processing in subclass.
				retVal = InternalProcess(server, session, packet);
			}
		}
	}
	return retVal;
}

//-----------------------------------------------------------------------------

AbstractServerGameStateTimer::AbstractServerGameStateTimer()
: m_timer(boost::posix_time::time_duration(0, 0, 0), boost::timers::portable::microsec_timer::manual_start)
{
}

AbstractServerGameStateTimer::~AbstractServerGameStateTimer()
{
}

void
AbstractServerGameStateTimer::Init()
{
	m_timer.reset();
	m_timer.start();
}

//-----------------------------------------------------------------------------

boost::thread_specific_ptr<ServerGameStateInit> ServerGameStateInit::Ptr;

ServerGameStateInit &
ServerGameStateInit::Instance()
{
	if (!Ptr.get())
		Ptr.reset(new ServerGameStateInit);

	return *Ptr;
}

ServerGameStateInit::ServerGameStateInit()
{
}

ServerGameStateInit::~ServerGameStateInit()
{
}

void
ServerGameStateInit::HandleNewSession(ServerGameThread &server, SessionWrapper session)
{
	if (session.sessionData.get() && session.playerData.get())
	{
		size_t curNumPlayers = server.GetCurNumberOfPlayers();

		// Check the number of players.
		if (curNumPlayers >= (size_t)server.GetGameData().maxNumberOfPlayers)
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
			server.GetSender().Send(session.sessionData->GetSocket(), joinGameAck);

			// Send notifications for connected players to client.
			PlayerDataList tmpPlayerList = server.GetFullPlayerDataList();
			PlayerDataList::iterator player_i = tmpPlayerList.begin();
			PlayerDataList::iterator player_end = tmpPlayerList.end();
			while (player_i != player_end)
			{
				server.GetSender().Send(session.sessionData->GetSocket(), CreateNetPacketPlayerJoined(*(*player_i)));
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

int
ServerGameStateInit::InternalProcess(ServerGameThread &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet)
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
				}
			}
			// Wait for all players to confirm start of game.
			server.SendToAllPlayers(boost::shared_ptr<NetPacket>(packet->Clone()), SessionData::Game);

			server.SetState(ServerGameStateWaitAck::Instance());
		}
	}
	else if (packet->ToNetPacketKickPlayer())
	{
		// Only admins are allowed to kick.
		if (session.playerData->GetRights() == PLAYER_RIGHTS_ADMIN)
		{
			NetPacketKickPlayer::Data kickPlayerData;
			packet->ToNetPacketKickPlayer()->GetData(kickPlayerData);

			server.InternalKickPlayer(kickPlayerData.playerId);
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

AbstractServerGameStateRunning::AbstractServerGameStateRunning()
{
}

AbstractServerGameStateRunning::~AbstractServerGameStateRunning()
{
}

void
AbstractServerGameStateRunning::HandleNewSession(ServerGameThread &server, SessionWrapper session)
{
	// Do not accept new sessions in this state.
	server.MoveSessionToLobby(session, NTF_NET_REMOVED_ALREADY_RUNNING);
}

//-----------------------------------------------------------------------------

boost::thread_specific_ptr<ServerGameStateWaitAck> ServerGameStateWaitAck::Ptr;

ServerGameStateWaitAck &
ServerGameStateWaitAck::Instance()
{
	if (!Ptr.get())
		Ptr.reset(new ServerGameStateWaitAck);

	return *Ptr;
}

ServerGameStateWaitAck::ServerGameStateWaitAck()
{
}

ServerGameStateWaitAck::~ServerGameStateWaitAck()
{
}

int
ServerGameStateWaitAck::Process(ServerGameThread &server)
{
	int retVal;

	if (GetTimer().elapsed().total_seconds() >= SERVER_START_GAME_TIMEOUT_SEC)
	{
		// On timeout: start anyway.
		server.SetState(SERVER_START_GAME_STATE::Instance());
		retVal = MSG_SOCK_INIT_DONE;
	}
	else
		retVal = AbstractServerGameStateReceiving::Process(server);

	return retVal;
}

int
ServerGameStateWaitAck::InternalProcess(ServerGameThread &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet)
{
	int retVal = MSG_SOCK_INTERNAL_PENDING;

	if (packet->ToNetPacketStartEventAck())
	{
		session.sessionData->SetReadyFlag();
		if (server.GetSessionManager().CountReadySessions() == server.GetSessionManager().GetRawSessionCount())
		{
			// Everyone is ready.
			server.GetSessionManager().ResetAllReadyFlags();
			server.SetState(SERVER_START_GAME_STATE::Instance());
			retVal = MSG_SOCK_INIT_DONE;
		}
		retVal = MSG_SOCK_INIT_DONE;
	}

	return retVal;
}

//-----------------------------------------------------------------------------

boost::thread_specific_ptr<ServerGameStateStartGame> ServerGameStateStartGame::Ptr;

ServerGameStateStartGame &
ServerGameStateStartGame::Instance()
{
	if (!Ptr.get())
		Ptr.reset(new ServerGameStateStartGame);

	return *Ptr;
}

ServerGameStateStartGame::ServerGameStateStartGame()
{
}

ServerGameStateStartGame::~ServerGameStateStartGame()
{
}

int
ServerGameStateStartGame::Process(ServerGameThread &server)
{
	server.InternalStartGame();

	boost::shared_ptr<NetPacket> answer(new NetPacketGameStart);

	NetPacketGameStart::Data gameStartData;
	gameStartData.startData = server.GetStartData();

	// Send player order to clients.
	// Assume player list is sorted by number.
	PlayerDataList tmpPlayerList = server.GetFullPlayerDataList();
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
	server.SetState(ServerGameStateStartHand::Instance());

	return MSG_NET_GAME_SERVER_START;
}

//-----------------------------------------------------------------------------

boost::thread_specific_ptr<ServerGameStateStartHand> ServerGameStateStartHand::Ptr;

ServerGameStateStartHand &
ServerGameStateStartHand::Instance()
{
	if (!Ptr.get())
		Ptr.reset(new ServerGameStateStartHand);

	return *Ptr;
}

ServerGameStateStartHand::ServerGameStateStartHand()
{
}

ServerGameStateStartHand::~ServerGameStateStartHand()
{
}

int
ServerGameStateStartHand::Process(ServerGameThread &server)
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

			server.GetSender().Send(tmpPlayer->getNetSessionData()->GetSocket(), notifyCards);
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
			static_cast<NetPacketPlayersActionDone *>(notifyBigBlind.get())->SetData(actionDoneData);
			server.SendToAllPlayers(notifyBigBlind, SessionData::Game);
			break;
		}
		++i;
	}

	server.SetState(ServerGameStateStartRound::Instance());

	return MSG_NET_GAME_SERVER_HAND_START;
}

//-----------------------------------------------------------------------------

boost::thread_specific_ptr<ServerGameStateStartRound> ServerGameStateStartRound::Ptr;

ServerGameStateStartRound &
ServerGameStateStartRound::Instance()
{
	if (!Ptr.get())
		Ptr.reset(new ServerGameStateStartRound);

	return *Ptr;
}

ServerGameStateStartRound::ServerGameStateStartRound()
{
}

ServerGameStateStartRound::~ServerGameStateStartRound()
{
}

int
ServerGameStateStartRound::Process(ServerGameThread &server)
{
	int retVal = MSG_SOCK_INTERNAL_PENDING;
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
		assert(newRound > curRound);
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

			server.SetState(ServerGameStateShowCardsDelay::Instance());
			retVal = MSG_NET_GAME_SERVER_CARDS_DELAY;
		}
		else
		{
			SendNewRoundCards(server, curGame, newRound);

			server.SetState(ServerGameStateDealCardsDelay::Instance());
			retVal = MSG_NET_GAME_SERVER_CARDS_DELAY;
		}
	}
	else
	{
		if (newRound != GAME_STATE_POST_RIVER) // continue hand
		{
			assert (!curGame.getCurrentHand()->getAllInCondition()); // this would be an error.

			// Retrieve current player.
			boost::shared_ptr<PlayerInterface> curPlayer = curGame.getCurrentPlayer();
			if (!curPlayer.get())
				throw NetException(ERR_NET_NO_CURRENT_PLAYER, 0);
			if (!curPlayer->getMyActiveStatus())
				throw NetException(ERR_NET_PLAYER_NOT_ACTIVE, 0);

			boost::shared_ptr<NetPacket> notification(new NetPacketPlayersTurn);
			NetPacketPlayersTurn::Data playersTurnData;
			playersTurnData.gameState = (GameState)curGame.getCurrentHand()->getCurrentRound();
			playersTurnData.playerId = curPlayer->getMyUniqueID();
			playersTurnData.minimumRaise = curGame.getCurrentHand()->getCurrentBeRo()->getMinimumRaise();
			static_cast<NetPacketPlayersTurn *>(notification.get())->SetData(playersTurnData);

			server.SendToAllPlayers(notification, SessionData::Game);

			server.SetState(ServerGameStateWaitPlayerAction::Instance());

			retVal = MSG_NET_GAME_SERVER_ROUND;
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
				endHandData.moneyWon = 0; // TODO
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
					tmpPlayerResult.moneyWon = 0; // TODO
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
				retVal = MSG_NET_GAME_SERVER_END;
			}
			else if (playersWithCash.size() == 1)
			{
				// View a dialog for a new game - delayed.
				server.SetState(ServerGameStateNextGameDelay::Instance());
				retVal = MSG_NET_GAME_SERVER_END;
			}
			else
			{
				server.SetState(ServerGameStateNextHandDelay::Instance());
				retVal = MSG_NET_GAME_SERVER_HAND_END;
			}
		}
	}
	return retVal;
}

//-----------------------------------------------------------------------------

boost::thread_specific_ptr<ServerGameStateWaitPlayerAction> ServerGameStateWaitPlayerAction::Ptr;

ServerGameStateWaitPlayerAction &
ServerGameStateWaitPlayerAction::Instance()
{
	if (!Ptr.get())
		Ptr.reset(new ServerGameStateWaitPlayerAction);

	return *Ptr;
}

ServerGameStateWaitPlayerAction::ServerGameStateWaitPlayerAction()
{
}

ServerGameStateWaitPlayerAction::~ServerGameStateWaitPlayerAction()
{
}

int
ServerGameStateWaitPlayerAction::Process(ServerGameThread &server)
{
	int retVal;

	boost::shared_ptr<PlayerInterface> curPlayer = server.GetGame().getCurrentPlayer();
	if (!curPlayer.get())
		throw NetException(ERR_NET_NO_CURRENT_PLAYER, 0);

	// If the player is computer controlled, let the engine act.
	if (curPlayer->getMyType() == PLAYER_TYPE_COMPUTER)
	{
		server.SetState(ServerGameStateComputerAction::Instance());
		retVal = MSG_SOCK_INTERNAL_PENDING;
	}
	// If the player we are waiting for left, continue without him.
	else if (!server.GetSessionManager().IsPlayerConnected(curPlayer->getMyName()))
	{
		PerformPlayerAction(server, curPlayer, PLAYER_ACTION_FOLD, 0);

		server.SetState(ServerGameStateStartRound::Instance());
		retVal = MSG_NET_GAME_SERVER_ACTION;
	}
	else if (GetTimer().elapsed().total_seconds() >= server.GetGameData().playerActionTimeoutSec + SERVER_PLAYER_TIMEOUT_ADD_DELAY_SEC)
	{
		// Player did not act fast enough. Act for him.
		if (server.GetGame().getCurrentHand()->getCurrentBeRo()->getHighestSet() == curPlayer->getMySet())
			PerformPlayerAction(server, curPlayer, PLAYER_ACTION_CHECK, 0);
		else
			PerformPlayerAction(server, curPlayer, PLAYER_ACTION_FOLD, 0);

		server.SetState(ServerGameStateStartRound::Instance());
		retVal = MSG_NET_GAME_SERVER_ACTION;
	}
	else
		retVal = AbstractServerGameStateReceiving::Process(server);

	return retVal;
}

int
ServerGameStateWaitPlayerAction::InternalProcess(ServerGameThread &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet)
{
	int retVal = MSG_SOCK_INTERNAL_PENDING;

	if (packet->ToNetPacketPlayersAction())
	{
		NetPacketPlayersAction::Data actionData;
		packet->ToNetPacketPlayersAction()->GetData(actionData);

		Game &curGame = server.GetGame();
		boost::shared_ptr<PlayerInterface> tmpPlayer = curGame.getPlayerByUniqueId(session.playerData->GetUniqueId());
		if (!tmpPlayer.get())
			throw NetException(ERR_NET_UNKNOWN_PLAYER_ID, 0);

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
			server.SetState(ServerGameStateStartRound::Instance());
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
			server.GetSender().Send(session.sessionData->GetSocket(), reject);
		}
	}

	return retVal;
}

void
ServerGameStateWaitPlayerAction::PerformPlayerAction(ServerGameThread &server, boost::shared_ptr<PlayerInterface> player, PlayerAction action, int bet)
{
	Game &curGame = server.GetGame();
	assert(player);
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

boost::thread_specific_ptr<ServerGameStateComputerAction> ServerGameStateComputerAction::Ptr;

ServerGameStateComputerAction &
ServerGameStateComputerAction::Instance()
{
	if (!Ptr.get())
		Ptr.reset(new ServerGameStateComputerAction);

	return *Ptr;
}

ServerGameStateComputerAction::ServerGameStateComputerAction()
{
}

ServerGameStateComputerAction::~ServerGameStateComputerAction()
{
}

int
ServerGameStateComputerAction::Process(ServerGameThread &server)
{
	int retVal = AbstractServerGameStateReceiving::Process(server);

	if (GetTimer().elapsed().total_seconds() >= SERVER_COMPUTER_ACTION_DELAY_SEC)
	{
		boost::shared_ptr<PlayerInterface> tmpPlayer = server.GetGame().getCurrentPlayer();

		tmpPlayer->action();
		SendPlayerAction(server, tmpPlayer);

		server.SetState(ServerGameStateStartRound::Instance());
		retVal = MSG_NET_GAME_SERVER_ACTION;
	}

	return retVal;
}

int
ServerGameStateComputerAction::InternalProcess(ServerGameThread &/*server*/, SessionWrapper /*session*/, boost::shared_ptr<NetPacket> /*packet*/)
{
	return MSG_SOCK_INTERNAL_PENDING;
}

//-----------------------------------------------------------------------------

boost::thread_specific_ptr<ServerGameStateDealCardsDelay> ServerGameStateDealCardsDelay::Ptr;

ServerGameStateDealCardsDelay &
ServerGameStateDealCardsDelay::Instance()
{
	if (!Ptr.get())
		Ptr.reset(new ServerGameStateDealCardsDelay);

	return *Ptr;
}

ServerGameStateDealCardsDelay::ServerGameStateDealCardsDelay()
{
}

ServerGameStateDealCardsDelay::~ServerGameStateDealCardsDelay()
{
}

int
ServerGameStateDealCardsDelay::Process(ServerGameThread &server)
{
	int retVal = AbstractServerGameStateReceiving::Process(server);

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
	if (!delay || GetTimer().elapsed().total_seconds() >= delay)
		server.SetState(ServerGameStateStartRound::Instance());

	return retVal;
}

int
ServerGameStateDealCardsDelay::InternalProcess(ServerGameThread &/*server*/, SessionWrapper /*session*/, boost::shared_ptr<NetPacket> /*packet*/)
{
	return MSG_SOCK_INTERNAL_PENDING;
}

//-----------------------------------------------------------------------------

boost::thread_specific_ptr<ServerGameStateShowCardsDelay> ServerGameStateShowCardsDelay::Ptr;

ServerGameStateShowCardsDelay &
ServerGameStateShowCardsDelay::Instance()
{
	if (!Ptr.get())
		Ptr.reset(new ServerGameStateShowCardsDelay);

	return *Ptr;
}

ServerGameStateShowCardsDelay::ServerGameStateShowCardsDelay()
{
}

ServerGameStateShowCardsDelay::~ServerGameStateShowCardsDelay()
{
}

int
ServerGameStateShowCardsDelay::Process(ServerGameThread &server)
{
	int retVal = AbstractServerGameStateReceiving::Process(server);

	if (GetTimer().elapsed().total_seconds() >= SERVER_SHOW_CARDS_DELAY_SEC)
	{
		Game &curGame = server.GetGame();
		SendNewRoundCards(server, curGame, curGame.getCurrentHand()->getCurrentRound());

		server.SetState(ServerGameStateDealCardsDelay::Instance());
		retVal = MSG_NET_GAME_SERVER_CARDS_DELAY;
	}

	return retVal;
}

int
ServerGameStateShowCardsDelay::InternalProcess(ServerGameThread &/*server*/, SessionWrapper /*session*/, boost::shared_ptr<NetPacket> /*packet*/)
{
	return MSG_SOCK_INTERNAL_PENDING;
}

//-----------------------------------------------------------------------------

boost::thread_specific_ptr<ServerGameStateNextHandDelay> ServerGameStateNextHandDelay::Ptr;

ServerGameStateNextHandDelay &
ServerGameStateNextHandDelay::Instance()
{
	if (!Ptr.get())
		Ptr.reset(new ServerGameStateNextHandDelay);

	return *Ptr;
}

ServerGameStateNextHandDelay::ServerGameStateNextHandDelay()
{
}

ServerGameStateNextHandDelay::~ServerGameStateNextHandDelay()
{
}

int
ServerGameStateNextHandDelay::Process(ServerGameThread &server)
{
	int retVal = AbstractServerGameStateReceiving::Process(server);

	if (GetTimer().elapsed().total_seconds() >= SERVER_DELAY_NEXT_HAND_SEC)
		server.SetState(ServerGameStateStartHand::Instance());

	return retVal;
}

int
ServerGameStateNextHandDelay::InternalProcess(ServerGameThread &/*server*/, SessionWrapper /*session*/, boost::shared_ptr<NetPacket> /*packet*/)
{
	return MSG_SOCK_INTERNAL_PENDING;
}

//-----------------------------------------------------------------------------

boost::thread_specific_ptr<ServerGameStateNextGameDelay> ServerGameStateNextGameDelay::Ptr;

ServerGameStateNextGameDelay &
ServerGameStateNextGameDelay::Instance()
{
	if (!Ptr.get())
		Ptr.reset(new ServerGameStateNextGameDelay);

	return *Ptr;
}

ServerGameStateNextGameDelay::ServerGameStateNextGameDelay()
{
}

ServerGameStateNextGameDelay::~ServerGameStateNextGameDelay()
{
}

int
ServerGameStateNextGameDelay::Process(ServerGameThread &server)
{
	int retVal = AbstractServerGameStateReceiving::Process(server);

	if (GetTimer().elapsed().total_seconds() >= SERVER_DELAY_NEXT_GAME_SEC)
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
		server.SetState(ServerGameStateInit::Instance());
		server.GetLobbyThread().NotifyReopeningGame(server.GetId());
	}

	return retVal;
}

int
ServerGameStateNextGameDelay::InternalProcess(ServerGameThread &/*server*/, SessionWrapper /*session*/, boost::shared_ptr<NetPacket> /*packet*/)
{
	return MSG_SOCK_INTERNAL_PENDING;
}

//-----------------------------------------------------------------------------

boost::thread_specific_ptr<ServerGameStateFinal> ServerGameStateFinal::Ptr;

ServerGameStateFinal &
ServerGameStateFinal::Instance()
{
	if (!Ptr.get())
		Ptr.reset(new ServerGameStateFinal);

	return *Ptr;
}

ServerGameStateFinal::ServerGameStateFinal()
{
}

ServerGameStateFinal::~ServerGameStateFinal()
{
}

int
ServerGameStateFinal::InternalProcess(ServerGameThread &/*server*/, SessionWrapper /*session*/, boost::shared_ptr<NetPacket> /*packet*/)
{
	return MSG_SOCK_INTERNAL_PENDING;
}

//-----------------------------------------------------------------------------
