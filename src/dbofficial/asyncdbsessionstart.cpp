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
/* Async database query: open a lobby activity row for a new session. */

#include <dbofficial/asyncdbsessionstart.h>
#include <dbofficial/dbidmanager.h>
#include <core/loghelper.h>


using namespace std;


AsyncDBSessionStart::AsyncDBSessionStart(unsigned sessionNo, const string &preparedName, const list<string> &params)
	: SingleAsyncDBQuery(sessionNo, preparedName, params)
{
}

AsyncDBSessionStart::~AsyncDBSessionStart()
{
}

bool
AsyncDBSessionStart::Init(DBIdManager& idManager)
{
	// Idempotent: Init() runs again when the query is retried after a
	// transient connection loss, and the run id must not be prepended twice.
	if (m_initDone)
		return true;

	DB_id runId = idManager.GetServerRunId();
	if (runId == DB_ID_INVALID) {
		// The run row is written on the first successful database connection.
		// Guests can reach the lobby before that, so defer instead of losing
		// the row.
		return false;
	}

	list<string> params;
	GetParams(params);
	ostringstream paramStream;
	paramStream << runId;
	// Run id is the first column of the insert.
	params.push_front(paramStream.str());
	SetParams(params);

	m_initDone = true;
	return true;
}

void
AsyncDBSessionStart::HandleResult(mysqlpp::Query &/*query*/, DBIdManager& /*idManager*/, mysqlpp::StoreQueryResult& /*result*/, boost::asio::io_context &service, ServerDBCallback &cb)
{
	// This query does not produce a result.
	HandleError(service, cb);
}

void
AsyncDBSessionStart::HandleNoResult(mysqlpp::Query &query, DBIdManager& idManager, boost::asio::io_context &/*service*/, ServerDBCallback &/*cb*/)
{
	query.reset();
	query << "SELECT LAST_INSERT_ID()";
	mysqlpp::StoreQueryResult tmpResult = query.store();
	if (!tmpResult || tmpResult.num_rows() != 1) {
		LOG_ERROR("AsyncDBSessionStart: LAST_INSERT_ID() failed for session "
			+ std::to_string(GetId()) + " (error: " + query.error() + ").");
		return;
	}
	DB_id insertId = tmpResult[0][0];
	if (insertId == 0) {
		LOG_ERROR("AsyncDBSessionStart: no insert id for session " + std::to_string(GetId()) + ".");
		return;
	}
	// Without this the closing UPDATE has no row to address, so the session
	// would stay open in the database forever.
	idManager.AddSessionId(GetId(), insertId);
}

void
AsyncDBSessionStart::HandleError(boost::asio::io_context &/*service*/, ServerDBCallback &/*cb*/)
{
	// Activity logging is a side channel: a failed insert is worth a log line,
	// but must not be reported back as a session error.
	LOG_ERROR("AsyncDBSessionStart: INSERT failed for session " + std::to_string(GetId()) + ".");
}
