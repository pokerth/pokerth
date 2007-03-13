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

#include <net/receiverhelper.h>
#include <net/socket_msg.h>
#include <net/netexception.h>
#include <cstring>

using namespace std;

#define RECV_TIMEOUT_MSEC 50

ReceiverHelper::ReceiverHelper()
: m_socket(INVALID_SOCKET), m_tmpInBufSize(0)
{
}

ReceiverHelper::~ReceiverHelper()
{
}

void
ReceiverHelper::Init(SOCKET socket)
{
	if (!IS_VALID_SOCKET(socket))
		return; // TODO: throw exception

	m_socket = socket;
}

boost::shared_ptr<NetPacket>
ReceiverHelper::Recv()
{
	boost::shared_ptr<NetPacket> tmpPacket(InternalGetPacket());

	if (!tmpPacket.get())
	{
		unsigned bufSize = RECV_BUF_SIZE - m_tmpInBufSize;

		if (bufSize) // check if there is room in the input buffer
		{
			fd_set readSet;
			struct timeval timeout;

			FD_ZERO(&readSet);
			FD_SET(m_socket, &readSet);

			timeout.tv_sec  = 0;
			timeout.tv_usec = RECV_TIMEOUT_MSEC * 1000;
			int selectResult = select(m_socket + 1, &readSet, NULL, NULL, &timeout);
			if (!IS_VALID_SELECT(selectResult))
			{
				throw NetException(ERR_SOCK_SELECT_FAILED, SOCKET_ERRNO());
			}
			if (selectResult > 0) // recv is possible
			{
				int bytesRecvd = recv(m_socket, m_tmpInBuf + m_tmpInBufSize, bufSize, 0);

				if (!IS_VALID_RECV(bytesRecvd))
				{
					throw NetException(ERR_SOCK_RECV_FAILED, SOCKET_ERRNO());
				}
				else if (bytesRecvd == 0)
				{
					throw NetException(ERR_SOCK_CONN_RESET, 0);
				}
				else
				{
					m_tmpInBufSize += bytesRecvd;
					tmpPacket = InternalGetPacket();
				}
			}
		}
	}
	return tmpPacket;
}

boost::shared_ptr<NetPacket>
ReceiverHelper::InternalGetPacket()
{
	boost::shared_ptr<NetPacket> tmpPacket;

	// This is necessary, because we use TCP.
	// Packets may be received in multiple chunks or
	// several packets may be received at once.
	if (m_tmpInBufSize >= sizeof(NetPacketHeader))
	{
		NetPacketHeader *tmpHeader = (NetPacketHeader *)m_tmpInBuf;
		u_int16_t tmpLen = ntohs(tmpHeader->length);

		if (tmpLen < sizeof(NetPacketHeader)
			|| tmpLen > MAX_PACKET_SIZE)
		{
			// Invalid packet - reset input buffer.
			m_tmpInBufSize = 0;
		}
		else if (m_tmpInBufSize >= tmpLen)
		{
			tmpPacket = InternalCreateNetPacket(tmpHeader);
			m_tmpInBufSize -= tmpLen;
		}
	}
	return tmpPacket;
}

boost::shared_ptr<NetPacket>
ReceiverHelper::InternalCreateNetPacket(const NetPacketHeader *p)
{
	boost::shared_ptr<NetPacket> tmpPacket;

	switch(ntohs(p->type))
	{
		case NET_TYPE_TEST:
			try
			{
				tmpPacket = boost::shared_ptr<NetPacket>(new TestNetPacket);
				tmpPacket->SetData(p);
			} catch (const NetException &)
			{
				tmpPacket.reset();
			}
			break;
	}
	return tmpPacket;
}

