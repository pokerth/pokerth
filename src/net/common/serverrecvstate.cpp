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

#include <net/serverrecvstate.h>
#include <net/serverrecvthread.h>
#include <net/receiverhelper.h>
#include <net/senderthread.h>
#include <net/netpacket.h>
#include <net/socket_msg.h>
#include <net/netexception.h>
#include <core/rand.h>
#include <gamedata.h>
#include <game.h>
#include <playerinterface.h>
#include <handinterface.h>

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

// Helper functions
// TODO: these are hacks.

static PlayerInterface *GetCurrentPlayer(Game &curGame)
{
	int curPlayerNum = 0;
	// TODO: no switch needed here if game states are polymorphic
	switch(curGame.getCurrentHand()->getActualRound()) {
		case GAME_STATE_PREFLOP: {
			curPlayerNum = curGame.getCurrentHand()->getPreflop()->getPlayersTurn();
		} break;
		case GAME_STATE_FLOP: {
			curPlayerNum = curGame.getCurrentHand()->getFlop()->getPlayersTurn();
		} break;
		case GAME_STATE_TURN: {
			curPlayerNum = curGame.getCurrentHand()->getTurn()->getPlayersTurn();
		} break;
		case GAME_STATE_RIVER: {
			curPlayerNum = curGame.getCurrentHand()->getRiver()->getPlayersTurn();
		} break;
		default: {
			// 
		}
	}
	assert(curPlayerNum < curGame.getStartQuantityPlayers()); // TODO: throw exception
	return curGame.getPlayerArray()[curPlayerNum];
}

static void SendNewRoundCards(ServerRecvThread &server, Game &curGame, int state)
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
			server.SendToAllPlayers(notifyCards);
		} break;
		case GAME_STATE_TURN: {
			// deal turn card
			int cards[5];
			curGame.getCurrentHand()->getBoard()->getMyCards(cards);
			boost::shared_ptr<NetPacket> notifyCards(new NetPacketDealTurnCard);
			NetPacketDealTurnCard::Data notifyCardsData;
			notifyCardsData.turnCard = static_cast<u_int16_t>(cards[3]);
			static_cast<NetPacketDealTurnCard *>(notifyCards.get())->SetData(notifyCardsData);
			server.SendToAllPlayers(notifyCards);
		} break;
		case GAME_STATE_RIVER: {
			// deal river card
			int cards[5];
			curGame.getCurrentHand()->getBoard()->getMyCards(cards);
			boost::shared_ptr<NetPacket> notifyCards(new NetPacketDealRiverCard);
			NetPacketDealRiverCard::Data notifyCardsData;
			notifyCardsData.riverCard = static_cast<u_int16_t>(cards[4]);
			static_cast<NetPacketDealRiverCard *>(notifyCards.get())->SetData(notifyCardsData);
			server.SendToAllPlayers(notifyCards);
		} break;
		default: {
			// 
		}
	}
}

//-----------------------------------------------------------------------------

ServerRecvState::~ServerRecvState()
{
}

//-----------------------------------------------------------------------------

ServerRecvStateReceiving::ServerRecvStateReceiving()
{
}

ServerRecvStateReceiving::~ServerRecvStateReceiving()
{
}

int
ServerRecvStateReceiving::Process(ServerRecvThread &server)
{
	int retVal = MSG_SOCK_INTERNAL_PENDING;
	SOCKET recvSock = server.Select();

	if (recvSock != INVALID_SOCKET)
	{
		SessionWrapper session = server.GetSession(recvSock);
		boost::shared_ptr<NetPacket> packet;
		try
		{
			packet = server.GetReceiver().Recv(recvSock);
		} catch (const NetException &)
		{
			if (session.sessionData.get())
			{
				server.CloseSessionDelayed(session);
				return retVal;
			}
		}

		// Ignore if no session / no packet.
		if (packet.get() && session.sessionData.get())
		{
			if (packet->ToNetPacketSendChatText()) // Chat text is always allowed.
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
					server.SendToAllPlayers(outChat);
				}
			}
			else
				retVal = InternalProcess(server, session, packet); // Let other class handle this.
		}
	}
	return retVal;
}

//-----------------------------------------------------------------------------

ServerRecvStateTimer::ServerRecvStateTimer()
{
}

ServerRecvStateTimer::~ServerRecvStateTimer()
{
}

void
ServerRecvStateTimer::Init()
{
	m_timer.reset();
	m_timer.start();
}

//-----------------------------------------------------------------------------

ServerRecvStateInit &
ServerRecvStateInit::Instance()
{
	static ServerRecvStateInit state;
	return state;
}

ServerRecvStateInit::ServerRecvStateInit()
: m_curUniquePlayerId(0)
{
}

ServerRecvStateInit::~ServerRecvStateInit()
{
}

void
ServerRecvStateInit::HandleNewConnection(ServerRecvThread &server, boost::shared_ptr<ConnectData> connData)
{
	// Create a random session id.
	// This id can be used to reconnect to the server if the connection was lost.
	unsigned sessionId;
	RandomBytes((unsigned char *)&sessionId, sizeof(sessionId)); // TODO: check for collisions.

	// Create a new session.
	boost::shared_ptr<SessionData> sessionData(new SessionData(connData->ReleaseSocket(), sessionId));
	server.AddSession(sessionData);
}

int
ServerRecvStateInit::InternalProcess(ServerRecvThread &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet)
{
	int retVal = MSG_SOCK_INIT_DONE;

	if (packet->ToNetPacketJoinGame())
	{
		// Session should be in initial state.
		if (session.sessionData->GetState() != SessionData::Init)
		{
			server.SessionError(session, ERR_SOCK_INVALID_STATE);
			return retVal;
		}

		const NetPacketJoinGame *tmpPacket = packet->ToNetPacketJoinGame();

		NetPacketJoinGame::Data joinGameData;
		tmpPacket->GetData(joinGameData);

		// Check the protocol version.
		if (joinGameData.versionMajor != NET_VERSION_MAJOR)
		{
			server.SessionError(session, ERR_NET_VERSION_NOT_SUPPORTED);
			return retVal;
		}

		size_t curNumPlayers = server.GetCurNumberOfPlayers();

		// Check the number of players.
		if (curNumPlayers >= (size_t)server.GetGameData().maxNumberOfPlayers)
		{
			server.SessionError(session, ERR_NET_SERVER_FULL);
			return retVal;
		}

		// Check the server password.
		if (!server.CheckPassword(joinGameData.password))
		{
			server.SessionError(session, ERR_NET_INVALID_PASSWORD);
			return retVal;
		}

		// Check whether the player name is correct.
		// Paranoia check, this is also done in netpacket.
		if (joinGameData.playerName.empty() || joinGameData.playerName.size() > MAX_NAME_SIZE)
		{
			server.SessionError(session, ERR_NET_INVALID_PLAYER_NAME);
			return retVal;
		}

		// Check whether this player is already connected.
		if (server.IsPlayerConnected(joinGameData.playerName))
		{
			server.SessionError(session, ERR_NET_PLAYER_NAME_IN_USE);
			return retVal;
		}

		// Create player data object.
		// TODO HACK first player is admin
		PlayerRights rights = PLAYER_RIGHTS_NORMAL;
		if (server.GetCurNumberOfPlayers() == 0)
			rights = PLAYER_RIGHTS_ADMIN;
		boost::shared_ptr<PlayerData> tmpPlayerData(
			new PlayerData(m_curUniquePlayerId++, 0, PLAYER_TYPE_HUMAN, rights));
		tmpPlayerData->SetName(joinGameData.playerName);
		tmpPlayerData->SetNetSessionData(session.sessionData);

		// Send ACK to client.
		boost::shared_ptr<NetPacket> answer(new NetPacketJoinGameAck);
		NetPacketJoinGameAck::Data joinGameAckData;
		joinGameAckData.sessionId = session.sessionData->GetId(); // TODO: currently unused.
		joinGameAckData.yourPlayerUniqueId = tmpPlayerData->GetUniqueId();
		joinGameAckData.gameData = server.GetGameData();
		joinGameAckData.ptype = tmpPlayerData->GetType();
		joinGameAckData.prights = tmpPlayerData->GetRights();
		static_cast<NetPacketJoinGameAck *>(answer.get())->SetData(joinGameAckData);
		server.GetSender().Send(session.sessionData->GetSocket(), answer);

		// Send notifications for connected players to client.
		PlayerDataList tmpPlayerList = server.GetPlayerDataList();
		PlayerDataList::iterator player_i = tmpPlayerList.begin();
		PlayerDataList::iterator player_end = tmpPlayerList.end();
		while (player_i != player_end)
		{
			boost::shared_ptr<NetPacket> otherPlayerJoined(new NetPacketPlayerJoined);
			NetPacketPlayerJoined::Data otherPlayerJoinedData;
			otherPlayerJoinedData.playerId = (*player_i)->GetUniqueId();
			otherPlayerJoinedData.playerName = (*player_i)->GetName();
			otherPlayerJoinedData.ptype = (*player_i)->GetType();
			static_cast<NetPacketPlayerJoined *>(otherPlayerJoined.get())->SetData(otherPlayerJoinedData);
			server.GetSender().Send(session.sessionData->GetSocket(), otherPlayerJoined);

			++player_i;
		}

		// Send "Player Joined" to other fully connected clients.
		boost::shared_ptr<NetPacket> thisPlayerJoined(new NetPacketPlayerJoined);
		NetPacketPlayerJoined::Data thisPlayerJoinedData;
		thisPlayerJoinedData.playerId = tmpPlayerData->GetUniqueId();
		thisPlayerJoinedData.playerName = tmpPlayerData->GetName();
		thisPlayerJoinedData.ptype = tmpPlayerData->GetType();
		thisPlayerJoinedData.prights = tmpPlayerData->GetRights();
		static_cast<NetPacketPlayerJoined *>(thisPlayerJoined.get())->SetData(thisPlayerJoinedData);
		server.SendToAllPlayers(thisPlayerJoined);

		// Set player data for session.
		server.SetSessionPlayerData(session.sessionData, tmpPlayerData);

		// Session is now established.
		session.sessionData->SetState(SessionData::Established);
	}
	else if (packet->ToNetPacketStartEvent())
	{
		server.InternalStartGame();
		server.SetState(SERVER_START_GAME_STATE::Instance());
	}
	else if (packet->ToNetPacketKickPlayer())
	{
		NetPacketKickPlayer::Data kickPlayerData;
		packet->ToNetPacketKickPlayer()->GetData(kickPlayerData);

		server.InternalKickPlayer(kickPlayerData.playerId);
	}
	else
	{
		server.SessionError(session, ERR_SOCK_INVALID_PACKET);
	}

	return retVal;
}

//-----------------------------------------------------------------------------

ServerRecvStateRunning::ServerRecvStateRunning()
{
}

ServerRecvStateRunning::~ServerRecvStateRunning()
{
}

void
ServerRecvStateRunning::HandleNewConnection(ServerRecvThread &server, boost::shared_ptr<ConnectData> connData)
{
	// Do not accept new connections in this state.
	server.RejectNewConnection(connData);
}

//-----------------------------------------------------------------------------

ServerRecvStateStartGame &
ServerRecvStateStartGame::Instance()
{
	static ServerRecvStateStartGame state;
	return state;
}

ServerRecvStateStartGame::ServerRecvStateStartGame()
{
}

ServerRecvStateStartGame::~ServerRecvStateStartGame()
{
}

int
ServerRecvStateStartGame::Process(ServerRecvThread &server)
{
	boost::shared_ptr<NetPacket> answer(new NetPacketGameStart);

	NetPacketGameStart::Data gameStartData;
	gameStartData.startData = server.GetStartData();

	// Assign player numbers. Assume Player List is sorted by number.
	PlayerDataList tmpPlayerList = server.GetPlayerDataList();
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

	server.SendToAllPlayers(answer);
	server.SetState(ServerRecvStateStartHand::Instance());

	return MSG_NET_GAME_SERVER_START;
}

//-----------------------------------------------------------------------------

ServerRecvStateStartHand &
ServerRecvStateStartHand::Instance()
{
	static ServerRecvStateStartHand state;
	return state;
}

ServerRecvStateStartHand::ServerRecvStateStartHand()
{
}

ServerRecvStateStartHand::~ServerRecvStateStartHand()
{
}

int
ServerRecvStateStartHand::Process(ServerRecvThread &server)
{
	boost::shared_ptr<NetPacket> answer(new NetPacketHandStart);

	// Initialize hand.
	Game &curGame = server.GetGame();
	curGame.initHand();

	// HACK: Skip GUI notification run
	curGame.getCurrentHand()->getFlop()->resetFirstRun();
	curGame.getCurrentHand()->getTurn()->resetFirstRun();
	curGame.getCurrentHand()->getRiver()->resetFirstRun();

	PlayerInterface **playerArray = curGame.getPlayerArray();

	// Send cards to all players.
	for (int i = 0; i < curGame.getStartQuantityPlayers(); i++)
	{
		// also send to inactive players, but not to disconnected players.
		if (playerArray[i]->getNetSessionData().get())
		{
			int cards[2];
			playerArray[i]->getMyCards(cards);
			boost::shared_ptr<NetPacket> notifyCards(new NetPacketHandStart);
			NetPacketHandStart::Data handStartData;
			handStartData.yourCards[0] = static_cast<unsigned>(cards[0]);
			handStartData.yourCards[1] = static_cast<unsigned>(cards[1]);
			static_cast<NetPacketHandStart *>(notifyCards.get())->SetData(handStartData);

			server.GetSender().Send(playerArray[i]->getNetSessionData()->GetSocket(), notifyCards);
		}
	}

	// Start hand.
	curGame.startHand();

	// Auto small blind / big blind at the beginning of hand.
	for (int i = 0; i < curGame.getStartQuantityPlayers(); i++)
	{
		if(playerArray[i]->getMyButton() == BUTTON_SMALL_BLIND)
		{
			boost::shared_ptr<NetPacket> notifySmallBlind(new NetPacketPlayersActionDone);
			NetPacketPlayersActionDone::Data actionDoneData;
			actionDoneData.gameState = GAME_STATE_PREFLOP_SMALL_BLIND;
			actionDoneData.playerId = playerArray[i]->getMyUniqueID();
			actionDoneData.playerAction = (PlayerAction)playerArray[i]->getMyAction();
			actionDoneData.totalPlayerBet = playerArray[i]->getMySet();
			actionDoneData.playerMoney = playerArray[i]->getMyCash();
			static_cast<NetPacketPlayersActionDone *>(notifySmallBlind.get())->SetData(actionDoneData);
			server.SendToAllPlayers(notifySmallBlind);
			break;
		}
	}
	for (int i = 0; i < curGame.getStartQuantityPlayers(); i++)
	{
		if(playerArray[i]->getMyButton() == BUTTON_BIG_BLIND)
		{
			boost::shared_ptr<NetPacket> notifyBigBlind(new NetPacketPlayersActionDone);
			NetPacketPlayersActionDone::Data actionDoneData;
			actionDoneData.gameState = GAME_STATE_PREFLOP_BIG_BLIND;
			actionDoneData.playerId = playerArray[i]->getMyUniqueID();
			actionDoneData.playerAction = (PlayerAction)playerArray[i]->getMyAction();
			actionDoneData.totalPlayerBet = playerArray[i]->getMySet();
			actionDoneData.playerMoney = playerArray[i]->getMyCash();
			static_cast<NetPacketPlayersActionDone *>(notifyBigBlind.get())->SetData(actionDoneData);
			server.SendToAllPlayers(notifyBigBlind);
			break;
		}
	}

	server.SetState(ServerRecvStateStartRound::Instance());

	return MSG_NET_GAME_SERVER_HAND_START;
}

//-----------------------------------------------------------------------------

ServerRecvStateStartRound &
ServerRecvStateStartRound::Instance()
{
	static ServerRecvStateStartRound state;
	return state;
}

ServerRecvStateStartRound::ServerRecvStateStartRound()
{
}

ServerRecvStateStartRound::~ServerRecvStateStartRound()
{
}

int
ServerRecvStateStartRound::Process(ServerRecvThread &server)
{
	int retVal = MSG_SOCK_INTERNAL_PENDING;
	Game &curGame = server.GetGame();

	// Main game loop.
	int curRound = curGame.getCurrentHand()->getActualRound();
	curGame.getCurrentHand()->switchRounds();
	if (!curGame.getCurrentHand()->getAllInCondition())
		GameRun(curGame);
	int newRound = curGame.getCurrentHand()->getActualRound();

	// If round changes, deal cards if needed.
	if (newRound != curRound)
	{
		assert(newRound > curRound);
		// Retrieve active players. If only one player is left, no cards are shown.
		std::list<PlayerInterface *> activePlayers = GetActivePlayers(curGame);

		if (curGame.getCurrentHand()->getAllInCondition()
			&& !curGame.getCurrentHand()->getCardsShown()
			&& activePlayers.size() > 1)
		{
			// Send cards of all active players to all players (all in).
			boost::shared_ptr<NetPacket> allIn(new NetPacketAllInShowCards);
			NetPacketAllInShowCards::Data allInData;

			std::list<PlayerInterface *>::iterator i = activePlayers.begin();
			std::list<PlayerInterface *>::iterator end = activePlayers.end();

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
			server.SendToAllPlayers(allIn);
			curGame.getCurrentHand()->setCardsShown(true);

			server.SetState(ServerRecvStateShowCardsDelay::Instance());
			retVal = MSG_NET_GAME_SERVER_CARDS_DELAY;
		}
		else
		{
			SendNewRoundCards(server, curGame, newRound);

			server.SetState(ServerRecvStateDealCardsDelay::Instance());
			retVal = MSG_NET_GAME_SERVER_CARDS_DELAY;
		}
	}
	else
	{
		if (newRound != GAME_STATE_POST_RIVER) // continue hand
		{
			assert (!curGame.getCurrentHand()->getAllInCondition()); // this would be an error.

			// Retrieve current player.
			PlayerInterface *curPlayer = GetCurrentPlayer(curGame);
			assert(curPlayer); // TODO throw exception
			assert(curPlayer->getMyActiveStatus()); // TODO throw exception

			boost::shared_ptr<NetPacket> notification(new NetPacketPlayersTurn);
			NetPacketPlayersTurn::Data playersTurnData;
			playersTurnData.gameState = (GameState)curGame.getCurrentHand()->getActualRound();
			playersTurnData.playerId = curPlayer->getMyUniqueID();
			static_cast<NetPacketPlayersTurn *>(notification.get())->SetData(playersTurnData);

			server.SendToAllPlayers(notification);

			server.SetState(ServerRecvStateWaitPlayerAction::Instance());

			retVal = MSG_NET_GAME_SERVER_ROUND;
		}
		else // hand is over
		{
			// Let the engine find out the winner(s).
			curGame.getCurrentHand()->getRiver()->postRiverRun();

			// Retrieve active players. If only one player is left, no cards are shown.
			std::list<PlayerInterface *> activePlayers = GetActivePlayers(curGame);
			// if (activePlayers.empty()) TODO throw exception

			if (activePlayers.size() == 1)
			{
				// End of Hand, but keep cards hidden.
				PlayerInterface *player = activePlayers.front();
				boost::shared_ptr<NetPacket> endHand(new NetPacketEndOfHandHideCards);
				NetPacketEndOfHandHideCards::Data endHandData;
				endHandData.playerId = player->getMyUniqueID();
				endHandData.moneyWon = 0; // TODO
				endHandData.playerMoney = player->getMyCash();
				static_cast<NetPacketEndOfHandHideCards *>(endHand.get())->SetData(endHandData);

				server.SendToAllPlayers(endHand);
			}
			else
			{
				// End of Hand - show cards of active players.
				boost::shared_ptr<NetPacket> endHand(new NetPacketEndOfHandShowCards);
				NetPacketEndOfHandShowCards::Data endHandData;

				std::list<PlayerInterface *>::iterator i = activePlayers.begin();
				std::list<PlayerInterface *>::iterator end = activePlayers.end();

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

				server.SendToAllPlayers(endHand);
			}

			// Remove disconnected players. This is the one and only place to do this.
			server.RemoveDisconnectedPlayers();

			// Start next hand - if enough players are left.
			int playersPositiveCashCounter = 0;
			for (int i = 0; i < curGame.getStartQuantityPlayers(); i++)
			{
				if (curGame.getCurrentHand()->getPlayerArray()[i]->getMyCash() > 0) 
					playersPositiveCashCounter++;
			}
			assert(playersPositiveCashCounter);
			if (playersPositiveCashCounter == 1)
			{
				// View a dialog for a new game - delayed.
				server.SetState(ServerRecvStateNextGameDelay::Instance());
				retVal = MSG_NET_GAME_SERVER_END;
			}
			else
			{
				server.SetState(ServerRecvStateNextHandDelay::Instance());
				retVal = MSG_NET_GAME_SERVER_HAND_END;
			}
		}
	}
	return retVal;
}

void
ServerRecvStateStartRound::GameRun(Game &curGame)
{
	// TODO: no switch needed here if game states are polymorphic
	switch(curGame.getCurrentHand()->getActualRound()) {
		case GAME_STATE_PREFLOP: {
			// Preflop starten
			curGame.getCurrentHand()->getPreflop()->preflopRun();
		} break;
		case GAME_STATE_FLOP: {
			// Flop starten
			curGame.getCurrentHand()->getFlop()->flopRun();
		} break;
		case GAME_STATE_TURN: {
			// Turn starten
			curGame.getCurrentHand()->getTurn()->turnRun();
		} break;
		case GAME_STATE_RIVER: {
			// River starten
			curGame.getCurrentHand()->getRiver()->riverRun();
		} break;
		default: {
			// 
		}
	}
}

std::list<PlayerInterface *>
ServerRecvStateStartRound::GetActivePlayers(Game &curGame)
{
	std::list<PlayerInterface *> activePlayers;
	for (int i = 0; i < curGame.getStartQuantityPlayers() ; i++)
	{ 
		if (curGame.getPlayerArray()[i]->getMyActiveStatus()
			&& curGame.getPlayerArray()[i]->getMyAction() != PLAYER_ACTION_FOLD)
		{
			activePlayers.push_back(curGame.getPlayerArray()[i]);
		}
	}
	return activePlayers;
}

//-----------------------------------------------------------------------------

ServerRecvStateWaitPlayerAction &
ServerRecvStateWaitPlayerAction::Instance()
{
	static ServerRecvStateWaitPlayerAction state;
	return state;
}

ServerRecvStateWaitPlayerAction::ServerRecvStateWaitPlayerAction()
{
}

ServerRecvStateWaitPlayerAction::~ServerRecvStateWaitPlayerAction()
{
}

int
ServerRecvStateWaitPlayerAction::Process(ServerRecvThread &server)
{
	int retVal;

	// If the player we are waiting for left, continue without him.
	PlayerInterface *tmpPlayer = GetCurrentPlayer(server.GetGame());
	assert(tmpPlayer);
	assert(!tmpPlayer->getMyName().empty());
	if (!server.IsPlayerConnected(tmpPlayer->getMyName()))
	{
		PerformPlayerAction(server, tmpPlayer, PLAYER_ACTION_FOLD, 0);

		server.SetState(ServerRecvStateStartRound::Instance());
		retVal = MSG_NET_GAME_SERVER_ACTION;
	}
	else if (GetTimer().elapsed().total_seconds() >= server.GetGameData().playerActionTimeoutSec + SERVER_PLAYER_TIMEOUT_ADD_DELAY_SEC)
	{
		// Player did not act fast enough. Act for him.
		if (GetHighestSet(server.GetGame()) == tmpPlayer->getMySet())
			PerformPlayerAction(server, tmpPlayer, PLAYER_ACTION_CHECK, 0);
		else
			PerformPlayerAction(server, tmpPlayer, PLAYER_ACTION_FOLD, 0);

		server.SetState(ServerRecvStateStartRound::Instance());
		retVal = MSG_NET_GAME_SERVER_ACTION;
	}
	else
		retVal = ServerRecvStateReceiving::Process(server);

	return retVal;
}

int
ServerRecvStateWaitPlayerAction::InternalProcess(ServerRecvThread &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet)
{
	int retVal = MSG_SOCK_INTERNAL_PENDING;

	if (packet->ToNetPacketPlayersAction())
	{
		NetPacketPlayersAction::Data actionData;
		packet->ToNetPacketPlayersAction()->GetData(actionData);
		
		Game &curGame = server.GetGame();
		PlayerInterface *tmpPlayer = curGame.getPlayerByUniqueId(session.playerData->GetUniqueId());
		assert(tmpPlayer); // TODO throw exception
		// TODO: check whether this is the correct player
		// TODO: check game state

		PerformPlayerAction(server, tmpPlayer, actionData.playerAction, actionData.playerBet);

		server.SetState(ServerRecvStateStartRound::Instance());
		retVal = MSG_NET_GAME_SERVER_ACTION;
	}

	return retVal;
}

void
ServerRecvStateWaitPlayerAction::PerformPlayerAction(ServerRecvThread &server, PlayerInterface *player, PlayerAction action, int bet)
{
	Game &curGame = server.GetGame();
	assert(player);
	player->setMyAction(action);
	// Only change the player bet if action is not fold/check
	if (action != PLAYER_ACTION_FOLD && action != PLAYER_ACTION_CHECK)
	{
		player->setMySet(bet);

		if (player->getMySet() > GetHighestSet(curGame))
			SetHighestSet(curGame, player->getMySet());
		// Update total sets.
		curGame.getCurrentHand()->getBoard()->collectSets();
	}

	boost::shared_ptr<NetPacket> notifyActionDone(new NetPacketPlayersActionDone);
	NetPacketPlayersActionDone::Data actionDoneData;
	actionDoneData.gameState = static_cast<GameState>(curGame.getCurrentHand()->getActualRound());
	actionDoneData.playerId = player->getMyUniqueID();
	actionDoneData.playerAction = static_cast<PlayerAction>(player->getMyAction());
	actionDoneData.totalPlayerBet = player->getMySet();
	actionDoneData.playerMoney = player->getMyCash();
	static_cast<NetPacketPlayersActionDone *>(notifyActionDone.get())->SetData(actionDoneData);
	server.SendToAllPlayers(notifyActionDone);
}

int
ServerRecvStateWaitPlayerAction::GetHighestSet(Game &curGame)
{
	int highestSet = 0;
	// TODO: no switch needed here if game states are polymorphic
	switch(curGame.getCurrentHand()->getActualRound())
	{
		case GAME_STATE_PREFLOP: {
			highestSet = curGame.getCurrentHand()->getPreflop()->getHighestSet();
		} break;
		case GAME_STATE_FLOP: {
			highestSet = curGame.getCurrentHand()->getFlop()->getHighestSet();
		} break;
		case GAME_STATE_TURN: {
			highestSet = curGame.getCurrentHand()->getTurn()->getHighestSet();
		} break;
		case GAME_STATE_RIVER: {
			highestSet = curGame.getCurrentHand()->getRiver()->getHighestSet();
		} break;
		default: {
			// 
		}
	}
	return highestSet;
}

void
ServerRecvStateWaitPlayerAction::SetHighestSet(Game &curGame, int highestSet)
{
	// TODO: no switch needed here if game states are polymorphic
	switch(curGame.getCurrentHand()->getActualRound())
	{
		case GAME_STATE_PREFLOP: {
			curGame.getCurrentHand()->getPreflop()->setHighestSet(highestSet);
		} break;
		case GAME_STATE_FLOP: {
			curGame.getCurrentHand()->getFlop()->setHighestSet(highestSet);
		} break;
		case GAME_STATE_TURN: {
			curGame.getCurrentHand()->getTurn()->setHighestSet(highestSet);
		} break;
		case GAME_STATE_RIVER: {
			curGame.getCurrentHand()->getRiver()->setHighestSet(highestSet);
		} break;
		default: {
			// 
		}
	}
}

//-----------------------------------------------------------------------------

ServerRecvStateDealCardsDelay &
ServerRecvStateDealCardsDelay::Instance()
{
	static ServerRecvStateDealCardsDelay state;
	return state;
}

ServerRecvStateDealCardsDelay::ServerRecvStateDealCardsDelay()
{
}

ServerRecvStateDealCardsDelay::~ServerRecvStateDealCardsDelay()
{
}

int
ServerRecvStateDealCardsDelay::Process(ServerRecvThread &server)
{
	int retVal = ServerRecvStateReceiving::Process(server);

	Game &curGame = server.GetGame();

	int allInDelay = curGame.getCurrentHand()->getAllInCondition() ? SERVER_DEAL_ADD_ALL_IN_DELAY_SEC : 0;
	int delay = 0;

	switch(curGame.getCurrentHand()->getActualRound())
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
		server.SetState(ServerRecvStateStartRound::Instance());

	return retVal;
}

int
ServerRecvStateDealCardsDelay::InternalProcess(ServerRecvThread &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet)
{
	return MSG_SOCK_INTERNAL_PENDING;
}

//-----------------------------------------------------------------------------

ServerRecvStateShowCardsDelay &
ServerRecvStateShowCardsDelay::Instance()
{
	static ServerRecvStateShowCardsDelay state;
	return state;
}

ServerRecvStateShowCardsDelay::ServerRecvStateShowCardsDelay()
{
}

ServerRecvStateShowCardsDelay::~ServerRecvStateShowCardsDelay()
{
}

int
ServerRecvStateShowCardsDelay::Process(ServerRecvThread &server)
{
	int retVal = ServerRecvStateReceiving::Process(server);

	if (GetTimer().elapsed().total_seconds() >= SERVER_SHOW_CARDS_DELAY_SEC)
	{
		Game &curGame = server.GetGame();
		SendNewRoundCards(server, curGame, curGame.getCurrentHand()->getActualRound());

		server.SetState(ServerRecvStateDealCardsDelay::Instance());
		retVal = MSG_NET_GAME_SERVER_CARDS_DELAY;
	}

	return retVal;
}

int
ServerRecvStateShowCardsDelay::InternalProcess(ServerRecvThread &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet)
{
	return MSG_SOCK_INTERNAL_PENDING;
}

//-----------------------------------------------------------------------------

ServerRecvStateNextHandDelay &
ServerRecvStateNextHandDelay::Instance()
{
	static ServerRecvStateNextHandDelay state;
	return state;
}

ServerRecvStateNextHandDelay::ServerRecvStateNextHandDelay()
{
}

ServerRecvStateNextHandDelay::~ServerRecvStateNextHandDelay()
{
}

int
ServerRecvStateNextHandDelay::Process(ServerRecvThread &server)
{
	int retVal = ServerRecvStateReceiving::Process(server);

	if (GetTimer().elapsed().total_seconds() >= SERVER_DELAY_NEXT_HAND_SEC)
		server.SetState(ServerRecvStateStartHand::Instance());

	return retVal;
}

int
ServerRecvStateNextHandDelay::InternalProcess(ServerRecvThread &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet)
{
	return MSG_SOCK_INTERNAL_PENDING;
}

//-----------------------------------------------------------------------------

ServerRecvStateNextGameDelay &
ServerRecvStateNextGameDelay::Instance()
{
	static ServerRecvStateNextGameDelay state;
	return state;
}

ServerRecvStateNextGameDelay::ServerRecvStateNextGameDelay()
{
}

ServerRecvStateNextGameDelay::~ServerRecvStateNextGameDelay()
{
}

int
ServerRecvStateNextGameDelay::Process(ServerRecvThread &server)
{
	int retVal = ServerRecvStateReceiving::Process(server);

	if (GetTimer().elapsed().total_seconds() >= SERVER_DELAY_NEXT_GAME_SEC)
	{
		Game &curGame = server.GetGame();
		// The game has ended. Notify all clients.
		PlayerInterface *winnerPlayer = NULL;
		for (int i = 0; i < curGame.getStartQuantityPlayers(); i++)
		{
			winnerPlayer = curGame.getCurrentHand()->getPlayerArray()[i];
			if (winnerPlayer->getMyCash() > 0)
				break;
		}

		boost::shared_ptr<NetPacket> endGame(new NetPacketEndOfGame);
		NetPacketEndOfGame::Data endGameData;
		endGameData.winnerPlayerId = winnerPlayer->getMyUniqueID();
		static_cast<NetPacketEndOfGame *>(endGame.get())->SetData(endGameData);

		server.SendToAllPlayers(endGame);

		// Wait for the start of a new game.
		server.SetState(ServerRecvStateInit::Instance());
	}

	return retVal;
}

int
ServerRecvStateNextGameDelay::InternalProcess(ServerRecvThread &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet)
{
	return MSG_SOCK_INTERNAL_PENDING;
}

//-----------------------------------------------------------------------------

ServerRecvStateFinal &
ServerRecvStateFinal::Instance()
{
	static ServerRecvStateFinal state;
	return state;
}

ServerRecvStateFinal::ServerRecvStateFinal()
{
}

ServerRecvStateFinal::~ServerRecvStateFinal()
{
}

int
ServerRecvStateFinal::InternalProcess(ServerRecvThread &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet)
{
	return MSG_SOCK_INTERNAL_PENDING;
}

//-----------------------------------------------------------------------------
