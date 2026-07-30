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

#include <boost/bind/bind.hpp>
#include <net/serveracceptwebhelper.h>
#include <net/sessiondata.h>
#include <net/webreceivebuffer.h>
#include <net/websocketdata.h>
#include <net/servertlsutil.h>

using namespace std;

ServerAcceptWebHelper::ServerAcceptWebHelper(ServerCallback &serverCallback, boost::shared_ptr<boost::asio::io_context> ioService,
		const string &webSocketResource, const string &webSocketOrigin, const bool &websocketTls)
	: m_ioService(ioService), m_serverCallback(serverCallback),
	  m_webSocketResource(webSocketResource), m_webSocketOrigin(webSocketOrigin)
{
	m_tls = websocketTls;
	if(m_tls){
		m_webSocketTlsServer.reset(new tls_server);
	}else{
		m_webSocketServer.reset(new server);
	}
	
}

void
ServerAcceptWebHelper::Listen(unsigned serverPort, bool /*ipv6*/, const std::string &/*logDir*/, boost::shared_ptr<ServerLobbyThread> lobbyThread)
{
	m_lobbyThread = lobbyThread;


	if(m_tls){
		// Set logging settings
#ifdef QT_NO_DEBUG
		m_webSocketTlsServer->clear_access_channels(websocketpp::log::alevel::all);
#else
		m_webSocketTlsServer->set_access_channels(websocketpp::log::alevel::all);
#endif

		m_webSocketTlsServer->init_asio(m_ioService.get());
		m_webSocketTlsServer->set_max_message_size(MAX_WEBSOCKET_MESSAGE_SIZE);

		m_webSocketTlsServer->set_validate_handler(boost::bind(boost::mem_fn(&ServerAcceptWebHelper::validate), this, boost::placeholders::_1));
		m_webSocketTlsServer->set_open_handler(boost::bind(boost::mem_fn(&ServerAcceptWebHelper::on_open), this, boost::placeholders::_1));
		m_webSocketTlsServer->set_close_handler(boost::bind(boost::mem_fn(&ServerAcceptWebHelper::on_close), this, boost::placeholders::_1));
		m_webSocketTlsServer->set_message_handler(boost::bind(boost::mem_fn(&ServerAcceptWebHelper::on_message), this, boost::placeholders::_1, boost::placeholders::_2));
		m_webSocketTlsServer->set_tls_init_handler(boost::bind(boost::mem_fn(&ServerAcceptWebHelper::on_tls_init), this, boost::placeholders::_1));

		m_webSocketTlsServer->listen(serverPort);
		m_webSocketTlsServer->start_accept();
	}else{
		// Set logging settings
#ifdef QT_NO_DEBUG
		m_webSocketServer->clear_access_channels(websocketpp::log::alevel::all);
#else
		m_webSocketServer->set_access_channels(websocketpp::log::alevel::all);
#endif

		m_webSocketServer->init_asio(m_ioService.get());
		m_webSocketServer->set_max_message_size(MAX_WEBSOCKET_MESSAGE_SIZE);

		m_webSocketServer->set_validate_handler(boost::bind(boost::mem_fn(&ServerAcceptWebHelper::validate), this, boost::placeholders::_1));
		m_webSocketServer->set_open_handler(boost::bind(boost::mem_fn(&ServerAcceptWebHelper::on_open), this, boost::placeholders::_1));
		m_webSocketServer->set_close_handler(boost::bind(boost::mem_fn(&ServerAcceptWebHelper::on_close), this, boost::placeholders::_1));
		m_webSocketServer->set_message_handler(boost::bind(boost::mem_fn(&ServerAcceptWebHelper::on_message), this, boost::placeholders::_1, boost::placeholders::_2));

		m_webSocketServer->listen(serverPort);
		m_webSocketServer->start_accept();
	}

}

void
ServerAcceptWebHelper::Close()
{
}

namespace
{
// Validate resource/origin and negotiate the length-prefix subprotocol.
// Templated so it works for both the TLS and non-TLS connection types.
template<typename ConPtr>
bool
ValidateAndNegotiate(ConPtr con, const std::string &resource, const std::string &origin)
{
	if (!con) {
		return false;
	}
	bool ok = (resource.empty() || con->get_resource() == resource)
			  && (origin.empty() ||
				  (con->get_origin() != "null" &&
				   (con->get_origin() == "http://" + origin || con->get_origin() == "http://www." + origin)));
	if (ok) {
		// If the client offered the length-prefix subprotocol, select it so it
		// is echoed back in the handshake. Both sides then frame every packet
		// with a 4-byte big-endian length prefix. Legacy clients offer no
		// subprotocol and keep the one-message-per-packet framing.
		const std::vector<std::string> &requested = con->get_requested_subprotocols();
		for (std::vector<std::string>::const_iterator it = requested.begin(); it != requested.end(); ++it) {
			if (*it == POKERTH_WS_SUBPROTOCOL_LEN) {
				con->select_subprotocol(POKERTH_WS_SUBPROTOCOL_LEN);
				break;
			}
		}
	}
	return ok;
}
}

bool
ServerAcceptWebHelper::validate(websocketpp::connection_hdl hdl)
{
	if (m_tls) {
		return ValidateAndNegotiate(m_webSocketTlsServer->get_con_from_hdl(hdl), m_webSocketResource, m_webSocketOrigin);
	}
	return ValidateAndNegotiate(m_webSocketServer->get_con_from_hdl(hdl), m_webSocketResource, m_webSocketOrigin);
}

void
ServerAcceptWebHelper::on_open(websocketpp::connection_hdl hdl)
{
	boost::shared_ptr<WebSocketData> webData(new WebSocketData);
	webData->webHandle = hdl;
	// Use the matching endpoint type and pick up the framing negotiated in
	// validate() (see ValidateAndNegotiate).
	if (m_tls) {
		webData->endpoint = boost::make_shared<WebSocketEndpointImpl<tls_server> >(m_webSocketTlsServer);
		tls_server::connection_ptr con = m_webSocketTlsServer->get_con_from_hdl(hdl);
		webData->lengthPrefixed = con && con->get_subprotocol() == POKERTH_WS_SUBPROTOCOL_LEN;
	} else {
		webData->endpoint = boost::make_shared<WebSocketEndpointImpl<server> >(m_webSocketServer);
		server::connection_ptr con = m_webSocketServer->get_con_from_hdl(hdl);
		webData->lengthPrefixed = con && con->get_subprotocol() == POKERTH_WS_SUBPROTOCOL_LEN;
	}
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

context_ptr ServerAcceptWebHelper::on_tls_init(websocketpp::connection_hdl hdl) {
    namespace asio = boost::asio;
    context_ptr ctx = websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::sslv23);
    // Configuration settings for TLS context
    try {
        ctx->set_options(asio::ssl::context::default_workarounds |
                         asio::ssl::context::no_sslv2 |
                         asio::ssl::context::no_sslv3 |
                         asio::ssl::context::single_dh_use);

        // Cert and key are resolved relative to the executable, not the
        // current working directory (see GetServerTlsDir).
        const std::string tlsDir = GetServerTlsDir();
        ctx->use_certificate_chain_file(tlsDir + "server.crt");
        ctx->use_private_key_file(tlsDir + "server.key", asio::ssl::context::pem);
        std::string ciphers;
        ciphers = "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:AES128-GCM-SHA256:AES256-GCM-SHA384:AES128-SHA256:AES256-SHA256:AES128-SHA:AES256-SHA:AES:CAMELLIA:DES-CBC3-SHA:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!MD5:!PSK:!aECDH:!EDH-DSS-DES-CBC3-SHA:!EDH-RSA-DES-CBC3-SHA:!KRB5-DES-CBC3-SHA";
        if (SSL_CTX_set_cipher_list(ctx->native_handle() , ciphers.c_str()) != 1) {
            std::cout << "Error setting cipher list" << std::endl;
        }
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }
    return ctx;
}
