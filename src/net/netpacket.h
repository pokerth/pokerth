/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
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
 *                                                                           *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/
/* PokerTH network packet. */

#ifndef _NETPACKET_H_
#define _NETPACKET_H_

#include <string>
#include <list>

#include <third_party/protobuf/pokerth.pb.h>
#include <gamedata.h>

#define NET_VERSION_MAJOR			5
#define NET_VERSION_MINOR			1

#define NET_HEADER_SIZE				4

#define MAX_FILE_DATA_SIZE			256
#define MAX_PACKET_SIZE				384
#define MAX_CHAT_TEXT_SIZE			128

/*#define MIN_PACKET_SIZE				4
#define MAX_NAME_SIZE				64
#define MAX_PASSWORD_SIZE			64
#define MAX_NUM_MANUAL_BLINDS		30
#define MAX_NUM_PLAYER_RESULTS		MAX_NUMBER_OF_PLAYERS
#define MAX_NUM_PLAYER_CARDS		MAX_NUMBER_OF_PLAYERS*/

// This is just a wrapper class for the protocol buffer.
class NetPacket
{
public:
	NetPacket();
	NetPacket(PokerTHMessage *msg);
	~NetPacket();

	static boost::shared_ptr<NetPacket> Create(const char *data, size_t dataSize);

	const PokerTHMessage *GetMsg() const
	{
		return m_msg;
	}
	PokerTHMessage *GetMsg()
	{
		return m_msg;
	}

	bool IsClientActivity() const;

	std::string ToString() const;

	static void SetGameData(const GameData &inData, NetGameInfo &outData);
	static void GetGameData(const NetGameInfo &inData, GameData &outData);

	static int NetErrorToGameError(ErrorMessage::ErrorReason netErrorReason);
	static ErrorMessage::ErrorReason GameErrorToNetError(int gameErrorReason);

private:
	PokerTHMessage *m_msg;
};

typedef std::list<boost::shared_ptr<NetPacket> > NetPacketList;

#endif

