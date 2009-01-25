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
#include <boost/asio.hpp>

using namespace std;
using boost::asio::ip::tcp;


#define SEND_ERROR_TIMEOUT_MSEC				20000
#define SEND_TIMEOUT_MSEC					10
#define SEND_QUEUE_SIZE						10000000
#define SEND_LOG_INTERVAL_SEC				60



void
SenderThread::SendDataManager::HandleWrite(const boost::system::error_code& error)
{
	SetWriteInProgress(false);
	SetCompleted(true);
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
	Run();
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
		boost::mutex::scoped_lock lock(m_sendQueueMapMutex);
		SendQueueMap::iterator pos = m_sendQueueMap.find(session->GetId());
		if (pos == m_sendQueueMap.end())
			pos = m_sendQueueMap.insert(SendQueueMap::value_type(session->GetId(), boost::shared_ptr<SendDataManager>(new SendDataManager(session, m_ioService)))).first;
		if (pos->second->list.size() < SEND_QUEUE_SIZE)
			pos->second->list.push_back(packet);
	}
}

void
SenderThread::Send(boost::shared_ptr<SessionData> session, const NetPacketList &packetList)
{
	if (!packetList.empty() && session.get())
	{
		boost::mutex::scoped_lock lock(m_sendQueueMapMutex);
		SendQueueMap::iterator pos = m_sendQueueMap.find(session->GetId());
		if (pos == m_sendQueueMap.end())
			pos = m_sendQueueMap.insert(SendQueueMap::value_type(session->GetId(), boost::shared_ptr<SendDataManager>(new SendDataManager(session, m_ioService)))).first;
		if (pos->second->list.size() + packetList.size() <= SEND_QUEUE_SIZE)
		{
			NetPacketList::const_iterator i = packetList.begin();
			NetPacketList::const_iterator end = packetList.end();
			while (i != end)
			{
				pos->second->list.push_back(*i);
				++i;
			}
		}
	}
}

void
SenderThread::Main()
{
	while (!ShouldTerminate())
	{
		{
			boost::mutex::scoped_lock lock(m_sendQueueMapMutex);
			SendQueueMap::iterator i = m_sendQueueMap.begin();
			SendQueueMap::iterator end = m_sendQueueMap.end();
			while (i != end)
			{
				SendQueueMap::iterator next = i;
				++next;
				boost::shared_ptr<SendDataManager> tmpManager = i->second;
				if (tmpManager->list.empty())
					m_sendQueueMap.erase(i);
				else
				{
					if (!tmpManager->IsWriteInProgress())
					{
						if (tmpManager->IsCompleted())
							tmpManager->list.pop_front();
						else
						{
							boost::shared_ptr<NetPacket> tmpPacket = tmpManager->list.front();
							boost::asio::async_write(
								*tmpManager->socket,
								boost::asio::buffer(tmpPacket->GetRawData(),
									tmpPacket->GetLen()),
								boost::bind(&SendDataManager::HandleWrite, tmpManager,
								boost::asio::placeholders::error));
							tmpManager->SetWriteInProgress(true);
						}
					}
				}
				i = next;
			}
			m_ioService.run_one();
			Msleep(SEND_TIMEOUT_MSEC);
		}
	}
}

