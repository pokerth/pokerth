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

#include <net/socket_helper.h>
#include <net/servermanager.h>
#include <net/serverlobbythread.h>
#include <net/serverircbot.h>
#include <net/serveraccepthelper.h>
#include <net/serverexception.h>
#include <net/socket_msg.h>
#include <net/socket_startup.h>
#include <core/loghelper.h>

#include <boost/bind.hpp>
#include <boost/algorithm/string/predicate.hpp>

using namespace std;

ServerManager::ServerManager(GuiInterface &gui, ConfigFile *config, AvatarManager &avatarManager)
: m_gui(gui), m_playerConfig(config), m_avatarManager(avatarManager)
{
	m_ioService.reset(new boost::asio::io_service);
	m_ircBot.reset(new ServerIrcBot);
}

ServerManager::~ServerManager()
{
}

void
ServerManager::Init(unsigned serverPort, bool ipv6, ServerNetworkMode mode, const string &pwd, const string &logDir, boost::shared_ptr<IrcThread> ircThread)
{
	m_lobbyThread.reset(new ServerLobbyThread(GetGui(), m_playerConfig, m_avatarManager, m_ioService));
	GetLobbyThread().Init(pwd, logDir);

	m_ircBot->Init(m_lobbyThread, ircThread);

	if (mode & NETWORK_MODE_TCP)
	{
		boost::shared_ptr<ServerAcceptHelper> tcpAcceptHelper(new ServerAcceptHelper(GetGui(), m_ioService));
		tcpAcceptHelper->Listen(serverPort, ipv6, false, pwd, logDir, m_lobbyThread);
		m_acceptHelperPool.push_back(tcpAcceptHelper);
	}
/*	if (mode & NETWORK_MODE_SCTP)
	{
		boost::shared_ptr<ServerAcceptHelper> sctpAcceptHelper(new ServerAcceptHelper(GetGui(), m_ioService));
		sctpAcceptHelper->Listen(serverPort, ipv6, true, pwd, logDir, m_lobbyThread);
		m_acceptHelperPool.push_back(sctpAcceptHelper);
	}*/
}

GuiInterface &
ServerManager::GetGui()
{
	return m_gui;
}

ServerIrcBot &
ServerManager::GetIrcBot()
{
	return *m_ircBot;
}

void
ServerManager::RunAll()
{
	m_ircBot->Run();
	GetLobbyThread().Run();
}

void
ServerManager::Process()
{
	m_ircBot->Process();
}

void
ServerManager::SignalTerminationAll()
{
	m_ircBot->SignalTermination();
	GetLobbyThread().SignalTermination();
}

bool
ServerManager::JoinAll(bool wait)
{
	m_ircBot->Join(wait);
	return GetLobbyThread().Join(wait ? NET_LOBBY_THREAD_TERMINATE_TIMEOUT_MSEC : 0);
}

ServerLobbyThread &
ServerManager::GetLobbyThread()
{
	assert(m_lobbyThread.get());
	return *m_lobbyThread;
}

