/***************************************************************************
 *   Copyright (C) 2009 by Lothar May                                      *
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
/* Manager for server bans. */

#ifndef _SERVERBANMANAGER_H_
#define _SERVERBANMANAGER_H_

#include <boost/regex.hpp>
#include <boost/thread.hpp>
#include <map>
#include <string>

class ServerBanManager
{
public:
	ServerBanManager();
	virtual ~ServerBanManager();

	void BanPlayerRegex(const std::string &playerRegex);
	void BanIPAddress(const std::string &ipAddress);
	bool UnBan(unsigned banId);
	void GetBanList(std::list<std::string> &list) const;
	void ClearBanList();

	bool IsPlayerBanned(const std::string &name) const;
	bool IsIPAddressBanned(const std::string &ipAddress) const;

protected:
	typedef std::map<unsigned, boost::regex> RegexMap;
	typedef std::map<unsigned, std::string> IPAddressMap;

	unsigned GetNextBanId();

private:
	RegexMap m_banPlayerNameMap;
	IPAddressMap m_banIPAddressMap;
	unsigned m_curBanId;
	mutable boost::mutex m_banMutex;
};

#endif
