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
/* State of network server. */

#ifndef _SERVERRECVSTATE_H_
#define _SERVERRECVSTATE_H_

#include <net/serverrecvthread.h>
#include <playerdata.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4250)
#endif

#define SERVER_INITIAL_STATE	ServerRecvStateInit
#define SERVER_START_GAME_STATE	ServerRecvStateStartGame

class Game;
class PlayerInterface;
class ServerCallback;

class ServerRecvState
{
public:
	virtual ~ServerRecvState();

	// Initialize after switching to this state.
	virtual void Init() = 0;

	// Handling of a new TCP connection.
	virtual void HandleNewConnection(ServerRecvThread &server, boost::shared_ptr<ConnectData> connData) = 0;

	// Main processing function of the current state.
	virtual int Process(ServerRecvThread &server) = 0;
};

// Abstract State: Receiving.
class ServerRecvStateReceiving : virtual public ServerRecvState
{
public:
	virtual ~ServerRecvStateReceiving();

	// Globally handle packets which are allowed in all running states.
	// Calls InternalProcess if packet has not been processed.
	virtual int Process(ServerRecvThread &server);

protected:

	ServerRecvStateReceiving();

	virtual int InternalProcess(ServerRecvThread &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet) = 0;
};

// Abstract State: Timer.
class ServerRecvStateTimer : virtual public ServerRecvState
{
public:
	virtual ~ServerRecvStateTimer();

	virtual void Init();

	const boost::microsec_timer &GetTimer() const {return m_timer;}

protected:

	ServerRecvStateTimer();

private:
	boost::microsec_timer m_timer;
};

// Abstract State: Game is running.
class ServerRecvStateRunning : virtual public ServerRecvState
{
public:
	virtual ~ServerRecvStateRunning();

	// Reject new connections.
	virtual void HandleNewConnection(ServerRecvThread &server, boost::shared_ptr<ConnectData> data);

protected:

	ServerRecvStateRunning();
};

// State: Initialization.
class ServerRecvStateInit : public ServerRecvStateReceiving
{
public:
	// Access the state singleton.
	static ServerRecvStateInit &Instance();

	virtual ~ServerRecvStateInit();

	virtual void Init() {}
	// 
	virtual void HandleNewConnection(ServerRecvThread &server, boost::shared_ptr<ConnectData> data);

protected:

	// Protected constructor - this is a singleton.
	ServerRecvStateInit();

	virtual int InternalProcess(ServerRecvThread &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet);

private:

	u_int16_t		m_curUniquePlayerId;
};

// State: Start server game.
class ServerRecvStateStartGame : public ServerRecvStateRunning
{
public:
	// Access the state singleton.
	static ServerRecvStateStartGame &Instance();

	virtual ~ServerRecvStateStartGame();

	virtual void Init() {}

	// 
	virtual int Process(ServerRecvThread &server);

protected:

	// Protected constructor - this is a singleton.
	ServerRecvStateStartGame();
};

// State: Start new hand.
class ServerRecvStateStartHand : public ServerRecvStateRunning
{
public:
	// Access the state singleton.
	static ServerRecvStateStartHand &Instance();

	virtual ~ServerRecvStateStartHand();

	virtual void Init() {}

	// 
	virtual int Process(ServerRecvThread &server);

protected:

	// Protected constructor - this is a singleton.
	ServerRecvStateStartHand();
};

// State: Start new round.
class ServerRecvStateStartRound : public ServerRecvStateRunning
{
public:
	// Access the state singleton.
	static ServerRecvStateStartRound &Instance();

	virtual ~ServerRecvStateStartRound();

	virtual void Init() {}

	// 
	virtual int Process(ServerRecvThread &server);

protected:

	// Protected constructor - this is a singleton.
	ServerRecvStateStartRound();

	static std::list<PlayerInterface *> GetActivePlayers(Game &curGame);
};

// State: Wait for a player action.
class ServerRecvStateWaitPlayerAction : public ServerRecvStateReceiving, public ServerRecvStateRunning, public ServerRecvStateTimer
{
public:
	// Access the state singleton.
	static ServerRecvStateWaitPlayerAction &Instance();

	virtual ~ServerRecvStateWaitPlayerAction();

	// Handle leaving players.
	virtual int Process(ServerRecvThread &server);

protected:

	// Protected constructor - this is a singleton.
	ServerRecvStateWaitPlayerAction();

	virtual int InternalProcess(ServerRecvThread &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet);

	static void PerformPlayerAction(ServerRecvThread &server, PlayerInterface *player, PlayerAction action, int bet);
	static void SendPlayerAction(ServerRecvThread &server, PlayerInterface *player);
	static int GetHighestSet(Game &curGame);
	static void SetHighestSet(Game &curGame, int highestSet);
};

// State: Delay after dealing cards
class ServerRecvStateDealCardsDelay : public ServerRecvStateReceiving, public ServerRecvStateRunning, public ServerRecvStateTimer
{
public:
	// Access the state singleton.
	static ServerRecvStateDealCardsDelay &Instance();

	virtual ~ServerRecvStateDealCardsDelay();

	// Overwrite default processing
	virtual int Process(ServerRecvThread &server);

protected:

	// Protected constructor - this is a singleton.
	ServerRecvStateDealCardsDelay();

	virtual int InternalProcess(ServerRecvThread &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet);
};

// State: Delay after showing cards (all in)
class ServerRecvStateShowCardsDelay : public ServerRecvStateReceiving, public ServerRecvStateRunning, public ServerRecvStateTimer
{
public:
	// Access the state singleton.
	static ServerRecvStateShowCardsDelay &Instance();

	virtual ~ServerRecvStateShowCardsDelay();

	// Overwrite default processing
	virtual int Process(ServerRecvThread &server);

protected:

	// Protected constructor - this is a singleton.
	ServerRecvStateShowCardsDelay();

	virtual int InternalProcess(ServerRecvThread &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet);
};

// State: Delay before next hand.
class ServerRecvStateNextHandDelay : public ServerRecvStateReceiving, public ServerRecvStateRunning, public ServerRecvStateTimer
{
public:
	// Access the state singleton.
	static ServerRecvStateNextHandDelay &Instance();

	virtual ~ServerRecvStateNextHandDelay();

	// Overwrite default processing
	virtual int Process(ServerRecvThread &server);

protected:

	// Protected constructor - this is a singleton.
	ServerRecvStateNextHandDelay();

	virtual int InternalProcess(ServerRecvThread &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet);
};

// State: Delay before next hand.
class ServerRecvStateNextGameDelay : public ServerRecvStateReceiving, public ServerRecvStateRunning, public ServerRecvStateTimer
{
public:
	// Access the state singleton.
	static ServerRecvStateNextGameDelay &Instance();

	virtual ~ServerRecvStateNextGameDelay();

	// Overwrite default processing
	virtual int Process(ServerRecvThread &server);

protected:

	// Protected constructor - this is a singleton.
	ServerRecvStateNextGameDelay();

	virtual int InternalProcess(ServerRecvThread &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet);
};

// State: Final.
class ServerRecvStateFinal : public ServerRecvStateReceiving, public ServerRecvStateRunning
{
public:
	// Access the state singleton.
	static ServerRecvStateFinal &Instance();

	virtual ~ServerRecvStateFinal();

	virtual void Init() {}

protected:

	// Protected constructor - this is a singleton.
	ServerRecvStateFinal();

	virtual int InternalProcess(ServerRecvThread &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet);
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
