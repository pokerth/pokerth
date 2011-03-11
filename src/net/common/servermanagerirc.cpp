/***************************************************************************
 *   Copyright (C) 2011 by Lothar May                                 *
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

#include <boost/asio.hpp>
#include <net/servermanagerirc.h>
#include <net/serveradminbot.h>
#include <net/serverlobbybot.h>
#include <net/serverlobbythread.h>
#include <net/ircthread.h>
#include <config/configfile.h>

using namespace std;

ServerManagerIrc::ServerManagerIrc(ConfigFile &config, GuiInterface &gui, ServerMode mode, AvatarManager &avatarManager)
	: ServerManager(config, gui)
{
	m_adminBot.reset(new ServerAdminBot);
	m_lobbyBot.reset(new ServerLobbyBot);
	m_lobbyThread.reset(new ServerLobbyThread(gui, mode, *m_lobbyBot, config, avatarManager, m_ioService));
}

ServerManagerIrc::~ServerManagerIrc()
{
}

void
ServerManagerIrc::Init(unsigned serverPort, bool ipv6, ServerTransportProtocol proto, const string &logDir)
{
	boost::shared_ptr<IrcThread> tmpIrcAdminThread;
	boost::shared_ptr<IrcThread> tmpIrcLobbyThread;
	ConfigFile &myConfig = GetConfig();
	if (myConfig.readConfigInt("UseAdminIRC")) {
		tmpIrcAdminThread = boost::shared_ptr<IrcThread>(new IrcThread(m_adminBot.get()));

		tmpIrcAdminThread->Init(
			myConfig.readConfigString("AdminIRCServerAddress"),
			myConfig.readConfigInt("AdminIRCServerPort"),
			myConfig.readConfigInt("AdminIRCServerUseIpv6") == 1,
			myConfig.readConfigString("AdminIRCServerNick"),
			myConfig.readConfigString("AdminIRCChannel"),
			myConfig.readConfigString("AdminIRCChannelPassword"));
	}

	if (myConfig.readConfigInt("UseLobbyIRC")) {
		tmpIrcLobbyThread = boost::shared_ptr<IrcThread>(new IrcThread(m_lobbyBot.get()));

		tmpIrcLobbyThread->Init(
			myConfig.readConfigString("LobbyIRCServerAddress"),
			myConfig.readConfigInt("LobbyIRCServerPort"),
			myConfig.readConfigInt("LobbyIRCServerUseIpv6") == 1,
			myConfig.readConfigString("LobbyIRCServerNick"),
			myConfig.readConfigString("LobbyIRCChannel"),
			myConfig.readConfigString("LobbyIRCChannelPassword"));
	}

	m_adminBot->Init(m_lobbyThread, tmpIrcAdminThread);
	m_lobbyBot->Init(m_lobbyThread, tmpIrcLobbyThread);
	ServerManager::Init(serverPort, ipv6, proto, logDir);
}

void
ServerManagerIrc::RunAll()
{
	m_adminBot->Run();
	m_lobbyBot->Run();
	ServerManager::RunAll();
}

void
ServerManagerIrc::Process()
{
	m_adminBot->Process();
	m_lobbyBot->Process();
	ServerManager::Process();
}

void
ServerManagerIrc::SignalTerminationAll()
{
	m_adminBot->SignalTermination();
	m_lobbyBot->SignalTermination();
	ServerManager::SignalTerminationAll();
}

bool
ServerManagerIrc::JoinAll(bool wait)
{
	m_adminBot->Join(wait);
	m_lobbyBot->Join(wait);
	return ServerManager::JoinAll(wait);
}

