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

#include <net/socket_helper.h>
#include <net/senderhelper.h>
#include <net/sendercallback.h>
#include <net/socket_msg.h>
#include <core/loghelper.h>
#include <cstring>
#include <cassert>

#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>

using namespace std;
using boost::asio::ip::tcp;


#define SEND_ERROR_TIMEOUT_MSEC				20000
#define SEND_TIMEOUT_MSEC					10
#define SEND_QUEUE_SIZE						10000000
#define SEND_LOG_INTERVAL_SEC				60


typedef std::list<boost::shared_ptr<NetPacket> > SendDataList;

class SendDataManager : public boost::enable_shared_from_this<SendDataManager>
{
	public:
		SendDataManager(boost::shared_ptr<boost::asio::ip::tcp::socket> s)
		: socket(s), writeInProgress(false)
		{
		}

		void HandleWrite(const boost::system::error_code& error);

		void AsyncSendNextPacket(bool handlerMode = false);

		boost::shared_ptr<boost::asio::ip::tcp::socket> socket;

		mutable boost::mutex dataMutex;
		SendDataList list;
		bool writeInProgress;
};


void
SendDataManager::HandleWrite(const boost::system::error_code& error)
{
	// TODO error handling
	AsyncSendNextPacket(true);
}

void
SendDataManager::AsyncSendNextPacket(bool handlerMode)
{
	boost::mutex::scoped_lock lock(dataMutex);
	if (!writeInProgress || handlerMode)
	{
		if (handlerMode)
			list.pop_front();
		if (!list.empty())
		{
			boost::shared_ptr<NetPacket> nextPacket = list.front();
			boost::asio::async_write(
				*socket,
				boost::asio::buffer(nextPacket->GetRawData(),
					nextPacket->GetLen()),
				boost::bind(&SendDataManager::HandleWrite, shared_from_this(),
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
	if (packet.get() && session.get())
	{
		boost::shared_ptr<SendDataManager> tmpManager;
		{
			// First: lock map of all queues. Locate/insert queue.
			boost::mutex::scoped_lock lock(m_sendQueueMapMutex);
			SendQueueMap::iterator pos = m_sendQueueMap.find(session->GetId());
			if (pos == m_sendQueueMap.end())
				pos = m_sendQueueMap.insert(SendQueueMap::value_type(session->GetId(), boost::shared_ptr<SendDataManager>(new SendDataManager(session->GetAsioSocket())))).first;
			tmpManager = pos->second;
		}
		{
			// Second: Add packet to specific queue.
			boost::mutex::scoped_lock lock(tmpManager->dataMutex);
			if (tmpManager->list.size() < SEND_QUEUE_SIZE)
			{
				tmpManager->list.push_back(packet);
			}
		}
		{
			// Third: Activate async send, if needed.
			tmpManager->AsyncSendNextPacket();
		}
	}
}

void
SenderHelper::Send(boost::shared_ptr<SessionData> session, const NetPacketList &packetList)
{
	if (!packetList.empty() && session.get())
	{
		boost::shared_ptr<SendDataManager> tmpManager;
		{
			// First: lock map of all queues. Locate/insert queue.
			boost::mutex::scoped_lock lock(m_sendQueueMapMutex);
			SendQueueMap::iterator pos = m_sendQueueMap.find(session->GetId());
			if (pos == m_sendQueueMap.end())
				pos = m_sendQueueMap.insert(SendQueueMap::value_type(session->GetId(), boost::shared_ptr<SendDataManager>(new SendDataManager(session->GetAsioSocket())))).first;
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
					tmpManager->list.push_back(*i);
					++i;
				}
			}
		}
		{
			// Third: Activate async send, if needed.
			tmpManager->AsyncSendNextPacket();
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

