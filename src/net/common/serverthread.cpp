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

#include <net/serverthread.h>
#include <net/servercontext.h>
#include <net/connectdata.h>
#include <net/serverrecvthread.h>
#include <net/socket_helper.h>
#include <net/serverexception.h>
#include <net/socket_msg.h>

#define ACCEPT_TIMEOUT_MSEC			50
#define NET_SERVER_LISTEN_BACKLOG	5


ServerThread::ServerThread(ServerCallback &cb)
: m_callback(cb)
{
	m_context.reset(new ServerContext);
	m_recvThread.reset(new ServerRecvThread(cb));
}

ServerThread::~ServerThread()
{
}

void
ServerThread::Init(unsigned serverPort, bool ipv6, const std::string &pwd,
	const GameData &gameData)
{
	if (IsRunning())
		return; // TODO: throw exception

	ServerContext &context = GetContext();

	context.SetAddrFamily(ipv6 ? AF_INET6 : AF_INET);
	context.SetServerPort(serverPort);

	GetRecvThread().Init(pwd, gameData);
}

void
ServerThread::StartGame()
{
	if (!IsRunning())
		return; // TODO: throw exception

	// Thread-safe notification.
	GetRecvThread().AddNotification(NOTIFY_GAME_START, 0, 0);
}

void
ServerThread::WaitForClientAction(GameState state, unsigned uniquePlayerId)
{
	if (!IsRunning())
		return; // TODO: throw exception

	// Thread-safe notification.
	GetRecvThread().AddNotification(NOTIFY_WAIT_FOR_CLIENT_ACTION, state, uniquePlayerId);
}

ServerCallback &
ServerThread::GetCallback()
{
	return m_callback;
}

void
ServerThread::Main()
{
	try
	{
		Listen();
		GetRecvThread().Run();

		while (!ShouldTerminate())
		{
			// The main server thread is simple. It only accepts connections.
			AcceptLoop();
		}
	} catch (const NetException &e)
	{
		GetCallback().SignalNetServerError(e.GetErrorId(), e.GetOsErrorCode());
	}
	GetRecvThread().SignalTermination();
	GetRecvThread().Join(RECEIVER_THREAD_TERMINATE_TIMEOUT);
}

void
ServerThread::Listen()
{
	ServerContext &context = GetContext();

//	if (context.GetServerPort() < 1024)
//		throw ServerException(ERR_SOCK_INVALID_PORT, 0);

	context.SetSocket(socket(context.GetAddrFamily(), SOCK_STREAM, 0));
	if (!IS_VALID_SOCKET(context.GetSocket()))
		throw ServerException(ERR_SOCK_CREATION_FAILED, SOCKET_ERRNO());

	unsigned long mode = 1;
	if (IOCTLSOCKET(context.GetSocket(), FIONBIO, &mode) == SOCKET_ERROR)
		throw ServerException(ERR_SOCK_CREATION_FAILED, SOCKET_ERRNO());

	context.GetServerSockaddr()->ss_family = context.GetAddrFamily();

	if (!socket_string_to_addr(
			"0.0.0.0",
			context.GetAddrFamily(),
			(struct sockaddr *)context.GetServerSockaddr(),
			context.GetServerSockaddrSize()))
	{
		throw ServerException(ERR_SOCK_SET_ADDR_FAILED, 0);
	}
	if (!socket_set_port(
			context.GetServerPort(),
			context.GetAddrFamily(),
			(struct sockaddr *)context.GetServerSockaddr(),
			context.GetServerSockaddrSize()))
	{
		throw ServerException(ERR_SOCK_SET_PORT_FAILED, 0);
	}

	if (!IS_VALID_BIND(bind(
			context.GetSocket(),
			(const struct sockaddr *)context.GetServerSockaddr(),
			context.GetServerSockaddrSize())))
	{
		throw ServerException(ERR_SOCK_BIND_FAILED, SOCKET_ERRNO());
	}

	if (!IS_VALID_LISTEN(listen(context.GetSocket(), NET_SERVER_LISTEN_BACKLOG)))
	{
		throw ServerException(ERR_SOCK_LISTEN_FAILED, SOCKET_ERRNO());
	}
}

void
ServerThread::AcceptLoop()
{
	ServerContext &context = GetContext();

	fd_set readSet;
	struct timeval timeout;

	FD_ZERO(&readSet);
	FD_SET(context.GetSocket(), &readSet);

	timeout.tv_sec  = 0;
	timeout.tv_usec = ACCEPT_TIMEOUT_MSEC * 1000;
	int selectResult = select(context.GetSocket() + 1, &readSet, NULL, NULL, &timeout);
	if (!IS_VALID_SELECT(selectResult))
	{
		throw ServerException(ERR_SOCK_SELECT_FAILED, SOCKET_ERRNO());
	}
	if (selectResult > 0) // accept is possible
	{
		boost::shared_ptr<ConnectData> tmpData(new ConnectData);
		tmpData->SetSocket(accept(context.GetSocket(), NULL, NULL));

		if (!IS_VALID_SOCKET(tmpData->GetSocket()))
		{
			throw ServerException(ERR_SOCK_ACCEPT_FAILED, SOCKET_ERRNO());
		}

		GetRecvThread().AddConnection(tmpData);
	}
}

const ServerContext &
ServerThread::GetContext() const
{
	assert(m_context.get());
	return *m_context;
}

ServerContext &
ServerThread::GetContext()
{
	assert(m_context.get());
	return *m_context;
}

ServerRecvThread &
ServerThread::GetRecvThread()
{
	assert(m_recvThread.get());
	return *m_recvThread;
}

