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

#include <net/sessiondata.h>

SessionData::SessionData(SOCKET sockfd, unsigned id)
: m_sockfd(sockfd), m_id(id), m_state(SessionData::Init)
{
}

SessionData::~SessionData()
{
	if (m_sockfd != INVALID_SOCKET)
		CLOSESOCKET(m_sockfd);
}

unsigned
SessionData::GetId() const
{
	// const value - no mutex needed.
	return m_id;
}

SessionData::State
SessionData::GetState() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_state;
}

void
SessionData::SetState(SessionData::State state)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_state = state;
}

SOCKET
SessionData::GetSocket() const
{
	// value never modified - no mutex needed.
	return m_sockfd;
}

const std::string &
SessionData::GetClientAddr() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_clientAddr;
}

void
SessionData::SetClientAddr(const std::string &addr)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_clientAddr = addr;
}

ReceiveBuffer &
SessionData::GetReceiveBuffer()
{
	// mutex protection, if needed, within buffer.
	return m_receiveBuffer;
}

