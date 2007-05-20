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

#include <string>

using namespace std;

#define ADD_PADDING(x) ((((x) + 3) >> 2) << 2)

#define NET_TYPE_JOIN_GAME						0x0001
#define NET_TYPE_JOIN_GAME_ACK					0x0002
#define NET_TYPE_PLAYER_JOINED					0x0003
#define NET_TYPE_PLAYER_LEFT					0x0004
#define NET_TYPE_GAME_START						0x0005
#define NET_TYPE_HAND_START						0x0006
#define NET_TYPE_PLAYERS_TURN					0x0007
#define NET_TYPE_PLAYERS_ACTION					0x0008
#define NET_TYPE_PLAYERS_ACTION_REJECTED		0x0009

#define NET_TYPE_ERROR							0x0400

#define NET_PLAYER_FLAG_HUMAN					0x01

#define NET_ERR_JOIN_GAME_VERSION_NOT_SUPPORTED	0x0001
#define NET_ERR_JOIN_GAME_SERVER_FULL			0x0002
#define NET_ERR_JOIN_GAME_ALREADY_RUNNING		0x0003
#define NET_ERR_JOIN_GAME_INVALID_PASSWORD		0x0004
#define NET_ERR_JOIN_GAME_PLAYER_NAME_IN_USE	0x0005
#define NET_ERR_JOIN_GAME_INVALID_PLAYER_NAME	0x0006
#define NET_ERR_GENERAL_INVALID_PACKET			0xFF01
#define NET_ERR_GENERAL_INVALID_STATE			0xFF02
#define NET_ERR_OTHER							0xFFFF

#ifdef _MSC_VER
	#pragma pack(push, 2)
#else
	#pragma align 2
#endif

struct NetPacketHeader
{
	u_int16_t			type;
	u_int16_t			length;
};

struct NetPacketJoinGameData
{
	NetPacketHeader		head;
	u_int16_t			requestedVersionMajor;
	u_int16_t			requestedVersionMinor;
	u_int16_t			passwordLength;
	u_int16_t			playerFlags;
	u_int16_t			playerNameLength;
	u_int16_t			reserved;
	char				password[1];
};

struct NetPacketJoinGameAckData
{
	NetPacketHeader		head;
	u_int32_t			sessionId;
	u_int16_t			playerId;
	u_int16_t			playerNumber;
	u_int16_t			numberOfPlayers;
	u_int16_t			smallBlind;
	u_int16_t			handsBeforeRaise;
	u_int16_t			proposedGuiSpeed;
	u_int32_t			startCash;
};

struct NetPacketJoinGameErrorData
{
	NetPacketHeader		head;
	u_int16_t			errorReason;
	u_int16_t			reserved;
};

struct NetPacketPlayerJoinedData
{
	NetPacketHeader		head;
	u_int16_t			playerId;
	u_int16_t			playerNumber;
	u_int16_t			playerFlags;
	u_int16_t			playerNameLength;
	char				playerName[1];
};

struct NetPacketPlayerLeftData
{
	NetPacketHeader		head;
	u_int16_t			playerId;
	u_int16_t			reserved;
};

struct NetPacketGameStartData
{
	NetPacketHeader		head;
	u_int16_t			startDealerPos;
	u_int16_t			reserved;
};

struct NetPacketHandStartData
{
	NetPacketHeader		head;
	u_int16_t			yourCards[2];
};

struct NetPacketPlayersTurnData
{
	NetPacketHeader		head;
	u_int16_t			gameState;
	u_int16_t			playerId;
};

struct NetPacketPlayersActionData
{
	NetPacketHeader		head;
	u_int16_t			gameState;
	u_int16_t			playerAction;
	u_int32_t			cashValue;
};

struct NetPacketPlayersActionRejectedData
{
	NetPacketHeader		head;
	u_int16_t			gameState;
	u_int16_t			playerAction;
	u_int32_t			cashValue;
	u_int16_t			rejectionReason;
	u_int16_t			reserved;
};

struct NetPacketErrorData
{
	NetPacketHeader		head;
	u_int16_t			reason;
	u_int16_t			reserved;
};

#ifdef _MSC_VER
	#pragma pack(pop)
#else
	#pragma align 0
#endif


boost::shared_ptr<NetPacket>
NetPacket::Create(char *data, unsigned &dataSize)
{
	boost::shared_ptr<NetPacket> tmpPacket;

	NetPacketHeader *tmpHeader = (NetPacketHeader *)data;
	u_int16_t tmpLen = ntohs(tmpHeader->length);

	// Check size restrictions.
	if (tmpLen < sizeof(NetPacketHeader)
		|| tmpLen > MAX_PACKET_SIZE)
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}
	else if (dataSize >= tmpLen)
	{
		// OK - we have a complete packet. Construct a corresponding object.
		try
		{
			switch(ntohs(tmpHeader->type))
			{
				case NET_TYPE_JOIN_GAME:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketJoinGame);
					break;
				case NET_TYPE_JOIN_GAME_ACK:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketJoinGameAck);
					break;
				case NET_TYPE_PLAYER_JOINED:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketPlayerJoined);
					break;
				case NET_TYPE_PLAYER_LEFT:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketPlayerLeft);
					break;
				case NET_TYPE_GAME_START:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketGameStart);
					break;
				case NET_TYPE_HAND_START:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketHandStart);
					break;
				case NET_TYPE_PLAYERS_TURN:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketPlayersTurn);
					break;
				case NET_TYPE_PLAYERS_ACTION:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketPlayersAction);
					break;
				case NET_TYPE_PLAYERS_ACTION_REJECTED:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketPlayersActionRejected);
					break;
				case NET_TYPE_ERROR:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketError);
					break;
			}
			if (tmpPacket.get())
				tmpPacket->SetRawData(tmpHeader);
		} catch (const NetException &)
		{
			tmpPacket.reset();
		}

		if (tmpLen < dataSize)
		{
			dataSize -= tmpLen;
			memmove(data, data + tmpLen, dataSize);
		}
		else
			dataSize = 0;
	}
	return tmpPacket;
}

NetPacket::NetPacket(u_int16_t type, u_int16_t initialLen)
: m_data(NULL)
{
	assert(initialLen >= sizeof(NetPacketHeader));
	m_data = (NetPacketHeader *)malloc(initialLen);
	assert(m_data);
	memset(m_data, 0, initialLen);
	m_data->type = htons(type);
	m_data->length = htons(initialLen);
}

NetPacket::~NetPacket()
{
	if (m_data)
		free(m_data);
}

const NetPacketHeader *
NetPacket::GetRawData() const
{
	assert(m_data);
	return m_data;
}

NetPacketHeader *
NetPacket::GetRawData()
{
	assert(m_data);
	return m_data;
}

void
NetPacket::SetRawData(const NetPacketHeader *p)
{
	if (!p)
		return;
	assert(m_data);

	u_int16_t tmpLen = ntohs(p->length);
	if (ntohs(p->type) != GetType())
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}
	// Check whether the data is valid.
	Check(p);
	// Resize makes sure m_data is large enough.
	Resize(tmpLen);
	memcpy(m_data, p, tmpLen);
}

u_int16_t
NetPacket::GetType() const
{
	return ntohs(GetRawData()->type);
}

u_int16_t
NetPacket::GetLen() const
{
	return ntohs(GetRawData()->length);
}

const NetPacketJoinGame *
NetPacket::ToNetPacketJoinGame() const
{
	return NULL;
}

const NetPacketJoinGameAck *
NetPacket::ToNetPacketJoinGameAck() const
{
	return NULL;
}

const NetPacketPlayerJoined *
NetPacket::ToNetPacketPlayerJoined() const
{
	return NULL;
}

const NetPacketPlayerLeft *
NetPacket::ToNetPacketPlayerLeft() const
{
	return NULL;
}

const NetPacketGameStart *
NetPacket::ToNetPacketGameStart() const
{
	return NULL;
}

const NetPacketHandStart *
NetPacket::ToNetPacketHandStart() const
{
	return NULL;
}

const NetPacketPlayersTurn *
NetPacket::ToNetPacketPlayersTurn() const
{
	return NULL;
}

const NetPacketPlayersAction *
NetPacket::ToNetPacketPlayersAction() const
{
	return NULL;
}

const NetPacketPlayersActionRejected *
NetPacket::ToNetPacketPlayersActionRejected() const
{
	return NULL;
}

const NetPacketError *
NetPacket::ToNetPacketError() const
{
	return NULL;
}

void
NetPacket::Resize(u_int16_t newLen)
{
	assert(m_data);
	u_int16_t oldLen = GetLen();
	if (newLen != oldLen)
	{
		if (newLen < sizeof(NetPacketHeader))
			throw NetException(ERR_SOCK_INTERNAL, 0);
		else
		{
			NetPacketHeader *newData = (NetPacketHeader *)malloc(newLen);
			if (!newData)
				throw NetException(ERR_SOCK_INTERNAL, 0);
			else
			{
				// Copy as much data as possible.
				memcpy(newData, m_data, newLen > oldLen ? oldLen : newLen);
				// Initialize new data to 0.
				if (newLen > oldLen)
					memset(((unsigned char *)newData) + oldLen, 0, newLen - oldLen);
				// Set new len.
				newData->length = htons(newLen);
				// Switch over to new data.
				free(m_data);
				m_data = newData;
			}
		}
	}
}

//-----------------------------------------------------------------------------

NetPacketJoinGame::NetPacketJoinGame()
: NetPacket(NET_TYPE_JOIN_GAME, sizeof(NetPacketJoinGameData))
{
	NetPacketJoinGameData *tmpData = (NetPacketJoinGameData *)GetRawData();
	assert(tmpData);
	tmpData->requestedVersionMajor = htons(NET_VERSION_MAJOR);
	tmpData->requestedVersionMinor = htons(NET_VERSION_MINOR);
}

NetPacketJoinGame::~NetPacketJoinGame()
{
}

boost::shared_ptr<NetPacket>
NetPacketJoinGame::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketJoinGame);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketJoinGame::SetData(const NetPacketJoinGame::Data &inData)
{
	u_int16_t playerNameLen = (u_int16_t)inData.playerName.length();
	u_int16_t passwordLen = (u_int16_t)inData.password.length();

	if (!playerNameLen || playerNameLen > MAX_NAME_SIZE)
		throw NetException(ERR_NET_INVALID_PLAYER_NAME, 0);
	if (passwordLen > MAX_PASSWORD_SIZE)
		throw NetException(ERR_NET_INVALID_PASSWORD_STR, 0);

	// Resize the packet so that the data fits in.
	Resize((u_int16_t)
		(sizeof(NetPacketJoinGameData) + ADD_PADDING(playerNameLen) + ADD_PADDING(passwordLen)));

	NetPacketJoinGameData *tmpData = (NetPacketJoinGameData *)GetRawData();
	assert(tmpData);

	// Set the data.
	tmpData->passwordLength = htons(passwordLen);
	tmpData->playerFlags = htons((inData.ptype == PLAYER_TYPE_HUMAN) ? NET_PLAYER_FLAG_HUMAN : 0);
	tmpData->playerNameLength = htons(playerNameLen);
	memcpy(tmpData->password, inData.password.c_str(), passwordLen);
	memcpy(tmpData->password + ADD_PADDING(passwordLen), inData.playerName.c_str(), playerNameLen);
}

void
NetPacketJoinGame::GetData(NetPacketJoinGame::Data &outData) const
{
	// We assume that the data is valid. Validity has already been checked.
	NetPacketJoinGameData *tmpData = (NetPacketJoinGameData *)GetRawData();
	assert(tmpData);

	outData.versionMajor = ntohs(tmpData->requestedVersionMajor);
	outData.versionMinor = ntohs(tmpData->requestedVersionMinor);

	outData.ptype = (ntohs(tmpData->playerFlags) & NET_PLAYER_FLAG_HUMAN) ? PLAYER_TYPE_HUMAN : PLAYER_TYPE_COMPUTER;

	u_int16_t passwordLen = ntohs(tmpData->passwordLength);
	outData.password = string(tmpData->password, passwordLen);
	outData.playerName = string(tmpData->password + ADD_PADDING(passwordLen), ntohs(tmpData->playerNameLength));
}

const NetPacketJoinGame *
NetPacketJoinGame::ToNetPacketJoinGame() const
{
	return this;
}

void
NetPacketJoinGame::Check(const NetPacketHeader* data) const
{
	assert(data);

	u_int16_t dataLen = ntohs(data->length);
	if (dataLen < sizeof(NetPacketJoinGameData))
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}

	NetPacketJoinGameData *tmpData = (NetPacketJoinGameData *)data;
	int passwordLength = ntohs(tmpData->passwordLength);
	int playerNameLength = ntohs(tmpData->playerNameLength);
	// Generous checking - larger packets are allowed.
	if (dataLen <
		sizeof(NetPacketJoinGameData)
		+ ADD_PADDING(passwordLength)
		+ ADD_PADDING(playerNameLength))
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}
	// Check string sizes.
	if (passwordLength > MAX_PASSWORD_SIZE
		|| !playerNameLength
		|| playerNameLength > MAX_NAME_SIZE)
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}
}

//-----------------------------------------------------------------------------

NetPacketJoinGameAck::NetPacketJoinGameAck()
: NetPacket(NET_TYPE_JOIN_GAME_ACK, sizeof(NetPacketJoinGameAckData))
{
}

NetPacketJoinGameAck::~NetPacketJoinGameAck()
{
}

boost::shared_ptr<NetPacket>
NetPacketJoinGameAck::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketJoinGameAck);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketJoinGameAck::SetData(const NetPacketJoinGameAck::Data &inData)
{
	NetPacketJoinGameAckData *tmpData = (NetPacketJoinGameAckData *)GetRawData();
	assert(tmpData);

	tmpData->sessionId			= htonl(inData.sessionId);
	tmpData->numberOfPlayers	= htons(inData.gameData.numberOfPlayers);
	tmpData->smallBlind			= htons(inData.gameData.smallBlind);
	tmpData->handsBeforeRaise	= htons(inData.gameData.handsBeforeRaise);
	tmpData->proposedGuiSpeed	= htons(inData.gameData.guiSpeed);
	tmpData->startCash			= htonl(inData.gameData.startCash);
	tmpData->playerId			= htons(inData.gameData.guiPlayerUniqueId);
	tmpData->playerNumber		= htons(inData.gameData.guiPlayerNum);
}

void
NetPacketJoinGameAck::GetData(NetPacketJoinGameAck::Data &outData) const
{
	NetPacketJoinGameAckData *tmpData = (NetPacketJoinGameAckData *)GetRawData();
	assert(tmpData);

	outData.sessionId					= ntohl(tmpData->sessionId);
	outData.gameData.numberOfPlayers	= ntohs(tmpData->numberOfPlayers);
	outData.gameData.smallBlind			= ntohs(tmpData->smallBlind);
	outData.gameData.handsBeforeRaise	= ntohs(tmpData->handsBeforeRaise);
	outData.gameData.guiSpeed			= ntohs(tmpData->proposedGuiSpeed);
	outData.gameData.startCash			= ntohl(tmpData->startCash);
	outData.gameData.guiPlayerUniqueId	= ntohs(tmpData->playerId);
	outData.gameData.guiPlayerNum		= ntohs(tmpData->playerNumber);
}

const NetPacketJoinGameAck *
NetPacketJoinGameAck::ToNetPacketJoinGameAck() const
{
	return this;
}

void
NetPacketJoinGameAck::Check(const NetPacketHeader* data) const
{
	assert(data);

	u_int16_t dataLen = ntohs(data->length);
	if (dataLen < sizeof(NetPacketJoinGameAckData))
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}
	// TODO: maybe some semantic checks
}

//-----------------------------------------------------------------------------

NetPacketPlayerJoined::NetPacketPlayerJoined()
: NetPacket(NET_TYPE_PLAYER_JOINED, sizeof(NetPacketPlayerJoinedData))
{
}

NetPacketPlayerJoined::~NetPacketPlayerJoined()
{
}

boost::shared_ptr<NetPacket>
NetPacketPlayerJoined::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketPlayerJoined);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketPlayerJoined::SetData(const NetPacketPlayerJoined::Data &inData)
{
	u_int16_t playerNameLen = (u_int16_t)inData.playerName.length();

	if (!playerNameLen || playerNameLen > MAX_NAME_SIZE)
		throw NetException(ERR_NET_INVALID_PLAYER_NAME, 0);

	// Resize the packet so that the data fits in.
	Resize((u_int16_t)
		(sizeof(NetPacketPlayerJoinedData) + ADD_PADDING(playerNameLen)));

	NetPacketPlayerJoinedData *tmpData = (NetPacketPlayerJoinedData *)GetRawData();
	assert(tmpData);

	// Set the data.
	tmpData->playerFlags = htons((inData.ptype == PLAYER_TYPE_HUMAN) ? NET_PLAYER_FLAG_HUMAN : 0);
	tmpData->playerId = htons(inData.playerId);
	tmpData->playerNumber = htons(inData.playerNumber);
	tmpData->playerNameLength = htons(playerNameLen);
	memcpy(tmpData->playerName, inData.playerName.c_str(), playerNameLen);
}

void
NetPacketPlayerJoined::GetData(NetPacketPlayerJoined::Data &outData) const
{
	// We assume that the data is valid. Validity has already been checked.
	NetPacketPlayerJoinedData *tmpData = (NetPacketPlayerJoinedData *)GetRawData();
	assert(tmpData);

	outData.ptype = (ntohs(tmpData->playerFlags) & NET_PLAYER_FLAG_HUMAN) ? PLAYER_TYPE_HUMAN : PLAYER_TYPE_COMPUTER;
	outData.playerId = ntohs(tmpData->playerId);
	outData.playerNumber = ntohs(tmpData->playerNumber);
	outData.playerName = string(tmpData->playerName, ntohs(tmpData->playerNameLength));
}

const NetPacketPlayerJoined *
NetPacketPlayerJoined::ToNetPacketPlayerJoined() const
{
	return this;
}

void
NetPacketPlayerJoined::Check(const NetPacketHeader* data) const
{
	assert(data);

	u_int16_t dataLen = ntohs(data->length);
	if (dataLen < sizeof(NetPacketPlayerJoinedData))
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}

	NetPacketPlayerJoinedData *tmpData = (NetPacketPlayerJoinedData *)data;
	int playerNameLength = ntohs(tmpData->playerNameLength);
	// Generous checking - larger packets are allowed.
	if (dataLen <
		sizeof(NetPacketPlayerJoinedData)
		+ ADD_PADDING(playerNameLength))
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}
	// Check string sizes.
	if (!playerNameLength
		|| playerNameLength > MAX_NAME_SIZE)
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}
}

//-----------------------------------------------------------------------------

NetPacketPlayerLeft::NetPacketPlayerLeft()
: NetPacket(NET_TYPE_PLAYER_LEFT, sizeof(NetPacketPlayerLeftData))
{
}

NetPacketPlayerLeft::~NetPacketPlayerLeft()
{
}

boost::shared_ptr<NetPacket>
NetPacketPlayerLeft::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketPlayerLeft);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketPlayerLeft::SetData(const NetPacketPlayerLeft::Data &inData)
{
	NetPacketPlayerLeftData *tmpData = (NetPacketPlayerLeftData *)GetRawData();
	assert(tmpData);

	// Set the data.
	tmpData->playerId = htons(inData.playerId);
}

void
NetPacketPlayerLeft::GetData(NetPacketPlayerLeft::Data &outData) const
{
	// We assume that the data is valid. Validity has already been checked.
	NetPacketPlayerLeftData *tmpData = (NetPacketPlayerLeftData *)GetRawData();
	assert(tmpData);

	outData.playerId = ntohs(tmpData->playerId);
}

const NetPacketPlayerLeft *
NetPacketPlayerLeft::ToNetPacketPlayerLeft() const
{
	return this;
}

void
NetPacketPlayerLeft::Check(const NetPacketHeader* data) const
{
	assert(data);

	u_int16_t dataLen = ntohs(data->length);
	if (dataLen < sizeof(NetPacketPlayerLeftData))
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}
}

//-----------------------------------------------------------------------------

NetPacketGameStart::NetPacketGameStart()
: NetPacket(NET_TYPE_GAME_START, sizeof(NetPacketGameStartData))
{
}

NetPacketGameStart::~NetPacketGameStart()
{
}

boost::shared_ptr<NetPacket>
NetPacketGameStart::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketGameStart);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketGameStart::SetData(const NetPacketGameStart::Data &inData)
{
	NetPacketGameStartData *tmpData = (NetPacketGameStartData *)GetRawData();
	assert(tmpData);

	tmpData->startDealerPos	= htons(inData.startData.startDealerPos);
}

void
NetPacketGameStart::GetData(NetPacketGameStart::Data &outData) const
{
	NetPacketGameStartData *tmpData = (NetPacketGameStartData *)GetRawData();
	assert(tmpData);

	outData.startData.startDealerPos	= ntohs(tmpData->startDealerPos);
}

const NetPacketGameStart *
NetPacketGameStart::ToNetPacketGameStart() const
{
	return this;
}

void
NetPacketGameStart::Check(const NetPacketHeader* data) const
{
	assert(data);

	u_int16_t dataLen = ntohs(data->length);
	if (dataLen < sizeof(NetPacketGameStartData))
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}
}

//-----------------------------------------------------------------------------

NetPacketHandStart::NetPacketHandStart()
: NetPacket(NET_TYPE_HAND_START, sizeof(NetPacketHandStartData))
{
}

NetPacketHandStart::~NetPacketHandStart()
{
}

boost::shared_ptr<NetPacket>
NetPacketHandStart::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketHandStart);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketHandStart::SetData(const NetPacketHandStart::Data &inData)
{
	NetPacketHandStartData *tmpData = (NetPacketHandStartData *)GetRawData();
	assert(tmpData);

	tmpData->yourCards[0]		= htons(inData.yourCards[0]);
	tmpData->yourCards[1]		= htons(inData.yourCards[1]);
}

void
NetPacketHandStart::GetData(NetPacketHandStart::Data &outData) const
{
	NetPacketHandStartData *tmpData = (NetPacketHandStartData *)GetRawData();
	assert(tmpData);

	outData.yourCards[0]		= ntohs(tmpData->yourCards[0]);
	outData.yourCards[1]		= ntohs(tmpData->yourCards[1]);
}

const NetPacketHandStart *
NetPacketHandStart::ToNetPacketHandStart() const
{
	return this;
}

void
NetPacketHandStart::Check(const NetPacketHeader* data) const
{
	assert(data);

	u_int16_t dataLen = ntohs(data->length);
	if (dataLen < sizeof(NetPacketHandStartData))
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}

	NetPacketHandStartData *tmpData = (NetPacketHandStartData *)data;
	if (ntohs(tmpData->yourCards[0]) > 51 || ntohs(tmpData->yourCards[1]) > 51)
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}
}

//-----------------------------------------------------------------------------

NetPacketPlayersTurn::NetPacketPlayersTurn()
: NetPacket(NET_TYPE_PLAYERS_TURN, sizeof(NetPacketPlayersTurnData))
{
}

NetPacketPlayersTurn::~NetPacketPlayersTurn()
{
}

boost::shared_ptr<NetPacket>
NetPacketPlayersTurn::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketPlayersTurn);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketPlayersTurn::SetData(const NetPacketPlayersTurn::Data &inData)
{
	NetPacketPlayersTurnData *tmpData = (NetPacketPlayersTurnData *)GetRawData();
	assert(tmpData);

	tmpData->gameState	= htons(inData.gameState);
	tmpData->playerId	= htons(inData.playerId);
}

void
NetPacketPlayersTurn::GetData(NetPacketPlayersTurn::Data &outData) const
{
	NetPacketPlayersTurnData *tmpData = (NetPacketPlayersTurnData *)GetRawData();
	assert(tmpData);

	outData.gameState	= static_cast<GameState>(ntohs(tmpData->gameState));
	outData.playerId	= ntohs(tmpData->playerId);
}

const NetPacketPlayersTurn *
NetPacketPlayersTurn::ToNetPacketPlayersTurn() const
{
	return this;
}

void
NetPacketPlayersTurn::Check(const NetPacketHeader* data) const
{
	assert(data);

	u_int16_t dataLen = ntohs(data->length);
	if (dataLen < sizeof(NetPacketPlayersTurnData))
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}

	// Check whether the state is valid.
	NetPacketPlayersTurnData *tmpData = (NetPacketPlayersTurnData *)data;
	if (ntohs(tmpData->gameState) > GAME_STATE_RIVER)
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}
}

//-----------------------------------------------------------------------------

NetPacketPlayersAction::NetPacketPlayersAction()
: NetPacket(NET_TYPE_PLAYERS_ACTION, sizeof(NetPacketPlayersActionData))
{
}

NetPacketPlayersAction::~NetPacketPlayersAction()
{
}

boost::shared_ptr<NetPacket>
NetPacketPlayersAction::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketPlayersAction);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketPlayersAction::SetData(const NetPacketPlayersAction::Data &inData)
{
	NetPacketPlayersActionData *tmpData = (NetPacketPlayersActionData *)GetRawData();
	assert(tmpData);

	tmpData->gameState		= htons(inData.gameState);
	tmpData->playerAction	= htons(inData.playerAction);
	tmpData->cashValue		= htonl(inData.cashValue);
}

void
NetPacketPlayersAction::GetData(NetPacketPlayersAction::Data &outData) const
{
	NetPacketPlayersActionData *tmpData = (NetPacketPlayersActionData *)GetRawData();
	assert(tmpData);

	outData.gameState		= static_cast<GameState>(ntohs(tmpData->gameState));
	outData.playerAction	= static_cast<PlayerAction>(ntohs(tmpData->playerAction));
	outData.cashValue		= ntohl(tmpData->cashValue);
}

const NetPacketPlayersAction *
NetPacketPlayersAction::ToNetPacketPlayersAction() const
{
	return this;
}

void
NetPacketPlayersAction::Check(const NetPacketHeader* data) const
{
	assert(data);

	u_int16_t dataLen = ntohs(data->length);
	if (dataLen < sizeof(NetPacketPlayersActionData))
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}

	// Check whether the state is valid.
	NetPacketPlayersActionData *tmpData = (NetPacketPlayersActionData *)data;
	if (ntohs(tmpData->gameState) > GAME_STATE_RIVER)
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}
	// Check whether the player action is valid.
	if (ntohs(tmpData->playerAction) > PLAYER_ACTION_ALLIN)
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}
}

//-----------------------------------------------------------------------------

NetPacketPlayersActionRejected::NetPacketPlayersActionRejected()
: NetPacket(NET_TYPE_PLAYERS_ACTION_REJECTED, sizeof(NetPacketPlayersActionRejectedData))
{
}

NetPacketPlayersActionRejected::~NetPacketPlayersActionRejected()
{
}

boost::shared_ptr<NetPacket>
NetPacketPlayersActionRejected::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketPlayersActionRejected);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketPlayersActionRejected::SetData(const NetPacketPlayersActionRejected::Data &inData)
{
	NetPacketPlayersActionRejectedData *tmpData = (NetPacketPlayersActionRejectedData *)GetRawData();
	assert(tmpData);

	tmpData->gameState			= htons(inData.gameState);
	tmpData->playerAction		= htons(inData.playerAction);
	tmpData->cashValue			= htonl(inData.cashValue);

	// TODO: set rejection reason
	tmpData->rejectionReason	= htons(0);
}

void
NetPacketPlayersActionRejected::GetData(NetPacketPlayersActionRejected::Data &outData) const
{
	NetPacketPlayersActionRejectedData *tmpData = (NetPacketPlayersActionRejectedData *)GetRawData();
	assert(tmpData);

	outData.gameState		= static_cast<GameState>(ntohs(tmpData->gameState));
	outData.playerAction	= static_cast<PlayerAction>(ntohs(tmpData->playerAction));
	outData.cashValue		= ntohl(tmpData->cashValue);

	// TODO: set rejection reason
}

const NetPacketPlayersActionRejected *
NetPacketPlayersActionRejected::ToNetPacketPlayersActionRejected() const
{
	return this;
}

void
NetPacketPlayersActionRejected::Check(const NetPacketHeader* data) const
{
	assert(data);

	u_int16_t dataLen = ntohs(data->length);
	if (dataLen < sizeof(NetPacketPlayersActionRejectedData))
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}

	// Check whether the state is valid.
	NetPacketPlayersActionRejectedData *tmpData = (NetPacketPlayersActionRejectedData *)data;
	if (ntohs(tmpData->gameState) > GAME_STATE_RIVER)
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}
	// Check whether the player action is valid.
	if (ntohs(tmpData->playerAction) > PLAYER_ACTION_ALLIN)
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}
	// TODO: check rejection reason
}

//-----------------------------------------------------------------------------

NetPacketError::NetPacketError()
: NetPacket(NET_TYPE_ERROR, sizeof(NetPacketErrorData))
{
}

NetPacketError::~NetPacketError()
{
}

boost::shared_ptr<NetPacket>
NetPacketError::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketError);
	try
	{
		newPacket->SetRawData(GetRawData());
	} catch (const NetException &)
	{
		// Need to return the new packet anyway.
	}
	return newPacket;
}

void
NetPacketError::SetData(const NetPacketError::Data &inData)
{
	NetPacketErrorData *tmpData = (NetPacketErrorData *)GetRawData();
	assert(tmpData);

	switch (inData.errorCode)
	{
	// Join Game Errors.
		case ERR_NET_VERSION_NOT_SUPPORTED :
			tmpData->reason = htons(NET_ERR_JOIN_GAME_VERSION_NOT_SUPPORTED);
			break;
		case ERR_NET_SERVER_FULL :
			tmpData->reason = htons(NET_ERR_JOIN_GAME_SERVER_FULL);
			break;
		case ERR_NET_GAME_ALREADY_RUNNING :
			tmpData->reason = htons(NET_ERR_JOIN_GAME_ALREADY_RUNNING);
			break;
		case ERR_NET_INVALID_PASSWORD :
			tmpData->reason = htons(NET_ERR_JOIN_GAME_INVALID_PASSWORD);
			break;
		case ERR_NET_PLAYER_NAME_IN_USE :
			tmpData->reason = htons(NET_ERR_JOIN_GAME_PLAYER_NAME_IN_USE);
			break;
		case ERR_NET_INVALID_PLAYER_NAME :
			tmpData->reason = htons(NET_ERR_JOIN_GAME_INVALID_PLAYER_NAME);
			break;
	// General Errors.
		case ERR_SOCK_INVALID_PACKET :
			tmpData->reason = htons(NET_ERR_GENERAL_INVALID_PACKET);
			break;
		case ERR_SOCK_INVALID_STATE :
			tmpData->reason = htons(NET_ERR_GENERAL_INVALID_STATE);
			break;
		default :
			tmpData->reason = htons(NET_ERR_OTHER);
			break;
	}
}

void
NetPacketError::GetData(NetPacketError::Data &outData) const
{
	NetPacketErrorData *tmpData = (NetPacketErrorData *)GetRawData();
	assert(tmpData);

	switch (ntohs(tmpData->reason))
	{
	// Join Game Errors.
		case NET_ERR_JOIN_GAME_VERSION_NOT_SUPPORTED :
			outData.errorCode = ERR_NET_VERSION_NOT_SUPPORTED;
			break;
		case NET_ERR_JOIN_GAME_SERVER_FULL :
			outData.errorCode = ERR_NET_SERVER_FULL;
			break;
		case NET_ERR_JOIN_GAME_ALREADY_RUNNING :
			outData.errorCode = ERR_NET_GAME_ALREADY_RUNNING;
			break;
		case NET_ERR_JOIN_GAME_INVALID_PASSWORD :
			outData.errorCode = ERR_NET_INVALID_PASSWORD;
			break;
		case NET_ERR_JOIN_GAME_PLAYER_NAME_IN_USE :
			outData.errorCode = ERR_NET_PLAYER_NAME_IN_USE;
			break;
		case NET_ERR_JOIN_GAME_INVALID_PLAYER_NAME :
			outData.errorCode = ERR_NET_INVALID_PLAYER_NAME;
			break;
	// General Errors.
		case NET_ERR_GENERAL_INVALID_PACKET :
			outData.errorCode = ERR_SOCK_INVALID_PACKET;
			break;
		case NET_ERR_GENERAL_INVALID_STATE :
			outData.errorCode = ERR_SOCK_INVALID_STATE;
			break;
		default :
			outData.errorCode = ERR_SOCK_INTERNAL;
			break;
	}
}

const NetPacketError *
NetPacketError::ToNetPacketError() const
{
	return this;
}

void
NetPacketError::Check(const NetPacketHeader* data) const
{
	assert(data);

	u_int16_t dataLen = ntohs(data->length);
	if (dataLen < sizeof(NetPacketErrorData))
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}
}

//-----------------------------------------------------------------------------

