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

#include <net/clientthread.h>
#include <net/clientstate.h>
#include <net/clientdata.h>
#include <net/senderthread.h>
#include <net/clientcallback.h>
#include <net/clientexception.h>
#include <net/socket_msg.h>


#include <cassert>

using namespace std;


ClientThread::ClientThread(ClientCallback &cb)
: m_curState(NULL), m_callback(cb)
{
	m_data.reset(new ClientData);
}

ClientThread::~ClientThread()
{
}

void
ClientThread::Init(const string &serverAddress, unsigned serverPort, bool ipv6, const string &pwd)
{
	if (IsRunning())
		return; // TODO: throw exception

	ClientData &data = GetData();

	data.addrFamily	= ipv6 ? AF_INET6 : AF_INET;
	data.serverAddr	= serverAddress;
	data.serverPort	= serverPort;
	data.password	= pwd;
}

void
ClientThread::Main()
{
	m_sender.reset(new SenderThread);
	SetState(CLIENT_INITIAL_STATE::Instance());
	try {
		while (!ShouldTerminate())
		{
			int msg = GetState().Process(*this);
			if (msg != MSG_SOCK_INTERNAL_PENDING)
				m_callback.SignalNetClientSuccess(msg);
		}
	} catch (const ClientException &e)
	{
		m_callback.SignalNetClientError(e.GetErrorId(), e.GetOsErrorCode());
	}
	GetSender().SignalTermination();
	GetSender().Join(100);
}

const ClientData &
ClientThread::GetData() const
{
	assert(m_data.get());
	return *m_data;
}

ClientData &
ClientThread::GetData()
{
	assert(m_data.get());
	return *m_data;
}

ClientState &
ClientThread::GetState()
{
	assert(m_curState);
	return *m_curState;
}

void
ClientThread::SetState(ClientState &newState)
{
	m_curState = &newState;
}

SenderThread &
ClientThread::GetSender()
{
	assert(m_sender.get());
	return *m_sender;
}

