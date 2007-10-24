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

#ifndef _WIN32
	#ifdef POKERTH_DEDICATED_SERVER
		#include <sstream>
		#include <syslog.h>
		#include <stdarg.h>
		#define LOG_ERROR(e) \
			do \
			{ \
				std::ostringstream outStream; \
				outStream << e << std::endl; \
				syslog(LOG_ERR, "%s", outStream.str().c_str()); \
			} \
			while(false)
		#define LOG_MSG(e) \
			do \
			{ \
				std::ostringstream outStream; \
				outStream << e << std::endl; \
				syslog(LOG_INFO, "%s", outStream.str().c_str()); \
			} \
			while(false)
	#endif
#endif

#ifndef LOG_ERROR
	#include <iostream>
	#define LOG_ERROR(e) \
		std::cout << e << std::endl
	#define LOG_MSG(e) \
		std::cout << e << std::endl
#endif

#endif

