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
/* Session data (a session is a valid client connection). */

#ifndef _SESSIONDATA_H_
#define _SESSIONDATA_H_

#include <playerdata.h>
#include <net/socket_helper.h>
#include <string>

#define SESSION_ID_INIT			0

class SessionData
{
public:
	enum State { Init, Established };

	SessionData(SOCKET sockfd, unsigned id);
	~SessionData();

	unsigned GetId() const
	{return m_id;}
	State GetState() const
	{return m_state;}
	void SetState(State state)
	{m_state = state;}

	const boost::shared_ptr<PlayerData> GetPlayerData() const
	{return m_playerData;}
	void SetPlayerData(boost::shared_ptr<PlayerData> playerData)
	{m_playerData = playerData;}

	SOCKET GetSocket() const
	{return m_sockfd;}

	const std::string &GetClientAddr() const
	{return m_clientAddr;}
	void SetClientAddr(const std::string &addr)
	{m_clientAddr = addr;}

private:
	SOCKET							m_sockfd;
	unsigned						m_id;
	State							m_state;
	std::string						m_clientAddr;
	boost::shared_ptr<PlayerData>	m_playerData;
};

#endif
