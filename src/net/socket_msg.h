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
/* Socket message definitions. */
#ifndef _SOCKET_MSG_H_
#define _SOCKET_MSG_H_

// Socket or connection related errors.
#define ERR_SOCK_INTERNAL				1
#define ERR_SOCK_SERVERADDR_NOT_SET		2
#define ERR_SOCK_INVALID_PORT			3
#define ERR_SOCK_CREATION_FAILED		4
#define ERR_SOCK_SET_ADDR_FAILED		5
#define ERR_SOCK_SET_PORT_FAILED		6
#define ERR_SOCK_RESOLVE_FAILED			7
#define ERR_SOCK_BIND_FAILED			8
#define ERR_SOCK_LISTEN_FAILED			9
#define ERR_SOCK_ACCEPT_FAILED			10
#define ERR_SOCK_CONNECT_FAILED			11
#define ERR_SOCK_CONNECT_TIMEOUT		12
#define ERR_SOCK_SELECT_FAILED			13
#define ERR_SOCK_RECV_FAILED			14
#define ERR_SOCK_SEND_FAILED			15
#define ERR_SOCK_CONN_RESET				16
#define ERR_SOCK_CONN_EXISTS			17
#define ERR_SOCK_INVALID_PACKET			18
#define ERR_SOCK_INVALID_STATE			19
#define ERR_SOCK_INVALID_TYPE			20
#define ERR_SOCK_INVALID_SERVERLIST_URL	21
#define ERR_SOCK_INVALID_SERVERLIST_XML 22
#define ERR_SOCK_UNZIP_FAILED			23
#define ERR_SOCK_TRANSFER_INIT_FAILED	24
#define ERR_SOCK_TRANSFER_OPEN_FAILED	25
#define ERR_SOCK_TRANSFER_INVALID_URL	26
#define ERR_SOCK_TRANSFER_SELECT_FAILED	27
#define ERR_SOCK_TRANSFER_FAILED		28
#define ERR_SOCK_CONNECT_IPV6_FAILED	29
#define ERR_SOCK_CONNECT_IPV6_TIMEOUT	30
// The following errors are game errors.
#define ERR_NET_VERSION_NOT_SUPPORTED	101
#define ERR_NET_SERVER_MAINTENANCE		102
#define ERR_NET_SERVER_FULL				103
#define ERR_NET_INVALID_PASSWORD		104
#define ERR_NET_INVALID_PASSWORD_STR	105
#define ERR_NET_PLAYER_NAME_IN_USE		106
#define ERR_NET_INVALID_PLAYER_NAME		107
#define ERR_NET_INVALID_PLAYER_CARDS	108
#define ERR_NET_INVALID_PLAYER_RESULTS	109
#define ERR_NET_INVALID_GAME_NAME		110
#define ERR_NET_INVALID_GAME_ROUND		111
#define ERR_NET_INVALID_SESSION			112
#define ERR_NET_UNKNOWN_GAME			113
#define ERR_NET_INVALID_CHAT_TEXT		114
#define ERR_NET_UNKNOWN_PLAYER_ID		115
#define ERR_NET_NO_CURRENT_PLAYER		116
#define ERR_NET_PLAYER_NOT_ACTIVE		117
#define ERR_NET_PLAYER_KICKED			118
#define ERR_NET_PLAYER_BANNED			119
#define ERR_NET_PLAYER_BLOCKED			120
#define ERR_NET_SESSION_TIMED_OUT		121
#define ERR_NET_INVALID_PLAYER_COUNT	122
#define ERR_NET_TOO_MANY_MANUAL_BLINDS	123
#define ERR_NET_INVALID_AVATAR_FILE		124
#define ERR_NET_AVATAR_TOO_LARGE		125
#define ERR_NET_BUF_INVALID_SIZE		126
#define ERR_NET_INVALID_REQUEST_ID		127
#define ERR_NET_WRONG_AVATAR_SIZE		128
#define ERR_NET_START_TIMEOUT			129
#define ERR_NET_GAME_TERMINATION_FAILED	130
#define ERR_NET_INTERNAL_GAME_ERROR		131
#define ERR_NET_DEALER_NOT_FOUND		132
#define ERR_NET_INIT_BLOCKED			133
#define ERR_NET_GSASL_INIT_FAILED		134
#define ERR_NET_GSASL_NO_SCRAM			135
#define ERR_NET_DB_CONNECT_FAILED		136

#define ERR_IRC_INTERNAL				151
#define ERR_IRC_CONNECT_FAILED			152
#define ERR_IRC_TERMINATED				153
#define ERR_IRC_SELECT_FAILED			154
#define ERR_IRC_RECV_FAILED				155
#define ERR_IRC_SEND_FAILED				156
#define ERR_IRC_INVALID_PARAM			157
#define ERR_IRC_TIMEOUT					158

// Notifications - removed from game
#define NTF_NET_INTERNAL				201
#define NTF_NET_REMOVED_ON_REQUEST		202
#define NTF_NET_REMOVED_GAME_FULL		203
#define NTF_NET_REMOVED_ALREADY_RUNNING	204
#define NTF_NET_REMOVED_KICKED			205
#define NTF_NET_REMOVED_TIMEOUT			206
#define NTF_NET_REMOVED_START_FAILED	207
#define NTF_NET_REMOVED_GAME_CLOSED		208

// Notifications - join failed
#define NTF_NET_JOIN_GAME_INVALID		210
#define NTF_NET_JOIN_GAME_FULL			211
#define NTF_NET_JOIN_ALREADY_RUNNING	212
#define NTF_NET_JOIN_INVALID_PASSWORD	213
#define NTF_NET_JOIN_GUEST_FORBIDDEN	214
#define NTF_NET_JOIN_NOT_INVITED		215
#define NTF_NET_JOIN_GAME_NAME_IN_USE	216
#define NTF_NET_JOIN_GAME_BAD_NAME		217
#define NTF_NET_JOIN_INVALID_SETTINGS	218
#define NTF_NET_JOIN_IP_BLOCKED			219
#define NTF_NET_JOIN_REJOIN_FAILED		220
#define NTF_NET_JOIN_NO_SPECTATORS		221

// Notifications - version
#define NTF_NET_NEW_RELEASE_AVAILABLE	231
#define NTF_NET_OUTDATED_BETA			232

// This is an internal message which is not reported.
#define MSG_SOCK_INTERNAL_PENDING		0

// The following messages are connect messages.
#define MSG_SOCK_INIT_DONE				1
#define MSG_SOCK_SERVER_LIST_DONE		2
#define MSG_SOCK_RESOLVE_DONE			3
#define MSG_SOCK_CONNECT_DONE			4
#define MSG_SOCK_SESSION_DONE			5

#define MSG_SOCK_LIMIT_CONNECT			MSG_SOCK_SESSION_DONE
#define MSG_SOCK_LAST					MSG_SOCK_SESSION_DONE

// The following messages are game messages.
#define MSG_NET_GAME_CLIENT_JOIN		MSG_SOCK_LIMIT_CONNECT + 1
#define MSG_NET_GAME_CLIENT_SYNCSTART	MSG_SOCK_LIMIT_CONNECT + 2
#define MSG_NET_GAME_CLIENT_SYNCREJOIN	MSG_SOCK_LIMIT_CONNECT + 3
#define MSG_NET_GAME_CLIENT_START		MSG_SOCK_LIMIT_CONNECT + 4
#define MSG_NET_GAME_CLIENT_HAND_START	MSG_SOCK_LIMIT_CONNECT + 5
#define MSG_NET_GAME_CLIENT_HAND_END	MSG_SOCK_LIMIT_CONNECT + 6
#define MSG_NET_GAME_CLIENT_END			MSG_SOCK_LIMIT_CONNECT + 7

#define MSG_NET_GAME_SERVER_START		MSG_SOCK_LIMIT_CONNECT + 8
#define MSG_NET_GAME_SERVER_HAND_START	MSG_SOCK_LIMIT_CONNECT + 9
#define MSG_NET_GAME_SERVER_HAND_END	MSG_SOCK_LIMIT_CONNECT + 10
#define MSG_NET_GAME_SERVER_ROUND		MSG_SOCK_LIMIT_CONNECT + 11
#define MSG_NET_GAME_SERVER_ACTION		MSG_SOCK_LIMIT_CONNECT + 12
#define MSG_NET_GAME_SERVER_CARDS_DELAY	MSG_SOCK_LIMIT_CONNECT + 13
#define MSG_NET_GAME_SERVER_END			MSG_SOCK_LIMIT_CONNECT + 14

#define MSG_NET_LIMIT_GAME				MSG_NET_GAME_SERVER_END

// Some messages which are displayed in the GUI
#define MSG_NET_AVATAR_REPORT_ACCEPTED		MSG_NET_LIMIT_GAME + 1
#define MSG_NET_AVATAR_REPORT_DUP			MSG_NET_LIMIT_GAME + 2
#define MSG_NET_AVATAR_REPORT_REJECTED		MSG_NET_LIMIT_GAME + 3
#define MSG_NET_GAMENAME_REPORT_ACCEPTED	MSG_NET_LIMIT_GAME + 4
#define MSG_NET_GAMENAME_REPORT_DUP			MSG_NET_LIMIT_GAME + 5
#define MSG_NET_GAMENAME_REPORT_REJECTED	MSG_NET_LIMIT_GAME + 6
#define MSG_NET_ADMIN_REMOVE_GAME_ACCEPTED	MSG_NET_LIMIT_GAME + 7
#define MSG_NET_ADMIN_REMOVE_GAME_REJECTED	MSG_NET_LIMIT_GAME + 8
#define MSG_NET_ADMIN_BAN_PLAYER_ACCEPTED	MSG_NET_LIMIT_GAME + 9
#define MSG_NET_ADMIN_BAN_PLAYER_PENDING	MSG_NET_LIMIT_GAME + 10
#define MSG_NET_ADMIN_BAN_PLAYER_NODB		MSG_NET_LIMIT_GAME + 11
#define MSG_NET_ADMIN_BAN_PLAYER_DBERROR	MSG_NET_LIMIT_GAME + 12
#define MSG_NET_ADMIN_BAN_PLAYER_REJECTED	MSG_NET_LIMIT_GAME + 13

#endif

