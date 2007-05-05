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

#include <core/boost/timer.hpp>

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
	boost::microsec_timer::time_duration_type dur;
	boost::microsec_timer t(dur);
	t.start();
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
		boost::shared_ptr<SessionData> session = server.GetSession(recvSock);
		boost::shared_ptr<NetPacket> packet;
		try
		{
			packet = server.GetReceiver().Recv(recvSock);
		} catch (const NetException &e)
		{
			if (session.get())
			{
				//server.CloseSessionDelayed(session);
				return retVal;
			}
		}

		// Ignore if no session / no packet.
		if (packet.get() && session.get())
		{
			// Session should be in initial state.
			if (session->GetState() != SessionData::Init)
			{
				//server.SessionError(session, ERR_SOCK_INVALID_STATE);
				return retVal;
			}

			// Only accept join game packets.
			const NetPacketJoinGame *tmpPacket = packet->ToNetPacketJoinGame();
			if (!tmpPacket)
			{
				//server.SessionError(session, ERR_SOCK_INVALID_PACKET);
				return retVal;
			}

			NetPacketJoinGame::Data joinGameData;
			tmpPacket->GetData(joinGameData);

			// Check the protocol version.
			if (joinGameData.versionMajor != NET_VERSION_MAJOR)
			{
				//server.SessionError(session, ERR_NET_VERSION_NOT_SUPPORTED);
				return retVal;
			}

			// Check the server password.
			if (!server.CheckPassword(joinGameData.password))
			{
				//server.SessionError(session, ERR_NET_INVALID_PASSWORD);
				return retVal;
			}

			size_t curNumPlayers = server.GetCurNumberOfPlayers();

			// Check the number of players.
			if (curNumPlayers >= (size_t)server.GetGameData().numberOfPlayers)
			{
				//server.SessionError(session, ERR_NET_SERVER_FULL);
				return retVal;
			}

			// Check whether this player is already connected.
			if (server.IsPlayerConnected(joinGameData.playerName))
			{
				//server.SessionError(session, ERR_NET_PLAYER_NAME_IN_USE);
				return retVal;
			}

			// Create player data object.
			boost::shared_ptr<PlayerData> tmpPlayerData(new PlayerData(m_curUniquePlayerId++));
			tmpPlayerData->SetName(joinGameData.playerName);
			tmpPlayerData->SetPlayerType(joinGameData.ptype);

			// Signal joining player to GUI.
			server.GetCallback().SignalNetServerPlayerJoined(tmpPlayerData->GetName());

			// Send ACK to client.
			boost::shared_ptr<NetPacket> answer(new NetPacketJoinGameAck);
			NetPacketJoinGameAck::Data joinGameAckData;
			joinGameAckData.playerId = tmpPlayerData->GetUniqueId();
			joinGameAckData.playerNumber = 0;//playerDataList.size();
			joinGameAckData.sessionId = session->GetId(); // TODO: currently unused.
			joinGameAckData.gameData = server.GetGameData();
			static_cast<NetPacketJoinGameAck *>(answer.get())->SetData(joinGameAckData);
			server.GetSender().Send(answer, recvSock);
			session->SetState(SessionData::Established);

			// Store player data in list.
			//playerDataList.push_back(tmpPlayerData);
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
	boost::shared_ptr<NetPacket> answer(new NetPacketGameStart);

	server.SendToAllPlayers(answer);
	Thread::Msleep(100);

	return MSG_SOCK_INTERNAL_PENDING;
}

//-----------------------------------------------------------------------------

