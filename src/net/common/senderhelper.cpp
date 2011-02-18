/***************************************************************************
 *   Copyright (C) 2007-2011 by Lothar May                                 *
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

#include <net/senderhelper.h>
#include <net/senddatamanager.h>
#include <net/sendercallback.h>
#include <net/socket_helper.h>
#include <net/socket_msg.h>
#include <core/loghelper.h>
#include <cstring>
#include <cassert>

using namespace std;

SenderHelper::SenderHelper(SenderCallback &cb, boost::shared_ptr<boost::asio::io_service> ioService)
	: m_callback(cb), m_ioService(ioService)
{
}

SenderHelper::~SenderHelper()
{
}

void
SenderHelper::Send(boost::shared_ptr<SessionData> session, boost::shared_ptr<NetPacket> packet)
{
	if (packet && session) {
		boost::shared_ptr<SendDataManager> tmpManager;
		{
			// First: lock map of all queues. Locate/insert queue.
			boost::mutex::scoped_lock lock(m_sendQueueMapMutex);
			SendQueueMap::iterator pos = m_sendQueueMap.find(session->GetId());
			if (pos == m_sendQueueMap.end())
				pos = m_sendQueueMap.insert(SendQueueMap::value_type(session->GetId(), boost::shared_ptr<SendDataManager>(new SendDataManager))).first;
			tmpManager = pos->second;
		}
		{
			// Second: Add packet to specific queue.
			boost::mutex::scoped_lock lock(tmpManager->dataMutex);
			if (tmpManager->GetAllocated() <= MAX_SEND_BUF_SIZE) {
				InternalStorePacket(*tmpManager, packet);
			}
		}
		{
			// Third: Activate async send, if needed.
			tmpManager->AsyncSendNextPacket(session->GetAsioSocket());
		}
	}
}

void
SenderHelper::Send(boost::shared_ptr<SessionData> session, const NetPacketList &packetList)
{
	if (!packetList.empty() && session) {
		boost::shared_ptr<SendDataManager> tmpManager;
		{
			// First: lock map of all queues. Locate/insert queue.
			boost::mutex::scoped_lock lock(m_sendQueueMapMutex);
			SendQueueMap::iterator pos = m_sendQueueMap.find(session->GetId());
			if (pos == m_sendQueueMap.end())
				pos = m_sendQueueMap.insert(SendQueueMap::value_type(session->GetId(), boost::shared_ptr<SendDataManager>(new SendDataManager))).first;
			tmpManager = pos->second;
		}
		{
			// Second: Add packets to specific queue.
			boost::mutex::scoped_lock lock(tmpManager->dataMutex);
			if (tmpManager->GetAllocated() <= MAX_SEND_BUF_SIZE) {
				NetPacketList::const_iterator i = packetList.begin();
				NetPacketList::const_iterator end = packetList.end();
				while (i != end) {
					if (*i)
						InternalStorePacket(*tmpManager, *i);
					++i;
				}
			}
		}
		{
			// Third: Activate async send, if needed.
			tmpManager->AsyncSendNextPacket(session->GetAsioSocket());
		}
	}
}

void
SenderHelper::SignalSessionTerminated(unsigned sessionId)
{
	boost::mutex::scoped_lock lock(m_sendQueueMapMutex);

	SendQueueMap::iterator pos = m_sendQueueMap.find(sessionId);
	if (pos != m_sendQueueMap.end())
		m_sendQueueMap.erase(pos);
}

void
SenderHelper::InternalStorePacket(SendDataManager &tmpManager, boost::shared_ptr<NetPacket> packet)
{
	asn_enc_rval_t e = der_encode(&asn_DEF_PokerTHMessage, packet->GetMsg(), &SendDataManager::EncodeToBuf, &tmpManager);
	//cerr << "OUT:" << endl << packet->ToString() << endl;
	if (e.encoded == -1)
		LOG_ERROR("Failed to encode NetPacket: " << packet->GetMsg()->present);
}

