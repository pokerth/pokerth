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

#define ERR_SOCK_INTERNAL				1
#define ERR_SOCK_SERVERADDR_NOT_SET		2
#define ERR_SOCK_INVALID_PORT			3
#define ERR_SOCK_CREATION_FAILED		4
#define ERR_SOCK_SET_PORT_FAILED		5
#define ERR_SOCK_RESOLVE_FAILED			6
#define ERR_SOCK_CONNECT_FAILED			7
#define ERR_SOCK_SELECT_FAILED			8
#define ERR_SOCK_RECV_FAILED			9
#define ERR_SOCK_SEND_FAILED			10
#define ERR_SOCK_CONN_RESET				11

// This is an internal message which is not reported.
#define MSG_SOCK_INTERNAL_PENDING		0

// The following messages are reported.
#define MSG_SOCK_INIT_DONE				1
#define MSG_SOCK_RESOLVE_DONE			2
#define MSG_SOCK_CONNECT_DONE			3
#define MSG_SOCK_SESSION_DONE			4

#define MSG_SOCK_LAST					MSG_SOCK_SESSION_DONE

#endif

