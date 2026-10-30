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
/* Async database queries for held back private messages. */

#include <boost/bind/bind.hpp>
#include <dbofficial/asyncdbpendingpm.h>
#include <dbofficial/dbidmanager.h>
#include <core/loghelper.h>
#include <cstdlib>

using namespace std;


AsyncDBPendingPmUpdate::AsyncDBPendingPmUpdate(const string &preparedName, const list<string> &params)
	: SingleAsyncDBQuery(0, preparedName, params)
{
}

AsyncDBPendingPmUpdate::~AsyncDBPendingPmUpdate()
{
}

bool
AsyncDBPendingPmUpdate::Init(DBIdManager& /*idManager*/)
{
	return true;
}

void
AsyncDBPendingPmUpdate::HandleResult(mysqlpp::Query &/*query*/, DBIdManager& /*idManager*/, mysqlpp::StoreQueryResult& /*result*/, boost::asio::io_context &service, ServerDBCallback &cb)
{
	// This query does not produce a result.
	HandleError(service, cb);
}

void
AsyncDBPendingPmUpdate::HandleNoResult(mysqlpp::Query &/*query*/, DBIdManager& /*idManager*/, boost::asio::io_context &/*service*/, ServerDBCallback &/*cb*/)
{
}

void
AsyncDBPendingPmUpdate::HandleError(boost::asio::io_context &/*service*/, ServerDBCallback &/*cb*/)
{
	LOG_ERROR("AsyncDBPendingPmUpdate: " << GetPreparedName() << " failed.");
}


AsyncDBPendingPmLoad::AsyncDBPendingPmLoad(const string &preparedName)
	: SingleAsyncDBQuery(0, preparedName, list<string>())
{
}

AsyncDBPendingPmLoad::~AsyncDBPendingPmLoad()
{
}

bool
AsyncDBPendingPmLoad::Init(DBIdManager& /*idManager*/)
{
	return true;
}

void
AsyncDBPendingPmLoad::HandleResult(mysqlpp::Query &/*query*/, DBIdManager& /*idManager*/, mysqlpp::StoreQueryResult& result, boost::asio::io_context &service, ServerDBCallback &cb)
{
	list<DBPendingPrivateMessage> messages;
	for (size_t i = 0; i < result.num_rows(); ++i) {
		DBPendingPrivateMessage msg;
		msg.id = strtoull(static_cast<string>(result[i][0]).c_str(), NULL, 10);
		msg.recipientId = result[i][1];
		msg.senderId = result[i][2];
		result[i][3].to_string(msg.senderName);
		result[i][4].to_string(msg.senderCountry);
		msg.ageSec = result[i][5];
		result[i][6].to_string(msg.text);
		messages.push_back(msg);
	}
	boost::asio::post(service, boost::bind(&ServerDBCallback::PendingPrivateMessagesLoaded, &cb, messages));
}

void
AsyncDBPendingPmLoad::HandleNoResult(mysqlpp::Query &/*query*/, DBIdManager& /*idManager*/, boost::asio::io_context &service, ServerDBCallback &cb)
{
	// This query should always produce a result.
	HandleError(service, cb);
}

void
AsyncDBPendingPmLoad::HandleError(boost::asio::io_context &/*service*/, ServerDBCallback &/*cb*/)
{
	// Deliberately no callback: without the stored messages the server does
	// not hold back new ones either, it rejects them as before.
	LOG_ERROR("AsyncDBPendingPmLoad: loading held back private messages failed.");
}


AsyncDBPlayerIdByName::AsyncDBPlayerIdByName(unsigned queryId, const string &preparedName, const list<string> &params)
	: SingleAsyncDBQuery(queryId, preparedName, params)
{
}

AsyncDBPlayerIdByName::~AsyncDBPlayerIdByName()
{
}

bool
AsyncDBPlayerIdByName::Init(DBIdManager& /*idManager*/)
{
	return true;
}

void
AsyncDBPlayerIdByName::HandleResult(mysqlpp::Query &/*query*/, DBIdManager& /*idManager*/, mysqlpp::StoreQueryResult& result, boost::asio::io_context &service, ServerDBCallback &cb)
{
	if (result.num_rows() != 1) {
		HandleError(service, cb);
		return;
	}
	const DB_id playerId = result[0][0];
	string playerName;
	result[0][1].to_string(playerName);
	boost::asio::post(service, boost::bind(&ServerDBCallback::PlayerIdByName, &cb, GetId(), playerId, playerName));
}

void
AsyncDBPlayerIdByName::HandleNoResult(mysqlpp::Query &/*query*/, DBIdManager& /*idManager*/, boost::asio::io_context &service, ServerDBCallback &cb)
{
	// This query should always produce a result.
	HandleError(service, cb);
}

void
AsyncDBPlayerIdByName::HandleError(boost::asio::io_context &service, ServerDBCallback &cb)
{
	// A reply in any case, the waiting message has to be rejected.
	boost::asio::post(service, boost::bind(&ServerDBCallback::PlayerIdByName, &cb, GetId(), DB_ID_INVALID, string()));
}
