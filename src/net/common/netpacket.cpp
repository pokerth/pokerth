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

#include <net/netpacket.h>
#include <net/socket_msg.h>

#include <boost/foreach.hpp>

using namespace std;

NetPacket::NetPacket()
{
	m_msg = PokerTHMessage::default_instance().New();
}

NetPacket::NetPacket(PokerTHMessage *msg)
	: m_msg(msg)
{
}

NetPacket::~NetPacket()
{
	delete m_msg;
}

boost::shared_ptr<NetPacket>
NetPacket::Create(const char *data, size_t dataSize)
{
	boost::shared_ptr<NetPacket> tmpPacket;

	// Check minimum requirements.
	if (data && dataSize > 0) {
		PokerTHMessage *msg = PokerTHMessage::default_instance().New();
		if (msg->ParseFromArray(data, static_cast<int>(dataSize))) {
			tmpPacket.reset(new NetPacket(msg));
		} else {
			delete msg;
		}
	}
	return tmpPacket;
}

bool
NetPacket::IsClientActivity() const
{
	bool retVal = false;
	if (m_msg &&
			(m_msg->messagetype() == PokerTHMessage::Type_InitMessage
			 || m_msg->messagetype() == PokerTHMessage::Type_JoinNewGameMessage
			 || m_msg->messagetype() == PokerTHMessage::Type_JoinExistingGameMessage
			 || m_msg->messagetype() == PokerTHMessage::Type_RejoinExistingGameMessage
			 || m_msg->messagetype() == PokerTHMessage::Type_KickPlayerRequestMessage
			 || m_msg->messagetype() == PokerTHMessage::Type_LeaveGameRequestMessage
			 || m_msg->messagetype() == PokerTHMessage::Type_StartEventMessage
			 || m_msg->messagetype() == PokerTHMessage::Type_MyActionRequestMessage
			 || m_msg->messagetype() == PokerTHMessage::Type_ResetTimeoutMessage
			 || m_msg->messagetype() == PokerTHMessage::Type_ChatRequestMessage)) {
		retVal = true;
	}
	return retVal;
}

string
NetPacket::ToString() const
{
	return m_msg ? m_msg->SerializeAsString() : "NULL";
}

void
NetPacket::SetGameData(const GameData &inData, NetGameInfo &outData)
{
	outData.set_netgametype(static_cast<NetGameInfo::NetGameType>(inData.gameType));
	outData.set_allowspectators(inData.allowSpectators);
	outData.set_maxnumplayers(inData.maxNumberOfPlayers);
	outData.set_raiseintervalmode(static_cast<NetGameInfo::RaiseIntervalMode>(inData.raiseIntervalMode));
	if (inData.raiseIntervalMode == RAISE_ON_HANDNUMBER) {
		outData.set_raiseeveryhands(inData.raiseSmallBlindEveryHandsValue);
	} else {
		outData.set_raiseeveryminutes(inData.raiseSmallBlindEveryMinutesValue);
	}
	outData.set_endraisemode(static_cast<NetGameInfo::EndRaiseMode>(inData.afterManualBlindsMode));
	outData.set_proposedguispeed(inData.guiSpeed);
	outData.set_delaybetweenhands(inData.delayBetweenHandsSec);
	outData.set_playeractiontimeout(inData.playerActionTimeoutSec);
	outData.set_endraisesmallblindvalue(inData.afterMBAlwaysRaiseValue);
	outData.set_firstsmallblind(inData.firstSmallBlind);
	outData.set_startmoney(inData.startMoney);

	BOOST_FOREACH(int manualBlind, inData.manualBlindsList) {
		outData.add_manualblinds(manualBlind);
	}
}

void
NetPacket::GetGameData(const NetGameInfo &inData, GameData &outData)
{
	int numManualBlinds					= inData.manualblinds_size();

	outData.gameType					= static_cast<GameType>(inData.netgametype());
	outData.allowSpectators				= inData.allowspectators();
	outData.maxNumberOfPlayers			= inData.maxnumplayers();
	outData.raiseIntervalMode			= static_cast<RaiseIntervalMode>(inData.raiseintervalmode());
	outData.raiseSmallBlindEveryHandsValue = outData.raiseSmallBlindEveryMinutesValue = 0;
	if (outData.raiseIntervalMode == RAISE_ON_HANDNUMBER)
		outData.raiseSmallBlindEveryHandsValue = inData.raiseeveryhands();
	else
		outData.raiseSmallBlindEveryMinutesValue = inData.raiseeveryminutes();
	outData.raiseMode					= numManualBlinds > 0 ? MANUAL_BLINDS_ORDER : DOUBLE_BLINDS;
	outData.afterManualBlindsMode		= static_cast<AfterManualBlindsMode>(inData.endraisemode());
	outData.guiSpeed					= inData.proposedguispeed();
	outData.delayBetweenHandsSec		= inData.delaybetweenhands();
	outData.playerActionTimeoutSec		= inData.playeractiontimeout();
	outData.firstSmallBlind				= inData.firstsmallblind();
	outData.afterMBAlwaysRaiseValue		= inData.endraisesmallblindvalue();
	outData.startMoney					= inData.startmoney();

	for (int i = 0; i < numManualBlinds; i++) {
		outData.manualBlindsList.push_back(static_cast<int>(inData.manualblinds(i)));
	}
}

int
NetPacket::NetErrorToGameError(ErrorMessage::ErrorReason netErrorReason)
{
	int retVal;
	switch(netErrorReason) {
	case ErrorMessage::initVersionNotSupported :
		retVal = ERR_NET_VERSION_NOT_SUPPORTED;
		break;
	case ErrorMessage::initServerFull :
		retVal = ERR_NET_SERVER_FULL;
		break;
	case ErrorMessage::initAuthFailure :
		retVal = ERR_NET_INVALID_PASSWORD;
		break;
	case ErrorMessage::initPlayerNameInUse :
		retVal = ERR_NET_PLAYER_NAME_IN_USE;
		break;
	case ErrorMessage::initInvalidPlayerName :
		retVal = ERR_NET_INVALID_PLAYER_NAME;
		break;
	case ErrorMessage::initServerMaintenance :
		retVal = ERR_NET_SERVER_MAINTENANCE;
		break;
	case ErrorMessage::initBlocked :
		retVal = ERR_NET_INIT_BLOCKED;
		break;
	case ErrorMessage::avatarTooLarge :
		retVal = ERR_NET_AVATAR_TOO_LARGE;
		break;
	case ErrorMessage::invalidPacket :
		retVal = ERR_SOCK_INVALID_PACKET;
		break;
	case ErrorMessage::invalidState :
		retVal = ERR_SOCK_INVALID_STATE;
		break;
	case ErrorMessage::kickedFromServer :
		retVal = ERR_NET_PLAYER_KICKED;
		break;
	case ErrorMessage::bannedFromServer :
		retVal = ERR_NET_PLAYER_BANNED;
		break;
	case ErrorMessage::blockedByServer :
		retVal = ERR_NET_PLAYER_BLOCKED;
		break;
	case ErrorMessage::sessionTimeout :
		retVal = ERR_NET_SESSION_TIMED_OUT;
		break;
	default :
		retVal = ERR_SOCK_INTERNAL;
		break;
	}
	return retVal;
}

ErrorMessage::ErrorReason
NetPacket::GameErrorToNetError(int gameErrorReason)
{
	ErrorMessage::ErrorReason retVal;
	switch(gameErrorReason) {
	case ERR_NET_VERSION_NOT_SUPPORTED :
		retVal = ErrorMessage::initVersionNotSupported;
		break;
	case ERR_NET_SERVER_FULL :
		retVal = ErrorMessage::initServerFull;
		break;
	case ERR_NET_INVALID_PASSWORD :
		retVal = ErrorMessage::initAuthFailure;
		break;
	case ERR_NET_PLAYER_NAME_IN_USE :
		retVal = ErrorMessage::initPlayerNameInUse;
		break;
	case ERR_NET_INVALID_PLAYER_NAME :
		retVal = ErrorMessage::initInvalidPlayerName;
		break;
	case ERR_NET_SERVER_MAINTENANCE :
		retVal = ErrorMessage::initServerMaintenance;
		break;
	case ERR_NET_INIT_BLOCKED :
		retVal = ErrorMessage::initBlocked;
		break;
	case ERR_NET_AVATAR_TOO_LARGE :
		retVal = ErrorMessage::avatarTooLarge;
		break;
	case ERR_SOCK_INVALID_PACKET :
		retVal = ErrorMessage::invalidPacket;
		break;
	case ERR_SOCK_INVALID_STATE :
		retVal = ErrorMessage::invalidState;
		break;
	case ERR_NET_PLAYER_KICKED :
		retVal = ErrorMessage::kickedFromServer;
		break;
	case ERR_NET_PLAYER_BLOCKED :
		retVal = ErrorMessage::blockedByServer;
		break;
	case ERR_NET_PLAYER_BANNED :
		retVal = ErrorMessage::bannedFromServer;
		break;
	case ERR_NET_SESSION_TIMED_OUT :
		retVal = ErrorMessage::sessionTimeout;
		break;
	default :
		retVal = ErrorMessage::reserved;
		break;
	}
	return retVal;
}

