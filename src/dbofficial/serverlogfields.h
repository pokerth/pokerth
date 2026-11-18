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
/* Filter columns of server_log, taken from the text of a log line. */

#ifndef _SERVERLOGFIELDS_H_
#define _SERVERLOGFIELDS_H_

#include <string>

// What docs/server_log_schema.sql describes for the columns next to the
// message. An empty string means "not in this line" and becomes NULL.
struct ServerLogFields {
	std::string sessionNo;
	std::string gameId;
	std::string playerUid;
	std::string playerId;
	std::string playerName;
	// Canonical text form, IPv4-mapped IPv6 already turned into IPv4.
	std::string ip;
};

ServerLogFields ParseServerLogFields(const std::string &message);

// Valid UTF-8 of at most maxChars code points: invalid bytes become U+FFFD,
// control characters other than tab and newline become a space.
std::string SanitizeServerLogText(const std::string &text, size_t maxChars);

#endif
