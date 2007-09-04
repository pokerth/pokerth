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

#include <net/ircthread.h>

#include <libircclient.h>

using namespace std;

struct IrcContext
{
	string serverAddress;
	unsigned serverPort;
	string nick;
	string channel;
};

void event_connect (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
	IrcContext *ctx = (IrcContext *) irc_get_ctx(session);

	irc_cmd_join(session, ctx->channel.c_str(), 0);
}


IrcThread::IrcThread()
{
	m_context.reset(new IrcContext);
}

IrcThread::~IrcThread()
{
}

void
IrcThread::Init(const std::string &serverAddress, unsigned serverPort, bool ipv6, const std::string &nick, const std::string &channel)
{
	if (IsRunning())
		return; // TODO: throw exception

	IrcContext &context = GetContext();

	context.serverAddress	= serverAddress;
	context.serverPort		= serverPort;
	context.nick			= nick;
	context.channel			= channel;
}

void
IrcThread::Main()
{
	irc_callbacks_t callbacks;
	irc_session_t *s;

	memset (&callbacks, 0, sizeof(callbacks));

	callbacks.event_connect = event_connect;
	//callbacks.event_join
	//callbacks.event_nick
	//callbacks.event_quit
	//callbacks.event_part
	//callbacks.event_mode
	//callbacks.event_topic
	//callbacks.event_kick
	//callbacks.event_channel
	//callbacks.event_privmsg
	//callbacks.event_notice
	//callbacks.event_invite
	//callbacks.event_umode
	//callbacks.event_ctcp_rep
	//callbacks.event_ctcp_action
	//callbacks.event_unknown
	//callbacks.event_numeric

	//callbacks.event_dcc_chat_req
	//callbacks.event_dcc_send_req

	s = irc_create_session(&callbacks);

	if (s)
	{
		irc_set_ctx(s, &GetContext());

		if (irc_connect(s, GetContext().serverAddress.c_str(), GetContext().serverPort, 0, GetContext().nick.c_str(), 0, 0) == 0)
			irc_run (s);
	}
}

const IrcContext &
IrcThread::GetContext() const
{
	assert(m_context.get());
	return *m_context;
}

IrcContext &
IrcThread::GetContext()
{
	assert(m_context.get());
	return *m_context;
}

