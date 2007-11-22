/***************************************************************************
 *   Copyright (C) 2007 by Lothar May                                      *
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

#include <net/senderthread.h>
#include <net/sendercallback.h>
#include <net/socket_msg.h>
#include <net/socket_helper.h>
#include <core/loghelper.h>
#include <cstring>
#include <cassert>

#include <core/boost/timers.hpp>
#include <boost/bind.hpp>

using namespace std;

#define SEND_ERROR_TIMEOUT_MSEC				20000
#define SEND_TIMEOUT_MSEC					10
#define SEND_QUEUE_SIZE						10000000

SenderThread::SenderThread(SenderCallback &cb)
: m_callback(cb)
{
}

SenderThread::~SenderThread()
{
}

void
SenderThread::Send(boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet)
{
	if (packet.get() && session.get())
	{
		boost::mutex::scoped_lock lock(m_sendQueueMutex);
		InternalStore(m_sendQueue, SEND_QUEUE_SIZE, session, packet);
	}
}

void
SenderThread::Send(boost::shared_ptr<SessionData> session, const NetPacketList &packetList)
{
	if (!packetList.empty() && session.get())
	{
		boost::mutex::scoped_lock lock(m_sendQueueMutex);
		InternalStore(m_sendQueue, SEND_QUEUE_SIZE, session, packetList);
	}
}

unsigned
SenderThread::GetNumPacketsInQueue() const
{
	unsigned numPackets;
	{
		boost::mutex::scoped_lock lock(m_sendQueueMutex);
		numPackets = m_sendQueue.size();
	}
	{
		boost::mutex::scoped_lock lock(m_stalledQueueMutex);
		numPackets += m_stalledQueue.size();
	}
	return numPackets;
}

bool
SenderThread::operator<(const SenderThread &other) const
{
	return GetNumPacketsInQueue() < other.GetNumPacketsInQueue();
}

void
SenderThread::InternalStore(SendDataList &sendQueue, unsigned maxQueueSize, boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet)
{
	if (sendQueue.size() < maxQueueSize) // Queue is limited in size.
		sendQueue.push_back(SendData(packet, session));
	// TODO: Throw exception if failed.
}

void
SenderThread::InternalStore(SendDataList &sendQueue, unsigned maxQueueSize, boost::shared_ptr<SessionData> session, const NetPacketList &packetList)
{
	if (sendQueue.size() + packetList.size() <= maxQueueSize)
	{
		NetPacketList::const_iterator i = packetList.begin();
		NetPacketList::const_iterator end = packetList.end();
		while (i != end)
		{
			sendQueue.push_back(SendData(*i, session));
			++i;
		}
	}
	// TODO: Throw exception if failed.
}

void
SenderThread::Main()
{
	while (!ShouldTerminate())
	{
		/*
		 * To prevent stalling of the sender, keeping the order
		 * of the packets in the sender queue is not guaranteed.
		 * Instead, only the order of the packets for a single
		 * target session is maintained.
		 *
		 * This could also be done by using one sender thread
		 * for each session, but that would require too many
		 * resources.
		 *
		 * The send queue is a list of packets. Whenever a
		 * select timeout occurs, the session of the current
		 * packet is placed in the stalled list, and all other
		 * packets from the queue for that session are also
		 * attached to the stalled list. This process is
		 * continued with the next packet in the send queue.
		 *
		 * When the send queue is empty, the list of stalled
		 * packets is copied back to the send queue, and
		 * everything starts from the beginning.
		 *
		 * No packets are lost in this algorithm, and at the same
		 * time, the send process is never fully stalled,
		 * except when only packets for stalled sessions are
		 * present.
		 *
		 * Note: If someone keeps putting in packets to send, the
		 * stalled packets will never be sent, but this is
		 * considered more a theoretical problem.
		 */

		// For reasons of simplicity, only one packet is sent at a time.
		SendData tmpData;
		// Check main queue.
		{
			boost::mutex::scoped_lock lock(m_sendQueueMutex);
			if (m_sendQueue.empty())
			{
				// Check stalled queue.
				// Attention: double lock (on purpose).
				boost::mutex::scoped_lock lock2(m_stalledQueueMutex);
				if (!m_stalledQueue.empty())
					m_sendQueue.swap(m_stalledQueue);
			}
			if (!m_sendQueue.empty())
			{
				tmpData = m_sendQueue.front();
				m_sendQueue.pop_front();
			}
		}

		if (tmpData.packet && tmpData.session)
		{
			const unsigned tmpLen = tmpData.packet->GetLen();
			if (tmpLen <= MAX_PACKET_SIZE)
			{
				SOCKET tmpSocket = tmpData.session->GetSocket();

				// send next chunk of data
				int bytesSent = send(tmpSocket, ((const char *)tmpData.packet->GetRawData()) + tmpData.bytesSent, tmpLen - tmpData.bytesSent, SOCKET_SEND_FLAGS);

				if (!IS_VALID_SEND(bytesSent))
				{
					// Never assume that this is a fatal error.
					int errCode = SOCKET_ERRNO();
					if (IS_SOCKET_ERR_WOULDBLOCK(errCode))
					{
						fd_set writeSet;
						struct timeval timeout;

						FD_ZERO(&writeSet);
						FD_SET(tmpSocket, &writeSet);

						timeout.tv_sec  = 0;
						timeout.tv_usec = SEND_TIMEOUT_MSEC * 1000;
						int selectResult = select(tmpSocket + 1, NULL, &writeSet, NULL, &timeout);
						if (!IS_VALID_SELECT(selectResult))
						{
							// Never assume that this is a fatal error.
							int errCode = SOCKET_ERRNO();
							if (!IS_SOCKET_ERR_WOULDBLOCK(errCode))
							{
								// Skip this packet - this is bad, and is therefore reported.
								// Ignore invalid or not connected sockets.
								if (errCode != SOCKET_ERR_NOTCONN && errCode != SOCKET_ERR_NOTSOCK)
									m_callback.SignalNetError(tmpData.session->GetId(), ERR_SOCK_SELECT_FAILED, errCode);
							}
							Msleep(SEND_TIMEOUT_MSEC);
						}
						else if (selectResult == 0)
						{
							// A timeout occured - don't block the thread.
							{
								// Attention: double lock (on purpose).
								// Stall all packets for that sender.
								boost::mutex::scoped_lock lock(m_sendQueueMutex);
								boost::mutex::scoped_lock lock2(m_stalledQueueMutex);
								m_stalledQueue.push_back(tmpData);
								SendDataList::iterator i = m_sendQueue.begin();
								SendDataList::iterator end = m_sendQueue.end();
								while (i != end)
								{
									SendDataList::iterator next = i;
									++next;
									if ((*i).session && (*i).session->GetId() == tmpData.session->GetId())
									{
										m_stalledQueue.push_back(*i);
										m_sendQueue.erase(i);
									}
									i = next;
								}
							}
						}
						else
						{
							// Select was successful - store the packet back in the main queue.
							boost::mutex::scoped_lock lock(m_sendQueueMutex);
							m_sendQueue.push_front(tmpData);
						}
					}
					else // other errors than would block
					{
						// Skip this packet - this is bad, and is therefore reported.
						// Ignore invalid or not connected sockets.
						if (errCode != SOCKET_ERR_NOTCONN && errCode != SOCKET_ERR_NOTSOCK)
							m_callback.SignalNetError(tmpData.session->GetId(), ERR_SOCK_SEND_FAILED, errCode);
						Msleep(SEND_TIMEOUT_MSEC);
					}
				}
				else if ((unsigned)bytesSent + tmpData.bytesSent < tmpLen)
				{
					if (bytesSent)
					{
						tmpData.bytesSent += bytesSent;
						// Send was partly successful - store the packet back in the main queue.
						boost::mutex::scoped_lock lock(m_sendQueueMutex);
						m_sendQueue.push_front(tmpData);
					}
					else
						Msleep(SEND_TIMEOUT_MSEC);
				}
			}
			else
				Msleep(SEND_TIMEOUT_MSEC);
		}
		else
			Msleep(SEND_TIMEOUT_MSEC);
	}
}

