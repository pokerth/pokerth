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
/* Manager thread for the server. */

#ifndef _SERVERMANAGER_H_
#define _SERVERMANAGER_H_

#include <boost/asio.hpp>
#include <net/serveradminbot.h>
#include <net/serverlobbybot.h>
#include <string>
#include <list>

#include <game_defs.h>
#include <gui/guiinterface.h>

class ServerLobbyThread;
class IrcThread;
class ServerAcceptInterface;
class SenderThread;
class ConfigFile;
class AvatarManager;

class ServerManager
{
public:
	ServerManager(GuiInterface &gui, ConfigFile &config, AvatarManager &avatarManager);
	virtual ~ServerManager();

	// Set the parameters.
	void Init(unsigned serverPort, bool ipv6, ServerTransportProtocol proto, ServerMode mode, const std::string &logDir, boost::shared_ptr<IrcThread> ircAdminThread, boost::shared_ptr<IrcThread> ircLobbyThread);

	// Main start function.
	void RunAll();

	// Let the server manager perform processing.
	void Process();

	void SignalTerminationAll();
	bool JoinAll(bool wait);

	GuiInterface &GetGui();
	ServerAdminBot &GetAdminBot();
	ServerLobbyBot &GetLobbyBot();

protected:
	typedef std::list<boost::shared_ptr<ServerAcceptInterface> > AcceptHelperList;

	ServerLobbyThread &GetLobbyThread();

private:
	GuiInterface &m_gui;
	ConfigFile &m_playerConfig;
	AvatarManager &m_avatarManager;

	boost::shared_ptr<boost::asio::io_service> m_ioService;
	boost::shared_ptr<ServerLobbyThread> m_lobbyThread;
	boost::shared_ptr<ServerAdminBot> m_adminBot;
	boost::shared_ptr<ServerLobbyBot> m_lobbyBot;
	AcceptHelperList m_acceptHelperPool;
};

#endif
