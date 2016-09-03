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
/* Composite async database query. */

#ifndef _COMPOSITEASYNCDBQUERY_H_
#define _COMPOSITEASYNCDBQUERY_H_

#include <dbofficial/asyncdbquery.h>
#include <dbofficial/querycontext.h>


class CompositeAsyncDBQuery : public AsyncDBQuery, public QueryContext
{
public:
	typedef std::list<boost::shared_ptr<AsyncDBQuery> > AsyncQueryList;
	// Note: This list must contain at least one query.
	CompositeAsyncDBQuery(const AsyncQueryList &queries);
	virtual ~CompositeAsyncDBQuery();

	virtual void Init(DBIdManager& idManager);

	virtual std::string GetPreparedName() const;
	virtual void GetParams(std::list<std::string> &params) const;
	virtual void SetParams(const std::list<std::string> &params);

	virtual void HandleResult(mysqlpp::Query &query, DBIdManager& idManager, mysqlpp::StoreQueryResult& result, boost::asio::io_service &service, ServerDBCallback &cb);
	virtual void HandleNoResult(mysqlpp::Query &query, DBIdManager& idManager, boost::asio::io_service &service, ServerDBCallback &cb);
	// One error will cancel the rest of the composite query.
	virtual void HandleError(boost::asio::io_service &service, ServerDBCallback &cb);

	virtual bool RequiresResultSet() const;
	virtual bool Next();

	virtual unsigned GetLastGameDBId() const;
	virtual void SetLastGameDBId(unsigned id);

private:
	AsyncQueryList				m_list;
	AsyncQueryList::iterator	m_currentQuery;
	bool						m_errorFlag;
	unsigned					m_lastGameDBId;
};

#endif
