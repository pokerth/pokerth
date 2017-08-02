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
/* Manager for server bans. */

#ifndef _SERVERBANMANAGER_H_
#define _SERVERBANMANAGER_H_

#include <db/dbdefs.h>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/regex.hpp>
#include <boost/thread.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <map>
#include <list>
#include <string>

class ServerBanManager : public boost::enable_shared_from_this<ServerBanManager>
{
public:
	ServerBanManager(boost::shared_ptr<boost::asio::io_service> ioService);
	virtual ~ServerBanManager();

	void SetAdminPlayerIds(const std::list<DB_id> &adminList);

	void BanPlayerName(const std::string &playerName, unsigned durationHours = 0);
	void BanPlayerRegex(const std::string &playerRegex, unsigned durationHours = 0);
	void BanIPAddress(const std::string &ipAddress, unsigned durationHours);
	bool UnBan(unsigned banId);
	void GetBanList(std::list<std::string> &list) const;
	void ClearBanList();

	bool IsAdminPlayer(DB_id playerId) const;
	bool IsPlayerBanned(const std::string &name) const;
	bool IsIPAddressBanned(const std::string &ipAddress) const;

	void InitGameNameBadWordList(const std::list<std::string> &badWordList);
	bool IsBadGameName(const std::string &name) const;

protected:

	struct TimedPlayerBan {
		boost::shared_ptr<boost::asio::steady_timer> timer;
		std::string nameStr;
		boost::regex nameRegex;
	};
	struct TimedIPBan {
		boost::shared_ptr<boost::asio::steady_timer> timer;
		std::string ipAddress;
	};

	typedef std::map<unsigned, TimedPlayerBan> RegexMap;
	typedef std::map<unsigned, TimedIPBan> IPAddressMap;
	typedef std::list<boost::regex> RegexList;
	typedef std::vector<DB_id> DBPlayerIdList;

	boost::shared_ptr<boost::asio::steady_timer> InternalRegisterTimedBan(unsigned timerId, unsigned durationHours);
	void TimerRemoveBan(const boost::system::error_code &ec, unsigned banId, boost::shared_ptr<boost::asio::steady_timer> timer);

	boost::shared_ptr<boost::asio::io_service> m_ioService;

	unsigned GetNextBanId();

private:
	RegexMap m_banPlayerNameMap;
	RegexList m_gameNameBadWordFilter;
	IPAddressMap m_banIPAddressMap;
	DBPlayerIdList m_adminPlayers;
	unsigned m_curBanId;
	mutable boost::mutex m_banMutex;
};

#endif
