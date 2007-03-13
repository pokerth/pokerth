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
#include <net/netcallback.h>
#include <net/socket_msg.h>
#include <cstring>

using namespace std;

#define SEND_TIMEOUT_MSEC 50

SenderThread::SenderThread(NetCallback &cb)
: m_tmpOutBufSize(0), m_callback(cb)
{
}

SenderThread::~SenderThread()
{
}

void
SenderThread::Init(SOCKET socket)
{
	if (!IS_VALID_SOCKET(socket) || IsRunning())
		return; // TODO: throw exception

	m_socket = socket;
}

void
SenderThread::Send(boost::shared_ptr<NetPacket> packet)
{
	boost::mutex::scoped_lock lock(m_outBufMutex);
	m_outBuf.push_back(packet);
}

void
SenderThread::Main()
{
	while (!ShouldTerminate())
	{
		// Send remaining bytes of output buffer OR
		// copy ONE packet to output buffer.
		// For reasons of simplicity, only one packet is sent at a time.
		if (!m_tmpOutBufSize)
		{
			boost::shared_ptr<NetPacket> tmpPacket;
			{
				boost::mutex::scoped_lock lock(m_outBufMutex);
				if (!m_outBuf.empty())
				{
					tmpPacket = m_outBuf.front();
					m_outBuf.pop_front();
				}
			}
			if (tmpPacket.get())
			{
				u_int16_t tmpLen = ntohs(tmpPacket->GetData()->length);
				if (tmpLen <= MAX_PACKET_SIZE)
				{
					m_tmpOutBufSize = tmpLen;
					memcpy(m_tmpOutBuf, tmpPacket->GetData(), m_tmpOutBufSize);
				}
			}
		}
		if (m_tmpOutBufSize)
		{
			fd_set writeSet;
			struct timeval timeout;

			FD_ZERO(&writeSet);
			FD_SET(m_socket, &writeSet);

			timeout.tv_sec  = 0;
			timeout.tv_usec = SEND_TIMEOUT_MSEC * 1000;
			int selectResult = select(m_socket + 1, NULL, &writeSet, NULL, &timeout);
			if (!IS_VALID_SELECT(selectResult))
			{
				m_callback.SignalNetError(ERR_SOCK_SELECT_FAILED, SOCKET_ERRNO());
				// Assume that this is a fatal error, terminate thread.
				return;
			}
			if (selectResult > 0) // send is possible
			{
				// send next chunk of data
				int bytesSent = send(m_socket, m_tmpOutBuf, m_tmpOutBufSize, 0);

				if (!IS_VALID_SEND(bytesSent))
				{
					m_callback.SignalNetError(ERR_SOCK_SEND_FAILED, SOCKET_ERRNO());
					// Assume that this is a fatal error, terminate thread.
					return;
				}
				else if ((unsigned)bytesSent < m_tmpOutBufSize)
				{
					m_tmpOutBufSize = m_tmpOutBufSize - (unsigned)bytesSent;
					memmove(m_tmpOutBuf, m_tmpOutBuf + bytesSent, m_tmpOutBufSize);
				}
				else
				{
					m_tmpOutBufSize = 0;
				}
			}
		}
		else
			Msleep(SEND_TIMEOUT_MSEC);
	}
}

