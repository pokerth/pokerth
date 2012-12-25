/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
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
/* libcurl transfer helper. */

#ifndef _TRANSFERHELPER_H_
#define _TRANSFERHELPER_H_

#include <boost/shared_ptr.hpp>
#include <string>

struct TransferData;


class TransferHelper
{
public:
	TransferHelper();
	virtual ~TransferHelper();

	// Set the parameters. Does not do any error checking.
	// Throws an exception on failure.
	void Init(const std::string &url, const std::string &targetFileName,
			  const std::string &user = "", const std::string &password = "",
			  size_t filesize = 0, const std::string &httpPost = "");

	// Returns true when done, false should call again.
	// Throws an exception on error.
	bool Process();

	// Close all handles.
	void Cleanup();

	// Get and reset the last (error) message.
	std::string ResetLastMessage();

protected:
	boost::shared_ptr<TransferData> GetData();

	virtual void InternalInit(const std::string &url, const std::string &targetFileName,
							  const std::string &user, const std::string &password,
							  size_t filesize, const std::string &httpPost) = 0;

private:

	boost::shared_ptr<TransferData> m_data;
};

#endif

