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
/* Network file upload thread. */

#ifndef _UPLOADERTHREAD_H_
#define _UPLOADERTHREAD_H_

#include <core/thread.h>

#include <queue>
#include <boost/shared_ptr.hpp>

#define UPLOADER_THREAD_TERMINATE_TIMEOUT		THREAD_WAIT_INFINITE
class UploadHelper;

class UploaderThread : public Thread
{
public:

	UploaderThread();
	virtual ~UploaderThread();

	void QueueUpload(const std::string &url, const std::string &user, const std::string &pwd, const std::string &filename, int filesize);

protected:
	struct UploadData
	{
		UploadData() : filesize(0) {}
		UploadData(const std::string &a, const std::string &u, const std::string &p, const std::string &f, int s)
		: address(a), user(u), pwd(p), filename(f), filesize(s) {}

		std::string address;
		std::string user;
		std::string pwd;
		std::string filename;
		int filesize;
	};

	typedef std::queue<UploadData> UploadDataQueue;

	// Main function of the thread.
	virtual void Main();

private:

	UploadDataQueue m_uploadQueue;
	mutable boost::mutex m_uploadQueueMutex;

	boost::shared_ptr<UploadHelper> m_uploadHelper;
	bool m_uploadInProgress;
};

#endif

