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

#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <string>

#define CLIENT_INITIAL_STATE ClientStateInit

class ClientThread;
class ClientCallback;
class ResolverThread;
class Game;
class NetPacket;
class DownloadHelper;

class ClientState
{
public:
	virtual ~ClientState();

	virtual void Enter(boost::shared_ptr<ClientThread> client) = 0;
	virtual void Exit(boost::shared_ptr<ClientThread> client) = 0;

	virtual void HandleRead(const boost::system::error_code& ec, boost::shared_ptr<ClientThread> client, size_t bytesRead) = 0;
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

	virtual void HandleRead(const boost::system::error_code& /*ec*/, boost::shared_ptr<ClientThread> /*client*/, size_t /*bytesRead*/) {}

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

	virtual void HandleRead(const boost::system::error_code& /*ec*/, boost::shared_ptr<ClientThread> /*client*/, size_t /*bytesRead*/) {}

protected:
	void HandleResolve(
			const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::iterator endpoint_iterator,
			boost::shared_ptr<ClientThread> client);

	// Protected constructor - this is a singleton.
	ClientStateStartResolve();
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

	virtual void HandleRead(const boost::system::error_code& /*ec*/, boost::shared_ptr<ClientThread> /*client*/, size_t /*bytesRead*/) {}

protected:

	// Protected constructor - this is a singleton.
	ClientStateStartServerListDownload();
};

// State: Synchronizing server list.
class ClientStateSynchronizingServerList : public ClientState
{
public:
	// Access the state singleton.
	static ClientStateSynchronizingServerList &Instance();
	virtual ~ClientStateSynchronizingServerList();

	virtual void Enter(boost::shared_ptr<ClientThread> client);
	virtual void Exit(boost::shared_ptr<ClientThread> client);

	virtual void HandleRead(const boost::system::error_code& /*ec*/, boost::shared_ptr<ClientThread> /*client*/, size_t /*bytesRead*/) {}

	void SetDownloadHelper(boost::shared_ptr<DownloadHelper> helper);

protected:

	// Protected constructor - this is a singleton.
	ClientStateSynchronizingServerList();

	// Poll for the completion of the download.
	void TimerLoop(const boost::system::error_code& ec, boost::shared_ptr<ClientThread> client);

private:

	boost::shared_ptr<DownloadHelper> m_downloadHelper;
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

	virtual void HandleRead(const boost::system::error_code& /*ec*/, boost::shared_ptr<ClientThread> /*client*/, size_t /*bytesRead*/) {}

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

	virtual void HandleRead(const boost::system::error_code& /*ec*/, boost::shared_ptr<ClientThread> /*client*/, size_t /*bytesRead*/) {}

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

	virtual void HandleRead(const boost::system::error_code& /*ec*/, boost::shared_ptr<ClientThread> /*client*/, size_t /*bytesRead*/) {}

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

	virtual void HandleRead(const boost::system::error_code& /*ec*/, boost::shared_ptr<ClientThread> /*client*/, size_t /*bytesRead*/) {}

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

// State: Session init.
class ClientStateStartSession : public ClientState
{
public:
	// Access the state singleton.
	static ClientStateStartSession &Instance();
	virtual ~ClientStateStartSession();

	virtual void Enter(boost::shared_ptr<ClientThread> client);
	virtual void Exit(boost::shared_ptr<ClientThread> client);

	virtual void HandleRead(const boost::system::error_code& /*ec*/, boost::shared_ptr<ClientThread> /*client*/, size_t /*bytesRead*/) {}

protected:

	// Protected constructor - this is a singleton.
	ClientStateStartSession();
};

// Abstract State: Receiving
class AbstractClientStateReceiving : public ClientState
{
public:
	virtual ~AbstractClientStateReceiving();

	virtual void HandleRead(const boost::system::error_code& ec, boost::shared_ptr<ClientThread> client, size_t bytesRead);

protected:
	AbstractClientStateReceiving();

	void HandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket);
	virtual void InternalHandlePacket(boost::shared_ptr<ClientThread> client, boost::shared_ptr<NetPacket> tmpPacket) = 0;
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

#endif
