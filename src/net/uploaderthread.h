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
/* Network file upload thread. */

#ifndef _UPLOADERTHREAD_H_
#define _UPLOADERTHREAD_H_

#include <boost/shared_ptr.hpp>
#include <queue>
#include <core/thread.h>


#define UPLOADER_THREAD_TERMINATE_TIMEOUT		THREAD_WAIT_INFINITE
class UploadHelper;
class UploadCallback;

class UploaderThread : public Thread
{
public:

	UploaderThread(UploadCallback *callback = NULL);
	virtual ~UploaderThread();

	void QueueUpload(const std::string &url, const std::string &user, const std::string &pwd, const std::string &filename, size_t filesize, const std::string &httpPost = "");

protected:
	struct UploadData {
		UploadData() : filesize(0) {}
		UploadData(const std::string &a, const std::string &u, const std::string &p, const std::string &f, size_t s, const std::string &h)
			: address(a), user(u), pwd(p), filename(f), filesize(s), httpPost(h) {}

		std::string address;
		std::string user;
		std::string pwd;
		std::string filename;
		size_t filesize;
		std::string httpPost;
	};

	typedef std::queue<UploadData> UploadDataQueue;

	// Main function of the thread.
	virtual void Main();

private:

	UploadDataQueue m_uploadQueue;
	mutable boost::mutex m_uploadQueueMutex;

	boost::shared_ptr<UploadHelper> m_uploadHelper;
	bool m_uploadInProgress;
	UploadCallback *m_callback;
};

#endif

