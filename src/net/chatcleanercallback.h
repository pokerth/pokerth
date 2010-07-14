/***************************************************************************
 *   Copyright (C) 2009 by Lothar May                                      *
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
/* Callback interface for the chatcleaner. */

#ifndef _CHATCLEANERCALLBACK_H_
#define _CHATCLEANERCALLBACK_H_

#include <string>


class ChatCleanerCallback
{
public:
	virtual ~ChatCleanerCallback();

	virtual void SignalChatBotMessage(const std::string &msg) = 0;
	virtual void SignalKickPlayer(unsigned playerId) = 0;
	virtual void SignalBanPlayer(unsigned playerId) = 0;
};

#endif
