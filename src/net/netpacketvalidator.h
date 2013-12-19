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
/* PokerTH network packet validation. */

#ifndef _NETPACKETVALIDATOR_H_
#define _NETPACKETVALIDATOR_H_

#include <map>

class NetPacket;
class NetGameInfo;

class NetPacketValidator
{
public:
	NetPacketValidator();

	bool IsValidPacket(const NetPacket &packet) const;

protected:
	static bool ValidateAnnounceMessage(const NetPacket &packet);
	static bool ValidateInitMessage(const NetPacket &packet);
	static bool ValidateAuthServerChallengeMessage(const NetPacket &packet);
	static bool ValidateAuthClientResponseMessage(const NetPacket &packet);
	static bool ValidateAuthServerVerificationMessage(const NetPacket &packet);
	static bool ValidateInitAckMessage(const NetPacket &packet);
	static bool ValidateAvatarRequestMessage(const NetPacket &packet);
	static bool ValidateAvatarHeaderMessage(const NetPacket &packet);
	static bool ValidateAvatarDataMessage(const NetPacket &packet);
	static bool ValidateAvatarEndMessage(const NetPacket &packet);
	static bool ValidateUnknownAvatarMessage(const NetPacket &packet);
	static bool ValidatePlayerListMessage(const NetPacket &packet);
	static bool ValidateGameListNewMessage(const NetPacket &packet);
	static bool ValidateGameListUpdateMessage(const NetPacket &packet);
	static bool ValidateGameListPlayerJoinedMessage(const NetPacket &packet);
	static bool ValidateGameListPlayerLeftMessage(const NetPacket &packet);
	static bool ValidateGameListAdminChangedMessage(const NetPacket &packet);
	static bool ValidatePlayerInfoRequestMessage(const NetPacket &packet);
	static bool ValidatePlayerInfoReplyMessage(const NetPacket &packet);
	static bool ValidateSubscriptionRequestMessage(const NetPacket &packet);
	static bool ValidateJoinExistingGameMessage(const NetPacket &packet);
	static bool ValidateJoinNewGameMessage(const NetPacket &packet);
	static bool ValidateRejoinExistingGameMessage(const NetPacket &packet);
	static bool ValidateJoinGameAckMessage(const NetPacket &packet);
	static bool ValidateJoinGameFailedMessage(const NetPacket &packet);
	static bool ValidateGamePlayerJoinedMessage(const NetPacket &packet);
	static bool ValidateGamePlayerLeftMessage(const NetPacket &packet);
	static bool ValidateGameAdminChangedMessage(const NetPacket &packet);
	static bool ValidateRemovedFromGameMessage(const NetPacket &packet);
	static bool ValidateKickPlayerRequestMessage(const NetPacket &packet);
	static bool ValidateLeaveGameRequestMessage(const NetPacket &packet);
	static bool ValidateInvitePlayerToGameMessage(const NetPacket &packet);
	static bool ValidateInviteNotifyMessage(const NetPacket &packet);
	static bool ValidateRejectGameInvitationMessage(const NetPacket &packet);
	static bool ValidateRejectInvNotifyMessage(const NetPacket &packet);
	static bool ValidateStartEventMessage(const NetPacket &packet);
	static bool ValidateStartEventAckMessage(const NetPacket &packet);
	static bool ValidateGameStartInitialMessage(const NetPacket &packet);
	static bool ValidateGameStartRejoinMessage(const NetPacket &packet);
	static bool ValidateHandStartMessage(const NetPacket &packet);
	static bool ValidatePlayersTurnMessage(const NetPacket &packet);
	static bool ValidateMyActionRequestMessage(const NetPacket &packet);
	static bool ValidateYourActionRejectedMessage(const NetPacket &packet);
	static bool ValidatePlayersActionDoneMessage(const NetPacket &packet);
	static bool ValidateDealFlopCardsMessage(const NetPacket &packet);
	static bool ValidateDealTurnCardMessage(const NetPacket &packet);
	static bool ValidateDealRiverCardMessage(const NetPacket &packet);
	static bool ValidateAllInShowCardsMessage(const NetPacket &packet);
	static bool ValidateEndOfHandShowCardsMessage(const NetPacket &packet);
	static bool ValidateEndOfHandHideCardsMessage(const NetPacket &packet);
	static bool ValidateShowMyCardsRequestMessage(const NetPacket &packet);
	static bool ValidateAfterHandShowCardsMessage(const NetPacket &packet);
	static bool ValidateEndOfGameMessage(const NetPacket &packet);
	static bool ValidatePlayerIdChangedMessage(const NetPacket &packet);
	static bool ValidateAskKickPlayerMessage(const NetPacket &packet);
	static bool ValidateAskKickDeniedMessage(const NetPacket &packet);
	static bool ValidateStartKickPetitionMessage(const NetPacket &packet);
	static bool ValidateVoteKickRequestMessage(const NetPacket &packet);
	static bool ValidateVoteKickReplyMessage(const NetPacket &packet);
	static bool ValidateKickPetitionUpdateMessage(const NetPacket &packet);
	static bool ValidateEndKickPetitionMessage(const NetPacket &packet);
	static bool ValidateStatisticsMessage(const NetPacket &packet);
	static bool ValidateChatRequestMessage(const NetPacket &packet);
	static bool ValidateChatMessage(const NetPacket &packet);
	static bool ValidateChatRejectMessage(const NetPacket &packet);
	static bool ValidateDialogMessage(const NetPacket &packet);
	static bool ValidateTimeoutWarningMessage(const NetPacket &packet);
	static bool ValidateResetTimeoutMessage(const NetPacket &packet);
	static bool ValidateReportAvatarMessage(const NetPacket &packet);
	static bool ValidateReportAvatarAckMessage(const NetPacket &packet);
	static bool ValidateReportGameMessage(const NetPacket &packet);
	static bool ValidateReportGameAckMessage(const NetPacket &packet);
	static bool ValidateErrorMessage(const NetPacket &packet);
	static bool ValidateAdminRemoveGameMessage(const NetPacket &packet);
	static bool ValidateAdminRemoveGameAckMessage(const NetPacket &packet);
	static bool ValidateAdminBanPlayerMessage(const NetPacket &packet);
	static bool ValidateAdminBanPlayerAckMessage(const NetPacket &packet);
	static bool ValidateGameListSpectatorJoinedMessage(const NetPacket &packet);
	static bool ValidateGameListSpectatorLeftMessage(const NetPacket &packet);
	static bool ValidateGameSpectatorJoinedMessage(const NetPacket &packet);
	static bool ValidateGameSpectatorLeftMessage(const NetPacket &packet);

	static bool ValidateGameInfo(const NetGameInfo &gameInfo);

	typedef bool (*ValidateFunctor)(const NetPacket &);
	typedef std::map<int, ValidateFunctor> ValidateFunctorMap;
private:

	ValidateFunctorMap m_validationMap;
};

#endif

