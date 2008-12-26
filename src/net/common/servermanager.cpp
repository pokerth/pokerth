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

#include <net/socket_helper.h>
#include <net/servermanager.h>
#include <net/servercontext.h>
#include <net/ircthread.h>
#include <net/connectdata.h>
#include <net/serverlobbythread.h>
#include <net/serveracceptthread.h>
#include <net/serverexception.h>
#include <net/socket_msg.h>
#include <net/socket_startup.h>
#include <core/loghelper.h>

#include <boost/bind.hpp>
#include <boost/algorithm/string/predicate.hpp>

#define SERVER_RESTART_IRC_BOT_INTERVAL_SEC			86400	// 1 day

using namespace std;

ServerManager::ServerManager(GuiInterface &gui, ConfigFile *config, AvatarManager &avatarManager)
: m_gui(gui), m_playerConfig(config), m_avatarManager(avatarManager)
{
}

ServerManager::~ServerManager()
{
}

void
ServerManager::Init(unsigned serverPort, bool ipv6, ServerNetworkMode mode, const string &pwd, const string &logDir, boost::shared_ptr<IrcThread> ircThread)
{
	m_lobbyThread.reset(new ServerLobbyThread(GetGui(), m_playerConfig, m_avatarManager));
	GetLobbyThread().Init(pwd, logDir);

	if (mode & NETWORK_MODE_TCP)
	{
		boost::shared_ptr<ServerAcceptThread> tcpAcceptThread(new ServerAcceptThread(GetGui()));
		tcpAcceptThread->Init(serverPort, ipv6, false, pwd, logDir, m_lobbyThread);
		m_acceptThreadPool.push_back(tcpAcceptThread);
	}
	if (mode & NETWORK_MODE_SCTP)
	{
		boost::shared_ptr<ServerAcceptThread> sctpAcceptThread(new ServerAcceptThread(GetGui()));
		sctpAcceptThread->Init(serverPort, ipv6, true, pwd, logDir, m_lobbyThread);
		m_acceptThreadPool.push_back(sctpAcceptThread);
	}
	m_ircThread = ircThread;
}

GuiInterface &
ServerManager::GetGui()
{
	return m_gui;
}

void
ServerManager::SignalIrcConnect(const std::string &server)
{
	LOG_MSG("Connected to IRC server " << server << ".");
}

void
ServerManager::SignalIrcSelfJoined(const std::string &nickName, const std::string &channel)
{
	LOG_MSG("Joined IRC channel " << channel << " as user " << nickName << ".");
	m_ircNick = nickName;
}

void
ServerManager::SignalIrcChatMsg(const std::string &nickName, const std::string &msg)
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
				{
					boost::posix_time::time_duration timeDiff(boost::posix_time::second_clock::local_time() - GetLobbyThread().GetStartTime());
					ostringstream statStream;
					statStream
						<< "Server uptime................ " << timeDiff.hours() / 24 << " days " << timeDiff.hours() % 24 << " hours " << timeDiff.minutes() << " minutes " << timeDiff.seconds() << " seconds";
					m_ircThread->SendChatMessage(statStream.str());
				}
				{
					ostringstream statStream;
					statStream
						<< "Players currently on Server.. " << tmpStats.numberOfPlayersOnServer;
					m_ircThread->SendChatMessage(statStream.str());
				}
				{
					ostringstream statStream;
					statStream
						<< "Games currently open......... " << tmpStats.numberOfGamesOpen;
					m_ircThread->SendChatMessage(statStream.str());
				}
				{
					ostringstream statStream;
					statStream
						<< "Total players ever logged in. " << tmpStats.totalPlayersEverLoggedIn
						<< "    (Max at a time: " << tmpStats.maxPlayersLoggedIn << ")";
					m_ircThread->SendChatMessage(statStream.str());
				}
				{
					ostringstream statStream;
					statStream
						<< "Total games ever open........ " << tmpStats.totalGamesEverCreated
						<< "    (Max at a time: " << tmpStats.maxGamesOpen << ")";
					m_ircThread->SendChatMessage(statStream.str());
				}
			}
			else if (command == "msg")
			{
				while (msgStream.peek() == ' ')
					msgStream.get();
				string message(msgStream.str().substr(msgStream.tellg()));
				if (!message.empty())
				{
					GetLobbyThread().SendGlobalNotice(message);
					m_ircThread->SendChatMessage(nickName + ": Global notice sent.");
				}
				else
					m_ircThread->SendChatMessage(nickName + ": Invalid message.");
			}
			else
				m_ircThread->SendChatMessage(nickName + ": Invalid command \"" + command + "\".");
		}
	}
}

void
ServerManager::SignalIrcError(int errorCode)
{
	LOG_MSG("IRC error " << errorCode << ".");
}

void
ServerManager::SignalIrcServerError(int errorCode)
{
	LOG_MSG("IRC server error " << errorCode << ".");
}

void
ServerManager::RunAll()
{
	if (m_ircThread)
		m_ircThread->Run();
	GetLobbyThread().Run();
	for_each(m_acceptThreadPool.begin(), m_acceptThreadPool.end(), boost::mem_fn(&ServerAcceptThread::Run));
}

void
ServerManager::Process()
{
	if (m_ircRestartTimer.elapsed().total_seconds() > SERVER_RESTART_IRC_BOT_INTERVAL_SEC)
	{
		if (m_ircThread)
		{
			m_ircThread->SignalTermination();
			if (m_ircThread->Join(NET_ADMIN_IRC_TERMINATE_TIMEOUT_MSEC))
			{
				boost::shared_ptr<IrcThread> tmpIrcThread(new IrcThread(*m_ircThread));
				tmpIrcThread->Run();
				m_ircThread = tmpIrcThread;
			}
		}
		m_ircRestartTimer.reset();
		m_ircRestartTimer.start();
	}
}

void
ServerManager::SignalTerminationAll()
{
	if (m_ircThread)
		m_ircThread->SignalTermination();
	GetLobbyThread().SignalTermination();
	for_each(m_acceptThreadPool.begin(), m_acceptThreadPool.end(), boost::mem_fn(&ServerAcceptThread::SignalTermination));
}

bool
ServerManager::JoinAll(bool wait)
{
	if (m_ircThread)
		m_ircThread->Join(wait ? NET_ADMIN_IRC_TERMINATE_TIMEOUT_MSEC : 0);
	bool lobbyThreadTerminated = GetLobbyThread().Join(wait ? NET_LOBBY_THREAD_TERMINATE_TIMEOUT_MSEC : 0);
	bool allAcceptThreadsTerminated = true;
	AcceptThreadList::iterator i = m_acceptThreadPool.begin();
	AcceptThreadList::iterator end = m_acceptThreadPool.end();
	while (i != end)
	{
		if (!(*i)->Join(wait ? NET_ACCEPT_THREAD_TERMINATE_TIMEOUT_MSEC : 0))
			allAcceptThreadsTerminated = false;
		++i;
	}
	return lobbyThreadTerminated || allAcceptThreadsTerminated;
}

ServerLobbyThread &
ServerManager::GetLobbyThread()
{
	assert(m_lobbyThread.get());
	return *m_lobbyThread;
}

