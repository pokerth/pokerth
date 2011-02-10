/***************************************************************************
 *   Copyright (C) 2009 by Lothar May                                      *
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

#include <net/serveradminbot.h>
#include <net/ircthread.h>
#include <net/serverlobbythread.h>
#include <net/serverbanmanager.h>
#include <net/serverexception.h>
#include <net/socket_msg.h>
#include <net/socket_startup.h>
#include <core/loghelper.h>

#include <boost/bind.hpp>
#include <boost/algorithm/string/predicate.hpp>

#define SERVER_RESTART_IRC_BOT_INTERVAL_SEC			86400	// 1 day

using namespace std;

ServerAdminBot::ServerAdminBot()
: m_ircRestartTimer(boost::posix_time::time_duration(0, 0, 0), boost::timers::portable::microsec_timer::auto_start)
{
}

ServerAdminBot::~ServerAdminBot()
{
}

void
ServerAdminBot::Init(boost::shared_ptr<ServerLobbyThread> lobbyThread, boost::shared_ptr<IrcThread> ircAdminThread)
{
	m_lobbyThread = lobbyThread;
	m_ircAdminThread = ircAdminThread;
}

void
ServerAdminBot::SignalIrcConnect(const std::string &server)
{
	LOG_MSG("Admin bot: Connected to IRC server " << server << ".");
}

void
ServerAdminBot::SignalIrcSelfJoined(const std::string &nickName, const std::string &channel)
{
	LOG_MSG("Admin bot: Joined IRC channel " << channel << " as user " << nickName << ".");
	m_ircNick = nickName;
}

void
ServerAdminBot::SignalIrcChatMsg(const std::string &nickName, const std::string &msg)
{
	if (m_ircAdminThread)
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
							m_ircAdminThread->SendChatMessage(nickName + ": Successfully kicked player \"" + playerName + "\" from the server.");
						else
							m_ircAdminThread->SendChatMessage(nickName + ": Player \"" + playerName + "\" was not found on the server.");
					}
				}
				else if (command == "cleaner-reconnect")
				{
					GetLobbyThread().ReconnectChatBot();
					m_ircAdminThread->SendChatMessage(nickName + ": Cleaner bot reconnect initiated.");
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
							m_ircAdminThread->SendChatMessage(nickName + ": The IP address of player \"" + playerName + "\" is: \"" + ipAddress + "\"");
						else
							m_ircAdminThread->SendChatMessage(nickName + ": The IP address of player \"" + playerName + "\" is unknown.");
					}
				}
				else if (command == "bannick")
				{
					while (msgStream.peek() == ' ')
						msgStream.get();
					string playerRegex(msgStream.str().substr(msgStream.tellg()));
					if (!playerRegex.empty())
					{
						GetLobbyThread().GetBanManager().BanPlayerRegex(playerRegex);
						m_ircAdminThread->SendChatMessage(nickName + ": The regex \"" + playerRegex + "\" was added to the player ban list.");
					}
				}
				else if (command == "banip")
				{
					while (msgStream.peek() == ' ')
						msgStream.get();
					string ipAddress;
					unsigned durationHours = 12;
					msgStream >> ipAddress;
					while (msgStream.peek() == ' ')
						msgStream.get();
					if (!msgStream.eof())
						msgStream >> durationHours;
					if (!ipAddress.empty())
					{
						ostringstream durationStr;
						durationStr << durationHours;
						GetLobbyThread().GetBanManager().BanIPAddress(ipAddress, durationHours);
						m_ircAdminThread->SendChatMessage(nickName + ": The IP address \"" + ipAddress + "\" was added to the IP address ban list for " + durationStr.str() + (durationHours == 1 ? " hour." : " hours."));
					}
				}
				else if (command == "listban")
				{
					list<string> banList;
					GetLobbyThread().GetBanManager().GetBanList(banList);
					list<string>::const_iterator i = banList.begin();
					list<string>::const_iterator end = banList.end();
					while (i != end)
					{
						m_ircAdminThread->SendChatMessage(*i);
						++i;
					}
					ostringstream banListStream;
					banListStream
						<< nickName << ": Total count of bans: " << banList.size();
					m_ircAdminThread->SendChatMessage(banListStream.str());
				}
				else if (command == "removeban")
				{
					unsigned banId = 0;
					msgStream >> banId;
					if (GetLobbyThread().GetBanManager().UnBan(banId))
						m_ircAdminThread->SendChatMessage(nickName + ": The ban was successfully removed.");
					else
						m_ircAdminThread->SendChatMessage(nickName + ": This ban does not exist.");
				}
				else if (command == "clearban")
				{
					GetLobbyThread().GetBanManager().ClearBanList();
					m_ircAdminThread->SendChatMessage(nickName + ": All ban lists were cleared.");
				}
				else if (command == "stat")
				{
					ServerStats tmpStats = GetLobbyThread().GetStats();
					{
						boost::posix_time::time_duration timeDiff(boost::posix_time::second_clock::local_time() - GetLobbyThread().GetStartTime());
						ostringstream statStream;
						statStream
							<< "Server uptime................ " << timeDiff.hours() / 24 << " days " << timeDiff.hours() % 24 << " hours " << timeDiff.minutes() << " minutes " << timeDiff.seconds() << " seconds";
						m_ircAdminThread->SendChatMessage(statStream.str());
					}
					{
						ostringstream statStream;
						statStream
							<< "Players currently on Server.. " << tmpStats.numberOfPlayersOnServer;
						m_ircAdminThread->SendChatMessage(statStream.str());
					}
					{
						ostringstream statStream;
						statStream
							<< "Games currently open......... " << tmpStats.numberOfGamesOpen;
						m_ircAdminThread->SendChatMessage(statStream.str());
					}
					{
						ostringstream statStream;
						statStream
							<< "Total players ever logged in. " << tmpStats.totalPlayersEverLoggedIn
							<< "    (Max at a time: " << tmpStats.maxPlayersLoggedIn << ")";
						m_ircAdminThread->SendChatMessage(statStream.str());
					}
					{
						ostringstream statStream;
						statStream
							<< "Total games ever open........ " << tmpStats.totalGamesEverCreated
							<< "    (Max at a time: " << tmpStats.maxGamesOpen << ")";
						m_ircAdminThread->SendChatMessage(statStream.str());
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
						m_ircAdminThread->SendChatMessage(nickName + ": Global chat message sent.");
					}
					else
						m_ircAdminThread->SendChatMessage(nickName + ": Invalid message.");
				}
				else if (command == "msg")
				{
					while (msgStream.peek() == ' ')
						msgStream.get();
					string message(msgStream.str().substr(msgStream.tellg()));
					if (!message.empty() && message.size() < MAX_CHAT_TEXT_SIZE)
					{
						GetLobbyThread().SendGlobalMsgBox(message);
						m_ircAdminThread->SendChatMessage(nickName + ": Global message box sent.");
					}
					else
						m_ircAdminThread->SendChatMessage(nickName + ": Invalid message.");
				}
				else
					m_ircAdminThread->SendChatMessage(nickName + ": Invalid command \"" + command + "\".");
			}
		} catch (...)
		{
			m_ircAdminThread->SendChatMessage(nickName + ": Syntax error. Please check the command.");
		}
	}
}

void
ServerAdminBot::SignalIrcError(int errorCode)
{
	LOG_MSG("Admin bot: IRC error " << errorCode << ".");
}

void
ServerAdminBot::SignalIrcServerError(int errorCode)
{
	LOG_MSG("Admin bot: IRC server error " << errorCode << ".");
}

void
ServerAdminBot::Run()
{
	if (m_ircAdminThread)
		m_ircAdminThread->Run();
}

void
ServerAdminBot::Process()
{
	if (m_ircRestartTimer.elapsed().total_seconds() > SERVER_RESTART_IRC_BOT_INTERVAL_SEC)
	{
		if (m_ircAdminThread)
		{
			m_ircAdminThread->SignalTermination();
			if (m_ircAdminThread->Join(NET_ADMIN_IRC_TERMINATE_TIMEOUT_MSEC))
			{
				boost::shared_ptr<IrcThread> tmpIrcThread(new IrcThread(*m_ircAdminThread));
				tmpIrcThread->Run();
				m_ircAdminThread = tmpIrcThread;
			}
		}
		m_ircRestartTimer.reset();
		m_ircRestartTimer.start();
	}
}

void
ServerAdminBot::SignalTermination()
{
	if (m_ircAdminThread)
		m_ircAdminThread->SignalTermination();
}

bool
ServerAdminBot::Join(bool wait)
{
	bool terminated = true;
	if (m_ircAdminThread)
		terminated = m_ircAdminThread->Join(wait ? NET_ADMIN_IRC_TERMINATE_TIMEOUT_MSEC : 0);
	return terminated;
}

ServerLobbyThread &
ServerAdminBot::GetLobbyThread()
{
	assert(m_lobbyThread.get());
	return *m_lobbyThread;
}

