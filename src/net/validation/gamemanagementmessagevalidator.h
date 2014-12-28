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
/* PokerTH game management message validation. */

#ifndef _GAMEMANAGEMENTMESSAGEVALIDATOR_H_
#define _GAMEMANAGEMENTMESSAGEVALIDATOR_H_

#include <map>
#include <boost/function.hpp>

class GameManagementMessage;

class GameManagementMessageValidator
{
public:
	GameManagementMessageValidator();

	bool IsValidMessage(const GameManagementMessage &msg) const;

protected:
	static bool ValidateJoinGameMessage(const GameManagementMessage &msg);
	static bool ValidateRejoinGameMessage(const GameManagementMessage &msg);
	static bool ValidateJoinGameAckMessage(const GameManagementMessage &msg);
	static bool ValidateJoinGameFailedMessage(const GameManagementMessage &msg);
	static bool ValidateGamePlayerJoinedMessage(const GameManagementMessage &msg);
	static bool ValidateGamePlayerLeftMessage(const GameManagementMessage &msg);
	static bool ValidateGameSpectatorJoinedMessage(const GameManagementMessage &msg);
	static bool ValidateGameSpectatorLeftMessage(const GameManagementMessage &msg);
	static bool ValidateGameAdminChangedMessage(const GameManagementMessage &msg);
	static bool ValidateRemovedFromGameMessage(const GameManagementMessage &msg);
	static bool ValidateKickPlayerRequestMessage(const GameManagementMessage &msg);
	static bool ValidateLeaveGameRequestMessage(const GameManagementMessage &msg);
	static bool ValidateInvitePlayerToGameMessage(const GameManagementMessage &msg);
	static bool ValidateStartEventMessage(const GameManagementMessage &msg);
	static bool ValidateStartEventAckMessage(const GameManagementMessage &msg);
	static bool ValidateGameStartInitialMessage(const GameManagementMessage &msg);
	static bool ValidateGameStartRejoinMessage(const GameManagementMessage &msg);
	static bool ValidateEndOfGameMessage(const GameManagementMessage &msg);
	static bool ValidatePlayerIdChangedMessage(const GameManagementMessage &msg);
	static bool ValidateAskKickPlayerMessage(const GameManagementMessage &msg);
	static bool ValidateAskKickDeniedMessage(const GameManagementMessage &msg);
	static bool ValidateStartKickPetitionMessage(const GameManagementMessage &msg);
	static bool ValidateVoteKickRequestMessage(const GameManagementMessage &msg);
	static bool ValidateVoteKickReplyMessage(const GameManagementMessage &msg);
	static bool ValidateKickPetitionUpdateMessage(const GameManagementMessage &msg);
	static bool ValidateEndKickPetitionMessage(const GameManagementMessage &msg);
	static bool ValidateChatRequestMessage(const GameManagementMessage &msg);
	static bool ValidateChatMessage(const GameManagementMessage &msg);
	static bool ValidateChatRejectMessage(const GameManagementMessage &msg);
	static bool ValidateErrorMessage(const GameManagementMessage &msg);

	typedef boost::function<bool (const GameManagementMessage &)> ValidateFunctor;
	typedef std::map<int, ValidateFunctor> ValidateFunctorMap;
private:

	ValidateFunctorMap m_validationMap;
};

#endif

