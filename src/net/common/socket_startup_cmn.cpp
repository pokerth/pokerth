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

#include <net/socket_startup.h>
#include <net/socket_helper.h>


bool
socket_has_sctp()
{
#ifdef IPPROTO_SCTP
	SOCKET test = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);

	if (test == INVALID_SOCKET)
		return false;
	CLOSESOCKET(test);
	return true;
#else
	return false;
#endif
}

bool
socket_has_ipv6()
{
	SOCKET test = socket(AF_INET6, SOCK_STREAM, 0);

	if (test == INVALID_SOCKET)
		return false;
	CLOSESOCKET(test);
	return true;
}

bool
socket_has_dual_stack()
{
	bool retVal = false;
	SOCKET test = socket(AF_INET6, SOCK_STREAM, 0);

	if (test != INVALID_SOCKET)
	{
		int ipv6only = 0;
		if (setsockopt(test, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&ipv6only, sizeof(ipv6only)) == 0)
			retVal = true;
		CLOSESOCKET(test);
	}
	return retVal;
}

