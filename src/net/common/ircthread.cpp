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

#include <net/socket_helper.h>
#include <net/ircthread.h>
#include <net/socket_msg.h>
#ifdef _WIN32
#include <libircclient/libircclient.h>
#else
#include <libircclient.h>
#endif

// We need to do the following to handle different versions of libircclient.
// Sadly, libircclient doesn't have actual definitions for its versions in its headers.
// However, we can use a definition that appeared in the same version we need
// to check for. Hacky, but hey, it works.
#ifdef LIBIRC_OPTION_SSL_NO_VERIFY
#ifdef _WIN32
#include <libircclient/libirc_rfcnumeric.h>
#else
#include <libirc_rfcnumeric.h>
#endif
#endif

#include <boost/algorithm/string/predicate.hpp>
#include <queue>
#include <sstream>
#include <cctype>
#include <cstring>

using namespace std;

#define IRC_WAIT_TERMINATION_MSEC			500
#define IRC_RECV_TIMEOUT_MSEC				50
#define IRC_MAX_RENAME_TRIES				5
#define IRC_MIN_RECONNECT_INTERVAL_SEC		60
#define IRC_SEND_LIMIT_BYTES				1024

#define IRC_RENAME_ATTACH					"|Lobby"
#define IRC_MAX_NICK_LEN					16


struct IrcContext {
	IrcContext(IrcThread &t) : ircThread(t), session(NULL), serverPort(0), useIPv6(false), renameTries(0), sendingBlocked(false), sendCounter(0) {}
	IrcThread &ircThread;
	irc_session_t *session;
	string serverAddress;
	unsigned serverPort;
	bool useIPv6;
	string origNick;
	string nick;
	string channel;
	string channelPassword;
	unsigned renameTries;
	bool sendingBlocked;
	size_t sendCounter;
	queue<string> sendQueue;
};

void irc_auto_rename_nick(irc_session_t *session)
{
	IrcContext *context = static_cast<IrcContext *>(irc_get_ctx(session));

	if (context->renameTries <= IRC_MAX_RENAME_TRIES) { // Limit number of rename tries.
		// Automatically rename the nick on collision.
		// First: Try to append the string "Lobby".
		if (context->nick.find(IRC_RENAME_ATTACH) == string::npos) {
			if (context->nick.length() + (sizeof(IRC_RENAME_ATTACH)) > IRC_MAX_NICK_LEN)
				context->nick = context->nick.substr(0, IRC_MAX_NICK_LEN - (sizeof(IRC_RENAME_ATTACH)));
			context->nick = context->nick + IRC_RENAME_ATTACH;
		} else {
			// This didn't work out. Append a number or increment it.
			string::reverse_iterator end = context->nick.rbegin();
			if (!context->nick.empty() && isdigit(*end)) {
				if (*end != '9')
					*end = (*end) + 1;
				else
					context->nick = context->nick + "0";
			} else
				context->nick = context->nick + "1";
		}
		irc_cmd_nick(session, context->nick.c_str());
		context->renameTries++;
	} else
		irc_cmd_quit(session, NULL);
}

void irc_notify_player_list(irc_session_t *session, const char *players)
{
	IrcContext *context = static_cast<IrcContext *>(irc_get_ctx(session));

	istringstream input(players);
	string name;
	input >> name;
	while (!input.fail() && !input.eof()) {
		context->ircThread.GetCallback().SignalIrcPlayerJoined(name);
		input >> name;
	}
}

void irc_handle_server_error(irc_session_t *session, unsigned irc_error_code)
{
	IrcContext *context = static_cast<IrcContext *>(irc_get_ctx(session));

	context->ircThread.GetCallback().SignalIrcServerError(irc_error_code);
}

void
irc_event_connect(irc_session_t *session, const char * /*irc_event*/, const char *origin, const char ** /*params*/, unsigned /*count*/)
{
	IrcContext *context = static_cast<IrcContext *>(irc_get_ctx(session));

	context->ircThread.GetCallback().SignalIrcConnect(origin);
	irc_cmd_join(session, context->channel.c_str(), context->channelPassword.c_str());
}

void
irc_event_join(irc_session_t *session, const char * /*irc_event*/, const char *origin, const char ** /*params*/, unsigned /*count*/)
{
	// someone joined the channel.
	IrcContext *context = static_cast<IrcContext *>(irc_get_ctx(session));

	if (context->nick == origin)
		context->ircThread.GetCallback().SignalIrcSelfJoined(context->nick, context->channel);
	else
		context->ircThread.GetCallback().SignalIrcPlayerJoined(origin);
}

void
irc_event_nick(irc_session_t *session, const char * /*irc_event*/, const char *origin, const char **params, unsigned count)
{
	// someone changed his/her nick
	IrcContext *context = static_cast<IrcContext *>(irc_get_ctx(session));

	if (count && context->nick != params[0]) { // only act if this was not an auto-rename
		if (context->nick == origin)
			context->nick = params[0];
		context->ircThread.GetCallback().SignalIrcPlayerChanged(origin, params[0]);
	}
}

void
irc_event_kick(irc_session_t *session, const char * /*irc_event*/, const char *origin, const char **params, unsigned count)
{
	// someone got kicked
	IrcContext *context = static_cast<IrcContext *>(irc_get_ctx(session));

	string byWhom(origin);
	string who;
	string reason;
	if (count >= 2)
		who = params[1];
	if (count >= 3)
		reason = params[2];
	context->ircThread.GetCallback().SignalIrcPlayerKicked(who, byWhom, reason);
	if (!who.empty())
		context->ircThread.GetCallback().SignalIrcPlayerLeft(who);
}

void
irc_event_leave(irc_session_t *session, const char * /*irc_event*/, const char *origin, const char ** /*params*/, unsigned /*count*/)
{
	// someone left the channel.
	IrcContext *context = static_cast<IrcContext *>(irc_get_ctx(session));

	context->ircThread.GetCallback().SignalIrcPlayerLeft(origin);
}

void
irc_event_channel(irc_session_t *session, const char * /*irc_event*/, const char *origin, const char **params, unsigned count)
{
	IrcContext *context = static_cast<IrcContext *>(irc_get_ctx(session));

	if (count >= 2 && boost::algorithm::iequals(context->channel, params[0])) { // check whether this message is for our channel
		// Signal the message (if any) to GUI.
		if (*params[1] != 0)
			context->ircThread.GetCallback().SignalIrcChatMsg(origin, params[1]);
	}
}

void
irc_event_unknown(irc_session_t *session, const char * irc_event, const char * /*origin*/, const char ** /*params*/, unsigned /*count*/)
{
	IrcContext *context = static_cast<IrcContext *>(irc_get_ctx(session));

	if (boost::algorithm::iequals(irc_event, "PONG")) {
		context->sendingBlocked = false;
		context->ircThread.FlushQueue();
	}
}

void
irc_event_numeric(irc_session_t * session, unsigned irc_event, const char * /*origin*/, const char **params, unsigned count)
{
	switch (irc_event) {
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
	case LIBIRC_RFC_ERR_NOSUCHNICK :
	case LIBIRC_RFC_ERR_NOSUCHCHANNEL :
	case LIBIRC_RFC_ERR_CANNOTSENDTOCHAN :
	case LIBIRC_RFC_ERR_TOOMANYCHANNELS :
	case LIBIRC_RFC_ERR_WASNOSUCHNICK :
	case LIBIRC_RFC_ERR_TOOMANYTARGETS :
	case LIBIRC_RFC_ERR_NOSUCHSERVICE :
	case LIBIRC_RFC_ERR_NOORIGIN :
	case LIBIRC_RFC_ERR_NORECIPIENT :
	case LIBIRC_RFC_ERR_NOTEXTTOSEND :
	case LIBIRC_RFC_ERR_NOTOPLEVEL :
	case LIBIRC_RFC_ERR_WILDTOPLEVEL :
	case LIBIRC_RFC_ERR_BADMASK :
	case LIBIRC_RFC_ERR_UNKNOWNCOMMAND :
	case LIBIRC_RFC_ERR_NOMOTD :
	case LIBIRC_RFC_ERR_NOADMININFO :
	case LIBIRC_RFC_ERR_FILEERROR :
	case LIBIRC_RFC_ERR_NONICKNAMEGIVEN :
	case LIBIRC_RFC_ERR_ERRONEUSNICKNAME :
	case LIBIRC_RFC_ERR_UNAVAILRESOURCE :
	case LIBIRC_RFC_ERR_USERNOTINCHANNEL :
	case LIBIRC_RFC_ERR_NOTONCHANNEL :
	case LIBIRC_RFC_ERR_USERONCHANNEL :
	case LIBIRC_RFC_ERR_NOLOGIN :
	case LIBIRC_RFC_ERR_SUMMONDISABLED :
	case LIBIRC_RFC_ERR_USERSDISABLED :
	case LIBIRC_RFC_ERR_NOTREGISTERED :
	case LIBIRC_RFC_ERR_NEEDMOREPARAMS :
	case LIBIRC_RFC_ERR_ALREADYREGISTRED :
	case LIBIRC_RFC_ERR_NOPERMFORHOST :
	case LIBIRC_RFC_ERR_PASSWDMISMATCH :
	case LIBIRC_RFC_ERR_YOUREBANNEDCREEP :
	case LIBIRC_RFC_ERR_YOUWILLBEBANNED :
	case LIBIRC_RFC_ERR_KEYSET :
	case LIBIRC_RFC_ERR_CHANNELISFULL :
	case LIBIRC_RFC_ERR_UNKNOWNMODE :
	case LIBIRC_RFC_ERR_INVITEONLYCHAN :
	case LIBIRC_RFC_ERR_BANNEDFROMCHAN :
	case LIBIRC_RFC_ERR_BADCHANNELKEY :
	case LIBIRC_RFC_ERR_BADCHANMASK :
	case LIBIRC_RFC_ERR_NOCHANMODES :
	case LIBIRC_RFC_ERR_BANLISTFULL :
	case LIBIRC_RFC_ERR_NOPRIVILEGES :
	case LIBIRC_RFC_ERR_CHANOPRIVSNEEDED :
	case LIBIRC_RFC_ERR_CANTKILLSERVER :
	case LIBIRC_RFC_ERR_RESTRICTED :
	case LIBIRC_RFC_ERR_UNIQOPPRIVSNEEDED :
	case LIBIRC_RFC_ERR_NOOPERHOST :
	case LIBIRC_RFC_ERR_UMODEUNKNOWNFLAG :
	case LIBIRC_RFC_ERR_USERSDONTMATCH :
		irc_handle_server_error(session, irc_event);
		break;
	}
}

IrcThread::IrcThread(const IrcThread &other)
	: Thread(),
	  m_terminationTimer(boost::posix_time::time_duration(0, 0, 0), boost::timers::portable::microsec_timer::manual_start),
	  m_lastConnectTimer(boost::posix_time::time_duration(0, 0, 0), boost::timers::portable::microsec_timer::manual_start)
{
	m_callback = other.m_callback;
	m_context.reset(new IrcContext(*this));

	IrcContext &context = GetContext();
	const IrcContext &otherContext = other.GetContext();

	context.serverAddress	= otherContext.serverAddress;
	context.serverPort		= otherContext.serverPort;
	context.useIPv6			= otherContext.useIPv6;
	context.origNick		= otherContext.origNick;
	context.nick			= otherContext.origNick; // do not use changed nick.
	context.channel			= otherContext.channel;
	context.channelPassword	= otherContext.channelPassword;
}

IrcThread::IrcThread(IrcCallback *callback)
	: m_callback(callback),
	  m_terminationTimer(boost::posix_time::time_duration(0, 0, 0), boost::timers::portable::microsec_timer::manual_start),
	  m_lastConnectTimer(boost::posix_time::time_duration(0, 0, 0), boost::timers::portable::microsec_timer::manual_start)
{
	assert(callback);
	m_context.reset(new IrcContext(*this));
}

IrcThread::~IrcThread()
{
	IrcContext &context = GetContext();
	if (context.session)
		irc_destroy_session(context.session);
}

void
IrcThread::Init(const std::string &serverAddress, unsigned serverPort, bool ipv6, const std::string &nick, const std::string &channel, const std::string &channelPassword)
{
	if (IsRunning() || serverAddress.empty() || nick.empty() || channel.empty())
		return; // TODO: throw exception

	IrcContext &context = GetContext();

	context.serverAddress	= serverAddress;
	context.serverPort		= serverPort;
	context.useIPv6			= ipv6;
	context.origNick		= nick;
	context.nick			= nick;
	context.channel			= channel;
	context.channelPassword	= channelPassword;
}

void
IrcThread::SendChatMessage(const std::string &msg)
{
	IrcContext &context = GetContext();
	if (!context.sendingBlocked) {
		irc_cmd_msg(context.session, context.channel.c_str(), msg.c_str());
		context.sendCounter += msg.size();

		if (context.sendCounter >= IRC_SEND_LIMIT_BYTES) {
			context.sendingBlocked = true;
			context.sendCounter = 0;
			SendPing();
		}
	} else
		context.sendQueue.push(msg);
}

void
IrcThread::SendPing()
{
	IrcContext &context = GetContext();
	irc_send_raw(context.session, "PING %s", context.serverAddress.c_str());
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
	do {
		if (IrcInit())
			IrcMain(); // Will loop until terminated.
	} while (!ShouldTerminate() && GetContext().session && !irc_is_connected(GetContext().session) && m_lastConnectTimer.elapsed().total_seconds() > IRC_MIN_RECONNECT_INTERVAL_SEC);
}

bool
IrcThread::IrcInit()
{
	bool retVal = false;

	IrcContext &context = GetContext();
	if (context.session) {
		irc_destroy_session(context.session);
		context.session = NULL;
	}
	// Initialize libirc stuff.
	irc_callbacks_t callbacks;
	memset (&callbacks, 0, sizeof(callbacks));

	callbacks.event_connect = irc_event_connect;
	callbacks.event_join = irc_event_join;
	callbacks.event_nick = irc_event_nick;
	callbacks.event_quit = irc_event_leave;
	callbacks.event_part = irc_event_leave;
	//callbacks.event_mode
	//callbacks.event_topic
	callbacks.event_kick = irc_event_kick;
	callbacks.event_channel = irc_event_channel;
	//callbacks.event_privmsg
	//callbacks.event_notice
	//callbacks.event_invite
	//callbacks.event_umode
	//callbacks.event_ctcp_rep
	//callbacks.event_ctcp_action
	callbacks.event_unknown = irc_event_unknown;
	callbacks.event_numeric = irc_event_numeric;

	//callbacks.event_dcc_chat_req
	//callbacks.event_dcc_send_req

	context.session = irc_create_session(&callbacks);

	if (context.session) {
		irc_set_ctx(context.session, &context);
		// We want nicknames only, strip them from nick!host.
		irc_option_set(context.session, LIBIRC_OPTION_STRIPNICKS);
		retVal = true;
	}
	return retVal;
}

void
IrcThread::IrcMain()
{
	m_lastConnectTimer.restart();
	IrcContext &context = GetContext();

	irc_session_t *s = context.session;

	bool connected = false;
	if (context.useIPv6)
		connected = irc_connect6(s, context.serverAddress.c_str(), context.serverPort, 0, context.nick.c_str(), 0, 0) == 0;
	else
		connected = irc_connect(s, context.serverAddress.c_str(), context.serverPort, 0, context.nick.c_str(), 0, 0) == 0;

	if (!connected)
		HandleIrcError(irc_errno(s));
	else {
		// Main loop.
		while (irc_is_connected(s)) {
			// Handle thread termination - gracefully.
			if (!m_terminationTimer.is_running()) {
				if (ShouldTerminate())
					m_terminationTimer.start();
			} else {
				if (m_terminationTimer.elapsed().total_milliseconds() > IRC_WAIT_TERMINATION_MSEC)
					break;
			}

			struct timeval timeout;
			fd_set readSet, writeSet;
			int maxfd = 0;


			FD_ZERO(&readSet);
			FD_ZERO(&writeSet);
			timeout.tv_sec = 0;
			timeout.tv_usec = IRC_RECV_TIMEOUT_MSEC * 1000;

			irc_add_select_descriptors(s, &readSet, &writeSet, &maxfd);

			int selectResult = select(maxfd + 1, &readSet, &writeSet, 0, &timeout);
			if (selectResult == -1) {
				GetCallback().SignalIrcError(ERR_IRC_SELECT_FAILED);
				break;
			}

			if (irc_process_select_descriptors(s, &readSet, &writeSet) != 0) {
				int errorCode = irc_errno(s);
				if (errorCode) {
					HandleIrcError(errorCode);
					break;
				}
			}
			Msleep(10); // paranoia
		}
	}
}

void
IrcThread::FlushQueue()
{
	IrcContext &context = GetContext();

	while (!context.sendingBlocked && !context.sendQueue.empty()) {
		string msg(context.sendQueue.front());
		context.sendQueue.pop();
		SendChatMessage(msg);
	}
}

void
IrcThread::HandleIrcError(int errorCode)
{
	int internalErrorCode = ERR_IRC_INTERNAL;
	switch(errorCode) {
	case LIBIRC_ERR_RESOLV:
	case LIBIRC_ERR_SOCKET:
	case LIBIRC_ERR_CONNECT:
		internalErrorCode = ERR_IRC_CONNECT_FAILED;
		break;
	case LIBIRC_ERR_INVAL:
	case LIBIRC_ERR_STATE:
		internalErrorCode = ERR_IRC_INVALID_PARAM;
		break;
	case LIBIRC_ERR_CLOSED:
	case LIBIRC_ERR_TERMINATED:
		internalErrorCode = ERR_IRC_TERMINATED;
		break;
	case LIBIRC_ERR_READ:
		internalErrorCode = ERR_IRC_RECV_FAILED;
		break;
	case LIBIRC_ERR_WRITE:
		internalErrorCode = ERR_IRC_SEND_FAILED;
		break;
	case LIBIRC_ERR_TIMEOUT:
		internalErrorCode = ERR_IRC_TIMEOUT;
		break;
	}
	GetCallback().SignalIrcError(internalErrorCode);
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
	assert(m_callback);
	return *m_callback;
}

