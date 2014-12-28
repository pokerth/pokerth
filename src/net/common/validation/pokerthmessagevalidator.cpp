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

#include <net/validation/pokerthmessagevalidator.h>
#include <net/validation/authmessagevalidator.h>
#include <net/validation/lobbymessagevalidator.h>
#include <net/validation/gamemessagevalidator.h>
#include <net/validation/validationhelper.h>
#include <boost/mem_fn.hpp>
#include <boost/bind.hpp>

using namespace std;


PokerTHMessageValidator::PokerTHMessageValidator()
{
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_AnnounceMessage, ValidateAnnounceMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_AuthMessage, boost::bind(boost::mem_fn(&PokerTHMessageValidator::ValidateAuthMessage), this, _1)));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_LobbyMessage, boost::bind(boost::mem_fn(&PokerTHMessageValidator::ValidateLobbyMessage), this, _1)));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_GameMessage, boost::bind(boost::mem_fn(&PokerTHMessageValidator::ValidateGameMessage), this, _1)));

	m_authValidator.reset(new AuthMessageValidator);
	m_lobbyValidator.reset(new LobbyMessageValidator);
	m_gameValidator.reset(new GameMessageValidator);
}

bool
PokerTHMessageValidator::IsValidMessage(const PokerTHMessage &msg) const
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
PokerTHMessageValidator::ValidateAnnounceMessage(const PokerTHMessage &msg)
{
	bool retVal = false;
	if (msg.has_announcemessage()) {
		const AnnounceMessage &announce = msg.announcemessage();
		if (VALIDATE_IS_UINT16(announce.protocolversion().majorversion())
				&& VALIDATE_IS_UINT16(announce.protocolversion().minorversion())
				&& VALIDATE_IS_UINT16(announce.latestgameversion().majorversion())
				&& VALIDATE_IS_UINT16(announce.latestgameversion().minorversion())
				&& VALIDATE_IS_UINT16(announce.latestbetarevision())
				&& VALIDATE_IS_UINT16(announce.numplayersonserver())) {

			retVal = true;
		}
	}
	return retVal;
}

bool
PokerTHMessageValidator::ValidateAuthMessage(const PokerTHMessage &msg)
{
	bool retVal = false;
	if (msg.has_authmessage()) {
		retVal = m_authValidator->IsValidMessage(msg.authmessage());
	}
	return retVal;
}

bool
PokerTHMessageValidator::ValidateLobbyMessage(const PokerTHMessage &msg)
{
	bool retVal = false;
	if (msg.has_lobbymessage()) {
		retVal = m_lobbyValidator->IsValidMessage(msg.lobbymessage());
	}
	return retVal;
}

bool
PokerTHMessageValidator::ValidateGameMessage(const PokerTHMessage &msg)
{
	bool retVal = false;
	if (msg.has_gamemessage()) {
		retVal = m_gameValidator->IsValidMessage(msg.gamemessage());
	}
	return retVal;
}

