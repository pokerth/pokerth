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
#ifndef GAME_DEFS_H
#define GAME_DEFS_H

#define MIN_NUMBER_OF_PLAYERS		2
#define MAX_NUMBER_OF_PLAYERS		10

#define MIN_GUI_SPEED			1
#define MAX_GUI_SPEED			11

#define DEBUG_MODE			0
#define SQLITE_LOG			1
#define HTML_LOG			0

#define POKERTH_VERSION_MAJOR	1
#define POKERTH_VERSION_MINOR	11
#define POKERTH_VERSION			((POKERTH_VERSION_MAJOR << 8) | POKERTH_VERSION_MINOR)

#define POKERTH_BETA_REVISION	0
#define POKERTH_BETA_RELEASE_STRING	 "1.1.2"

#define SQLITE_LOG_VERSION		1

#define RANKING_GAME_START_CASH 10000
#define RANKING_GAME_NUMBER_OF_PLAYERS 10
#define RANKING_GAME_START_SBLIND 50
#define RANKING_GAME_RAISE_EVERY_HAND 11

enum ServerMode {
	SERVER_MODE_LAN,
	SERVER_MODE_INTERNET_NOAUTH,
	SERVER_MODE_INTERNET_AUTH
};

enum ServerTransportProtocol {
	TRANSPORT_PROTOCOL_TCP = 1,
	TRANSPORT_PROTOCOL_SCTP = 2,
	TRANSPORT_PROTOCOL_TCP_SCTP = 3,
	TRANSPORT_PROTOCOL_WEBSOCKET = 4,
	TRANSPORT_PROTOCOL_TCP_WEBSOCKET = 5,
	TRANSPORT_PROTOCOL_TCP_SCTP_WEBSOCKET = 7
};

enum GameState {
	GAME_STATE_PREFLOP = 0,
	GAME_STATE_FLOP,
	GAME_STATE_TURN,
	GAME_STATE_RIVER,
	GAME_STATE_POST_RIVER,
	GAME_STATE_PREFLOP_SMALL_BLIND = 0xF0,
	GAME_STATE_PREFLOP_BIG_BLIND = 0xF1
};

enum PlayerAction {
	PLAYER_ACTION_NONE = 0,
	PLAYER_ACTION_FOLD,
	PLAYER_ACTION_CHECK,
	PLAYER_ACTION_CALL,
	PLAYER_ACTION_BET,
	PLAYER_ACTION_RAISE,
	PLAYER_ACTION_ALLIN
};

enum PlayerActionCode {
	ACTION_CODE_VALID = 0,
	ACTION_CODE_INVALID_STATE,
	ACTION_CODE_NOT_YOUR_TURN,
	ACTION_CODE_NOT_ALLOWED
};

enum PlayerActionLog {
	LOG_ACTION_NONE = 0,
	LOG_ACTION_DEALER,      // starts as dealer
	LOG_ACTION_SMALL_BLIND, // posts small blind
	LOG_ACTION_BIG_BLIND,   // posts big blind
	LOG_ACTION_FOLD,        // folds
	LOG_ACTION_CHECK,       // checks
	LOG_ACTION_CALL,        // calls
	LOG_ACTION_BET,         // bets
	LOG_ACTION_ALL_IN,      // is all in with
	LOG_ACTION_SHOW,        // shows
	LOG_ACTION_HAS,         // has
	LOG_ACTION_WIN,         // wins
	LOG_ACTION_WIN_SIDE_POT,// wins (side pot)
	LOG_ACTION_SIT_OUT,     // sits out
	LOG_ACTION_WIN_GAME,    // wins game
	LOG_ACTION_LEFT,		// has left the game
	LOG_ACTION_KICKED,		// was kicked from the game
	LOG_ACTION_ADMIN,		// is game admin now
	LOG_ACTION_JOIN			// has joined the game
};

#define LOG_UPLOAD_ID_SIZE		40
#define LOG_UPLOAD_OK_STR		"OK"
#define LOG_UPLOAD_ERROR_STR	"ERROR"

enum LogUploadErrorCode {
	LOG_UPLOAD_ERROR_NO_FILE = 1,
	LOG_UPLOAD_ERROR_OPEN_DB = 2,
	LOG_UPLOAD_ERROR_MAX_NUM_TOTAL = 3,
	LOG_UPLOAD_ERROR_MAX_NUM_IP = 4,
	LOG_UPLOAD_ERROR_FILE_SIZE = 5,
	LOG_UPLOAD_ERROR_FILE_EXT = 6,
	LOG_UPLOAD_ERROR_FILE_HEAD = 7,
	LOG_UPLOAD_ERROR_ID = 8,
	LOG_UPLOAD_ERROR_FILE_MOVE = 9,
	LOG_UPLOAD_ERROR_INSERT_DB = 10
};

enum DenyKickPlayerReason {
	KICK_DENIED_INVALID_STATE = 0,
	KICK_DENIED_TOO_FEW_PLAYERS,
	KICK_DENIED_TEMPORARY,
	KICK_DENIED_OTHER_IN_PROGRESS,
	KICK_DENIED_INVALID_PLAYER_ID
};

enum KickVote {
	KICK_VOTE_AGAINST = 0,
	KICK_VOTE_IN_FAVOUR
};

enum DenyVoteReason {
	VOTE_DENIED_INVALID_PETITION = 0,
	VOTE_DENIED_ALREADY_VOTED
};

enum EndPetitionReason {
	PETITION_END_ENOUGH_VOTES = 0,
	PETITION_END_NOT_ENOUGH_PLAYERS,
	PETITION_END_PLAYER_LEFT,
	PETITION_END_TIMEOUT
};

enum DenyGameInvitationReason {
	DENY_GAME_INVITATION_NO = 0,
	DENY_GAME_INVITATION_BUSY
};

enum Button {
	BUTTON_NONE = 0,
	BUTTON_DEALER,
	BUTTON_SMALL_BLIND,
	BUTTON_BIG_BLIND
};

enum NetTimeoutReason {
	NETWORK_TIMEOUT_GENERIC = 0,
	NETWORK_TIMEOUT_GAME_ADMIN_IDLE,
	NETWORK_TIMEOUT_KICK_AFTER_AUTOFOLD
};

#endif
