/***************************************************************************
 *   Copyright (C) 2007-2010 by Lothar May                                 *
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

#include <net/socket_helper.h>
#include <net/servermanager.h>
#include <net/serverlobbythread.h>
#include <net/serveraccepthelper.h>
#include <net/serverexception.h>
#include <net/socket_msg.h>
#include <net/socket_startup.h>
#include <core/loghelper.h>

#include <boost/bind.hpp>
#include <boost/algorithm/string/predicate.hpp>

using namespace std;

ServerManager::ServerManager(GuiInterface &gui, ConfigFile &config, AvatarManager &avatarManager)
	: m_gui(gui), m_playerConfig(config), m_avatarManager(avatarManager)
{
	m_ioService.reset(new boost::asio::io_service);
	m_adminBot.reset(new ServerAdminBot);
	m_lobbyBot.reset(new ServerLobbyBot);
}

ServerManager::~ServerManager()
{
}

void
ServerManager::Init(unsigned serverPort, bool ipv6, ServerTransportProtocol proto, ServerMode mode, const string &logDir, boost::shared_ptr<IrcThread> ircAdminThread, boost::shared_ptr<IrcThread> ircLobbyThread)
{
	m_lobbyThread.reset(new ServerLobbyThread(GetGui(), mode, *m_lobbyBot, m_playerConfig, m_avatarManager, m_ioService));
	GetLobbyThread().Init(logDir);

	m_adminBot->Init(m_lobbyThread, ircAdminThread);
	m_lobbyBot->Init(m_lobbyThread, ircLobbyThread);

	if (proto & TRANSPORT_PROTOCOL_TCP) {
		boost::shared_ptr<ServerAcceptHelper> tcpAcceptHelper(new ServerAcceptHelper(GetGui(), m_ioService));
		tcpAcceptHelper->Listen(serverPort, ipv6, false, logDir, m_lobbyThread);
		m_acceptHelperPool.push_back(tcpAcceptHelper);
	}
	// TODO: Re-add SCTP support once asio supports SCTP.
	/*	if (mode & TRANSPORT_PROTOCOL_SCTP)
		{
			boost::shared_ptr<ServerAcceptHelper> sctpAcceptHelper(new ServerAcceptHelper(GetGui(), m_ioService));
			sctpAcceptHelper->Listen(serverPort, ipv6, true, logDir, m_lobbyThread);
			m_acceptHelperPool.push_back(sctpAcceptHelper);
		}*/
}

GuiInterface &
ServerManager::GetGui()
{
	return m_gui;
}

ServerAdminBot &
ServerManager::GetAdminBot()
{
	return *m_adminBot;
}

ServerLobbyBot &
ServerManager::GetLobbyBot()
{
	return *m_lobbyBot;
}

void
ServerManager::RunAll()
{
	m_adminBot->Run();
	m_lobbyBot->Run();
	GetLobbyThread().Run();
}

void
ServerManager::Process()
{
	m_adminBot->Process();
	m_lobbyBot->Process();
}

void
ServerManager::SignalTerminationAll()
{
	m_adminBot->SignalTermination();
	m_lobbyBot->SignalTermination();
	GetLobbyThread().SignalTermination();
}

bool
ServerManager::JoinAll(bool wait)
{
	m_adminBot->Join(wait);
	m_lobbyBot->Join(wait);
	return GetLobbyThread().Join(wait ? NET_LOBBY_THREAD_TERMINATE_TIMEOUT_MSEC : 0);
}

ServerLobbyThread &
ServerManager::GetLobbyThread()
{
	assert(m_lobbyThread);
	return *m_lobbyThread;
}

