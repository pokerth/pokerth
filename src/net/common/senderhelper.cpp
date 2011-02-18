/***************************************************************************
 *   Copyright (C) 2007-2009 by Lothar May                                 *
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
#include <boost/enable_shared_from_this.hpp>
#include <boost/swap.hpp>

#include <net/senderhelper.h>
#include <net/sendercallback.h>
#include <net/socket_helper.h>
#include <net/socket_msg.h>
#include <core/loghelper.h>
#include <cstring>
#include <cassert>

using namespace std;
using boost::asio::ip::tcp;


#define SEND_BUF_FIRST_ALLOC_CHUNKSIZE		4096
#define MAX_SEND_BUF_SIZE					SEND_BUF_FIRST_ALLOC_CHUNKSIZE * 1000


class SendDataManager : public boost::enable_shared_from_this<SendDataManager>
{
public:
	SendDataManager()
		: sendBuf(NULL), curWriteBuf(NULL), sendBufAllocated(0), sendBufUsed(0),
		  curWriteBufAllocated(0), curWriteBufUsed(0)
	{
	}

	~SendDataManager()
	{
		free(sendBuf);
		free(curWriteBuf);
	}

	inline size_t GetSendBufLeft() const
	{
		int bytesLeft = sendBufAllocated - sendBufUsed;
		return bytesLeft < 0 ? (size_t)0 : (size_t)bytesLeft;
	}

	inline size_t GetAllocated() const
	{
		return sendBufAllocated;
	}

	inline bool ReallocSendBuf()
	{
		bool retVal = false;
		size_t allocAmount = sendBufAllocated * 2;
		if (0 == allocAmount) {
			allocAmount = (size_t)SEND_BUF_FIRST_ALLOC_CHUNKSIZE;
		}
		char *tempBuf = (char *)realloc(sendBuf, allocAmount);
		if (tempBuf) {
			sendBuf = tempBuf;
			sendBufAllocated = allocAmount;
			retVal = true;
		}
		return retVal;
	}

	inline void AppendToSendBufWithoutCheck(const char *data, size_t size)
	{
		memcpy(sendBuf + sendBufUsed, data, size);
		sendBufUsed += size;
	}

	void HandleWrite(boost::shared_ptr<boost::asio::ip::tcp::socket> socket, const boost::system::error_code &error);

	void AsyncSendNextPacket(boost::shared_ptr<boost::asio::ip::tcp::socket> socket);

	mutable boost::mutex dataMutex;

private:
	char *sendBuf;
	char *curWriteBuf;
	size_t sendBufAllocated;
	size_t sendBufUsed;
	size_t curWriteBufAllocated;
	size_t curWriteBufUsed;
};


void
SendDataManager::HandleWrite(boost::shared_ptr<boost::asio::ip::tcp::socket> socket, const boost::system::error_code &error)
{
	if (!error) {
		// Successfully sent the data.
		curWriteBufUsed = 0;
		// Send more data, if available.
		AsyncSendNextPacket(socket);
	}
}

void
SendDataManager::AsyncSendNextPacket(boost::shared_ptr<boost::asio::ip::tcp::socket> socket)
{
	boost::mutex::scoped_lock lock(dataMutex);
	if (!curWriteBufUsed) {
		// Swap buffers and send data.
		boost::swap(curWriteBuf, sendBuf);
		boost::swap(curWriteBufAllocated, sendBufAllocated);
		boost::swap(curWriteBufUsed, sendBufUsed);
		if (curWriteBufUsed) {
			boost::asio::async_write(
				*socket,
				boost::asio::buffer(curWriteBuf, curWriteBufUsed),
				boost::bind(&SendDataManager::HandleWrite,
							shared_from_this(),
							socket,
							boost::asio::placeholders::error));
		}
	}
}

SenderHelper::SenderHelper(SenderCallback &cb, boost::shared_ptr<boost::asio::io_service> ioService)
	: m_callback(cb), m_ioService(ioService)
{
}

SenderHelper::~SenderHelper()
{
}

void
SenderHelper::Send(boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet)
{
	if (packet && session) {
		boost::shared_ptr<SendDataManager> tmpManager;
		{
			// First: lock map of all queues. Locate/insert queue.
			boost::mutex::scoped_lock lock(m_sendQueueMapMutex);
			SendQueueMap::iterator pos = m_sendQueueMap.find(session->GetId());
			if (pos == m_sendQueueMap.end())
				pos = m_sendQueueMap.insert(SendQueueMap::value_type(session->GetId(), boost::shared_ptr<SendDataManager>(new SendDataManager))).first;
			tmpManager = pos->second;
		}
		{
			// Second: Add packet to specific queue.
			boost::mutex::scoped_lock lock(tmpManager->dataMutex);
			if (tmpManager->GetAllocated() <= MAX_SEND_BUF_SIZE) {
				InternalStorePacket(*tmpManager, packet);
			}
		}
		{
			// Third: Activate async send, if needed.
			tmpManager->AsyncSendNextPacket(session->GetAsioSocket());
		}
	}
}

void
SenderHelper::Send(boost::shared_ptr<SessionData> session, const NetPacketList &packetList)
{
	if (!packetList.empty() && session) {
		boost::shared_ptr<SendDataManager> tmpManager;
		{
			// First: lock map of all queues. Locate/insert queue.
			boost::mutex::scoped_lock lock(m_sendQueueMapMutex);
			SendQueueMap::iterator pos = m_sendQueueMap.find(session->GetId());
			if (pos == m_sendQueueMap.end())
				pos = m_sendQueueMap.insert(SendQueueMap::value_type(session->GetId(), boost::shared_ptr<SendDataManager>(new SendDataManager))).first;
			tmpManager = pos->second;
		}
		{
			// Second: Add packets to specific queue.
			boost::mutex::scoped_lock lock(tmpManager->dataMutex);
			if (tmpManager->GetAllocated() <= MAX_SEND_BUF_SIZE) {
				NetPacketList::const_iterator i = packetList.begin();
				NetPacketList::const_iterator end = packetList.end();
				while (i != end) {
					if (*i)
						InternalStorePacket(*tmpManager, *i);
					++i;
				}
			}
		}
		{
			// Third: Activate async send, if needed.
			tmpManager->AsyncSendNextPacket(session->GetAsioSocket());
		}
	}
}

void
SenderHelper::SignalSessionTerminated(unsigned sessionId)
{
	boost::mutex::scoped_lock lock(m_sendQueueMapMutex);

	SendQueueMap::iterator pos = m_sendQueueMap.find(sessionId);
	if (pos != m_sendQueueMap.end())
		m_sendQueueMap.erase(pos);
}

static int der_encode_to_buffer_cb(const void *data, size_t size, void *arg) {
	SendDataManager *m = (SendDataManager *)arg;

	// Realloc buffer if necessary.
	while (m->GetSendBufLeft() < size) {
		if (!m->ReallocSendBuf()) {
			return -1;
		}
	}

	m->AppendToSendBufWithoutCheck((const char*)data, size);

	return 0;
}

void
SenderHelper::InternalStorePacket(SendDataManager &tmpManager, boost::shared_ptr<NetPacket> packet)
{
	asn_enc_rval_t e = der_encode(&asn_DEF_PokerTHMessage, packet->GetMsg(), der_encode_to_buffer_cb, &tmpManager);
	//cerr << "OUT:" << endl << packet->ToString() << endl;
	if (e.encoded == -1)
		LOG_ERROR("Failed to encode NetPacket: " << packet->GetMsg()->present);
}

