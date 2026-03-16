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

#include <boost/bind/bind.hpp>
#include <dbofficial/asyncdbupdatescore.h>
#include <dbofficial/dbidmanager.h>


using namespace std;


AsyncDBUpdateScore::AsyncDBUpdateScore(unsigned queryId, const string &preparedName, const std::list<std::string> &params)
	: SingleAsyncDBQuery(queryId, preparedName, params)
{
}

AsyncDBUpdateScore::~AsyncDBUpdateScore()
{
}

bool
AsyncDBUpdateScore::Init(DBIdManager& idManager)
{
	// Guard: Init() must be idempotent because it is called again on
	// retry after a transient DB connection loss.  Without this guard
	// the game-DB-ID would be prepended to the parameter list a second
	// time, corrupting the CALL statement.
	if (m_resolvedGameDbId != DB_ID_INVALID)
		return true;

	DB_id gameDbId = idManager.GetGameDBId(GetId());
	if (gameDbId == DB_ID_INVALID) {
		// CreateGame hasn't completed yet or failed.
		return false;
	}

	std::list<std::string> params;
	GetParams(params);
	ostringstream paramStream;
	paramStream << gameDbId;
	// Add game id as first parameter (param for stored procedure).
	params.push_front(paramStream.str());
	SetParams(params);
	// Cache the resolved DB ID so we can detect invalid retries.
	// Do NOT remove the game ID here — it must survive a connection-loss
	// retry where Init() is called again.  Removal happens in
	// HandleNoResult() after the query has actually succeeded.
	m_resolvedGameDbId = gameDbId;
	return true;
}

void
AsyncDBUpdateScore::HandleResult(mysqlpp::Query &/*query*/, DBIdManager& /*idManager*/, mysqlpp::StoreQueryResult& /*result*/, boost::asio::io_context &service, ServerDBCallback &cb)
{
	// This query does not produce a result.
	HandleError(service, cb);
}

void
AsyncDBUpdateScore::HandleNoResult(mysqlpp::Query &/*query*/, DBIdManager& idManager, boost::asio::io_context &/*service*/, ServerDBCallback &/*cb*/)
{
	// Query succeeded — now it is safe to remove the game ID so that
	// "update score" cannot be called twice for the same game.
	idManager.RemoveGameId(GetId());
}

void
AsyncDBUpdateScore::HandleError(boost::asio::io_context &service, ServerDBCallback &cb)
{
	boost::asio::post(service, boost::bind(&ServerDBCallback::QueryError, &cb,
		"AsyncDBUpdateScore: Failed to update scores for game " + std::to_string(GetId())
		+ " (dbId " + std::to_string(m_resolvedGameDbId) + ")."));
}
