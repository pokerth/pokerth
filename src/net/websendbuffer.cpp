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

#include <net/websendbuffer.h>
#include <net/websocketdata.h>
#include <net/netpacket.h>
#include <net/sessiondata.h>

using namespace std;


WebSendBuffer::WebSendBuffer()
	: closeAfterSend(false)
{
}

void
WebSendBuffer::SetCloseAfterSend()
{
	closeAfterSend = true;
}

void
WebSendBuffer::HandleWrite(boost::shared_ptr<boost::asio::ip::tcp::socket> /*socket*/, const boost::system::error_code &/*error*/)
{
}

void
WebSendBuffer::AsyncSendNextPacket(boost::shared_ptr<SessionData> session)
{
	if (closeAfterSend) {
		boost::shared_ptr<WebSocketData> webData = session->GetWebData();
		if (webData && webData->endpoint) {
			websocketpp::lib::error_code ec;
			webData->endpoint->Close(webData->webHandle, "PokerTH server closed the connection.", ec);
		}
	}
}

void
WebSendBuffer::InternalStorePacket(boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet)
{
	boost::shared_ptr<WebSocketData> webData = session->GetWebData();
	const bool prefixed = webData && webData->lengthPrefixed;

	uint32_t packetSize = packet->GetMsg()->ByteSizeLong();
	const size_t headerSize = prefixed ? NET_HEADER_SIZE : 0;
	google::protobuf::uint8 *buf = new google::protobuf::uint8[packetSize + headerSize];
	if (prefixed) {
		// 4-byte big-endian length prefix, identical to the native TCP framing.
		buf[0] = static_cast<google::protobuf::uint8>((packetSize >> 24) & 0xFF);
		buf[1] = static_cast<google::protobuf::uint8>((packetSize >> 16) & 0xFF);
		buf[2] = static_cast<google::protobuf::uint8>((packetSize >> 8) & 0xFF);
		buf[3] = static_cast<google::protobuf::uint8>(packetSize & 0xFF);
	}
	packet->GetMsg()->SerializeWithCachedSizesToArray(buf + headerSize);

	if (webData && webData->endpoint) {
		websocketpp::lib::error_code ec;
		webData->endpoint->Send(webData->webHandle, string((const char *)buf, packetSize + headerSize), ec);
		if (ec) {
			SetCloseAfterSend();
		}
	} else {
		SetCloseAfterSend();
	}

	delete[] buf;
}

