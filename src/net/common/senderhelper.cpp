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

#include <net/senderhelper.h>
#include <net/sendercallback.h>
#include <net/socket_helper.h>
#include <net/socket_msg.h>
#include <core/loghelper.h>
#include <cstring>
#include <cassert>

using namespace std;
using boost::asio::ip::tcp;


#define SEND_ERROR_TIMEOUT_MSEC				20000
#define SEND_TIMEOUT_MSEC					10
#define SEND_QUEUE_SIZE						10000000
#define SEND_LOG_INTERVAL_SEC				60

class EncodedPacket
{
public:
	EncodedPacket(const unsigned char *data, unsigned size)
		: m_size(size)
	{
		m_data = new unsigned char[size];
		memcpy(m_data, data, size);
	}

	~EncodedPacket()
	{
		delete[] m_data;
	}

	unsigned GetSize() const
	{
		return m_size;
	}

	const unsigned char *GetData() const
	{
		return m_data;
	}

private:
	unsigned m_size;
	unsigned char *m_data;
};

typedef std::list<boost::shared_ptr<EncodedPacket> > SendDataList;

class SendDataManager : public boost::enable_shared_from_this<SendDataManager>
{
	public:
		SendDataManager()
		: sendBuf(NULL), writeInProgress(false)
		{
		}

		~SendDataManager()
		{
			delete[] sendBuf;
		}

		void HandleWrite(boost::shared_ptr<boost::asio::ip::tcp::socket> socket, const boost::system::error_code &error);

		void AsyncSendNextPacket(boost::shared_ptr<boost::asio::ip::tcp::socket> socket, bool handlerMode = false);

		mutable boost::mutex dataMutex;
		SendDataList list;
		char *sendBuf;
		bool writeInProgress;
};


void
SendDataManager::HandleWrite(boost::shared_ptr<boost::asio::ip::tcp::socket> socket, const boost::system::error_code &error)
{
	if (!error)
		AsyncSendNextPacket(socket, true);
}

void
SendDataManager::AsyncSendNextPacket(boost::shared_ptr<boost::asio::ip::tcp::socket> socket, bool handlerMode)
{
	boost::mutex::scoped_lock lock(dataMutex);
	if (!writeInProgress || handlerMode)
	{
		delete[] sendBuf;
		sendBuf = NULL;

		unsigned bufPos = 0;
		unsigned bufSize = 0;
		// Count required bytes.
		SendDataList::iterator i = list.begin();
		SendDataList::iterator end = list.end();
		while (i != end)
		{
			bufSize += (*i)->GetSize();
			++i;
		}
		if (bufSize)
		{
			sendBuf = new char[bufSize];
			i = list.begin();
			end = list.end();
			while (i != end)
			{
				memcpy(sendBuf + bufPos, (*i)->GetData(), (*i)->GetSize());
				bufPos += (*i)->GetSize();
				++i;
			}
			list.clear();
			boost::asio::async_write(
				*socket,
				boost::asio::buffer(sendBuf, bufSize),
				boost::bind(&SendDataManager::HandleWrite,
					shared_from_this(),
					socket,
					boost::asio::placeholders::error));
			writeInProgress = true;
		}
		else
			writeInProgress = false;
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
	if (packet && session)
	{
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
			if (tmpManager->list.size() < SEND_QUEUE_SIZE)
			{
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
	if (!packetList.empty() && session)
	{
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
			if (tmpManager->list.size() + packetList.size() <= SEND_QUEUE_SIZE)
			{
				NetPacketList::const_iterator i = packetList.begin();
				NetPacketList::const_iterator end = packetList.end();
				while (i != end)
				{
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

void
SenderHelper::InternalStorePacket(SendDataManager &tmpManager, boost::shared_ptr<NetPacket> packet)
{
	unsigned char buf[MAX_PACKET_SIZE];
	asn_enc_rval_t e;
	e = der_encode_to_buffer(&asn_DEF_PokerTHMessage, packet->GetMsg(), buf, MAX_PACKET_SIZE);
	if (e.encoded == -1)
		LOG_ERROR("Failed to encode NetPacket: " << packet->GetMsg()->present);
	else
		tmpManager.list.push_back(boost::shared_ptr<EncodedPacket>(new EncodedPacket(buf, e.encoded)));
}

