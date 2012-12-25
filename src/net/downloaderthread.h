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
/* Network file download thread. */

#ifndef _DOWNLOADERTHREAD_H_
#define _DOWNLOADERTHREAD_H_

#include <boost/shared_ptr.hpp>
#include <queue>
#include <vector>

#include <core/thread.h>

#define DOWNLOADER_THREAD_TERMINATE_TIMEOUT		THREAD_WAIT_INFINITE
class DownloadHelper;

class DownloaderThread : public Thread
{
public:

	DownloaderThread();
	virtual ~DownloaderThread();

	void QueueDownload(unsigned downloadId, const std::string &url, const std::string &filename);
	bool HasDownloadResult() const;
	bool GetDownloadResult(unsigned &downloadId, std::vector<unsigned char> &filedata);

protected:
	struct DownloadData {
		DownloadData() : id(0) {}
		DownloadData(unsigned i, const std::string &a, const std::string &f)
			: id(i), address(a), filename(f) {}

		unsigned id;
		std::string address;
		std::string filename;
	};
	struct ResultData {
		ResultData() : id(0) {}
		ResultData(unsigned i, const std::vector<unsigned char> &d)
			: id(i), data(d) {}

		unsigned id;
		std::vector<unsigned char> data;
	};

	typedef std::queue<DownloadData> DownloadDataQueue;
	typedef std::queue<ResultData> DownloadDoneQueue;

	// Main function of the thread.
	virtual void Main();

private:

	DownloadDataQueue m_downloadQueue;
	mutable boost::mutex m_downloadQueueMutex;

	DownloadDoneQueue m_downloadDoneQueue;
	mutable boost::mutex m_downloadDoneQueueMutex;

	boost::shared_ptr<DownloadHelper> m_downloadHelper;
	bool m_downloadInProgress;

	boost::shared_ptr<DownloadData> m_curDownloadData;
};

#endif

