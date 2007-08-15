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
/* Network server thread to accept connections. */

#ifndef _SERVERACCEPTTHREAD_H_
#define _SERVERACCEPTTHREAD_H_

#include <game_defs.h>
#include <core/thread.h>
#include <gui/guiinterface.h>
#include <string>
#include <memory>

class ServerContext;
class ServerLobbyThread;
class ServerSenderCallback;
class SenderThread;
class ConfigFile;
struct GameData;

class ServerAcceptThread : public Thread
{
public:
	ServerAcceptThread(GuiInterface &gui, ConfigFile *config);
	virtual ~ServerAcceptThread();

	// Set the parameters.
	void Init(unsigned serverPort, bool ipv6, bool sctp, const std::string &pwd,
		const GameData &gameData);

	ServerCallback &GetCallback();
	GuiInterface &GetGui();

protected:

	// Main function of the thread.
	virtual void Main();

	void Listen();
	void AcceptLoop();

	const ServerContext &GetContext() const;
	ServerContext &GetContext();

	ServerLobbyThread &GetLobbyThread();

private:
	std::auto_ptr<ServerContext> m_context;
	std::auto_ptr<ServerLobbyThread> m_lobbyThread;

	GuiInterface &m_gui;
};

#endif
