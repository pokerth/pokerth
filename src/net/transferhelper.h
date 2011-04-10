/***************************************************************************
 *   Copyright (C) 2009 by Lothar May                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
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
			  const std::string &user = "", const std::string &password = "", size_t filesize = 0);

	// Returns true when done, false should call again.
	// Throws an exception on error.
	bool Process();

	// Close all handles.
	void Cleanup();

protected:
	boost::shared_ptr<TransferData> GetData();

	virtual void InternalInit(const std::string &url, const std::string &targetFileName,
							  const std::string &user, const std::string &password, size_t filesize) = 0;

private:

	boost::shared_ptr<TransferData> m_data;
};

#endif

