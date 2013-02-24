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

#include <net/serverbanmanager.h>
#include <algorithm>

using namespace std;


ServerBanManager::ServerBanManager(boost::shared_ptr<boost::asio::io_service> ioService)
	: m_ioService(ioService), m_curBanId(0)
{
}

ServerBanManager::~ServerBanManager()
{
}

void
ServerBanManager::SetAdminPlayerIds(const std::list<DB_id> &adminList)
{
	boost::mutex::scoped_lock lock(m_banMutex);
	m_adminPlayers.resize(adminList.size());
	copy(adminList.begin(), adminList.end(), m_adminPlayers.begin());
	sort(m_adminPlayers.begin(), m_adminPlayers.end());
}

void
ServerBanManager::BanPlayerName(const std::string &playerName, unsigned durationHours)
{
	boost::mutex::scoped_lock lock(m_banMutex);
	unsigned banId = GetNextBanId();

	TimedPlayerBan tmpBan;
	tmpBan.timer = InternalRegisterTimedBan(banId, durationHours);
	tmpBan.nameStr = playerName;
	m_banPlayerNameMap[banId] = tmpBan;
}

void
ServerBanManager::BanPlayerRegex(const string &playerRegex, unsigned durationHours)
{
	boost::mutex::scoped_lock lock(m_banMutex);
	unsigned banId = GetNextBanId();

	TimedPlayerBan tmpBan;
	tmpBan.timer = InternalRegisterTimedBan(banId, durationHours);
	tmpBan.nameRegex = boost::regex(playerRegex, boost::regex::extended | boost::regex::icase);
	m_banPlayerNameMap[banId] = tmpBan;
}

void
ServerBanManager::BanIPAddress(const string &ipAddress, unsigned durationHours)
{
	boost::mutex::scoped_lock lock(m_banMutex);
	unsigned banId = GetNextBanId();

	TimedIPBan tmpBan;
	tmpBan.timer = InternalRegisterTimedBan(banId, durationHours);
	tmpBan.ipAddress = ipAddress;
	m_banIPAddressMap[banId] = tmpBan;
}

bool
ServerBanManager::UnBan(unsigned banId)
{
	bool retVal = false;
	boost::mutex::scoped_lock lock(m_banMutex);
	RegexMap::iterator posNick = m_banPlayerNameMap.find(banId);
	if (posNick != m_banPlayerNameMap.end()) {
		if (posNick->second.timer)
			posNick->second.timer->cancel();
		m_banPlayerNameMap.erase(posNick);
		retVal = true;
	} else {
		IPAddressMap::iterator posIP = m_banIPAddressMap.find(banId);
		if (posIP != m_banIPAddressMap.end()) {
			if (posIP->second.timer)
				posIP->second.timer->cancel();
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
	while (i_nick != end_nick) {
		ostringstream banText;
		if ((*i_nick).second.nameStr.empty())
			banText << (*i_nick).first << ": (nickRegex) - " << (*i_nick).second.nameRegex.str();
		else
			banText << (*i_nick).first << ": (nickStr) - " << (*i_nick).second.nameStr;

		if ((*i_nick).second.timer)
			banText << " duration: " << (*i_nick).second.timer->expires_from_now().hours() << "h";
		list.push_back(banText.str());
		++i_nick;
	}
	IPAddressMap::const_iterator i_ip = m_banIPAddressMap.begin();
	IPAddressMap::const_iterator end_ip = m_banIPAddressMap.end();
	while (i_ip != end_ip) {
		ostringstream banText;
		banText << (*i_ip).first << ": (IP) - " << (*i_ip).second.ipAddress;
		if ((*i_ip).second.timer)
			banText << " duration: " << (*i_ip).second.timer->expires_from_now().hours() << "h";
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
ServerBanManager::IsAdminPlayer(DB_id playerId) const
{
	bool retVal = false;
	if (playerId != DB_ID_INVALID) {
		boost::mutex::scoped_lock lock(m_banMutex);

		if (binary_search(m_adminPlayers.begin(), m_adminPlayers.end(), playerId)) {
			retVal = true;
		}
	}
	return retVal;
}

bool
ServerBanManager::IsPlayerBanned(const std::string &name) const
{
	bool retVal = false;
	boost::mutex::scoped_lock lock(m_banMutex);
	RegexMap::const_iterator i = m_banPlayerNameMap.begin();
	RegexMap::const_iterator end = m_banPlayerNameMap.end();
	while (i != end) {
		// Use regex only if name not set.
		if ((*i).second.nameStr.empty()) {
			if (regex_match(name, (*i).second.nameRegex)) {
				retVal = true;
				break;
			}
		} else {
			if (name == (*i).second.nameStr) {
				retVal = true;
				break;
			}
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
	while (i != end) {
		if (ipAddress == (*i).second.ipAddress) {
			retVal = true;
			break;
		}
		++i;
	}

	return retVal;
}

void
ServerBanManager::InitGameNameBadWordList(const std::list<string> &badWordList)
{
	list<string>::const_iterator i = badWordList.begin();
	list<string>::const_iterator end = badWordList.end();
	while (i != end) {
		m_gameNameBadWordFilter.push_back(boost::regex(*i, boost::regex::extended | boost::regex::icase));
		++i;
	}
}

bool
ServerBanManager::IsBadGameName(const std::string &name) const
{
	bool retVal = false;
	RegexList::const_iterator i = m_gameNameBadWordFilter.begin();
	RegexList::const_iterator end = m_gameNameBadWordFilter.end();
	while (i != end) {
		if (regex_match(name, *i)) {
			retVal = true;
			break;
		}
		++i;
	}
	return retVal;
}

boost::shared_ptr<boost::asio::deadline_timer>
ServerBanManager::InternalRegisterTimedBan(unsigned timerId, unsigned durationHours)
{
	boost::shared_ptr<boost::asio::deadline_timer> tmpTimer;
	if (durationHours) {
		tmpTimer.reset(new boost::asio::deadline_timer(*m_ioService));
		tmpTimer->expires_from_now(
			boost::posix_time::hours(durationHours));
		tmpTimer->async_wait(
			boost::bind(
				&ServerBanManager::TimerRemoveBan, shared_from_this(), boost::asio::placeholders::error, timerId, tmpTimer));
	}
	return tmpTimer;
}

void
ServerBanManager::TimerRemoveBan(const boost::system::error_code &ec, unsigned banId, boost::shared_ptr<boost::asio::deadline_timer> timer)
{
	if (!ec && timer)
		UnBan(banId);
}

unsigned
ServerBanManager::GetNextBanId()
{
	m_curBanId++;
	if (m_curBanId == 0) // 0 is an invalid id.
		m_curBanId++;

	return m_curBanId;
}

