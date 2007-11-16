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

#include <deque>
#include <boost/shared_ptr.hpp>

#define SENDER_THREAD_TERMINATE_TIMEOUT		THREAD_WAIT_INFINITE

class SenderThread : public Thread
{
public:
	SenderThread(SenderCallback &cb);
	virtual ~SenderThread();

	void Send(boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet);
	void Send(boost::shared_ptr<SessionData> session, const NetPacketList &packetList);

	void SendLowPrio(boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet);
	void SendLowPrio(boost::shared_ptr<SessionData> session, const NetPacketList &packetList);

	unsigned GetNumPacketsInQueue() const;
	bool operator<(const SenderThread &other) const;

protected:
	typedef std::pair<boost::shared_ptr<NetPacket>, boost::shared_ptr<SessionData> > SendData;
	typedef std::deque<SendData> SendDataDeque;

	// Main function of the thread.
	virtual void Main();

	void InternalStore(SendDataDeque &sendQueue, unsigned maxQueueSize, boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet);
	void InternalStore(SendDataDeque &sendQueue, unsigned maxQueueSize, boost::shared_ptr<SessionData> session, const NetPacketList &packetList);

	void RemoveCurSendData();

private:

	boost::shared_ptr<SessionData> m_curSession;

	std::deque<SendData> m_outBuf;
	mutable boost::mutex m_outBufMutex;

	std::deque<SendData> m_lowPrioOutBuf;
	mutable boost::mutex m_lowPrioOutBufMutex;

	char m_tmpOutBuf[MAX_PACKET_SIZE];
	unsigned m_tmpOutBufSize;
	bool m_tmpIsLowPrio;
	unsigned m_lastInvalidSessionId;

	SenderCallback &m_callback;
};

#endif

