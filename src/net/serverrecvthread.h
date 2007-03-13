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
/* Network server receive thread. */

#ifndef _SERVERRECVTHREAD_H_
#define _SERVERRECVTHREAD_H_

#include <core/thread.h>
#include <deque>
#include <boost/shared_ptr.hpp>

#include <net/connectdata.h>

class ServerRecvState;
class SenderThread;
class ReceiverHelper;
class ServerSenderCallback;

class ServerRecvThread : public Thread
{
public:
	ServerRecvThread();
	virtual ~ServerRecvThread();

	void AddConnection(boost::shared_ptr<ConnectData> data);

protected:

	// Main function of the thread.
	virtual void Main();

	ServerRecvState &GetState();
	void SetState(ServerRecvState &newState);

	//const ServerRecvContext &GetContext() const;
	//ServerRecvContext &GetContext();

	SenderThread &GetSender();
	ReceiverHelper &GetReceiver();

	ServerSenderCallback &GetSenderCallback();

private:
	//std::auto_ptr<ServerRecvContext> m_context;

	std::deque<boost::shared_ptr<ConnectData> > m_connectQueue;
	mutable boost::mutex m_connectQueueMutex;
	ServerRecvState *m_curState;

	std::auto_ptr<ReceiverHelper> m_receiver;
	std::auto_ptr<SenderThread> m_sender;

	std::auto_ptr<ServerSenderCallback> m_senderCallback;
};

#endif
