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

#include <net/socket_helper.h>
#include <libircclient.h>
#include <sstream>
#include <cctype>

using namespace std;

#define IRC_WAIT_TERMINATION_MSEC 500

struct IrcContext
{
	IrcContext(IrcThread &t) : ircThread(t), session(NULL), serverPort(0) {}
	IrcThread &ircThread;
	irc_session_t *session;
	string serverAddress;
	unsigned serverPort;
	string nick;
	string channel;
};

void irc_auto_rename_nick(irc_session_t *session)
{
	IrcContext *context = (IrcContext *) irc_get_ctx(session);

	// Automatically rename the nick on collision.
	// First: Try to append the string "Lobby".
	if (context->nick.find("|Lobby") == string::npos)
		context->nick = context->nick + "|Lobby";
	else
	{
		// This didn't work out. Append a number or increment it.
		string::reverse_iterator end = context->nick.rbegin();
		if (!context->nick.empty() && isdigit(*end))
		{
			if (*end != '9')
				*end = (*end) + 1;
			else
				context->nick = context->nick + "0";
		}
		else
			context->nick = context->nick + "1";
	}
	irc_cmd_nick(session, context->nick.c_str());
}

void irc_notify_player_list(irc_session_t *session, const char *players)
{
	IrcContext *context = (IrcContext *) irc_get_ctx(session);

	istringstream input(players);
	string name;
	input >> name;
	while (!input.fail() && !input.eof())
	{
		context->ircThread.GetCallback().SignalIrcPlayerJoined(name);
		input >> name;
	}
}

void
irc_event_connect(irc_session_t *session, const char *irc_event, const char *origin, const char **params, unsigned count)
{
	IrcContext *context = (IrcContext *) irc_get_ctx(session);

	context->ircThread.GetCallback().SignalIrcConnect(origin);
	irc_cmd_join(session, context->channel.c_str(), 0);
}

void
irc_event_join(irc_session_t *session, const char *irc_event, const char *origin, const char **params, unsigned count)
{
	// someone joined the channel.
	IrcContext *context = (IrcContext *) irc_get_ctx(session);

	if (context->nick == origin)
		context->ircThread.GetCallback().SignalIrcSelfJoined(context->nick, context->channel);
	else
		context->ircThread.GetCallback().SignalIrcPlayerJoined(origin);
}

void
irc_event_leave(irc_session_t *session, const char *irc_event, const char *origin, const char **params, unsigned count)
{
	// someone left the channel.
	IrcContext *context = (IrcContext *) irc_get_ctx(session);

	context->ircThread.GetCallback().SignalIrcPlayerLeft(origin);
}

void
irc_event_channel(irc_session_t *session, const char *irc_event, const char *origin, const char **params, unsigned count)
{
	IrcContext *context = (IrcContext *) irc_get_ctx(session);

	if (context->channel == params[0]) // check whether this message is for our channel
	{
		// Signal the message (if any) to GUI.
		if (params[1] && *params[1] != 0)
			context->ircThread.GetCallback().SignalIrcChatMsg(origin, params[1]);
	}
}

void
irc_event_numeric(irc_session_t * session, unsigned irc_event, const char *origin, const char **params, unsigned count)
{
	switch (irc_event)
	{
		case LIBIRC_RFC_ERR_NICKNAMEINUSE :
		case LIBIRC_RFC_ERR_NICKCOLLISION :
			irc_auto_rename_nick(session);
			break;
		case LIBIRC_RFC_RPL_TOPIC :
			break;
		case LIBIRC_RFC_RPL_NAMREPLY :
			if (count >= 4)
				irc_notify_player_list(session, params[3]);
			break;
	}
}

IrcThread::IrcThread(IrcCallback &callback)
: m_callback(callback), m_terminationTimer(boost::posix_time::time_duration(0, 0, 0), boost::timers::portable::microsec_timer::manual_start)
{
	m_context.reset(new IrcContext(*this));
}

IrcThread::~IrcThread()
{
}

void
IrcThread::Init(const std::string &serverAddress, unsigned serverPort, bool ipv6, const std::string &nick, const std::string &channel)
{
	if (IsRunning() || serverAddress.empty() || nick.empty() || channel.empty())
		return; // TODO: throw exception

	IrcContext &context = GetContext();

	context.serverAddress	= serverAddress;
	context.serverPort		= serverPort;
	context.nick			= nick;
	context.channel			= channel;

	// Initialize libirc stuff.
	irc_callbacks_t callbacks;
	memset (&callbacks, 0, sizeof(callbacks));

	callbacks.event_connect = irc_event_connect;
	callbacks.event_join = irc_event_join;
	//callbacks.event_nick
	callbacks.event_quit = irc_event_leave;
	callbacks.event_part = irc_event_leave;
	//callbacks.event_mode
	//callbacks.event_topic
	//callbacks.event_kick
	callbacks.event_channel = irc_event_channel;
	//callbacks.event_privmsg
	//callbacks.event_notice
	//callbacks.event_invite
	//callbacks.event_umode
	//callbacks.event_ctcp_rep
	//callbacks.event_ctcp_action
	//callbacks.event_unknown
	callbacks.event_numeric = irc_event_numeric;

	//callbacks.event_dcc_chat_req
	//callbacks.event_dcc_send_req

	context.session = irc_create_session(&callbacks);

	if (context.session)
	{
		irc_set_ctx(context.session, &context);
		// We want nicknames only, strip them from nick!host.
		irc_option_set(context.session, LIBIRC_OPTION_STRIPNICKS);
	}
}

void
IrcThread::SendChatMessage(const std::string &msg)
{
	IrcContext &context = GetContext();
	irc_cmd_msg(context.session, context.channel.c_str(), msg.c_str());
}

void
IrcThread::SignalTermination()
{
	Thread::SignalTermination();
	irc_cmd_quit(GetContext().session, NULL);
}

void
IrcThread::Main()
{
	IrcContext &context = GetContext();

	irc_session_t *s = context.session;
	if (s)
	{
		if (irc_connect(s, context.serverAddress.c_str(), context.serverPort, 0, context.nick.c_str(), 0, 0) == 0)
		{
			// Main loop.
			while (irc_is_connected(s))
			{
				// Handle thread termination - gracefully.
				if (!m_terminationTimer.is_running())
				{
					if (ShouldTerminate())
						m_terminationTimer.start();
				}
				else
				{
					if (m_terminationTimer.elapsed().total_milliseconds() > IRC_WAIT_TERMINATION_MSEC)
						break;
				}
				
				struct timeval timeout;
				fd_set readSet, writeSet;
				int maxfd = 0;


				FD_ZERO(&readSet);
				FD_ZERO(&writeSet);
				timeout.tv_sec = 0;
				timeout.tv_usec = RECV_TIMEOUT_MSEC * 1000;

				irc_add_select_descriptors(s, &readSet, &writeSet, &maxfd);

				int selectResult = select(maxfd + 1, &readSet, &writeSet, 0, &timeout);
				if (!IS_VALID_SELECT(selectResult))
				{
					//todo
					break;
				}

				if (irc_process_select_descriptors(s, &readSet, &writeSet) != 0)
				{
					//todo
					break;
				}
			}
		}
		irc_destroy_session(s);
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

IrcCallback &
IrcThread::GetCallback()
{
	return m_callback;
}

