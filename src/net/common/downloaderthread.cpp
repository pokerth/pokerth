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

#include <net/downloaderthread.h>
#include <net/downloadhelper.h>
#include <net/netexception.h>
#include <boost/filesystem.hpp>
#include <core/loghelper.h>

#include <fstream>
#include <algorithm>

#define DOWNLOAD_DELAY_MSEC					20

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
DownloaderThread::HasDownloadResult() const
{
	boost::mutex::scoped_lock lock(m_downloadDoneQueueMutex);
	return !m_downloadDoneQueue.empty();
}

bool
DownloaderThread::GetDownloadResult(unsigned &downloadId, std::vector<unsigned char> &filedata)
{
	bool result = false;
	boost::mutex::scoped_lock lock(m_downloadDoneQueueMutex);
	if (!m_downloadDoneQueue.empty()) {
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
	while (!ShouldTerminate()) {
		try {
			if (m_downloadInProgress) {
				m_downloadInProgress = !m_downloadHelper->Process();
			}

			if (!m_downloadInProgress) {
				// Previous download was finished.
				if (m_curDownloadData) {
					path filepath(m_curDownloadData->filename);
					ifstream instream(filepath.file_string().c_str(), ios_base::in | ios_base::binary);
					// Find out file size.
					// Not fully portable, but works on win/linux/mac.
					instream.seekg(0, ios_base::beg);
					std::streampos startPos = instream.tellg();
					instream.seekg(0, ios_base::end);
					std::streampos endPos = instream.tellg();
					instream.seekg(0, ios_base::beg);
					std::streamoff posDiff(endPos - startPos);
					unsigned fileSize = (unsigned)posDiff;

					vector<unsigned char> fileData(fileSize);
					instream.read((char *)&fileData[0], fileSize);
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
					if (!m_downloadQueue.empty()) {
						m_curDownloadData.reset(new DownloadData(m_downloadQueue.front()));
						m_downloadQueue.pop();
					}
				}
				if (m_curDownloadData && !m_curDownloadData->filename.empty()) {
					path filepath(m_curDownloadData->filename);
					m_downloadHelper->Init(m_curDownloadData->address, filepath.file_string());
					m_downloadInProgress = true;
				}
			}
		} catch (const NetException &e) {
			LOG_ERROR("Download failed: " << e.what());
			m_downloadInProgress = false;
			m_curDownloadData.reset();
		}
	}
}

