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
#include <curl/curl.h>

#include <cstdio>

using namespace std;

struct DownloadData
{
	CURL *curlHandle;
	CURLM *curlMultiHandle;
	FILE *targetFile;
};


DownloadHelper::DownloadHelper()
{
	m_data.reset(new DownloadData);
	m_data->curlHandle = NULL;
	m_data->curlMultiHandle = NULL;
	m_data->targetFile = NULL;
}

DownloadHelper::~DownloadHelper()
{
	Cleanup();
}

static size_t curlWriter(void *ptr, size_t size, size_t nmemb, void *stream)
{
	int written = fwrite(ptr, size, nmemb, (FILE *)stream);
	return written;
}

void
DownloadHelper::Init(const string &url, const string &targetFileName)
{
	m_data->targetFile = fopen(targetFileName.c_str(), "wb");
	m_data->curlHandle = curl_easy_init();
	m_data->curlMultiHandle = curl_multi_init();

	// TODO throw exception on error

	curl_easy_setopt(m_data->curlHandle, CURLOPT_URL, url.c_str());
	curl_easy_setopt(m_data->curlHandle, CURLOPT_WRITEFUNCTION, curlWriter);
	curl_easy_setopt(m_data->curlHandle, CURLOPT_WRITEDATA, m_data->targetFile);

	curl_multi_add_handle(m_data->curlMultiHandle, m_data->curlHandle);
}

bool
DownloadHelper::Process()
{
	bool retVal = false;
	int runningHandles;
	while(CURLM_CALL_MULTI_PERFORM ==
		curl_multi_perform(m_data->curlMultiHandle, &runningHandles));

	if (runningHandles)
	{
		struct timeval timeout;
		fd_set readSet;
		fd_set writeSet;
		fd_set exceptSet;
		int maxfd;

		FD_ZERO(&readSet);
		FD_ZERO(&writeSet);
		FD_ZERO(&exceptSet);

		timeout.tv_sec = 0;
		timeout.tv_usec = RECV_TIMEOUT_MSEC * 1000;

		curl_multi_fdset(m_data->curlMultiHandle, &readSet, &writeSet, &exceptSet, &maxfd);

		if (maxfd >= 0)
			int selectResult = select(maxfd+1, &readSet, &writeSet, &exceptSet, &timeout);
		// TODO throw exception on error
	}
	else
	{
		Cleanup();
		retVal = true;
	}
	return retVal;
}

void
DownloadHelper::Cleanup()
{
	if (m_data->curlMultiHandle)
	{
		curl_multi_cleanup(m_data->curlMultiHandle);
		m_data->curlMultiHandle = NULL;
	}
	if (m_data->curlHandle)
	{
		curl_easy_cleanup(m_data->curlHandle);
		m_data->curlHandle = NULL;
	}
	if (m_data->targetFile)
	{
		fclose(m_data->targetFile);
		m_data->targetFile = NULL;
	}
}

