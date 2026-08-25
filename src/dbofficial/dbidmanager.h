/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2016 Felix Hammer, Florian Thauer, Lothar May          *
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
/* Manager for database internal ids */

#ifndef _DBIDMANAGER_H_
#define _DBIDMANAGER_H_

#include <db/dbdefs.h>
#include <boost/thread.hpp>
#include <map>

class DBIdManager
{
public:
	void AddGameId(unsigned gameId, DB_id databaseId);
	void RemoveGameId(unsigned gameId);

	DB_id GetGameDBId(unsigned gameId) const;

	// Activity logging. The run id is set once per process, on the first
	// successful database connection; session rows carry it so that a gap in
	// the data can be told apart from a server that was not running.
	void SetServerRunId(DB_id runId);
	DB_id GetServerRunId() const;

	void AddSessionId(unsigned sessionNo, DB_id databaseId);
	DB_id TakeSessionDBId(unsigned sessionNo);

protected:
	typedef std::map<unsigned, DB_id> DBMap;

private:
	DBMap					m_gameIdMap;
	mutable boost::mutex	m_gameIdMapMutex;

	DBMap					m_sessionIdMap;
	mutable boost::mutex	m_sessionIdMapMutex;
	DB_id					m_serverRunId{DB_ID_INVALID};
	mutable boost::mutex	m_serverRunIdMutex;
};

#endif // _DBIDMANAGER_H_
