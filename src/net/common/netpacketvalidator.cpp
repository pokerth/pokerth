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

#include <net/netpacketvalidator.h>
#include <net/netpacket.h>

using namespace std;

#define VALIDATE_IS_UINT16(__val) ((__val) >= 0 && (__val) <= 65535)
#define VALIDATE_STRING_SIZE(__str, __minsize, __maxsize) ((__str).size() >= (__minsize) && (__str).size() <= (__maxsize))
#define VALIDATE_INT_RANGE(__val, __minval, __maxval) ((__val) >= (__minval) && (__val) <= (__maxval))

NetPacketValidator::NetPacketValidator()
{
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_AnnounceMessage, ValidateAnnounceMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_InitMessage, ValidateInitMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_AuthServerChallengeMessage, ValidateAuthServerChallengeMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_AuthClientResponseMessage, ValidateAuthClientResponseMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_AuthServerVerificationMessage, ValidateAuthServerVerificationMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_InitAckMessage, ValidateInitAckMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_AvatarRequestMessage, ValidateAvatarRequestMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_AvatarHeaderMessage, ValidateAvatarHeaderMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_AvatarDataMessage, ValidateAvatarDataMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_AvatarEndMessage, ValidateAvatarEndMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_UnknownAvatarMessage, ValidateUnknownAvatarMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_PlayerListMessage, ValidatePlayerListMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_GameListNewMessage, ValidateGameListNewMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_GameListUpdateMessage, ValidateGameListUpdateMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_GameListPlayerJoinedMessage, ValidateGameListPlayerJoinedMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_GameListPlayerLeftMessage, ValidateGameListPlayerLeftMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_GameListAdminChangedMessage, ValidateGameListAdminChangedMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_PlayerInfoRequestMessage, ValidatePlayerInfoRequestMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_PlayerInfoReplyMessage, ValidatePlayerInfoReplyMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_SubscriptionRequestMessage, ValidateSubscriptionRequestMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_JoinExistingGameMessage, ValidateJoinExistingGameMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_JoinNewGameMessage, ValidateJoinNewGameMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_RejoinExistingGameMessage, ValidateRejoinExistingGameMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_JoinGameAckMessage, ValidateJoinGameAckMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_JoinGameFailedMessage, ValidateJoinGameFailedMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_GamePlayerJoinedMessage, ValidateGamePlayerJoinedMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_GamePlayerLeftMessage, ValidateGamePlayerLeftMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_GameAdminChangedMessage, ValidateGameAdminChangedMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_RemovedFromGameMessage, ValidateRemovedFromGameMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_KickPlayerRequestMessage, ValidateKickPlayerRequestMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_LeaveGameRequestMessage, ValidateLeaveGameRequestMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_InvitePlayerToGameMessage, ValidateInvitePlayerToGameMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_InviteNotifyMessage, ValidateInviteNotifyMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_RejectGameInvitationMessage, ValidateRejectGameInvitationMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_RejectInvNotifyMessage, ValidateRejectInvNotifyMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_StartEventMessage, ValidateStartEventMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_StartEventAckMessage, ValidateStartEventAckMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_GameStartInitialMessage, ValidateGameStartInitialMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_GameStartRejoinMessage, ValidateGameStartRejoinMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_HandStartMessage, ValidateHandStartMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_PlayersTurnMessage, ValidatePlayersTurnMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_MyActionRequestMessage, ValidateMyActionRequestMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_YourActionRejectedMessage, ValidateYourActionRejectedMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_PlayersActionDoneMessage, ValidatePlayersActionDoneMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_DealFlopCardsMessage, ValidateDealFlopCardsMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_DealTurnCardMessage, ValidateDealTurnCardMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_DealRiverCardMessage, ValidateDealRiverCardMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_AllInShowCardsMessage, ValidateAllInShowCardsMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_EndOfHandShowCardsMessage, ValidateEndOfHandShowCardsMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_EndOfHandHideCardsMessage, ValidateEndOfHandHideCardsMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_ShowMyCardsRequestMessage, ValidateShowMyCardsRequestMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_AfterHandShowCardsMessage, ValidateAfterHandShowCardsMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_EndOfGameMessage, ValidateEndOfGameMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_PlayerIdChangedMessage, ValidatePlayerIdChangedMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_AskKickPlayerMessage, ValidateAskKickPlayerMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_AskKickDeniedMessage, ValidateAskKickDeniedMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_StartKickPetitionMessage, ValidateStartKickPetitionMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_VoteKickRequestMessage, ValidateVoteKickRequestMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_VoteKickReplyMessage, ValidateVoteKickReplyMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_KickPetitionUpdateMessage, ValidateKickPetitionUpdateMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_EndKickPetitionMessage, ValidateEndKickPetitionMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_StatisticsMessage, ValidateStatisticsMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_ChatRequestMessage, ValidateChatRequestMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_ChatMessage, ValidateChatMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_ChatRejectMessage, ValidateChatRejectMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_DialogMessage, ValidateDialogMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_TimeoutWarningMessage, ValidateTimeoutWarningMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_ResetTimeoutMessage, ValidateResetTimeoutMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_ReportAvatarMessage, ValidateReportAvatarMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_ReportAvatarAckMessage, ValidateReportAvatarAckMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_ReportGameMessage, ValidateReportGameMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_ReportGameAckMessage, ValidateReportGameAckMessage));
	m_validationMap.insert(make_pair(PokerTHMessage_PokerTHMessageType_Type_ErrorMessage, ValidateErrorMessage));
}

bool
NetPacketValidator::IsValidPacket(const NetPacket &packet) const
{
	// Default: Invalid packet.
	bool retVal = false;
	ValidateFunctorMap::const_iterator pos = m_validationMap.find(packet.GetMsg()->messagetype());
	packet.GetMsg()->afterhandshowcardsmessage();
	if (pos != m_validationMap.end()) {
		// Call validation functor.
		retVal = pos->second(packet);
	}
	return retVal;
}

bool
NetPacketValidator::ValidateAnnounceMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_announcemessage()) {
		const AnnounceMessage &msg = packet.GetMsg()->announcemessage();
		if (VALIDATE_IS_UINT16(msg.protocolversion().majorversion())
			&& VALIDATE_IS_UINT16(msg.protocolversion().minorversion())
			&& VALIDATE_IS_UINT16(msg.latestgameversion().majorversion())
			&& VALIDATE_IS_UINT16(msg.latestgameversion().minorversion())
			&& VALIDATE_IS_UINT16(msg.latestbetarevision())
			&& VALIDATE_IS_UINT16(msg.numplayersonserver())) {

			retVal = true;
		}
	}
	return retVal;
}

bool
NetPacketValidator::ValidateInitMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_initmessage()) {
		const InitMessage &msg = packet.GetMsg()->initmessage();
		if (VALIDATE_IS_UINT16(msg.requestedversion().majorversion())
			&& VALIDATE_IS_UINT16(msg.requestedversion().minorversion())
			&& (!msg.has_mylastsessionid() || msg.mylastsessionid().size() == 16)
			&& (!msg.has_authserverpassword() || VALIDATE_STRING_SIZE(msg.authserverpassword(), 1, 64))) {

			retVal = true;
		}
	}
	return retVal;
}

bool
NetPacketValidator::ValidateAuthServerChallengeMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_authserverchallengemessage()) {
		const AuthServerChallengeMessage &msg = packet.GetMsg()->authserverchallengemessage();
		if (VALIDATE_STRING_SIZE(msg.serverchallenge(), 1, 256)) {
			retVal = true;
		}
	}
	return retVal;
}

bool
NetPacketValidator::ValidateAuthClientResponseMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_authclientresponsemessage()) {
		const AuthClientResponseMessage &msg = packet.GetMsg()->authclientresponsemessage();
		if (VALIDATE_STRING_SIZE(msg.clientresponse(), 1, 256)) {
			retVal = true;
		}
	}
	return retVal;
}

bool
NetPacketValidator::ValidateAuthServerVerificationMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_authserververificationmessage()) {
		const AuthServerVerificationMessage &msg = packet.GetMsg()->authserververificationmessage();
		if (VALIDATE_STRING_SIZE(msg.serververification(), 1, 256)) {
			retVal = true;
		}
	}
	return retVal;
}

bool
NetPacketValidator::ValidateInitAckMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_initackmessage()) {
		const InitAckMessage &msg = packet.GetMsg()->initackmessage();
		if (msg.yoursessionid().size() == 16
			&& msg.yourplayerid() != 0
			&& (!msg.has_youravatarhash() || msg.youravatarhash().size() == 16)
			&& (!msg.has_rejoingameid() || msg.rejoingameid() != 0)) {

			retVal = true;
		}
	}
	return retVal;
}

bool
NetPacketValidator::ValidateAvatarRequestMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_avatarrequestmessage()) {
		const AvatarRequestMessage &msg = packet.GetMsg()->avatarrequestmessage();
		if (msg.requestid() != 0
			&& msg.avatarhash().size() == 16) {

			retVal = true;
		}
	}
	return retVal;
}

bool
NetPacketValidator::ValidateAvatarHeaderMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_avatarheadermessage()) {
		const AvatarHeaderMessage &msg = packet.GetMsg()->avatarheadermessage();
		if (msg.requestid() != 0
			&& (VALIDATE_INT_RANGE(msg.avatarsize(), 32, 30720))) {

			retVal = true;
		}
	}
	return retVal;
}

bool
NetPacketValidator::ValidateAvatarDataMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_avatardatamessage()) {
		const AvatarDataMessage &msg = packet.GetMsg()->avatardatamessage();
		if (msg.requestid() != 0
			&& VALIDATE_STRING_SIZE(msg.avatarblock(), 1, 256)) {

			retVal = true;
		}
	}
	return retVal;
}

bool
NetPacketValidator::ValidateAvatarEndMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_avatarendmessage()) {
		const AvatarEndMessage &msg = packet.GetMsg()->avatarendmessage();
		if (msg.requestid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
NetPacketValidator::ValidateUnknownAvatarMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_unknownavatarmessage()) {
		const UnknownAvatarMessage &msg = packet.GetMsg()->unknownavatarmessage();
		if (msg.requestid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
NetPacketValidator::ValidatePlayerListMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_playerlistmessage()) {
		const PlayerListMessage &msg = packet.GetMsg()->playerlistmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateGameListNewMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_gamelistnewmessage()) {
		const GameListNewMessage &msg = packet.GetMsg()->gamelistnewmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateGameListUpdateMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_gamelistupdatemessage()) {
		const GameListUpdateMessage &msg = packet.GetMsg()->gamelistupdatemessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateGameListPlayerJoinedMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_gamelistplayerjoinedmessage()) {
		const GameListPlayerJoinedMessage &msg = packet.GetMsg()->gamelistplayerjoinedmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateGameListPlayerLeftMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_gamelistplayerleftmessage()) {
		const GameListPlayerLeftMessage &msg = packet.GetMsg()->gamelistplayerleftmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateGameListAdminChangedMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_gamelistadminchangedmessage()) {
		const GameListAdminChangedMessage &msg = packet.GetMsg()->gamelistadminchangedmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidatePlayerInfoRequestMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_playerinforequestmessage()) {
		const PlayerInfoRequestMessage &msg = packet.GetMsg()->playerinforequestmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidatePlayerInfoReplyMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_playerinforeplymessage()) {
		const PlayerInfoReplyMessage &msg = packet.GetMsg()->playerinforeplymessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateSubscriptionRequestMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_subscriptionrequestmessage()) {
		const SubscriptionRequestMessage &msg = packet.GetMsg()->subscriptionrequestmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateJoinExistingGameMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_joinexistinggamemessage()) {
		const JoinExistingGameMessage &msg = packet.GetMsg()->joinexistinggamemessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateJoinNewGameMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_joinnewgamemessage()) {
		const JoinNewGameMessage &msg = packet.GetMsg()->joinnewgamemessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateRejoinExistingGameMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_rejoinexistinggamemessage()) {
		const RejoinExistingGameMessage &msg = packet.GetMsg()->rejoinexistinggamemessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateJoinGameAckMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_joingameackmessage()) {
		const JoinGameAckMessage &msg = packet.GetMsg()->joingameackmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateJoinGameFailedMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_joingamefailedmessage()) {
		const JoinGameFailedMessage &msg = packet.GetMsg()->joingamefailedmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateGamePlayerJoinedMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_gameplayerjoinedmessage()) {
		const GamePlayerJoinedMessage &msg = packet.GetMsg()->gameplayerjoinedmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateGamePlayerLeftMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_gameplayerleftmessage()) {
		const GamePlayerLeftMessage &msg = packet.GetMsg()->gameplayerleftmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateGameAdminChangedMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_gameadminchangedmessage()) {
		const GameAdminChangedMessage &msg = packet.GetMsg()->gameadminchangedmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateRemovedFromGameMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_removedfromgamemessage()) {
		const RemovedFromGameMessage &msg = packet.GetMsg()->removedfromgamemessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateKickPlayerRequestMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_kickplayerrequestmessage()) {
		const KickPlayerRequestMessage &msg = packet.GetMsg()->kickplayerrequestmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateLeaveGameRequestMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_leavegamerequestmessage()) {
		const LeaveGameRequestMessage &msg = packet.GetMsg()->leavegamerequestmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateInvitePlayerToGameMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_inviteplayertogamemessage()) {
		const InvitePlayerToGameMessage &msg = packet.GetMsg()->inviteplayertogamemessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateInviteNotifyMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_invitenotifymessage()) {
		const InviteNotifyMessage &msg = packet.GetMsg()->invitenotifymessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateRejectGameInvitationMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_rejectgameinvitationmessage()) {
		const RejectGameInvitationMessage &msg = packet.GetMsg()->rejectgameinvitationmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateRejectInvNotifyMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_rejectinvnotifymessage()) {
		const RejectInvNotifyMessage &msg = packet.GetMsg()->rejectinvnotifymessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateStartEventMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_starteventmessage()) {
		const StartEventMessage &msg = packet.GetMsg()->starteventmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateStartEventAckMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_starteventackmessage()) {
		const StartEventAckMessage &msg = packet.GetMsg()->starteventackmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateGameStartInitialMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_gamestartinitialmessage()) {
		const GameStartInitialMessage &msg = packet.GetMsg()->gamestartinitialmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateGameStartRejoinMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_gamestartrejoinmessage()) {
		const GameStartRejoinMessage &msg = packet.GetMsg()->gamestartrejoinmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateHandStartMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_handstartmessage()) {
		const HandStartMessage &msg = packet.GetMsg()->handstartmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidatePlayersTurnMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_playersturnmessage()) {
		const PlayersTurnMessage &msg = packet.GetMsg()->playersturnmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateMyActionRequestMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_myactionrequestmessage()) {
		const MyActionRequestMessage &msg = packet.GetMsg()->myactionrequestmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateYourActionRejectedMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_youractionrejectedmessage()) {
		const YourActionRejectedMessage &msg = packet.GetMsg()->youractionrejectedmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidatePlayersActionDoneMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_playersactiondonemessage()) {
		const PlayersActionDoneMessage &msg = packet.GetMsg()->playersactiondonemessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateDealFlopCardsMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_dealflopcardsmessage()) {
		const DealFlopCardsMessage &msg = packet.GetMsg()->dealflopcardsmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateDealTurnCardMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_dealturncardmessage()) {
		const DealTurnCardMessage &msg = packet.GetMsg()->dealturncardmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateDealRiverCardMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_dealrivercardmessage()) {
		const DealRiverCardMessage &msg = packet.GetMsg()->dealrivercardmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateAllInShowCardsMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_allinshowcardsmessage()) {
		const AllInShowCardsMessage &msg = packet.GetMsg()->allinshowcardsmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateEndOfHandShowCardsMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_endofhandshowcardsmessage()) {
		const EndOfHandShowCardsMessage &msg = packet.GetMsg()->endofhandshowcardsmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateEndOfHandHideCardsMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_endofhandhidecardsmessage()) {
		const EndOfHandHideCardsMessage &msg = packet.GetMsg()->endofhandhidecardsmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateShowMyCardsRequestMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_showmycardsrequestmessage()) {
		const ShowMyCardsRequestMessage &msg = packet.GetMsg()->showmycardsrequestmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateAfterHandShowCardsMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_afterhandshowcardsmessage()) {
		const AfterHandShowCardsMessage &msg = packet.GetMsg()->afterhandshowcardsmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateEndOfGameMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_endofgamemessage()) {
		const EndOfGameMessage &msg = packet.GetMsg()->endofgamemessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidatePlayerIdChangedMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_playeridchangedmessage()) {
		const PlayerIdChangedMessage &msg = packet.GetMsg()->playeridchangedmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateAskKickPlayerMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_askkickplayermessage()) {
		const AskKickPlayerMessage &msg = packet.GetMsg()->askkickplayermessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateAskKickDeniedMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_askkickdeniedmessage()) {
		const AskKickDeniedMessage &msg = packet.GetMsg()->askkickdeniedmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateStartKickPetitionMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_startkickpetitionmessage()) {
		const StartKickPetitionMessage &msg = packet.GetMsg()->startkickpetitionmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateVoteKickRequestMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_votekickrequestmessage()) {
		const VoteKickRequestMessage &msg = packet.GetMsg()->votekickrequestmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateVoteKickReplyMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_votekickreplymessage()) {
		const VoteKickReplyMessage &msg = packet.GetMsg()->votekickreplymessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateKickPetitionUpdateMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_kickpetitionupdatemessage()) {
		const KickPetitionUpdateMessage &msg = packet.GetMsg()->kickpetitionupdatemessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateEndKickPetitionMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_endkickpetitionmessage()) {
		const EndKickPetitionMessage &msg = packet.GetMsg()->endkickpetitionmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateStatisticsMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_statisticsmessage()) {
		const StatisticsMessage &msg = packet.GetMsg()->statisticsmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateChatRequestMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_chatrequestmessage()) {
		const ChatRequestMessage &msg = packet.GetMsg()->chatrequestmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateChatMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_chatmessage()) {
		const ChatMessage &msg = packet.GetMsg()->chatmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateChatRejectMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_chatrejectmessage()) {
		const ChatRejectMessage &msg = packet.GetMsg()->chatrejectmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateDialogMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_dialogmessage()) {
		const DialogMessage &msg = packet.GetMsg()->dialogmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateTimeoutWarningMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_timeoutwarningmessage()) {
		const TimeoutWarningMessage &msg = packet.GetMsg()->timeoutwarningmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateResetTimeoutMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_resettimeoutmessage()) {
		const ResetTimeoutMessage &msg = packet.GetMsg()->resettimeoutmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateReportAvatarMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_reportavatarmessage()) {
		const ReportAvatarMessage &msg = packet.GetMsg()->reportavatarmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateReportAvatarAckMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_reportavatarackmessage()) {
		const ReportAvatarAckMessage &msg = packet.GetMsg()->reportavatarackmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateReportGameMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_reportgamemessage()) {
		const ReportGameMessage &msg = packet.GetMsg()->reportgamemessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateReportGameAckMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_reportgameackmessage()) {
		const ReportGameAckMessage &msg = packet.GetMsg()->reportgameackmessage();
		retVal = true;
	}
	return retVal;
}

bool
NetPacketValidator::ValidateErrorMessage(const NetPacket &packet)
{
	bool retVal = false;
	if (packet.GetMsg()->has_errormessage()) {
		const ErrorMessage &msg = packet.GetMsg()->errormessage();
		retVal = true;
	}
	return retVal;
}

