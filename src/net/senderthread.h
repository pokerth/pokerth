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
#include <net/senderinterface.h>
#include <net/netpacket.h>
#include <net/sendercallback.h>

#include <list>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>

class SessionData;
#define SENDER_THREAD_TERMINATE_TIMEOUT		THREAD_WAIT_INFINITE

class SenderThread : public Thread, public SenderInterface
{
public:
	SenderThread(SenderCallback &cb);
	virtual ~SenderThread();

	virtual void Start();
	virtual void SignalStop();
	virtual void WaitStop();

	virtual void Send(boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet);
	virtual void Send(boost::shared_ptr<SessionData> session, const NetPacketList &packetList);

protected:
	typedef std::list<boost::shared_ptr<NetPacket> > SendDataList;

	class SendDataManager
	{
		public:
			SendDataManager(boost::shared_ptr<SessionData> s, boost::asio::io_service &ioService)
			: session(s), m_writeInProgress(false), m_completed(false)
			{
				socket.reset(new boost::asio::ip::tcp::socket(
					ioService, boost::asio::ip::tcp::v6(), s->GetSocket()));
			}

			void HandleWrite(const boost::system::error_code& error);

			bool IsWriteInProgress() const
			{
				boost::mutex::scoped_lock lock(m_mutex);
				return m_writeInProgress;
			}

			void SetWriteInProgress(bool v)
			{
				boost::mutex::scoped_lock lock(m_mutex);
				m_writeInProgress = v;
			}

			bool IsCompleted() const
			{
				boost::mutex::scoped_lock lock(m_mutex);
				return m_completed;
			}

			void SetCompleted(bool v)
			{
				boost::mutex::scoped_lock lock(m_mutex);
				m_completed = v;
			}

			boost::shared_ptr<SessionData> session;
			boost::shared_ptr<boost::asio::ip::tcp::socket> socket;
			SendDataList list;

		private:
			mutable boost::mutex m_mutex;
			bool m_writeInProgress;
			bool m_completed;
	};
	typedef std::map<SessionId, boost::shared_ptr<SendDataManager> > SendQueueMap;

	// Main function of the thread.
	virtual void Main();

private:

	boost::asio::io_service m_ioService;

	SendQueueMap m_sendQueueMap;
	mutable boost::mutex m_sendQueueMapMutex;

	SenderCallback &m_callback;
};

#endif

