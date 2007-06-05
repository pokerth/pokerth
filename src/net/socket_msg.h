/***************************************************************************
 *   Copyright (C) 2007 by Lothar May                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
/* Socket message definitions. */
#ifndef _SOCKET_MSG_H_
#define _SOCKET_MSG_H_

// Socket or socket related errors.
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
// The following errors are game errors.
#define ERR_NET_VERSION_NOT_SUPPORTED	101
#define ERR_NET_SERVER_FULL				102
#define ERR_NET_GAME_ALREADY_RUNNING	103
#define ERR_NET_INVALID_PASSWORD		104
#define ERR_NET_INVALID_PASSWORD_STR	105
#define ERR_NET_PLAYER_NAME_IN_USE		106
#define ERR_NET_INVALID_PLAYER_NAME		107
#define ERR_NET_INVALID_PLAYER_RESULTS	108
#define ERR_NET_INVALID_CHAT_TEXT		109
#define ERR_NET_UNKNOWN_PLAYER_ID		110

// This is an internal message which is not reported.
#define MSG_SOCK_INTERNAL_PENDING		0

// The following messages are connect messages.
#define MSG_SOCK_INIT_DONE				1
#define MSG_SOCK_RESOLVE_DONE			2
#define MSG_SOCK_CONNECT_DONE			3
#define MSG_SOCK_SESSION_DONE			4

#define MSG_SOCK_LIMIT_CONNECT			MSG_SOCK_SESSION_DONE
#define MSG_SOCK_LAST					MSG_SOCK_SESSION_DONE

// The following messages are game messages.
#define MSG_NET_GAME_CLIENT_START		5
#define MSG_NET_GAME_SERVER_START		6
#define MSG_NET_GAME_CLIENT_HAND_START	7
#define MSG_NET_GAME_CLIENT_HAND_END	8
#define MSG_NET_GAME_SERVER_HAND_START	9
#define MSG_NET_GAME_SERVER_HAND_END	10
#define MSG_NET_GAME_SERVER_ROUND		11
#define MSG_NET_GAME_SERVER_ACTION		12
#define MSG_NET_GAME_SERVER_CARDS_DELAY	13

#endif

