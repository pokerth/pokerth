/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2011 Felix Hammer, Florian Thauer, Lothar May          *
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
 *****************************************************************************/

#include <net/socket_helper.h>
#include <net/uploadhelper.h>
#include <net/netexception.h>
#include <net/socket_msg.h>
#include <net/transferdata.h>
#include <sys/stat.h>

#include <cstdio>

using namespace std;


size_t
readFunction(char *bufptr, size_t size, size_t nitems, void *userp)
{
	return fread(bufptr, size, nitems, (FILE *)userp);
}

UploadHelper::UploadHelper()
{
}

UploadHelper::~UploadHelper()
{
}

void
UploadHelper::InternalInit(const string &/*url*/, const string &targetFileName, const string &user, const string &password, size_t filesize)
{
	// Open target file for reading.
	GetData()->targetFile = fopen(targetFileName.c_str(), "rb");
	if (!GetData()->targetFile)
		throw NetException(__FILE__, __LINE__, ERR_SOCK_TRANSFER_OPEN_FAILED, 0);

	GetData()->userCredentials = user + ":" + password;
	// Assume that the following calls never fail.
	curl_easy_setopt(GetData()->curlHandle, CURLOPT_READFUNCTION, readFunction);
	curl_easy_setopt(GetData()->curlHandle, CURLOPT_READDATA, GetData()->targetFile);
	curl_easy_setopt(GetData()->curlHandle, CURLOPT_INFILESIZE, filesize);
	curl_easy_setopt(GetData()->curlHandle, CURLOPT_USERPWD, GetData()->userCredentials.c_str());
	curl_easy_setopt(GetData()->curlHandle, CURLOPT_UPLOAD, 1L);
	curl_easy_setopt(GetData()->curlHandle, CURLOPT_PUT, 1L);
	curl_easy_setopt(GetData()->curlHandle, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(GetData()->curlHandle, CURLOPT_SSL_VERIFYHOST, 0L);
}

