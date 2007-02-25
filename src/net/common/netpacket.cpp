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

#include <net/netpacket.h>

NetPacket::~NetPacket()
{
}

//-----------------------------------------------------------------------------

TestNetPacket::TestNetPacket(u_int32_t value)
{
	m_data.head.type = 0;
	m_data.head.length = sizeof(m_data);
	m_data.test = value;
}

TestNetPacket::~TestNetPacket()
{
}

NetPacketHeader *
TestNetPacket::GetData()
{
	return (NetPacketHeader *)&m_data;
}

