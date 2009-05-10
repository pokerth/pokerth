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

#ifndef _SENDERHELPER_H_
#define _SENDERHELPER_H_

#include <net/senderinterface.h>
#include <net/netpacket.h>
#include <net/sendercallback.h>

class SessionData;
class SendDataManager;

class SenderHelper : public SenderInterface
{
public:
	SenderHelper(SenderCallback &cb, boost::shared_ptr<boost::asio::io_service> ioService);
	virtual ~SenderHelper();

	virtual void Send(boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet);
	virtual void Send(boost::shared_ptr<SessionData> session, const NetPacketList &packetList);

	virtual void Process();

	virtual void SignalSessionTerminated(unsigned sessionId);

protected:
	typedef std::list<unsigned> SessionIdList;

	typedef std::map<SessionId, boost::shared_ptr<SendDataManager> > SendQueueMap;

private:

	SenderCallback &m_callback;
	boost::shared_ptr<boost::asio::io_service> m_ioService;

	SendQueueMap m_sendQueueMap;
	mutable boost::mutex m_sendQueueMapMutex;

	SessionIdList m_changedSessions;
	mutable boost::mutex m_changedSessionsMutex;

	SessionIdList m_removedSessions;
	mutable boost::mutex m_removedSessionsMutex;
};

#endif

