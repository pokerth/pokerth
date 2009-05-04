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

#include <core/timermanager.h>


using namespace std;


TimerManager::TimerManager()
: m_curTimerId(0)
{
}

unsigned
TimerManager::RegisterTimer(unsigned timeoutMsec, boost::function<void()> timerHandler, bool timerRepeat)
{
	// Register a new timer callback.
	boost::recursive_mutex::scoped_lock lock(m_timerMutex);
	// Use a unique id for each timer.
	unsigned id = GetNextTimerId();
	// Sort all timers by tick on which the callback occurs.
	unsigned absoluteTimer = static_cast<unsigned>(m_softwareTimer.elapsed().total_milliseconds()) + timeoutMsec;
	m_timerMap.insert(
		TimerMap::value_type(absoluteTimer, TimerData(id, timeoutMsec, timerHandler, timerRepeat)));
	return id;
}

bool
TimerManager::UnregisterTimer(unsigned timerId)
{
	// Remove a timer callback from the map.
	boost::recursive_mutex::scoped_lock lock(m_timerMutex);
	bool unregistered = false;
	TimerMap::iterator i = m_timerMap.begin();
	TimerMap::iterator end = m_timerMap.end();
	while (i != end)
	{
		if (i->second.id == timerId)
		{
			m_timerMap.erase(i);
			unregistered = true;
			break;
		}
		++i;
	}
	return unregistered;
}

void
TimerManager::Process()
{
	boost::recursive_mutex::scoped_lock lock(m_timerMutex);
	bool timerOccurred;
	do
	{
		timerOccurred = false;
		TimerMap::iterator i = m_timerMap.begin();
		if (i != m_timerMap.end())
		{
			// Check whether the timer occured.
			unsigned currentTicks = static_cast<unsigned>(m_softwareTimer.elapsed().total_milliseconds());
			unsigned timerTicks = i->first;
			if (currentTicks >= timerTicks)
			{
				// Grab a copy of the timer.
				TimerData timer = i->second;
				// Remove the timer, re-add if it is repeating.
				m_timerMap.erase(i);
				if (timer.repeat)
				{
					// Re-add the repeating timer.
					// Try to be precise.
					unsigned absoluteTimer = currentTicks + timer.msec;
					unsigned tickDiff = currentTicks - timerTicks;
					if (absoluteTimer >= tickDiff)
						absoluteTimer -= tickDiff;
					m_timerMap.insert(
						TimerMap::value_type(absoluteTimer, timer));
				}

				// The callback may register/unregister a timer (a recursive mutex is used).
				timer.handler();
				timerOccurred = true;
			}
		}
	} while (timerOccurred);
}

unsigned
TimerManager::GetNextTimerId()
{
	return ++m_curTimerId;
}

