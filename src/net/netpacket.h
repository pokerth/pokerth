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
/* PokerTH network packet. */

#ifndef _NETPACKET_H_
#define _NETPACKET_H_

#include <string>
#include <net/socket_helper.h>

#ifdef _MSC_VER
	#pragma pack(push, 2)
#else
	#pragma align 2
#endif

struct NetPacketHeader
{
	u_int16_t	type;
	u_int16_t	length;
};

struct NetPacketInit
{
	NetPacketHeader head;
	u_int32_t	test;
};

#ifdef _MSC_VER
	#pragma pack(pop)
#else
	#pragma align 0
#endif


class NetPacket
{
public:
	virtual ~NetPacket();

	virtual NetPacketHeader *GetData() = 0;
};

class TestNetPacket : public NetPacket
{
public:
	TestNetPacket(u_int32_t value);
	virtual ~TestNetPacket();

	virtual NetPacketHeader *GetData();

protected:
	NetPacketInit m_data;
};

#endif

