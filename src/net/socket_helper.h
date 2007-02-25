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
/* Socket helper defines and functions. */
#ifndef _SOCKET_HELPER_H_
#define _SOCKET_HELPER_H_

#include <net/genericsocket.h>

#ifndef bzero
	#include <cstring>
	#define	bzero(_ptr, _n)		std::memset(_ptr, 0, _n)
#endif

#ifdef _WIN32
#define CLOSESOCKET				closesocket
#define IOCTLSOCKET				ioctlsocket
#define SOCKET_ERRNO()			WSAGetLastError()
#define SOCKET_ERR_WOULDBLOCK	WSAEWOULDBLOCK
#else
#define SOCKET					int
#define SOCKET_ERROR			-1
#define INVALID_SOCKET			-1
#define CLOSESOCKET				close
#define SOCKET_ERRNO()			errno
#define IOCTLSOCKET				ioctl
#define SOCKET_ERR_WOULDBLOCK	EWOULDBLOCK
#endif

#define IS_VALID_SOCKET(_s)		((_s) != INVALID_SOCKET)
#define IS_VALID_CONNECT(_c)	((_c) == 0)
#define IS_VALID_RECV(_r)		((_r) != SOCKET_ERROR)
#define IS_VALID_SEND(_s)		((_s) != SOCKET_ERROR)
#define IS_VALID_BIND(_b)		((_b) != SOCKET_ERROR)

// All char *s are assumed to be UTF-8.

/**
 * Convert an address string to a numeric address.
 * str is assumed to be UTF-8 encoded.
 */
bool socket_string_to_addr(const char *str, int addrFamily, struct sockaddr *addr, int addrLen);

/**
 * Resolve a name to a numeric address.
 * str is assumed to be UTF-8 encoded.
 */
bool socket_resolve(const char *str, const char *port, int addrFamily, int sockType, int protocol, struct sockaddr *addr, int addrLen);

/**
 * Set the port in the sockaddr structure.
 */
bool socket_set_port(unsigned port, int addrFamily, struct sockaddr *addr, int addrLen);

/**
 * Internal function (common for all OSs).
 */
bool internal_socket_resolve(const char *str, const char *port, int addrFamily, int sockType, int protocol, struct sockaddr *addr, int addrLen);

#endif

