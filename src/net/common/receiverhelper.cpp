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
ReceiverHelper::Recv(SOCKET sock)
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
			FD_SET(sock, &readSet);

			timeout.tv_sec  = 0;
			timeout.tv_usec = RECV_TIMEOUT_MSEC * 1000;
			int selectResult = select(sock + 1, &readSet, NULL, NULL, &timeout);
			if (!IS_VALID_SELECT(selectResult))
			{
				throw NetException(ERR_SOCK_SELECT_FAILED, SOCKET_ERRNO());
			}
			if (selectResult > 0) // recv is possible
			{
				int bytesRecvd = recv(sock, m_tmpInBuf + m_tmpInBufSize, bufSize, 0);

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
	if (m_tmpInBufSize >= MIN_PACKET_SIZE)
	{
		try
		{
			// This call will also handle the memmove stuff, i.e.
			// buffering for partial packets.
			tmpPacket = NetPacket::Create(m_tmpInBuf, m_tmpInBufSize);
		} catch (const NetException &)
		{
			// Reset buffer on error.
			m_tmpInBufSize = 0;
			// TODO: log error/increase error counter.
		}
	}
	return tmpPacket;
}

