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
/* Server database interface. */

#ifndef _SERVERDBINTERFACE_H_
#define _SERVERDBINTERFACE_H_

#include <db/serverdbcallback.h>
#include <string>
#include <list>

typedef std::list<DB_id> db_list;

class ServerDBInterface
{
public:
	virtual ~ServerDBInterface();

	virtual void Init(const std::string &host, const std::string &user, const std::string &pwd,
					  const std::string &database, const std::string &encryptionKey) = 0;

	virtual void Start() = 0;
	virtual void Stop() = 0;

	virtual void AsyncPlayerLogin(unsigned requestId, const std::string &playerName) = 0;
	virtual void AsyncCheckAvatarBlacklist(unsigned requestId, const std::string &avatarHash) = 0;
	virtual void PlayerPostLogin(DB_id playerId, const std::string &avatarHash, const std::string &avatarType) = 0;
	virtual void PlayerLogout(DB_id playerId) = 0;

	virtual void AsyncCreateGame(unsigned requestId, const std::string &gameName) = 0;
	virtual void SetGamePlayerPlace(unsigned requestId, DB_id playerId, unsigned place) = 0;
	virtual void EndGame(unsigned requestId) = 0;

	virtual void AsyncReportAvatar(unsigned requestId, unsigned replyId, DB_id reportedPlayerId, const std::string &avatarHash, const std::string &avatarType, DB_id *byPlayerId) = 0;
	virtual void AsyncReportGame(unsigned requestId, unsigned replyId, DB_id *creatorPlayerId, unsigned gameId, const std::string &gameName, DB_id *byPlayerId) = 0;

	virtual void AsyncQueryAdminPlayers(unsigned requestId) = 0;
	virtual void AsyncBlockPlayer(unsigned requestId, unsigned replyId, DB_id playerId, int valid, int active) = 0;
};

#endif
