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

#define ERR_SOCK_SERVERADDR_NOT_SET		1
#define ERR_SOCK_INVALID_PORT			2
#define ERR_SOCK_CREATION_FAILED		10
#define ERR_SOCK_SET_PORT_FAILED		11
#define ERR_SOCK_RESOLVE_FAILED			12
#define ERR_SOCK_CONNECT_FAILED			13

#define MSG_SOCK_INIT_DONE				1
#define MSG_SOCK_RESOLVE_DONE			2
#define MSG_SOCK_CONNECT_DONE			3

#endif

