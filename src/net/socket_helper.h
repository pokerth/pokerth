/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2011 Felix Hammer, Florian Thauer, Lothar May          *
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
 *****************************************************************************/
/* Socket helper defines. */
#ifndef _SOCKET_HELPER_H_
#define _SOCKET_HELPER_H_

#ifndef bzero
#include <cstring>
#define	bzero(_ptr, _n)		std::memset(_ptr, 0, _n)
#endif

#ifdef _WIN32

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

#include <sys/types.h>

#endif // _WIN32

#define MAX_ADDR_STRING_LEN 256

#endif

