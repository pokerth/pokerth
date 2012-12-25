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
/* Helper class for logging. */

#ifndef _LOGHELPER_H_
#define _LOGHELPER_H_

#include <string>
#include <sstream>

void loghelper_init(const std::string &logDir, int logLevel);

void internal_log_err(const std::string &msg);
void internal_log_msg(const std::string &msg);
void internal_log_level(const std::string &msg, int logLevel);

#define LOG_ERROR(_e) \
	do \
	{ \
		std::ostringstream outStream; \
		outStream << _e << std::endl; \
		internal_log_err(outStream.str()); \
	} \
	while(false)
#define LOG_MSG(_e) \
	do \
	{ \
		std::ostringstream outStream; \
		outStream << _e << std::endl; \
		internal_log_msg(outStream.str()); \
	} \
	while(false)
#define LOG_VERBOSE(_e) \
	do \
	{ \
		std::ostringstream outStream; \
		outStream << _e << std::endl; \
		internal_log_level(outStream.str(), 2); \
	} \
	while(false)

#endif

