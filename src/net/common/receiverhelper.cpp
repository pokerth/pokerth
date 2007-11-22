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

#ifdef _WIN32
	#define FD_SETSIZE 512
#endif

#include <net/receiverhelper.h>
#include <net/socket_msg.h>
#include <net/netexception.h>
#include <core/loghelper.h>
#include <cstring>

using namespace std;


ReceiverHelper::ReceiverHelper()
{
}

ReceiverHelper::~ReceiverHelper()
{
}

boost::shared_ptr<NetPacket>
ReceiverHelper::Recv(SOCKET sock, ReceiveBuffer &buf)
{
	if (buf.receivedPackets.empty())
	{
		int bufSize = RECV_BUF_SIZE - buf.recvBufUsed;

		if (bufSize > 0) // check if there is room in the input buffer
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
				throw NetException(__FILE__, __LINE__, ERR_SOCK_SELECT_FAILED, SOCKET_ERRNO());
			}
			if (selectResult > 0) // recv is possible
			{
				int bytesRecvd = recv(sock, buf.recvBuf + buf.recvBufUsed, bufSize, 0);

				if (!IS_VALID_RECV(bytesRecvd))
				{
					int errCode = SOCKET_ERRNO();
					if (!IS_SOCKET_ERR_WOULDBLOCK(errCode))
						throw NetException(__FILE__, __LINE__, ERR_SOCK_RECV_FAILED, SOCKET_ERRNO());
				}
				else if (bytesRecvd == 0)
				{
					throw NetException(__FILE__, __LINE__, ERR_SOCK_CONN_RESET, 0);
				}
				else
				{
					buf.recvBufUsed += bytesRecvd;
					InternalGetPackets(buf);
				}
			}
		}
	}
	boost::shared_ptr<NetPacket> tmpPacket;
	if (!buf.receivedPackets.empty())
	{
		tmpPacket = buf.receivedPackets.front();
		buf.receivedPackets.pop_front();
	}
	return tmpPacket;
}

void
ReceiverHelper::InternalGetPackets(ReceiveBuffer &buf)
{
	bool dataAvailable = true;
	do
	{
		boost::shared_ptr<NetPacket> tmpPacket;
		// This is necessary, because we use TCP.
		// Packets may be received in multiple chunks or
		// several packets may be received at once.
		if (buf.recvBufUsed >= MIN_PACKET_SIZE)
		{
			try
			{
				// This call will also handle the memmove stuff, i.e.
				// buffering for partial packets.
				tmpPacket = NetPacket::Create(buf.recvBuf, buf.recvBufUsed);
			} catch (const NetException &e)
			{
				// Reset buffer on error.
				buf.recvBufUsed = 0;
				LOG_ERROR(e.what());
			}
		}
		if (tmpPacket.get())
			buf.receivedPackets.push_back(tmpPacket);
		else
			dataAvailable = false;
	} while(dataAvailable);
}

