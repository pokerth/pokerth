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

#include <boost/bind.hpp>
#include <dbofficial/asyncdbreportgame.h>
#include <dbofficial/dbidmanager.h>


using namespace std;


AsyncDBReportGame::AsyncDBReportGame(unsigned queryId, unsigned replyId, unsigned gameId, const string &preparedName, const list<string> &params)
	: SingleAsyncDBQuery(queryId, preparedName, params), m_replyId(replyId), m_gameId(gameId)
{
}

AsyncDBReportGame::~AsyncDBReportGame()
{
}

void
AsyncDBReportGame::Init(DBIdManager& idManager)
{
	std::list<std::string> params;
	GetParams(params);
	ostringstream paramStream;
	unsigned tmpGameId = idManager.GetGameDBId(m_gameId);
	if (tmpGameId) {
		paramStream << idManager.GetGameDBId(m_gameId);
		// Add game id as last parameter (where-clause).
		params.push_back(paramStream.str());
	} else {
		params.push_back("NULL");
	}
	SetParams(params);
}

void
AsyncDBReportGame::HandleResult(mysqlpp::Query &/*query*/, DBIdManager& /*idManager*/, mysqlpp::StoreQueryResult& /*result*/, boost::asio::io_service &service, ServerDBCallback &cb)
{
	// This query does not produce a result.
	HandleError(service, cb);
}

void
AsyncDBReportGame::HandleNoResult(mysqlpp::Query &/*query*/, DBIdManager& /*idManager*/, boost::asio::io_service &service, ServerDBCallback &cb)
{
	service.post(boost::bind(&ServerDBCallback::ReportGameSuccess, &cb, GetId(), m_replyId));
}

void
AsyncDBReportGame::HandleError(boost::asio::io_service &service, ServerDBCallback &cb)
{
	service.post(boost::bind(&ServerDBCallback::ReportGameFailed, &cb, GetId(), m_replyId));
}
