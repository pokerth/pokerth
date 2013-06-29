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
/* State of a network game. */

#ifndef _SERVERGAMESTATE_H_
#define _SERVERGAMESTATE_H_

#include <boost/asio.hpp>
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
	virtual void Enter(boost::shared_ptr<ServerGame> server) = 0;
	virtual void Exit(boost::shared_ptr<ServerGame> server) = 0;

	virtual void NotifyGameAdminChanged(boost::shared_ptr<ServerGame> server) = 0;
	virtual void NotifySessionRemoved(boost::shared_ptr<ServerGame> server) = 0;

	// Handling of a new player session.
	virtual void HandleNewPlayer(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session) = 0;
	// Handling of a new spectator session.
	virtual void HandleNewSpectator(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session) = 0;

	// Main processing function of the current state.
	virtual void ProcessPacket(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet) = 0;
};

// Abstract State: Receiving.
class AbstractServerGameStateReceiving : public ServerGameState
{
public:
	virtual ~AbstractServerGameStateReceiving();

	// Globally handle packets which are allowed in all running states.
	// Calls InternalProcess if packet has not been processed.
	virtual void ProcessPacket(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet);

	static boost::shared_ptr<NetPacket> CreateNetPacketPlayerJoined(unsigned gameId, const PlayerData &playerData);
	static boost::shared_ptr<NetPacket> CreateNetPacketSpectatorJoined(unsigned gameId, const PlayerData &playerData);
	static boost::shared_ptr<NetPacket> CreateNetPacketJoinGameAck(const ServerGame &server, const PlayerData &playerData, bool spectateOnly);
	static boost::shared_ptr<NetPacket> CreateNetPacketHandStart(const ServerGame &server);

	static void AcceptNewSession(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session, bool spectateOnly);

protected:

	virtual void InternalProcessPacket(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet) = 0;
};

// State: Initialization.
class ServerGameStateInit : public AbstractServerGameStateReceiving
{
public:
	static ServerGameStateInit &Instance();
	virtual void Enter(boost::shared_ptr<ServerGame> server);
	virtual void Exit(boost::shared_ptr<ServerGame> server);

	virtual ~ServerGameStateInit();

	virtual void NotifyGameAdminChanged(boost::shared_ptr<ServerGame> server);
	virtual void NotifySessionRemoved(boost::shared_ptr<ServerGame> server);

	virtual void HandleNewPlayer(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session);
	virtual void HandleNewSpectator(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session);

protected:
	ServerGameStateInit();

	void RegisterAdminTimer(boost::shared_ptr<ServerGame> server);
	void UnregisterAdminTimer(boost::shared_ptr<ServerGame> server);
	void RegisterAutoStartTimer(boost::shared_ptr<ServerGame> server);
	void UnregisterAutoStartTimer(boost::shared_ptr<ServerGame> server);
	void TimerAutoStart(const boost::system::error_code &ec, boost::shared_ptr<ServerGame> server);
	void TimerAdminWarning(const boost::system::error_code &ec, boost::shared_ptr<ServerGame> server);
	void TimerAdminTimeout(const boost::system::error_code &ec, boost::shared_ptr<ServerGame> server);
	void SendStartEvent(ServerGame &server, bool fillWithComputerPlayers);

	virtual void InternalProcessPacket(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet);

private:
	static ServerGameStateInit s_state;
};

// Wait for Ack of start event and start game.
class ServerGameStateStartGame : public AbstractServerGameStateReceiving
{
public:
	static ServerGameStateStartGame &Instance();
	virtual void Enter(boost::shared_ptr<ServerGame> server);
	virtual void Exit(boost::shared_ptr<ServerGame> server);

	virtual ~ServerGameStateStartGame();

	virtual void NotifyGameAdminChanged(boost::shared_ptr<ServerGame> /*server*/) {}
	virtual void NotifySessionRemoved(boost::shared_ptr<ServerGame> /*server*/) {}
	virtual void HandleNewPlayer(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session);
	virtual void HandleNewSpectator(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session);

protected:
	ServerGameStateStartGame();

	virtual void InternalProcessPacket(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet);
	void TimerTimeout(const boost::system::error_code &ec, boost::shared_ptr<ServerGame> server);
	void DoStart(boost::shared_ptr<ServerGame> server);

private:
	static ServerGameStateStartGame s_state;
};

class AbstractServerGameStateRunning : public AbstractServerGameStateReceiving
{
public:
	virtual ~AbstractServerGameStateRunning();

	virtual void HandleNewPlayer(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session);
	virtual void HandleNewSpectator(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session);

protected:
	virtual void InternalProcessPacket(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet);
};

// State: Within hand.
class ServerGameStateHand : public AbstractServerGameStateRunning
{
public:
	static ServerGameStateHand &Instance();
	virtual void Enter(boost::shared_ptr<ServerGame> server);
	virtual void Exit(boost::shared_ptr<ServerGame> server);

	virtual ~ServerGameStateHand();

	virtual void NotifyGameAdminChanged(boost::shared_ptr<ServerGame> /*server*/) {}
	virtual void NotifySessionRemoved(boost::shared_ptr<ServerGame> /*server*/) {}

protected:
	ServerGameStateHand();

	virtual void InternalProcessPacket(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet);
	void TimerLoop(const boost::system::error_code &ec, boost::shared_ptr<ServerGame> server);
	void EngineLoop(boost::shared_ptr<ServerGame> server);
	void TimerShowCards(const boost::system::error_code &ec, boost::shared_ptr<ServerGame> server);
	void TimerComputerAction(const boost::system::error_code &ec, boost::shared_ptr<ServerGame> server);
	void TimerNextHand(const boost::system::error_code &ec, boost::shared_ptr<ServerGame> server);
	void TimerNextGame(const boost::system::error_code &ec, boost::shared_ptr<ServerGame> server, unsigned winnerPlayerId);
	int GetDealCardsDelaySec(ServerGame &server);
	static void StartNewHand(boost::shared_ptr<ServerGame> server);
	static void CheckPlayerTimeouts(boost::shared_ptr<ServerGame> server);
	static void ReactivatePlayers(boost::shared_ptr<ServerGame> server);
	static void InitRejoiningPlayers(boost::shared_ptr<ServerGame> server);
	static void InitNewSpectators(boost::shared_ptr<ServerGame> server);
	static void PerformRejoin(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session);
	static void SendGameData(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session);

private:
	static ServerGameStateHand s_state;

	friend class ServerGameStateStartGame;
	friend class ServerGameStateWaitNextHand;
};

// State: Wait for a player action.
class ServerGameStateWaitPlayerAction : public AbstractServerGameStateRunning
{
public:
	static ServerGameStateWaitPlayerAction &Instance();
	virtual void Enter(boost::shared_ptr<ServerGame> server);
	virtual void Exit(boost::shared_ptr<ServerGame> server);

	virtual ~ServerGameStateWaitPlayerAction();

	virtual void NotifyGameAdminChanged(boost::shared_ptr<ServerGame> /*server*/) {}
	virtual void NotifySessionRemoved(boost::shared_ptr<ServerGame> /*server*/) {}

protected:
	ServerGameStateWaitPlayerAction();

	virtual void InternalProcessPacket(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet);
	void TimerTimeout(const boost::system::error_code &ec, boost::shared_ptr<ServerGame> server);

private:
	static ServerGameStateWaitPlayerAction s_state;
};

// State: Wait for the next hand.
class ServerGameStateWaitNextHand : public AbstractServerGameStateRunning
{
public:
	static ServerGameStateWaitNextHand &Instance();
	virtual void Enter(boost::shared_ptr<ServerGame> server);
	virtual void Exit(boost::shared_ptr<ServerGame> server);

	virtual ~ServerGameStateWaitNextHand();

	virtual void NotifyGameAdminChanged(boost::shared_ptr<ServerGame> /*server*/) {}
	virtual void NotifySessionRemoved(boost::shared_ptr<ServerGame> /*server*/) {}

protected:
	ServerGameStateWaitNextHand();

	virtual void InternalProcessPacket(boost::shared_ptr<ServerGame> server, boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet);
	void TimerTimeout(const boost::system::error_code &ec, boost::shared_ptr<ServerGame> server);

private:
	static ServerGameStateWaitNextHand s_state;
};

class ServerGameStateFinal : public ServerGameState
{
public:
	static ServerGameStateFinal &Instance();

	virtual ~ServerGameStateFinal() {}
	virtual void Enter(boost::shared_ptr<ServerGame> /*server*/) {}
	virtual void Exit(boost::shared_ptr<ServerGame> /*server*/) {}

	virtual void NotifyGameAdminChanged(boost::shared_ptr<ServerGame> /*server*/) {}
	virtual void NotifySessionRemoved(boost::shared_ptr<ServerGame> /*server*/) {}

	// Handling of a new session.
	virtual void HandleNewPlayer(boost::shared_ptr<ServerGame> /*server*/, boost::shared_ptr<SessionData> /*session*/) {}
	virtual void HandleNewSpectator(boost::shared_ptr<ServerGame> /*server*/, boost::shared_ptr<SessionData> /*session*/) {}

	// Main processing function of the current state.
	virtual void ProcessPacket(boost::shared_ptr<ServerGame> /*server*/, boost::shared_ptr<SessionData> /*session*/, boost::shared_ptr<NetPacket> /*packet*/) {}

protected:
	ServerGameStateFinal() {}

private:
	static ServerGameStateFinal s_state;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
