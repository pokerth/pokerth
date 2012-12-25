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

#include <net/uploaderthread.h>
#include <net/uploadhelper.h>
#include <net/uploadcallback.h>
#include <net/netexception.h>
#include <boost/filesystem.hpp>
#include <core/loghelper.h>

#define UPLOAD_DELAY_MSEC					100

using namespace std;
using namespace boost::filesystem;


UploaderThread::UploaderThread(UploadCallback *callback)
	: m_uploadInProgress(false), m_callback(callback)
{
	m_uploadHelper.reset(new UploadHelper());
}

UploaderThread::~UploaderThread()
{
}

void
UploaderThread::QueueUpload(const string &url, const string &user, const string &pwd, const string &filename, size_t filesize, const string &httpPost)
{
	boost::mutex::scoped_lock lock(m_uploadQueueMutex);
	m_uploadQueue.push(UploadData(url, user, pwd, filename, filesize, httpPost));
}

void
UploaderThread::Main()
{
	string lastfile;
	while (!ShouldTerminate()) {
		try {
			if (m_uploadInProgress) {
				m_uploadInProgress = !m_uploadHelper->Process();
			}

			if (!m_uploadInProgress) {
				string lastMsg(m_uploadHelper->ResetLastMessage());
				if (!lastMsg.empty() && m_callback) {
					m_callback->UploadCompleted(lastfile, lastMsg);
				}
				Msleep(UPLOAD_DELAY_MSEC);
				UploadData data;
				{
					boost::mutex::scoped_lock lock(m_uploadQueueMutex);
					if (!m_uploadQueue.empty()) {
						data = m_uploadQueue.front();
						lastfile = data.filename;
						m_uploadQueue.pop();
					}
				}
				if (!data.filename.empty() && data.filesize > 0) {
					path filepath(data.filename);
					string url(data.address);
					if (data.httpPost.empty()) {
#if BOOST_FILESYSTEM_VERSION != 3
						url += filepath.leaf();
#else
						url += filepath.filename().string();
#endif
					}
					m_uploadHelper->Init(url, filepath.file_string(), data.user, data.pwd, data.filesize, data.httpPost);
					m_uploadInProgress = true;
				}
			}
		} catch (const NetException &e) {
			LOG_ERROR("Upload failed: " << e.what());
			m_uploadInProgress = false;
			if (m_callback) {
				m_callback->UploadError(lastfile, e.what());
			}
		}
	}
}

