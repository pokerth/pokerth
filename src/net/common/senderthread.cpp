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
#include <net/senderthread.h>
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
		SendDataManager(boost::shared_ptr<SessionData> s)
		: session(s), writeInProgress(false)
		{
		}

		void HandleWrite(const boost::system::error_code& error);

		void AsyncSendNextPacket(bool handlerMode = false);

		boost::shared_ptr<SessionData> session;

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
				*session->GetAsioSocket(),
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

SenderThread::SenderThread(SenderCallback &cb)
: m_callback(cb)
{
}

SenderThread::~SenderThread()
{
}

void
SenderThread::Start()
{
	m_ioServiceBarrier.reset(new boost::barrier(2));
	Run();
	m_ioServiceBarrier->wait();
}

void
SenderThread::SignalStop()
{
	SignalTermination();
}

void
SenderThread::WaitStop()
{
	Join(SENDER_THREAD_TERMINATE_TIMEOUT);
}

void
SenderThread::Send(boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet)
{
	if (packet.get() && session.get())
	{
		boost::shared_ptr<SendDataManager> tmpManager;
		{
			// First: lock map of all queues. Locate/insert queue.
			boost::mutex::scoped_lock lock(m_sendQueueMapMutex);
			SendQueueMap::iterator pos = m_sendQueueMap.find(session->GetId());
			if (pos == m_sendQueueMap.end())
				pos = m_sendQueueMap.insert(SendQueueMap::value_type(session->GetId(), boost::shared_ptr<SendDataManager>(new SendDataManager(session)))).first;
			tmpManager = pos->second;
		}
		{
			// Second: Add packet to specific queue.
			boost::mutex::scoped_lock lock(tmpManager->dataMutex);
			if (tmpManager->list.size() < SEND_QUEUE_SIZE)
				tmpManager->list.push_back(packet);
		}
		{
			// Third: Update notification list.
			boost::mutex::scoped_lock lock(m_changedSessionsMutex);
			m_changedSessions.push_back(tmpManager->session->GetId());
		}
	}
}

void
SenderThread::Send(boost::shared_ptr<SessionData> session, const NetPacketList &packetList)
{
	if (!packetList.empty() && session.get())
	{
		boost::shared_ptr<SendDataManager> tmpManager;
		{
			// First: lock map of all queues. Locate/insert queue.
			boost::mutex::scoped_lock lock(m_sendQueueMapMutex);
			SendQueueMap::iterator pos = m_sendQueueMap.find(session->GetId());
			if (pos == m_sendQueueMap.end())
				pos = m_sendQueueMap.insert(SendQueueMap::value_type(session->GetId(), boost::shared_ptr<SendDataManager>(new SendDataManager(session)))).first;
			tmpManager = pos->second;
		}
		{
			// Second: Add packet to specific queue.
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
			// Third: Update notification list.
			boost::mutex::scoped_lock lock(m_changedSessionsMutex);
			m_changedSessions.push_back(tmpManager->session->GetId());
		}
	}
}

boost::shared_ptr<boost::asio::io_service>
SenderThread::GetIOService()
{
	return m_ioService;
}

void
SenderThread::Main()
{
	m_ioService.reset(new boost::asio::io_service());
	m_ioServiceBarrier->wait();
	boost::asio::io_service::work ioWork(*m_ioService);
	while (!ShouldTerminate())
	{
		bool sessionValid;
		do
		{
			sessionValid = false;
			unsigned sessionId;

			{
				boost::mutex::scoped_lock lock(m_changedSessionsMutex);
				if (!m_changedSessions.empty())
				{
					sessionId = m_changedSessions.front();
					m_changedSessions.pop_front();
					sessionValid = true;
				}
			}
			boost::shared_ptr<SendDataManager> tmpManager;
			if (sessionValid)
			{
				boost::mutex::scoped_lock lock(m_sendQueueMapMutex);
				SendQueueMap::iterator pos = m_sendQueueMap.find(sessionId);
				if (pos != m_sendQueueMap.end())
					tmpManager = pos->second;
			}
			if (tmpManager)
				tmpManager->AsyncSendNextPacket();
		} while (sessionValid);

		m_ioService->poll();
		Msleep(SEND_TIMEOUT_MSEC);
	}
}

