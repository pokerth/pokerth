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

#define SERVER_WAIT_TIMEOUT_MSEC	50


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
				// TODO: Deactivate player.
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

	// Session should be in initial state.
	if (session.sessionData->GetState() != SessionData::Init)
	{
		server.SessionError(session, ERR_SOCK_INVALID_STATE);
		return retVal;
	}

	// Only accept join game packets.
	const NetPacketJoinGame *tmpPacket = packet->ToNetPacketJoinGame();
	if (!tmpPacket)
	{
		server.SessionError(session, ERR_SOCK_INVALID_PACKET);
		return retVal;
	}

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
	if (curNumPlayers >= (size_t)server.GetGameData().numberOfPlayers)
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
	boost::shared_ptr<PlayerData> tmpPlayerData(
		new PlayerData(m_curUniquePlayerId++, server.GetNextPlayerNumber(), joinGameData.ptype));
	tmpPlayerData->SetName(joinGameData.playerName);
	tmpPlayerData->SetNetSessionData(session.sessionData);

	// Send ACK to client.
	boost::shared_ptr<NetPacket> answer(new NetPacketJoinGameAck);
	NetPacketJoinGameAck::Data joinGameAckData;
	joinGameAckData.sessionId = session.sessionData->GetId(); // TODO: currently unused.
	joinGameAckData.yourPlayerUniqueId = tmpPlayerData->GetUniqueId();
	joinGameAckData.yourPlayerNum = tmpPlayerData->GetNumber();
	joinGameAckData.gameData = server.GetGameData();
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
		otherPlayerJoinedData.playerNumber = (*player_i)->GetNumber();
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
	thisPlayerJoinedData.playerNumber = tmpPlayerData->GetNumber();
	thisPlayerJoinedData.ptype = tmpPlayerData->GetType();
	static_cast<NetPacketPlayerJoined *>(thisPlayerJoined.get())->SetData(thisPlayerJoinedData);
	server.SendToAllPlayers(thisPlayerJoined);

	// Set player data for session.
	server.SetSessionPlayerData(session.sessionData, tmpPlayerData);

	// Session is now established.
	session.sessionData->SetState(SessionData::Established);

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
	PlayerInterface **playerArray = curGame.getPlayerArray();

	// Send cards to all players.
	for (int i = 0; i < curGame.getActualQuantityPlayers(); i++)
	{
		assert(playerArray[i]->getNetSessionData().get()); // TODO throw exception

		int cards[2];
		playerArray[i]->getMyCards(cards);
		boost::shared_ptr<NetPacket> notifyCards(new NetPacketHandStart);
		NetPacketHandStart::Data handStartData;
		handStartData.yourCards[0] = static_cast<unsigned>(cards[0]);
		handStartData.yourCards[1] = static_cast<unsigned>(cards[1]);
		static_cast<NetPacketHandStart *>(notifyCards.get())->SetData(handStartData);

		server.GetSender().Send(playerArray[i]->getNetSessionData()->GetSocket(), notifyCards);
	}

	// Start hand.
	curGame.startHand();

	// Auto small blind / big blind at the beginning of hand.
	for (int i = 0; i < curGame.getActualQuantityPlayers(); i++)
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
			actionDoneData.potSize = curGame.getCurrentHand()->getBoard()->getPot();
			actionDoneData.curHandBets = playerArray[i]->getMySet(); // first bet only
			static_cast<NetPacketPlayersActionDone *>(notifySmallBlind.get())->SetData(actionDoneData);
			server.SendToAllPlayers(notifySmallBlind);
			break;
		}
	}
	for (int i = 0; i < curGame.getActualQuantityPlayers(); i++)
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
			actionDoneData.potSize = curGame.getCurrentHand()->getBoard()->getPot();
			actionDoneData.curHandBets = curGame.getCurrentHand()->getBoard()->getSets();
			static_cast<NetPacketPlayersActionDone *>(notifyBigBlind.get())->SetData(actionDoneData);
			server.SendToAllPlayers(notifyBigBlind);
			break;
		}
	}

	server.SetState(ServerRecvStateStartRound::Instance());

	return MSG_NET_GAME_SERVER_HAND;
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
	Game &curGame = server.GetGame();

	curGame.getCurrentHand()->switchRounds();

	// Call main loop - if state changes, call again.
	int newRound = curGame.getCurrentHand()->getActualRound();
	int curRound;
	do
	{
		curRound = newRound;
		GameRun(curGame, curRound);
		newRound = curGame.getCurrentHand()->getActualRound();
	} while (newRound != curRound);

	// Retrieve current player.
	PlayerInterface *curPlayer = GetCurrentPlayer(curGame);

	boost::shared_ptr<NetPacket> notification(new NetPacketPlayersTurn);
	NetPacketPlayersTurn::Data playersTurnData;
	playersTurnData.gameState = (GameState)curGame.getCurrentHand()->getActualRound();
	playersTurnData.playerId = curPlayer->getMyUniqueID();
	static_cast<NetPacketPlayersTurn *>(notification.get())->SetData(playersTurnData);

	server.SendToAllPlayers(notification);

	server.SetState(ServerRecvStateWaitPlayerAction::Instance());

	return MSG_NET_GAME_SERVER_ROUND;
}

void
ServerRecvStateStartRound::GameRun(Game &curGame, int state)
{
	// TODO: no switch needed here if game states are polymorphic
	switch(state) {
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

PlayerInterface *
ServerRecvStateStartRound::GetCurrentPlayer(Game &curGame)
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
	assert(curPlayerNum < curGame.getActualQuantityPlayers());
	return curGame.getPlayerArray()[curPlayerNum];
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

void
ServerRecvStateWaitPlayerAction::HandleNewConnection(ServerRecvThread &server, boost::shared_ptr<ConnectData> connData)
{
	// C++ really sucks concerning multiple inheritance *sigh*.
	// So this is a copy & paste from ServerRecvStateRunning.

	// Do not accept new connections in this state.
	server.RejectNewConnection(connData);
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

		tmpPlayer->setMyAction(actionData.playerAction);
		tmpPlayer->setMySet(actionData.playerBet);

		if (tmpPlayer->getMySet() > GetHighestSet(curGame))
			SetHighestSet(curGame, tmpPlayer->getMySet());

		boost::shared_ptr<NetPacket> notifyActionDone(new NetPacketPlayersActionDone);
		NetPacketPlayersActionDone::Data actionDoneData;
		actionDoneData.gameState = static_cast<GameState>(curGame.getCurrentHand()->getActualRound());
		actionDoneData.playerId = session.playerData->GetUniqueId();
		actionDoneData.playerAction = actionData.playerAction;
		actionDoneData.totalPlayerBet = tmpPlayer->getMySet();
		actionDoneData.playerMoney = tmpPlayer->getMyCash();
		actionDoneData.potSize = curGame.getCurrentHand()->getBoard()->getPot();
		actionDoneData.curHandBets = curGame.getCurrentHand()->getBoard()->getSets();
		static_cast<NetPacketPlayersActionDone *>(notifyActionDone.get())->SetData(actionDoneData);
		server.SendToAllPlayers(notifyActionDone);

		server.SetState(ServerRecvStateStartRound::Instance());
		retVal = MSG_NET_GAME_SERVER_ACTION;
	}

	return retVal;
}

int
ServerRecvStateWaitPlayerAction::GetHighestSet(Game &curGame)
{
	int highestSet = 0;
	// TODO: no switch needed here if game states are polymorphic
	switch(curGame.getCurrentHand()->getActualRound()) {
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
	switch(curGame.getCurrentHand()->getActualRound()) {
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
ServerRecvStateFinal::Process(ServerRecvThread &server)
{
	Thread::Msleep(10);

	return MSG_SOCK_INTERNAL_PENDING;
}

//-----------------------------------------------------------------------------
