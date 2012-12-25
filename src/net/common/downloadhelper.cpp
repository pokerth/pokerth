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
DownloadHelper::InternalInit(const string &/*url*/, const string &targetFileName, const string &/*user*/, const string &/*password*/, size_t /*filesize*/, const string &/*httpPost*/)
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

