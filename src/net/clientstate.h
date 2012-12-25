/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/
/* State of network client. */

#ifndef _CLIENTSTATE_H_
#define _CLIENTSTATE_H_

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <string>

#define CLIENT_INITIAL_STATE ClientStateInit
#define CLIENT_FINAL_STATE ClientStateFinal

class ClientThread;
class ClientContext;
class ClientCallback;
class Game;
class NetPacket;
class DownloadHelper;

class ClientState
{
public:
	virtual ~ClientState();

	virtual void Enter(boost::shared_ptr<ClientThread> client) = 0;
	virtual void Exit(boost::shared_ptr<ClientThread> client) = 0;

	virtual void HandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket) = 0;
};

// State: Initialization.
class ClientStateInit : public ClientState
{
public:
	// Access the state singleton.
	static ClientStateInit &Instance();
	virtual ~ClientStateInit();

	// Some basic initialization (socket creation, basic checks).
	virtual void Enter(boost::shared_ptr<ClientThread> client);
	virtual void Exit(boost::shared_ptr<ClientThread> client);

	virtual void HandlePacket(boost::shared_ptr<ClientThread> /*client*/, boost::shared_ptr<NetPacket> /*tmpPacket*/) {}

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
	virtual void Enter(boost::shared_ptr<ClientThread> client);
	virtual void Exit(boost::shared_ptr<ClientThread> client);

	virtual void HandlePacket(boost::shared_ptr<ClientThread> /*client*/, boost::shared_ptr<NetPacket> /*tmpPacket*/) {}

protected:

	// Protected constructor - this is a singleton.
	ClientStateStartResolve();

	void HandleResolve(
		const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::iterator endpoint_iterator,
		boost::shared_ptr<ClientThread> client);
};

// State: Start download of the server list.
class ClientStateStartServerListDownload : public ClientState
{
public:
	// Access the state singleton.
	static ClientStateStartServerListDownload &Instance();
	virtual ~ClientStateStartServerListDownload();

	// Initiate the name resolution.
	virtual void Enter(boost::shared_ptr<ClientThread> client);
	virtual void Exit(boost::shared_ptr<ClientThread> client);

	virtual void HandlePacket(boost::shared_ptr<ClientThread> /*client*/, boost::shared_ptr<NetPacket> /*tmpPacket*/) {}

protected:

	// Protected constructor - this is a singleton.
	ClientStateStartServerListDownload();
};

// State: Downloading the server list.
class ClientStateDownloadingServerList : public ClientState
{
public:
	// Access the state singleton.
	static ClientStateDownloadingServerList &Instance();
	virtual ~ClientStateDownloadingServerList();

	virtual void Enter(boost::shared_ptr<ClientThread> client);
	virtual void Exit(boost::shared_ptr<ClientThread> client);

	virtual void HandlePacket(boost::shared_ptr<ClientThread> /*client*/, boost::shared_ptr<NetPacket> /*tmpPacket*/) {}

	void SetDownloadHelper(boost::shared_ptr<DownloadHelper> helper);

protected:

	// Protected constructor - this is a singleton.
	ClientStateDownloadingServerList();

	// Poll for the completion of the download.
	void TimerLoop(const boost::system::error_code& ec, boost::shared_ptr<ClientThread> client);

private:

	boost::shared_ptr<DownloadHelper> m_downloadHelper;
};

// State: Reading the server list.
class ClientStateReadingServerList : public ClientState
{
public:
	static ClientStateReadingServerList &Instance();
	virtual ~ClientStateReadingServerList();

	virtual void Enter(boost::shared_ptr<ClientThread> client);
	virtual void Exit(boost::shared_ptr<ClientThread> client);

	virtual void HandlePacket(boost::shared_ptr<ClientThread> /*client*/, boost::shared_ptr<NetPacket> /*tmpPacket*/) {}

protected:

	// Protected constructor - this is a singleton.
	ClientStateReadingServerList();
};

// State: Waiting for the user to choose a server.
class ClientStateWaitChooseServer : public ClientState
{
public:
	static ClientStateWaitChooseServer &Instance();
	virtual ~ClientStateWaitChooseServer();

	virtual void Enter(boost::shared_ptr<ClientThread> client);
	virtual void Exit(boost::shared_ptr<ClientThread> client);

	virtual void HandlePacket(boost::shared_ptr<ClientThread> /*client*/, boost::shared_ptr<NetPacket> /*tmpPacket*/) {}

protected:

	// Protected constructor - this is a singleton.
	ClientStateWaitChooseServer();

	void TimerLoop(const boost::system::error_code& ec, boost::shared_ptr<ClientThread> client);
};

// State: Initiate server connection.
class ClientStateStartConnect : public ClientState
{
public:
	// Access the state singleton.
	static ClientStateStartConnect &Instance();
	virtual ~ClientStateStartConnect();

	// Call connect.
	virtual void Enter(boost::shared_ptr<ClientThread> client);
	virtual void Exit(boost::shared_ptr<ClientThread> client);

	virtual void HandlePacket(boost::shared_ptr<ClientThread> /*client*/, boost::shared_ptr<NetPacket> /*tmpPacket*/) {}

	void SetRemoteEndpoint(boost::asio::ip::tcp::resolver::iterator endpointIterator);

protected:

	// Protected constructor - this is a singleton.
	ClientStateStartConnect();

	void HandleConnect(const boost::system::error_code& ec,
					   boost::asio::ip::tcp::resolver::iterator endpoint_iterator,
					   boost::shared_ptr<ClientThread> client);

	void TimerTimeout(const boost::system::error_code& ec, boost::shared_ptr<ClientThread> client);

private:
	boost::asio::ip::tcp::resolver::iterator m_remoteEndpointIterator;
};

// Abstract State: Receiving
class AbstractClientStateReceiving : public ClientState
{
public:
	virtual ~AbstractClientStateReceiving();

	virtual void HandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket);

protected:
	AbstractClientStateReceiving();

	virtual void InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket) = 0;
};

// State: Session init.
class ClientStateStartSession : public AbstractClientStateReceiving
{
public:
	// Access the state singleton.
	static ClientStateStartSession &Instance();
	virtual ~ClientStateStartSession();

	virtual void Enter(boost::shared_ptr<ClientThread> client);
	virtual void Exit(boost::shared_ptr<ClientThread> client);

protected:
	// Protected constructor - this is a singleton.
	ClientStateStartSession();

	virtual void InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket);
};

// State: Waiting for the user to enter login data.
class ClientStateWaitEnterLogin : public ClientState
{
public:
	static ClientStateWaitEnterLogin &Instance();
	virtual ~ClientStateWaitEnterLogin();

	virtual void Enter(boost::shared_ptr<ClientThread> client);
	virtual void Exit(boost::shared_ptr<ClientThread> client);

	virtual void HandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket);

protected:

	// Protected constructor - this is a singleton.
	ClientStateWaitEnterLogin();

	void TimerLoop(const boost::system::error_code& ec, boost::shared_ptr<ClientThread> client);
};

// State: Wait for Authentication Challenge.
class ClientStateWaitAuthChallenge : public AbstractClientStateReceiving
{
public:
	// Access the state singleton.
	static ClientStateWaitAuthChallenge &Instance();
	virtual ~ClientStateWaitAuthChallenge();

	virtual void Enter(boost::shared_ptr<ClientThread> client);
	virtual void Exit(boost::shared_ptr<ClientThread> client);

protected:
	// Protected constructor - this is a singleton.
	ClientStateWaitAuthChallenge();

	virtual void InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket);
};

// State: Wait for Authentication Verification.
class ClientStateWaitAuthVerify : public AbstractClientStateReceiving
{
public:
	// Access the state singleton.
	static ClientStateWaitAuthVerify &Instance();
	virtual ~ClientStateWaitAuthVerify();

	virtual void Enter(boost::shared_ptr<ClientThread> client);
	virtual void Exit(boost::shared_ptr<ClientThread> client);

protected:
	// Protected constructor - this is a singleton.
	ClientStateWaitAuthVerify();

	virtual void InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket);
};

// State: Wait for Session ACK.
class ClientStateWaitSession : public AbstractClientStateReceiving
{
public:
	// Access the state singleton.
	static ClientStateWaitSession &Instance();
	virtual ~ClientStateWaitSession();

	virtual void Enter(boost::shared_ptr<ClientThread> client);
	virtual void Exit(boost::shared_ptr<ClientThread> client);

protected:
	// Protected constructor - this is a singleton.
	ClientStateWaitSession();

	virtual void InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket);
};

// State: Wait for Join.
class ClientStateWaitJoin : public AbstractClientStateReceiving
{
public:
	// Access the state singleton.
	static ClientStateWaitJoin &Instance();
	virtual ~ClientStateWaitJoin();

	virtual void Enter(boost::shared_ptr<ClientThread> client);
	virtual void Exit(boost::shared_ptr<ClientThread> client);

protected:

	// Protected constructor - this is a singleton.
	ClientStateWaitJoin();

	virtual void InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket);
};

// State: Wait for start of the game or start info.
class ClientStateWaitGame : public AbstractClientStateReceiving
{
public:
	// Access the state singleton.
	static ClientStateWaitGame &Instance();
	virtual ~ClientStateWaitGame();

	virtual void Enter(boost::shared_ptr<ClientThread> client);
	virtual void Exit(boost::shared_ptr<ClientThread> client);

protected:

	// Protected constructor - this is a singleton.
	ClientStateWaitGame();

	virtual void InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket);
};

// State: Synchronize on game start.
class ClientStateSynchronizeStart : public AbstractClientStateReceiving
{
public:
	// Access the state singleton.
	static ClientStateSynchronizeStart &Instance();
	virtual ~ClientStateSynchronizeStart();

	virtual void Enter(boost::shared_ptr<ClientThread> client);
	virtual void Exit(boost::shared_ptr<ClientThread> client);

protected:

	// Protected constructor - this is a singleton.
	ClientStateSynchronizeStart();

	void TimerLoop(const boost::system::error_code& ec, boost::shared_ptr<ClientThread> client);
	virtual void InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket);
};

// State: Wait for game start.
class ClientStateWaitStart : public AbstractClientStateReceiving
{
public:
	// Access the state singleton.
	static ClientStateWaitStart &Instance();
	virtual ~ClientStateWaitStart();

	virtual void Enter(boost::shared_ptr<ClientThread> client);
	virtual void Exit(boost::shared_ptr<ClientThread> client);

protected:

	// Protected constructor - this is a singleton.
	ClientStateWaitStart();

	virtual void InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket);
};

// State: Wait for start of the next hand.
class ClientStateWaitHand : public AbstractClientStateReceiving
{
public:
	// Access the state singleton.
	static ClientStateWaitHand &Instance();
	virtual ~ClientStateWaitHand();

	virtual void Enter(boost::shared_ptr<ClientThread> client);
	virtual void Exit(boost::shared_ptr<ClientThread> client);

protected:

	// Protected constructor - this is a singleton.
	ClientStateWaitHand();

	virtual void InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket);
};

// State: Hand Loop.
class ClientStateRunHand : public AbstractClientStateReceiving
{
public:
	// Access the state singleton.
	static ClientStateRunHand &Instance();
	virtual ~ClientStateRunHand();

	virtual void Enter(boost::shared_ptr<ClientThread> client);
	virtual void Exit(boost::shared_ptr<ClientThread> client);

protected:

	// Protected constructor - this is a singleton.
	ClientStateRunHand();

	virtual void InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket);

	static void ResetPlayerActions(Game &curGame);
	static void ResetPlayerSets(Game &curGame);
};

class ClientStateFinal : public ClientState
{
public:
	static ClientStateFinal &Instance();
	virtual ~ClientStateFinal() {}

	virtual void Enter(boost::shared_ptr<ClientThread> /*client*/) {}
	virtual void Exit(boost::shared_ptr<ClientThread> /*client*/) {}

	virtual void HandlePacket(boost::shared_ptr<ClientThread> /*client*/, boost::shared_ptr<NetPacket> /*tmpPacket*/) {}

protected:
	// Protected constructor - this is a singleton.
	ClientStateFinal() {}
};

#endif
