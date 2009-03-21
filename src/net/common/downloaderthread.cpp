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

#include <net/downloaderthread.h>
#include <net/downloadhelper.h>
#include <net/netexception.h>
#include <boost/filesystem.hpp>
#include <core/loghelper.h>

#include <algorithm>

#define DOWNLOAD_DELAY_MSEC					10

using namespace std;
using namespace boost::filesystem;


DownloaderThread::DownloaderThread()
: m_downloadInProgress(false)
{
	m_downloadHelper.reset(new DownloadHelper());
}

DownloaderThread::~DownloaderThread()
{
}

void
DownloaderThread::QueueDownload(unsigned downloadId, const std::string &url, const std::string &filename)
{
	boost::mutex::scoped_lock lock(m_downloadQueueMutex);
	m_downloadQueue.push(DownloadData(downloadId, url, filename));
}

bool
DownloaderThread::PollDownloadResult(unsigned &downloadId, std::vector<unsigned char> &filedata)
{
	bool result = false;
	boost::mutex::scoped_lock lock(m_downloadDoneQueueMutex);
	if (!m_downloadDoneQueue.empty())
	{
		const ResultData &d = m_downloadDoneQueue.front();
		downloadId = d.id;
		filedata = d.data;
		m_downloadDoneQueue.pop();
		result = true;
	}
	return result;
}

void
DownloaderThread::Main()
{
	while (!ShouldTerminate())
	{
		try
		{
			if (m_downloadInProgress)
			{
				m_downloadInProgress = !m_downloadHelper->Process();
			}

			if (!m_downloadInProgress)
			{
				// Previous download was finished.
				if (m_curDownloadData)
				{
					path filepath(m_curDownloadData->filename);
					ifstream instream(filepath.file_string().c_str(), ios_base::in | ios_base::binary);
					vector<unsigned char> fileData;
					copy(istream_iterator<unsigned char>(instream), istream_iterator<unsigned char>(), back_inserter(fileData));
					instream.close();
					remove(filepath);

					{
						boost::mutex::scoped_lock lock(m_downloadDoneQueueMutex);
						m_downloadDoneQueue.push(ResultData(m_curDownloadData->id, fileData));
					}
					m_curDownloadData.reset();
				}

				// Take a break.
				Msleep(DOWNLOAD_DELAY_MSEC);

				// Start next download.
				{
					boost::mutex::scoped_lock lock(m_downloadQueueMutex);
					if (!m_downloadQueue.empty())
					{
						m_curDownloadData.reset(new DownloadData(m_downloadQueue.front()));
						m_downloadQueue.pop();
					}
				}
				if (m_curDownloadData && !m_curDownloadData->filename.empty())
				{
					path filepath(m_curDownloadData->filename);
					m_downloadHelper->Init(m_curDownloadData->address, filepath.file_string().c_str());
					m_downloadInProgress = true;
				}
			}
		}
		catch (const NetException &e)
		{
			LOG_ERROR(e.what());
		}
	}
}

