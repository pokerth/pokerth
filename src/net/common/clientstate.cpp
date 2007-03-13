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

#include <net/clientstate.h>
#include <net/clientthread.h>
#include <net/clientdata.h>
#include <net/senderthread.h>
#include <net/receiverhelper.h>
#include <net/netpacket.h>
#include <net/resolverthread.h>
#include <net/clientexception.h>
#include <net/socket_helper.h>
#include <net/socket_msg.h>

#include <stdexcept>

using namespace std;

#define CLIENT_WAIT_TIMEOUT_MSEC	100


ClientState::~ClientState()
{
}

//-----------------------------------------------------------------------------

ClientStateInit &
ClientStateInit::Instance()
{
	static ClientStateInit state;
	return state;
}

ClientStateInit::ClientStateInit()
{
}

ClientStateInit::~ClientStateInit()
{
}

int
ClientStateInit::Process(ClientThread &client)
{
	ClientData &data = client.GetData();

	if (data.serverAddr.empty())
		throw ClientException(ERR_SOCK_SERVERADDR_NOT_SET, 0);

//	if (data.serverPort < 1024)
//		throw ClientException(ERR_SOCK_INVALID_PORT, 0);

	data.sockfd = socket(data.addrFamily, SOCK_STREAM, 0);
	if (!IS_VALID_SOCKET(data.sockfd))
		throw ClientException(ERR_SOCK_CREATION_FAILED, SOCKET_ERRNO());

	unsigned long mode = 1;
	if (IOCTLSOCKET(data.sockfd, FIONBIO, &mode) == SOCKET_ERROR)
		throw ClientException(ERR_SOCK_CREATION_FAILED, SOCKET_ERRNO());

	client.SetState(ClientStateStartResolve::Instance());

	return MSG_SOCK_INIT_DONE;
}

//-----------------------------------------------------------------------------

ClientStateStartResolve &
ClientStateStartResolve::Instance()
{
	static ClientStateStartResolve state;
	return state;
}

ClientStateStartResolve::ClientStateStartResolve()
{
}

ClientStateStartResolve::~ClientStateStartResolve()
{
}

int
ClientStateStartResolve::Process(ClientThread &client)
{
	int retVal;

	ClientData &data = client.GetData();

	data.clientAddr.ss_family = data.addrFamily;

	// Treat the server address as numbers first.
	if (socket_string_to_addr(
		data.serverAddr.c_str(),
		data.addrFamily,
		(struct sockaddr *)&data.clientAddr,
		data.GetServerAddrSize()))
	{
		// Success - but we still need to set the port.
		if (!socket_set_port(data.serverPort, data.addrFamily, (struct sockaddr *)&data.clientAddr, data.GetServerAddrSize()))
			throw ClientException(ERR_SOCK_SET_PORT_FAILED, 0);

		// No need to resolve - start connecting.
		client.SetState(ClientStateStartConnect::Instance());
		retVal = MSG_SOCK_RESOLVE_DONE;
	}
	else
	{
		// Start name resolution in a separate thread, since it is blocking
		// for up to about 30 seconds.
		std::auto_ptr<ResolverThread> resolver(new ResolverThread);
		resolver->Init(data);
		resolver->Run();

		ClientStateResolving::Instance().SetResolver(resolver.release());
		client.SetState(ClientStateResolving::Instance());

		retVal = MSG_SOCK_INTERNAL_PENDING;
	}

	return retVal;
}

//-----------------------------------------------------------------------------

ClientStateResolving &
ClientStateResolving::Instance()
{
	static ClientStateResolving state;
	return state;
}

ClientStateResolving::ClientStateResolving()
: m_resolver(NULL)
{
}

ClientStateResolving::~ClientStateResolving()
{
	Cleanup();
}

void
ClientStateResolving::SetResolver(ResolverThread *resolver)
{
	Cleanup();

	m_resolver = resolver;
}

int
ClientStateResolving::Process(ClientThread &client)
{
	int retVal;

	if (!m_resolver)
		throw ClientException(ERR_SOCK_RESOLVE_FAILED, 0);

	if (m_resolver->Join(CLIENT_WAIT_TIMEOUT_MSEC))
	{
		ClientData &data = client.GetData();
		bool success = m_resolver->GetResult(data);
		Cleanup(); // Not required, but better keep things clean.

		if (!success)
			throw ClientException(ERR_SOCK_RESOLVE_FAILED, 0);

		client.SetState(ClientStateStartConnect::Instance());
		retVal = MSG_SOCK_RESOLVE_DONE;
	}
	else
		retVal = MSG_SOCK_INTERNAL_PENDING;

	return retVal;
}


void
ClientStateResolving::Cleanup()
{
	if (m_resolver)
	{
		if (m_resolver->Join(500))
			delete m_resolver;
		// If the resolver does not terminate fast enough, leave it
		// as memory leak.
		m_resolver = NULL;
	}
}

//-----------------------------------------------------------------------------

ClientStateStartConnect &
ClientStateStartConnect::Instance()
{
	static ClientStateStartConnect state;
	return state;
}

ClientStateStartConnect::ClientStateStartConnect()
{
}

ClientStateStartConnect::~ClientStateStartConnect()
{
}

int
ClientStateStartConnect::Process(ClientThread &client)
{
	int retVal;
	ClientData &data = client.GetData();

	int connectResult = connect(data.sockfd, (struct sockaddr *)&data.clientAddr, data.GetServerAddrSize());

	if (IS_VALID_CONNECT(connectResult))
	{
		client.SetState(ClientStateStartSession::Instance());
		retVal = MSG_SOCK_CONNECT_DONE;
	}
	else
	{
		int errCode = SOCKET_ERRNO();
		if (errCode == SOCKET_ERR_WOULDBLOCK)
		{
			client.SetState(ClientStateConnecting::Instance());
			retVal = MSG_SOCK_INTERNAL_PENDING;
		}
		else
			throw ClientException(ERR_SOCK_CONNECT_FAILED, SOCKET_ERRNO());
	}

	return retVal;
}

//-----------------------------------------------------------------------------

ClientStateConnecting &
ClientStateConnecting::Instance()
{
	static ClientStateConnecting state;
	return state;
}

ClientStateConnecting::ClientStateConnecting()
{
}

ClientStateConnecting::~ClientStateConnecting()
{
}

int
ClientStateConnecting::Process(ClientThread &client)
{
	int retVal;
	ClientData &data = client.GetData();

	fd_set writeSet;
	struct timeval timeout;

	FD_ZERO(&writeSet);
	FD_SET(data.sockfd, &writeSet);

	timeout.tv_sec  = 0;
	timeout.tv_usec = CLIENT_WAIT_TIMEOUT_MSEC * 1000;
	int selectResult = select(data.sockfd + 1, NULL, &writeSet, NULL, &timeout);

	if (selectResult > 0) // success
	{
		// Check whether the connect call succeeded.
		int connectResult = 0;
		socklen_t tmpSize = sizeof(connectResult);
		getsockopt(data.sockfd, SOL_SOCKET, SO_ERROR, (char *)&connectResult, &tmpSize);
		if (connectResult != 0)
			throw ClientException(ERR_SOCK_CONNECT_FAILED, connectResult);
		client.SetState(ClientStateStartSession::Instance());
		retVal = MSG_SOCK_CONNECT_DONE;
	}
	else if (selectResult == 0) // timeout
		retVal = MSG_SOCK_INTERNAL_PENDING;
	else
		throw ClientException(ERR_SOCK_SELECT_FAILED, SOCKET_ERRNO());


	return retVal;
}

//-----------------------------------------------------------------------------

ClientStateStartSession &
ClientStateStartSession::Instance()
{
	static ClientStateStartSession state;
	return state;
}

ClientStateStartSession::ClientStateStartSession()
{
}

ClientStateStartSession::~ClientStateStartSession()
{
}

int
ClientStateStartSession::Process(ClientThread &client)
{
	client.GetReceiver().Init(client.GetData().sockfd);
	client.GetSender().Init(client.GetData().sockfd);
	client.GetSender().Run();

	boost::shared_ptr<NetPacket> packet(new TestNetPacket(10));
	client.GetSender().Send(packet);

	client.SetState(ClientStateWaitSession::Instance());

	return MSG_SOCK_INTERNAL_PENDING;
}

//-----------------------------------------------------------------------------

ClientStateWaitSession &
ClientStateWaitSession::Instance()
{
	static ClientStateWaitSession state;
	return state;
}

ClientStateWaitSession::ClientStateWaitSession()
{
}

ClientStateWaitSession::~ClientStateWaitSession()
{
}

int
ClientStateWaitSession::Process(ClientThread &client)
{
	int retVal;
	ClientData &data = client.GetData();

	// delegate to receiver helper class

	boost::shared_ptr<NetPacket> tmpPacket = client.GetReceiver().Recv();

	if (tmpPacket.get())
	{
		client.SetState(ClientStateFinal::Instance());
		retVal = MSG_SOCK_SESSION_DONE;
	}
	else
	{
		retVal = MSG_SOCK_INTERNAL_PENDING;
		Thread::Msleep(CLIENT_WAIT_TIMEOUT_MSEC);
	}

	return retVal;
}

//-----------------------------------------------------------------------------

ClientStateFinal &
ClientStateFinal::Instance()
{
	static ClientStateFinal state;
	return state;
}

ClientStateFinal::ClientStateFinal()
{
}

ClientStateFinal::~ClientStateFinal()
{
}

int
ClientStateFinal::Process(ClientThread &client)
{
	Thread::Msleep(CLIENT_WAIT_TIMEOUT_MSEC);

	return MSG_SOCK_INTERNAL_PENDING;
}
