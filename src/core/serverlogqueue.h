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
 *****************************************************************************/
/* Copy of server_messages.log for the official server's database. */

#ifndef _SERVERLOGQUEUE_H_
#define _SERVERLOGQUEUE_H_

#include <ctime>
#include <deque>
#include <string>

// One line of server_messages.log, as handed to the database thread.
struct ServerLogLine {
	std::time_t time;
	// "ERR", "MSG" or "OUT", the tag of the file line.
	const char *level;
	// Without the trailing newline.
	std::string message;
};

// The official server keeps every log line in memory until the database
// thread has written it to server_log (docs/server_log_schema.sql). Only the
// official server build collects anything: in every other build the queue
// stays switched off from the start and all of these are no-ops.
//
// Collecting starts in loghelper_init(), so that the startup lines logged
// before the first database connection are not lost. The database thread then
// switches it on for good once it has seen the table, or off if the table is
// missing.

// Off discards everything queued and stops collecting; on resumes collecting.
void serverlog_queue_set_enabled(bool enabled);
bool serverlog_queue_is_enabled();

// Move up to maxLines lines (and roughly maxBytes of message text, but always
// at least one line) from the queue into out, oldest first. droppedOut is the
// number of lines thrown away because the queue was full since the last call.
// Returns false if there was nothing to take.
bool serverlog_queue_take(std::deque<ServerLogLine> &out, size_t maxLines, size_t maxBytes, unsigned long &droppedOut);

// Put lines taken by serverlog_queue_take() back in front of the queue, e.g.
// after the connection was lost while writing them.
void serverlog_queue_put_back(std::deque<ServerLogLine> &lines);

size_t serverlog_queue_size();

#endif
