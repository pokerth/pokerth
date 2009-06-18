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

#include <net/serverbanmanager.h>

using namespace std;


ServerBanManager::ServerBanManager(boost::shared_ptr<boost::asio::io_service> ioService)
: m_ioService(ioService), m_curBanId(0)
{
}

ServerBanManager::~ServerBanManager()
{
}

void
ServerBanManager::BanPlayerRegex(const string &playerRegex)
{
	boost::mutex::scoped_lock lock(m_banMutex);
	m_banPlayerNameMap[GetNextBanId()] = boost::regex(playerRegex, boost::regex_constants::no_except);
}

void
ServerBanManager::BanIPAddress(const string &ipAddress, unsigned durationHours)
{
	boost::mutex::scoped_lock lock(m_banMutex);
	unsigned banId = GetNextBanId();
	boost::shared_ptr<TimedIPAddress> tmpItem(new TimedIPAddress(ipAddress, *m_ioService));
	tmpItem->timer.expires_from_now(
		boost::posix_time::hours(durationHours));
	tmpItem->timer.async_wait(
		boost::bind(
			&ServerBanManager::TimerRemoveIPBan, shared_from_this(), boost::asio::placeholders::error, banId, tmpItem));

	m_banIPAddressMap[banId] = tmpItem;
}

bool
ServerBanManager::UnBan(unsigned banId)
{
	bool retVal = false;
	boost::mutex::scoped_lock lock(m_banMutex);
	RegexMap::iterator posNick = m_banPlayerNameMap.find(banId);
	if (posNick != m_banPlayerNameMap.end())
	{
		m_banPlayerNameMap.erase(posNick);
		retVal = true;
	}
	else
	{
		IPAddressMap::iterator posIP = m_banIPAddressMap.find(banId);
		if (posIP != m_banIPAddressMap.end())
		{
			posIP->second->timer.cancel();
			m_banIPAddressMap.erase(posIP);
			retVal = true;
		}
	}
	return retVal;
}

void
ServerBanManager::GetBanList(list<string> &list) const
{
	boost::mutex::scoped_lock lock(m_banMutex);
	RegexMap::const_iterator i_nick = m_banPlayerNameMap.begin();
	RegexMap::const_iterator end_nick = m_banPlayerNameMap.end();
	while (i_nick != end_nick)
	{
		ostringstream banText;
		banText << (*i_nick).first << ": (nick) - " << (*i_nick).second.str();
		list.push_back(banText.str());
		++i_nick;
	}
	IPAddressMap::const_iterator i_ip = m_banIPAddressMap.begin();
	IPAddressMap::const_iterator end_ip = m_banIPAddressMap.end();
	while (i_ip != end_ip)
	{
		ostringstream banText;
		banText << (*i_ip).first << ": (IP) - " << (*i_ip).second->ipAddress << " duration: " << (*i_ip).second->timer.expires_from_now().hours() << "h";
		list.push_back(banText.str());
		++i_ip;
	}
}

void
ServerBanManager::ClearBanList()
{
	boost::mutex::scoped_lock lock(m_banMutex);
	m_banPlayerNameMap.clear();
	m_banIPAddressMap.clear();
}

bool
ServerBanManager::IsPlayerBanned(const std::string &name) const
{
	bool retVal = false;
	boost::mutex::scoped_lock lock(m_banMutex);
	RegexMap::const_iterator i = m_banPlayerNameMap.begin();
	RegexMap::const_iterator end = m_banPlayerNameMap.end();
	while (i != end)
	{
		if (regex_match(name, (*i).second))
		{
			retVal = true;
			break;
		}
		++i;
	}

	return retVal;
}

bool
ServerBanManager::IsIPAddressBanned(const std::string &ipAddress) const
{
	bool retVal = false;
	boost::mutex::scoped_lock lock(m_banMutex);
	IPAddressMap::const_iterator i = m_banIPAddressMap.begin();
	IPAddressMap::const_iterator end = m_banIPAddressMap.end();
	while (i != end)
	{
		if (ipAddress == (*i).second->ipAddress)
		{
			retVal = true;
			break;
		}
		++i;
	}

	return retVal;
}

void
ServerBanManager::TimerRemoveIPBan(const boost::system::error_code &ec, unsigned timerId, boost::shared_ptr<TimedIPAddress> item)
{
	if (!ec && item)
		UnBan(timerId);
}

unsigned
ServerBanManager::GetNextBanId()
{
	m_curBanId++;
	if (m_curBanId == 0) // 0 is an invalid id.
		m_curBanId++;

	return m_curBanId;
}

