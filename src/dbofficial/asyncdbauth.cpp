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
#include <dbofficial/asyncdbauth.h>


using namespace std;


AsyncDBAuth::AsyncDBAuth(unsigned queryId, const string &preparedName, const list<string> &params)
	: SingleAsyncDBQuery(queryId, preparedName, params)
{
}

AsyncDBAuth::~AsyncDBAuth()
{
}

void
AsyncDBAuth::HandleResult(mysqlpp::Query &/*query*/, DBIdManager& /*idManager*/, mysqlpp::StoreQueryResult& result, boost::asio::io_service &service, ServerDBCallback &cb)
{
	if (result.num_rows() != 1) {
		service.post(boost::bind(&ServerDBCallback::PlayerLoginFailed, &cb, GetId()));
	} else {
		int blocked = result[0][2];
		int active = result[0][5];
		if ((active != 1) || (blocked != 0)) {
			service.post(boost::bind(&ServerDBCallback::PlayerLoginBlocked, &cb, GetId()));
		} else {
			mysqlpp::String secret(result[0][1]);
			mysqlpp::String country(result[0][3]);
			mysqlpp::String last_login(result[0][4]);
			boost::shared_ptr<DBPlayerData> tmpData(new DBPlayerData);
			tmpData->id = result[0][0];
			secret.to_string(tmpData->secret);
			if (!country.is_null())
				country.to_string(tmpData->country);
			last_login.to_string(tmpData->last_login);

			service.post(boost::bind(&ServerDBCallback::PlayerLoginSuccess, &cb, GetId(), tmpData));
		}
	}
}

void
AsyncDBAuth::HandleNoResult(mysqlpp::Query &/*query*/, DBIdManager& /*idManager*/, boost::asio::io_service &service, ServerDBCallback &cb)
{
	HandleError(service, cb);
}

void
AsyncDBAuth::HandleError(boost::asio::io_service &service, ServerDBCallback &cb)
{
	service.post(boost::bind(&ServerDBCallback::PlayerLoginFailed, &cb, GetId()));
}
