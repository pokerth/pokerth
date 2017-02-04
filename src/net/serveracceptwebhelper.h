/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2013 Felix Hammer, Florian Thauer, Lothar May          *
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
/* Network server helper to accept websocket connections. */

#ifndef _SERVERACCEPTWEBHELPER_H_
#define _SERVERACCEPTWEBHELPER_H_

#include <net/websocket_defs.h>
#include <net/serveracceptinterface.h>
#include <net/serverlobbythread.h>

class ServerAcceptWebHelper : public ServerAcceptInterface
{
public:
	ServerAcceptWebHelper(ServerCallback &serverCallback, boost::shared_ptr<boost::asio::io_service> ioService,
						  const std::string &webSocketResource, const std::string &webSocketOrigin);

	virtual void Listen(unsigned serverPort, bool ipv6, const std::string &logDir,
						boost::shared_ptr<ServerLobbyThread> lobbyThread);

	virtual void Close();

protected:
#if defined(__GXX_EXPERIMENTAL_CXX0X__) || (__cplusplus >= 201103L) // c++11
	typedef std::map<websocketpp::connection_hdl, boost::weak_ptr<SessionData>, std::owner_less<websocketpp::connection_hdl> > SessionMap;
#else
	typedef std::map<websocketpp::connection_hdl, boost::weak_ptr<SessionData> > SessionMap;
#endif
	bool validate(websocketpp::connection_hdl hdl);
	void on_open(websocketpp::connection_hdl hdl);
	void on_close(websocketpp::connection_hdl hdl);
	void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg);

private:
	boost::shared_ptr<boost::asio::io_service> m_ioService;
	ServerCallback &m_serverCallback;
	boost::shared_ptr<server> m_webSocketServer;
	SessionMap m_sessionMap;
	std::string m_webSocketResource;
	std::string m_webSocketOrigin;

	boost::shared_ptr<ServerLobbyThread> m_lobbyThread;
};

#endif
