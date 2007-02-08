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
#ifndef _WIN32
#error This source code is Win32 only.
#endif

#include "socket_helper.h"


bool
socket_string_to_addr(const char *str, int addrFamily, struct sockaddr * addr, int addrLen)
{
	// TODO: convert str from UTF-8 to UTF-16 (Win32 byte order)
	//return (WSAStringToAddress((wchar_t *)str, addrFamily, NULL, addr, &addrLen) != SOCKET_ERROR);
}

bool
socket_resolve(const char *str, int addrFamily, int sockType, int protocol, struct sockaddr *addr, int addrLen)
{
}

