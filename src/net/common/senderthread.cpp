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
#include <cstring>

#include <core/boost/timers.hpp>
#include <boost/bind.hpp>

using namespace std;

#define SEND_ERROR_NORMAL_TIMEOUT_MSEC		20000
#define SEND_ERROR_LOW_PRIO_TIMEOUT_MSEC	10000
#define SEND_TIMEOUT_MSEC					10
#define SEND_QUEUE_SIZE						1000
#define SEND_LOW_PRIO_QUEUE_SIZE			50000

SenderThread::SenderThread(SenderCallback &cb)
: m_tmpOutBufSize(0), m_tmpIsLowPrio(false), m_callback(cb)
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
		boost::mutex::scoped_lock lock(m_outBufMutex);
		InternalStore(m_outBuf, SEND_QUEUE_SIZE, session, packet);
	}
}

void
SenderThread::Send(boost::shared_ptr<SessionData> session, const NetPacketList &packetList)
{
	if (!packetList.empty() && session.get())
	{
		boost::mutex::scoped_lock lock(m_outBufMutex);
		InternalStore(m_outBuf, SEND_QUEUE_SIZE, session, packetList);
	}
}

void
SenderThread::SendLowPrio(boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet)
{
	if (packet.get() && session.get())
	{
		boost::mutex::scoped_lock lock(m_lowPrioOutBufMutex);
		InternalStore(m_lowPrioOutBuf, SEND_LOW_PRIO_QUEUE_SIZE, session, packet);
	}
}

void
SenderThread::SendLowPrio(boost::shared_ptr<SessionData> session, const NetPacketList &packetList)
{
	if (!packetList.empty() && session.get())
	{
		boost::mutex::scoped_lock lock(m_lowPrioOutBufMutex);
		InternalStore(m_lowPrioOutBuf, SEND_LOW_PRIO_QUEUE_SIZE, session, packetList);
	}
}

void
SenderThread::InternalStore(SendDataDeque &sendQueue, unsigned maxQueueSize, boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet)
{
	if (sendQueue.size() < maxQueueSize) // Queue is limited in size.
		sendQueue.push_back(std::make_pair(packet, session));
	// TODO: Throw exception if failed.
}

void
SenderThread::InternalStore(SendDataDeque &sendQueue, unsigned maxQueueSize, boost::shared_ptr<SessionData> session, const NetPacketList &packetList)
{
	if (sendQueue.size() + packetList.size() < maxQueueSize)
	{
		NetPacketList::const_iterator i = packetList.begin();
		NetPacketList::const_iterator end = packetList.end();
		while (i != end)
		{
			sendQueue.push_back(std::make_pair(*i, session));
			++i;
		}
	}
	// TODO: Throw exception if failed.
}

void 
SenderThread::RemoveCurSendData()
{
	m_tmpOutBufSize = 0;
	m_curSession.reset();
	// TODO use callback to remove session.
}

void
SenderThread::Main()
{
	boost::timers::portable::microsec_timer sendTimer(boost::posix_time::time_duration(0, 0, 0), boost::timers::portable::microsec_timer::manual_start);

	while (!ShouldTerminate())
	{
		// Send remaining bytes of output buffer OR
		// copy ONE packet to output buffer.
		// For reasons of simplicity, only one packet is sent at a time.
		if (!m_tmpOutBufSize)
		{
			SendData tmpData;
			// Check main queue first.
			{
				boost::mutex::scoped_lock lock(m_outBufMutex);
				if (!m_outBuf.empty())
				{
					tmpData = m_outBuf.front();
					m_outBuf.pop_front();
					m_tmpIsLowPrio = false;
				}
			}

			// Check low prio queue only if there is nothing in the main queue.
			if (!tmpData.first.get())
			{
				boost::mutex::scoped_lock lock(m_lowPrioOutBufMutex);
				if (!m_lowPrioOutBuf.empty())
				{
					tmpData = m_lowPrioOutBuf.front();
					m_lowPrioOutBuf.pop_front();
					m_tmpIsLowPrio = true;
				}
			}

			if (tmpData.first.get())
			{
				if (tmpData.second.get())
				{
					u_int16_t tmpLen = tmpData.first->GetLen();
					if (tmpLen <= MAX_PACKET_SIZE)
					{
						m_curSession = tmpData.second;
						m_tmpOutBufSize = tmpLen;
						memcpy(m_tmpOutBuf, tmpData.first->GetRawData(), tmpLen);
						sendTimer.restart();
					}
				}
			}
		}
		if (m_tmpOutBufSize)
		{
			SOCKET tmpSocket = m_curSession->GetSocket();

			// send next chunk of data
			int bytesSent = send(tmpSocket, m_tmpOutBuf, m_tmpOutBufSize, SOCKET_SEND_FLAGS);

			if (!IS_VALID_SEND(bytesSent))
			{
				// Never assume that this is a fatal error.
				int errCode = SOCKET_ERRNO();
				if (errCode == SOCKET_ERR_WOULDBLOCK)
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
						if (errCode != SOCKET_ERR_WOULDBLOCK)
						{
							// Skip this packet - this is bad, and is therefore reported.
							// Ignore invalid or not connected sockets.
							if (errCode != SOCKET_ERR_NOTCONN && errCode != SOCKET_ERR_NOTSOCK)
								m_callback.SignalNetError(m_curSession->GetId(), ERR_SOCK_SELECT_FAILED, errCode);
							RemoveCurSendData();
						}
						Msleep(SEND_TIMEOUT_MSEC);
					}
				}
				else
				{
					// Skip this packet - this is bad, and is therefore reported.
					// Ignore invalid or not connected sockets.
					if (errCode != SOCKET_ERR_NOTCONN && errCode != SOCKET_ERR_NOTSOCK)
						m_callback.SignalNetError(m_curSession->GetId(), ERR_SOCK_SEND_FAILED, errCode);
					RemoveCurSendData();
				}
				Msleep(SEND_TIMEOUT_MSEC);
			}
			else if ((unsigned)bytesSent < m_tmpOutBufSize)
			{
				if (bytesSent)
				{
					m_tmpOutBufSize -= (unsigned)bytesSent;
					memmove(m_tmpOutBuf, m_tmpOutBuf + bytesSent, m_tmpOutBufSize);
				}
				else
					Msleep(SEND_TIMEOUT_MSEC);
			}
			else
			{
				m_tmpOutBufSize = 0;
				m_curSession.reset();
				sendTimer.reset();
			}
		}
		else
			Msleep(SEND_TIMEOUT_MSEC);

		// Check whether the send timed out.
		if (sendTimer.is_running())
		{
			bool doRemove = false;

			unsigned msec = static_cast<unsigned>(sendTimer.elapsed().total_milliseconds());
			if (m_tmpIsLowPrio)
			{
				if (msec > SEND_ERROR_LOW_PRIO_TIMEOUT_MSEC)
					doRemove = true;
			}
			else
			{
				if (msec > SEND_ERROR_NORMAL_TIMEOUT_MSEC)
					doRemove = true;
			}
			if (doRemove)
			{
				RemoveCurSendData();
				sendTimer.reset();
			}
		}
	}
}

