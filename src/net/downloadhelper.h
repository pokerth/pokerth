/***************************************************************************
 *   Copyright (C) 2008 by Lothar May                                      *
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
/* libcurl download helper. */

#ifndef _DOWNLOADHELPER_H_
#define _DOWNLOADHELPER_H_

#include <string>
#include <memory>

struct DownloadData;


class DownloadHelper
{
public:
	DownloadHelper();
	virtual ~DownloadHelper();

	// Set the parameters. Does not do any error checking.
	// Throws an exception on failure.
	void Init(const std::string &url, const std::string &targetFileName);

	// Returns true when done, false should call again.
	// Throws an exception on error.
	bool Process();

	// Close all handles.
	void Cleanup();

private:

	std::auto_ptr<DownloadData> m_data;
};

#endif

