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
/* State of network client. */

#ifndef _CLIENTSTATE_H_
#define _CLIENTSTATE_H_

#include <net/socket_helper.h> // needed for correct order of header files.
#include <string>
#include <memory>
#include <core/boost/timer.hpp>

#define CLIENT_INITIAL_STATE ClientStateInit

class ClientThread;
class ClientCallback;
class ResolverThread;

class ClientState
{
public:
	virtual ~ClientState();

	// Main processing function of the current state.
	virtual int Process(ClientThread &client) = 0;
};

// State: Initialization.
class ClientStateInit : public ClientState
{
public:
	// Access the state singleton.
	static ClientStateInit &Instance();

	virtual ~ClientStateInit();

	// Some basic initialization (socket creation, basic checks).
	virtual int Process(ClientThread &client);

protected:

	// Protected constructor - this is a singleton.
	ClientStateInit();
};

// State: Starting name resolution.
class ClientStateStartResolve : public ClientState
{
public:
	// Access the state singleton.
	static ClientStateStartResolve &Instance();

	virtual ~ClientStateStartResolve();

	// Initiate the name resolution.
	virtual int Process(ClientThread &client);

protected:

	// Protected constructor - this is a singleton.
	ClientStateStartResolve();
};

// State: Name resolution.
class ClientStateResolving : public ClientState
{
public:
	// Access the state singleton.
	static ClientStateResolving &Instance();

	virtual ~ClientStateResolving();

	void SetResolver(ResolverThread *resolver);

	// Poll for the completion of the name resolution.
	virtual int Process(ClientThread &client);

protected:

	// Protected constructor - this is a singleton.
	ClientStateResolving();

	void Cleanup();

private:

	ResolverThread *m_resolver;
};

// State: Initiate server connection.
class ClientStateStartConnect : public ClientState
{
public:
	// Access the state singleton.
	static ClientStateStartConnect &Instance();

	virtual ~ClientStateStartConnect();

	void SetTimer(boost::microsec_timer timer);

	// Call connect.
	virtual int Process(ClientThread &client);

protected:

	// Protected constructor - this is a singleton.
	ClientStateStartConnect();
};

// State: Connecting to server.
class ClientStateConnecting : public ClientState
{
public:
	// Access the state singleton.
	static ClientStateConnecting &Instance();

	virtual ~ClientStateConnecting();

	void SetTimer(const boost::microsec_timer &timer);

	// "Poll" for the completion of the TCP/IP connect call.
	virtual int Process(ClientThread &client);

protected:

	// Protected constructor - this is a singleton.
	ClientStateConnecting();

private:

	boost::microsec_timer m_connectTimer;
};

// State: Session init.
class ClientStateStartSession : public ClientState
{
public:
	// Access the state singleton.
	static ClientStateStartSession &Instance();

	virtual ~ClientStateStartSession();

	// sleep.
	virtual int Process(ClientThread &client);

protected:

	// Protected constructor - this is a singleton.
	ClientStateStartSession();
};

// State: Wait for Session ACK.
class ClientStateWaitSession : public ClientState
{
public:
	// Access the state singleton.
	static ClientStateWaitSession &Instance();

	virtual ~ClientStateWaitSession();

	// select on socket.
	virtual int Process(ClientThread &client);

protected:

	// Protected constructor - this is a singleton.
	ClientStateWaitSession();
};

// State: Wait for start of the game or start info.
class ClientStateWaitGame : public ClientState
{
public:
	// Access the state singleton.
	static ClientStateWaitGame &Instance();

	virtual ~ClientStateWaitGame();

	// select on socket.
	virtual int Process(ClientThread &client);

protected:

	// Protected constructor - this is a singleton.
	ClientStateWaitGame();
};

// State: Wait for start of the next hand.
class ClientStateWaitHand : public ClientState
{
public:
	// Access the state singleton.
	static ClientStateWaitHand &Instance();

	virtual ~ClientStateWaitHand();

	// select on socket.
	virtual int Process(ClientThread &client);

protected:

	// Protected constructor - this is a singleton.
	ClientStateWaitHand();
};

// State: Final (TODO).
class ClientStateFinal : public ClientState
{
public:
	// Access the state singleton.
	static ClientStateFinal &Instance();

	virtual ~ClientStateFinal();

	// sleep.
	virtual int Process(ClientThread &client);

protected:

	// Protected constructor - this is a singleton.
	ClientStateFinal();
};

#endif
