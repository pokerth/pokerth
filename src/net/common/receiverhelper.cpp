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

using namespace std;


ReceiverHelper::ReceiverHelper()
{
}

ReceiverHelper::~ReceiverHelper()
{
}

void
ReceiverHelper::ScanPackets(ReceiveBuffer &buf)
{
	bool dataAvailable = true;
	do
	{
		boost::shared_ptr<NetPacket> tmpPacket;
		// This is necessary, because we use TCP.
		// Packets may be received in multiple chunks or
		// several packets may be received at once.
		if (buf.recvBufUsed)
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
		if (tmpPacket)
		{
			//cerr << "IN:" << endl << tmpPacket->ToString() << endl;
			buf.receivedPackets.push_back(tmpPacket);
		}
		else
			dataAvailable = false;
	} while(dataAvailable);
}

