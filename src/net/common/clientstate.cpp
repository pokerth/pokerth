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
#include <net/socket_msg.h>
#include <net/clientcallback.h>

#include <stdexcept>
#include <sstream>

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

void
ClientStateInit::Process(ClientThread &client, ClientCallback &cb)
{
	ClientData &data = client.GetData();

	if (data.serverAddr.empty())
	{
		cb.SignalNetClientError(ERR_SOCK_SERVERADDR_NOT_SET, 0);
		throw runtime_error("ClientStateInit"); // TODO: own exception
	}

	if (data.serverPort < 1024)
	{
		cb.SignalNetClientError(ERR_SOCK_INVALID_PORT, 0);
		throw runtime_error("ClientStateInit");
	}

	data.sockfd = socket(data.addrFamily, SOCK_STREAM, 0);
	if (!IS_VALID_SOCKET(data.sockfd))
	{
		cb.SignalNetClientError(ERR_SOCK_CREATION_FAILED, SOCKET_ERRNO());
		throw runtime_error("ClientStateInit");
	}

#if 0
	unsigned long mode = 1;
	if (IOCTLSOCKET(data.sockfd, FIONBIO, &mode) == SOCKET_ERROR)
	{
		cb.SignalNetClientError(ERR_SOCK_CREATION_FAILED, SOCKET_ERRNO());
		throw runtime_error("ClientStateInit");
	}
#endif

	cb.SignalNetClientSuccess(MSG_SOCK_INIT_DONE);
	client.SetState(ClientStateResolve::Instance());
}

//-----------------------------------------------------------------------------

ClientStateResolve &
ClientStateResolve::Instance()
{
	static ClientStateResolve state;
	return state;
}

ClientStateResolve::ClientStateResolve()
{
}

ClientStateResolve::~ClientStateResolve()
{
}

void
ClientStateResolve::Process(ClientThread &client, ClientCallback &cb)
{
	ClientData &data = client.GetData();

	data.clientAddr.ss_family = data.addrFamily;

	// Treat the server address as numbers first.
	if (socket_string_to_addr(data.serverAddr.c_str(), data.addrFamily, (struct sockaddr *)&data.clientAddr, data.GetServerAddrSize()))
	{
		// Set the port.
		if (!socket_set_port(data.serverPort, data.addrFamily, (struct sockaddr *)&data.clientAddr, data.GetServerAddrSize()))
		{
			cb.SignalNetClientError(ERR_SOCK_SET_PORT_FAILED, 0);
			throw runtime_error("ClientStateResolve");
		}
	}
	else
	{
		// This did not work out - try name resolution.
		ostringstream tmpStr;
		tmpStr << data.serverPort;
		if (!socket_resolve(data.serverAddr.c_str(), tmpStr.str().c_str(), data.addrFamily, SOCK_STREAM, 0, (struct sockaddr *)&data.clientAddr, data.GetServerAddrSize()))
		{
			cb.SignalNetClientError(ERR_SOCK_RESOLVE_FAILED, 0); // TODO: use errno value
			throw runtime_error("ClientStateResolve");
		}
	}
	cb.SignalNetClientSuccess(MSG_SOCK_RESOLVE_DONE);
	client.SetState(ClientStateConnect::Instance());
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

void
ClientStateConnect::Process(ClientThread &client, ClientCallback &cb)
{
	ClientData &data = client.GetData();

	if (!IS_VALID_CONNECT(connect(data.sockfd, (struct sockaddr *)&data.clientAddr, data.GetServerAddrSize())))
	{
		cb.SignalNetClientError(ERR_SOCK_CONNECT_FAILED, SOCKET_ERRNO());
		throw runtime_error("ClientStateResolve");
	}
	cb.SignalNetClientSuccess(MSG_SOCK_RESOLVE_DONE);
	client.SetState(ClientStateFinal::Instance());
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

void
ClientStateFinal::Process(ClientThread &client, ClientCallback &cb)
{
	boost::xtime t;
	boost::xtime_get(&t, boost::TIME_UTC);
	t.nsec += 10000;
	if (t.nsec > NANOSECONDS_PER_SECOND)
	{
		t.sec++;
		t.nsec -= NANOSECONDS_PER_SECOND;
	}
	boost::thread::sleep(t);
}
