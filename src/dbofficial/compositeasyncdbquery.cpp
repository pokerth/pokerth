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

#include <dbofficial/compositeasyncdbquery.h>


using namespace std;


CompositeAsyncDBQuery::CompositeAsyncDBQuery(const AsyncQueryList &queries)
	: m_list(queries), m_errorFlag(false), m_lastGameDBId(0)
{
	m_currentQuery = m_list.begin();
}

CompositeAsyncDBQuery::~CompositeAsyncDBQuery()
{
}

void
CompositeAsyncDBQuery::Init(DBIdManager& idManager)
{
	(*m_currentQuery)->Init(idManager);
}

std::string
CompositeAsyncDBQuery::GetPreparedName() const
{
	return (*m_currentQuery)->GetPreparedName();
}

void
CompositeAsyncDBQuery::GetParams(std::list<std::string> &params) const
{
	(*m_currentQuery)->GetParams(params);
}

void
CompositeAsyncDBQuery::SetParams(const std::list<std::string> &params)
{
	(*m_currentQuery)->SetParams(params);
}

void
CompositeAsyncDBQuery::HandleResult(mysqlpp::Query &query, DBIdManager& idManager, mysqlpp::StoreQueryResult& result, boost::asio::io_service &service, ServerDBCallback &cb)
{
	(*m_currentQuery)->HandleResult(query, idManager, result, service, cb);
}

void
CompositeAsyncDBQuery::HandleNoResult(mysqlpp::Query &query, DBIdManager& idManager, boost::asio::io_service &service, ServerDBCallback &cb)
{
	(*m_currentQuery)->HandleNoResult(query, idManager, service, cb);
}

void
CompositeAsyncDBQuery::HandleError(boost::asio::io_service &service, ServerDBCallback &cb)
{
	(*m_currentQuery)->HandleError(service, cb);
	m_errorFlag = true;
}

bool
CompositeAsyncDBQuery::RequiresResultSet() const
{
	return (*m_currentQuery)->RequiresResultSet();
}

bool
CompositeAsyncDBQuery::Next()
{
	bool retVal = false;
	AsyncQueryList::iterator pos = m_currentQuery;
	++pos;
	if (!m_errorFlag && pos != m_list.end()) {
		m_currentQuery = pos;
		retVal = true;
	}
	return retVal;
}

unsigned
CompositeAsyncDBQuery::GetLastGameDBId() const
{
	return m_lastGameDBId;
}

void
CompositeAsyncDBQuery::SetLastGameDBId(unsigned id)
{
	m_lastGameDBId = id;
}
