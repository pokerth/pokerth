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
/* Network server thread. */

#ifndef _SERVERTHREAD_H_
#define _SERVERTHREAD_H_

#include <game_defs.h>
#include <core/thread.h>
#include <string>
#include <memory>
#include <net/servercallback.h>

class ServerContext;
class ServerRecvThread;
class ServerSenderCallback;
class SenderThread;
struct GameData;

class ServerThread : public Thread
{
public:
	ServerThread(ServerCallback &gui);
	virtual ~ServerThread();

	// Set the parameters.
	void Init(unsigned serverPort, bool ipv6, const std::string &pwd,
		const GameData &gameData);
	void StartGame();
	void WaitForClientAction(GameState state, unsigned uniquePlayerId);

	ServerCallback &GetCallback();

protected:

	// Main function of the thread.
	virtual void Main();

	void Listen();
	void AcceptLoop();

	const ServerContext &GetContext() const;
	ServerContext &GetContext();

	ServerRecvThread &GetRecvThread();

private:
	std::auto_ptr<ServerContext> m_context;
	std::auto_ptr<ServerRecvThread> m_recvThread;

	ServerCallback &m_callback;
};

#endif
