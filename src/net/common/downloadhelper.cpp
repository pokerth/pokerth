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

#include <net/socket_helper.h>
#include <net/downloadhelper.h>
#include <net/netexception.h>
#include <net/socket_msg.h>
#include <net/transferdata.h>

#include <cstdio>

using namespace std;


DownloadHelper::DownloadHelper()
{
}

DownloadHelper::~DownloadHelper()
{
}

void
DownloadHelper::InternalInit(const string &/*url*/, const string &targetFileName, const string &/*user*/, const string &/*password*/, size_t /*filesize*/)
{
	// Open target file for writing.
	GetData()->targetFile = fopen(targetFileName.c_str(), "wb");
	if (!GetData()->targetFile)
		throw NetException(__FILE__, __LINE__, ERR_SOCK_TRANSFER_OPEN_FAILED, 0);

	// Assume that the following calls never fail.
	// NOTE: A writefunction needs to be set if a DLL version of curl is used on Windows.
	curl_easy_setopt(GetData()->curlHandle, CURLOPT_WRITEFUNCTION, NULL);
	curl_easy_setopt(GetData()->curlHandle, CURLOPT_WRITEDATA, GetData()->targetFile);
}

