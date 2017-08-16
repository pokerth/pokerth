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
/* Context of network client. */

#ifndef _CLIENTCONTEXT_H_
#define _CLIENTCONTEXT_H_

#include <boost/shared_ptr.hpp>

#include <net/sessiondata.h>
#include <playerdata.h>


class ClientContext
{
public:
	ClientContext();
	virtual ~ClientContext();

	boost::shared_ptr<SessionData> GetSessionData() const;
	void SetSessionData(boost::shared_ptr<SessionData> sessionData);
	boost::shared_ptr<boost::asio::ip::tcp::resolver> GetResolver() const;
	void SetResolver(boost::shared_ptr<boost::asio::ip::tcp::resolver> resolver);
	bool GetSctp() const
	{
		return m_sctp;
	}
	void SetSctp(bool sctp)
	{
		m_sctp = sctp;
	}
	int GetAddrFamily() const
	{
		return m_addrFamily;
	}
	void SetAddrFamily(int addrFamily)
	{
		m_addrFamily = addrFamily;
	}
	const std::string &GetServerAddr() const
	{
		return m_serverAddr;
	}
	void SetServerAddr(const std::string &serverAddr)
	{
		m_serverAddr = serverAddr;
	}
	const std::string &GetServerPassword() const
	{
		return m_serverPassword;
	}
	void SetServerPassword(const std::string &serverPassword)
	{
		m_serverPassword = serverPassword;
	}
	const std::string &GetServerListUrl() const
	{
		return m_serverListUrl;
	}
	void SetServerListUrl(const std::string &serverListUrl)
	{
		m_serverListUrl = serverListUrl;
	}
	bool GetUseServerList() const
	{
		return m_useServerList;
	}
	void SetUseServerList(bool use)
	{
		m_useServerList = use;
	}
	unsigned GetServerPort() const
	{
		return m_serverPort;
	}
	void SetServerPort(unsigned serverPort)
	{
		m_serverPort = serverPort;
	}
	const std::string &GetAvatarServerAddr() const
	{
		return m_avatarServerAddr;
	}
	void SetAvatarServerAddr(const std::string &avatarServerAddr)
	{
		m_avatarServerAddr = avatarServerAddr;
	}
	const std::string &GetPassword() const
	{
		return m_password;
	}
	void SetPassword(const std::string &password)
	{
		m_password = password;
	}
	const std::string &GetPlayerName() const
	{
		return m_playerName;
	}
	void SetPlayerName(const std::string &playerName)
	{
		m_playerName = playerName;
	}
	PlayerRights GetPlayerRights() const
	{
		return m_playerRights;
	}
	void SetPlayerRights(PlayerRights rights)
	{
		m_playerRights = rights;
	}
	const std::string &GetAvatarFile() const
	{
		return m_avatarFile;
	}
	void SetAvatarFile(const std::string &avatarFile)
	{
		m_avatarFile = avatarFile;
	}
	const std::string &GetCacheDir() const
	{
		return m_cacheDir;
	}
	void SetCacheDir(const std::string &cacheDir)
	{
		m_cacheDir = cacheDir;
	}
	bool GetSubscribeLobbyMsg() const
	{
		return m_hasSubscribedLobbyMsg;
	}
	void SetSubscribeLobbyMsg(bool setSubscribe)
	{
		m_hasSubscribedLobbyMsg = setSubscribe;
	}

	const std::string &GetSessionGuid() const
	{
		return m_sessionGuid;
	}

	void SetSessionGuid(const std::string &sessionGuid)
	{
		m_sessionGuid = sessionGuid;
	}

private:
	boost::shared_ptr<SessionData> m_sessionData;
	boost::shared_ptr<boost::asio::ip::tcp::resolver> m_resolver;
	bool				m_sctp;
	int					m_addrFamily;
	std::string			m_serverAddr;
	std::string			m_serverPassword;
	std::string			m_serverListUrl;
	bool				m_useServerList;
	unsigned			m_serverPort;
	std::string			m_avatarServerAddr;
	std::string			m_password;
	std::string			m_playerName;
	PlayerRights		m_playerRights;
	std::string			m_avatarFile;
	std::string			m_cacheDir;
	bool				m_hasSubscribedLobbyMsg;
	std::string			m_sessionGuid;
};

#endif
