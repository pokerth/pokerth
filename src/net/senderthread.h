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
/* Network sender thread. */

#ifndef _SENDERTHREAD_H_
#define _SENDERTHREAD_H_

#include <core/thread.h>
#include <net/sessiondata.h>
#include <net/netpacket.h>
#include <net/sendercallback.h>

#include <list>
#include <boost/shared_ptr.hpp>
#include <third_party/boost/timers.hpp>

#define SENDER_THREAD_TERMINATE_TIMEOUT		THREAD_WAIT_INFINITE

class SenderThread : public Thread
{
public:
	SenderThread(SenderCallback &cb);
	virtual ~SenderThread();

	void Send(boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet);
	void Send(boost::shared_ptr<SessionData> session, const NetPacketList &packetList);

	unsigned GetNumPacketsInQueue() const;
	bool operator<(const SenderThread &other) const;

protected:
	struct SendData
	{
		SendData()
		: bytesSent(0) {}
		SendData(boost::shared_ptr<NetPacket> p, boost::shared_ptr<SessionData> s)
		: packet(p), session(s), bytesSent(0) {}
		boost::shared_ptr<NetPacket> packet;
		boost::shared_ptr<SessionData> session;
		unsigned bytesSent;
	};
	typedef std::list<SendData> SendDataList;
	typedef std::list<SessionId> SessionIdList;

	// Main function of the thread.
	virtual void Main();

	void InternalStore(SendDataList &sendQueue, unsigned maxQueueSize, boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet);
	void InternalStore(SendDataList &sendQueue, unsigned maxQueueSize, boost::shared_ptr<SessionData> session, const NetPacketList &packetList);

private:

	SendDataList m_sendQueue;
	mutable boost::mutex m_sendQueueMutex;

	SendDataList m_stalledQueue;
	mutable boost::mutex m_stalledQueueMutex;

	SessionIdList m_sessionsStalled; // Cache
	mutable boost::mutex m_sessionsStalledMutex;

	SenderCallback &m_callback;

	boost::timers::portable::microsec_timer m_logTimer;
};

#endif

