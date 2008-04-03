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
#include <boost/version.hpp> // solve compatibility issues


// This is ugly, but I can't help it.
inline void ADD_MSEC_TO_XTIME(boost::xtime &xt, unsigned msec)
{
	xt.sec += msec / 1000;
	xt.nsec += (msec % 1000) * 1000000;
	if (xt.nsec > NANOSECONDS_PER_SECOND)
	{
		xt.sec++;
		xt.nsec -= NANOSECONDS_PER_SECOND;
	}
}


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
{
}

Thread::~Thread()
{
}

void
Thread::Run()
{
	boost::mutex::scoped_lock threadLock(m_threadObjMutex);

	// Create the boost thread object.
	if (!m_threadObj.get())
	{
		// Initialise data structures within the context of the thread
		// who runs/terminates this thread.
		m_userReqTerminateLock.reset(new boost::timed_mutex::scoped_try_lock(m_shouldTerminateMutex));
		m_threadStartBarrier.reset(new boost::barrier(2));

		m_threadObj.reset(new boost::thread(ThreadStarter(*this)));
		m_threadStartBarrier->wait();
	}
}

void
Thread::SignalTermination()
{
	// Unlock the shouldTerminateMutex.
	if (m_userReqTerminateLock.get()) // cannot signal before calling Run
		m_userReqTerminateLock->unlock();
}

bool
Thread::Join(unsigned msecTimeout)
{
	if (!IsRunning())
		return true;

	bool tmpIsTerminated;
	if (msecTimeout == THREAD_WAIT_INFINITE)
	{
		// Wait infinitely.
		boost::timed_mutex::scoped_lock lock(m_isTerminatedMutex);
		tmpIsTerminated = true;
	}
	else
	{
		// Wait for the termination of the application code.
#if (BOOST_VERSION) >= 103500
		boost::defer_lock_t defer;
		boost::timed_mutex::scoped_timed_lock lock(m_isTerminatedMutex, defer);
		tmpIsTerminated = lock.timed_lock(boost::posix_time::millisec(msecTimeout));
#else
		// Calculate time after timeout
		boost::xtime t;
		boost::xtime_get(&t, boost::TIME_UTC);
		ADD_MSEC_TO_XTIME(t, msecTimeout);

		boost::timed_mutex::scoped_timed_lock lock(m_isTerminatedMutex, t);
		tmpIsTerminated = lock.locked();
#endif
	}

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
Thread::Msleep(unsigned msecs)
{
	boost::xtime t;
	boost::xtime_get(&t, boost::TIME_UTC);
	ADD_MSEC_TO_XTIME(t, msecs);

	boost::thread::sleep(t);
}

void
Thread::MainWrapper()
{
	boost::timed_mutex::scoped_lock lock(m_isTerminatedMutex);
	assert(m_threadStartBarrier.get());
	m_threadStartBarrier->wait();
	this->Main();
}

bool
Thread::ShouldTerminate() const
{
#if (BOOST_VERSION) >= 103500
	boost::defer_lock_t defer;
	boost::timed_mutex::scoped_try_lock lock(m_shouldTerminateMutex, defer);
	return lock.try_lock();
#else
	boost::timed_mutex::scoped_try_lock lock(m_shouldTerminateMutex);
	return lock.locked();
#endif
}

bool
Thread::IsRunning() const
{
	boost::mutex::scoped_lock threadLock(m_threadObjMutex);
	return (m_threadObj.get() != NULL);
}

