/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2011 Felix Hammer, Florian Thauer, Lothar May          *
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
 *****************************************************************************/
/* Manager thread for the server. */

#ifndef _SERVERMANAGER_H_
#define _SERVERMANAGER_H_

#include <boost/asio.hpp>
#include <string>
#include <list>

#include <game_defs.h>
#include <gui/guiinterface.h>

class ServerLobbyThread;
class ServerAcceptInterface;
class SenderThread;
class ConfigFile;
class AvatarManager;

class ServerManager
{
public:
	ServerManager(ConfigFile &config, GuiInterface &gui);
	ServerManager(ConfigFile &config, GuiInterface &gui, ServerMode mode, AvatarManager &avatarManager);
	virtual ~ServerManager();

	// Set the parameters.
	virtual void Init(unsigned serverPort, bool ipv6, ServerTransportProtocol proto, const std::string &logDir);

	// Main start function.
	virtual void RunAll();

	virtual void SignalTerminationAll();
	virtual bool JoinAll(bool wait);

protected:
	typedef std::list<boost::shared_ptr<ServerAcceptInterface> > AcceptHelperList;

	ServerLobbyThread &GetLobbyThread();
	ConfigFile &GetConfig() {
		return m_playerConfig;
	}
	GuiInterface &GetGui() {
		return m_gui;
	}
	boost::shared_ptr<boost::asio::io_service> m_ioService;
	boost::shared_ptr<ServerLobbyThread> m_lobbyThread;

private:
	ConfigFile &m_playerConfig;
	GuiInterface &m_gui;

	AcceptHelperList m_acceptHelperPool;
};

#endif
