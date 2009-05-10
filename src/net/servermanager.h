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
/* Manager thread for the server. */

#ifndef _SERVERMANAGER_H_
#define _SERVERMANAGER_H_

#include <boost/asio.hpp>
#include <third_party/boost/timers.hpp>
#include <string>
#include <list>

#include <game_defs.h>
#include <gui/guiinterface.h>

class ServerLobbyThread;
class ServerAcceptManager;
class SenderThread;
class ConfigFile;
class AvatarManager;
class IrcThread;

class ServerManager : public IrcCallback
{
public:
	ServerManager(GuiInterface &gui, ConfigFile *config, AvatarManager &avatarManager);
	virtual ~ServerManager();

	// Set the parameters.
	void Init(unsigned serverPort, bool ipv6, ServerNetworkMode mode, const std::string &pwd, const std::string &logDir, boost::shared_ptr<IrcThread> ircThread);

	// Main start function.
	void RunAll();

	// Let the server manager perform processing.
	void Process();

	void SignalTerminationAll();
	bool JoinAll(bool wait);

	GuiInterface &GetGui();

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
	typedef std::list<boost::shared_ptr<ServerAcceptManager> > AcceptManagerList;

	ServerLobbyThread &GetLobbyThread();

private:
	GuiInterface &m_gui;
	ConfigFile *m_playerConfig;
	AvatarManager &m_avatarManager;

	std::string m_ircNick;

	boost::shared_ptr<boost::asio::io_service> m_ioService;
	boost::shared_ptr<ServerLobbyThread> m_lobbyThread;
	boost::shared_ptr<IrcThread> m_ircThread;
	boost::timers::portable::microsec_timer m_ircRestartTimer;
	AcceptManagerList m_acceptManagerPool;
};

#endif
