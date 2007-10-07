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
#include <core/boost/timers.hpp>

#define CLIENT_INITIAL_STATE ClientStateInit

class ClientThread;
class ClientCallback;
class ResolverThread;
class Game;
class NetPacket;

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

	void SetTimer(const boost::timers::portable::microsec_timer &timer);

	// "Poll" for the completion of the TCP/IP connect call.
	virtual int Process(ClientThread &client);

protected:

	// Protected constructor - this is a singleton.
	ClientStateConnecting();

private:

	boost::timers::portable::microsec_timer m_connectTimer;
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

// Abstract State: Receiving
class AbstractClientStateReceiving : public ClientState
{
public:
	virtual ~AbstractClientStateReceiving();

	// select on socket.
	virtual int Process(ClientThread &client);

protected:

	virtual int InternalProcess(ClientThread &client, boost::shared_ptr<NetPacket> packet) = 0;

	AbstractClientStateReceiving();
};

// State: Wait for Session ACK.
class ClientStateWaitSession : public AbstractClientStateReceiving
{
public:
	// Access the state singleton.
	static ClientStateWaitSession &Instance();

	virtual ~ClientStateWaitSession();


protected:

	// Protected constructor - this is a singleton.
	ClientStateWaitSession();

	virtual int InternalProcess(ClientThread &client, boost::shared_ptr<NetPacket> packet);
};

// State: Wait for Join.
class ClientStateWaitJoin : public AbstractClientStateReceiving
{
public:
	// Access the state singleton.
	static ClientStateWaitJoin &Instance();

	virtual ~ClientStateWaitJoin();


protected:

	// Protected constructor - this is a singleton.
	ClientStateWaitJoin();

	virtual int InternalProcess(ClientThread &client, boost::shared_ptr<NetPacket> packet);
};

// State: Wait for start of the game or start info.
class ClientStateWaitGame : public AbstractClientStateReceiving
{
public:
	// Access the state singleton.
	static ClientStateWaitGame &Instance();

	virtual ~ClientStateWaitGame();

protected:

	// Protected constructor - this is a singleton.
	ClientStateWaitGame();

	virtual int InternalProcess(ClientThread &client, boost::shared_ptr<NetPacket> packet);
};

// State: Synchronize on game start.
class ClientStateSynchronizeStart : public AbstractClientStateReceiving
{
public:
	// Access the state singleton.
	static ClientStateSynchronizeStart &Instance();

	virtual ~ClientStateSynchronizeStart();

	virtual int Process(ClientThread &client);
protected:

	// Protected constructor - this is a singleton.
	ClientStateSynchronizeStart();

	virtual int InternalProcess(ClientThread &client, boost::shared_ptr<NetPacket> packet);
};

// State: Wait for game start.
class ClientStateWaitStart : public AbstractClientStateReceiving
{
public:
	// Access the state singleton.
	static ClientStateWaitStart &Instance();

	virtual ~ClientStateWaitStart();

protected:

	// Protected constructor - this is a singleton.
	ClientStateWaitStart();

	virtual int InternalProcess(ClientThread &client, boost::shared_ptr<NetPacket> packet);
};

// State: Wait for start of the next hand.
class ClientStateWaitHand : public AbstractClientStateReceiving
{
public:
	// Access the state singleton.
	static ClientStateWaitHand &Instance();

	virtual ~ClientStateWaitHand();

protected:

	// Protected constructor - this is a singleton.
	ClientStateWaitHand();

	virtual int InternalProcess(ClientThread &client, boost::shared_ptr<NetPacket> packet);
};

// State: Hand Loop.
class ClientStateRunHand : public AbstractClientStateReceiving
{
public:
	// Access the state singleton.
	static ClientStateRunHand &Instance();

	virtual ~ClientStateRunHand();

protected:

	// Protected constructor - this is a singleton.
	ClientStateRunHand();

	virtual int InternalProcess(ClientThread &client, boost::shared_ptr<NetPacket> packet);

	static void ResetPlayerActions(Game &curGame);
	static void ResetPlayerSets(Game &curGame);
};

// State: Final (just for testing, should not be used).
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
