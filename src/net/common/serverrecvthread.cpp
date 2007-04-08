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


ServerRecvThread::ServerRecvThread(ServerCallback &cb)
: m_callback(cb)
{
	m_senderCallback.reset(new ServerSenderCallback(*this));
	m_sender.reset(new SenderThread(GetSenderCallback()));
	m_receiver.reset(new ReceiverHelper);
}

ServerRecvThread::~ServerRecvThread()
{
	CleanupConnectQueue();
	CleanupSessionMap();
}

void
ServerRecvThread::SendToAllPlayers(boost::shared_ptr<NetPacket> packet)
{
	// This function needs to be thread safe.
	boost::mutex::scoped_lock lock(m_sessionMapMutex);

	SocketSessionMap::iterator i = m_sessionMap.begin();
	SocketSessionMap::iterator end = m_sessionMap.end();

	while (i != end)
	{
		// Send each client a copy of the packet.
		GetSender().Send(boost::shared_ptr<NetPacket>(packet->Clone()), i->first);
		++i;
	}
}

void
ServerRecvThread::AddConnection(boost::shared_ptr<ConnectData> data)
{
	boost::mutex::scoped_lock lock(m_connectQueueMutex);
	m_connectQueue.push_back(data);
}

void
ServerRecvThread::AddNotification(unsigned notification)
{
	boost::mutex::scoped_lock lock(m_notificationQueueMutex);
	m_notificationQueue.push_back(notification);
}

ServerCallback &
ServerRecvThread::GetCallback()
{
	return m_callback;
}

void
ServerRecvThread::Main()
{
	SetState(SERVER_INITIAL_STATE::Instance());
	GetSender().Run();

	try
	{
		while (!ShouldTerminate())
		{
			{
				// Handle one incoming connection at a time.
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
			// Process current state.
			GetState().Process(*this);
			// Process thread-safe notifications.
			NotificationLoop();
		}
	} catch (const NetException &)
	{
		// TODO
	}
	GetSender().SignalTermination();
	GetSender().Join(SENDER_THREAD_TERMINATE_TIMEOUT);

	CleanupConnectQueue();
	CleanupSessionMap();
}

void
ServerRecvThread::NotificationLoop()
{
	boost::mutex::scoped_lock lock(m_notificationQueueMutex);
	// Process all notifications.
	while (!m_notificationQueue.empty())
	{
		unsigned notification = m_notificationQueue.front();
		m_notificationQueue.pop_front();

		switch(notification)
		{
			case NOTIFY_GAME_START:
				SetState(SERVER_START_GAME_STATE::Instance());
				break;
		}
	}
}

SOCKET
ServerRecvThread::Select()
{
	SOCKET retSock = INVALID_SOCKET;

	SOCKET maxSock = INVALID_SOCKET;
	fd_set rdset;
	FD_ZERO(&rdset);

	{
		boost::mutex::scoped_lock lock(m_sessionMapMutex);
		SocketSessionMap::iterator i = m_sessionMap.begin();
		SocketSessionMap::iterator end = m_sessionMap.end();

		while (i != end)
		{
			SOCKET tmpSock = i->first;
			FD_SET(tmpSock, &rdset);
			if (tmpSock > maxSock || maxSock == INVALID_SOCKET)
				maxSock = tmpSock;
			++i;
		}
	}

	if (maxSock == INVALID_SOCKET)
	{
		Msleep(RECV_TIMEOUT_MSEC); // just sleep if there is no session
	}
	else
	{
		// wait for data
		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = RECV_TIMEOUT_MSEC * 1000;
		int selectResult = select(maxSock + 1, &rdset, NULL, NULL, &timeout);
		if (!IS_VALID_SELECT(selectResult))
		{
			throw ServerException(ERR_SOCK_SELECT_FAILED, SOCKET_ERRNO());
		}
		if (selectResult > 0) // one (or more) of the sockets is readable
		{
			// Check which socket is readable, return the first.
			boost::mutex::scoped_lock lock(m_sessionMapMutex);
			SocketSessionMap::iterator i = m_sessionMap.begin();
			SocketSessionMap::iterator end = m_sessionMap.end();

			while (i != end)
			{
				SOCKET tmpSock = i->first;
				if (FD_ISSET(tmpSock, &rdset))
				{
					retSock = tmpSock;
					break;
				}
				++i;
			}
		}
	}
	return retSock;
}

void
ServerRecvThread::CleanupConnectQueue()
{
	boost::mutex::scoped_lock lock(m_connectQueueMutex);

	// Sockets will be closed automatically.
	m_connectQueue.clear();
}

void
ServerRecvThread::CleanupSessionMap()
{
	boost::mutex::scoped_lock lock(m_sessionMapMutex);

	// We need to manually close all sockets for the sessions.
	// This is "not great", but there are some issues when
	// automatically closing them.
	SocketSessionMap::iterator i = m_sessionMap.begin();
	SocketSessionMap::iterator end = m_sessionMap.end();

	while (i != end)
	{
		CLOSESOCKET(i->first);
		++i;
	}
	m_sessionMap.clear();
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

boost::shared_ptr<SessionData>
ServerRecvThread::GetSession(SOCKET sock)
{
	boost::shared_ptr<SessionData> tmpSession;
	boost::mutex::scoped_lock lock(m_sessionMapMutex);

	SocketSessionMap::iterator pos = m_sessionMap.find(sock);
	if (pos != m_sessionMap.end())
	{
		tmpSession = pos->second;
	}
	return tmpSession;
}

void
ServerRecvThread::AddSession(boost::shared_ptr<ConnectData> connData, boost::shared_ptr<SessionData> sessionData)
{
	boost::mutex::scoped_lock lock(m_sessionMapMutex);

	SocketSessionMap::iterator pos = m_sessionMap.lower_bound(connData->GetSocket());

	// If pos points to a pair whose key is equivalent to the socket, this handle
	// already exists within the list.
	if (pos != m_sessionMap.end() && connData->GetSocket() == pos->first)
	{
		throw ServerException(ERR_SOCK_CONN_EXISTS, 0);
	}
	m_sessionMap.insert(pos, SocketSessionMap::value_type(connData->ReleaseSocket(), sessionData));
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

