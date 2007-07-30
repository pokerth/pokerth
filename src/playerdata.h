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
/* Player data. */

#ifndef _PLAYERDATA_H_
#define _PLAYERDATA_H_

#include <string>
#include <list>
#include <map>
#include <boost/shared_ptr.hpp>

class SessionData;

enum PlayerType
{
	PLAYER_TYPE_COMPUTER,
	PLAYER_TYPE_HUMAN
};

enum PlayerRights
{
	PLAYER_RIGHTS_NORMAL,
	PLAYER_RIGHTS_ADMIN
};

class PlayerData
{
public:
	PlayerData(unsigned uniqueId, int number, PlayerType type);
	~PlayerData();

	const std::string &GetName() const
	{return m_name;}
	void SetName(const std::string &name)
	{m_name = name;}
	const std::string &GetAvatarFile() const
	{return m_avatarFile;}
	void SetAvatarFile(const std::string &avatarFile)
	{m_avatarFile = avatarFile;}
	boost::shared_ptr<SessionData> GetNetSessionData()
	{return m_netSessionData;}
	void SetNetSessionData(boost::shared_ptr<SessionData> session)
	{m_netSessionData = session;}
	PlayerType GetType() const
	{return m_type;}
	unsigned GetUniqueId() const
	{return m_uniqueId;}
	int GetNumber() const
	{return m_number;}
	void SetNumber(int number)
	{m_number = number;}

	bool operator<(const PlayerData &other)
	{return m_number < other.GetNumber();}

private:
	unsigned						m_uniqueId;
	int								m_number;
	std::string						m_name;
	std::string						m_avatarFile;
	PlayerType						m_type;
	boost::shared_ptr<SessionData>	m_netSessionData;
};

typedef std::list<boost::shared_ptr<PlayerData> > PlayerDataList;
typedef std::map<unsigned, boost::shared_ptr<PlayerData> > PlayerDataMap;

#endif

