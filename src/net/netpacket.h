/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2011 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *****************************************************************************/
/* PokerTH network packet. */

#ifndef _NETPACKET_H_
#define _NETPACKET_H_

#include <string>
#include <list>

#include <third_party/asn1/PokerTHMessage.h>
#include <gamedata.h>

#define NET_VERSION_MAJOR			3
#define NET_VERSION_MINOR			0

#define MAX_FILE_DATA_SIZE			256
#define MAX_PACKET_SIZE				384
#define MAX_CHAT_TEXT_SIZE			128

/*#define MIN_PACKET_SIZE				4
#define MAX_NAME_SIZE				64
#define MAX_PASSWORD_SIZE			64
#define MAX_NUM_MANUAL_BLINDS		30
#define MAX_NUM_PLAYER_RESULTS		MAX_NUMBER_OF_PLAYERS
#define MAX_NUM_PLAYER_CARDS		MAX_NUMBER_OF_PLAYERS*/

#define STL_STRING_FROM_OCTET_STRING(_a) (string((const char *)(_a).buf, (_a).size))

// This is just a wrapper class for the ASN.1 structure.
class NetPacket
{
public:
	enum MemAllocType { NoAlloc = 0, Alloc };
	NetPacket(MemAllocType alloc = NoAlloc);
	~NetPacket();

	static boost::shared_ptr<NetPacket> Create(char *data, size_t &dataSize);

	const PokerTHMessage_t *GetMsg() const {
		return m_msg;
	}
	PokerTHMessage_t *GetMsg() {
		return m_msg;
	}

	bool IsClientActivity() const;

	std::string ToString() const;

	static void SetGameData(const GameData &inData, NetGameInfo_t *outData);
	static void GetGameData(const NetGameInfo_t *inData, GameData &outData);

	static int NetErrorToGameError(long netErrorReason);
	static long GameErrorToNetError(int gameErrorReason);

private:
	PokerTHMessage_t *m_msg;
};

typedef std::list<boost::shared_ptr<NetPacket> > NetPacketList;

#endif

