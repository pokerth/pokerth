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

using namespace std;

#define SEND_ERROR_TIMEOUT_MSEC				20000
#define SEND_TIMEOUT_MSEC					10
#define SEND_QUEUE_SIZE						10000000
#define SEND_LOG_INTERVAL_SEC				60

SenderThread::SenderThread(SenderCallback &cb)
: m_callback(cb), m_bytesSent(0)
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
		{
			boost::mutex::scoped_lock lock(m_sendQueueMutex);
			if (m_sendQueue.size() < SEND_QUEUE_SIZE)
			{
				m_sendQueue.push_back(packet);
			}
		}
		{
			boost::mutex::scoped_lock lock(m_sessionDataMutex);
			m_sessionSocket = session->GetSocket();
			m_sessionId = session->GetId();
		}
	}
}

void
SenderThread::Send(boost::shared_ptr<SessionData> session, const NetPacketList &packetList)
{
	if (!packetList.empty() && session.get())
	{
		boost::mutex::scoped_lock lock(m_sendQueueMutex);
		if (m_sendQueue.size() + packetList.size() <= SEND_QUEUE_SIZE)
		{
			NetPacketList::const_iterator i = packetList.begin();
			NetPacketList::const_iterator end = packetList.end();
			while (i != end)
			{
				m_sendQueue.push_back(*i);
				++i;
			}
		}
		{
			boost::mutex::scoped_lock lock(m_sessionDataMutex);
			m_sessionSocket = session->GetSocket();
			m_sessionId = session->GetId();
		}
	}
}

void
SenderThread::Main()
{
	while (!ShouldTerminate())
	{
		if (!m_curPacket)
		{
			boost::mutex::scoped_lock lock(m_sendQueueMutex);
			if (!m_sendQueue.empty())
			{
				m_curPacket = m_sendQueue.front();
				m_sendQueue.pop_front();
			}
		}

		if (m_curPacket)
		{
			const unsigned tmpLen = m_curPacket->GetLen();
			if (tmpLen > MAX_PACKET_SIZE)
				m_curPacket.reset(); // TODO log
			else
			{
				SOCKET tmpSocket;
				{
					boost::mutex::scoped_lock lock(m_sessionDataMutex);
					tmpSocket = m_sessionSocket;
				}

				// send next chunk of data
				int bytesSent = send(tmpSocket, ((const char *)m_curPacket->GetRawData()) + m_bytesSent, tmpLen - m_bytesSent, SOCKET_SEND_FLAGS);

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
								{
									boost::mutex::scoped_lock lock(m_sessionDataMutex);
									m_callback.SignalNetError(m_sessionId, ERR_SOCK_SELECT_FAILED, errCode);
								}
							}
							Msleep(SEND_TIMEOUT_MSEC);
						}
					}
					else // other errors than would block
					{
						// Skip this packet - this is bad, and is therefore reported.
						// Ignore invalid or not connected sockets.
						if (errCode != SOCKET_ERR_NOTCONN && errCode != SOCKET_ERR_NOTSOCK)
						{
							boost::mutex::scoped_lock lock(m_sessionDataMutex);
							m_callback.SignalNetError(m_sessionId, ERR_SOCK_SEND_FAILED, errCode);
						}
						Msleep(SEND_TIMEOUT_MSEC);
					}
				}
				else if ((unsigned)bytesSent + m_bytesSent < tmpLen)
				{
					if (bytesSent)
					{
						// Send was partly successful.
						m_bytesSent += bytesSent;
					}
					else
						Msleep(SEND_TIMEOUT_MSEC);
				}
				else //if ((unsigned)bytesSent + m_bytesSent == tmpLen)
				{
					m_curPacket.reset();
					m_bytesSent = 0;
				}
			}
		}
		else
			Msleep(SEND_TIMEOUT_MSEC);
	}
	boost::mutex::scoped_lock lock(m_sessionDataMutex);
	if (m_sessionSocket != INVALID_SOCKET)
		CLOSESOCKET(m_sessionSocket);
}

