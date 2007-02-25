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
#include <net/resolverthread.h>
#include <net/clientexception.h>
#include <net/socket_msg.h>

#include <stdexcept>

using namespace std;


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

//	unsigned long mode = 1;
//	if (IOCTLSOCKET(data.sockfd, FIONBIO, &mode) == SOCKET_ERROR)
//		throw ClientException(ERR_SOCK_CREATION_FAILED, SOCKET_ERRNO());

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
		client.SetState(ClientStateConnect::Instance());
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

	if (m_resolver->Join(100))
	{
		ClientData &data = client.GetData();
		bool success = m_resolver->GetResult(data);
		Cleanup(); // Not required, but better keep things clean.

		if (!success)
			throw ClientException(ERR_SOCK_RESOLVE_FAILED, 0);

		client.SetState(ClientStateConnect::Instance());
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

ClientStateConnect &
ClientStateConnect::Instance()
{
	static ClientStateConnect state;
	return state;
}

ClientStateConnect::ClientStateConnect()
{
}

ClientStateConnect::~ClientStateConnect()
{
}

int
ClientStateConnect::Process(ClientThread &client)
{
	ClientData &data = client.GetData();

	if (!IS_VALID_CONNECT(connect(data.sockfd, (struct sockaddr *)&data.clientAddr, data.GetServerAddrSize())))
		throw ClientException(ERR_SOCK_CONNECT_FAILED, SOCKET_ERRNO());

	client.SetState(ClientStateFinal::Instance());

	return MSG_SOCK_CONNECT_DONE;
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
	Thread::Msleep(10);

	return MSG_SOCK_INTERNAL_PENDING;
}
