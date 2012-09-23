/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2011 Felix Hammer, Florian Thauer, Lothar May          *
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
 *****************************************************************************/

#include <net/senderhelper.h>
#include <net/sessiondata.h>
#include <net/sendbuffer.h>
#include <net/socket_helper.h>
#include <net/socket_msg.h>
#include <core/loghelper.h>
#include <cstring>
#include <cassert>

using namespace std;

SenderHelper::SenderHelper(boost::shared_ptr<boost::asio::io_service> ioService)
	: m_ioService(ioService)
{
}

SenderHelper::~SenderHelper()
{
}

void
SenderHelper::Send(boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet)
{
	if (packet && session) {
		SendBuffer &tmpBuffer = session->GetSendBuffer();
		// Add packet to specific queue.
		boost::mutex::scoped_lock lock(tmpBuffer.dataMutex);
		InternalStorePacket(tmpBuffer, packet);
		// Activate async send, if needed.
		tmpBuffer.AsyncSendNextPacket(session->GetAsioSocket());
	}
}

void
SenderHelper::Send(boost::shared_ptr<SessionData> session, const NetPacketList &packetList)
{
	if (!packetList.empty() && session) {
		SendBuffer &tmpBuffer = session->GetSendBuffer();
		// Add packets to specific queue.
		boost::mutex::scoped_lock lock(tmpBuffer.dataMutex);
		NetPacketList::const_iterator i = packetList.begin();
		NetPacketList::const_iterator end = packetList.end();
		while (i != end) {
			if (*i)
				InternalStorePacket(tmpBuffer, *i);
			++i;
		}
		// Activate async send, if needed.
		tmpBuffer.AsyncSendNextPacket(session->GetAsioSocket());
	}
}

void
SenderHelper::SetCloseAfterSend(boost::shared_ptr<SessionData> session)
{
	SendBuffer &tmpBuffer = session->GetSendBuffer();
	// Add packet to specific queue.
	boost::mutex::scoped_lock lock(tmpBuffer.dataMutex);
	// Mark that the socket should be closed after the send operation.
	tmpBuffer.SetCloseAfterSend();
	// Activate async send, if needed.
	tmpBuffer.AsyncSendNextPacket(session->GetAsioSocket());
}

void
SenderHelper::InternalStorePacket(SendBuffer &tmpBuffer, boost::shared_ptr<NetPacket> packet)
{
	uint32_t packetSize = packet->GetMsg()->ByteSize();
	google::protobuf::uint8 *buf = new google::protobuf::uint8[packetSize + NET_HEADER_SIZE];
	*((uint32_t *)buf) = htonl(packetSize);
	packet->GetMsg()->SerializeWithCachedSizesToArray(&buf[NET_HEADER_SIZE]);
	SendBuffer::EncodeToBuf(buf, packetSize + NET_HEADER_SIZE, &tmpBuffer);
	delete[] buf;
}

