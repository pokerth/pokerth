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
/* Buffer for sending network data. */

#ifndef _SENDBUFFER_H_
#define _SENDBUFFER_H_

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <cstdlib>


#define SEND_BUF_FIRST_ALLOC_CHUNKSIZE		4096
#define MAX_SEND_BUF_SIZE					SEND_BUF_FIRST_ALLOC_CHUNKSIZE * 256


class SendBuffer : public boost::enable_shared_from_this<SendBuffer>
{
public:
	SendBuffer();
	~SendBuffer();

	inline size_t GetSendBufLeft() const {
		int bytesLeft = (int)(sendBufAllocated - sendBufUsed);
		return bytesLeft < 0 ? (size_t)0 : (size_t)bytesLeft;
	}

	inline size_t GetAllocated() const {
		return sendBufAllocated;
	}

	inline bool ReallocSendBuf() {
		bool retVal = false;
		size_t allocAmount = sendBufAllocated * 2;
		if (0 == allocAmount) {
			allocAmount = (size_t)SEND_BUF_FIRST_ALLOC_CHUNKSIZE;
		}
		if (allocAmount <= MAX_SEND_BUF_SIZE) {
			char *tempBuf = (char *)std::realloc(sendBuf, allocAmount);
			if (tempBuf) {
				sendBuf = tempBuf;
				sendBufAllocated = allocAmount;
				retVal = true;
			}
		}
		return retVal;
	}

	inline void AppendToSendBufWithoutCheck(const char *data, size_t size) {
		std::memcpy(sendBuf + sendBufUsed, data, size);
		sendBufUsed += size;
	}

	void HandleWrite(boost::shared_ptr<boost::asio::ip::tcp::socket> socket, const boost::system::error_code &error);
	void AsyncSendNextPacket(boost::shared_ptr<boost::asio::ip::tcp::socket> socket);

	static int EncodeToBuf(const void *data, size_t size, void *arg);

	mutable boost::mutex dataMutex;

private:
	char *sendBuf;
	char *curWriteBuf;
	size_t sendBufAllocated;
	size_t sendBufUsed;
	size_t curWriteBufAllocated;
	size_t curWriteBufUsed;
};

#endif

