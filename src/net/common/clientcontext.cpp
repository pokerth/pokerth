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

#include <net/clientcontext.h>
#include <net/senderthread.h>

class ClientSenderCallback : public SenderCallback, public SessionDataCallback
{
public:
	ClientSenderCallback() {}
	virtual ~ClientSenderCallback() {}

	virtual void SignalNetError(SessionId /*session*/, int /*errorID*/, int /*osErrorID*/)
	{
	}

	virtual void SignalSessionTerminated(unsigned /*session*/)
	{
	}

private:
};


ClientContext::ClientContext()
: m_protocol(0), m_addrFamily(AF_INET), m_useServerList(false), m_serverPort(0),
  m_hasSubscribedLobbyMsg(true)
{
	bzero(&m_clientSockaddr, sizeof(m_clientSockaddr));
	m_senderCallback.reset(new ClientSenderCallback());
	m_senderThread.reset(new SenderThread(*m_senderCallback));
	m_senderThread->Start();
}

ClientContext::~ClientContext()
{
	m_senderThread->SignalStop();
	m_senderThread->WaitStop();
}

SOCKET
ClientContext::GetSocket() const
{
	assert(m_sessionData.get());
	return m_sessionData->GetSocket();
}

void
ClientContext::SetSocket(SOCKET sockfd)
{
	m_sessionData.reset(new SessionData(sockfd, SESSION_ID_GENERIC, m_senderThread, *m_senderCallback));
}

boost::shared_ptr<SessionData>
ClientContext::GetSessionData() const
{
	return m_sessionData;
}

