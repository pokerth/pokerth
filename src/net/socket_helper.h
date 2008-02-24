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
#define CLOSESOCKET						closesocket
#define IOCTLSOCKET						ioctlsocket
#define SOCKET_ERRNO()					WSAGetLastError()
#define IS_SOCKET_ERR_WOULDBLOCK(_e)	((_e) == WSAEWOULDBLOCK)
#define SOCKET_ERR_NOTCONN				WSAENOTCONN
#define SOCKET_ERR_NOTSOCK				WSAENOTSOCK

#ifndef IPV6_V6ONLY
	#define IPV6_V6ONLY					27
#endif

#ifdef __GNUC__ /* mingw provides stdint.h */
	#include <stdint.h>
	typedef uint16_t					u_int16_t;
	typedef uint32_t					u_int32_t;
#else
	typedef unsigned __int16			u_int16_t;
	typedef unsigned __int32			u_int32_t;
	typedef __int16						int16_t;
	typedef __int32						int32_t;
#endif

typedef unsigned char					u_char;

#else
#define SOCKET							int
#define SOCKET_ERROR					-1
#define INVALID_SOCKET					-1
#define CLOSESOCKET						close
#define SOCKET_ERRNO()					errno
#define IOCTLSOCKET						ioctl
#define SOCKET_ERR_NOTCONN				ENOTCONN
#define SOCKET_ERR_NOTSOCK				ENOTSOCK

#define IS_SOCKET_ERR_WOULDBLOCK(_e)	((_e) == EINPROGRESS || (_e) == EAGAIN || (_e) == EWOULDBLOCK)

#endif

#define IS_VALID_SOCKET(_s)		((_s) != INVALID_SOCKET)
#define IS_VALID_CONNECT(_c)	((_c) == 0)
#define IS_VALID_BIND(_b)		((_b) != SOCKET_ERROR)
#define IS_VALID_LISTEN(_l)		((_l) != SOCKET_ERROR)
#define IS_VALID_RECV(_r)		((_r) != SOCKET_ERROR)
#define IS_VALID_SEND(_s)		((_s) != SOCKET_ERROR)
#define IS_VALID_SELECT(_s)		((_s) != SOCKET_ERROR)

#define RECV_TIMEOUT_MSEC	50

#ifdef MSG_NOSIGNAL
	#define SOCKET_SEND_FLAGS			MSG_NOSIGNAL
#else
	#define SOCKET_SEND_FLAGS			0
#endif

#ifdef IPPROTO_SCTP
	#define SOCKET_IPPROTO_SCTP			IPPROTO_SCTP
#else
	#define SOCKET_IPPROTO_SCTP			0
#endif

#define MAX_ADDR_STRING_LEN 256

// All char *s are assumed to be UTF-8.

/**
 * Convert an address string to a numeric address.
 * str is assumed to be UTF-8 encoded.
 */
bool socket_string_to_addr(const char *str, int addrFamily, struct sockaddr *addr, int addrLen);

/**
 * Convert a numeric address to an address string.
 */
bool socket_addr_to_string(struct sockaddr *addr, int addrLen, int addrFamily, char *str, int strLen);

/**
 * Resolve a name to a numeric address.
 * str is assumed to be UTF-8 encoded.
 */
bool socket_resolve(const char *str, const char *port, int addrFamily, int sockType, int protocol, struct sockaddr *addr, int addrLen);

/**
 * Set the port in the sockaddr structure.
 */
bool socket_set_port(unsigned port, int addrFamily, struct sockaddr *addr, int addrLen);

#endif

