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
/* A manager for asynchronous software timer. */

#ifndef _TIMERMANAGER_H_
#define _TIMERMANAGER_H_

#include <map>
#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>

class TimerManager
{
public:
	TimerManager(boost::shared_ptr<boost::asio::io_service> ioService);

	unsigned RegisterTimer(unsigned timeoutMsec, boost::function<void()> timerHandler, bool autoRestart = false);
	bool UnregisterTimer(unsigned timerId);

protected:

	struct TimerData
	{
		unsigned id;
		boost::shared_ptr<boost::asio::deadline_timer> timer;
		boost::function<void()> userHandler;
		unsigned durationMsec;
		bool autoRestart;
		bool cancelled;
	};

	void Handler(const boost::system::error_code &ec, boost::shared_ptr<TimerData> data);
	typedef std::map<unsigned,  boost::shared_ptr<TimerData> > TimerMap;

	unsigned GetNextTimerId();

private:
	boost::shared_ptr<boost::asio::io_service>	m_ioService;
	mutable boost::recursive_mutex	m_timerMutex;
	TimerMap						m_timerMap;
	unsigned						m_curTimerId;
};

#endif
