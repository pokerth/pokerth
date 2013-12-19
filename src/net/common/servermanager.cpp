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
#include <net/servermanager.h>
#include <net/socket_helper.h>
#include <net/serverlobbythread.h>
#include <net/serveraccepthelper.h>
#include <net/serveracceptwebhelper.h>
#include <net/serverexception.h>
#include <net/socket_msg.h>
#include <net/socket_startup.h>
#include <net/serverircbotcallback.h>
#include <core/loghelper.h>

#include <boost/bind.hpp>
#include <boost/algorithm/string/predicate.hpp>

using namespace std;

class GenericIrcCallback : public ServerIrcBotCallback
{
	virtual void SignalLobbyMessage(unsigned /*playerId*/, const std::string &/*playerName*/, const std::string &/*msg*/) {}
};

static GenericIrcCallback g_ircCallback;

ServerManager::ServerManager(ConfigFile &config, GuiInterface &gui)
	: m_playerConfig(config), m_gui(gui)
{
	m_ioService.reset(new boost::asio::io_service);
}

ServerManager::ServerManager(ConfigFile &config, GuiInterface &gui, ServerMode mode, AvatarManager &avatarManager)
	: m_playerConfig(config), m_gui(gui)
{
	m_ioService.reset(new boost::asio::io_service);
	m_lobbyThread.reset(new ServerLobbyThread(gui, mode, g_ircCallback, config, avatarManager, m_ioService));
}

ServerManager::~ServerManager()
{
	size_t remainingHandler = 0;
	// Call all pending handlers to clean up.
	do {
		m_ioService->reset();
		remainingHandler = m_ioService->poll();
	} while (remainingHandler > 0);
	m_ioService->reset();
}

void
ServerManager::Init(unsigned serverPort, unsigned websocketPort, bool ipv6, int proto, const string &logDir,
					const string &webSocketResource, const string &webSocketOrigin)
{
	GetLobbyThread().Init(logDir);

	if (proto & TRANSPORT_PROTOCOL_TCP) {
		boost::shared_ptr<ServerAcceptInterface> tcpAcceptHelper(new ServerAcceptHelper<boost::asio::ip::tcp>(GetGui(), m_ioService));
		tcpAcceptHelper->Listen(serverPort, ipv6, logDir, m_lobbyThread);
		m_acceptHelperPool.push_back(tcpAcceptHelper);
	}
	/*	if (proto & TRANSPORT_PROTOCOL_SCTP)
		{
			boost::shared_ptr<ServerAcceptInterface> sctpAcceptHelper(new ServerAcceptHelper<boost::asio::ip::sctp>(GetGui(), m_ioService));
			sctpAcceptHelper->Listen(serverPort, ipv6, logDir, m_lobbyThread);
			m_acceptHelperPool.push_back(sctpAcceptHelper);
		}*/
	if (proto & TRANSPORT_PROTOCOL_WEBSOCKET) {
		boost::shared_ptr<ServerAcceptInterface> webAcceptHelper(
			new ServerAcceptWebHelper(GetGui(), m_ioService, webSocketResource, webSocketOrigin));
		webAcceptHelper->Listen(websocketPort, ipv6, logDir, m_lobbyThread);
		m_acceptHelperPool.push_back(webAcceptHelper);
	}
}

void
ServerManager::RunAll()
{
	GetLobbyThread().Run();
}

void
ServerManager::SignalTerminationAll()
{
	GetLobbyThread().SignalTermination();
}

bool
ServerManager::JoinAll(bool wait)
{
	return GetLobbyThread().Join(wait ? NET_LOBBY_THREAD_TERMINATE_TIMEOUT_MSEC : 0);
}

ServerLobbyThread &
ServerManager::GetLobbyThread()
{
	assert(m_lobbyThread);
	return *m_lobbyThread;
}

