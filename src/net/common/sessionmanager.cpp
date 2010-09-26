/***************************************************************************
 *   Copyright (C) 2007-2009 by Lothar May                                 *
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
#include <net/senderhelper.h>
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

bool
SessionManager::HasSessions() const
{
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);
	return !m_sessionMap.empty();
}

void
SessionManager::AddSession(boost::shared_ptr<SessionData> sessionData)
{
	AddSession(SessionWrapper(sessionData, boost::shared_ptr<PlayerData>()));
}

void
SessionManager::AddSession(SessionWrapper session)
{
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);

	SessionMap::iterator pos = m_sessionMap.lower_bound(session.sessionData->GetId());

	// If pos points to a pair whose key is equivalent to the socket, this handle
	// already exists within the list.
	if (pos != m_sessionMap.end() && session.sessionData->GetId() == pos->first)
	{
		throw ServerException(__FILE__, __LINE__, ERR_SOCK_CONN_EXISTS, 0);
	}
	m_sessionMap.insert(pos, SessionMap::value_type(session.sessionData->GetId(), session));
}

void
SessionManager::SetSessionPlayerData(SessionId session, boost::shared_ptr<PlayerData> playerData)
{
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);
	SessionMap::iterator pos = m_sessionMap.find(session);

	if (pos != m_sessionMap.end())
		pos->second.playerData = playerData;
}

bool
SessionManager::RemoveSession(SessionId session)
{
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);
	return m_sessionMap.erase(session) == 1;
}

SessionWrapper
SessionManager::GetSessionById(SessionId id) const
{
	SessionWrapper tmpSession;
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);
	SessionMap::const_iterator pos = m_sessionMap.find(id);
	if (pos != m_sessionMap.end())
		tmpSession = pos->second;
	return tmpSession;
}

SessionWrapper
SessionManager::GetSessionByPlayerName(const string playerName) const
{
	SessionWrapper tmpSession;
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);

	SessionMap::const_iterator session_i = m_sessionMap.begin();
	SessionMap::const_iterator session_end = m_sessionMap.end();

	while (session_i != session_end)
	{
		// Check all players which are fully connected.
		if (session_i->second.sessionData->GetState() != SessionData::Init)
		{
			boost::shared_ptr<PlayerData> tmpPlayer(session_i->second.playerData);
			if (!tmpPlayer.get())
				throw ServerException(__FILE__, __LINE__, ERR_NET_INVALID_SESSION, 0);
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
SessionManager::GetSessionByUniquePlayerId(unsigned uniqueId, bool initSessions) const
{
	SessionWrapper tmpSession;
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);

	SessionMap::const_iterator session_i = m_sessionMap.begin();
	SessionMap::const_iterator session_end = m_sessionMap.end();

	while (session_i != session_end)
	{
		// Check all players which are fully connected.
		if (initSessions || session_i->second.sessionData->GetState() != SessionData::Init)
		{
			boost::shared_ptr<PlayerData> tmpPlayer(session_i->second.playerData);
			if (tmpPlayer && tmpPlayer->GetUniqueId() == uniqueId)
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
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);

	SessionMap::const_iterator session_i = m_sessionMap.begin();
	SessionMap::const_iterator session_end = m_sessionMap.end();

	while (session_i != session_end)
	{
		// Get all players in the game.
		if (session_i->second.sessionData->GetState() == SessionData::Game)
		{
			boost::shared_ptr<PlayerData> tmpPlayer(session_i->second.playerData);
			if (!tmpPlayer.get() || tmpPlayer->GetName().empty())
				throw ServerException(__FILE__, __LINE__, ERR_NET_INVALID_SESSION, 0);
			playerList.push_back(tmpPlayer);
		}
		++session_i;
	}
	return playerList;
}

PlayerIdList
SessionManager::GetPlayerIdList(SessionData::State state) const
{
	PlayerIdList playerList;
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);

	SessionMap::const_iterator session_i = m_sessionMap.begin();
	SessionMap::const_iterator session_end = m_sessionMap.end();

	while (session_i != session_end)
	{
		// Get all players in the game.
		if (session_i->second.sessionData->GetState() == state)
		{
			playerList.push_back(session_i->second.playerData->GetUniqueId());
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

	if (tmpSession.sessionData && tmpSession.playerData)
		retVal = true;

	return retVal;
}

bool
SessionManager::IsPlayerConnected(unsigned uniqueId) const
{
	bool retVal = false;

	SessionWrapper tmpSession = GetSessionByUniquePlayerId(uniqueId);

	if (tmpSession.sessionData && tmpSession.playerData)
		retVal = true;

	return retVal;
}

void
SessionManager::ForEach(boost::function<void (SessionWrapper)> func)
{
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);

	SessionMap::iterator i = m_sessionMap.begin();
	SessionMap::iterator end = m_sessionMap.end();

	while (i != end)
	{
		SessionMap::iterator next = i;
		next++;
		func((*i).second);
		i = next;
	}
}

unsigned
SessionManager::CountReadySessions() const
{
	unsigned counter = 0;
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);

	SessionMap::const_iterator i = m_sessionMap.begin();
	SessionMap::const_iterator end = m_sessionMap.end();

	while (i != end)
	{
		if ((*i).second.sessionData->IsReady())
			++counter;
		++i;
	}
	return counter;
}

void
SessionManager::ResetAllReadyFlags()
{
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);

	SessionMap::iterator i = m_sessionMap.begin();
	SessionMap::iterator end = m_sessionMap.end();

	while (i != end)
	{
		(*i).second.sessionData->ResetReadyFlag();
		++i;
	}
}

void
SessionManager::Clear()
{
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);
	SessionMap::iterator i = m_sessionMap.begin();
	SessionMap::iterator end = m_sessionMap.end();

	boost::system::error_code ec;
	while (i != end)
	{
		i->second.sessionData->GetAsioSocket()->close(ec);
		++i;
	}
	m_sessionMap.clear();
}

unsigned
SessionManager::GetRawSessionCount()
{
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);
	return m_sessionMap.size();
}

void
SessionManager::SendToAllSessions(SenderHelper &sender, boost::shared_ptr<NetPacket> packet, SessionData::State state)
{
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);

	SessionMap::iterator i = m_sessionMap.begin();
	SessionMap::iterator end = m_sessionMap.end();

	while (i != end)
	{
		if (!i->second.sessionData.get())
			throw ServerException(__FILE__, __LINE__, ERR_NET_INVALID_SESSION, 0);

		// Send each client (with a certain state) a copy of the packet.
		if (i->second.sessionData->GetState() == state)
			sender.Send(i->second.sessionData, packet);
		++i;
	}
}

void
SessionManager::SendLobbyMsgToAllSessions(SenderHelper &sender, boost::shared_ptr<NetPacket> packet, SessionData::State state)
{
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);

	SessionMap::iterator i = m_sessionMap.begin();
	SessionMap::iterator end = m_sessionMap.end();

	while (i != end)
	{
		if (!i->second.sessionData.get())
			throw ServerException(__FILE__, __LINE__, ERR_NET_INVALID_SESSION, 0);

		// Send each client (with a certain state) a copy of the packet.
		if (i->second.sessionData->GetState() == state && i->second.sessionData->WantsLobbyMsg())
			sender.Send(i->second.sessionData, packet);
		++i;
	}
}

void
SessionManager::SendToAllButOneSessions(SenderHelper &sender, boost::shared_ptr<NetPacket> packet, SessionId except, SessionData::State state)
{
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);

	SessionMap::iterator i = m_sessionMap.begin();
	SessionMap::iterator end = m_sessionMap.end();

	while (i != end)
	{
		// Send each fully connected client but one a copy of the packet.
		if (i->second.sessionData->GetState() == state)
			if (i->first != except)
				sender.Send(i->second.sessionData, packet);
		++i;
	}
}

