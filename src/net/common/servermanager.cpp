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
#include <net/ircthread.h>
#include <net/connectdata.h>
#include <net/serverlobbythread.h>
#include <net/serveraccepthelper.h>
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
	m_ioService.reset(new boost::asio::io_service);
}

ServerManager::~ServerManager()
{
}

void
ServerManager::Init(unsigned serverPort, bool ipv6, ServerNetworkMode mode, const string &pwd, const string &logDir, boost::shared_ptr<IrcThread> ircThread)
{
	m_lobbyThread.reset(new ServerLobbyThread(GetGui(), m_playerConfig, m_avatarManager, m_ioService));
	GetLobbyThread().Init(pwd, logDir);

	if (mode & NETWORK_MODE_TCP)
	{
		boost::shared_ptr<ServerAcceptHelper> tcpAcceptHelper(new ServerAcceptHelper(GetGui(), m_ioService));
		tcpAcceptHelper->Listen(serverPort, ipv6, false, pwd, logDir, m_lobbyThread);
		m_acceptHelperPool.push_back(tcpAcceptHelper);
	}
	if (mode & NETWORK_MODE_SCTP)
	{
		boost::shared_ptr<ServerAcceptHelper> sctpAcceptHelper(new ServerAcceptHelper(GetGui(), m_ioService));
		sctpAcceptHelper->Listen(serverPort, ipv6, true, pwd, logDir, m_lobbyThread);
		m_acceptHelperPool.push_back(sctpAcceptHelper);
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
		try
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
				else if (command == "showip")
				{
					while (msgStream.peek() == ' ')
						msgStream.get();
					string playerName(msgStream.str().substr(msgStream.tellg()));
					if (!playerName.empty())
					{
						string ipAddress(GetLobbyThread().GetPlayerIPAddress(playerName));
						if (!ipAddress.empty())
							m_ircThread->SendChatMessage(nickName + ": The IP address of player \"" + playerName + "\" is: \"" + ipAddress + "\"");
						else
							m_ircThread->SendChatMessage(nickName + ": The IP address of player \"" + playerName + "\" is unknown.");
					}
				}
				else if (command == "bannick")
				{
					while (msgStream.peek() == ' ')
						msgStream.get();
					string playerRegex(msgStream.str().substr(msgStream.tellg()));
					if (!playerRegex.empty())
					{
						GetLobbyThread().BanPlayerRegex(playerRegex);
						m_ircThread->SendChatMessage(nickName + ": The regex \"" + playerRegex + "\" was added to the player ban list.");
					}
				}
				else if (command == "banip")
				{
					while (msgStream.peek() == ' ')
						msgStream.get();
					string ipAddress(msgStream.str().substr(msgStream.tellg()));
					if (!ipAddress.empty())
					{
						GetLobbyThread().BanIPAddress(ipAddress);
						m_ircThread->SendChatMessage(nickName + ": The IP address \"" + ipAddress + "\" was added to the IP address ban list.");
					}
				}
				else if (command == "listban")
				{
					list<string> banList;
					GetLobbyThread().GetBanList(banList);
					list<string>::const_iterator i = banList.begin();
					list<string>::const_iterator end = banList.end();
					while (i != end)
					{
						m_ircThread->SendChatMessage(*i);
						++i;
					}
					ostringstream banListStream;
					banListStream
						<< nickName << ": Total count of bans: " << banList.size();
					m_ircThread->SendChatMessage(banListStream.str());
				}
				else if (command == "removeban")
				{
					unsigned banId = 0;
					msgStream >> banId;
					if (GetLobbyThread().UnBan(banId))
						m_ircThread->SendChatMessage(nickName + ": The ban was successfully removed.");
					else
						m_ircThread->SendChatMessage(nickName + ": This ban does not exist.");
				}
				else if (command == "clearban")
				{
					GetLobbyThread().ClearBanList();
					m_ircThread->SendChatMessage(nickName + ": All ban lists were cleared.");
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
				else if (command == "chat")
				{
					while (msgStream.peek() == ' ')
						msgStream.get();
					string chat(msgStream.str().substr(msgStream.tellg()));
					if (!chat.empty() && chat.size() < MAX_CHAT_TEXT_SIZE)
					{
						GetLobbyThread().SendGlobalChat(chat);
						m_ircThread->SendChatMessage(nickName + ": Global chat message sent.");
					}
					else
						m_ircThread->SendChatMessage(nickName + ": Invalid message.");
				}
				else if (command == "msg")
				{
					while (msgStream.peek() == ' ')
						msgStream.get();
					string message(msgStream.str().substr(msgStream.tellg()));
					if (!message.empty() && message.size() < MAX_CHAT_TEXT_SIZE)
					{
						GetLobbyThread().SendGlobalMsgBox(message);
						m_ircThread->SendChatMessage(nickName + ": Global message box sent.");
					}
					else
						m_ircThread->SendChatMessage(nickName + ": Invalid message.");
				}
				else
					m_ircThread->SendChatMessage(nickName + ": Invalid command \"" + command + "\".");
			}
		} catch (...)
		{
			m_ircThread->SendChatMessage(nickName + ": Syntax error. Please check the command.");
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
}

bool
ServerManager::JoinAll(bool wait)
{
	if (m_ircThread)
		m_ircThread->Join(wait ? NET_ADMIN_IRC_TERMINATE_TIMEOUT_MSEC : 0);
	return GetLobbyThread().Join(wait ? NET_LOBBY_THREAD_TERMINATE_TIMEOUT_MSEC : 0);
}

ServerLobbyThread &
ServerManager::GetLobbyThread()
{
	assert(m_lobbyThread.get());
	return *m_lobbyThread;
}

