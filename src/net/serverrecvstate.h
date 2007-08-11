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
class AbstractServerRecvStateReceiving : virtual public ServerRecvState
{
public:
	virtual ~AbstractServerRecvStateReceiving();

	// Globally handle packets which are allowed in all running states.
	// Calls InternalProcess if packet has not been processed.
	virtual int Process(ServerRecvThread &server);

protected:

	AbstractServerRecvStateReceiving();

	virtual int InternalProcess(ServerRecvThread &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet) = 0;
};

// Abstract State: Timer.
class AbstractServerRecvStateTimer : virtual public ServerRecvState
{
public:
	virtual ~AbstractServerRecvStateTimer();

	virtual void Init();

	const boost::microsec_timer &GetTimer() const {return m_timer;}

protected:

	AbstractServerRecvStateTimer();

private:
	boost::microsec_timer m_timer;
};

// Abstract State: Game is running.
class AbstractServerRecvStateRunning : virtual public ServerRecvState
{
public:
	virtual ~AbstractServerRecvStateRunning();

	// Reject new connections.
	virtual void HandleNewConnection(ServerRecvThread &server, boost::shared_ptr<ConnectData> data);

protected:

	AbstractServerRecvStateRunning();
};

// State: Initialization.
class ServerRecvStateInit : public AbstractServerRecvStateReceiving
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

	static boost::shared_ptr<NetPacket> CreateNetPacketPlayerJoined(const PlayerData &playerData);

private:

	u_int16_t m_curUniquePlayerId;
	static boost::thread_specific_ptr<ServerRecvStateInit>	Ptr;
};

// State: Start server game.
class ServerRecvStateStartGame : public AbstractServerRecvStateRunning
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

private:

	static boost::thread_specific_ptr<ServerRecvStateStartGame>	Ptr;
};

// State: Start new hand.
class ServerRecvStateStartHand : public AbstractServerRecvStateRunning
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

private:

	static boost::thread_specific_ptr<ServerRecvStateStartHand>	Ptr;
};

// State: Start new round.
class ServerRecvStateStartRound : public AbstractServerRecvStateRunning
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

	static std::list<boost::shared_ptr<PlayerInterface> > GetActivePlayers(Game &curGame);

private:

	static boost::thread_specific_ptr<ServerRecvStateStartRound>	Ptr;
};

// State: Wait for a player action.
class ServerRecvStateWaitPlayerAction : public AbstractServerRecvStateReceiving, public AbstractServerRecvStateRunning, public AbstractServerRecvStateTimer
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

	static void PerformPlayerAction(ServerRecvThread &server, boost::shared_ptr<PlayerInterface> player, PlayerAction action, int bet);

private:

	static boost::thread_specific_ptr<ServerRecvStateWaitPlayerAction>	Ptr;
};

// State: Delay and computer action
class ServerRecvStateComputerAction : public AbstractServerRecvStateReceiving, public AbstractServerRecvStateRunning, public AbstractServerRecvStateTimer
{
public:
	// Access the state singleton.
	static ServerRecvStateComputerAction &Instance();

	virtual ~ServerRecvStateComputerAction();

	// Overwrite default processing
	virtual int Process(ServerRecvThread &server);

protected:

	// Protected constructor - this is a singleton.
	ServerRecvStateComputerAction();

	virtual int InternalProcess(ServerRecvThread &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet);

private:

	static boost::thread_specific_ptr<ServerRecvStateComputerAction>	Ptr;
};

// State: Delay after dealing cards
class ServerRecvStateDealCardsDelay : public AbstractServerRecvStateReceiving, public AbstractServerRecvStateRunning, public AbstractServerRecvStateTimer
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

private:

	static boost::thread_specific_ptr<ServerRecvStateDealCardsDelay>	Ptr;
};

// State: Delay after showing cards (all in)
class ServerRecvStateShowCardsDelay : public AbstractServerRecvStateReceiving, public AbstractServerRecvStateRunning, public AbstractServerRecvStateTimer
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

private:

	static boost::thread_specific_ptr<ServerRecvStateShowCardsDelay>	Ptr;
};

// State: Delay before next hand.
class ServerRecvStateNextHandDelay : public AbstractServerRecvStateReceiving, public AbstractServerRecvStateRunning, public AbstractServerRecvStateTimer
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

private:

	static boost::thread_specific_ptr<ServerRecvStateNextHandDelay>	Ptr;
};

// State: Delay before next hand.
class ServerRecvStateNextGameDelay : public AbstractServerRecvStateReceiving, public AbstractServerRecvStateRunning, public AbstractServerRecvStateTimer
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

private:

	static boost::thread_specific_ptr<ServerRecvStateNextGameDelay>	Ptr;
};

// State: Final.
class ServerRecvStateFinal : public AbstractServerRecvStateReceiving, public AbstractServerRecvStateRunning
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

private:

	static boost::thread_specific_ptr<ServerRecvStateFinal>	Ptr;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
