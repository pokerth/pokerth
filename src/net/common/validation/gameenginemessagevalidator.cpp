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

#include <net/validation/gameenginemessagevalidator.h>
#include <net/validation/validationhelper.h>

using namespace std;


GameEngineMessageValidator::GameEngineMessageValidator()
{
	m_validationMap.insert(make_pair(GameEngineMessage_GameEngineMessageType_Type_HandStartMessage, ValidateHandStartMessage));
	m_validationMap.insert(make_pair(GameEngineMessage_GameEngineMessageType_Type_PlayersTurnMessage, ValidatePlayersTurnMessage));
	m_validationMap.insert(make_pair(GameEngineMessage_GameEngineMessageType_Type_MyActionRequestMessage, ValidateMyActionRequestMessage));
	m_validationMap.insert(make_pair(GameEngineMessage_GameEngineMessageType_Type_YourActionRejectedMessage, ValidateYourActionRejectedMessage));
	m_validationMap.insert(make_pair(GameEngineMessage_GameEngineMessageType_Type_PlayersActionDoneMessage, ValidatePlayersActionDoneMessage));
	m_validationMap.insert(make_pair(GameEngineMessage_GameEngineMessageType_Type_DealFlopCardsMessage, ValidateDealFlopCardsMessage));
	m_validationMap.insert(make_pair(GameEngineMessage_GameEngineMessageType_Type_DealTurnCardMessage, ValidateDealTurnCardMessage));
	m_validationMap.insert(make_pair(GameEngineMessage_GameEngineMessageType_Type_DealRiverCardMessage, ValidateDealRiverCardMessage));
	m_validationMap.insert(make_pair(GameEngineMessage_GameEngineMessageType_Type_AllInShowCardsMessage, ValidateAllInShowCardsMessage));
	m_validationMap.insert(make_pair(GameEngineMessage_GameEngineMessageType_Type_EndOfHandShowCardsMessage, ValidateEndOfHandShowCardsMessage));
	m_validationMap.insert(make_pair(GameEngineMessage_GameEngineMessageType_Type_EndOfHandHideCardsMessage, ValidateEndOfHandHideCardsMessage));
	m_validationMap.insert(make_pair(GameEngineMessage_GameEngineMessageType_Type_ShowMyCardsRequestMessage, ValidateShowMyCardsRequestMessage));
	m_validationMap.insert(make_pair(GameEngineMessage_GameEngineMessageType_Type_AfterHandShowCardsMessage, ValidateAfterHandShowCardsMessage));
}

bool
GameEngineMessageValidator::IsValidMessage(const GameEngineMessage &msg) const
{
	// Default: Invalid packet.
	bool retVal = false;
	ValidateFunctorMap::const_iterator pos = m_validationMap.find(msg.messagetype());
	if (pos != m_validationMap.end()) {
		// Call validation functor.
		retVal = pos->second(msg);
	}
	return retVal;
}

bool
GameEngineMessageValidator::ValidateHandStartMessage(const GameEngineMessage &msg)
{
	bool retVal = false;
	if (msg.has_handstartmessage()) {
		const HandStartMessage &handStart = msg.handstartmessage();
		if (VALIDATE_UINT_RANGE(handStart.smallblind(), 1, 100000000)
				&& VALIDATE_LIST_SIZE(handStart.seatstates(), 2, 10)) {
			retVal = true;
		}
	}
	return retVal;
}

bool
GameEngineMessageValidator::ValidatePlayersTurnMessage(const GameEngineMessage &msg)
{
	bool retVal = false;
	if (msg.has_playersturnmessage()) {
		const PlayersTurnMessage &playersTurn = msg.playersturnmessage();
		if (playersTurn.playerid() != 0 ) {
			retVal = true;
		}
	}
	return retVal;
}

bool
GameEngineMessageValidator::ValidateMyActionRequestMessage(const GameEngineMessage &msg)
{
	bool retVal = false;
	if (msg.has_myactionrequestmessage()) {
		const MyActionRequestMessage &actionRequest = msg.myactionrequestmessage();
		if (actionRequest.handnum() != 0
				&& VALIDATE_UINT_UPPER(actionRequest.myrelativebet(), 10000000)) {

			retVal = true;
		}
	}
	return retVal;
}

bool
GameEngineMessageValidator::ValidateYourActionRejectedMessage(const GameEngineMessage &msg)
{
	bool retVal = false;
	if (msg.has_youractionrejectedmessage()) {
		//const YourActionRejectedMessage &actionRejected = msg.youractionrejectedmessage();
		retVal = true;
	}
	return retVal;
}

bool
GameEngineMessageValidator::ValidatePlayersActionDoneMessage(const GameEngineMessage &msg)
{
	bool retVal = false;
	if (msg.has_playersactiondonemessage()) {
		const PlayersActionDoneMessage &actionDone = msg.playersactiondonemessage();
		if (actionDone.playerid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
GameEngineMessageValidator::ValidateDealFlopCardsMessage(const GameEngineMessage &msg)
{
	bool retVal = false;
	if (msg.has_dealflopcardsmessage()) {
		const DealFlopCardsMessage &dealFlop = msg.dealflopcardsmessage();
		if (VALIDATE_UINT_UPPER(dealFlop.flopcard1(), 51)
				&& VALIDATE_UINT_UPPER(dealFlop.flopcard2(), 51)
				&& VALIDATE_UINT_UPPER(dealFlop.flopcard3(), 51)) {
			retVal = true;
		}
	}
	return retVal;
}

bool
GameEngineMessageValidator::ValidateDealTurnCardMessage(const GameEngineMessage &msg)
{
	bool retVal = false;
	if (msg.has_dealturncardmessage()) {
		const DealTurnCardMessage &dealTurn = msg.dealturncardmessage();
		if (VALIDATE_UINT_UPPER(dealTurn.turncard(), 51)) {
			retVal = true;
		}
	}
	return retVal;
}

bool
GameEngineMessageValidator::ValidateDealRiverCardMessage(const GameEngineMessage &msg)
{
	bool retVal = false;
	if (msg.has_dealrivercardmessage()) {
		const DealRiverCardMessage &dealRiver = msg.dealrivercardmessage();
		if (VALIDATE_UINT_UPPER(dealRiver.rivercard(), 51)) {
			retVal = true;
		}
	}
	return retVal;
}

bool
GameEngineMessageValidator::ValidateAllInShowCardsMessage(const GameEngineMessage &msg)
{
	bool retVal = false;
	if (msg.has_allinshowcardsmessage()) {
		const AllInShowCardsMessage &allInShow = msg.allinshowcardsmessage();
		if (VALIDATE_LIST_SIZE(allInShow.playersallin(), 1, 10)) {
			retVal = true;
		}
	}
	return retVal;
}

bool
GameEngineMessageValidator::ValidateEndOfHandShowCardsMessage(const GameEngineMessage &msg)
{
	bool retVal = false;
	if (msg.has_endofhandshowcardsmessage()) {
		const EndOfHandShowCardsMessage &endHandShow = msg.endofhandshowcardsmessage();
		if (VALIDATE_LIST_SIZE(endHandShow.playerresults(), 1, 10)) {
			retVal = true;
		}
	}
	return retVal;
}

bool
GameEngineMessageValidator::ValidateEndOfHandHideCardsMessage(const GameEngineMessage &msg)
{
	bool retVal = false;
	if (msg.has_endofhandhidecardsmessage()) {
		const EndOfHandHideCardsMessage &endHandHide = msg.endofhandhidecardsmessage();
		if (endHandHide.playerid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
GameEngineMessageValidator::ValidateShowMyCardsRequestMessage(const GameEngineMessage &/*msg*/)
{
	//bool retVal = false;
	//if (msg.has_showmycardsrequestmessage()) {
	//	const ShowMyCardsRequestMessage &showCards = msg.showmycardsrequestmessage();
	//	retVal = true;
	//}
	//return retVal;
	return true; // Empty packet, always valid.
}

bool
GameEngineMessageValidator::ValidateAfterHandShowCardsMessage(const GameEngineMessage &msg)
{
	bool retVal = false;
	if (msg.has_afterhandshowcardsmessage()) {
		//const AfterHandShowCardsMessage &afterHandShow = msg.afterhandshowcardsmessage();
		retVal = true;
	}
	return retVal;
}

