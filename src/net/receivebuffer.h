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
/* Buffer for ReceiveHelper. */

#ifndef _RECEIVEBUFFER_H_
#define _RECEIVEBUFFER_H_

#include <net/netpacket.h>

// MUST be larger than MAX_PACKET_SIZE
#define RECV_BUF_SIZE		2 * MAX_PACKET_SIZE

struct ReceiveBuffer
{
	ReceiveBuffer() : recvBufUsed(0) {}
	NetPacketList					receivedPackets;
	char							recvBuf[RECV_BUF_SIZE];
	unsigned						recvBufUsed;
};

#endif
