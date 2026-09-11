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
/* Async database query: refresh the single live lobby statistics row. */

#include <dbofficial/asyncdblivestats.h>
#include <dbofficial/dbidmanager.h>
#include <core/loghelper.h>


using namespace std;


AsyncDBLiveStats::AsyncDBLiveStats(const string &preparedName, const list<string> &params)
// The row is a singleton, so there is no id to address it by - the
// statement names it itself.
	: SingleAsyncDBQuery(0, preparedName, params)
{
}

AsyncDBLiveStats::~AsyncDBLiveStats()
{
}

bool
AsyncDBLiveStats::Init(DBIdManager& idManager)
{
	// Idempotent: Init() runs again when the query is retried after a
	// transient connection loss, and the run id must not be prepended twice.
	if (m_initDone)
		return true;

	// Unlike a session row this one is never deferred: the run id only tells
	// the reader which process wrote the snapshot, and a heartbeat which
	// arrives late is worth less than one which arrives without it.
	DB_id runId = idManager.GetServerRunId();

	list<string> params;
	GetParams(params);
	// Run id is the first parameter of the upsert.
	if (runId == DB_ID_INVALID) {
		params.push_front("NULL");
	} else {
		ostringstream paramStream;
		paramStream << runId;
		params.push_front(paramStream.str());
	}
	SetParams(params);

	m_initDone = true;
	return true;
}

void
AsyncDBLiveStats::HandleResult(mysqlpp::Query &/*query*/, DBIdManager& /*idManager*/, mysqlpp::StoreQueryResult& /*result*/, boost::asio::io_context &service, ServerDBCallback &cb)
{
	// This query does not produce a result.
	HandleError(service, cb);
}

void
AsyncDBLiveStats::HandleNoResult(mysqlpp::Query &/*query*/, DBIdManager& /*idManager*/, boost::asio::io_context &/*service*/, ServerDBCallback &/*cb*/)
{
	// No action required.
}

void
AsyncDBLiveStats::HandleError(boost::asio::io_context &/*service*/, ServerDBCallback &/*cb*/)
{
	// One lost heartbeat only means the website shows a snapshot which is a
	// minute older; the next tick overwrites it anyway.
	LOG_ERROR("AsyncDBLiveStats: live statistics upsert failed.");
}
