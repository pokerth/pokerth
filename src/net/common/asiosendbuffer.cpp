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

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <net/asiosendbuffer.h>
#include <net/sessiondata.h>
#include <net/netpacket.h>
#include <boost/swap.hpp>

using namespace std;


AsioSendBuffer::AsioSendBuffer()
	: sendBuf(NULL), curWriteBuf(NULL), sendBufAllocated(0), sendBufUsed(0),
	  curWriteBufAllocated(0), curWriteBufUsed(0), closeAfterSend(false)
{
}

AsioSendBuffer::~AsioSendBuffer()
{
	free(sendBuf);
	free(curWriteBuf);
}

void
AsioSendBuffer::SetCloseAfterSend()
{
	closeAfterSend = true;
}

void
AsioSendBuffer::HandleWrite(boost::shared_ptr<boost::asio::ip::tcp::socket> socket, const boost::system::error_code &error)
{
	if (!error) {
		// Successfully sent the data.
		boost::mutex::scoped_lock lock(dataMutex);
		curWriteBufUsed = 0;
		// Send more data, if available.
		AsyncSendNextPacket(socket);
	}
}

void
AsioSendBuffer::AsyncSendNextPacket(boost::shared_ptr<SessionData> session)
{
	AsyncSendNextPacket(session->GetAsioSocket());
}

void
AsioSendBuffer::AsyncSendNextPacket(boost::shared_ptr<boost::asio::ip::tcp::socket> socket)
{
	if (!curWriteBufUsed) {
		// Swap buffers and send data.
		boost::swap(curWriteBuf, sendBuf);
		boost::swap(curWriteBufAllocated, sendBufAllocated);
		boost::swap(curWriteBufUsed, sendBufUsed);
		if (curWriteBufUsed) {
			boost::asio::async_write(
				*socket,
				boost::asio::buffer(curWriteBuf, curWriteBufUsed),
				boost::bind(&SendBuffer::HandleWrite,
							shared_from_this(),
							socket,
							boost::asio::placeholders::error));
		} else if (closeAfterSend) {
			socket->close();
		}
	}
}

void
AsioSendBuffer::InternalStorePacket(boost::shared_ptr<SessionData> /*session*/, boost::shared_ptr<NetPacket> packet)
{
	uint32_t packetSize = packet->GetMsg()->ByteSize();
	google::protobuf::uint8 *buf = new google::protobuf::uint8[packetSize + NET_HEADER_SIZE];
	*((uint32_t *)buf) = htonl(packetSize);
	packet->GetMsg()->SerializeWithCachedSizesToArray(&buf[NET_HEADER_SIZE]);
	EncodeToBuf(buf, packetSize + NET_HEADER_SIZE);
	delete[] buf;
}

int
AsioSendBuffer::EncodeToBuf(const void *data, size_t size)
{
	// Realloc buffer if necessary.
	while (GetSendBufLeft() < size) {
		if (!ReallocSendBuf()) {
			return -1;
		}
	}

	AppendToSendBufWithoutCheck((const char*)data, size);

	return 0;
}

