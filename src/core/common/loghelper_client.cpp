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

#ifdef POKERTH_DEDICATED_SERVER
#error This file is only for the client.
#endif

#include <core/loghelper.h>
#include <iostream>


using namespace std;


static int g_logLevel = 1;

void
loghelper_init(const std::string & /*logDir*/, int logLevel)
{
	// Do not log to file as client.
	g_logLevel = logLevel;
}

void
internal_log_err(const string &msg)
{
	cerr << msg;
}

void
internal_log_msg(const std::string &msg)
{
	if (g_logLevel)
		cout << msg;
}

void
internal_log_level(const std::string &msg, int logLevel)
{
	if (g_logLevel >= logLevel)
		cout << msg;
}

