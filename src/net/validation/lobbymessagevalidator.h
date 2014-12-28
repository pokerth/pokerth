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
/* PokerTH lobby message validation. */

#ifndef _LOBBYMESSAGEVALIDATOR_H_
#define _LOBBYMESSAGEVALIDATOR_H_

#include <map>
#include <boost/function.hpp>

class LobbyMessage;
class NetGameInfo;

class LobbyMessageValidator
{
public:
	LobbyMessageValidator();

	bool IsValidMessage(const LobbyMessage &msg) const;

protected:
	static bool ValidateInitDoneMessage(const LobbyMessage &msg);
	static bool ValidateAvatarRequestMessage(const LobbyMessage &msg);
	static bool ValidateAvatarHeaderMessage(const LobbyMessage &msg);
	static bool ValidateAvatarDataMessage(const LobbyMessage &msg);
	static bool ValidateAvatarEndMessage(const LobbyMessage &msg);
	static bool ValidateUnknownAvatarMessage(const LobbyMessage &msg);
	static bool ValidatePlayerListMessage(const LobbyMessage &msg);
	static bool ValidateGameListNewMessage(const LobbyMessage &msg);
	static bool ValidateGameListUpdateMessage(const LobbyMessage &msg);
	static bool ValidateGameListPlayerJoinedMessage(const LobbyMessage &msg);
	static bool ValidateGameListPlayerLeftMessage(const LobbyMessage &msg);
	static bool ValidateGameListSpectatorJoinedMessage(const LobbyMessage &msg);
	static bool ValidateGameListSpectatorLeftMessage(const LobbyMessage &msg);
	static bool ValidateGameListAdminChangedMessage(const LobbyMessage &msg);
	static bool ValidatePlayerInfoRequestMessage(const LobbyMessage &msg);
	static bool ValidatePlayerInfoReplyMessage(const LobbyMessage &msg);
	static bool ValidateSubscriptionRequestMessage(const LobbyMessage &msg);
	static bool ValidateSubscriptionReplyMessage(const LobbyMessage &msg);
	static bool ValidateCreateGameMessage(const LobbyMessage &msg);
	static bool ValidateCreateGameFailedMessage(const LobbyMessage &msg);
	static bool ValidateJoinGameMessage(const LobbyMessage &msg);
	static bool ValidateRejoinGameMessage(const LobbyMessage &msg);
	static bool ValidateJoinGameAckMessage(const LobbyMessage &msg);
	static bool ValidateJoinGameFailedMessage(const LobbyMessage &msg);
	static bool ValidateInviteNotifyMessage(const LobbyMessage &msg);
	static bool ValidateRejectGameInvitationMessage(const LobbyMessage &msg);
	static bool ValidateRejectInvNotifyMessage(const LobbyMessage &msg);
	static bool ValidateStatisticsMessage(const LobbyMessage &msg);
	static bool ValidateChatRequestMessage(const LobbyMessage &msg);
	static bool ValidateChatMessage(const LobbyMessage &msg);
	static bool ValidateChatRejectMessage(const LobbyMessage &msg);
	static bool ValidateDialogMessage(const LobbyMessage &msg);
	static bool ValidateTimeoutWarningMessage(const LobbyMessage &msg);
	static bool ValidateResetTimeoutMessage(const LobbyMessage &msg);
	static bool ValidateReportAvatarMessage(const LobbyMessage &msg);
	static bool ValidateReportAvatarAckMessage(const LobbyMessage &msg);
	static bool ValidateReportGameMessage(const LobbyMessage &msg);
	static bool ValidateReportGameAckMessage(const LobbyMessage &msg);
	static bool ValidateAdminRemoveGameMessage(const LobbyMessage &msg);
	static bool ValidateAdminRemoveGameAckMessage(const LobbyMessage &msg);
	static bool ValidateAdminBanPlayerMessage(const LobbyMessage &msg);
	static bool ValidateAdminBanPlayerAckMessage(const LobbyMessage &msg);
	static bool ValidateErrorMessage(const LobbyMessage &msg);

	static bool ValidateGameInfo(const NetGameInfo &gameInfo);

	typedef boost::function<bool (const LobbyMessage &)> ValidateFunctor;
	typedef std::map<int, ValidateFunctor> ValidateFunctorMap;
private:

	ValidateFunctorMap m_validationMap;
};

#endif

