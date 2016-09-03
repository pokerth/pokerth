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
#include <dbofficial/asyncdbgameplace.h>
#include <dbofficial/dbidmanager.h>


using namespace std;


AsyncDBGamePlace::AsyncDBGamePlace(unsigned queryId, const string &preparedName, const list<string> &params)
	: SingleAsyncDBQuery(queryId, preparedName, params)
{
}

AsyncDBGamePlace::~AsyncDBGamePlace()
{
}

void
AsyncDBGamePlace::Init(DBIdManager& idManager)
{
	std::list<std::string> params;
	GetParams(params);
	ostringstream paramStream;
	paramStream << idManager.GetGameDBId(GetId());
	// Add game id as first parameter (according to the order in insert).
	params.push_front(paramStream.str());
	SetParams(params);
}

void
AsyncDBGamePlace::HandleResult(mysqlpp::Query &/*query*/, DBIdManager& /*idManager*/, mysqlpp::StoreQueryResult& /*result*/, boost::asio::io_service &service, ServerDBCallback &cb)
{
	// This query does not produce a result.
	HandleError(service, cb);
}

void
AsyncDBGamePlace::HandleNoResult(mysqlpp::Query &/*query*/, DBIdManager& /*idManager*/, boost::asio::io_service &/*service*/, ServerDBCallback &/*cb*/)
{
	// No action required.
}

void
AsyncDBGamePlace::HandleError(boost::asio::io_service &service, ServerDBCallback &cb)
{
	service.post(boost::bind(&ServerDBCallback::QueryError, &cb, "AsyncDBGamePlace: Failure."));
}
