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
/* Callback interface for irc gui. */

#ifndef _IRCCALLBACK_H_
#define _IRCCALLBACK_H_

#include <string>

class IrcCallback
{
public:
	virtual ~IrcCallback();

	virtual void SignalIrcConnect(const std::string &server) = 0;
	virtual void SignalIrcSelfJoined(const std::string &nickName, const std::string &channel) = 0;
	virtual void SignalIrcPlayerJoined(const std::string &nickName) = 0;
	virtual void SignalIrcPlayerChanged(const std::string &oldNick, const std::string &newNick) = 0;
	virtual void SignalIrcPlayerKicked(const std::string &nickName, const std::string &byWhom, const std::string &reason) = 0;
	virtual void SignalIrcPlayerLeft(const std::string &nickName) = 0;
	virtual void SignalIrcChatMsg(const std::string &nickName, const std::string &msg) = 0;
	virtual void SignalIrcError(int errorCode) = 0;
	virtual void SignalIrcServerError(int errorCode) = 0;
};

#endif
