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

#include <net/validation/gamemessagevalidator.h>
#include <net/validation/gamemanagementmessagevalidator.h>
#include <net/validation/gameenginemessagevalidator.h>
#include <net/validation/validationhelper.h>
#include <boost/mem_fn.hpp>
#include <boost/bind.hpp>

using namespace std;

GameMessageValidator::GameMessageValidator()
{
	m_validationMap.insert(make_pair(GameMessage_GameMessageType_Type_GameManagementMessage, boost::bind(boost::mem_fn(&GameMessageValidator::ValidateGameManagementMessage), this, _1)));
	m_validationMap.insert(make_pair(GameMessage_GameMessageType_Type_GameEngineMessage, boost::bind(boost::mem_fn(&GameMessageValidator::ValidateGameEngineMessage), this, _1)));
	m_managementValidator.reset(new GameManagementMessageValidator);
	m_engineValidator.reset(new GameEngineMessageValidator);
}

bool
GameMessageValidator::IsValidMessage(const GameMessage &msg) const
{
	// Default: Invalid packet.
	bool retVal = false;
	if (msg.gameid() != 0) {
		ValidateFunctorMap::const_iterator pos = m_validationMap.find(msg.messagetype());
		if (pos != m_validationMap.end()) {
			// Call validation functor.
			retVal = pos->second(msg);
		}
	}
	return retVal;
}

bool
GameMessageValidator::ValidateGameManagementMessage(const GameMessage &msg)
{
	bool retVal = false;
	if (msg.has_gamemanagementmessage()) {
		retVal = m_managementValidator->IsValidMessage(msg.gamemanagementmessage());
	}
	return retVal;
}

bool
GameMessageValidator::ValidateGameEngineMessage(const GameMessage &msg)
{
	bool retVal = false;
	if (msg.has_gameenginemessage()) {
		retVal = m_engineValidator->IsValidMessage(msg.gameenginemessage());
	}
	return retVal;
}

