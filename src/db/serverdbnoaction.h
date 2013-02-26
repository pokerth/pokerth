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
	virtual void AsyncCheckAvatarBlacklist(unsigned /*requestId*/, const std::string &/*avatarHash*/) {}
	virtual void PlayerPostLogin(DB_id /*playerId*/, const std::string &/*avatarHash*/, const std::string &/*avatarType*/) {}
	virtual void PlayerLogout(DB_id /*playerId*/) {}

	virtual void AsyncCreateGame(unsigned /*requestId*/, const std::string &/*gameName*/) {}
	virtual void SetGamePlayerPlace(unsigned /*requestId*/, DB_id /*playerId*/, unsigned /*place*/) {}
	virtual void EndGame(unsigned /*requestId*/) {}

	virtual void AsyncReportAvatar(unsigned /*requestId*/, unsigned /*replyId*/, DB_id /*reportedPlayerId*/, const std::string &/*avatarHash*/, const std::string &/*avatarType*/, DB_id * /*byPlayerId*/) {}
	virtual void AsyncReportGame(unsigned /*requestId*/, unsigned /*replyId*/, DB_id * /*creatorPlayerId*/, unsigned /*gameId*/, const std::string &/*gameName*/, DB_id * /*byPlayerId*/) {}

	virtual void AsyncQueryAdminPlayers(unsigned /*requestId*/) {}
	virtual void AsyncBlockPlayer(unsigned /*requestId*/, unsigned /*replyId*/, DB_id /*playerId*/, int /*valid*/, int /*active*/) {}
};

#endif // _SERVERDBNOACTION_H_
