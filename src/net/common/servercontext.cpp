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

#include <net/servercontext.h>

ServerContext::ServerContext()
: m_sockfd(INVALID_SOCKET), m_protocol(0), m_addrFamily(AF_INET), m_serverPort(0)
{
	bzero(&m_serverSockaddr, sizeof(m_serverSockaddr));
}

ServerContext::~ServerContext()
{
	if (m_sockfd != INVALID_SOCKET)
		CLOSESOCKET(m_sockfd);
}

SOCKET
ServerContext::GetSocket() const
{
	return m_sockfd;
}

u_int32_t
ServerContext::GetId() const
{
	// Id is unused for main server thread.
	return 0;
}

void
ServerContext::SetSocket(SOCKET sockfd)
{
	m_sockfd = sockfd;
}

