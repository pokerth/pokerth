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

#include <net/uploaderthread.h>
#include <net/uploadhelper.h>
#include <net/netexception.h>
#include <boost/filesystem.hpp>
#include <core/loghelper.h>

#define UPLOAD_DELAY_MSEC					100

using namespace std;
using namespace boost::filesystem;


UploaderThread::UploaderThread()
: m_uploadInProgress(false)
{
	m_uploadHelper.reset(new UploadHelper());
}

UploaderThread::~UploaderThread()
{
}

void
UploaderThread::QueueUpload(const string &url, const string &user, const string &pwd, const string &filename, int filesize)
{
	boost::mutex::scoped_lock lock(m_uploadQueueMutex);
	m_uploadQueue.push(UploadData(url, user, pwd, filename, filesize));
}

void
UploaderThread::Main()
{
	while (!ShouldTerminate())
	{
		try
		{
			if (m_uploadInProgress)
			{
				m_uploadInProgress = !m_uploadHelper->Process();
			}

			if (!m_uploadInProgress)
			{
				Msleep(UPLOAD_DELAY_MSEC);
				UploadData data;
				{
					boost::mutex::scoped_lock lock(m_uploadQueueMutex);
					if (!m_uploadQueue.empty())
					{
						data = m_uploadQueue.front();
						m_uploadQueue.pop();
					}
				}
				if (!data.filename.empty() && data.filesize > 0)
				{
					path filepath(data.filename);
					m_uploadHelper->Init(data.address + filepath.leaf(), filepath.file_string(), data.user, data.pwd, data.filesize);
					m_uploadInProgress = true;
				}
			}
		}
		catch (const NetException &e)
		{
			LOG_ERROR(e.what());
			m_uploadInProgress = false;
		}
	}
}

