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

#include <net/validation/gamemanagementmessagevalidator.h>
#include <net/validation/validationhelper.h>

using namespace std;

GameManagementMessageValidator::GameManagementMessageValidator()
{
	m_validationMap.insert(make_pair(GameManagementMessage_GameManagementMessageType_Type_GamePlayerJoinedMessage, ValidateGamePlayerJoinedMessage));
	m_validationMap.insert(make_pair(GameManagementMessage_GameManagementMessageType_Type_GamePlayerLeftMessage, ValidateGamePlayerLeftMessage));
	m_validationMap.insert(make_pair(GameManagementMessage_GameManagementMessageType_Type_GameSpectatorJoinedMessage, ValidateGameSpectatorJoinedMessage));
	m_validationMap.insert(make_pair(GameManagementMessage_GameManagementMessageType_Type_GameSpectatorLeftMessage, ValidateGameSpectatorLeftMessage));
	m_validationMap.insert(make_pair(GameManagementMessage_GameManagementMessageType_Type_GameAdminChangedMessage, ValidateGameAdminChangedMessage));
	m_validationMap.insert(make_pair(GameManagementMessage_GameManagementMessageType_Type_RemovedFromGameMessage, ValidateRemovedFromGameMessage));
	m_validationMap.insert(make_pair(GameManagementMessage_GameManagementMessageType_Type_KickPlayerRequestMessage, ValidateKickPlayerRequestMessage));
	m_validationMap.insert(make_pair(GameManagementMessage_GameManagementMessageType_Type_LeaveGameRequestMessage, ValidateLeaveGameRequestMessage));
	m_validationMap.insert(make_pair(GameManagementMessage_GameManagementMessageType_Type_InvitePlayerToGameMessage, ValidateInvitePlayerToGameMessage));
	m_validationMap.insert(make_pair(GameManagementMessage_GameManagementMessageType_Type_StartEventMessage, ValidateStartEventMessage));
	m_validationMap.insert(make_pair(GameManagementMessage_GameManagementMessageType_Type_StartEventAckMessage, ValidateStartEventAckMessage));
	m_validationMap.insert(make_pair(GameManagementMessage_GameManagementMessageType_Type_GameStartInitialMessage, ValidateGameStartInitialMessage));
	m_validationMap.insert(make_pair(GameManagementMessage_GameManagementMessageType_Type_GameStartRejoinMessage, ValidateGameStartRejoinMessage));
	m_validationMap.insert(make_pair(GameManagementMessage_GameManagementMessageType_Type_EndOfGameMessage, ValidateEndOfGameMessage));
	m_validationMap.insert(make_pair(GameManagementMessage_GameManagementMessageType_Type_PlayerIdChangedMessage, ValidatePlayerIdChangedMessage));
	m_validationMap.insert(make_pair(GameManagementMessage_GameManagementMessageType_Type_AskKickPlayerMessage, ValidateAskKickPlayerMessage));
	m_validationMap.insert(make_pair(GameManagementMessage_GameManagementMessageType_Type_AskKickDeniedMessage, ValidateAskKickDeniedMessage));
	m_validationMap.insert(make_pair(GameManagementMessage_GameManagementMessageType_Type_StartKickPetitionMessage, ValidateStartKickPetitionMessage));
	m_validationMap.insert(make_pair(GameManagementMessage_GameManagementMessageType_Type_VoteKickRequestMessage, ValidateVoteKickRequestMessage));
	m_validationMap.insert(make_pair(GameManagementMessage_GameManagementMessageType_Type_VoteKickReplyMessage, ValidateVoteKickReplyMessage));
	m_validationMap.insert(make_pair(GameManagementMessage_GameManagementMessageType_Type_KickPetitionUpdateMessage, ValidateKickPetitionUpdateMessage));
	m_validationMap.insert(make_pair(GameManagementMessage_GameManagementMessageType_Type_EndKickPetitionMessage, ValidateEndKickPetitionMessage));
	m_validationMap.insert(make_pair(GameManagementMessage_GameManagementMessageType_Type_ChatRequestMessage, ValidateChatRequestMessage));
	m_validationMap.insert(make_pair(GameManagementMessage_GameManagementMessageType_Type_ChatMessage, ValidateChatMessage));
	m_validationMap.insert(make_pair(GameManagementMessage_GameManagementMessageType_Type_ChatRejectMessage, ValidateChatRejectMessage));
	m_validationMap.insert(make_pair(GameManagementMessage_GameManagementMessageType_Type_ErrorMessage, ValidateErrorMessage));
}

bool
GameManagementMessageValidator::IsValidMessage(const GameManagementMessage &msg) const
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
GameManagementMessageValidator::ValidateGamePlayerJoinedMessage(const GameManagementMessage &msg)
{
	bool retVal = false;
	if (msg.has_gameplayerjoinedmessage()) {
		const GamePlayerJoinedMessage &playerJoined = msg.gameplayerjoinedmessage();
		if (playerJoined.playerid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
GameManagementMessageValidator::ValidateGamePlayerLeftMessage(const GameManagementMessage &msg)
{
	bool retVal = false;
	if (msg.has_gameplayerleftmessage()) {
		const GamePlayerLeftMessage &playerLeft = msg.gameplayerleftmessage();
		if (playerLeft.playerid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
GameManagementMessageValidator::ValidateGameAdminChangedMessage(const GameManagementMessage &msg)
{
	bool retVal = false;
	if (msg.has_gameadminchangedmessage()) {
		const GameAdminChangedMessage &adminChanged = msg.gameadminchangedmessage();
		if (adminChanged.newadminplayerid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
GameManagementMessageValidator::ValidateRemovedFromGameMessage(const GameManagementMessage &msg)
{
	bool retVal = false;
	if (msg.has_removedfromgamemessage()) {
		retVal = true;
	}
	return retVal;
}

bool
GameManagementMessageValidator::ValidateKickPlayerRequestMessage(const GameManagementMessage &msg)
{
	bool retVal = false;
	if (msg.has_kickplayerrequestmessage()) {
		const KickPlayerRequestMessage &kickRequest = msg.kickplayerrequestmessage();
		if (kickRequest.playerid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
GameManagementMessageValidator::ValidateLeaveGameRequestMessage(const GameManagementMessage &msg)
{
	bool retVal = false;
	if (msg.has_leavegamerequestmessage()) {
		retVal = true;
	}
	return retVal;
}

bool
GameManagementMessageValidator::ValidateInvitePlayerToGameMessage(const GameManagementMessage &msg)
{
	bool retVal = false;
	if (msg.has_inviteplayertogamemessage()) {
		const InvitePlayerToGameMessage &invitePlayer = msg.inviteplayertogamemessage();
		if (invitePlayer.playerid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
GameManagementMessageValidator::ValidateStartEventMessage(const GameManagementMessage &msg)
{
	bool retVal = false;
	if (msg.has_starteventmessage()) {
		retVal = true;
	}
	return retVal;
}

bool
GameManagementMessageValidator::ValidateStartEventAckMessage(const GameManagementMessage &msg)
{
	bool retVal = false;
	if (msg.has_starteventackmessage()) {
		retVal = true;
	}
	return retVal;
}

bool
GameManagementMessageValidator::ValidateGameStartInitialMessage(const GameManagementMessage &msg)
{
	bool retVal = false;
	if (msg.has_gamestartinitialmessage()) {
		const GameStartInitialMessage &startInitial = msg.gamestartinitialmessage();
		if (startInitial.startdealerplayerid() != 0
				&& VALIDATE_LIST_SIZE(startInitial.playerseats(), 2, 10)) {

			retVal = true;
		}
	}
	return retVal;
}

bool
GameManagementMessageValidator::ValidateGameStartRejoinMessage(const GameManagementMessage &msg)
{
	bool retVal = false;
	if (msg.has_gamestartrejoinmessage()) {
		const GameStartRejoinMessage &startRejoin = msg.gamestartrejoinmessage();
		if (startRejoin.startdealerplayerid() != 0
				&& startRejoin.handnum() != 0
				&& VALIDATE_LIST_SIZE(startRejoin.rejoinplayerdata(), 2, 10)) {

			retVal = true;
		}
	}
	return retVal;
}

bool
GameManagementMessageValidator::ValidateEndOfGameMessage(const GameManagementMessage &msg)
{
	bool retVal = false;
	if (msg.has_endofgamemessage()) {
		const EndOfGameMessage &endGame = msg.endofgamemessage();
		if (endGame.winnerplayerid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
GameManagementMessageValidator::ValidatePlayerIdChangedMessage(const GameManagementMessage &msg)
{
	bool retVal = false;
	if (msg.has_playeridchangedmessage()) {
		const PlayerIdChangedMessage &idChanged = msg.playeridchangedmessage();
		if (idChanged.oldplayerid() != 0 && idChanged.newplayerid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
GameManagementMessageValidator::ValidateAskKickPlayerMessage(const GameManagementMessage &msg)
{
	bool retVal = false;
	if (msg.has_askkickplayermessage()) {
		const AskKickPlayerMessage &askKick = msg.askkickplayermessage();
		if (askKick.playerid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
GameManagementMessageValidator::ValidateAskKickDeniedMessage(const GameManagementMessage &msg)
{
	bool retVal = false;
	if (msg.has_askkickdeniedmessage()) {
		const AskKickDeniedMessage &kickDenied = msg.askkickdeniedmessage();
		if (kickDenied.playerid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
GameManagementMessageValidator::ValidateStartKickPetitionMessage(const GameManagementMessage &msg)
{
	bool retVal = false;
	if (msg.has_startkickpetitionmessage()) {
		const StartKickPetitionMessage &kickPetition = msg.startkickpetitionmessage();
		if (kickPetition.petitionid() != 0
				&& kickPetition.proposingplayerid() != 0
				&& kickPetition.kickplayerid() != 0
				&& kickPetition.kicktimeoutsec() > 0
				&& kickPetition.numvotesneededtokick() > 0) {

			retVal = true;
		}
	}
	return retVal;
}

bool
GameManagementMessageValidator::ValidateVoteKickRequestMessage(const GameManagementMessage &msg)
{
	bool retVal = false;
	if (msg.has_votekickrequestmessage()) {
		const VoteKickRequestMessage &voteKick = msg.votekickrequestmessage();
		if (voteKick.petitionid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
GameManagementMessageValidator::ValidateVoteKickReplyMessage(const GameManagementMessage &msg)
{
	bool retVal = false;
	if (msg.has_votekickreplymessage()) {
		const VoteKickReplyMessage &kickReply = msg.votekickreplymessage();
		if (kickReply.petitionid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
GameManagementMessageValidator::ValidateKickPetitionUpdateMessage(const GameManagementMessage &msg)
{
	bool retVal = false;
	if (msg.has_kickpetitionupdatemessage()) {
		const KickPetitionUpdateMessage &kickUpdate = msg.kickpetitionupdatemessage();
		if (kickUpdate.petitionid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
GameManagementMessageValidator::ValidateEndKickPetitionMessage(const GameManagementMessage &msg)
{
	bool retVal = false;
	if (msg.has_endkickpetitionmessage()) {
		const EndKickPetitionMessage &endPetition = msg.endkickpetitionmessage();
		if (endPetition.petitionid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
GameManagementMessageValidator::ValidateChatRequestMessage(const GameManagementMessage &msg)
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
GameManagementMessageValidator::ValidateChatMessage(const GameManagementMessage &msg)
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
GameManagementMessageValidator::ValidateChatRejectMessage(const GameManagementMessage &msg)
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
GameManagementMessageValidator::ValidateErrorMessage(const GameManagementMessage &msg)
{
	bool retVal = false;
	if (msg.has_errormessage()) {
		//const ErrorMessage &msg = msg.errormessage();
		retVal = true;
	}
	return retVal;
}

bool
GameManagementMessageValidator::ValidateGameSpectatorJoinedMessage(const GameManagementMessage &msg)
{
	bool retVal = false;
	if (msg.has_gamespectatorjoinedmessage()) {
		const GameSpectatorJoinedMessage &spectatorJoined = msg.gamespectatorjoinedmessage();
		if (spectatorJoined.playerid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

bool
GameManagementMessageValidator::ValidateGameSpectatorLeftMessage(const GameManagementMessage &msg)
{
	bool retVal = false;
	if (msg.has_gamespectatorleftmessage()) {
		const GameSpectatorLeftMessage &spectatorLeft = msg.gamespectatorleftmessage();
		if (spectatorLeft.playerid() != 0) {
			retVal = true;
		}
	}
	return retVal;
}

