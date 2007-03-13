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

#include <net/serverrecvthread.h>
#include <net/serverexception.h>
#include <net/serverrecvstate.h>
#include <net/senderthread.h>
#include <net/sendercallback.h>
#include <net/receiverhelper.h>
#include <net/socket_msg.h>

class ServerSenderCallback : public SenderCallback
{
public:
	ServerSenderCallback(ServerRecvThread &server) : m_server(server) {}
	virtual ~ServerSenderCallback() {}

	virtual void SignalNetError(SOCKET sock, int errorID, int osErrorID)
	{
		// TODO
	}

private:
	ServerRecvThread &m_server;
};


ServerRecvThread::ServerRecvThread()
{
	m_senderCallback.reset(new ServerSenderCallback(*this));
}

ServerRecvThread::~ServerRecvThread()
{
}

void
ServerRecvThread::AddConnection(boost::shared_ptr<ConnectData> data)
{
	boost::mutex::scoped_lock lock(m_connectQueueMutex);
	m_connectQueue.push_back(data);
}

void
ServerRecvThread::Main()
{
	SetState(SERVER_INITIAL_STATE::Instance());

	m_sender.reset(new SenderThread(GetSenderCallback()));
	m_receiver.reset(new ReceiverHelper);
	GetSender().Run();

	try
	{
		while (!ShouldTerminate())
		{
			{
				boost::shared_ptr<ConnectData> tmpData;
				{
					boost::mutex::scoped_lock lock(m_connectQueueMutex);
					if (!m_connectQueue.empty())
					{
						tmpData = m_connectQueue.front();
						m_connectQueue.pop_front();
					}
				}
				if (tmpData.get())
					GetState().HandleNewConnection(*this, tmpData);
			}
			GetState().Process(*this);
		}
	} catch (const NetException &)
	{
		// TODO
	}
	GetSender().SignalTermination();
	GetSender().Join(SENDER_THREAD_TERMINATE_TIMEOUT);
}

ServerRecvState &
ServerRecvThread::GetState()
{
	assert(m_curState);
	return *m_curState;
}

void
ServerRecvThread::SetState(ServerRecvState &newState)
{
	m_curState = &newState;
}

SenderThread &
ServerRecvThread::GetSender()
{
	assert(m_sender.get());
	return *m_sender;
}

ReceiverHelper &
ServerRecvThread::GetReceiver()
{
	assert(m_receiver.get());
	return *m_receiver;
}

ServerSenderCallback &
ServerRecvThread::GetSenderCallback()
{
	assert(m_senderCallback.get());
	return *m_senderCallback;
}

