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

size_t
writeFunction(char *bufptr, size_t size, size_t nitems, void *userp)
{
	string msgPart(bufptr, size * nitems);
	((string *)userp)->append(msgPart);
	return size * nitems;
}

UploadHelper::UploadHelper()
{
}

UploadHelper::~UploadHelper()
{
}

void
UploadHelper::InternalInit(const string &/*url*/, const string &targetFileName, const string &user, const string &password, size_t filesize, const string &httpPost)
{
	curl_easy_setopt(GetData()->curlHandle, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(GetData()->curlHandle, CURLOPT_SSL_VERIFYHOST, 0L);

	if (!user.empty() || !password.empty()) {
		GetData()->userCredentials = user + ":" + password;
		curl_easy_setopt(GetData()->curlHandle, CURLOPT_USERPWD, GetData()->userCredentials.c_str());
	}
	if (httpPost.empty()) {
		// Open target file for reading.
		GetData()->targetFile = fopen(targetFileName.c_str(), "rb");
		if (!GetData()->targetFile)
			throw NetException(__FILE__, __LINE__, ERR_SOCK_TRANSFER_OPEN_FAILED, 0);
		curl_easy_setopt(GetData()->curlHandle, CURLOPT_READFUNCTION, readFunction);
		curl_easy_setopt(GetData()->curlHandle, CURLOPT_READDATA, GetData()->targetFile);
		curl_easy_setopt(GetData()->curlHandle, CURLOPT_PUT, 1L);
		curl_easy_setopt(GetData()->curlHandle, CURLOPT_UPLOAD, 1L);
		curl_easy_setopt(GetData()->curlHandle, CURLOPT_INFILESIZE, filesize);
	} else {
		// Curl will handle file I/O.
		struct curl_httppost *last = NULL;
		curl_formadd(&GetData()->post, &last,
					 CURLFORM_COPYNAME, httpPost.c_str(),
					 CURLFORM_FILE, targetFileName.c_str(),
					 CURLFORM_END);
		curl_easy_setopt(GetData()->curlHandle, CURLOPT_HTTPPOST, GetData()->post);
		curl_easy_setopt(GetData()->curlHandle, CURLOPT_WRITEFUNCTION, writeFunction);
		curl_easy_setopt(GetData()->curlHandle, CURLOPT_WRITEDATA, &GetData()->returnMessage);
	}
}

