/***************************************************************************
 *   Copyright (C) 2007 by Lothar May                                      *
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
/* Base class for threads (used by network client/server). */

#ifndef _THREAD_H_
#define _THREAD_H_

#include <boost/thread.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/shared_ptr.hpp>

#ifndef NANOSECONDS_PER_SECOND
#define NANOSECONDS_PER_SECOND 1000000000
#endif

class Thread
{
public:
	Thread();
	virtual ~Thread();

	// Start the thread. Will do nothing if the
	// thread was already started.
	void Run();

	// Signal that the thread should be terminated
	void SignalTermination();

	// Wait for the termination of the thread.
	// Only one Thread should wait for the termination!
	// You SHOULD always call join for a thread.
	bool Join(unsigned msecTimeout);

	// Sleep the currently active thread.
	static void Msleep(unsigned msecs);

protected:

	// Startup function.
	void MainWrapper();

	// Main function of the thread.
	virtual void Main() = 0;

	// Checks whether termination has been requested.
	bool ShouldTerminate() const;

	// Checks whether the thread is running.
	bool IsRunning() const;

private:

	// Flag specifying whether the application code within the
	// thread was terminated.
	mutable boost::timed_mutex m_isTerminatedMutex;

	// Flag specifying whether the thread should be terminated.
	mutable boost::timed_mutex m_shouldTerminateMutex;
	mutable boost::timed_mutex::scoped_try_lock m_userReqTerminateLock;

	// The boost thread object.
	boost::shared_ptr<boost::thread> m_threadObj;
	mutable boost::mutex m_threadObjMutex;

	boost::barrier m_threadStartBarrier;

friend class ThreadStarter;
};

#endif
