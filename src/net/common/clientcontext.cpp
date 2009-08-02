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

#include <boost/asio.hpp>
#include <net/clientcontext.h>


ClientContext::ClientContext()
: m_sctp(false), m_addrFamily(AF_INET), m_useServerList(false), m_serverPort(0),
  m_hasSubscribedLobbyMsg(true)
{
}

ClientContext::~ClientContext()
{
	m_sessionData.reset();
}

boost::shared_ptr<SessionData>
ClientContext::GetSessionData() const
{
	return m_sessionData;
}

void
ClientContext::SetSessionData(boost::shared_ptr<SessionData> sessionData)
{
	m_sessionData = sessionData;
}

boost::shared_ptr<boost::asio::ip::tcp::resolver>
ClientContext::GetResolver() const
{
	return m_resolver;
}

void
ClientContext::SetResolver(boost::shared_ptr<boost::asio::ip::tcp::resolver> resolver)
{
	m_resolver = resolver;
}

