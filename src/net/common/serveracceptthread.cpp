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

#include <net/serveracceptthread.h>
#include <net/servercontext.h>
#include <net/ircthread.h>
#include <net/connectdata.h>
#include <net/serverlobbythread.h>
#include <net/socket_helper.h>
#include <net/serverexception.h>
#include <net/socket_msg.h>
#include <net/socket_startup.h>
#include <core/loghelper.h>

#include <boost/algorithm/string/predicate.hpp>

#define ACCEPT_TIMEOUT_MSEC			50
#define NET_SERVER_LISTEN_BACKLOG	5

using namespace std;

ServerAcceptThread::ServerAcceptThread(GuiInterface &gui, ConfigFile *config, AvatarManager &avatarManager)
: m_gui(gui)
{
	m_context.reset(new ServerContext);
	m_lobbyThread.reset(new ServerLobbyThread(gui, config, avatarManager));
}

ServerAcceptThread::~ServerAcceptThread()
{
}

void
ServerAcceptThread::Init(unsigned serverPort, bool ipv6, bool sctp, const string &pwd, const string &logDir, boost::shared_ptr<IrcThread> ircThread)
{
	if (IsRunning())
	{
		assert(false);
		return;
	}

	ServerContext &context = GetContext();

	context.SetProtocol(sctp ? SOCKET_IPPROTO_SCTP : 0);
	// If a "dual stack" is available, the pokerth server always uses ipv6.
	// In this case, ipv4 requests will be mapped to ipv6.
	context.SetAddrFamily(socket_has_dual_stack() ? AF_INET6 : (ipv6 ? AF_INET6 : AF_INET));
	context.SetServerPort(serverPort);

	GetLobbyThread().Init(pwd, logDir);
	m_ircThread = ircThread;
}

ServerCallback &
ServerAcceptThread::GetCallback()
{
	return m_gui;
}

GuiInterface &
ServerAcceptThread::GetGui()
{
	return m_gui;
}

void
ServerAcceptThread::SignalIrcConnect(const std::string &server)
{
	LOG_MSG("Connected to IRC server " << server << ".");
}

void
ServerAcceptThread::SignalIrcSelfJoined(const std::string &nickName, const std::string &channel)
{
	LOG_MSG("Joined IRC channel " << channel << " as user " << nickName << ".");
	m_ircNick = nickName;
}

void
ServerAcceptThread::SignalIrcChatMsg(const std::string &nickName, const std::string &msg)
{
	if (m_ircThread)
	{
		istringstream msgStream(msg);
		string target;
		msgStream >> target;
		if (boost::algorithm::iequals(target, m_ircNick + ":"))
		{
			string command;
			msgStream >> command;
			if (command == "kick")
			{
				while (msgStream.peek() == ' ')
					msgStream.get();
				string playerName(msgStream.str().substr(msgStream.tellg()));
				if (!playerName.empty())
				{
					if (GetLobbyThread().KickPlayerByName(playerName))
						m_ircThread->SendChatMessage(nickName + ": Successfully kicked player \"" + playerName + "\" from the server.");
					else
						m_ircThread->SendChatMessage(nickName + ": Player \"" + playerName + "\" was not found on the server.");
				}
			}
			else if (command == "stat")
			{
				ServerStats tmpStats = GetLobbyThread().GetStats();
				ostringstream statStream;
				statStream
					<< "Players on Server: " << tmpStats.numberOfPlayersOnServer;
				m_ircThread->SendChatMessage(statStream.str());
			}
			else
				m_ircThread->SendChatMessage(nickName + ": Invalid command \"" + command + "\".");
		}
	}
}

void
ServerAcceptThread::SignalIrcError(int errorCode)
{
}

void
ServerAcceptThread::SignalIrcServerError(int errorCode)
{
}

void
ServerAcceptThread::Main()
{
	try
	{
		Listen();
		if (m_ircThread)
			m_ircThread->Run();
		GetLobbyThread().Run();

		while (!ShouldTerminate() && !GetLobbyThread().Join(0))
		{
			// The main server thread is simple. It only accepts connections.
			AcceptLoop();
		}
	} catch (const PokerTHException &e)
	{
		GetCallback().SignalNetServerError(e.GetErrorId(), e.GetOsErrorCode());
		LOG_ERROR(e.what());
	}
	GetLobbyThread().SignalTermination();
	GetLobbyThread().Join(LOBBY_THREAD_TERMINATE_TIMEOUT_MSEC);

	if (m_ircThread)
	{
		m_ircThread->SignalTermination();
		m_ircThread->Join(ADMIN_IRC_TERMINATE_TIMEOUT_MSEC);
	}
}

void
ServerAcceptThread::Listen()
{
	ServerContext &context = GetContext();

	if (context.GetServerPort() < 1024)
		throw ServerException(__FILE__, __LINE__, ERR_SOCK_INVALID_PORT, 0);

	context.SetSocket(socket(context.GetAddrFamily(), SOCK_STREAM, context.GetProtocol()));
	if (!IS_VALID_SOCKET(context.GetSocket()))
		throw ServerException(__FILE__, __LINE__, ERR_SOCK_CREATION_FAILED, SOCKET_ERRNO());

	unsigned long mode = 1;
	if (IOCTLSOCKET(context.GetSocket(), FIONBIO, &mode) == SOCKET_ERROR)
		throw ServerException(__FILE__, __LINE__, ERR_SOCK_CREATION_FAILED, SOCKET_ERRNO());

	// The following three calls are optional. If they fail, we don't care.
	int reuse = 1;
	setsockopt(context.GetSocket(), SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));
	int nodelay = 1;
	setsockopt(context.GetSocket(), SOL_SOCKET, TCP_NODELAY, (char *)&nodelay, sizeof(nodelay));
	// Enable dual-stack socket on Windows Vista.
	int ipv6only = 0;
	setsockopt(context.GetSocket(), IPPROTO_IPV6, IPV6_V6ONLY, (char *)&ipv6only, sizeof(ipv6only));

	context.GetServerSockaddr()->ss_family = context.GetAddrFamily();

	const char *localAddr = (context.GetAddrFamily() == AF_INET6) ? "::0" : "0.0.0.0";
	if (!socket_string_to_addr(
			localAddr,
			context.GetAddrFamily(),
			(struct sockaddr *)context.GetServerSockaddr(),
			context.GetServerSockaddrSize()))
	{
		throw ServerException(__FILE__, __LINE__, ERR_SOCK_SET_ADDR_FAILED, 0);
	}
	if (!socket_set_port(
			context.GetServerPort(),
			context.GetAddrFamily(),
			(struct sockaddr *)context.GetServerSockaddr(),
			context.GetServerSockaddrSize()))
	{
		throw ServerException(__FILE__, __LINE__, ERR_SOCK_SET_PORT_FAILED, 0);
	}

	if (!IS_VALID_BIND(bind(
			context.GetSocket(),
			(const struct sockaddr *)context.GetServerSockaddr(),
			context.GetServerSockaddrSize())))
	{
		throw ServerException(__FILE__, __LINE__, ERR_SOCK_BIND_FAILED, SOCKET_ERRNO());
	}

	if (!IS_VALID_LISTEN(listen(context.GetSocket(), NET_SERVER_LISTEN_BACKLOG)))
	{
		throw ServerException(__FILE__, __LINE__, ERR_SOCK_LISTEN_FAILED, SOCKET_ERRNO());
	}
}

void
ServerAcceptThread::AcceptLoop()
{
	ServerContext &context = GetContext();

	fd_set readSet;
	struct timeval timeout;

	FD_ZERO(&readSet);
	FD_SET(context.GetSocket(), &readSet);

	timeout.tv_sec  = 0;
	timeout.tv_usec = ACCEPT_TIMEOUT_MSEC * 1000;
	int selectResult = select(context.GetSocket() + 1, &readSet, NULL, NULL, &timeout);
	if (!IS_VALID_SELECT(selectResult))
	{
		throw ServerException(__FILE__, __LINE__, ERR_SOCK_SELECT_FAILED, SOCKET_ERRNO());
	}
	if (selectResult > 0) // accept is possible
	{
		boost::shared_ptr<ConnectData> tmpData(new ConnectData);
		tmpData->SetSocket(accept(context.GetSocket(), NULL, NULL));

		if (!IS_VALID_SOCKET(tmpData->GetSocket()))
		{
			throw ServerException(__FILE__, __LINE__, ERR_SOCK_ACCEPT_FAILED, SOCKET_ERRNO());
		}
		unsigned long mode = 1;
		if (IOCTLSOCKET(tmpData->GetSocket(), FIONBIO, &mode) == SOCKET_ERROR)
		{
			throw ServerException(__FILE__, __LINE__, ERR_SOCK_CREATION_FAILED, SOCKET_ERRNO());
		}
		// Optional call - don't check return value.
		// Enable keepalive - won't be of much use but better than nothing.
		int keepalive = 1;
		setsockopt(tmpData->GetSocket(), SOL_SOCKET, SO_KEEPALIVE, (char *)&keepalive, sizeof(keepalive));

		GetLobbyThread().AddConnection(tmpData);
	}
}

const ServerContext &
ServerAcceptThread::GetContext() const
{
	assert(m_context.get());
	return *m_context;
}

ServerContext &
ServerAcceptThread::GetContext()
{
	assert(m_context.get());
	return *m_context;
}

ServerLobbyThread &
ServerAcceptThread::GetLobbyThread()
{
	assert(m_lobbyThread.get());
	return *m_lobbyThread;
}

