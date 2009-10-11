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
/* Server database callback for async database operations. */

#ifndef _SERVERDBCALLBACK_H_
#define _SERVERDBCALLBACK_H_

#include <string>

typedef unsigned DB_id;
#define DB_ID_INVALID 0

// Callback operations are posted using the io service,
// and will therefore be executed in the io service thread.
class ServerDBCallback
{
public:
	virtual ~ServerDBCallback();

	virtual void ConnectSuccess() = 0;
	virtual void ConnectFailed(const std::string &error) = 0;

	virtual void QueryError(const std::string &error) = 0;

	virtual void PlayerLoginSuccess(unsigned requestId, DB_id playerId) = 0;
	virtual void PlayerLoginFailed(unsigned requestId) = 0;

	virtual void CreateGameSuccess(unsigned requestId, DB_id gameId) = 0;
	virtual void CreateGameFailed(unsigned requestId) = 0;
};

#endif
