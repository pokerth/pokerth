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
#include <game_defs.h>
#include <gamedata.h>
#include <net/socket_helper.h>

#define NET_VERSION_MAJOR			1
#define NET_VERSION_MINOR			0

#define MIN_PACKET_SIZE				4
#define MAX_PACKET_SIZE				256
#define MAX_NAME_SIZE				64
#define MAX_PASSWORD_SIZE			64


struct NetPacketHeader;

class NetPacketJoinGame;
class NetPacketJoinGameAck;
class NetPacketPlayerJoined;
class NetPacketPlayerLeft;
class NetPacketGameStart;
class NetPacketHandStart;
class NetPacketPlayersTurn;
class NetPacketPlayersAction;
class NetPacketPlayersActionDone;
class NetPacketPlayersActionRejected;
class NetPacketError;

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
	virtual const NetPacketPlayerJoined *ToNetPacketPlayerJoined() const;
	virtual const NetPacketPlayerLeft *ToNetPacketPlayerLeft() const;
	virtual const NetPacketGameStart *ToNetPacketGameStart() const;
	virtual const NetPacketHandStart *ToNetPacketHandStart() const;
	virtual const NetPacketPlayersTurn *ToNetPacketPlayersTurn() const;
	virtual const NetPacketPlayersAction *ToNetPacketPlayersAction() const;
	virtual const NetPacketPlayersActionDone *ToNetPacketPlayersActionDone() const;
	virtual const NetPacketPlayersActionRejected *ToNetPacketPlayersActionRejected() const;
	virtual const NetPacketError *ToNetPacketError() const;

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
		int versionMajor;
		int versionMinor;
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
		int16_t		yourPlayerNum;
		u_int16_t	yourPlayerUniqueId;
		GameData	gameData;
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

class NetPacketPlayerJoined : public NetPacket
{
public:
	struct Data
	{
		u_int16_t	playerId;
		u_int16_t	playerNumber;
		PlayerType ptype;
		std::string playerName;
	};

	NetPacketPlayerJoined();
	virtual ~NetPacketPlayerJoined();

	virtual boost::shared_ptr<NetPacket> Clone() const;

	void SetData(const Data &inData);
	void GetData(Data &outData) const;

	virtual const NetPacketPlayerJoined *ToNetPacketPlayerJoined() const;

protected:

	virtual void Check(const NetPacketHeader* data) const;
};

class NetPacketPlayerLeft : public NetPacket
{
public:
	struct Data
	{
		u_int16_t	playerId;
	};

	NetPacketPlayerLeft();
	virtual ~NetPacketPlayerLeft();

	virtual boost::shared_ptr<NetPacket> Clone() const;

	void SetData(const Data &inData);
	void GetData(Data &outData) const;

	virtual const NetPacketPlayerLeft *ToNetPacketPlayerLeft() const;

protected:

	virtual void Check(const NetPacketHeader* data) const;
};

class NetPacketGameStart : public NetPacket
{
public:
	struct Data
	{
		StartData	startData;
		u_int16_t	reserved;
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

class NetPacketHandStart : public NetPacket
{
public:
	struct Data
	{
		u_int16_t	yourCards[2];
	};

	NetPacketHandStart();
	virtual ~NetPacketHandStart();

	virtual boost::shared_ptr<NetPacket> Clone() const;

	void SetData(const Data &inData);
	void GetData(Data &outData) const;

	virtual const NetPacketHandStart *ToNetPacketHandStart() const;

protected:

	virtual void Check(const NetPacketHeader* data) const;
};

class NetPacketPlayersTurn : public NetPacket
{
public:
	struct Data
	{
		GameState	gameState;
		u_int16_t	playerId;
	};

	NetPacketPlayersTurn();
	virtual ~NetPacketPlayersTurn();

	virtual boost::shared_ptr<NetPacket> Clone() const;

	void SetData(const Data &inData);
	void GetData(Data &outData) const;

	virtual const NetPacketPlayersTurn *ToNetPacketPlayersTurn() const;

protected:

	virtual void Check(const NetPacketHeader* data) const;
};

class NetPacketPlayersAction : public NetPacket
{
public:
	struct Data
	{
		GameState		gameState;
		PlayerAction	playerAction;
		u_int32_t		playerBet;
	};

	NetPacketPlayersAction();
	virtual ~NetPacketPlayersAction();

	virtual boost::shared_ptr<NetPacket> Clone() const;

	void SetData(const Data &inData);
	void GetData(Data &outData) const;

	virtual const NetPacketPlayersAction *ToNetPacketPlayersAction() const;

protected:

	virtual void Check(const NetPacketHeader* data) const;
};

class NetPacketPlayersActionDone : public NetPacket
{
public:
	struct Data
	{
		GameState		gameState;
		u_int16_t		playerId;
		PlayerAction	playerAction;
		u_int32_t		totalPlayerBet;
		u_int32_t		playerMoney;
		u_int32_t		potSize;
		u_int32_t		curHandBets;
	};

	NetPacketPlayersActionDone();
	virtual ~NetPacketPlayersActionDone();

	virtual boost::shared_ptr<NetPacket> Clone() const;

	void SetData(const Data &inData);
	void GetData(Data &outData) const;

	virtual const NetPacketPlayersActionDone *ToNetPacketPlayersActionDone() const;

protected:

	virtual void Check(const NetPacketHeader* data) const;
};

class NetPacketPlayersActionRejected : public NetPacket
{
public:

	struct Data
	{
		GameState		gameState;
		PlayerAction	playerAction;
		u_int32_t		playerBet;
		int				rejectionReason;
	};

	NetPacketPlayersActionRejected();
	virtual ~NetPacketPlayersActionRejected();

	virtual boost::shared_ptr<NetPacket> Clone() const;

	void SetData(const Data &inData);
	void GetData(Data &outData) const;

	virtual const NetPacketPlayersActionRejected *ToNetPacketPlayersActionRejected() const;

protected:

	virtual void Check(const NetPacketHeader* data) const;
};

class NetPacketError : public NetPacket
{
public:
	struct Data
	{
		int		errorCode;
	};

	NetPacketError();
	virtual ~NetPacketError();

	virtual boost::shared_ptr<NetPacket> Clone() const;

	void SetData(const Data &inData);
	void GetData(Data &outData) const;

	virtual const NetPacketError *ToNetPacketError() const;

protected:

	virtual void Check(const NetPacketHeader* data) const;
};

#endif

