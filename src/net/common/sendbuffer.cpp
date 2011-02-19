/***************************************************************************
 *   Copyright (C) 2011 by Lothar May                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <net/sendbuffer.h>
#include <boost/swap.hpp>

using namespace std;
using boost::asio::ip::tcp;


SendBuffer::SendBuffer()
	: sendBuf(NULL), curWriteBuf(NULL), sendBufAllocated(0), sendBufUsed(0),
	  curWriteBufAllocated(0), curWriteBufUsed(0)
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

