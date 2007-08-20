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

#ifndef _SERVERGAMESTATE_H_
#define _SERVERGAMESTATE_H_

#include <net/servergamethread.h>
#include <playerdata.h>
#include <core/boost/timer.hpp>


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4250)
#endif

#define SERVER_INITIAL_STATE	ServerGameStateInit
#define SERVER_START_GAME_STATE	ServerGameStateStartGame

class Game;
class PlayerInterface;
class ServerCallback;

class ServerGameState
{
public:
	virtual ~ServerGameState();

	// Initialize after switching to this state.
	virtual void Init() = 0;

	// Handling of a new session.
	virtual void HandleNewSession(ServerGameThread &server, SessionWrapper session) = 0;

	// Main processing function of the current state.
	virtual int Process(ServerGameThread &server) = 0;
};

// Abstract State: Receiving.
class AbstractServerGameStateReceiving : virtual public ServerGameState
{
public:
	virtual ~AbstractServerGameStateReceiving();

	// Globally handle packets which are allowed in all running states.
	// Calls InternalProcess if packet has not been processed.
	virtual int Process(ServerGameThread &server);

protected:

	AbstractServerGameStateReceiving();

	virtual int InternalProcess(ServerGameThread &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet) = 0;
};

// Abstract State: Timer.
class AbstractServerGameStateTimer : virtual public ServerGameState
{
public:
	virtual ~AbstractServerGameStateTimer();

	virtual void Init();

	const boost::microsec_timer &GetTimer() const {return m_timer;}

protected:

	AbstractServerGameStateTimer();

private:
	boost::microsec_timer m_timer;
};

// Abstract State: Game is running.
class AbstractServerGameStateRunning : virtual public ServerGameState
{
public:
	virtual ~AbstractServerGameStateRunning();

	// Reject new connections.
	virtual void HandleNewSession(ServerGameThread &server, SessionWrapper session);

protected:

	AbstractServerGameStateRunning();
};

// State: Initialization.
class ServerGameStateInit : public AbstractServerGameStateReceiving
{
public:
	// Access the state singleton.
	static ServerGameStateInit &Instance();

	virtual ~ServerGameStateInit();

	virtual void Init() {}
	// 
	virtual void HandleNewSession(ServerGameThread &server, SessionWrapper session);

protected:

	// Protected constructor - this is a singleton.
	ServerGameStateInit();

	virtual int InternalProcess(ServerGameThread &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet);

	static boost::shared_ptr<NetPacket> CreateNetPacketPlayerJoined(const PlayerData &playerData);

private:

	u_int32_t m_curUniquePlayerId;
	static boost::thread_specific_ptr<ServerGameStateInit>	Ptr;
};

// State: Start server game.
class ServerGameStateStartGame : public AbstractServerGameStateRunning
{
public:
	// Access the state singleton.
	static ServerGameStateStartGame &Instance();

	virtual ~ServerGameStateStartGame();

	virtual void Init() {}

	// 
	virtual int Process(ServerGameThread &server);

protected:

	// Protected constructor - this is a singleton.
	ServerGameStateStartGame();

private:

	static boost::thread_specific_ptr<ServerGameStateStartGame>	Ptr;
};

// State: Start new hand.
class ServerGameStateStartHand : public AbstractServerGameStateRunning
{
public:
	// Access the state singleton.
	static ServerGameStateStartHand &Instance();

	virtual ~ServerGameStateStartHand();

	virtual void Init() {}

	// 
	virtual int Process(ServerGameThread &server);

protected:

	// Protected constructor - this is a singleton.
	ServerGameStateStartHand();

private:

	static boost::thread_specific_ptr<ServerGameStateStartHand>	Ptr;
};

// State: Start new round.
class ServerGameStateStartRound : public AbstractServerGameStateRunning
{
public:
	// Access the state singleton.
	static ServerGameStateStartRound &Instance();

	virtual ~ServerGameStateStartRound();

	virtual void Init() {}

	// 
	virtual int Process(ServerGameThread &server);

protected:

	// Protected constructor - this is a singleton.
	ServerGameStateStartRound();

	static std::list<boost::shared_ptr<PlayerInterface> > GetActivePlayers(Game &curGame);

private:

	static boost::thread_specific_ptr<ServerGameStateStartRound>	Ptr;
};

// State: Wait for a player action.
class ServerGameStateWaitPlayerAction : public AbstractServerGameStateReceiving, public AbstractServerGameStateRunning, public AbstractServerGameStateTimer
{
public:
	// Access the state singleton.
	static ServerGameStateWaitPlayerAction &Instance();

	virtual ~ServerGameStateWaitPlayerAction();

	// Handle leaving players.
	virtual int Process(ServerGameThread &server);

protected:

	// Protected constructor - this is a singleton.
	ServerGameStateWaitPlayerAction();

	virtual int InternalProcess(ServerGameThread &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet);

	static void PerformPlayerAction(ServerGameThread &server, boost::shared_ptr<PlayerInterface> player, PlayerAction action, int bet);

private:

	static boost::thread_specific_ptr<ServerGameStateWaitPlayerAction>	Ptr;
};

// State: Delay and computer action
class ServerGameStateComputerAction : public AbstractServerGameStateReceiving, public AbstractServerGameStateRunning, public AbstractServerGameStateTimer
{
public:
	// Access the state singleton.
	static ServerGameStateComputerAction &Instance();

	virtual ~ServerGameStateComputerAction();

	// Overwrite default processing
	virtual int Process(ServerGameThread &server);

protected:

	// Protected constructor - this is a singleton.
	ServerGameStateComputerAction();

	virtual int InternalProcess(ServerGameThread &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet);

private:

	static boost::thread_specific_ptr<ServerGameStateComputerAction>	Ptr;
};

// State: Delay after dealing cards
class ServerGameStateDealCardsDelay : public AbstractServerGameStateReceiving, public AbstractServerGameStateRunning, public AbstractServerGameStateTimer
{
public:
	// Access the state singleton.
	static ServerGameStateDealCardsDelay &Instance();

	virtual ~ServerGameStateDealCardsDelay();

	// Overwrite default processing
	virtual int Process(ServerGameThread &server);

protected:

	// Protected constructor - this is a singleton.
	ServerGameStateDealCardsDelay();

	virtual int InternalProcess(ServerGameThread &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet);

private:

	static boost::thread_specific_ptr<ServerGameStateDealCardsDelay>	Ptr;
};

// State: Delay after showing cards (all in)
class ServerGameStateShowCardsDelay : public AbstractServerGameStateReceiving, public AbstractServerGameStateRunning, public AbstractServerGameStateTimer
{
public:
	// Access the state singleton.
	static ServerGameStateShowCardsDelay &Instance();

	virtual ~ServerGameStateShowCardsDelay();

	// Overwrite default processing
	virtual int Process(ServerGameThread &server);

protected:

	// Protected constructor - this is a singleton.
	ServerGameStateShowCardsDelay();

	virtual int InternalProcess(ServerGameThread &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet);

private:

	static boost::thread_specific_ptr<ServerGameStateShowCardsDelay>	Ptr;
};

// State: Delay before next hand.
class ServerGameStateNextHandDelay : public AbstractServerGameStateReceiving, public AbstractServerGameStateRunning, public AbstractServerGameStateTimer
{
public:
	// Access the state singleton.
	static ServerGameStateNextHandDelay &Instance();

	virtual ~ServerGameStateNextHandDelay();

	// Overwrite default processing
	virtual int Process(ServerGameThread &server);

protected:

	// Protected constructor - this is a singleton.
	ServerGameStateNextHandDelay();

	virtual int InternalProcess(ServerGameThread &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet);

private:

	static boost::thread_specific_ptr<ServerGameStateNextHandDelay>	Ptr;
};

// State: Delay before next hand.
class ServerGameStateNextGameDelay : public AbstractServerGameStateReceiving, public AbstractServerGameStateRunning, public AbstractServerGameStateTimer
{
public:
	// Access the state singleton.
	static ServerGameStateNextGameDelay &Instance();

	virtual ~ServerGameStateNextGameDelay();

	// Overwrite default processing
	virtual int Process(ServerGameThread &server);

protected:

	// Protected constructor - this is a singleton.
	ServerGameStateNextGameDelay();

	virtual int InternalProcess(ServerGameThread &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet);

private:

	static boost::thread_specific_ptr<ServerGameStateNextGameDelay>	Ptr;
};

// State: Final.
class ServerGameStateFinal : public AbstractServerGameStateReceiving, public AbstractServerGameStateRunning
{
public:
	// Access the state singleton.
	static ServerGameStateFinal &Instance();

	virtual ~ServerGameStateFinal();

	virtual void Init() {}

protected:

	// Protected constructor - this is a singleton.
	ServerGameStateFinal();

	virtual int InternalProcess(ServerGameThread &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet);

private:

	static boost::thread_specific_ptr<ServerGameStateFinal>	Ptr;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
