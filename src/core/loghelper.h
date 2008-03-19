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
/* Helper class for logging. */

#ifndef _LOGHELPER_H_
#define _LOGHELPER_H_

#include <string>
#include <sstream>

void loghelper_init(const std::string &logDir, int logLevel);

void internal_log_err(const std::string &msg);
void internal_log_msg(const std::string &msg);
void internal_log_level(const std::string &msg, int logLevel);

#define LOG_ERROR(e) \
	do \
	{ \
		std::ostringstream outStream; \
		outStream << e << std::endl; \
		internal_log_err(outStream.str()); \
	} \
	while(false)
#define LOG_MSG(e) \
	do \
	{ \
		std::ostringstream outStream; \
		outStream << e << std::endl; \
		internal_log_msg(outStream.str()); \
	} \
	while(false)
#define LOG_VERBOSE(e) \
	do \
	{ \
		std::ostringstream outStream; \
		outStream << e << std::endl; \
		internal_log_level(outStream.str(), 2); \
	} \
	while(false)

#endif

