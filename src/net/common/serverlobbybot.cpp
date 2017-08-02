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

#include <net/serverlobbybot.h>
#include <net/ircthread.h>
#include <net/serverlobbythread.h>
#include <net/serverbanmanager.h>
#include <net/serverexception.h>
#include <net/socket_msg.h>
#include <net/socket_startup.h>
#include <core/loghelper.h>

#include <boost/bind.hpp>
#include <boost/algorithm/string/predicate.hpp>

#define SERVER_RESTART_IRC_BOT_INTERVAL_SEC			86400	// 1 day

using namespace std;

#ifdef BOOST_ASIO_HAS_STD_CHRONO
using namespace std::chrono;
#else
using namespace boost::chrono;
#endif

ServerLobbyBot::ServerLobbyBot(boost::shared_ptr<boost::asio::io_service> ioService)
	: m_reconnectTimer(*ioService)
{
}

ServerLobbyBot::~ServerLobbyBot()
{
}

void
ServerLobbyBot::Init(boost::shared_ptr<ServerLobbyThread> lobbyThread, boost::shared_ptr<IrcThread> ircLobbyThread)
{
	m_lobbyThread = lobbyThread;
	m_ircLobbyThread = ircLobbyThread;
}

void
ServerLobbyBot::SignalIrcConnect(const std::string &server)
{
	LOG_MSG("Lobby bot: Connected to IRC server " << server << ".");
}

void
ServerLobbyBot::SignalIrcSelfJoined(const std::string &nickName, const std::string &channel)
{
	LOG_MSG("Lobby bot: Joined IRC channel " << channel << " as user " << nickName << ".");
	m_ircNick = nickName;
}

void
ServerLobbyBot::SignalIrcChatMsg(const std::string &/*nickName*/, const std::string &/*msg*/)
{
}

void
ServerLobbyBot::SignalIrcError(int errorCode)
{
	LOG_MSG("Lobby bot: IRC error " << errorCode << ".");
}

void
ServerLobbyBot::SignalIrcServerError(int errorCode)
{
	LOG_MSG("Lobby bot: IRC server error " << errorCode << ".");
}

void
ServerLobbyBot::SignalLobbyMessage(unsigned playerId, const std::string &playerName, const std::string &msg)
{
	if (m_ircLobbyThread && !msg.empty()) {
		ostringstream ircMsg;
		if (playerId)
			ircMsg << playerName << " (" << playerId << "): " << msg;
		else
			ircMsg << playerName << ": " << msg;
		m_ircLobbyThread->SendChatMessage(ircMsg.str());
	}
}

void
ServerLobbyBot::Run()
{
	if (m_ircLobbyThread) {
		// Initialise the reconnect timer.
		m_reconnectTimer.expires_from_now(
			seconds(SERVER_RESTART_IRC_BOT_INTERVAL_SEC));
		m_reconnectTimer.async_wait(
			boost::bind(
				&ServerLobbyBot::Reconnect, shared_from_this(), boost::asio::placeholders::error));

		m_ircLobbyThread->Run();
	}
}

void
ServerLobbyBot::Reconnect(const boost::system::error_code& ec)
{
	if (!ec) {
		if (m_ircLobbyThread) {
			m_ircLobbyThread->SignalTermination();
			if (m_ircLobbyThread->Join(NET_ADMIN_IRC_TERMINATE_TIMEOUT_MSEC)) {
				boost::shared_ptr<IrcThread> tmpIrcThread(new IrcThread(*m_ircLobbyThread));
				tmpIrcThread->Run();
				m_ircLobbyThread = tmpIrcThread;
			}
		}
		m_reconnectTimer.expires_from_now(
			seconds(SERVER_RESTART_IRC_BOT_INTERVAL_SEC));
		m_reconnectTimer.async_wait(
			boost::bind(
				&ServerLobbyBot::Reconnect, shared_from_this(), boost::asio::placeholders::error));
	}
}

void
ServerLobbyBot::SignalTermination()
{
	if (m_ircLobbyThread)
		m_ircLobbyThread->SignalTermination();

	// Terminated the reconnect timer.
	m_reconnectTimer.cancel();
}

bool
ServerLobbyBot::Join(bool wait)
{
	bool terminated = true;
	if (m_ircLobbyThread)
		terminated = m_ircLobbyThread->Join(wait ? NET_ADMIN_IRC_TERMINATE_TIMEOUT_MSEC : 0);
	return terminated;
}

ServerLobbyThread &
ServerLobbyBot::GetLobbyThread()
{
	assert(m_lobbyThread.get());
	return *m_lobbyThread;
}

