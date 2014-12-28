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

#include <net/validation/authmessagevalidator.h>
#include <net/validation/validationhelper.h>

using namespace std;


AuthMessageValidator::AuthMessageValidator()
{
	m_validationMap.insert(make_pair(AuthMessage_AuthMessageType_Type_AuthClientRequestMessage, ValidateAuthClientRequestMessage));
	m_validationMap.insert(make_pair(AuthMessage_AuthMessageType_Type_AuthServerChallengeMessage, ValidateAuthServerChallengeMessage));
	m_validationMap.insert(make_pair(AuthMessage_AuthMessageType_Type_AuthClientResponseMessage, ValidateAuthClientResponseMessage));
	m_validationMap.insert(make_pair(AuthMessage_AuthMessageType_Type_AuthServerVerificationMessage, ValidateAuthServerVerificationMessage));
}

bool
AuthMessageValidator::IsValidMessage(const AuthMessage &msg) const
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
AuthMessageValidator::ValidateAuthClientRequestMessage(const AuthMessage &msg)
{
	bool retVal = false;
	if (msg.has_authclientrequestmessage()) {
		const AuthClientRequestMessage &request = msg.authclientrequestmessage();
		if (VALIDATE_IS_UINT16(request.requestedversion().majorversion())
				&& VALIDATE_IS_UINT16(request.requestedversion().minorversion())
				&& (!request.has_mylastsessionid() || request.mylastsessionid().size() == 16)
				&& (!request.has_authserverpassword() || VALIDATE_STRING_SIZE(request.authserverpassword(), 1, 64))
				&& (!request.has_nickname() || VALIDATE_STRING_SIZE(request.nickname(), 1, 64))
				&& (!request.has_clientuserdata() || VALIDATE_STRING_SIZE(request.clientuserdata(), 1, 256))
				&& (!request.has_avatarhash() || request.avatarhash().size() == 16)) {

			retVal = true;
		}
	}
	return retVal;
}

bool
AuthMessageValidator::ValidateAuthServerChallengeMessage(const AuthMessage &msg)
{
	bool retVal = false;
	if (msg.has_authserverchallengemessage()) {
		const AuthServerChallengeMessage &challenge = msg.authserverchallengemessage();
		if (VALIDATE_STRING_SIZE(challenge.serverchallenge(), 1, 256)) {
			retVal = true;
		}
	}
	return retVal;
}

bool
AuthMessageValidator::ValidateAuthClientResponseMessage(const AuthMessage &msg)
{
	bool retVal = false;
	if (msg.has_authclientresponsemessage()) {
		const AuthClientResponseMessage &response = msg.authclientresponsemessage();
		if (VALIDATE_STRING_SIZE(response.clientresponse(), 1, 256)) {
			retVal = true;
		}
	}
	return retVal;
}

bool
AuthMessageValidator::ValidateAuthServerVerificationMessage(const AuthMessage &msg)
{
	bool retVal = false;
	if (msg.has_authserververificationmessage()) {
		const AuthServerVerificationMessage &verification = msg.authserververificationmessage();
		if (VALIDATE_STRING_SIZE(verification.serververification(), 1, 256)) {
			retVal = true;
		}
	}
	return retVal;
}

