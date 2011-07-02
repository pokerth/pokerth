/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2011 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *****************************************************************************/
/* Network sender thread. */

#ifndef _SENDERHELPER_H_
#define _SENDERHELPER_H_

#include <boost/asio.hpp>
#include <net/netpacket.h>

class SessionData;
class SendBuffer;

class SenderHelper
{
public:
	SenderHelper(boost::shared_ptr<boost::asio::io_service> ioService);
	~SenderHelper();

	void Send(boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet);
	void Send(boost::shared_ptr<SessionData> session, const NetPacketList &packetList);

protected:
	void InternalStorePacket(SendBuffer &tmpManager, boost::shared_ptr<NetPacket> packet);

private:

	boost::shared_ptr<boost::asio::io_service> m_ioService;
};

#endif

