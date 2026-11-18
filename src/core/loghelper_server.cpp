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

#ifndef POKERTH_DEDICATED_SERVER
#error This file is only for the server.
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4100)
#endif

#include <core/loghelper.h>
#include <core/serverlogqueue.h>
#include <ctime>
#include <fstream>
#include <mutex>
#include <boost/filesystem.hpp>
#include <boost/date_time.hpp>


using namespace std;
using namespace boost::filesystem;
using namespace boost::posix_time;

#define SERVER_MSG_LOG_FILE_NAME				"server_messages.log"

// Bounds of the in-memory copy for the database: enough for a database outage
// of several hours at the usual rate, small enough not to matter if the
// database stays away for good. Beyond either limit the oldest lines go.
#define SERVER_LOG_QUEUE_MAX_LINES				20000
#define SERVER_LOG_QUEUE_MAX_BYTES				(8 * 1024 * 1024)

static string g_logFile;
static int g_logLevel = 1;

namespace
{
std::mutex g_queueMutex;
std::deque<ServerLogLine> g_queue;
size_t g_queueBytes = 0;
unsigned long g_queueDropped = 0;
bool g_queueEnabled = false;

// Caller holds g_queueMutex.
void
trim_queue()
{
	while (!g_queue.empty()
			&& (g_queue.size() > SERVER_LOG_QUEUE_MAX_LINES || g_queueBytes > SERVER_LOG_QUEUE_MAX_BYTES)) {
		g_queueBytes -= g_queue.front().message.size();
		g_queue.pop_front();
		++g_queueDropped;
	}
}

void
queue_line(const char *level, const string &msg)
{
	std::lock_guard<std::mutex> lock(g_queueMutex);
	if (!g_queueEnabled)
		return;
	ServerLogLine line;
	line.time = std::time(NULL);
	line.level = level;
	line.message = msg;
	// The LOG_* macros end every message with a newline; the file needs it,
	// a table row does not.
	while (!line.message.empty() && (line.message.back() == '\n' || line.message.back() == '\r'))
		line.message.pop_back();
	g_queueBytes += line.message.size();
	g_queue.push_back(std::move(line));
	trim_queue();
}
} // namespace

void
serverlog_queue_set_enabled(bool enabled)
{
	std::lock_guard<std::mutex> lock(g_queueMutex);
	g_queueEnabled = enabled;
	if (!enabled) {
		g_queue.clear();
		g_queueBytes = 0;
		g_queueDropped = 0;
	}
}

bool
serverlog_queue_is_enabled()
{
	std::lock_guard<std::mutex> lock(g_queueMutex);
	return g_queueEnabled;
}

bool
serverlog_queue_take(std::deque<ServerLogLine> &out, size_t maxLines, size_t maxBytes, unsigned long &droppedOut)
{
	std::lock_guard<std::mutex> lock(g_queueMutex);
	droppedOut = g_queueDropped;
	g_queueDropped = 0;
	size_t bytes = 0;
	while (!g_queue.empty() && out.size() < maxLines
			&& (out.empty() || bytes + g_queue.front().message.size() <= maxBytes)) {
		bytes += g_queue.front().message.size();
		g_queueBytes -= g_queue.front().message.size();
		out.push_back(std::move(g_queue.front()));
		g_queue.pop_front();
	}
	return !out.empty();
}

void
serverlog_queue_put_back(std::deque<ServerLogLine> &lines)
{
	std::lock_guard<std::mutex> lock(g_queueMutex);
	if (!g_queueEnabled)
		return;
	while (!lines.empty()) {
		g_queueBytes += lines.back().message.size();
		g_queue.push_front(std::move(lines.back()));
		lines.pop_back();
	}
	trim_queue();
}

size_t
serverlog_queue_size()
{
	std::lock_guard<std::mutex> lock(g_queueMutex);
	return g_queue.size();
}

void
loghelper_init(const string &logDir, int logLevel)
{
	path tmpLogFile(logDir);
	tmpLogFile /= SERVER_MSG_LOG_FILE_NAME;

	g_logFile = tmpLogFile.string();
	g_logLevel = logLevel;

#ifdef POKERTH_OFFICIAL_SERVER
	// Collect from the very first line: the database thread decides later
	// whether the lines are wanted, see serverlogqueue.h.
	serverlog_queue_set_enabled(true);
#endif
}

void
loghelper_write_raw(const std::string & /*line*/)
{
	// Client-only diagnostic sink; the server logs via internal_log_* above.
}

void
internal_log_err(const string &msg)
{
	queue_line("ERR", msg);
	if (!g_logFile.empty()) {
		std::ofstream o(g_logFile.c_str(), ios_base::out | ios_base::app);
		if (!o.fail()) {
			o << second_clock::local_time() << " ERR: " << msg;
			o.flush();
		}
	}
}

void
internal_log_msg(const std::string &msg)
{
	if (g_logLevel) {
		queue_line("MSG", msg);
		if (!g_logFile.empty()) {
			std::ofstream o(g_logFile.c_str(), ios_base::out | ios_base::app);
			if (!o.fail())
				o << second_clock::local_time() << " MSG: " << msg;
		}
	}
}

void
internal_log_level(const std::string &msg, int logLevel)
{
	if (g_logLevel >= logLevel) {
		queue_line("OUT", msg);
		if (!g_logFile.empty()) {
			std::ofstream o(g_logFile.c_str(), ios_base::out | ios_base::app);
			if (!o.fail())
				o << second_clock::local_time() << " OUT: " << msg;
		}
	}
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

