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
: m_userReqTerminateLock(m_shouldTerminateMutex), m_threadStartBarrier(2)
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
	{
		m_threadObj.reset(new boost::thread(ThreadStarter(*this)));
		m_threadStartBarrier.wait();
	}
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
	if (!IsRunning())
		return true;

	// Calculate time after timeout
	boost::xtime t;
	boost::xtime_get(&t, boost::TIME_UTC);
	ADD_MSEC_TO_XTIME(t, msecTimeout);

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
	m_threadStartBarrier.wait();
	this->Main();
}

bool
Thread::ShouldTerminate() const
{
	boost::timed_mutex::scoped_try_lock lock(m_shouldTerminateMutex);
	return lock.locked();
}

bool
Thread::IsRunning() const
{
	boost::mutex::scoped_lock threadLock(m_threadObjMutex);
	return (m_threadObj.get() != NULL);
}

