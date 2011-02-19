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
/* Thread safe server session manager. */

#ifndef _SESSIONMANAGER_H_
#define _SESSIONMANAGER_H_

#include <boost/function.hpp>
#include <map>

#include <net/sessiondata.h>
#include <playerdata.h>
#include <gamedata.h>
#include <core/thread.h>

class NetPacket;
class SenderHelper;

struct SessionWrapper {
	SessionWrapper() {}
	SessionWrapper(boost::shared_ptr<SessionData> s, boost::shared_ptr<PlayerData> p)
		: sessionData(s), playerData(p) {}
	boost::shared_ptr<SessionData>	sessionData;
	boost::shared_ptr<PlayerData>	playerData;
};

class SessionManager
{
public:
	SessionManager();
	virtual ~SessionManager();

	bool HasSessions() const;

	void AddSession(boost::shared_ptr<SessionData> sessionData); // new Sessions without player data
	void AddSession(SessionWrapper session);
	void SetSessionPlayerData(SessionId session, boost::shared_ptr<PlayerData> playerData);
	bool RemoveSession(SessionId session);

	SessionWrapper GetSessionById(SessionId id) const;
	SessionWrapper GetSessionByPlayerName(const std::string &playerName) const;
	SessionWrapper GetSessionByUniquePlayerId(unsigned uniqueId, bool initSessions = false) const;

	PlayerDataList GetPlayerDataList() const;
	PlayerIdList GetPlayerIdList(SessionData::State state) const;
	bool IsPlayerConnected(const std::string &playerName) const;
	bool IsPlayerConnected(unsigned uniqueId) const;
	bool IsClientAddressConnected(const std::string &clientAddress) const;

	void ForEach(boost::function<void (SessionWrapper)> func);

	unsigned CountReadySessions() const;
	void ResetAllReadyFlags();

	void Clear();
	unsigned GetRawSessionCount();
	unsigned GetEstablishedSessionCount();

	void SendToAllSessions(SenderHelper &sender, boost::shared_ptr<NetPacket> packet, SessionData::State state);
	void SendLobbyMsgToAllSessions(SenderHelper &sender, boost::shared_ptr<NetPacket> packet, SessionData::State state);
	void SendToAllButOneSessions(SenderHelper &sender, boost::shared_ptr<NetPacket> packet, SessionId except, SessionData::State state);

protected:

	typedef std::map<SessionId, SessionWrapper> SessionMap;

private:

	SessionMap m_sessionMap;
	mutable boost::recursive_mutex m_sessionMapMutex;
};

#endif
