/***************************************************************************
 *   Copyright (C) 2010 by Lothar May                                      *
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
/* Generic server database implementation. Basically does nothing. */

#ifndef _SERVERDBNOACTION_H_
#define _SERVERDBNOACTION_H_

#include <db/serverdbinterface.h>


class ServerDBNoAction : public ServerDBInterface
{
public:
    ServerDBNoAction();
	virtual ~ServerDBNoAction();

	virtual void Init(const std::string &/*host*/, const std::string &/*user*/, const std::string &/*pwd*/,
					  const std::string &/*database*/, const std::string &/*encryptionKey*/) {}

	virtual void Start() {}
	virtual void Stop() {}

	virtual void AsyncPlayerLogin(unsigned /*requestId*/, const std::string &/*playerName*/) {}
	virtual void PlayerPostLogin(DB_id /*playerId*/, const std::string &/*avatarHash*/, const std::string &/*avatarType*/) {}
	virtual void PlayerLogout(DB_id /*playerId*/) {}

	virtual void AsyncCreateGame(unsigned /*requestId*/, const std::string &/*gameName*/) {}
	virtual void SetGamePlayerPlace(DB_id /*gameId*/, DB_id /*playerId*/, unsigned /*place*/) {}
	virtual void EndGame(DB_id /*gameId*/) {}
};

#endif // _SERVERDBNOACTION_H_
