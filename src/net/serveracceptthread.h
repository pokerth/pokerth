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
/* Network server thread to accept connections. */

#ifndef _SERVERACCEPTTHREAD_H_
#define _SERVERACCEPTTHREAD_H_

#include <game_defs.h>
#include <core/thread.h>

#include <gui/guiinterface.h>
#include <string>

class ServerContext;
class ServerLobbyThread;
class ServerSenderCallback;
class SenderThread;
class ConfigFile;
class AvatarManager;
class IrcThread;
struct GameData;

class ServerAcceptThread : public Thread, public IrcCallback
{
public:
	ServerAcceptThread(GuiInterface &gui, ConfigFile *config, AvatarManager &avatarManager);
	virtual ~ServerAcceptThread();

	// Set the parameters.
	void Init(unsigned serverPort, bool ipv6, bool sctp, const std::string &pwd, const std::string &logDir, boost::shared_ptr<IrcThread> ircThread);

	ServerCallback &GetCallback();
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

	// Main function of the thread.
	virtual void Main();

	void Listen();
	void AcceptLoop();

	const ServerContext &GetContext() const;
	ServerContext &GetContext();

	ServerLobbyThread &GetLobbyThread();

private:
	boost::shared_ptr<ServerContext> m_context;
	boost::shared_ptr<ServerLobbyThread> m_lobbyThread;

	GuiInterface &m_gui;

	std::string m_ircNick;
	boost::shared_ptr<IrcThread> m_ircThread;
};

#endif
