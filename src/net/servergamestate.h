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

#include <net/servergamethread.h>
#include <playerdata.h>


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
	ServerGameState(ServerGameThread &serverGame) : m_serverGame(serverGame) {}
	virtual ~ServerGameState();

	virtual void NotifyGameAdminChanged() = 0;

	// Handling of a new session.
	virtual void HandleNewSession(SessionWrapper session) = 0;

	// Main processing function of the current state.
	virtual int ProcessPacket(SessionWrapper session, boost::shared_ptr<NetPacket> packet) = 0;

protected:
	ServerGameThread &GetServerGame() {return m_serverGame;}

private:
	ServerGameThread &m_serverGame;
};

// Abstract State: Receiving.
class AbstractServerGameStateReceiving : public ServerGameState
{
public:
	AbstractServerGameStateReceiving(ServerGameThread &serverGame);
	virtual ~AbstractServerGameStateReceiving();

	// Globally handle packets which are allowed in all running states.
	// Calls InternalProcess if packet has not been processed.
	virtual int ProcessPacket(SessionWrapper session, boost::shared_ptr<NetPacket> packet);

protected:

	virtual int InternalProcessPacket(SessionWrapper session, boost::shared_ptr<NetPacket> packet) = 0;
};

// State: Initialization.
class ServerGameStateInit : public AbstractServerGameStateReceiving
{
public:
	ServerGameStateInit(ServerGameThread &serverGame);
	virtual ~ServerGameStateInit();

	virtual void NotifyGameAdminChanged();

	virtual void HandleNewSession(SessionWrapper session);

protected:

	void RegisterAdminTimers();
	void UnregisterAdminTimers();
	void TimerAdminWarning();
	void TimerAdminTimeout();

	virtual int InternalProcessPacket(SessionWrapper session, boost::shared_ptr<NetPacket> packet);

	static boost::shared_ptr<NetPacket> CreateNetPacketPlayerJoined(const PlayerData &playerData);

private:

	unsigned	m_adminWarningTimerId;
	unsigned	m_adminTimeoutTimerId;
	bool		m_timerRegistered;
};

// Wait for Ack of start event and start game.
class ServerGameStateStartGame : public AbstractServerGameStateReceiving
{
public:

	ServerGameStateStartGame(ServerGameThread &serverGame);
	virtual ~ServerGameStateStartGame();

	virtual void NotifyGameAdminChanged() {}
	virtual void HandleNewSession(SessionWrapper session); 

protected:

	virtual int InternalProcessPacket(SessionWrapper session, boost::shared_ptr<NetPacket> packet);
	void TimerTimeout();
	void DoStart();

private:
	unsigned	m_timeoutTimerId;
};

// State: Within hand.
class ServerGameStateHand : public AbstractServerGameStateReceiving
{
public:
	ServerGameStateHand(ServerGameThread &serverGame);
	virtual ~ServerGameStateHand();

	virtual void NotifyGameAdminChanged() {}
	virtual void HandleNewSession(SessionWrapper session);

protected:
	virtual int InternalProcessPacket(SessionWrapper session, boost::shared_ptr<NetPacket> packet);
	void TimerLoop();
	void TimerShowCards();
	void TimerComputerAction();
	void TimerNextHand();
	void TimerNextGame();
	int GetDealCardsDelaySec();

private:
	unsigned	m_loopTimerId;
};

// State: Wait for a player action.
class ServerGameStateWaitPlayerAction : public AbstractServerGameStateReceiving
{
public:
	ServerGameStateWaitPlayerAction(ServerGameThread &serverGame);
	virtual ~ServerGameStateWaitPlayerAction();

	virtual void NotifyGameAdminChanged() {}
	virtual void HandleNewSession(SessionWrapper session);

protected:

	virtual int InternalProcessPacket(SessionWrapper session, boost::shared_ptr<NetPacket> packet);
	void TimerTimeout();

private:
	unsigned	m_timeoutTimerId;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
