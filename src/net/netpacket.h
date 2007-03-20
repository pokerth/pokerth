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

#define MAX_PACKET_SIZE		256

#define NET_TYPE_INIT			0
#define NET_TYPE_INIT_ACK		1
#define NET_TYPE_GAME_START		2

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

struct NetPacketInitData
{
	NetPacketHeader head;
	u_int32_t	test;
};

struct NetPacketInitAckData
{
	NetPacketHeader head;
	u_int32_t	test;
};

struct NetPacketGameStartData
{
	NetPacketHeader head;
	u_int32_t	test;
};

#ifdef _MSC_VER
	#pragma pack(pop)
#else
	#pragma align 0
#endif

class NetPacketInit;
class NetPacketInitAck;
class NetPacketGameStart;

class NetPacket
{
public:
	virtual ~NetPacket();

	virtual void SetData(const NetPacketHeader *p) = 0;
	virtual const NetPacketHeader *GetData() const = 0;

	virtual const NetPacketInit *ToNetPacketInit() const;
	virtual const NetPacketInitAck *ToNetPacketInitAck() const;
	virtual const NetPacketGameStart *ToNetPacketGameStart() const;
};

class NetPacketInit : public NetPacket
{
public:
	NetPacketInit();
	NetPacketInit(u_int32_t value);
	virtual ~NetPacketInit();

	virtual const NetPacketHeader *GetData() const;
	virtual void SetData(const NetPacketHeader *p);

	virtual const NetPacketInit *ToNetPacketInit() const;

protected:
	void Init();

private:
	NetPacketInitData m_data;
};

class NetPacketInitAck : public NetPacket
{
public:
	NetPacketInitAck();
	NetPacketInitAck(u_int32_t value);
	virtual ~NetPacketInitAck();

	virtual const NetPacketHeader *GetData() const;
	virtual void SetData(const NetPacketHeader *p);

	virtual const NetPacketInitAck *ToNetPacketInitAck() const;

protected:
	void Init();

private:
	NetPacketInitAckData m_data;
};

class NetPacketGameStart : public NetPacket
{
public:
	NetPacketGameStart();
	NetPacketGameStart(u_int32_t value);
	virtual ~NetPacketGameStart();

	virtual const NetPacketHeader *GetData() const;
	virtual void SetData(const NetPacketHeader *p);

	virtual const NetPacketGameStart *ToNetPacketGameStart() const;

protected:
	void Init();

private:
	NetPacketGameStartData m_data;
};

#endif

