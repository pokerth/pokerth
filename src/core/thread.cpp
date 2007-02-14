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

#include "thread.h"

#ifndef NANOSECONDS_PER_SECOND
#define NANOSECONDS_PER_SECOND 1000000000
#endif

// Helper class for thread creation.
class ThreadStarter
{
public:
	ThreadStarter(Thread &thread) : m_thread(thread) {}
	void operator()()
	{
		m_thread.MainWrapper();
	}

private:
	Thread &m_thread;
};

Thread::Thread()
: m_isTerminatedMutexLock(m_isTerminatedMutex), m_userReqTerminateLock(m_shouldTerminateMutex)
{
}

Thread::~Thread()
{
}

void
Thread::Run()
{
	// Create the boost thread object.
	boost::mutex::scoped_lock threadLock(m_threadObjMutex);

	if (!m_threadObj.get())
		m_threadObj.reset(new boost::thread(ThreadStarter(*this)));
}

void
Thread::SignalTermination()
{
	// Unlock the shouldTerminateMutex.
	m_userReqTerminateLock.unlock();
}

bool
Thread::Join(unsigned msecTimeout)
{
	// Calculate time after timeout
	boost::xtime t;
	boost::xtime_get(&t, boost::TIME_UTC);
	t.sec += msecTimeout / 1000;
	t.nsec += (msecTimeout % 1000) * 1000;
	if (t.nsec > NANOSECONDS_PER_SECOND)
	{
		t.sec++;
		t.nsec -= NANOSECONDS_PER_SECOND;
	}

	// Wait for the termination of the application code.
	boost::timed_mutex::scoped_timed_lock lock(m_isTerminatedMutex, t);
	bool tmpIsTerminated = lock.locked();

	if (tmpIsTerminated)
	{
		boost::mutex::scoped_lock lock(m_threadObjMutex);
		// Wait for "real" termination of the thread.
		if (m_threadObj.get())
		{
			m_threadObj->join();
			m_threadObj.reset();
		}
	}

	return tmpIsTerminated;
}

void
Thread::MainWrapper()
{
	this->Main();
	// Thread has been terminated.
	// Unlock the isTerminated mutex.
	m_isTerminatedMutexLock.unlock();
}

bool
Thread::ShouldTerminate() const
{
	boost::timed_mutex::scoped_try_lock lock(m_shouldTerminateMutex);
	return lock.locked();
}

