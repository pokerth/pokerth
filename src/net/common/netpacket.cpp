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
#define NET_TYPE_PLAYERS_ACTION_DONE			0x0009
#define NET_TYPE_PLAYERS_ACTION_REJECTED		0x000A
#define NET_TYPE_DEAL_FLOP_CARDS				0x000B
#define NET_TYPE_DEAL_TURN_CARD					0x000C
#define NET_TYPE_DEAL_RIVER_CARD				0x000D
#define NET_TYPE_ALL_IN_SHOW_CARDS				0x000E
#define NET_TYPE_END_OF_HAND_SHOW_CARDS			0x000F
#define NET_TYPE_END_OF_HAND_HIDE_CARDS			0x0010
#define NET_TYPE_END_OF_GAME					0x0011

#define NET_TYPE_SEND_CHAT_TEXT					0x0200
#define NET_TYPE_CHAT_TEXT						0x0201

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
#define NET_ERR_GENERAL_PLAYER_KICKED			0xFF03
#define NET_ERR_OTHER							0xFFFF

#ifdef _MSC_VER
	#pragma pack(push, 1)
	#define GCC_PACKED
#else
	#define GCC_PACKED __attribute__ ((__packed__))
#endif


struct GCC_PACKED NetPacketHeader
{
	u_int16_t			type;
	u_int16_t			length;
};

struct GCC_PACKED NetPacketJoinGameData
{
	NetPacketHeader		head;
	u_int16_t			requestedVersionMajor;
	u_int16_t			requestedVersionMinor;
	u_int16_t			passwordLength;
	u_int16_t			playerFlags;
	u_int16_t			playerNameLength;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketJoinGameAckData
{
	NetPacketHeader		head;
	u_int32_t			sessionId;
	u_int16_t			playerId;
	u_int16_t			maxNumberOfPlayers;
	u_int16_t			smallBlind;
	u_int16_t			handsBeforeRaise;
	u_int16_t			proposedGuiSpeed;
	u_int16_t			playerActionTimeout;
	u_int32_t			startMoney;
};

struct GCC_PACKED NetPacketJoinGameErrorData
{
	NetPacketHeader		head;
	u_int16_t			errorReason;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketPlayerJoinedData
{
	NetPacketHeader		head;
	u_int16_t			playerId;
	u_int16_t			playerFlags;
	u_int16_t			playerNameLength;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketPlayerLeftData
{
	NetPacketHeader		head;
	u_int16_t			playerId;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketGameStartData
{
	NetPacketHeader		head;
	u_int16_t			startDealerPlayerId;
	u_int16_t			numberOfPlayers;
};

struct GCC_PACKED PlayerSlotData
{
	u_int16_t			playerId;
};

struct GCC_PACKED NetPacketHandStartData
{
	NetPacketHeader		head;
	u_int16_t			yourCard1;
	u_int16_t			yourCard2;
};

struct GCC_PACKED NetPacketPlayersTurnData
{
	NetPacketHeader		head;
	u_int16_t			gameState;
	u_int16_t			playerId;
};

struct GCC_PACKED NetPacketPlayersActionData
{
	NetPacketHeader		head;
	u_int16_t			gameState;
	u_int16_t			playerAction;
	u_int32_t			playerBet;
};

struct GCC_PACKED NetPacketPlayersActionDoneData
{
	NetPacketHeader		head;
	u_int16_t			gameState;
	u_int16_t			playerId;
	u_int16_t			playerAction;
	u_int16_t			reserved;
	u_int32_t			totalPlayerBet;
	u_int32_t			playerMoney;
};

struct GCC_PACKED NetPacketPlayersActionRejectedData
{
	NetPacketHeader		head;
	u_int16_t			gameState;
	u_int16_t			playerAction;
	u_int32_t			playerBet;
	u_int16_t			rejectionReason;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketDealFlopCardsData
{
	NetPacketHeader		head;
	u_int16_t			flopCard1;
	u_int16_t			flopCard2;
	u_int16_t			flopCard3;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketDealTurnCardData
{
	NetPacketHeader		head;
	u_int16_t			turnCard;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketDealRiverCardData
{
	NetPacketHeader		head;
	u_int16_t			riverCard;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketAllInShowCardsData
{
	NetPacketHeader		head;
	u_int16_t			numberOfPlayerCards;
	u_int16_t			reserved;
};

struct GCC_PACKED PlayerCardsData
{
	u_int16_t			playerId;
	u_int16_t			card1;
	u_int16_t			card2;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketEndOfHandShowCardsData
{
	NetPacketHeader		head;
	u_int16_t			numberOfPlayerResults;
	u_int16_t			reserved;
};

struct GCC_PACKED PlayerResultData
{
	u_int16_t			playerId;
	u_int16_t			card1;
	u_int16_t			card2;
	u_int16_t			bestHandPos1;
	u_int16_t			bestHandPos2;
	u_int16_t			bestHandPos3;
	u_int16_t			bestHandPos4;
	u_int16_t			bestHandPos5;
	u_int32_t			valueOfCards;
	u_int32_t			moneyWon;
	u_int32_t			playerMoney;
};

struct GCC_PACKED NetPacketEndOfHandHideCardsData
{
	NetPacketHeader		head;
	u_int16_t			playerId;
	u_int16_t			reserved;
	u_int32_t			moneyWon;
	u_int32_t			playerMoney;
};

struct GCC_PACKED NetPacketEndOfGameData
{
	NetPacketHeader		head;
	u_int16_t			winnerPlayerId;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketSendChatTextData
{
	NetPacketHeader		head;
	u_int16_t			textLength;
	u_int16_t			reserved;
};

struct GCC_PACKED NetPacketChatTextData
{
	NetPacketHeader		head;
	u_int16_t			textLength;
	u_int16_t			playerId;
	char				text[1];
};

struct GCC_PACKED NetPacketErrorData
{
	NetPacketHeader		head;
	u_int16_t			reason;
	u_int16_t			reserved;
};

#ifdef _MSC_VER
	#pragma pack(pop)
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
				case NET_TYPE_PLAYERS_ACTION_DONE:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketPlayersActionDone);
					break;
				case NET_TYPE_PLAYERS_ACTION_REJECTED:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketPlayersActionRejected);
					break;
				case NET_TYPE_DEAL_FLOP_CARDS:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketDealFlopCards);
					break;
				case NET_TYPE_DEAL_TURN_CARD:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketDealTurnCard);
					break;
				case NET_TYPE_DEAL_RIVER_CARD:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketDealRiverCard);
					break;
				case NET_TYPE_ALL_IN_SHOW_CARDS:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketAllInShowCards);
					break;
				case NET_TYPE_END_OF_HAND_SHOW_CARDS:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketEndOfHandShowCards);
					break;
				case NET_TYPE_END_OF_HAND_HIDE_CARDS:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketEndOfHandHideCards);
					break;
				case NET_TYPE_END_OF_GAME:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketEndOfGame);
					break;
				case NET_TYPE_SEND_CHAT_TEXT:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketSendChatText);
					break;
				case NET_TYPE_CHAT_TEXT:
					tmpPacket = boost::shared_ptr<NetPacket>(new NetPacketChatText);
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

const NetPacketPlayersActionDone *
NetPacket::ToNetPacketPlayersActionDone() const
{
	return NULL;
}

const NetPacketPlayersActionRejected *
NetPacket::ToNetPacketPlayersActionRejected() const
{
	return NULL;
}

const NetPacketDealFlopCards *
NetPacket::ToNetPacketDealFlopCards() const
{
	return NULL;
}

const NetPacketDealTurnCard *
NetPacket::ToNetPacketDealTurnCard() const
{
	return NULL;
}

const NetPacketDealRiverCard *
NetPacket::ToNetPacketDealRiverCard() const
{
	return NULL;
}

const NetPacketAllInShowCards *
NetPacket::ToNetPacketAllInShowCards() const
{
	return NULL;
}

const NetPacketEndOfHandShowCards *
NetPacket::ToNetPacketEndOfHandShowCards() const
{
	return NULL;
}

const NetPacketEndOfHandHideCards *
NetPacket::ToNetPacketEndOfHandHideCards() const
{
	return NULL;
}

const NetPacketEndOfGame *
NetPacket::ToNetPacketEndOfGame() const
{
	return NULL;
}

const NetPacketSendChatText *
NetPacket::ToNetPacketSendChatText() const
{
	return NULL;
}

const NetPacketChatText *
NetPacket::ToNetPacketChatText() const
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
	char *passwordPtr = (char *)tmpData + sizeof(NetPacketJoinGameData);
	memcpy(passwordPtr, inData.password.c_str(), passwordLen);
	memcpy(passwordPtr + ADD_PADDING(passwordLen), inData.playerName.c_str(), playerNameLen);
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
	char *passwordPtr = (char *)tmpData + sizeof(NetPacketJoinGameData);
	outData.password = string(passwordPtr, passwordLen);
	outData.playerName = string(passwordPtr + ADD_PADDING(passwordLen), ntohs(tmpData->playerNameLength));
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

	tmpData->sessionId				= htonl(inData.sessionId);
	tmpData->playerId				= htons(inData.yourPlayerUniqueId);
	tmpData->maxNumberOfPlayers		= htons(inData.gameData.maxNumberOfPlayers);
	tmpData->smallBlind				= htons(inData.gameData.smallBlind);
	tmpData->handsBeforeRaise		= htons(inData.gameData.handsBeforeRaise);
	tmpData->proposedGuiSpeed		= htons(inData.gameData.guiSpeed);
	tmpData->playerActionTimeout	= htons(inData.gameData.playerActionTimeoutSec);
	tmpData->startMoney				= htonl(inData.gameData.startMoney);
}

void
NetPacketJoinGameAck::GetData(NetPacketJoinGameAck::Data &outData) const
{
	NetPacketJoinGameAckData *tmpData = (NetPacketJoinGameAckData *)GetRawData();
	assert(tmpData);

	outData.sessionId						= ntohl(tmpData->sessionId);
	outData.yourPlayerUniqueId				= ntohs(tmpData->playerId);
	outData.gameData.maxNumberOfPlayers		= ntohs(tmpData->maxNumberOfPlayers);
	outData.gameData.smallBlind				= ntohs(tmpData->smallBlind);
	outData.gameData.handsBeforeRaise		= ntohs(tmpData->handsBeforeRaise);
	outData.gameData.guiSpeed				= ntohs(tmpData->proposedGuiSpeed);
	outData.gameData.playerActionTimeoutSec	= ntohs(tmpData->playerActionTimeout);
	outData.gameData.startMoney				= ntohl(tmpData->startMoney);
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
	if (dataLen != sizeof(NetPacketJoinGameAckData))
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
	tmpData->playerNameLength = htons(playerNameLen);
	char *namePtr = (char *)tmpData + sizeof(NetPacketPlayerJoinedData);
	memcpy(namePtr, inData.playerName.c_str(), playerNameLen);
}

void
NetPacketPlayerJoined::GetData(NetPacketPlayerJoined::Data &outData) const
{
	// We assume that the data is valid. Validity has already been checked.
	NetPacketPlayerJoinedData *tmpData = (NetPacketPlayerJoinedData *)GetRawData();
	assert(tmpData);

	outData.ptype = (ntohs(tmpData->playerFlags) & NET_PLAYER_FLAG_HUMAN) ? PLAYER_TYPE_HUMAN : PLAYER_TYPE_COMPUTER;
	outData.playerId = ntohs(tmpData->playerId);
	char *namePtr = (char *)tmpData + sizeof(NetPacketPlayerJoinedData);
	outData.playerName = string(namePtr, ntohs(tmpData->playerNameLength));
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
	if (dataLen != sizeof(NetPacketPlayerLeftData))
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
	u_int16_t numPlayers = (u_int16_t)inData.playerSlots.size();

	if (!numPlayers || numPlayers > MAX_NUMBER_OF_PLAYERS || numPlayers != inData.startData.numberOfPlayers)
		throw NetException(ERR_NET_INVALID_PLAYER_COUNT, 0);

	// Resize the packet so that the data fits in.
	Resize((u_int16_t)
		(sizeof(NetPacketGameStartData) + ADD_PADDING(numPlayers * sizeof(PlayerSlotData))));

	NetPacketGameStartData *tmpData = (NetPacketGameStartData *)GetRawData();
	assert(tmpData);

	tmpData->startDealerPlayerId	= htons(inData.startData.startDealerPlayerId);
	tmpData->numberOfPlayers		= htons(numPlayers);

	PlayerSlotList::const_iterator i = inData.playerSlots.begin();
	PlayerSlotList::const_iterator end = inData.playerSlots.end();

	// Copy the player slot data to continous memory
	PlayerSlotData *curPlayerSlotData =
		(PlayerSlotData *)((char *)tmpData + sizeof(NetPacketGameStartData));
	while (i != end)
	{
		curPlayerSlotData->playerId		= htons((*i).playerId);
		++curPlayerSlotData;
		++i;
	}
}

void
NetPacketGameStart::GetData(NetPacketGameStart::Data &outData) const
{
	NetPacketGameStartData *tmpData = (NetPacketGameStartData *)GetRawData();
	assert(tmpData);

	outData.startData.startDealerPlayerId	= ntohs(tmpData->startDealerPlayerId);
	u_int16_t numPlayers = ntohs(tmpData->numberOfPlayers);
	outData.startData.numberOfPlayers = numPlayers;

	PlayerSlotData *curPlayerSlotData =
		(PlayerSlotData *)((char *)tmpData + sizeof(NetPacketGameStartData));

	// Store all available player slots.
	for (int i = 0; i < numPlayers; i++)
	{
		PlayerSlot tmpPlayerSlot;
		tmpPlayerSlot.playerId	= ntohs(curPlayerSlotData->playerId);

		outData.playerSlots.push_back(tmpPlayerSlot);
		++curPlayerSlotData;
	}
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
	// TODO additional checking
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

	tmpData->yourCard1		= htons(inData.yourCards[0]);
	tmpData->yourCard2		= htons(inData.yourCards[1]);
}

void
NetPacketHandStart::GetData(NetPacketHandStart::Data &outData) const
{
	NetPacketHandStartData *tmpData = (NetPacketHandStartData *)GetRawData();
	assert(tmpData);

	outData.yourCards[0]		= ntohs(tmpData->yourCard1);
	outData.yourCards[1]		= ntohs(tmpData->yourCard2);
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
	if (dataLen != sizeof(NetPacketHandStartData))
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}

	NetPacketHandStartData *tmpData = (NetPacketHandStartData *)data;
	if (ntohs(tmpData->yourCard1) > 51 || ntohs(tmpData->yourCard2) > 51)
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
	if (dataLen != sizeof(NetPacketPlayersTurnData))
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
	tmpData->playerBet		= htonl(inData.playerBet);
}

void
NetPacketPlayersAction::GetData(NetPacketPlayersAction::Data &outData) const
{
	NetPacketPlayersActionData *tmpData = (NetPacketPlayersActionData *)GetRawData();
	assert(tmpData);

	outData.gameState		= static_cast<GameState>(ntohs(tmpData->gameState));
	outData.playerAction	= static_cast<PlayerAction>(ntohs(tmpData->playerAction));
	outData.playerBet		= ntohl(tmpData->playerBet);
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
	if (dataLen != sizeof(NetPacketPlayersActionData))
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

NetPacketPlayersActionDone::NetPacketPlayersActionDone()
: NetPacket(NET_TYPE_PLAYERS_ACTION_DONE, sizeof(NetPacketPlayersActionDoneData))
{
}

NetPacketPlayersActionDone::~NetPacketPlayersActionDone()
{
}

boost::shared_ptr<NetPacket>
NetPacketPlayersActionDone::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketPlayersActionDone);
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
NetPacketPlayersActionDone::SetData(const NetPacketPlayersActionDone::Data &inData)
{
	NetPacketPlayersActionDoneData *tmpData = (NetPacketPlayersActionDoneData *)GetRawData();
	assert(tmpData);

	tmpData->gameState		= htons(inData.gameState);
	tmpData->playerId		= htons(inData.playerId);
	tmpData->playerAction	= htons(inData.playerAction);
	tmpData->totalPlayerBet	= htonl(inData.totalPlayerBet);
	tmpData->playerMoney	= htonl(inData.playerMoney);
}

void
NetPacketPlayersActionDone::GetData(NetPacketPlayersActionDone::Data &outData) const
{
	NetPacketPlayersActionDoneData *tmpData = (NetPacketPlayersActionDoneData *)GetRawData();
	assert(tmpData);

	outData.gameState		= static_cast<GameState>(ntohs(tmpData->gameState));
	outData.playerId		= ntohs(tmpData->playerId);
	outData.playerAction	= static_cast<PlayerAction>(ntohs(tmpData->playerAction));
	outData.totalPlayerBet	= ntohl(tmpData->totalPlayerBet);
	outData.playerMoney		= ntohl(tmpData->playerMoney);
}

const NetPacketPlayersActionDone *
NetPacketPlayersActionDone::ToNetPacketPlayersActionDone() const
{
	return this;
}

void
NetPacketPlayersActionDone::Check(const NetPacketHeader* data) const
{
	assert(data);

	u_int16_t dataLen = ntohs(data->length);
	if (dataLen != sizeof(NetPacketPlayersActionDoneData))
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}

	NetPacketPlayersActionDoneData *tmpData = (NetPacketPlayersActionDoneData *)data;
	// TODO: Check whether the state is valid.
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
	tmpData->playerBet			= htonl(inData.playerBet);

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
	outData.playerBet		= ntohl(tmpData->playerBet);

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
	if (dataLen != sizeof(NetPacketPlayersActionRejectedData))
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

NetPacketDealFlopCards::NetPacketDealFlopCards()
: NetPacket(NET_TYPE_DEAL_FLOP_CARDS, sizeof(NetPacketDealFlopCardsData))
{
}

NetPacketDealFlopCards::~NetPacketDealFlopCards()
{
}

boost::shared_ptr<NetPacket>
NetPacketDealFlopCards::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketDealFlopCards);
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
NetPacketDealFlopCards::SetData(const NetPacketDealFlopCards::Data &inData)
{
	NetPacketDealFlopCardsData *tmpData = (NetPacketDealFlopCardsData *)GetRawData();
	assert(tmpData);

	tmpData->flopCard1		= htons(inData.flopCards[0]);
	tmpData->flopCard2		= htons(inData.flopCards[1]);
	tmpData->flopCard3		= htons(inData.flopCards[2]);
}

void
NetPacketDealFlopCards::GetData(NetPacketDealFlopCards::Data &outData) const
{
	NetPacketDealFlopCardsData *tmpData = (NetPacketDealFlopCardsData *)GetRawData();
	assert(tmpData);

	outData.flopCards[0]		= ntohs(tmpData->flopCard1);
	outData.flopCards[1]		= ntohs(tmpData->flopCard2);
	outData.flopCards[2]		= ntohs(tmpData->flopCard3);
}

const NetPacketDealFlopCards *
NetPacketDealFlopCards::ToNetPacketDealFlopCards() const
{
	return this;
}

void
NetPacketDealFlopCards::Check(const NetPacketHeader* data) const
{
	assert(data);

	u_int16_t dataLen = ntohs(data->length);
	if (dataLen != sizeof(NetPacketDealFlopCardsData))
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}

	NetPacketDealFlopCardsData *tmpData = (NetPacketDealFlopCardsData *)data;
	if (ntohs(tmpData->flopCard1) > 51 || ntohs(tmpData->flopCard2) > 51 || ntohs(tmpData->flopCard3) > 51)
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}
}

//-----------------------------------------------------------------------------

NetPacketDealTurnCard::NetPacketDealTurnCard()
: NetPacket(NET_TYPE_DEAL_TURN_CARD, sizeof(NetPacketDealTurnCardData))
{
}

NetPacketDealTurnCard::~NetPacketDealTurnCard()
{
}

boost::shared_ptr<NetPacket>
NetPacketDealTurnCard::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketDealTurnCard);
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
NetPacketDealTurnCard::SetData(const NetPacketDealTurnCard::Data &inData)
{
	NetPacketDealTurnCardData *tmpData = (NetPacketDealTurnCardData *)GetRawData();
	assert(tmpData);

	tmpData->turnCard			= htons(inData.turnCard);
}

void
NetPacketDealTurnCard::GetData(NetPacketDealTurnCard::Data &outData) const
{
	NetPacketDealTurnCardData *tmpData = (NetPacketDealTurnCardData *)GetRawData();
	assert(tmpData);

	outData.turnCard			= ntohs(tmpData->turnCard);
}

const NetPacketDealTurnCard *
NetPacketDealTurnCard::ToNetPacketDealTurnCard() const
{
	return this;
}

void
NetPacketDealTurnCard::Check(const NetPacketHeader* data) const
{
	assert(data);

	u_int16_t dataLen = ntohs(data->length);
	if (dataLen != sizeof(NetPacketDealTurnCardData))
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}

	NetPacketDealTurnCardData *tmpData = (NetPacketDealTurnCardData *)data;
	if (ntohs(tmpData->turnCard) > 51)
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}
}

//-----------------------------------------------------------------------------

NetPacketDealRiverCard::NetPacketDealRiverCard()
: NetPacket(NET_TYPE_DEAL_RIVER_CARD, sizeof(NetPacketDealRiverCardData))
{
}

NetPacketDealRiverCard::~NetPacketDealRiverCard()
{
}

boost::shared_ptr<NetPacket>
NetPacketDealRiverCard::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketDealRiverCard);
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
NetPacketDealRiverCard::SetData(const NetPacketDealRiverCard::Data &inData)
{
	NetPacketDealRiverCardData *tmpData = (NetPacketDealRiverCardData *)GetRawData();
	assert(tmpData);

	tmpData->riverCard			= htons(inData.riverCard);
}

void
NetPacketDealRiverCard::GetData(NetPacketDealRiverCard::Data &outData) const
{
	NetPacketDealRiverCardData *tmpData = (NetPacketDealRiverCardData *)GetRawData();
	assert(tmpData);

	outData.riverCard			= ntohs(tmpData->riverCard);
}

const NetPacketDealRiverCard *
NetPacketDealRiverCard::ToNetPacketDealRiverCard() const
{
	return this;
}

void
NetPacketDealRiverCard::Check(const NetPacketHeader* data) const
{
	assert(data);

	u_int16_t dataLen = ntohs(data->length);
	if (dataLen != sizeof(NetPacketDealRiverCardData))
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}

	NetPacketDealRiverCardData *tmpData = (NetPacketDealRiverCardData *)data;
	if (ntohs(tmpData->riverCard) > 51)
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}
}

//-----------------------------------------------------------------------------

NetPacketAllInShowCards::NetPacketAllInShowCards()
: NetPacket(NET_TYPE_ALL_IN_SHOW_CARDS, sizeof(NetPacketAllInShowCardsData))
{
}

NetPacketAllInShowCards::~NetPacketAllInShowCards()
{
}

boost::shared_ptr<NetPacket>
NetPacketAllInShowCards::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketAllInShowCards);
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
NetPacketAllInShowCards::SetData(const NetPacketAllInShowCards::Data &inData)
{
	u_int16_t numPlayerCards = (u_int16_t)inData.playerCards.size();

	if (!numPlayerCards || numPlayerCards > MAX_NUM_PLAYER_CARDS)
		throw NetException(ERR_NET_INVALID_PLAYER_CARDS, 0);

	// Resize the packet so that the data fits in.
	Resize((u_int16_t)
		(sizeof(NetPacketAllInShowCardsData) + numPlayerCards * sizeof(PlayerCardsData)));

	NetPacketAllInShowCardsData *tmpData = (NetPacketAllInShowCardsData *)GetRawData();
	assert(tmpData);

	tmpData->numberOfPlayerCards		= htons(numPlayerCards);

	PlayerCardsList::const_iterator i = inData.playerCards.begin();
	PlayerCardsList::const_iterator end = inData.playerCards.end();

	// Copy the player cards data to continous memory
	PlayerCardsData *curPlayerCardsData =
		(PlayerCardsData *)((char *)tmpData + sizeof(NetPacketAllInShowCardsData));
	while (i != end)
	{
		curPlayerCardsData->playerId		= htons((*i).playerId);
		curPlayerCardsData->card1			= htons((*i).cards[0]);
		curPlayerCardsData->card2			= htons((*i).cards[1]);
		++curPlayerCardsData;
		++i;
	}
}

void
NetPacketAllInShowCards::GetData(NetPacketAllInShowCards::Data &outData) const
{
	NetPacketAllInShowCardsData *tmpData = (NetPacketAllInShowCardsData *)GetRawData();
	assert(tmpData);

	outData.playerCards.clear();

	u_int16_t numPlayerCards = ntohs(tmpData->numberOfPlayerCards);
	PlayerCardsData *curPlayerCardsData =
		(PlayerCardsData *)((char *)tmpData + sizeof(NetPacketAllInShowCardsData));

	// Store all available player cards.
	for (int i = 0; i < numPlayerCards; i++)
	{
		PlayerCards tmpPlayerCards;
		tmpPlayerCards.playerId		= ntohs(curPlayerCardsData->playerId);
		tmpPlayerCards.cards[0]		= ntohs(curPlayerCardsData->card1);
		tmpPlayerCards.cards[1]		= ntohs(curPlayerCardsData->card2);

		outData.playerCards.push_back(tmpPlayerCards);
		++curPlayerCardsData;
	}
}

const NetPacketAllInShowCards *
NetPacketAllInShowCards::ToNetPacketAllInShowCards() const
{
	return this;
}

void
NetPacketAllInShowCards::Check(const NetPacketHeader* data) const
{
	assert(data);

	u_int16_t dataLen = ntohs(data->length);
	if (dataLen < sizeof(NetPacketAllInShowCardsData))
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}

	NetPacketAllInShowCardsData *tmpData = (NetPacketAllInShowCardsData *)data;
	int numPlayerCards = ntohs(tmpData->numberOfPlayerCards);

	if (dataLen !=
		sizeof(NetPacketAllInShowCardsData)
		+ numPlayerCards * sizeof(PlayerCardsData))
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}
	// TODO: semantic checks within PlayerCardsData
}

//-----------------------------------------------------------------------------

NetPacketEndOfHandShowCards::NetPacketEndOfHandShowCards()
: NetPacket(NET_TYPE_END_OF_HAND_SHOW_CARDS, sizeof(NetPacketEndOfHandShowCardsData))
{
}

NetPacketEndOfHandShowCards::~NetPacketEndOfHandShowCards()
{
}

boost::shared_ptr<NetPacket>
NetPacketEndOfHandShowCards::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketEndOfHandShowCards);
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
NetPacketEndOfHandShowCards::SetData(const NetPacketEndOfHandShowCards::Data &inData)
{
	u_int16_t numPlayerResults = (u_int16_t)inData.playerResults.size();

	if (!numPlayerResults || numPlayerResults > MAX_NUM_PLAYER_RESULTS)
		throw NetException(ERR_NET_INVALID_PLAYER_RESULTS, 0);

	// Resize the packet so that the data fits in.
	Resize((u_int16_t)
		(sizeof(NetPacketEndOfHandShowCardsData) + numPlayerResults * sizeof(PlayerResultData)));

	NetPacketEndOfHandShowCardsData *tmpData = (NetPacketEndOfHandShowCardsData *)GetRawData();
	assert(tmpData);

	tmpData->numberOfPlayerResults		= htons(numPlayerResults);

	PlayerResultList::const_iterator i = inData.playerResults.begin();
	PlayerResultList::const_iterator end = inData.playerResults.end();

	// Copy the player result data to continous memory
	PlayerResultData *curPlayerResultData =
		(PlayerResultData *)((char *)tmpData + sizeof(NetPacketEndOfHandShowCardsData));
	while (i != end)
	{
		curPlayerResultData->playerId		= htons((*i).playerId);
		curPlayerResultData->card1			= htons((*i).cards[0]);
		curPlayerResultData->card2			= htons((*i).cards[1]);
		curPlayerResultData->bestHandPos1	= htons((*i).bestHandPos[0]);
		curPlayerResultData->bestHandPos2	= htons((*i).bestHandPos[1]);
		curPlayerResultData->bestHandPos3	= htons((*i).bestHandPos[2]);
		curPlayerResultData->bestHandPos4	= htons((*i).bestHandPos[3]);
		curPlayerResultData->bestHandPos5	= htons((*i).bestHandPos[4]);
		curPlayerResultData->valueOfCards	= htonl((*i).valueOfCards);
		curPlayerResultData->moneyWon		= htonl((*i).moneyWon);
		curPlayerResultData->playerMoney	= htonl((*i).playerMoney);
		++curPlayerResultData;
		++i;
	}
}

void
NetPacketEndOfHandShowCards::GetData(NetPacketEndOfHandShowCards::Data &outData) const
{
	NetPacketEndOfHandShowCardsData *tmpData = (NetPacketEndOfHandShowCardsData *)GetRawData();
	assert(tmpData);

	outData.playerResults.clear();

	u_int16_t numPlayerResults = ntohs(tmpData->numberOfPlayerResults);
	PlayerResultData *curPlayerResultData =
		(PlayerResultData *)((char *)tmpData + sizeof(NetPacketEndOfHandShowCardsData));

	// Store all available player results.
	for (int i = 0; i < numPlayerResults; i++)
	{
		PlayerResult tmpPlayerResult;
		tmpPlayerResult.playerId		= ntohs(curPlayerResultData->playerId);
		tmpPlayerResult.cards[0]		= ntohs(curPlayerResultData->card1);
		tmpPlayerResult.cards[1]		= ntohs(curPlayerResultData->card2);
		tmpPlayerResult.bestHandPos[0]	= ntohs(curPlayerResultData->bestHandPos1);
		tmpPlayerResult.bestHandPos[1]	= ntohs(curPlayerResultData->bestHandPos2);
		tmpPlayerResult.bestHandPos[2]	= ntohs(curPlayerResultData->bestHandPos3);
		tmpPlayerResult.bestHandPos[3]	= ntohs(curPlayerResultData->bestHandPos4);
		tmpPlayerResult.bestHandPos[4]	= ntohs(curPlayerResultData->bestHandPos5);
		tmpPlayerResult.valueOfCards	= ntohl(curPlayerResultData->valueOfCards);
		tmpPlayerResult.moneyWon		= ntohl(curPlayerResultData->moneyWon);
		tmpPlayerResult.playerMoney		= ntohl(curPlayerResultData->playerMoney);

		outData.playerResults.push_back(tmpPlayerResult);
		++curPlayerResultData;
	}
}

const NetPacketEndOfHandShowCards *
NetPacketEndOfHandShowCards::ToNetPacketEndOfHandShowCards() const
{
	return this;
}

void
NetPacketEndOfHandShowCards::Check(const NetPacketHeader* data) const
{
	assert(data);

	u_int16_t dataLen = ntohs(data->length);
	if (dataLen < sizeof(NetPacketEndOfHandShowCardsData))
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}

	NetPacketEndOfHandShowCardsData *tmpData = (NetPacketEndOfHandShowCardsData *)data;
	int numPlayerResults = ntohs(tmpData->numberOfPlayerResults);

	if (dataLen !=
		sizeof(NetPacketEndOfHandShowCardsData)
		+ numPlayerResults * sizeof(PlayerResultData))
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}
	// TODO: semantic checks within PlayerResultData
}

//-----------------------------------------------------------------------------

NetPacketEndOfHandHideCards::NetPacketEndOfHandHideCards()
: NetPacket(NET_TYPE_END_OF_HAND_HIDE_CARDS, sizeof(NetPacketEndOfHandHideCardsData))
{
}

NetPacketEndOfHandHideCards::~NetPacketEndOfHandHideCards()
{
}

boost::shared_ptr<NetPacket>
NetPacketEndOfHandHideCards::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketEndOfHandHideCards);
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
NetPacketEndOfHandHideCards::SetData(const NetPacketEndOfHandHideCards::Data &inData)
{
	NetPacketEndOfHandHideCardsData *tmpData = (NetPacketEndOfHandHideCardsData *)GetRawData();
	assert(tmpData);

	tmpData->playerId			= htons(inData.playerId);
	tmpData->moneyWon			= htonl(inData.moneyWon);
	tmpData->playerMoney		= htonl(inData.playerMoney);
}

void
NetPacketEndOfHandHideCards::GetData(NetPacketEndOfHandHideCards::Data &outData) const
{
	NetPacketEndOfHandHideCardsData *tmpData = (NetPacketEndOfHandHideCardsData *)GetRawData();
	assert(tmpData);

	outData.playerId			= ntohs(tmpData->playerId);
	outData.moneyWon			= ntohl(tmpData->moneyWon);
	outData.playerMoney			= ntohl(tmpData->playerMoney);
}

const NetPacketEndOfHandHideCards *
NetPacketEndOfHandHideCards::ToNetPacketEndOfHandHideCards() const
{
	return this;
}

void
NetPacketEndOfHandHideCards::Check(const NetPacketHeader* data) const
{
	assert(data);

	u_int16_t dataLen = ntohs(data->length);
	if (dataLen != sizeof(NetPacketEndOfHandHideCardsData))
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}
}

//-----------------------------------------------------------------------------

NetPacketEndOfGame::NetPacketEndOfGame()
: NetPacket(NET_TYPE_END_OF_GAME, sizeof(NetPacketEndOfGameData))
{
}

NetPacketEndOfGame::~NetPacketEndOfGame()
{
}

boost::shared_ptr<NetPacket>
NetPacketEndOfGame::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketEndOfGame);
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
NetPacketEndOfGame::SetData(const NetPacketEndOfGame::Data &inData)
{
	NetPacketEndOfGameData *tmpData = (NetPacketEndOfGameData *)GetRawData();
	assert(tmpData);

	tmpData->winnerPlayerId	= htons(inData.winnerPlayerId);
}

void
NetPacketEndOfGame::GetData(NetPacketEndOfGame::Data &outData) const
{
	NetPacketEndOfGameData *tmpData = (NetPacketEndOfGameData *)GetRawData();
	assert(tmpData);

	outData.winnerPlayerId	= ntohs(tmpData->winnerPlayerId);
}

const NetPacketEndOfGame *
NetPacketEndOfGame::ToNetPacketEndOfGame() const
{
	return this;
}

void
NetPacketEndOfGame::Check(const NetPacketHeader* data) const
{
	assert(data);

	u_int16_t dataLen = ntohs(data->length);
	if (dataLen != sizeof(NetPacketEndOfGameData))
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}
}

//-----------------------------------------------------------------------------

NetPacketSendChatText::NetPacketSendChatText()
: NetPacket(NET_TYPE_SEND_CHAT_TEXT, sizeof(NetPacketSendChatTextData))
{
}

NetPacketSendChatText::~NetPacketSendChatText()
{
}

boost::shared_ptr<NetPacket>
NetPacketSendChatText::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketSendChatText);
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
NetPacketSendChatText::SetData(const NetPacketSendChatText::Data &inData)
{
	u_int16_t textLen = (u_int16_t)inData.text.length();

	if (!textLen || textLen > MAX_CHAT_TEXT_SIZE)
		throw NetException(ERR_NET_INVALID_CHAT_TEXT, 0);

	// Resize the packet so that the data fits in.
	Resize((u_int16_t)
		(sizeof(NetPacketSendChatTextData) + ADD_PADDING(textLen)));

	NetPacketSendChatTextData *tmpData = (NetPacketSendChatTextData *)GetRawData();
	assert(tmpData);

	// Set the data.
	tmpData->textLength = htons(textLen);
	char *textPtr = (char *)tmpData + sizeof(NetPacketSendChatTextData);
	memcpy(textPtr, inData.text.c_str(), textLen);
}

void
NetPacketSendChatText::GetData(NetPacketSendChatText::Data &outData) const
{
	// We assume that the data is valid. Validity has already been checked.
	NetPacketSendChatTextData *tmpData = (NetPacketSendChatTextData *)GetRawData();
	assert(tmpData);

	char *textPtr = (char *)tmpData + sizeof(NetPacketSendChatTextData);
	outData.text = string(textPtr, ntohs(tmpData->textLength));
}

const NetPacketSendChatText *
NetPacketSendChatText::ToNetPacketSendChatText() const
{
	return this;
}

void
NetPacketSendChatText::Check(const NetPacketHeader* data) const
{
	assert(data);

	u_int16_t dataLen = ntohs(data->length);
	if (dataLen < sizeof(NetPacketSendChatTextData))
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}

	NetPacketSendChatTextData *tmpData = (NetPacketSendChatTextData *)data;
	int textLength = ntohs(tmpData->textLength);
	// Generous checking - larger packets are allowed.
	if (dataLen <
		sizeof(NetPacketSendChatTextData)
		+ ADD_PADDING(textLength))
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}
	// Check string sizes.
	if (!textLength
		|| textLength > MAX_CHAT_TEXT_SIZE)
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}
}

//-----------------------------------------------------------------------------

NetPacketChatText::NetPacketChatText()
: NetPacket(NET_TYPE_CHAT_TEXT, sizeof(NetPacketChatTextData))
{
}

NetPacketChatText::~NetPacketChatText()
{
}

boost::shared_ptr<NetPacket>
NetPacketChatText::Clone() const
{
	boost::shared_ptr<NetPacket> newPacket(new NetPacketChatText);
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
NetPacketChatText::SetData(const NetPacketChatText::Data &inData)
{
	u_int16_t textLen = (u_int16_t)inData.text.length();

	if (!textLen || textLen > MAX_CHAT_TEXT_SIZE)
		throw NetException(ERR_NET_INVALID_CHAT_TEXT, 0);

	// Resize the packet so that the data fits in.
	Resize((u_int16_t)
		(sizeof(NetPacketChatTextData) + ADD_PADDING(textLen)));

	NetPacketChatTextData *tmpData = (NetPacketChatTextData *)GetRawData();
	assert(tmpData);

	// Set the data.
	tmpData->playerId = htons(inData.playerId);
	tmpData->textLength = htons(textLen);
	char *textPtr = (char *)tmpData + sizeof(NetPacketChatTextData);
	memcpy(textPtr, inData.text.c_str(), textLen);
}

void
NetPacketChatText::GetData(NetPacketChatText::Data &outData) const
{
	// We assume that the data is valid. Validity has already been checked.
	NetPacketChatTextData *tmpData = (NetPacketChatTextData *)GetRawData();
	assert(tmpData);

	outData.playerId = ntohs(tmpData->playerId);
	char *textPtr = (char *)tmpData + sizeof(NetPacketChatTextData);
	outData.text = string(textPtr, ntohs(tmpData->textLength));
}

const NetPacketChatText *
NetPacketChatText::ToNetPacketChatText() const
{
	return this;
}

void
NetPacketChatText::Check(const NetPacketHeader* data) const
{
	assert(data);

	u_int16_t dataLen = ntohs(data->length);
	if (dataLen < sizeof(NetPacketChatTextData))
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}

	NetPacketChatTextData *tmpData = (NetPacketChatTextData *)data;
	int textLength = ntohs(tmpData->textLength);
	// Generous checking - larger packets are allowed.
	if (dataLen <
		sizeof(NetPacketChatTextData)
		+ ADD_PADDING(textLength))
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}
	// Check string sizes.
	if (!textLength
		|| textLength > MAX_CHAT_TEXT_SIZE)
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}
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
		case ERR_NET_PLAYER_KICKED :
			tmpData->reason = htons(NET_ERR_GENERAL_PLAYER_KICKED);
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
		case NET_ERR_GENERAL_PLAYER_KICKED :
			outData.errorCode = ERR_NET_PLAYER_KICKED;
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
	if (dataLen < sizeof(NetPacketErrorData)) // graceful size checking only for error packets
	{
		throw NetException(ERR_SOCK_INVALID_PACKET, 0);
	}
}

//-----------------------------------------------------------------------------

