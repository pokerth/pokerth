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

#include <net/socket_helper.h>
#include <cstring>

using namespace std;

bool
socket_set_port(unsigned port, int addrFamily, struct sockaddr *addr, int addrLen)
{
	bool retVal = false;

	if (addr)
	{
		if (addrFamily == AF_INET && addrLen >= (int)sizeof(sockaddr_in))
		{
			((sockaddr_in *)addr)->sin_port = htons(port);
			retVal = true;
		}
		else if (addrFamily == AF_INET6 && addrLen >= (int)sizeof(sockaddr_in6))
		{
			((sockaddr_in6 *)addr)->sin6_port = htons(port);
			retVal = true;
		}
	}

	return retVal;
}

