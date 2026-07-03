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
/* Structure for web socket data for a session. */

#ifndef _WEBSOCKETDATA_H_
#define _WEBSOCKETDATA_H_

#include <net/websocket_defs.h>


// Subprotocol name a client must request (Sec-WebSocket-Protocol) to enable
// length-prefixed framing. Clients that do not request it keep the legacy
// "one websocket message == one packet" framing for backward compatibility.
#define POKERTH_WS_SUBPROTOCOL_LEN	"pokerth-len-v1"

// Type-erased view on the underlying websocketpp endpoint. The TLS and non-TLS
// endpoints are distinct C++ types (server vs tls_server); this interface lets
// the rest of the server treat them uniformly, so the websocket send/close
// paths work for both ws:// and wss:// connections.
class WebSocketEndpoint
{
public:
	virtual ~WebSocketEndpoint() {}
	virtual void Send(websocketpp::connection_hdl hdl, const std::string &payload,
					  websocketpp::lib::error_code &ec) = 0;
	virtual void Close(websocketpp::connection_hdl hdl, const std::string &reason,
					   websocketpp::lib::error_code &ec) = 0;
	// Remote IP address of the connection, or empty string on failure.
	virtual std::string RemoteAddress(websocketpp::connection_hdl hdl) = 0;
};

template<typename ServerType>
class WebSocketEndpointImpl : public WebSocketEndpoint
{
public:
	explicit WebSocketEndpointImpl(boost::shared_ptr<ServerType> srv) : m_server(srv) {}

	void Send(websocketpp::connection_hdl hdl, const std::string &payload,
			  websocketpp::lib::error_code &ec) override
	{
		m_server->send(hdl, payload, websocketpp::frame::opcode::BINARY, ec);
	}

	void Close(websocketpp::connection_hdl hdl, const std::string &reason,
			   websocketpp::lib::error_code &ec) override
	{
		m_server->close(hdl, websocketpp::close::status::normal, reason, ec);
	}

	std::string RemoteAddress(websocketpp::connection_hdl hdl) override
	{
		try {
			typename ServerType::connection_ptr con = m_server->get_con_from_hdl(hdl);
			if (con) {
				boost::system::error_code ec;
				auto ep = con->get_raw_socket().remote_endpoint(ec);
				if (!ec) {
					return ep.address().to_string();
				}
			}
		} catch (...) {
		}
		return std::string();
	}

private:
	boost::shared_ptr<ServerType> m_server;
};

struct WebSocketData {
	boost::shared_ptr<WebSocketEndpoint> endpoint;
	websocketpp::connection_hdl webHandle;
	// When true, every packet on this connection is delimited by a 4-byte
	// big-endian length prefix (same wire framing as the native TCP path),
	// in both directions. Negotiated via the websocket subprotocol at open.
	bool lengthPrefixed = false;
};

#endif
