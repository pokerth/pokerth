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

#include <playerdata.h>
#include <net/socket_helper.h>

#define MIN_PACKET_SIZE				4
#define MAX_PACKET_SIZE				256
#define MAX_NAME_SIZE				64
#define MAX_PASSWORD_SIZE			64


enum JoinGameErrorReason
{
	JOIN_UNSUPPORTED_VERSION,
	JOIN_SERVER_FULL,
	JOIN_GAME_RUNNING,
	JOIN_INVALID_PASSWORD,
	JOIN_UNKNOWN
};

struct NetPacketHeader;

class NetPacketJoinGame;
class NetPacketJoinGameAck;
class NetPacketJoinGameError;
class NetPacketGameStart;

class NetPacket
{
public:
	static boost::shared_ptr<NetPacket> Create(char *data, unsigned &dataSize);

	NetPacket(u_int16_t type, u_int16_t initialLen);
	virtual ~NetPacket();

	virtual boost::shared_ptr<NetPacket> Clone() const = 0;

	const NetPacketHeader *GetRawData() const;
	NetPacketHeader *GetRawData();
	void SetRawData(const NetPacketHeader *p);

	u_int16_t GetType() const;
	u_int16_t GetLen() const;

	virtual const NetPacketJoinGame *ToNetPacketJoinGame() const;
	virtual const NetPacketJoinGameAck *ToNetPacketJoinGameAck() const;
	virtual const NetPacketJoinGameError *ToNetPacketJoinGameError() const;
	virtual const NetPacketGameStart *ToNetPacketGameStart() const;

protected:

	virtual void Check(const NetPacketHeader* data) const = 0;

	void Resize(u_int16_t newLen);

private:

	NetPacketHeader *m_data;
};

class NetPacketJoinGame : public NetPacket
{
public:
	struct Data
	{
		PlayerType ptype;
		std::string playerName;
		std::string password;
	};

	NetPacketJoinGame();
	virtual ~NetPacketJoinGame();

	virtual boost::shared_ptr<NetPacket> Clone() const;

	void SetData(const Data &inData);
	void GetData(Data &outData) const;

	virtual const NetPacketJoinGame *ToNetPacketJoinGame() const;

protected:

	virtual void Check(const NetPacketHeader* data) const;
};

class NetPacketJoinGameAck : public NetPacket
{
public:
	struct Data
	{
		u_int32_t	sessionId;
		u_int16_t	playerId;
		u_int16_t	playerNumber;
		u_int16_t	numberOfPlayers;
		u_int16_t	smallBlind;
		u_int16_t	handsBeforeRaise;
		u_int16_t	gameSpeed;
		u_int32_t	startCash;
	};

	NetPacketJoinGameAck();
	virtual ~NetPacketJoinGameAck();

	virtual boost::shared_ptr<NetPacket> Clone() const;

	void SetData(const Data &inData);
	void GetData(Data &outData) const;

	virtual const NetPacketJoinGameAck *ToNetPacketJoinGameAck() const;

protected:

	virtual void Check(const NetPacketHeader* data) const;
};

class NetPacketJoinGameError : public NetPacket
{
public:
	struct Data
	{
		JoinGameErrorReason	reason;
	};

	NetPacketJoinGameError();
	virtual ~NetPacketJoinGameError();

	virtual boost::shared_ptr<NetPacket> Clone() const;

	void SetData(const Data &inData);
	void GetData(Data &outData) const;

	virtual const NetPacketJoinGameError *ToNetPacketJoinGameError() const;

protected:

	virtual void Check(const NetPacketHeader* data) const;
};

class NetPacketGameStart : public NetPacket
{
public:
	struct Data
	{
		u_int16_t	yourCards[2];
	};

	NetPacketGameStart();
	virtual ~NetPacketGameStart();

	virtual boost::shared_ptr<NetPacket> Clone() const;

	void SetData(const Data &inData);
	void GetData(Data &outData) const;

	virtual const NetPacketGameStart *ToNetPacketGameStart() const;

protected:

	virtual void Check(const NetPacketHeader* data) const;
};

#endif

