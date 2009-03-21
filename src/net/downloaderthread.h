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
/* Network file download thread. */

#ifndef _DOWNLOADERTHREAD_H_
#define _DOWNLOADERTHREAD_H_

#include <core/thread.h>

#include <queue>
#include <vector>
#include <boost/shared_ptr.hpp>

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
	struct DownloadData
	{
		DownloadData() : id(0) {}
		DownloadData(unsigned i, const std::string &a, const std::string &f)
		: id(i), address(a), filename(f) {}

		unsigned id;
		std::string address;
		std::string filename;
	};
	struct ResultData
	{
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

