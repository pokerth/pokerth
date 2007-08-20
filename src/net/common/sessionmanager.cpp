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

#include <net/sessionmanager.h>
#include <net/senderthread.h>
#include <net/serverexception.h>
#include <net/socket_msg.h>

using namespace std;


SessionManager::SessionManager()
{
}

SessionManager::~SessionManager()
{
	Clear();
}

void
SessionManager::AddSession(boost::shared_ptr<SessionData> sessionData)
{
	AddSession(SessionWrapper(sessionData, boost::shared_ptr<PlayerData>()));
}

void
SessionManager::AddSession(SessionWrapper session)
{
	boost::mutex::scoped_lock lock(m_sessionMapMutex);

	SessionMap::iterator pos = m_sessionMap.lower_bound(session.sessionData->GetSocket());

	// If pos points to a pair whose key is equivalent to the socket, this handle
	// already exists within the list.
	if (pos != m_sessionMap.end() && session.sessionData->GetSocket() == pos->first)
	{
		throw ServerException(ERR_SOCK_CONN_EXISTS, 0);
	}
	m_sessionMap.insert(pos, SessionMap::value_type(session.sessionData->GetSocket(), session));
}

void
SessionManager::SetSessionPlayerData(SOCKET session, boost::shared_ptr<PlayerData> playerData)
{
	boost::mutex::scoped_lock lock(m_sessionMapMutex);
	SessionMap::iterator pos = m_sessionMap.find(session);

	if (pos != m_sessionMap.end())
		pos->second.playerData = playerData;
}

void
SessionManager::RemoveSession(SOCKET session)
{
	boost::mutex::scoped_lock lock(m_sessionMapMutex);
	m_sessionMap.erase(session);
}

SessionWrapper
SessionManager::Select(unsigned timeoutMsec)
{
	SessionWrapper retSession;
	SOCKET maxSock = INVALID_SOCKET;
	fd_set rdset;
	FD_ZERO(&rdset);

	{
		boost::mutex::scoped_lock lock(m_sessionMapMutex);
		SessionMap::iterator i = m_sessionMap.begin();
		SessionMap::iterator end = m_sessionMap.end();

		while (i != end)
		{
			SOCKET tmpSock = i->first;
			FD_SET(tmpSock, &rdset);
			if (tmpSock > maxSock || maxSock == INVALID_SOCKET)
				maxSock = tmpSock;
			++i;
		}
	}

	if (maxSock == INVALID_SOCKET)
	{
		Thread::Msleep(timeoutMsec); // just sleep if there is no session
	}
	else
	{
		// wait for data
		struct timeval timeout;
		timeout.tv_sec = timeoutMsec / 1000;
		timeout.tv_usec = (timeoutMsec % 1000) * 1000;
		int selectResult = select(maxSock + 1, &rdset, NULL, NULL, &timeout);
		if (!IS_VALID_SELECT(selectResult))
		{
			throw ServerException(ERR_SOCK_SELECT_FAILED, SOCKET_ERRNO());
		}
		if (selectResult > 0) // one (or more) of the sockets is readable
		{
			// Check which socket is readable, return the first.
			boost::mutex::scoped_lock lock(m_sessionMapMutex);
			SessionMap::iterator i = m_sessionMap.begin();
			SessionMap::iterator end = m_sessionMap.end();

			while (i != end)
			{
				if (FD_ISSET(i->first, &rdset))
				{
					retSession = i->second;
					break;
				}
				++i;
			}
		}
	}
	return retSession;
}

SessionWrapper
SessionManager::GetSessionByPlayerName(const string playerName) const
{
	SessionWrapper tmpSession;
	boost::mutex::scoped_lock lock(m_sessionMapMutex);

	SessionMap::const_iterator session_i = m_sessionMap.begin();
	SessionMap::const_iterator session_end = m_sessionMap.end();

	while (session_i != session_end)
	{
		// Check all players which are fully connected.
		if (session_i->second.sessionData->GetState() == SessionData::Established)
		{
			boost::shared_ptr<PlayerData> tmpPlayer(session_i->second.playerData);
			assert(tmpPlayer.get());
			if (tmpPlayer->GetName() == playerName)
			{
				tmpSession = session_i->second;
				break;
			}
		}

		++session_i;
	}
	return tmpSession;
}

SessionWrapper
SessionManager::GetSessionByUniquePlayerId(unsigned uniqueId) const
{
	SessionWrapper tmpSession;
	boost::mutex::scoped_lock lock(m_sessionMapMutex);

	SessionMap::const_iterator session_i = m_sessionMap.begin();
	SessionMap::const_iterator session_end = m_sessionMap.end();

	while (session_i != session_end)
	{
		// Check all players which are fully connected.
		if (session_i->second.sessionData->GetState() == SessionData::Established)
		{
			boost::shared_ptr<PlayerData> tmpPlayer(session_i->second.playerData);
			assert(tmpPlayer.get());
			if (tmpPlayer->GetUniqueId() == uniqueId)
			{
				tmpSession = session_i->second;
				break;
			}
		}

		++session_i;
	}
	return tmpSession;
}

PlayerDataList
SessionManager::GetPlayerDataList() const
{
	PlayerDataList playerList;
	boost::mutex::scoped_lock lock(m_sessionMapMutex);

	SessionMap::const_iterator session_i = m_sessionMap.begin();
	SessionMap::const_iterator session_end = m_sessionMap.end();

	while (session_i != session_end)
	{
		// Get all players which are fully connected.
		if (session_i->second.sessionData->GetState() == SessionData::Established)
		{
			boost::shared_ptr<PlayerData> tmpPlayer(session_i->second.playerData);
			assert(tmpPlayer.get());
			assert(!tmpPlayer->GetName().empty());
			playerList.push_back(tmpPlayer);
		}
		++session_i;
	}
	return playerList;
}

bool
SessionManager::IsPlayerConnected(const string &playerName) const
{
	bool retVal = false;

	SessionWrapper tmpSession = GetSessionByPlayerName(playerName);

	if (tmpSession.sessionData.get() && tmpSession.playerData.get())
		retVal = true;

	return retVal;
}

bool
SessionManager::IsPlayerConnected(unsigned uniqueId) const
{
	bool retVal = false;

	SessionWrapper tmpSession = GetSessionByUniquePlayerId(uniqueId);

	if (tmpSession.sessionData.get() && tmpSession.playerData.get())
		retVal = true;

	return retVal;
}

void
SessionManager::Clear()
{
	boost::mutex::scoped_lock lock(m_sessionMapMutex);

	// Sockets will be closed automatically.
	m_sessionMap.clear();
}

unsigned
SessionManager::GetRawSessionCount()
{
	boost::mutex::scoped_lock lock(m_sessionMapMutex);
	return m_sessionMap.size();
}

void
SessionManager::SendToAllSessions(SenderThread &sender, boost::shared_ptr<NetPacket> packet)
{
	boost::mutex::scoped_lock lock(m_sessionMapMutex);

	SessionMap::iterator i = m_sessionMap.begin();
	SessionMap::iterator end = m_sessionMap.end();

	while (i != end)
	{
		assert(i->second.sessionData.get());

		// Send each fully connected client a copy of the packet.
		if (i->second.sessionData->GetState() == SessionData::Established)
			sender.Send(i->first, boost::shared_ptr<NetPacket>(packet->Clone()));
		++i;
	}
}

void
SessionManager::SendToAllButOneSessions(SenderThread &sender, boost::shared_ptr<NetPacket> packet, SOCKET except)
{
	boost::mutex::scoped_lock lock(m_sessionMapMutex);

	SessionMap::iterator i = m_sessionMap.begin();
	SessionMap::iterator end = m_sessionMap.end();

	while (i != end)
	{
		// Send each fully connected client but one a copy of the packet.
		if (i->second.sessionData->GetState() == SessionData::Established)
			if (i->first != except)
				sender.Send(i->first, boost::shared_ptr<NetPacket>(packet->Clone()));
		++i;
	}
}

