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
/* Network client thread. */

#ifndef _CLIENTTHREAD_H_
#define _CLIENTTHREAD_H_

#include <core/thread.h>
#include <string>
#include <memory>

class ClientContext;
class ClientState;
class ClientCallback;
class SenderThread;
class ReceiverHelper;
class ClientSenderCallback;

class ClientThread : public Thread
{
public:
	ClientThread(ClientCallback &gui);
	virtual ~ClientThread();

	// Set the parameters. Does not do any error checking.
	// Error checking will be done during connect
	// (i.e. after starting the thread).
	void Init(const std::string &serverAddress, unsigned serverPort, bool ipv6, const std::string &pwd);

	ClientCallback &GetCallback();

protected:

	// Main function of the thread.
	virtual void Main();

	const ClientContext &GetContext() const;
	ClientContext &GetContext();

	ClientState &GetState();
	void SetState(ClientState &newState);

	SenderThread &GetSender();
	ReceiverHelper &GetReceiver();

	ClientSenderCallback &GetSenderCallback();

private:

	std::auto_ptr<ClientContext> m_context;
	std::auto_ptr<ClientSenderCallback> m_senderCallback;
	ClientState *m_curState;
	ClientCallback &m_callback;
	std::auto_ptr<SenderThread> m_sender;
	std::auto_ptr<ReceiverHelper> m_receiver;

friend class ClientStateInit;
friend class ClientStateStartResolve;
friend class ClientStateResolving;
friend class ClientStateStartConnect;
friend class ClientStateConnecting;
friend class ClientStateStartSession;
friend class ClientStateWaitSession;
};

#endif
