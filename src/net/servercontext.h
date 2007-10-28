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
/* Context of network server. */

#ifndef _SERVERCONTEXT_H_
#define _SERVERCONTEXT_H_

#include <net/netcontext.h>


class ServerContext : public NetContext
{
public:
	ServerContext();
	virtual ~ServerContext();

	virtual SOCKET GetSocket() const;

	void SetSocket(SOCKET sockfd);

	int GetProtocol() const
	{return m_protocol;}
	void SetProtocol(int protocol)
	{m_protocol = protocol;}
	int GetAddrFamily() const
	{return m_addrFamily;}
	void SetAddrFamily(int addrFamily)
	{m_addrFamily = addrFamily;}
	unsigned GetServerPort() const
	{return m_serverPort;}
	void SetServerPort(unsigned serverPort)
	{m_serverPort = serverPort;}
	const sockaddr_storage *GetServerSockaddr() const
	{return &m_serverSockaddr;}
	sockaddr_storage *GetServerSockaddr()
	{return &m_serverSockaddr;}

	int GetServerSockaddrSize() const
	{return m_addrFamily == AF_INET6 ? sizeof(sockaddr_in6) : sizeof(sockaddr_in);}

private:
	SOCKET				m_sockfd;
	int					m_protocol;
	int					m_addrFamily;
	unsigned			m_serverPort;
	sockaddr_storage	m_serverSockaddr;
};

#endif
