/* 
 * Copyright (C) 2004 Georgy Yunaev tim@krasnogorsk.ru
 *
 * This library is free software; you can redistribute it and/or modify it 
 * under the terms of the GNU Lesser General Public License as published by 
 * the Free Software Foundation; either version 2 of the License, or (at your 
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public 
 * License for more details.
 *
 * $Id: errors.c 2 2004-09-08 11:49:02Z gyunaev $
 */

static const char * libirc_strerror[LIBIRC_ERR_MAX] = 
{
	"No error",
	"Invalid argument",
	"Host not resolved",
	"Socket error",
	"Could not connect",
	"Remote connection closed",
	"Out of memory",
	"Could not accept new connection",
	"Object not found",
	"Could not DCC send this object",
	"Read error",
	"Write error",
	"Illegal operation for this state",
	"Timeout error",
	"Could not open file",
	"IRC session terminated",
};


int irc_errno (irc_session_t * session)
{
	return session->lasterror;
}


const char * irc_strerror (int ircerrno)
{
	if ( ircerrno >= 0 && ircerrno < LIBIRC_ERR_MAX )
		return libirc_strerror[ircerrno];
	else
		return "Invalid irc_errno value";
}

