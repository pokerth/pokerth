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
/* Callback interface for network client gui. */

#ifndef _CLIENTCALLBACK_H_
#define _CLIENTCALLBACK_H_

#include <string>
#include <boost/shared_ptr.hpp>

class Game;

class ClientCallback
{
public:
	virtual ~ClientCallback();

	virtual void SignalNetClientConnect(int actionID) = 0;
	virtual void SignalNetClientGameInfo(int actionID) = 0;
	virtual void SignalNetClientError(int errorID, int osErrorID) = 0;

	virtual void SignalNetClientGameStart(boost::shared_ptr<Game> game) = 0;
	virtual void SignalNetClientPlayerJoined(const std::string &playerName) = 0;
	virtual void SignalNetClientPlayerLeft(const std::string &playerName) = 0;

	virtual void SignalNetClientChatMsg(const std::string &playerName, const std::string &msg) = 0;
};

#endif
