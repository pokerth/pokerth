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
#include <tools.h>
#include <playerinterface.h>
#include <handinterface.h>

using namespace std;

#define SERVER_WAIT_TIMEOUT_MSEC	50


ServerRecvState::~ServerRecvState()
{
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
ServerRecvStateInit::Process(ServerRecvThread &server)
{
	int retVal = MSG_SOCK_INIT_DONE;
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

			// Check the server password.
			if (!server.CheckPassword(joinGameData.password))
			{
				server.SessionError(session, ERR_NET_INVALID_PASSWORD);
				return retVal;
			}

			size_t curNumPlayers = server.GetCurNumberOfPlayers();

			// Check the number of players.
			if (curNumPlayers >= (size_t)server.GetGameData().numberOfPlayers)
			{
				server.SessionError(session, ERR_NET_SERVER_FULL);
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
			joinGameAckData.playerId = tmpPlayerData->GetUniqueId();
			joinGameAckData.playerNumber = tmpPlayerData->GetNumber();
			joinGameAckData.sessionId = session.sessionData->GetId(); // TODO: currently unused.
			joinGameAckData.gameData = server.GetGameData();
			static_cast<NetPacketJoinGameAck *>(answer.get())->SetData(joinGameAckData);
			server.GetSender().Send(recvSock, answer);

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

			// Send "Player Joined" to other clients.
			boost::shared_ptr<NetPacket> thisPlayerJoined(new NetPacketPlayerJoined);
			NetPacketPlayerJoined::Data thisPlayerJoinedData;
			thisPlayerJoinedData.playerId = tmpPlayerData->GetUniqueId();
			thisPlayerJoinedData.playerName = tmpPlayerData->GetName();
			thisPlayerJoinedData.playerNumber = tmpPlayerData->GetNumber();
			thisPlayerJoinedData.ptype = tmpPlayerData->GetType();
			static_cast<NetPacketPlayerJoined *>(thisPlayerJoined.get())->SetData(thisPlayerJoinedData);
			server.SendToAllButOnePlayers(thisPlayerJoined, session.sessionData->GetSocket());

			// Set player data for session.
			server.SetSessionPlayerData(session.sessionData, tmpPlayerData);

			// Session is now established.
			session.sessionData->SetState(SessionData::Established);
		}
	}
	return retVal;
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

void
ServerRecvStateStartGame::HandleNewConnection(ServerRecvThread &server, boost::shared_ptr<ConnectData> connData)
{
	// TODO: send error msg
}

int
ServerRecvStateStartGame::Process(ServerRecvThread &server)
{
	GameData &gameData = server.GetGameData();

	// Set dealer pos.
	Tools myTool;
	myTool.getRandNumber(0, gameData.numberOfPlayers-1, 1, &gameData.startDealerPos, 0);

	boost::shared_ptr<NetPacket> answer(new NetPacketGameStart);

	NetPacketGameStart::Data gameStartData;
	gameStartData.startDealerPos = gameData.startDealerPos;
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

void
ServerRecvStateStartHand::HandleNewConnection(ServerRecvThread &server, boost::shared_ptr<ConnectData> connData)
{
	// TODO: send error msg
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
		if (playerArray[i]->getNetSessionData().get())
		{
			int cards[2];
			playerArray[i]->getMyCards(cards);
			boost::shared_ptr<NetPacket> notification(new NetPacketHandStart);
			NetPacketHandStart::Data handStartData;
			handStartData.yourCards[0] = static_cast<unsigned>(cards[0]);
			handStartData.yourCards[1] = static_cast<unsigned>(cards[1]);
			static_cast<NetPacketHandStart *>(notification.get())->SetData(handStartData);

			server.GetSender().Send(playerArray[i]->getNetSessionData()->GetSocket(), notification);
		}
	}

	// Start hand.
	curGame.startHand();

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

void
ServerRecvStateStartRound::HandleNewConnection(ServerRecvThread &server, boost::shared_ptr<ConnectData> connData)
{
	// TODO: send error msg
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

	server.SetState(ServerRecvStateFinal::Instance());

	return MSG_NET_GAME_SERVER_ROUND;
}

void
ServerRecvStateStartRound::GameRun(Game &curGame, int state)
{
	// TODO: no switch needed here if game states are polymorphic
	switch(state) {
		case 0: {
			// Preflop starten
			curGame.getCurrentHand()->getPreflop()->preflopRun();
		} break;
		case 1: {
			// Flop starten
			curGame.getCurrentHand()->getFlop()->flopRun();
		} break;
		case 2: {
			// Turn starten
			curGame.getCurrentHand()->getTurn()->turnRun();
		} break;
		case 3: {
			// River starten
			curGame.getCurrentHand()->getRiver()->riverRun();
		} break;
		default: {
			// TODO
		}
	}
}

PlayerInterface *
ServerRecvStateStartRound::GetCurrentPlayer(Game &curGame)
{
	int curPlayerNum = 0;
	// TODO: no switch needed here if game states are polymorphic
	switch(curGame.getCurrentHand()->getActualRound()) {
		case 0: {
			curPlayerNum = curGame.getCurrentHand()->getPreflop()->getPlayersTurn();
		} break;
		case 1: {
			curPlayerNum = curGame.getCurrentHand()->getFlop()->getPlayersTurn();
		} break;
		case 2: {
			curPlayerNum = curGame.getCurrentHand()->getTurn()->getPlayersTurn();
		} break;
		case 3: {
			curPlayerNum = curGame.getCurrentHand()->getRiver()->getPlayersTurn();
		} break;
		default: {
			// TODO
		}
	}
	assert(curPlayerNum < curGame.getActualQuantityPlayers());
	return curGame.getPlayerArray()[curPlayerNum];
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

void
ServerRecvStateFinal::HandleNewConnection(ServerRecvThread &server, boost::shared_ptr<ConnectData> connData)
{
	// TODO: send error msg
}

int
ServerRecvStateFinal::Process(ServerRecvThread &server)
{
	Thread::Msleep(10);

	return MSG_SOCK_INTERNAL_PENDING;
}

//-----------------------------------------------------------------------------
