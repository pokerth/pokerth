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
/* IRC lobby bot for the server. */

#ifndef _SERVERLOBBYBOT_H_
#define _SERVERLOBBYBOT_H_

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <string>
#include <list>

#include <game_defs.h>
#include <net/serverircbotcallback.h>
#include <gui/guiinterface.h>

class ServerLobbyThread;
class SenderThread;
class ConfigFile;
class AvatarManager;
class IrcThread;

class ServerLobbyBot : public IrcCallback, public ServerIrcBotCallback, public boost::enable_shared_from_this<ServerLobbyBot>
{
public:
	ServerLobbyBot(boost::shared_ptr<boost::asio::io_service> ioService);
	virtual ~ServerLobbyBot();

	void Init(boost::shared_ptr<ServerLobbyThread> lobbyThread, boost::shared_ptr<IrcThread> ircLobbyThread);

	// Main start function.
	void Run();

	// Reconnect the bot.
	void Reconnect(const boost::system::error_code& ec);

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

	virtual void SignalLobbyMessage(unsigned playerId, const std::string &playerName, const std::string &msg);

protected:
	ServerLobbyThread &GetLobbyThread();

private:
	std::string m_ircNick;

	boost::shared_ptr<ServerLobbyThread> m_lobbyThread;
	boost::shared_ptr<IrcThread> m_ircLobbyThread;

	boost::asio::steady_timer m_reconnectTimer;
};

#endif
