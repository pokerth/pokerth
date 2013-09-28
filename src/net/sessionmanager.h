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

class SessionManager
{
public:
	SessionManager();
	virtual ~SessionManager();

	void AddSession(boost::shared_ptr<SessionData> sessionData);
	void SetSessionPlayerData(SessionId session, boost::shared_ptr<PlayerData> playerData);
	bool RemoveSession(SessionId session);

	boost::shared_ptr<SessionData> GetSessionById(SessionId id) const;
	boost::shared_ptr<SessionData> GetSessionByPlayerName(const std::string &playerName) const;
	boost::shared_ptr<SessionData> GetSessionByUniquePlayerId(unsigned uniqueId, bool initSessions = false) const;

	PlayerDataList GetPlayerDataList() const;
	PlayerDataList GetSpectatorDataList() const;
	PlayerIdList GetPlayerIdList(int state) const;
	bool IsPlayerConnected(const std::string &playerName) const;
	bool IsPlayerConnected(unsigned uniqueId) const;
	bool IsClientAddressConnected(const std::string &clientAddress) const;

	void ForEach(boost::function<void (boost::shared_ptr<SessionData>)> func);

	unsigned CountReadySessions() const;
	void ResetAllReadyFlags();

	void Clear();
	unsigned GetRawSessionCount() const;
	unsigned GetSessionCountWithState(int state) const;
	bool HasSessionWithState(int state) const;

	void SendToAllSessions(SenderHelper &sender, boost::shared_ptr<NetPacket> packet, int state);
	void SendLobbyMsgToAllSessions(SenderHelper &sender, boost::shared_ptr<NetPacket> packet, int state);
	void SendToAllButOneSessions(SenderHelper &sender, boost::shared_ptr<NetPacket> packet, SessionId except, int state);

protected:

	typedef std::map<SessionId, boost::shared_ptr<SessionData> > SessionMap;

private:

	SessionMap m_sessionMap;
	mutable boost::recursive_mutex m_sessionMapMutex;
};

#endif
