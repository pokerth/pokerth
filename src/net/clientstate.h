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

#include <string>
#include <memory>

#define CLIENT_INITIAL_STATE ClientStateInit

class ClientThread;
class ClientCallback;

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

// State: Resolving name.
class ClientStateResolve : public ClientState
{
public:
	// Access the state singleton.
	static ClientStateResolve &Instance();

	virtual ~ClientStateResolve();

	// "Poll" for the completion of the name resolution.
	virtual int Process(ClientThread &client);

protected:

	// Protected constructor - this is a singleton.
	ClientStateResolve();
};

// State: Connecting to server.
class ClientStateConnect : public ClientState
{
public:
	// Access the state singleton.
	static ClientStateConnect &Instance();

	virtual ~ClientStateConnect();

	// "Poll" for the completion of the TCP/IP connect call.
	virtual int Process(ClientThread &client);

protected:

	// Protected constructor - this is a singleton.
	ClientStateConnect();
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
