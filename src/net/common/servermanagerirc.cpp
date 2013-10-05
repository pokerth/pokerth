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
	m_adminBot.reset(new ServerAdminBot(m_ioService));
	m_lobbyBot.reset(new ServerLobbyBot(m_ioService));
	m_lobbyThread.reset(new ServerLobbyThread(gui, mode, *m_lobbyBot, config, avatarManager, m_ioService));
}

ServerManagerIrc::~ServerManagerIrc()
{
}

void
ServerManagerIrc::Init(unsigned serverPort, unsigned websocketPort, bool ipv6, int proto, const string &logDir,
					   const std::string &webSocketResource, const std::string &webSocketOrigin)
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

	m_adminBot->Init(m_lobbyThread, tmpIrcAdminThread, myConfig.readConfigString("CacheDir"));
	m_lobbyBot->Init(m_lobbyThread, tmpIrcLobbyThread);
	ServerManager::Init(serverPort, websocketPort, ipv6, proto, logDir, webSocketResource, webSocketOrigin);
}

void
ServerManagerIrc::RunAll()
{
	m_adminBot->Run();
	m_lobbyBot->Run();
	ServerManager::RunAll();
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

