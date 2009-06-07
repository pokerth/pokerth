/***************************************************************************
 *   Copyright (C) 2007-2009 by Lothar May                                 *
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
/* State of a network game. */

#ifndef _SERVERGAMESTATE_H_
#define _SERVERGAMESTATE_H_

#include <playerdata.h>
#include <net/sessionmanager.h>


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4250)
#endif

#define SERVER_INITIAL_STATE	ServerGameStateInit

class Game;
class ServerGame;
class PlayerInterface;
class ServerCallback;

class ServerGameState
{
public:
	virtual ~ServerGameState();
	virtual void Enter(ServerGame &server) = 0;
	virtual void Exit(ServerGame &server) = 0;

	virtual void NotifyGameAdminChanged(ServerGame &server) = 0;

	// Handling of a new session.
	virtual void HandleNewSession(ServerGame &server, SessionWrapper session) = 0;

	// Main processing function of the current state.
	virtual int ProcessPacket(ServerGame &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet) = 0;
};

// Abstract State: Receiving.
class AbstractServerGameStateReceiving : public ServerGameState
{
public:
	virtual ~AbstractServerGameStateReceiving();

	// Globally handle packets which are allowed in all running states.
	// Calls InternalProcess if packet has not been processed.
	virtual int ProcessPacket(ServerGame &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet);

protected:

	virtual int InternalProcessPacket(ServerGame &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet) = 0;
};

// State: Initialization.
class ServerGameStateInit : public AbstractServerGameStateReceiving
{
public:
	static ServerGameStateInit &Instance();
	virtual void Enter(ServerGame &server);
	virtual void Exit(ServerGame &server);

	virtual ~ServerGameStateInit();

	virtual void NotifyGameAdminChanged(ServerGame &server);

	virtual void HandleNewSession(ServerGame &server, SessionWrapper session);

protected:
	ServerGameStateInit();

	void RegisterAdminTimer(ServerGame &server);
	void UnregisterAdminTimer(ServerGame &server);
	void TimerAdminWarning(ServerGame &server);
	void TimerAdminTimeout(ServerGame &server);

	virtual int InternalProcessPacket(ServerGame &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet);

	static boost::shared_ptr<NetPacket> CreateNetPacketPlayerJoined(const PlayerData &playerData);

private:
	static ServerGameStateInit s_state;
};

// Wait for Ack of start event and start game.
class ServerGameStateStartGame : public AbstractServerGameStateReceiving
{
public:
	static ServerGameStateStartGame &Instance();
	virtual void Enter(ServerGame &server);
	virtual void Exit(ServerGame &server);

	virtual ~ServerGameStateStartGame();

	virtual void NotifyGameAdminChanged(ServerGame &/*server*/) {}
	virtual void HandleNewSession(ServerGame &server, SessionWrapper session);

protected:
	ServerGameStateStartGame();

	virtual int InternalProcessPacket(ServerGame &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet);
	void TimerTimeout(ServerGame &server);
	void DoStart(ServerGame &server);

private:
	static ServerGameStateStartGame s_state;
};

// State: Within hand.
class ServerGameStateHand : public AbstractServerGameStateReceiving
{
public:
	static ServerGameStateHand &Instance();
	virtual void Enter(ServerGame &server);
	virtual void Exit(ServerGame &server);

	virtual ~ServerGameStateHand();

	virtual void NotifyGameAdminChanged(ServerGame &/*server*/) {}
	virtual void HandleNewSession(ServerGame &server, SessionWrapper session);

protected:
	ServerGameStateHand();

	virtual int InternalProcessPacket(ServerGame &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet);
	void TimerLoop(ServerGame &server);
	void TimerShowCards(ServerGame &server);
	void TimerComputerAction(ServerGame &server);
	void TimerNextHand(ServerGame &server);
	void TimerNextGame(ServerGame &server);
	int GetDealCardsDelaySec(ServerGame &server);
	static void StartNewHand(ServerGame &server);

private:
	static ServerGameStateHand s_state;

friend class ServerGameStateStartGame;
};

// State: Wait for a player action.
class ServerGameStateWaitPlayerAction : public AbstractServerGameStateReceiving
{
public:
	static ServerGameStateWaitPlayerAction &Instance();
	virtual void Enter(ServerGame &server);
	virtual void Exit(ServerGame &server);

	virtual ~ServerGameStateWaitPlayerAction();

	virtual void NotifyGameAdminChanged(ServerGame &/*server*/) {}
	virtual void HandleNewSession(ServerGame &server, SessionWrapper session);

protected:
	ServerGameStateWaitPlayerAction();

	virtual int InternalProcessPacket(ServerGame &server, SessionWrapper session, boost::shared_ptr<NetPacket> packet);
	void TimerTimeout(ServerGame &server);

private:
	static ServerGameStateWaitPlayerAction s_state;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
