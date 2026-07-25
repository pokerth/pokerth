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

#ifdef POKERTH_DEDICATED_SERVER
#error This file is only for the client.
#endif

#include <core/loghelper.h>
#include <iostream>
#include <fstream>
#include <mutex>
#include <thread>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>


using namespace std;


static int g_logLevel = 1;

namespace {

// Guards both g_logFile and g_logFileReady. Accessed from the GUI thread (Qt
// message handler / QML console.log) and the network thread (engine LOG_*).
std::mutex &logFileMutex()
{
	static std::mutex m;
	return m;
}
std::ofstream g_logFile;
bool g_logFileReady = false;

// "2026-06-14 21:03:11.482 [t140123…] " – the thread id is the important bit:
// it lets us see the GUI-thread vs network-thread interleaving that causes the
// end-of-round SQLite flush to block.
std::string timestampPrefix()
{
	using namespace std::chrono;
	const auto now = system_clock::now();
	const auto t = system_clock::to_time_t(now);
	const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
	std::tm tmv{};
#ifdef _WIN32
	localtime_s(&tmv, &t);
#else
	localtime_r(&t, &tmv);
#endif
	std::ostringstream os;
	os << std::put_time(&tmv, "%Y-%m-%d %H:%M:%S")
	   << '.' << std::setw(3) << std::setfill('0') << ms.count()
	   << " [t" << std::this_thread::get_id() << "] ";
	return os.str();
}

// Rotation period: wipe the debug log once its first entry is this old, so it
// cannot grow without bound.
constexpr int LOG_ROTATE_HOURS = 48;

// Parse the log's first-entry time (written by timestampPrefix() in local time,
// so it is parsed back the same way). Reads only the leading
// "YYYY-MM-DD HH:MM:SS". Returns false on a missing/short/corrupt first line, in
// which case the caller keeps appending (never wipe on doubt).
bool parseLogStart(const std::string &path, std::time_t &out)
{
	std::ifstream in(path.c_str());
	if (!in.good())
		return false;
	std::string firstLine;
	if (!std::getline(in, firstLine) || firstLine.size() < 19)
		return false;
	std::tm tmv{};
	std::istringstream ts(firstLine.substr(0, 19));
	ts >> std::get_time(&tmv, "%Y-%m-%d %H:%M:%S");
	if (ts.fail())
		return false;
	tmv.tm_isdst = -1; // let mktime resolve DST for the local time we parsed
	const std::time_t start = std::mktime(&tmv);
	if (start == static_cast<std::time_t>(-1))
		return false;
	out = start;
	return true;
}

// True once the log's first entry has aged past LOG_ROTATE_HOURS.
bool rotationDue(std::time_t periodStart)
{
	return periodStart != 0
	       && std::difftime(std::time(nullptr), periodStart) >= LOG_ROTATE_HOURS * 3600.0;
}

} // namespace

void
loghelper_init(const std::string &logDir, int logLevel)
{
	g_logLevel = logLevel;

	std::lock_guard<std::mutex> lock(logFileMutex());
	if (g_logFileReady || logDir.empty())
		return;

	const char last = logDir.back();
	const std::string sep = (last == '/' || last == '\\') ? "" : "/";
	const std::string path = logDir + sep + "pokerth-debug.log";
	// Rotate only here, at app start: if an existing log's first entry is older
	// than LOG_ROTATE_HOURS, truncate instead of appending. A running session is
	// never touched - the log grows for the whole session and is only cleared on
	// the next start once it has aged past the rotation period.
	std::time_t existingStart = 0;
	const bool rotate = parseLogStart(path, existingStart) && rotationDue(existingStart);
	const std::ios::openmode mode = rotate
		? (std::ios::out | std::ios::trunc)
		: (std::ios::out | std::ios::app);
	g_logFile.open(path.c_str(), mode);
	if (g_logFile.is_open()) {
		g_logFileReady = true;
		g_logFile << timestampPrefix()
		          << "==== PokerTH client debug log started ===="
		          << std::endl;
	}
}

void
loghelper_write_raw(const std::string &line)
{
	std::lock_guard<std::mutex> lock(logFileMutex());
	if (!g_logFileReady)
		return;
	// The LOG_* macros append a trailing std::endl; strip it so we control the
	// line break (and don't get blank lines in the file).
	std::string msg = line;
	while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r'))
		msg.pop_back();
	g_logFile << timestampPrefix() << msg << std::endl; // endl → flush, so a
	                                                    // freeze still leaves the
	                                                    // last line on disk.
}

void
internal_log_err(const string &msg)
{
	cerr << msg;
	loghelper_write_raw("[ERR] " + msg);
}

void
internal_log_msg(const std::string &msg)
{
	if (g_logLevel)
		cout << msg;
	loghelper_write_raw("[MSG] " + msg);
}

void
internal_log_level(const std::string &msg, int logLevel)
{
	if (g_logLevel >= logLevel)
		cout << msg;
	loghelper_write_raw("[V" + std::to_string(logLevel) + "] " + msg);
}

