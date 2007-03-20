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
#include <net/netexception.h>
#include <net/socket_msg.h>

NetPacket::~NetPacket()
{
}

const NetPacketInit *
NetPacket::ToNetPacketInit() const
{
	return NULL;
}

const NetPacketInitAck *
NetPacket::ToNetPacketInitAck() const
{
	return NULL;
}

const NetPacketGameStart *
NetPacket::ToNetPacketGameStart() const
{
	return NULL;
}

//-----------------------------------------------------------------------------

NetPacketInit::NetPacketInit()
{
	Init();
}

NetPacketInit::NetPacketInit(u_int32_t value)
{
	Init();
	m_data.test = htonl(value);
}

NetPacketInit::~NetPacketInit()
{
}

void
NetPacketInit::Init()
{
	m_data.head.type = htons(NET_TYPE_INIT);
	m_data.head.length = htons(sizeof(m_data));
	m_data.test = htonl(0);
}

boost::shared_ptr<NetPacket>
NetPacketInit::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketInit);
	try
	{
		newPacket->SetData(GetData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

const NetPacketHeader *
NetPacketInit::GetData() const
{
	return (const NetPacketHeader *)&m_data;
}

void
NetPacketInit::SetData(const NetPacketHeader *p)
{
	u_int16_t tmpLen = ntohs(p->length);
	if (tmpLen != sizeof(m_data)
		|| ntohs(p->type) != NET_TYPE_INIT)
	{
		throw NetException(ERR_SOCK_INTERNAL, 0);
	}

	memcpy(&m_data, p, tmpLen);
}

const NetPacketInit *
NetPacketInit::ToNetPacketInit() const
{
	return this;
}

//-----------------------------------------------------------------------------

NetPacketInitAck::NetPacketInitAck()
{
	Init();
}

NetPacketInitAck::NetPacketInitAck(u_int32_t value)
{
	Init();
	m_data.test = htonl(value);
}

NetPacketInitAck::~NetPacketInitAck()
{
}

void
NetPacketInitAck::Init()
{
	m_data.head.type = htons(NET_TYPE_INIT_ACK);
	m_data.head.length = htons(sizeof(m_data));
	m_data.test = htonl(0);
}

boost::shared_ptr<NetPacket>
NetPacketInitAck::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketInitAck);
	try
	{
		newPacket->SetData(GetData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

const NetPacketHeader *
NetPacketInitAck::GetData() const
{
	return (const NetPacketHeader *)&m_data;
}

void
NetPacketInitAck::SetData(const NetPacketHeader *p)
{
	u_int16_t tmpLen = ntohs(p->length);
	if (tmpLen != sizeof(m_data)
		|| ntohs(p->type) != NET_TYPE_INIT_ACK)
	{
		throw NetException(ERR_SOCK_INTERNAL, 0);
	}

	memcpy(&m_data, p, tmpLen);
}

const NetPacketInitAck *
NetPacketInitAck::ToNetPacketInitAck() const
{
	return this;
}

//-----------------------------------------------------------------------------

NetPacketGameStart::NetPacketGameStart()
{
	Init();
}

NetPacketGameStart::NetPacketGameStart(u_int32_t value)
{
	Init();
	m_data.test = htonl(value);
}

NetPacketGameStart::~NetPacketGameStart()
{
}

void
NetPacketGameStart::Init()
{
	m_data.head.type = htons(NET_TYPE_GAME_START);
	m_data.head.length = htons(sizeof(m_data));
	m_data.test = htonl(0);
}

boost::shared_ptr<NetPacket>
NetPacketGameStart::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketGameStart);
	try
	{
		newPacket->SetData(GetData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

const NetPacketHeader *
NetPacketGameStart::GetData() const
{
	return (NetPacketHeader *)&m_data;
}

void
NetPacketGameStart::SetData(const NetPacketHeader *p)
{
	u_int16_t tmpLen = ntohs(p->length);
	if (tmpLen != sizeof(m_data)
		|| ntohs(p->type) != NET_TYPE_GAME_START)
	{
		throw NetException(ERR_SOCK_INTERNAL, 0);
	}

	memcpy(&m_data, p, tmpLen);
}

const NetPacketGameStart *
NetPacketGameStart::ToNetPacketGameStart() const
{
	return this;
}

