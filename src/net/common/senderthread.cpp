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
#include <net/netpacket.h>

#define SEND_TIMEOUT_MSEC 50

SenderThread::SenderThread()
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
	boost::shared_ptr<NetPacket> tmpPacket;
	while (!ShouldTerminate())
	{
		if (!tmpPacket.get())
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
			fd_set writeSet;
			struct timeval timeout;

			FD_ZERO(&writeSet);
			FD_SET(m_socket, &writeSet);

			timeout.tv_sec  = 0;
			timeout.tv_usec = SEND_TIMEOUT_MSEC * 1000;
			int selectResult = select(m_socket + 1, NULL, &writeSet, NULL, &timeout);
			if (selectResult > 0) // send is possible
			{
				send(m_socket, (const char *)tmpPacket->GetData(), tmpPacket->GetData()->length, 0);
				tmpPacket.reset();
			}
		}
		else
			Msleep(SEND_TIMEOUT_MSEC);
	}
}

