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
/* IRC admin bot for the server. */

#ifndef _SERVERADMINBOT_H_
#define _SERVERADMINBOT_H_

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <third_party/boost/timers.hpp>
#include <string>
#include <list>

#include <game_defs.h>
#include <net/serverircbotcallback.h>
#include <gui/guiinterface.h>

class ServerLobbyThread;
class ServerAcceptHelper;
class SenderThread;
class ConfigFile;
class AvatarManager;
class IrcThread;

class ServerAdminBot : public IrcCallback
{
public:
	ServerAdminBot();
	virtual ~ServerAdminBot();

	void Init(boost::shared_ptr<ServerLobbyThread> lobbyThread, boost::shared_ptr<IrcThread> ircAdminThread);

	// Main start function.
	void Run();

	// Perform processing.
	void Process();

	void SignalTermination();
	bool Join(bool wait);

	virtual void SignalIrcConnect(const std::string &server);
	virtual void SignalIrcSelfJoined(const std::string &nickName, const std::string &channel);
	virtual void SignalIrcPlayerJoined(const std::string & /*nickName*/) {}
	virtual void SignalIrcPlayerChanged(const std::string & /*oldNick*/, const std::string & /*newNick*/) {}
	virtual void SignalIrcPlayerKicked(const std::string & /*nickName*/, const std::string & /*byWhom*/, const std::string & /*reason*/) {}
	virtual void SignalIrcPlayerLeft(const std::string & /*nickName*/) {}
	virtual void SignalIrcChatMsg(const std::string &nickName, const std::string &msg);
	virtual void SignalIrcError(int errorCode);
	virtual void SignalIrcServerError(int errorCode);

protected:
	ServerLobbyThread &GetLobbyThread();

private:
	std::string m_ircNick;
	mutable boost::mutex m_notifyMutex;
	int m_notifyTimeoutMinutes;
	int m_notifyIntervalMinutes;
	int m_notifyCounter;

	boost::shared_ptr<ServerLobbyThread> m_lobbyThread;
	boost::shared_ptr<IrcThread> m_ircAdminThread;
	boost::timers::portable::second_timer m_ircRestartTimer;
	boost::timers::portable::second_timer m_notifyTimer;
};

#endif
