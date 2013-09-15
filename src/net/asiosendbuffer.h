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
/* Buffer for sending network data. */

#ifndef _ASIOSENDBUFFER_H_
#define _ASIOSENDBUFFER_H_

#include <net/sendbuffer.h>
#include <cstdlib>


#define SEND_BUF_FIRST_ALLOC_CHUNKSIZE		4096
#define MAX_SEND_BUF_SIZE					SEND_BUF_FIRST_ALLOC_CHUNKSIZE * 256


class AsioSendBuffer : public SendBuffer
{
public:
	AsioSendBuffer();
	virtual ~AsioSendBuffer();

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

	virtual void SetCloseAfterSend();

	virtual void AsyncSendNextPacket(boost::shared_ptr<SessionData> session);
	void AsyncSendNextPacket(boost::shared_ptr<boost::asio::ip::tcp::socket> socket);
	virtual void InternalStorePacket(boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet);
	int EncodeToBuf(const void *data, size_t size);

	virtual void HandleWrite(boost::shared_ptr<boost::asio::ip::tcp::socket> socket, const boost::system::error_code &error);

private:
	char *sendBuf;
	char *curWriteBuf;
	size_t sendBufAllocated;
	size_t sendBufUsed;
	size_t curWriteBufAllocated;
	size_t curWriteBufUsed;
	bool closeAfterSend;
};

#endif

