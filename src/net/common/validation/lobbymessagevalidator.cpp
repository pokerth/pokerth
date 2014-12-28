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

#include <net/validation/lobbymessagevalidator.h>
#include <net/validation/validationhelper.h>

using namespace std;

LobbyMessageValidator::LobbyMessageValidator()
{
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_InitDoneMessage, ValidateInitDoneMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_AvatarRequestMessage, ValidateAvatarRequestMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_AvatarHeaderMessage, ValidateAvatarHeaderMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_AvatarDataMessage, ValidateAvatarDataMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_AvatarEndMessage, ValidateAvatarEndMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_UnknownAvatarMessage, ValidateUnknownAvatarMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_PlayerListMessage, ValidatePlayerListMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_GameListNewMessage, ValidateGameListNewMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_GameListUpdateMessage, ValidateGameListUpdateMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_GameListPlayerJoinedMessage, ValidateGameListPlayerJoinedMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_GameListPlayerLeftMessage, ValidateGameListPlayerLeftMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_GameListAdminChangedMessage, ValidateGameListAdminChangedMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_PlayerInfoRequestMessage, ValidatePlayerInfoRequestMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_PlayerInfoReplyMessage, ValidatePlayerInfoReplyMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_SubscriptionRequestMessage, ValidateSubscriptionRequestMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_CreateGameMessage, ValidateCreateGameMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_JoinGameMessage, ValidateJoinGameMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_RejoinGameMessage, ValidateRejoinGameMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_JoinGameAckMessage, ValidateJoinGameAckMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_JoinGameFailedMessage, ValidateJoinGameFailedMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_InviteNotifyMessage, ValidateInviteNotifyMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_RejectGameInvitationMessage, ValidateRejectGameInvitationMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_RejectInvNotifyMessage, ValidateRejectInvNotifyMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_StatisticsMessage, ValidateStatisticsMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_ChatRequestMessage, ValidateChatRequestMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_ChatMessage, ValidateChatMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_ChatRejectMessage, ValidateChatRejectMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_DialogMessage, ValidateDialogMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_TimeoutWarningMessage, ValidateTimeoutWarningMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_ResetTimeoutMessage, ValidateResetTimeoutMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_ReportAvatarMessage, ValidateReportAvatarMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_ReportAvatarAckMessage, ValidateReportAvatarAckMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_ReportGameMessage, ValidateReportGameMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_ReportGameAckMessage, ValidateReportGameAckMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_ErrorMessage, ValidateErrorMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_AdminRemoveGameMessage, ValidateAdminRemoveGameMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_AdminRemoveGameAckMessage, ValidateAdminRemoveGameAckMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_AdminBanPlayerMessage, ValidateAdminBanPlayerMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_AdminBanPlayerAckMessage, ValidateAdminBanPlayerAckMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_GameListSpectatorJoinedMessage, ValidateGameListSpectatorJoinedMessage));
	m_validationMap.insert(make_pair(LobbyMessage_LobbyMessageType_Type_GameListSpectatorLeftMessage, ValidateGameListSpectatorLeftMessage));
}

bool
LobbyMessageValidator::IsValidMessage(const LobbyMessage &msg) const
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
LobbyMessageValidator::ValidateInitDoneMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_initdonemessage()) {
		const InitDoneMessage &initDone = msg.initdonemessage();
		if (initDone.yoursessionid().size() == 16
				&& initDone.yourplayerid() != 0
				&& (!initDone.has_youravatarhash() || initDone.youravatarhash().size() == 16)
				&& (!initDone.has_rejoingameid() || initDone.rejoingameid() != 0)) {

			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateAvatarRequestMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_avatarrequestmessage()) {
		const AvatarRequestMessage &avatarRequest = msg.avatarrequestmessage();
		if (avatarRequest.requestid() != 0
				&& avatarRequest.avatarhash().size() == 16) {

			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateAvatarHeaderMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_avatarheadermessage()) {
		const AvatarHeaderMessage &avatarHeader = msg.avatarheadermessage();
		if (avatarHeader.requestid() != 0
				&& VALIDATE_UINT_RANGE(avatarHeader.avatarsize(), 32, 30720)) {

			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateAvatarDataMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_avatardatamessage()) {
		const AvatarDataMessage &avatarData = msg.avatardatamessage();
		if (avatarData.requestid() != 0
				&& VALIDATE_STRING_SIZE(avatarData.avatarblock(), 1, 256)) {

			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateAvatarEndMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_avatarendmessage()) {
		const AvatarEndMessage &avatarEnd = msg.avatarendmessage();
		if (avatarEnd.requestid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateUnknownAvatarMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_unknownavatarmessage()) {
		const UnknownAvatarMessage &unknownAvatar = msg.unknownavatarmessage();
		if (unknownAvatar.requestid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidatePlayerListMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_playerlistmessage()) {
		const PlayerListMessage &player = msg.playerlistmessage();
		if (player.playerid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateGameListNewMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_gamelistnewmessage()) {
		const GameListNewMessage &gameNew = msg.gamelistnewmessage();
		if (gameNew.gameid() != 0
				&& VALIDATE_LIST_SIZE(gameNew.playerids(), 0, 10)
				&& gameNew.adminplayerid() != 0
				&& ValidateGameInfo(gameNew.gameinfo())) {

			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateGameListUpdateMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_gamelistupdatemessage()) {
		const GameListUpdateMessage &gameUpdate = msg.gamelistupdatemessage();
		if (gameUpdate.gameid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateGameListPlayerJoinedMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_gamelistplayerjoinedmessage()) {
		const GameListPlayerJoinedMessage &playerJoined = msg.gamelistplayerjoinedmessage();
		if (playerJoined.gameid() != 0 && playerJoined.playerid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateGameListPlayerLeftMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_gamelistplayerleftmessage()) {
		const GameListPlayerLeftMessage &playerLeft = msg.gamelistplayerleftmessage();
		if (playerLeft.gameid() != 0 && playerLeft.playerid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateGameListAdminChangedMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_gamelistadminchangedmessage()) {
		const GameListAdminChangedMessage &adminChanged = msg.gamelistadminchangedmessage();
		if (adminChanged.gameid() != 0 && adminChanged.newadminplayerid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidatePlayerInfoRequestMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_playerinforequestmessage()) {
		const PlayerInfoRequestMessage &infoRequest = msg.playerinforequestmessage();
		if (VALIDATE_LIST_SIZE(infoRequest.playerid(), 1, 10)) {
			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidatePlayerInfoReplyMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_playerinforeplymessage()) {
		const PlayerInfoReplyMessage &infoReply = msg.playerinforeplymessage();
		if (infoReply.playerid() != 0) {
			// TODO maybe additional checks.
			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateSubscriptionRequestMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_subscriptionrequestmessage()) {
		//const SubscriptionRequestMessage &subscribe = msg.subscriptionrequestmessage();
		retVal = true;
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateCreateGameMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_creategamemessage()) {
		const CreateGameMessage &createGame = msg.creategamemessage();
		if (ValidateGameInfo(createGame.gameinfo())
				&& (!createGame.has_password() || VALIDATE_STRING_SIZE(createGame.password(), 1, 64))) {

			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateJoinGameMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_joingamemessage()) {
		const JoinGameMessage &joinGame = msg.joingamemessage();
		if (joinGame.gameid() != 0
				&& (!joinGame.has_password() || VALIDATE_STRING_SIZE(joinGame.password(), 1, 64))) {

			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateRejoinGameMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_rejoingamemessage()) {
		const RejoinGameMessage &rejoinGame = msg.rejoingamemessage();
		if (rejoinGame.gameid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateJoinGameAckMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_joingameackmessage()) {
		const JoinGameAckMessage &joinAck = msg.joingameackmessage();
		if (joinAck.gameid() != 0 && ValidateGameInfo(joinAck.gameinfo())) {
			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateJoinGameFailedMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_joingamefailedmessage()) {
		const JoinGameFailedMessage &joinFailed = msg.joingamefailedmessage();
		if (joinFailed.gameid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateInviteNotifyMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_invitenotifymessage()) {
		const InviteNotifyMessage &inviteNotify = msg.invitenotifymessage();
		if (inviteNotify.gameid() != 0 && inviteNotify.playeridwho() != 0 && inviteNotify.playeridbywhom() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateRejectGameInvitationMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_rejectgameinvitationmessage()) {
		const RejectGameInvitationMessage &rejectInvite = msg.rejectgameinvitationmessage();
		if (rejectInvite.gameid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateRejectInvNotifyMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_rejectinvnotifymessage()) {
		const RejectInvNotifyMessage &rejectNotify = msg.rejectinvnotifymessage();
		if (rejectNotify.gameid() != 0 && rejectNotify.playerid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateStatisticsMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_statisticsmessage()) {
		//const StatisticsMessage &stat = msg.statisticsmessage();
		retVal = true;
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateChatRequestMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_chatrequestmessage()) {
		const ChatRequestMessage &chatRequest = msg.chatrequestmessage();
		if (VALIDATE_STRING_SIZE(chatRequest.chattext(), 1, 128)) {
			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateChatMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_chatmessage()) {
		const ChatMessage &chat = msg.chatmessage();
		if (VALIDATE_STRING_SIZE(chat.chattext(), 1, 128)) {
			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateChatRejectMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_chatrejectmessage()) {
		const ChatRejectMessage &chatReject = msg.chatrejectmessage();
		if (VALIDATE_STRING_SIZE(chatReject.chattext(), 1, 128)) {
			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateDialogMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_dialogmessage()) {
		const DialogMessage &dialog = msg.dialogmessage();
		if (VALIDATE_STRING_SIZE(dialog.notificationtext(), 1, 128)) {
			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateTimeoutWarningMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_timeoutwarningmessage()) {
		//const TimeoutWarningMessage &timeoutWarning = msg.timeoutwarningmessage();
		retVal = true;
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateResetTimeoutMessage(const LobbyMessage &/*msg*/)
{
	//bool retVal = false;
	//if (msg.has_resettimeoutmessage()) {
	//	const ResetTimeoutMessage &resetTimeout = msg.resettimeoutmessage();
	//	retVal = true;
	//}
	//return retVal;
	return true; // Empty packet, always valid.
}

bool
LobbyMessageValidator::ValidateReportAvatarMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_reportavatarmessage()) {
		const ReportAvatarMessage &reportAvatar = msg.reportavatarmessage();
		if (reportAvatar.reportedplayerid() != 0
				&& reportAvatar.reportedavatarhash().size() == 16) {

			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateReportAvatarAckMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_reportavatarackmessage()) {
		const ReportAvatarAckMessage &reportAvatarAck = msg.reportavatarackmessage();
		if (reportAvatarAck.reportedplayerid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateReportGameMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_reportgamemessage()) {
		const ReportGameMessage &reportGame = msg.reportgamemessage();
		if (reportGame.reportedgameid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateReportGameAckMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_reportgameackmessage()) {
		const ReportGameAckMessage &reportGameAck = msg.reportgameackmessage();
		if (reportGameAck.reportedgameid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateErrorMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_errormessage()) {
		//const ErrorMessage &error = msg.errormessage();
		retVal = true;
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateAdminRemoveGameMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_adminremovegamemessage()) {
		const AdminRemoveGameMessage &removeGame = msg.adminremovegamemessage();
		if (removeGame.removegameid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateAdminRemoveGameAckMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_adminremovegameackmessage()) {
		const AdminRemoveGameAckMessage &removeGameAck = msg.adminremovegameackmessage();
		if (removeGameAck.removegameid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateAdminBanPlayerMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_adminbanplayermessage()) {
		const AdminBanPlayerMessage &banPlayer = msg.adminbanplayermessage();
		if (banPlayer.banplayerid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateAdminBanPlayerAckMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_adminbanplayerackmessage()) {
		const AdminBanPlayerAckMessage &banPlayerAck = msg.adminbanplayerackmessage();
		if (banPlayerAck.banplayerid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateGameInfo(const NetGameInfo &gameInfo)
{
	bool retVal = false;
	if (VALIDATE_STRING_SIZE(gameInfo.gamename(), 1, 64)
			&& VALIDATE_UINT_RANGE(gameInfo.maxnumplayers(), 2, 10)
			&& (!gameInfo.has_raiseeveryhands() || VALIDATE_UINT_RANGE(gameInfo.raiseeveryhands(), 1, 1000))
			&& (!gameInfo.has_raiseeveryminutes() || VALIDATE_UINT_RANGE(gameInfo.raiseeveryminutes(), 1, 1000))
			&& (!gameInfo.has_endraisesmallblindvalue() || VALIDATE_UINT_UPPER(gameInfo.endraisesmallblindvalue(), 1000000))
			&& VALIDATE_UINT_RANGE(gameInfo.proposedguispeed(), 1, 11)
			&& VALIDATE_UINT_RANGE(gameInfo.delaybetweenhands(), 5, 20)
			&& VALIDATE_UINT_UPPER(gameInfo.playeractiontimeout(), 60)
			&& VALIDATE_UINT_RANGE(gameInfo.firstsmallblind(), 1, 20000)
			&& VALIDATE_UINT_RANGE(gameInfo.startmoney(), 1, 1000000)
			&& VALIDATE_LIST_SIZE(gameInfo.manualblinds(), 0, 30)
			&& ValidateListIntRange(gameInfo.manualblinds(), 1, 1000000)) {

		retVal = true;
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateGameListSpectatorJoinedMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_gamelistspectatorjoinedmessage()) {
		const GameListSpectatorJoinedMessage &spectatorJoined = msg.gamelistspectatorjoinedmessage();
		if (spectatorJoined.gameid() != 0 && spectatorJoined.playerid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
LobbyMessageValidator::ValidateGameListSpectatorLeftMessage(const LobbyMessage &msg)
{
	bool retVal = false;
	if (msg.has_gamelistspectatorleftmessage()) {
		const GameListSpectatorLeftMessage &spectatorLeft = msg.gamelistspectatorleftmessage();
		if (spectatorLeft.gameid() != 0 && spectatorLeft.playerid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

