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

#include <net/serveracceptwebhelper.h>
#include <net/sessiondata.h>
#include <net/webreceivebuffer.h>
#include <net/websocketdata.h>

using namespace std;

ServerAcceptWebHelper::ServerAcceptWebHelper(ServerCallback &serverCallback, boost::shared_ptr<boost::asio::io_service> ioService,
		const string &webSocketResource, const string &webSocketOrigin)
	: m_ioService(ioService), m_serverCallback(serverCallback),
	  m_webSocketResource(webSocketResource), m_webSocketOrigin(webSocketOrigin)
{
	m_webSocketServer.reset(new server);
}

void
ServerAcceptWebHelper::Listen(unsigned serverPort, bool /*ipv6*/, const std::string &/*logDir*/, boost::shared_ptr<ServerLobbyThread> lobbyThread)
{
	m_lobbyThread = lobbyThread;

	// Set logging settings
#ifdef QT_NO_DEBUG
	m_webSocketServer->clear_access_channels(websocketpp::log::alevel::all);
#else
	m_webSocketServer->set_access_channels(websocketpp::log::alevel::all);
#endif

	m_webSocketServer->init_asio(m_ioService.get());

	m_webSocketServer->set_validate_handler(boost::bind(boost::mem_fn(&ServerAcceptWebHelper::validate), this, _1));
	m_webSocketServer->set_open_handler(boost::bind(boost::mem_fn(&ServerAcceptWebHelper::on_open), this, _1));
	m_webSocketServer->set_close_handler(boost::bind(boost::mem_fn(&ServerAcceptWebHelper::on_close), this, _1));
	m_webSocketServer->set_message_handler(boost::bind(boost::mem_fn(&ServerAcceptWebHelper::on_message), this, _1, _2));

	m_webSocketServer->listen(serverPort);
	m_webSocketServer->start_accept();
}

void
ServerAcceptWebHelper::Close()
{
}

bool
ServerAcceptWebHelper::validate(websocketpp::connection_hdl hdl)
{
	bool retVal = false;
	server::connection_ptr con = m_webSocketServer->get_con_from_hdl(hdl);
	if ((m_webSocketResource.empty() || con->get_resource() == m_webSocketResource)
			&& (m_webSocketOrigin.empty() ||
				(con->get_origin() != "null" &&
				 (con->get_origin() == "http://" + m_webSocketOrigin || con->get_origin() == "http://www." + m_webSocketOrigin)))) {
		retVal = true;
	}
	return retVal;
}

void
ServerAcceptWebHelper::on_open(websocketpp::connection_hdl hdl)
{
	boost::shared_ptr<WebSocketData> webData(new WebSocketData);
	webData->webSocketServer = m_webSocketServer;
	webData->webHandle = hdl;
	boost::shared_ptr<SessionData> sessionData(new SessionData(webData, m_lobbyThread->GetNextSessionId(), m_lobbyThread->GetSessionDataCallback(), *m_ioService, 0));
	m_sessionMap.insert(make_pair(hdl, sessionData));
	m_lobbyThread->AddConnection(sessionData);
}

void
ServerAcceptWebHelper::on_close(websocketpp::connection_hdl hdl)
{
	SessionMap::iterator pos = m_sessionMap.find(hdl);
	if (pos != m_sessionMap.end()) {
		boost::shared_ptr<SessionData> tmpSession = pos->second.lock();
		if (tmpSession) {
			tmpSession->Close();
		}
		m_sessionMap.erase(pos);
	}
}

void
ServerAcceptWebHelper::on_message(websocketpp::connection_hdl hdl, server::message_ptr msg)
{
	if (msg->get_opcode() == websocketpp::frame::opcode::BINARY) {
		SessionMap::iterator pos = m_sessionMap.find(hdl);
		if (pos != m_sessionMap.end()) {
			boost::shared_ptr<SessionData> tmpSession = pos->second.lock();
			if (tmpSession) {
				tmpSession->GetReceiveBuffer().HandleMessage(tmpSession, msg->get_payload());
			}
		}
	}
}

