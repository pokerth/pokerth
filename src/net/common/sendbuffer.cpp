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

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <net/sendbuffer.h>
#include <boost/swap.hpp>

using namespace std;


SendBuffer::SendBuffer()
	: sendBuf(NULL), curWriteBuf(NULL), sendBufAllocated(0), sendBufUsed(0),
	  curWriteBufAllocated(0), curWriteBufUsed(0), closeAfterSend(false)
{
}

SendBuffer::~SendBuffer()
{
	free(sendBuf);
	free(curWriteBuf);
}

void
SendBuffer::HandleWrite(boost::shared_ptr<boost::asio::ip::tcp::socket> socket, const boost::system::error_code &error)
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
SendBuffer::AsyncSendNextPacket(boost::shared_ptr<boost::asio::ip::tcp::socket> socket)
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

int
SendBuffer::EncodeToBuf(const void *data, size_t size, void *arg)
{
	SendBuffer *m = (SendBuffer *)arg;

	// Realloc buffer if necessary.
	while (m->GetSendBufLeft() < size) {
		if (!m->ReallocSendBuf()) {
			return -1;
		}
	}

	m->AppendToSendBufWithoutCheck((const char*)data, size);

	return 0;
}

