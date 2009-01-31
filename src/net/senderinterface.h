/***************************************************************************
 *   Copyright (C) 2009 by Lothar May                                      *
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
/* Interface for network sender functions. */

#ifndef _SENDERINTERFACE_H_
#define _SENDERINTERFACE_H_

#include <net/sessiondata.h>
#include <net/netpacket.h>
#include <boost/shared_ptr.hpp>

class SenderInterface
{
public:
	virtual ~SenderInterface();

	virtual void Start() = 0;
	virtual void SignalStop() = 0;
	virtual void WaitStop() = 0;

	virtual void Send(boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet) = 0;
	virtual void Send(boost::shared_ptr<SessionData> session, const NetPacketList &packetList) = 0;

	virtual void SignalSessionTerminated(unsigned sessionId) = 0;
};

#endif

