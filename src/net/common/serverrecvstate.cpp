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
{
}

ServerRecvStateInit::~ServerRecvStateInit()
{
}

void
ServerRecvStateInit::HandleNewConnection(ServerRecvThread &server, boost::shared_ptr<ConnectData> connData)
{
	boost::shared_ptr<SessionData> sessionData(new SessionData);
	server.AddSession(connData, sessionData);
}

int
ServerRecvStateInit::Process(ServerRecvThread &server)
{
	SOCKET recvSock = server.Select();

	if (recvSock != INVALID_SOCKET)
	{
		boost::shared_ptr<NetPacket> packet = server.GetReceiver().Recv(recvSock);
		boost::shared_ptr<SessionData> session = server.GetSession(recvSock);

		// Ignore if no session / no packet.
		if (packet.get() && session.get())
		{
			if (session->GetState() == SessionData::Init)
			{
				// Only accept join game packets.
				const NetPacketJoinGame *tmpPacket = packet->ToNetPacketJoinGame();
				if (tmpPacket)
				{
					// TODO: check password
					NetPacketJoinGame::Data playerData;
					tmpPacket->GetData(playerData);
					server.GetCallback().SignalNetServerPlayerJoined(playerData.playerName);
					boost::shared_ptr<NetPacket> answer(new NetPacketJoinGameAck);
					server.GetSender().Send(answer, recvSock);
					session->SetState(SessionData::Established);
				}
				else
				{
					// TODO send error message, invalid packet
				}
			}
			else
			{
				// TODO send error message, invalid state
			}
		}
	}
	return MSG_SOCK_INIT_DONE;
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

