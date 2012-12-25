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

#include <net/serveradminbot.h>
#include <net/ircthread.h>
#include <net/serverlobbythread.h>
#include <net/serverbanmanager.h>
#include <net/serverexception.h>
#include <net/socket_msg.h>
#include <net/socket_startup.h>
#include <core/loghelper.h>

#include <boost/filesystem.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string/predicate.hpp>

#define SERVER_RESTART_IRC_BOT_INTERVAL_SEC			86400	// 1 day
#define SERVER_NOTIFY_IRC_BOT_INTERVAL_SEC			1
#define SERVER_CHECK_IRC_BOT_INTERVAL_SEC			60

using namespace std;
using namespace boost::filesystem;

ServerAdminBot::ServerAdminBot(boost::shared_ptr<boost::asio::io_service> ioService)
	: m_notifyTimeoutMinutes(0), m_notifyIntervalMinutes(0), m_notifyCounter(0),
	  m_notifyTimer(boost::posix_time::time_duration(0, 0, 0), boost::timers::portable::second_timer::manual_start),
	  m_reconnectTimer(*ioService), m_notifyLoopTimer(*ioService), m_checkFileTimer(*ioService)
{
}

ServerAdminBot::~ServerAdminBot()
{
}

void
ServerAdminBot::Init(boost::shared_ptr<ServerLobbyThread> lobbyThread, boost::shared_ptr<IrcThread> ircAdminThread, const std::string &cacheDir)
{
	m_lobbyThread = lobbyThread;
	m_ircAdminThread = ircAdminThread;
	m_cacheDir = cacheDir;
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
	if (m_ircAdminThread) {
		try {
			istringstream msgStream(msg);
			string target;
			msgStream >> target;
			if (boost::algorithm::iequals(target, m_ircNick + ":")) {
				string command;
				msgStream >> command;
				if (command == "kick") {
					while (msgStream.peek() == ' ')
						msgStream.get();
					string playerName(msgStream.str().substr(msgStream.tellg()));
					if (!playerName.empty()) {
						if (GetLobbyThread().KickPlayerByName(playerName))
							m_ircAdminThread->SendChatMessage(nickName + ": Successfully kicked player \"" + playerName + "\" from the server.");
						else
							m_ircAdminThread->SendChatMessage(nickName + ": Player \"" + playerName + "\" was not found on the server.");
					}
				} else if (command == "removegame") {
					while (msgStream.peek() == ' ')
						msgStream.get();
					string playerName(msgStream.str().substr(msgStream.tellg()));
					if (!playerName.empty()) {
						if (GetLobbyThread().RemoveGameByPlayerName(playerName))
							m_ircAdminThread->SendChatMessage(nickName + ": Successfully removed game of player \"" + playerName + "\".");
						else
							m_ircAdminThread->SendChatMessage(nickName + ": Player \"" + playerName + "\" is currently not playing a game.");
					}
				} else if (command == "cleaner-reconnect") {
					GetLobbyThread().ReconnectChatBot();
					m_ircAdminThread->SendChatMessage(nickName + ": Cleaner bot reconnect initiated.");
				} else if (command == "showip") {
					while (msgStream.peek() == ' ')
						msgStream.get();
					string playerName(msgStream.str().substr(msgStream.tellg()));
					if (!playerName.empty()) {
						string ipAddress(GetLobbyThread().GetPlayerIPAddress(playerName));
						if (!ipAddress.empty())
							m_ircAdminThread->SendChatMessage(nickName + ": The IP address of player \"" + playerName + "\" is: \"" + ipAddress + "\"");
						else
							m_ircAdminThread->SendChatMessage(nickName + ": The IP address of player \"" + playerName + "\" is unknown.");
					}
				} else if (command == "bannick") {
					while (msgStream.peek() == ' ')
						msgStream.get();
					string playerRegex(msgStream.str().substr(msgStream.tellg()));
					if (!playerRegex.empty()) {
						GetLobbyThread().GetBanManager().BanPlayerRegex(playerRegex);
						m_ircAdminThread->SendChatMessage(nickName + ": The regex \"" + playerRegex + "\" was added to the player ban list.");
					}
				} else if (command == "banip") {
					while (msgStream.peek() == ' ')
						msgStream.get();
					string ipAddress;
					unsigned durationHours = 12;
					msgStream >> ipAddress;
					while (msgStream.peek() == ' ')
						msgStream.get();
					if (!msgStream.eof())
						msgStream >> durationHours;
					if (!ipAddress.empty()) {
						ostringstream durationStr;
						durationStr << durationHours;
						GetLobbyThread().GetBanManager().BanIPAddress(ipAddress, durationHours);
						m_ircAdminThread->SendChatMessage(nickName + ": The IP address \"" + ipAddress + "\" was added to the IP address ban list for " + durationStr.str() + (durationHours == 1 ? " hour." : " hours."));
					}
				} else if (command == "listban") {
					list<string> banList;
					GetLobbyThread().GetBanManager().GetBanList(banList);
					list<string>::const_iterator i = banList.begin();
					list<string>::const_iterator end = banList.end();
					while (i != end) {
						m_ircAdminThread->SendChatMessage(*i);
						++i;
					}
					ostringstream banListStream;
					banListStream
							<< nickName << ": Total count of bans: " << banList.size();
					m_ircAdminThread->SendChatMessage(banListStream.str());
				} else if (command == "removeban") {
					unsigned banId = 0;
					msgStream >> banId;
					if (GetLobbyThread().GetBanManager().UnBan(banId))
						m_ircAdminThread->SendChatMessage(nickName + ": The ban was successfully removed.");
					else
						m_ircAdminThread->SendChatMessage(nickName + ": This ban does not exist.");
				} else if (command == "clearban") {
					GetLobbyThread().GetBanManager().ClearBanList();
					m_ircAdminThread->SendChatMessage(nickName + ": All ban lists were cleared.");
				} else if (command == "stat") {
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
				} else if (command == "chat") {
					while (msgStream.peek() == ' ')
						msgStream.get();
					string chat(msgStream.str().substr(msgStream.tellg()));
					if (!chat.empty() && chat.size() < MAX_CHAT_TEXT_SIZE) {
						GetLobbyThread().SendGlobalChat(chat);
						m_ircAdminThread->SendChatMessage(nickName + ": Global chat message sent.");
					} else
						m_ircAdminThread->SendChatMessage(nickName + ": Invalid message.");
				} else if (command == "msg") {
					while (msgStream.peek() == ' ')
						msgStream.get();
					string message(msgStream.str().substr(msgStream.tellg()));
					if (!message.empty() && message.size() < MAX_CHAT_TEXT_SIZE) {
						GetLobbyThread().SendGlobalMsgBox(message);
						m_ircAdminThread->SendChatMessage(nickName + ": Global message box sent.");
					} else
						m_ircAdminThread->SendChatMessage(nickName + ": Invalid message.");
				} else if (command == "notify-restart") {
					int timeoutMinutes = 0;
					int intervalMinutes = 0;
					msgStream >> timeoutMinutes;
					msgStream >> intervalMinutes;
					if (timeoutMinutes && intervalMinutes) {
						ostringstream ack;
						ack << nickName << ": Restart planned in " << timeoutMinutes << " minutes, notifying every " << intervalMinutes << " minutes.";
						m_ircAdminThread->SendChatMessage(ack.str());
						boost::mutex::scoped_lock lock(m_notifyMutex);
						m_notifyTimeoutMinutes = timeoutMinutes;
						m_notifyIntervalMinutes = intervalMinutes;
						m_notifyCounter = 0;
						m_notifyTimer.reset();
						m_notifyTimer.start();
					} else {
						m_ircAdminThread->SendChatMessage(nickName + ": Invalid parameters.");
					}

				} else
					m_ircAdminThread->SendChatMessage(nickName + ": Invalid command \"" + command + "\".");
			}
		} catch (...) {
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
	if (m_ircAdminThread) {
		// Initialise the timers.
		m_reconnectTimer.expires_from_now(
			boost::posix_time::seconds(SERVER_RESTART_IRC_BOT_INTERVAL_SEC));
		m_reconnectTimer.async_wait(
			boost::bind(
				&ServerAdminBot::ReconnectHandler, shared_from_this(), boost::asio::placeholders::error));
		m_notifyLoopTimer.expires_from_now(
			boost::posix_time::seconds(SERVER_NOTIFY_IRC_BOT_INTERVAL_SEC));
		m_notifyLoopTimer.async_wait(
			boost::bind(
				&ServerAdminBot::NotifyLoop, shared_from_this(), boost::asio::placeholders::error));
		m_checkFileTimer.expires_from_now(
			boost::posix_time::seconds(SERVER_CHECK_IRC_BOT_INTERVAL_SEC));
		m_checkFileTimer.async_wait(
			boost::bind(
				&ServerAdminBot::CheckFileHandler, shared_from_this(), boost::asio::placeholders::error));

		m_ircAdminThread->Run();
	}
}

void
ServerAdminBot::ReconnectHandler(const boost::system::error_code& ec)
{
	if (!ec) {
		Reconnect();

		m_reconnectTimer.expires_from_now(
			boost::posix_time::seconds(SERVER_RESTART_IRC_BOT_INTERVAL_SEC));
		m_reconnectTimer.async_wait(
			boost::bind(
				&ServerAdminBot::ReconnectHandler, shared_from_this(), boost::asio::placeholders::error));
	}
}

void
ServerAdminBot::Reconnect()
{
	if (m_ircAdminThread) {
		m_ircAdminThread->SignalTermination();
		if (m_ircAdminThread->Join(NET_ADMIN_IRC_TERMINATE_TIMEOUT_MSEC)) {
			boost::shared_ptr<IrcThread> tmpIrcThread(new IrcThread(*m_ircAdminThread));
			tmpIrcThread->Run();
			m_ircAdminThread = tmpIrcThread;
		}
	}
}

void
ServerAdminBot::CheckFileHandler(const boost::system::error_code& ec)
{
	if (!ec) {
		// Reconnect the irc bot if a signal file exists.
		path cachePath(m_cacheDir);
		cachePath /= "SignalAdminReconnect";
		if (exists(cachePath)) {
			remove(cachePath);
			Reconnect();
		}
		m_checkFileTimer.expires_from_now(
			boost::posix_time::seconds(SERVER_CHECK_IRC_BOT_INTERVAL_SEC));
		m_checkFileTimer.async_wait(
			boost::bind(
				&ServerAdminBot::CheckFileHandler, shared_from_this(), boost::asio::placeholders::error));
	}
}

void
ServerAdminBot::NotifyLoop(const boost::system::error_code& ec)
{
	if (!ec) {
		boost::mutex::scoped_lock lock(m_notifyMutex);

		if (m_notifyTimeoutMinutes && m_notifyTimer.elapsed().total_seconds() >= m_notifyCounter * m_notifyIntervalMinutes * 60) {
			int remainingMinutes = m_notifyTimeoutMinutes - m_notifyCounter * m_notifyIntervalMinutes;
			++m_notifyCounter;
			if (remainingMinutes > 1) {
				ostringstream notifyStream;
				notifyStream << "The server will be restarted in " << remainingMinutes << " minutes.";
				GetLobbyThread().SendGlobalChat(notifyStream.str());
				notifyStream.str("");
				notifyStream << "Der Server wird in " << remainingMinutes << " Minuten neu gestartet werden.";
				GetLobbyThread().SendGlobalChat(notifyStream.str());
				notifyStream.str("");
				notifyStream << "Le serveur va être redémarré dans " << remainingMinutes << " minutes.";
				GetLobbyThread().SendGlobalChat(notifyStream.str());
				notifyStream.str("");
				notifyStream << "Il server verrà riavviato in " << remainingMinutes << " minuti.";
				GetLobbyThread().SendGlobalChat(notifyStream.str());
				notifyStream.str("");
				notifyStream << "El servidor se reiniciará en " << remainingMinutes << " minutos.";
				GetLobbyThread().SendGlobalChat(notifyStream.str());
			} else if (remainingMinutes == 1) {
				ostringstream notifyStream;
				notifyStream << "The server will be restarted in " << remainingMinutes << " minute.";
				GetLobbyThread().SendGlobalChat(notifyStream.str());
				notifyStream.str("");
				notifyStream << "Der Server wird in " << remainingMinutes << " Minute neu gestartet werden.";
				GetLobbyThread().SendGlobalChat(notifyStream.str());
				notifyStream.str("");
				notifyStream << "Le serveur va être redémarré dans " << remainingMinutes << " minute.";
				GetLobbyThread().SendGlobalChat(notifyStream.str());
				notifyStream.str("");
				notifyStream << "Il server verrà riavviato in " << remainingMinutes << " minuto.";
				GetLobbyThread().SendGlobalChat(notifyStream.str());
				notifyStream.str("");
				notifyStream << "El servidor se reiniciará en " << remainingMinutes << " minuto.";
				GetLobbyThread().SendGlobalChat(notifyStream.str());
			} else {
				GetLobbyThread().SendGlobalChat("The server will be restarted NOW.");
				GetLobbyThread().SendGlobalMsgBox("The server will be restarted NOW.");
				m_notifyTimeoutMinutes = m_notifyCounter = m_notifyIntervalMinutes = 0;
				m_notifyTimer.reset();
			}
		}
		m_notifyLoopTimer.expires_from_now(
			boost::posix_time::seconds(SERVER_NOTIFY_IRC_BOT_INTERVAL_SEC));
		m_notifyLoopTimer.async_wait(
			boost::bind(
				&ServerAdminBot::NotifyLoop, shared_from_this(), boost::asio::placeholders::error));

	}
}

void
ServerAdminBot::SignalTermination()
{
	if (m_ircAdminThread)
		m_ircAdminThread->SignalTermination();

	// Terminated the timers.
	m_notifyLoopTimer.cancel();
	m_reconnectTimer.cancel();
	m_checkFileTimer.cancel();
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

