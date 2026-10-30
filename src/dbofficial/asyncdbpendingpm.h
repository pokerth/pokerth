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

#ifndef _ASYNCDBPENDINGPM_H_
#define _ASYNCDBPENDINGPM_H_

#include <dbofficial/singleasyncdbquery.h>


// Insert or delete held back messages - fire and forget: the messages live in
// the memory of the server, a failed write only costs them after a restart.
class AsyncDBPendingPmUpdate : public SingleAsyncDBQuery
{
public:
	AsyncDBPendingPmUpdate(const std::string &preparedName, const std::list<std::string> &params);
	virtual ~AsyncDBPendingPmUpdate();

	virtual bool Init(DBIdManager& idManager);

	virtual void HandleResult(mysqlpp::Query &query, DBIdManager& idManager, mysqlpp::StoreQueryResult& result, boost::asio::io_context &service, ServerDBCallback &cb);
	virtual void HandleNoResult(mysqlpp::Query &query, DBIdManager& idManager, boost::asio::io_context &service, ServerDBCallback &cb);
	virtual void HandleError(boost::asio::io_context &service, ServerDBCallback &cb);

	virtual bool RequiresResultSet() const
	{
		return false;
	}
};

// Read all held back messages, once after the first connect.
class AsyncDBPendingPmLoad : public SingleAsyncDBQuery
{
public:
	explicit AsyncDBPendingPmLoad(const std::string &preparedName);
	virtual ~AsyncDBPendingPmLoad();

	virtual bool Init(DBIdManager& idManager);

	virtual void HandleResult(mysqlpp::Query &query, DBIdManager& idManager, mysqlpp::StoreQueryResult& result, boost::asio::io_context &service, ServerDBCallback &cb);
	virtual void HandleNoResult(mysqlpp::Query &query, DBIdManager& idManager, boost::asio::io_context &service, ServerDBCallback &cb);
	virtual void HandleError(boost::asio::io_context &service, ServerDBCallback &cb);

	virtual bool RequiresResultSet() const
	{
		return true;
	}
};

// The account id for a name, for messages to players who are offline.
class AsyncDBPlayerIdByName : public SingleAsyncDBQuery
{
public:
	AsyncDBPlayerIdByName(unsigned queryId, const std::string &preparedName, const std::list<std::string> &params);
	virtual ~AsyncDBPlayerIdByName();

	virtual bool Init(DBIdManager& idManager);

	virtual void HandleResult(mysqlpp::Query &query, DBIdManager& idManager, mysqlpp::StoreQueryResult& result, boost::asio::io_context &service, ServerDBCallback &cb);
	virtual void HandleNoResult(mysqlpp::Query &query, DBIdManager& idManager, boost::asio::io_context &service, ServerDBCallback &cb);
	virtual void HandleError(boost::asio::io_context &service, ServerDBCallback &cb);

	virtual bool RequiresResultSet() const
	{
		return true;
	}
};

#endif
