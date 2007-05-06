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

#include <net/clientthread.h>
#include <net/clientstate.h>
#include <net/clientcontext.h>
#include <net/senderthread.h>
#include <net/receiverhelper.h>
#include <net/clientexception.h>
#include <net/socket_msg.h>
#include <gamedata.h>

#include <cassert>

using namespace std;


class ClientSenderCallback : public SenderCallback
{
public:
	ClientSenderCallback(ClientThread &client) : m_client(client) {}
	virtual ~ClientSenderCallback() {}

	virtual void SignalNetError(SOCKET sock, int errorID, int osErrorID)
	{
		// For now, we ignore the socket.
		// Just signal the error.
		// We assume that the client thread will be terminated.
		m_client.GetCallback().SignalNetClientError(errorID, osErrorID);
	}

private:
	ClientThread &m_client;
};


ClientThread::ClientThread(ClientCallback &cb)
: m_curState(NULL), m_callback(cb)
{
	m_context.reset(new ClientContext);
	m_senderCallback.reset(new ClientSenderCallback(*this));
	m_sender.reset(new SenderThread(GetSenderCallback()));
	m_receiver.reset(new ReceiverHelper);
	m_gameData.reset(new GameData);
}

ClientThread::~ClientThread()
{
}

void
ClientThread::Init(
	const string &serverAddress, unsigned serverPort, bool ipv6, const string &pwd, const string &playerName)
{
	if (IsRunning())
		return; // TODO: throw exception

	ClientContext &context = GetContext();

	context.SetAddrFamily(ipv6 ? AF_INET6 : AF_INET);
	context.SetServerAddr(serverAddress);
	context.SetServerPort(serverPort);
	context.SetPassword(pwd);
	context.SetPlayerName(playerName);
}

ClientCallback &
ClientThread::GetCallback()
{
	return m_callback;
}

void
ClientThread::Main()
{
	SetState(CLIENT_INITIAL_STATE::Instance());

	GetSender().Run();

	try
	{
		while (!ShouldTerminate())
		{
			int msg = GetState().Process(*this);
			if (msg != MSG_SOCK_INTERNAL_PENDING)
			{
				if (msg <= MSG_SOCK_LIMIT_CONNECT)
					GetCallback().SignalNetClientConnect(msg);
				else
					GetCallback().SignalNetClientGameInfo(msg);

				// Additionally signal the start of the game.
				if (msg == MSG_NET_GAME_START)
					GetCallback().SignalNetClientGameStart(GetGameData());
			}
		}
	} catch (const NetException &e)
	{
		GetCallback().SignalNetClientError(e.GetErrorId(), e.GetOsErrorCode());
	}
	GetSender().SignalTermination();
	GetSender().Join(SENDER_THREAD_TERMINATE_TIMEOUT);
}

const ClientContext &
ClientThread::GetContext() const
{
	assert(m_context.get());
	return *m_context;
}

ClientContext &
ClientThread::GetContext()
{
	assert(m_context.get());
	return *m_context;
}

ClientState &
ClientThread::GetState()
{
	assert(m_curState);
	return *m_curState;
}

void
ClientThread::SetState(ClientState &newState)
{
	m_curState = &newState;
}

SenderThread &
ClientThread::GetSender()
{
	assert(m_sender.get());
	return *m_sender;
}

ReceiverHelper &
ClientThread::GetReceiver()
{
	assert(m_receiver.get());
	return *m_receiver;
}

const GameData &
ClientThread::GetGameData() const
{
	assert(m_gameData.get());
	return *m_gameData;
}

void
ClientThread::SetGameData(const GameData &gameData)
{
	assert(m_gameData.get());
	*m_gameData = gameData;
}

ClientSenderCallback &
ClientThread::GetSenderCallback()
{
	assert(m_senderCallback.get());
	return *m_senderCallback;
}

ClientThread::PlayerMap &
ClientThread::GetPlayerMap()
{
	return m_playerMap;
}

