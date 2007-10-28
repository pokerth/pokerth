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

#include <net/clientcontext.h>

ClientContext::ClientContext()
: m_protocol(0), m_addrFamily(AF_INET), m_serverPort(0)
{
	bzero(&m_clientSockaddr, sizeof(m_clientSockaddr));
}

ClientContext::~ClientContext()
{
}

SOCKET
ClientContext::GetSocket() const
{
	assert(m_sessionData.get());
	return m_sessionData->GetSocket();
}

void
ClientContext::SetSocket(SOCKET sockfd)
{
	m_sessionData.reset(new SessionData(sockfd, SESSION_ID_GENERIC));
}

boost::shared_ptr<SessionData>
ClientContext::GetSessionData() const
{
	return m_sessionData;
}

