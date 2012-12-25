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
#include <net/transferhelper.h>
#include <net/netexception.h>
#include <net/socket_msg.h>
#include <net/transferdata.h>

#include <cstdio>

using namespace std;

#define CURL_RECV_TIMEOUT_MSEC		50

TransferHelper::TransferHelper()
{
	m_data.reset(new TransferData);
}

TransferHelper::~TransferHelper()
{
	Cleanup();
}

void
TransferHelper::Init(const string &url, const string &targetFileName, const string &user, const string &password, size_t filesize, const string &httpPost)
{
	// Cleanup data.
	Cleanup();
	m_data->returnMessage.clear();
	// Initialise curl.
	m_data->curlHandle = curl_easy_init();
	if (!m_data->curlHandle)
		throw NetException(__FILE__, __LINE__, ERR_SOCK_TRANSFER_INIT_FAILED, 0);
	m_data->curlMultiHandle = curl_multi_init();
	if (!m_data->curlMultiHandle)
		throw NetException(__FILE__, __LINE__, ERR_SOCK_TRANSFER_INIT_FAILED, 0);

	// Use a copy of the url string, because some curl versions require a copy.
	m_data->curlUrl = url;
	if (curl_easy_setopt(m_data->curlHandle, CURLOPT_URL, m_data->curlUrl.c_str()) != CURLE_OK)
		throw NetException(__FILE__, __LINE__, ERR_SOCK_TRANSFER_INVALID_URL, 0);

	InternalInit(url, targetFileName, user, password, filesize, httpPost);

	// Use the multi interface for better abort handling.
	if (curl_multi_add_handle(m_data->curlMultiHandle, m_data->curlHandle) != CURLM_OK)
		throw NetException(__FILE__, __LINE__, ERR_SOCK_TRANSFER_INIT_FAILED, 0);
}

bool
TransferHelper::Process()
{
	bool retVal = false;
	int runningHandles = 0;
	CURLMcode curlResult;
	do {
		curlResult = curl_multi_perform(m_data->curlMultiHandle, &runningHandles);
	} while (curlResult == CURLM_CALL_MULTI_PERFORM);

	if (curlResult != CURLM_OK)
		throw NetException(__FILE__, __LINE__, ERR_SOCK_TRANSFER_FAILED, 0);

	if (runningHandles) {
		struct timeval timeout;
		fd_set readSet;
		fd_set writeSet;
		fd_set exceptSet;
		int maxfd = -1;

		FD_ZERO(&readSet);
		FD_ZERO(&writeSet);
		FD_ZERO(&exceptSet);

		timeout.tv_sec = 0;
		timeout.tv_usec = CURL_RECV_TIMEOUT_MSEC * 1000;

		curl_multi_fdset(m_data->curlMultiHandle, &readSet, &writeSet, &exceptSet, &maxfd);

		if (maxfd >= 0) {
			int selectResult = select(maxfd+1, &readSet, &writeSet, &exceptSet, &timeout);
			if (selectResult == -1)
				throw NetException(__FILE__, __LINE__, ERR_SOCK_TRANSFER_SELECT_FAILED, 0);
		}
	} else {
		// Retrieve actual error code.
		int numMsgs;
		CURLMsg *tmpMsg;
		CURLcode code = CURLE_FAILED_INIT;
		do {
			tmpMsg = curl_multi_info_read(m_data->curlMultiHandle, &numMsgs);
			if (tmpMsg)
				code = tmpMsg->data.result;
		} while (tmpMsg && tmpMsg->msg != CURLMSG_DONE);

		// Clean up the curl handles.
		Cleanup();

		// Throw exception if an error occured.
		if (code != CURLE_OK) {
			if (code == CURLE_URL_MALFORMAT)
				throw NetException(__FILE__, __LINE__, ERR_SOCK_TRANSFER_INVALID_URL, 0);
			else
				throw NetException(__FILE__, __LINE__, ERR_SOCK_TRANSFER_FAILED, 0);
		}

		retVal = true;
	}
	return retVal;
}

void
TransferHelper::Cleanup()
{
	if (m_data->post) {
		curl_formfree(m_data->post);
		m_data->post = NULL;
	}
	if (m_data->curlMultiHandle) {
		curl_multi_cleanup(m_data->curlMultiHandle);
		m_data->curlMultiHandle = NULL;
	}
	if (m_data->curlHandle) {
		curl_easy_cleanup(m_data->curlHandle);
		m_data->curlHandle = NULL;
	}
	if (m_data->targetFile) {
		fflush(m_data->targetFile);
		fclose(m_data->targetFile);
		m_data->targetFile = NULL;
	}
}

string
TransferHelper::ResetLastMessage()
{
	string retVal(m_data->returnMessage);
	m_data->returnMessage.clear();
	return retVal;
}

boost::shared_ptr<TransferData>
TransferHelper::GetData()
{
	return m_data;
}

