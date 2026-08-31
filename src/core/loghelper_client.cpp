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
#include <cstdio>

// The crash handler needs backtrace() and POSIX signal/file calls. bionic
// (Android) has no execinfo.h, Windows has neither - there the handler is a
// no-op and only the [SHUTDOWN] marker distinguishes a crash from a clean exit.
#if (defined(__linux__) && !defined(__ANDROID__)) || defined(__APPLE__)
#define POKERTH_CRASH_HANDLER 1
#include <csignal>
#include <cstring>
#include <execinfo.h>
#include <fcntl.h>
#include <unistd.h>
#endif


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
#ifdef POKERTH_CRASH_HANDLER
// Second, raw handle on the same file, opened O_APPEND alongside the stream.
// The crash handler must not touch g_logFile: neither std::ofstream nor the
// mutex above is async-signal-safe (and the mutex may well be held by the very
// thread that just died), so it writes through this descriptor with write(2).
int g_crashLogFd = -1;
// Own stack for the handler, so a crash caused by stack exhaustion can still
// run it. SIGSTKSZ is not a compile-time constant on newer glibc.
char g_crashStack[65536];
#endif

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
	// than LOG_ROTATE_HOURS, the file is moved aside to "pokerth-debug.log.1" and
	// the session starts a fresh one. A running session is never touched - the log
	// grows for the whole session and is only rotated on the next start once it has
	// aged past the rotation period.
	//
	// Moving instead of truncating is what makes a crash analysable: the client is
	// restarted right after it (to rejoin the table), and that restart used to wipe
	// the very log that held the crash. One kept generation survives it; two files
	// are still a bounded amount of disk.
	std::time_t existingStart = 0;
	if (parseLogStart(path, existingStart) && rotationDue(existingStart)) {
		const std::string previous = path + ".1";
		// Windows: rename() fails if the target exists, so drop it first. If the
		// move fails for any reason we simply keep appending - never lose lines.
		std::remove(previous.c_str());
		std::rename(path.c_str(), previous.c_str());
	}
	g_logFile.open(path.c_str(), std::ios::out | std::ios::app);
	if (g_logFile.is_open()) {
		g_logFileReady = true;
		g_logFile << timestampPrefix()
		          << "==== PokerTH client debug log started ===="
		          << std::endl;
#ifdef POKERTH_CRASH_HANDLER
		g_crashLogFd = ::open(path.c_str(), O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC, 0644);
#endif
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

#ifdef POKERTH_CRASH_HANDLER
namespace {

// write(2) on the raw descriptor - the only file output allowed from a signal
// handler (write/strlen/time/backtrace_symbols_fd are on the POSIX
// async-signal-safe list, the iostream above is not).
void crashWrite(const char *text)
{
	const ssize_t written = ::write(g_crashLogFd, text, std::strlen(text));
	(void)written;   // nothing sensible left to do if even this fails
}

// Unsigned value to decimal or hex; snprintf() is not signal-safe.
void crashWriteNumber(unsigned long long value, unsigned base)
{
	char buffer[32];
	int pos = static_cast<int>(sizeof(buffer));
	buffer[--pos] = '\0';
	if (value == 0)
		buffer[--pos] = '0';
	while (value != 0 && pos > 0) {
		const unsigned digit = static_cast<unsigned>(value % base);
		buffer[--pos] = static_cast<char>(digit < 10 ? '0' + digit : 'a' + digit - 10);
		value /= base;
	}
	crashWrite(buffer + pos);
}

void crashSignalHandler(int sig, siginfo_t *info, void *)
{
	if (g_crashLogFd >= 0) {
		// No timestampPrefix() here: localtime_r() and ostringstream are not
		// signal-safe. The epoch seconds pin the entry down well enough next to
		// the timestamped lines above it.
		crashWrite("\n[CRASH] signal ");
		crashWriteNumber(static_cast<unsigned long long>(sig), 10);
		if (info != nullptr) {
			crashWrite(" at 0x");
			crashWriteNumber(reinterpret_cast<unsigned long long>(info->si_addr), 16);
		}
		crashWrite(" epoch ");
		crashWriteNumber(static_cast<unsigned long long>(::time(nullptr)), 10);
		crashWrite("\n[CRASH] backtrace:\n");
		void *frames[64];
		const int count = ::backtrace(frames, static_cast<int>(sizeof(frames) / sizeof(frames[0])));
		// _fd variant: resolves and writes without malloc(), unlike
		// backtrace_symbols() - the heap may be exactly what is broken here.
		::backtrace_symbols_fd(frames, count, g_crashLogFd);
		crashWrite("[CRASH] end of backtrace\n");
		::fsync(g_crashLogFd);
	}
	// Hand the signal back to its default action: SA_RESETHAND has already
	// restored SIG_DFL and SA_NODEFER left the signal unblocked, so this raise()
	// ends the process exactly as it would have without us - core dump included,
	// so coredumpctl still gets its full backtrace.
	::raise(sig);
}

} // namespace
#endif

void
loghelper_install_crash_handler()
{
#ifdef POKERTH_CRASH_HANDLER
	std::lock_guard<std::mutex> lock(logFileMutex());
	if (g_crashLogFd < 0)
		return;   // no log file (logging off, LogDir unusable) - nothing to write to

	// Alternate stack: a stack overflow raises SIGSEGV with no room left for the
	// handler's own frame; with SA_ONSTACK it runs on g_crashStack instead.
	stack_t altStack{};
	altStack.ss_sp = g_crashStack;
	altStack.ss_size = sizeof(g_crashStack);
	altStack.ss_flags = 0;
	::sigaltstack(&altStack, nullptr);

	struct sigaction action{};
	action.sa_sigaction = crashSignalHandler;
	// No :: here - on macOS sigemptyset() is a macro, which cannot be qualified.
	sigemptyset(&action.sa_mask);
	action.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESETHAND | SA_NODEFER;
	// SIGABRT covers qFatal()/abort() and failed assertions, the rest are the
	// hardware faults.
	for (int sig : {SIGSEGV, SIGABRT, SIGBUS, SIGILL, SIGFPE})
		::sigaction(sig, &action, nullptr);
#endif
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

