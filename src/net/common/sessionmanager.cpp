/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/

#include <net/sessionmanager.h>
#include <net/senderhelper.h>
#include <net/serverexception.h>
#include <net/socket_msg.h>

using namespace std;

#define SERVER_MAX_GUEST_USERS_LOBBY	50		// LG: Maximum number of guests users in lobby allowed

SessionManager::SessionManager()
{
}

SessionManager::~SessionManager()
{
	Clear();
}

void
SessionManager::AddSession(boost::shared_ptr<SessionData> session)
{
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);

	SessionMap::iterator pos = m_sessionMap.lower_bound(session->GetId());

	// If pos points to a pair whose key is equivalent to the socket, this handle
	// already exists within the list.
	if (pos != m_sessionMap.end() && session->GetId() == pos->first) {
		throw ServerException(__FILE__, __LINE__, ERR_SOCK_CONN_EXISTS, 0);
	}
	m_sessionMap.insert(pos, SessionMap::value_type(session->GetId(), session));
}

void
SessionManager::SetSessionPlayerData(SessionId session, boost::shared_ptr<PlayerData> playerData)
{
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);
	SessionMap::iterator pos = m_sessionMap.find(session);

	if (pos != m_sessionMap.end())
		pos->second->SetPlayerData(playerData);
}

bool
SessionManager::RemoveSession(SessionId session)
{
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);
	return m_sessionMap.erase(session) == 1;
}

boost::shared_ptr<SessionData>
SessionManager::GetSessionById(SessionId id) const
{
	boost::shared_ptr<SessionData> tmpSession;
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);
	SessionMap::const_iterator pos = m_sessionMap.find(id);
	if (pos != m_sessionMap.end())
		tmpSession = pos->second;
	return tmpSession;
}

boost::shared_ptr<SessionData>
SessionManager::GetSessionByPlayerName(const string &playerName) const
{
	boost::shared_ptr<SessionData> tmpSession;
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);

	SessionMap::const_iterator session_i = m_sessionMap.begin();
	SessionMap::const_iterator session_end = m_sessionMap.end();

	while (session_i != session_end) {
		// Check all players which are fully connected.
		if (session_i->second->GetState() != SessionData::Init) {
			boost::shared_ptr<PlayerData> tmpPlayer(session_i->second->GetPlayerData());
			if (!tmpPlayer)
				throw ServerException(__FILE__, __LINE__, ERR_NET_INVALID_SESSION, 0);
			if (tmpPlayer->GetName() == playerName) {
				tmpSession = session_i->second;
				break;
			}
		}

		++session_i;
	}
	return tmpSession;
}

boost::shared_ptr<SessionData>
SessionManager::GetSessionByUniquePlayerId(unsigned uniqueId, bool initSessions) const
{
	boost::shared_ptr<SessionData> tmpSession;
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);

	SessionMap::const_iterator session_i = m_sessionMap.begin();
	SessionMap::const_iterator session_end = m_sessionMap.end();

	while (session_i != session_end) {
		// Check all players which are fully connected.
		if (initSessions || session_i->second->GetState() != SessionData::Init) {
			boost::shared_ptr<PlayerData> tmpPlayer(session_i->second->GetPlayerData());
			if (tmpPlayer && tmpPlayer->GetUniqueId() == uniqueId) {
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

	while (session_i != session_end) {
		// Get all players in the game.
		if (session_i->second->GetState() == SessionData::Game) {
			boost::shared_ptr<PlayerData> tmpPlayer(session_i->second->GetPlayerData());
			if (!tmpPlayer.get() || tmpPlayer->GetName().empty())
				throw ServerException(__FILE__, __LINE__, ERR_NET_INVALID_SESSION, 0);
			playerList.push_back(tmpPlayer);
		}
		++session_i;
	}
	return playerList;
}

PlayerDataList
SessionManager::GetSpectatorDataList() const
{
	PlayerDataList spectatorList;
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);

	SessionMap::const_iterator session_i = m_sessionMap.begin();
	SessionMap::const_iterator session_end = m_sessionMap.end();

	while (session_i != session_end) {
		// Get all spectators of the game.
		if (session_i->second->GetState() == SessionData::Spectating || session_i->second->GetState() == SessionData::SpectatorWaiting) {
			boost::shared_ptr<PlayerData> tmpPlayer(session_i->second->GetPlayerData());
			if (!tmpPlayer.get() || tmpPlayer->GetName().empty())
				throw ServerException(__FILE__, __LINE__, ERR_NET_INVALID_SESSION, 0);
			spectatorList.push_back(tmpPlayer);
		}
		++session_i;
	}
	return spectatorList;
}

PlayerIdList
SessionManager::GetPlayerIdList(int state) const
{
	PlayerIdList playerList;
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);

	SessionMap::const_iterator session_i = m_sessionMap.begin();
	SessionMap::const_iterator session_end = m_sessionMap.end();

	while (session_i != session_end) {
		// Get all players in the game.
		if ((session_i->second->GetState() & state) != 0) {
			playerList.push_back(session_i->second->GetPlayerData()->GetUniqueId());
		}
		++session_i;
	}
	return playerList;
}

bool
SessionManager::IsPlayerConnected(const string &playerName) const
{
	bool retVal = false;

	boost::shared_ptr<SessionData> tmpSession = GetSessionByPlayerName(playerName);

	if (tmpSession && tmpSession->GetPlayerData())
		retVal = true;

	return retVal;
}

bool
SessionManager::IsPlayerConnected(unsigned uniqueId) const
{
	bool retVal = false;

	boost::shared_ptr<SessionData> tmpSession = GetSessionByUniquePlayerId(uniqueId);

	if (tmpSession && tmpSession->GetPlayerData())
		retVal = true;

	return retVal;
}

bool
SessionManager::IsClientAddressConnected(const std::string &clientAddress) const
{
	bool retVal = false;
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);

	SessionMap::const_iterator i = m_sessionMap.begin();
	SessionMap::const_iterator end = m_sessionMap.end();

	while (i != end) {
		if ((*i).second->GetClientAddr() == clientAddress) {
			retVal = true;
			break;
		}
		++i;
	}
	return retVal;
}

bool
SessionManager::IsGuestAllowedToConnect(const std::string &clientAddress) const
{
	bool retVal = true;
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);

	SessionMap::const_iterator i = m_sessionMap.begin();
	SessionMap::const_iterator end = m_sessionMap.end();

	int num = 0;
	while (i != end) {
		boost::shared_ptr<PlayerData> tmpPlayer(i->second->GetPlayerData());
		if (tmpPlayer && tmpPlayer->GetRights() == PLAYER_RIGHTS_GUEST) {
			num++;
			if (i->second->GetClientAddr() == clientAddress) {
				// guest has same ip as another guest in lobby or
				retVal = false;
				break;
			}
		}
		++i;
	}

	// Check if number of players in lobby exceeds max
	if (num >= SERVER_MAX_GUEST_USERS_LOBBY) retVal = false;

	return retVal;
}

void
SessionManager::ForEach(boost::function<void (boost::shared_ptr<SessionData>)> func)
{
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);

	SessionMap::iterator i = m_sessionMap.begin();
	SessionMap::iterator end = m_sessionMap.end();

	while (i != end) {
		SessionMap::iterator next = i;
		++next;
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

	while (i != end) {
		if ((*i).second->IsReady())
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

	while (i != end) {
		(*i).second->ResetReadyFlag();
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
	while (i != end) {
		// Close all raw handles.
		i->second->CloseSocketHandle();
		i->second->CloseWebSocketHandle();
		++i;
	}
	m_sessionMap.clear();
}

unsigned
SessionManager::GetRawSessionCount() const
{
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);
	return (unsigned)m_sessionMap.size();
}

unsigned
SessionManager::GetSessionCountWithState(int state) const
{
	unsigned counter = 0;
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);

	SessionMap::const_iterator i = m_sessionMap.begin();
	SessionMap::const_iterator end = m_sessionMap.end();

	while (i != end) {
		if ((i->second->GetState() & state) != 0)
			++counter;
		++i;
	}
	return counter;
}

bool
SessionManager::HasSessionWithState(int state) const
{
	bool retVal = false;
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);

	SessionMap::const_iterator i = m_sessionMap.begin();
	SessionMap::const_iterator end = m_sessionMap.end();

	while (i != end) {
		if ((i->second->GetState() & state) != 0) {
			retVal = true;
			break;
		}
		++i;
	}
	return retVal;
}

void
SessionManager::SendToAllSessions(SenderHelper &sender, boost::shared_ptr<NetPacket> packet, int state)
{
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);

	SessionMap::iterator i = m_sessionMap.begin();
	SessionMap::iterator end = m_sessionMap.end();

	while (i != end) {
		if (!i->second.get())
			throw ServerException(__FILE__, __LINE__, ERR_NET_INVALID_SESSION, 0);

		// Send each client (with a certain state) a copy of the packet.
		if ((i->second->GetState() & state) != 0)
			sender.Send(i->second, packet);
		++i;
	}
}

void
SessionManager::SendLobbyMsgToAllSessions(SenderHelper &sender, boost::shared_ptr<NetPacket> packet, int state)
{
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);

	SessionMap::iterator i = m_sessionMap.begin();
	SessionMap::iterator end = m_sessionMap.end();

	while (i != end) {
		if (!i->second.get())
			throw ServerException(__FILE__, __LINE__, ERR_NET_INVALID_SESSION, 0);

		// Send each client (with a certain state) a copy of the packet.
		if ((i->second->GetState() & state) != 0 && i->second->WantsLobbyMsg())
			sender.Send(i->second, packet);
		++i;
	}
}

void
SessionManager::SendToAllButOneSessions(SenderHelper &sender, boost::shared_ptr<NetPacket> packet, SessionId except, int state)
{
	boost::recursive_mutex::scoped_lock lock(m_sessionMapMutex);

	SessionMap::iterator i = m_sessionMap.begin();
	SessionMap::iterator end = m_sessionMap.end();

	while (i != end) {
		// Send each fully connected client but one a copy of the packet.
		if ((i->second->GetState() & state) != 0)
			if (i->first != except)
				sender.Send(i->second, packet);
		++i;
	}
}

