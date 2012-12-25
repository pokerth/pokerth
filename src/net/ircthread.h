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
/* IRC thread for the lobby. */

#ifndef _IRCTHREAD_H_
#define _IRCTHREAD_H_

#include <third_party/boost/timers.hpp>
#include <string>

#include <net/irccallback.h>
#include <core/thread.h>

struct IrcContext;

class IrcThread : public Thread
{
public:
	IrcThread(const IrcThread &other);
	IrcThread(IrcCallback *callback);
	virtual ~IrcThread();

	// Set the parameters.
	void Init(const std::string &serverAddress, unsigned serverPort, bool ipv6, const std::string &nick, const std::string &channel, const std::string &channelPassword);

	// Send a chat message to the channel.
	void SendChatMessage(const std::string &msg);
	void SendPing();
	void FlushQueue();

	virtual void SignalTermination();

	IrcCallback &GetCallback();

protected:

	// Main function of the thread.
	virtual void Main();
	bool IrcInit();
	void IrcMain();

	void HandleIrcError(int errorCode);

	const IrcContext &GetContext() const;
	IrcContext &GetContext();

private:
	boost::shared_ptr<IrcContext> m_context;
	IrcCallback *m_callback;

	boost::timers::portable::microsec_timer m_terminationTimer;
	boost::timers::portable::microsec_timer m_lastConnectTimer;
};

#endif
