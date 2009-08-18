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
#include <net/socket_msg.h>

using namespace std;

NetPacket::NetPacket(MemAllocType alloc)
: m_msg(NULL)
{
	if (alloc != NoAlloc)
		m_msg = (PokerTHMessage_t *)calloc(1, sizeof(PokerTHMessage_t));
}

NetPacket::~NetPacket()
{
	ASN_STRUCT_FREE(asn_DEF_PokerTHMessage, m_msg);
}

boost::shared_ptr<NetPacket>
NetPacket::Create(char *data, unsigned &dataSize)
{
	boost::shared_ptr<NetPacket> tmpPacket;

	// Check minimum requirements.
	if (data && dataSize > 0)
	{
		PokerTHMessage_t *msg = NULL;
		asn_dec_rval_t retVal = ber_decode(0, &asn_DEF_PokerTHMessage, (void **)&msg, data, dataSize);
		if(retVal.code == RC_OK)
		{
			// ASN.1 BER decoding was successful.
			tmpPacket.reset(new NetPacket(NoAlloc));
			tmpPacket->m_msg = msg;
			// Consume the bytes.
			if (retVal.consumed < dataSize)
			{
				dataSize -= retVal.consumed;
				memmove(data, data + retVal.consumed, dataSize);
			}
			else
				dataSize = 0;
		}
		else
		{
			// Free the partially decoded message (if applicable).
			ASN_STRUCT_FREE(asn_DEF_PokerTHMessage, msg);
		}
	}
	return tmpPacket;
}

bool
NetPacket::IsClientActivity() const
{
	bool retVal = false;
	if (m_msg &&
		(m_msg->present == PokerTHMessage_PR_initMessage
		|| m_msg->present == PokerTHMessage_PR_joinGameRequestMessage
		|| m_msg->present == PokerTHMessage_PR_kickPlayerRequestMessage
		|| m_msg->present == PokerTHMessage_PR_leaveGameRequestMessage
		|| m_msg->present == PokerTHMessage_PR_startEventMessage
		|| m_msg->present == PokerTHMessage_PR_myActionRequestMessage
		|| m_msg->present == PokerTHMessage_PR_resetTimeoutMessage
		|| m_msg->present == PokerTHMessage_PR_chatRequestMessage))
	{
		retVal = true;
	}
	return retVal;
}

static int
net_packet_print_to_string(const void *buffer, size_t size, void *packetStr)
{
	string *tmpString = (string *)packetStr;
	*tmpString += string((const char *)buffer, size);
	return 0;
}

string
NetPacket::ToString() const
{
	string packetString;
	xer_encode(&asn_DEF_PokerTHMessage, m_msg, XER_F_BASIC, &net_packet_print_to_string, &packetString);
	return packetString;
}

void
NetPacket::SetGameData(const GameData &inData, NetGameInfo_t *outData)
{
	assert(outData);

	int numManualBlinds = (int)inData.manualBlindsList.size();

	outData->maxNumPlayers				= inData.maxNumberOfPlayers;
	outData->raiseIntervalMode.present	= static_cast<raiseIntervalMode_PR>(inData.raiseIntervalMode);
	if (inData.raiseIntervalMode == RAISE_ON_HANDNUMBER)
	{
		outData->raiseIntervalMode.present = raiseIntervalMode_PR_raiseEveryHands;
		outData->raiseIntervalMode.choice.raiseEveryHands = inData.raiseSmallBlindEveryHandsValue;
	}
	else
	{
		outData->raiseIntervalMode.present = raiseIntervalMode_PR_raiseEveryMinutes;
		outData->raiseIntervalMode.choice.raiseEveryMinutes = inData.raiseSmallBlindEveryMinutesValue;
	}
	outData->endRaiseMode				= inData.afterManualBlindsMode;
	outData->proposedGuiSpeed			= inData.guiSpeed;
	outData->playerActionTimeout		= inData.playerActionTimeoutSec;
	outData->endRaiseSmallBlindValue	= inData.afterMBAlwaysRaiseValue;
	outData->firstSmallBlind			= inData.firstSmallBlind;
	outData->startMoney					= inData.startMoney;

	if (numManualBlinds)
	{
		list<int>::const_iterator i = inData.manualBlindsList.begin();
		list<int>::const_iterator end = inData.manualBlindsList.end();
		while (i != end)
		{
			long *manualBlind = (long *)calloc(1, sizeof(long));
			*manualBlind = *i;
			ASN_SEQUENCE_ADD(&outData->manualBlinds.list, manualBlind);
			++i;
		}
	}
}

void
NetPacket::GetGameData(const NetGameInfo_t *inData, GameData &outData)
{
	assert(inData);

	int numManualBlinds = (int)inData->manualBlinds.list.count;

	outData.maxNumberOfPlayers			= inData->maxNumPlayers;
	outData.raiseIntervalMode			= static_cast<RaiseIntervalMode>(inData->raiseIntervalMode.present);
	outData.raiseSmallBlindEveryHandsValue = outData.raiseSmallBlindEveryMinutesValue = 0;
	if (inData->raiseIntervalMode.present == raiseIntervalMode_PR_raiseEveryHands)
		outData.raiseSmallBlindEveryHandsValue = inData->raiseIntervalMode.choice.raiseEveryHands;
	else
		outData.raiseSmallBlindEveryMinutesValue = inData->raiseIntervalMode.choice.raiseEveryMinutes;
	outData.raiseMode					= numManualBlinds > 0 ? MANUAL_BLINDS_ORDER : DOUBLE_BLINDS;
	outData.afterManualBlindsMode		= static_cast<AfterManualBlindsMode>(inData->endRaiseMode);
	outData.guiSpeed					= inData->proposedGuiSpeed;
	outData.playerActionTimeoutSec		= inData->playerActionTimeout;
	outData.firstSmallBlind				= inData->firstSmallBlind;
	outData.afterMBAlwaysRaiseValue		= inData->endRaiseSmallBlindValue;
	outData.startMoney					= inData->startMoney;

	if (numManualBlinds)
	{
		long **manualBlindsPtr = inData->manualBlinds.list.array;
		int counter = 0;
		for (int i = 0; i < numManualBlinds; i++)
		{
			outData.manualBlindsList.push_back((int)*manualBlindsPtr[i]);
			counter++;
		}
	}
}

int
NetPacket::NetErrorToGameError(long netErrorReason)
{
	int retVal;
	switch(netErrorReason)
	{
		case errorReason_errorInitVersionNotSupported :
			retVal = ERR_NET_VERSION_NOT_SUPPORTED;
			break;
		case errorReason_errorInitServerFull :
			retVal = ERR_NET_SERVER_FULL;
			break;
		case errorReason_errorInitAuthFailure :
			retVal = ERR_NET_INVALID_PASSWORD;
			break;
		case errorReason_errorInitPlayerNameInUse :
			retVal = ERR_NET_PLAYER_NAME_IN_USE;
			break;
		case errorReason_errorInitInvalidPlayerName :
			retVal = ERR_NET_INVALID_PLAYER_NAME;
			break;
		case errorReason_errorInitServerMaintenance :
			retVal = ERR_NET_SERVER_MAINTENANCE;
			break;
		case errorReason_errorAvatarTooLarge :
			retVal = ERR_NET_AVATAR_TOO_LARGE;
			break;
		case errorReason_errorAvatarUploadBlocked :
			retVal = ERR_NET_AVATAR_UPLOAD_BLOCKED;
			break;
		case errorReason_errorInvalidPacket :
			retVal = ERR_SOCK_INVALID_PACKET;
			break;
		case errorReason_errorInvalidState :
			retVal = ERR_SOCK_INVALID_STATE;
			break;
		case errorReason_errorKickedFromServer :
			retVal = ERR_NET_PLAYER_KICKED;
			break;
		case errorReason_errorBannedFromServer :
			retVal = ERR_NET_PLAYER_BANNED;
			break;
		case errorReason_errorSessionTimeout :
			retVal = ERR_NET_SESSION_TIMED_OUT;
			break;
		default :
			retVal = ERR_SOCK_INTERNAL;
			break;
	}
	return retVal;
}

long
NetPacket::GameErrorToNetError(int gameErrorReason)
{
	int retVal;
	switch(gameErrorReason)
	{
		case ERR_NET_VERSION_NOT_SUPPORTED :
			retVal = errorReason_errorInitVersionNotSupported;
			break;
		case ERR_NET_SERVER_FULL :
			retVal = errorReason_errorInitServerFull;
			break;
		case ERR_NET_INVALID_PASSWORD :
			retVal = errorReason_errorInitAuthFailure;
			break;
		case ERR_NET_PLAYER_NAME_IN_USE :
			retVal = errorReason_errorInitPlayerNameInUse;
			break;
		case ERR_NET_INVALID_PLAYER_NAME :
			retVal = errorReason_errorInitInvalidPlayerName;
			break;
		case ERR_NET_SERVER_MAINTENANCE :
			retVal = errorReason_errorInitServerMaintenance;
			break;
		case ERR_NET_AVATAR_TOO_LARGE :
			retVal = errorReason_errorAvatarTooLarge;
			break;
		case ERR_NET_AVATAR_UPLOAD_BLOCKED :
			retVal = errorReason_errorAvatarUploadBlocked;
			break;
		case ERR_SOCK_INVALID_PACKET :
			retVal = errorReason_errorInvalidPacket;
			break;
		case ERR_SOCK_INVALID_STATE :
			retVal = errorReason_errorInvalidState;
			break;
		case ERR_NET_PLAYER_KICKED :
			retVal = errorReason_errorKickedFromServer;
			break;
		case ERR_NET_PLAYER_BANNED :
			retVal = errorReason_errorBannedFromServer;
			break;
		case ERR_NET_SESSION_TIMED_OUT :
			retVal = errorReason_errorSessionTimeout;
			break;
		default :
			retVal = errorReason_errorReserved;
			break;
	}
	return retVal;
}

