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

#ifdef _WIN32

#include <stdint.h>
typedef uint16_t					u_int16_t;
typedef uint32_t					u_int32_t;
#ifndef __GNUC__ /* ssize_t not defined in MSVC */
typedef _W64 int					ssize_t;
#define _SSIZE_T_
#endif

typedef unsigned char					u_char;

#else

#include <sys/types.h>

#endif // _WIN32

#define MAX_ADDR_STRING_LEN 256

#endif

