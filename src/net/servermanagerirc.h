/***************************************************************************
 *   Copyright (C) 2011 by Lothar May                                      *
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
/* Manager thread for the server, with irc bots. */

#ifndef _SERVERMANAGERIRC_H_
#define _SERVERMANAGERIRC_H_

#include <net/servermanager.h>

class ServerAdminBot;
class ServerLobbyBot;

class ServerManagerIrc : public ServerManager
{
public:
	ServerManagerIrc(ConfigFile &config, GuiInterface &gui, ServerMode mode, AvatarManager &avatarManager);
	virtual ~ServerManagerIrc();

	// Set the parameters.
	virtual void Init(unsigned serverPort, bool ipv6, ServerTransportProtocol proto, const std::string &logDir);

	// Main start function.
	virtual void RunAll();

	// Let the server manager perform processing.
	virtual void Process();

	virtual void SignalTerminationAll();
	virtual bool JoinAll(bool wait);

	GuiInterface &GetGui();

private:

	boost::shared_ptr<ServerAdminBot> m_adminBot;
	boost::shared_ptr<ServerLobbyBot> m_lobbyBot;
};

#endif
