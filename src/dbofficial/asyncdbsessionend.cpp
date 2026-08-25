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
/* Async database query: complete the lobby activity row of a closing session. */

#include <dbofficial/asyncdbsessionend.h>
#include <dbofficial/dbidmanager.h>
#include <core/loghelper.h>


using namespace std;


AsyncDBSessionEnd::AsyncDBSessionEnd(unsigned sessionNo, const string &preparedName, const list<string> &params)
	: SingleAsyncDBQuery(sessionNo, preparedName, params)
{
}

AsyncDBSessionEnd::~AsyncDBSessionEnd()
{
}

bool
AsyncDBSessionEnd::Init(DBIdManager& idManager)
{
	if (m_initDone)
		return true;

	// Normally the insert for this session has already run, because the queue
	// is FIFO and the session was opened before it was closed. It can still be
	// pending though: a deferred insert is requeued at the *back*, so it can
	// end up behind this update. Defer until the row exists. If the insert
	// failed outright the id never arrives and the row stays open, which is
	// what the server_gone cleanup on the next start is for.
	DB_id sessionDbId = idManager.TakeSessionDBId(GetId());
	if (sessionDbId == DB_ID_INVALID)
		return false;

	list<string> params;
	GetParams(params);
	ostringstream paramStream;
	paramStream << sessionDbId;
	// Row id is the last parameter, matching the WHERE clause of the update.
	params.push_back(paramStream.str());
	SetParams(params);

	m_resolvedSessionDbId = sessionDbId;
	m_initDone = true;
	return true;
}

void
AsyncDBSessionEnd::HandleResult(mysqlpp::Query &/*query*/, DBIdManager& /*idManager*/, mysqlpp::StoreQueryResult& /*result*/, boost::asio::io_context &service, ServerDBCallback &cb)
{
	// This query does not produce a result.
	HandleError(service, cb);
}

void
AsyncDBSessionEnd::HandleNoResult(mysqlpp::Query &/*query*/, DBIdManager& /*idManager*/, boost::asio::io_context &/*service*/, ServerDBCallback &/*cb*/)
{
	// No action required.
}

void
AsyncDBSessionEnd::HandleError(boost::asio::io_context &/*service*/, ServerDBCallback &/*cb*/)
{
	LOG_ERROR("AsyncDBSessionEnd: UPDATE failed for session " + std::to_string(GetId())
		+ " (row " + std::to_string(m_resolvedSessionDbId) + ").");
}
