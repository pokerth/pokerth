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

#include <net/sessiondata.h>
#include <net/webreceivebuffer.h>
#include <net/websocketdata.h>
#include <core/loghelper.h>

using namespace std;


WebReceiveBuffer::WebReceiveBuffer()
{
}

void
WebReceiveBuffer::StartAsyncRead(boost::shared_ptr<SessionData> /*session*/)
{
	// Nothing to do. This is handled internally by websocketpp.
}

void
WebReceiveBuffer::HandleRead(boost::shared_ptr<SessionData> /*session*/, const boost::system::error_code &/*error*/, size_t /*bytesRead*/)
{
	LOG_ERROR("WebReceiveBuffer::HandleRead should never be called because Websocket I/O is message based.");
}

void
WebReceiveBuffer::HandleMessage(boost::shared_ptr<SessionData> session, const string &msg)
{
	boost::shared_ptr<WebSocketData> webData = session->GetWebData();
	if (webData && webData->lengthPrefixed) {
		// Length-prefixed framing: do not rely on websocket message boundaries.
		// Accumulate and scan out every complete 4-byte-prefixed packet.
		m_recvBuffer.append(msg);
		ScanPrefixedPackets(session);
	} else {
		// Legacy framing: exactly one packet per websocket message.
		ProcessPacket(session, msg.c_str(), msg.size());
	}
}

void
WebReceiveBuffer::ScanPrefixedPackets(boost::shared_ptr<SessionData> session)
{
	// Same framing as the native TCP path (see AsioReceiveBuffer::ScanPackets):
	// 4-byte big-endian length, followed by that many protobuf payload bytes.
	while (m_recvBuffer.size() >= NET_HEADER_SIZE) {
		const unsigned char *p = reinterpret_cast<const unsigned char *>(m_recvBuffer.data());
		size_t packetSize =
			(static_cast<size_t>(p[0]) << 24) |
			(static_cast<size_t>(p[1]) << 16) |
			(static_cast<size_t>(p[2]) << 8) |
			static_cast<size_t>(p[3]);

		if (packetSize == 0 || packetSize > MAX_PACKET_SIZE) {
			LOG_ERROR("Session " << session->GetId() << " - Invalid websocket packet size: " << packetSize
					  << " (max: " << MAX_PACKET_SIZE << ") - closing connection");
			m_recvBuffer.clear();
			session->Close();
			return;
		}
		if (m_recvBuffer.size() < packetSize + NET_HEADER_SIZE) {
			break; // Wait for the rest of the packet.
		}
		ProcessPacket(session, m_recvBuffer.data() + NET_HEADER_SIZE, packetSize);
		m_recvBuffer.erase(0, packetSize + NET_HEADER_SIZE);
	}
}

void
WebReceiveBuffer::ProcessPacket(boost::shared_ptr<SessionData> session, const char *data, size_t size)
{
	boost::shared_ptr<NetPacket> tmpPacket;
	try {
		tmpPacket = NetPacket::Create(data, size);
		if (!tmpPacket || !validator.IsValidPacket(*tmpPacket)) {
			LOG_ERROR("Session " << session->GetId() << " - Invalid packet"
					  << (tmpPacket ? string(": ") + std::to_string(tmpPacket->GetMsg()->messagetype()) : ""));
			tmpPacket.reset();
		}
	} catch (const exception &e) {
		LOG_ERROR("Session " << session->GetId() << " - " << e.what());
	}
	if (tmpPacket) {
		session->HandlePacket(tmpPacket);
	}
}

