/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
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
 *                                                                           *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <net/asioreceivebuffer.h>
#include <net/sessiondata.h>
#include <core/loghelper.h>
#include <boost/swap.hpp>

using namespace std;

AsioReceiveBuffer::AsioReceiveBuffer()
	: recvBufUsed(0)
{
	recvBuf[0] = 0;
}

void
AsioReceiveBuffer::StartAsyncRead(boost::shared_ptr<SessionData> session)
{
	session->GetAsioSocket()->async_read_some(
		boost::asio::buffer(recvBuf + recvBufUsed, RECV_BUF_SIZE - recvBufUsed),
		boost::bind(
			&ReceiveBuffer::HandleRead,
			shared_from_this(),
			session,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
}

void
AsioReceiveBuffer::HandleRead(boost::shared_ptr<SessionData> session, const boost::system::error_code &error, size_t bytesRead)
{
	if (error != boost::asio::error::operation_aborted) {
		try {
			if (!error) {
				recvBufUsed += bytesRead;
				ScanPackets(session);
				ProcessPackets(session);
				StartAsyncRead(session);
			} else if (error == boost::asio::error::interrupted || error == boost::asio::error::try_again) {
				LOG_ERROR("Session " << session->GetId() << " - recv interrupted: " << error);
				StartAsyncRead(session);
			} else {
				LOG_ERROR("Session " << session->GetId() << " - Connection closed: " << error);
				// On error: Close this session.
				session->Close();
			}
		} catch (const exception &e) {
			LOG_ERROR("Session " << session->GetId() << " - unhandled exception in HandleRead: " << e.what());
			throw;
		}
	}
}

void
AsioReceiveBuffer::HandleMessage(boost::shared_ptr<SessionData> /*session*/, const string &/*msg*/)
{
	LOG_ERROR("AsioReceiveBuffer::HandleMessage should never be called because TCP I/O is not message based.");
}

void
AsioReceiveBuffer::ScanPackets(boost::shared_ptr<SessionData> session)
{
	bool dataAvailable = true;
	do {
		boost::shared_ptr<NetPacket> tmpPacket;
		// This is necessary, because we use TCP.
		// Packets may be received in multiple chunks or
		// several packets may be received at once.
		if (recvBufUsed >= NET_HEADER_SIZE) {
			// Read the size of the packet (first 4 bytes in network byte order).
			uint32_t nativeVal;
			memcpy(&nativeVal, &recvBuf[0], sizeof(uint32_t));
			size_t packetSize = ntohl(nativeVal);
			if (packetSize > MAX_PACKET_SIZE) {
				recvBufUsed = 0;
				LOG_ERROR("Session " << session->GetId() << " - Invalid packet size: " << packetSize);
			} else if (recvBufUsed >= packetSize + NET_HEADER_SIZE) {
				try {
					tmpPacket = NetPacket::Create(&recvBuf[NET_HEADER_SIZE], packetSize);
					if (tmpPacket) {
						recvBufUsed -= (packetSize + NET_HEADER_SIZE);
						if (recvBufUsed) {
							memmove(recvBuf, recvBuf + packetSize + NET_HEADER_SIZE, recvBufUsed);
						}
					}
				} catch (const exception &e) {
					// Reset buffer on error.
					recvBufUsed = 0;
					LOG_ERROR("Session " << session->GetId() << " - " << e.what());
				}
			}
		}
		if (tmpPacket) {
			if (validator.IsValidPacket(*tmpPacket)) {
				receivedPackets.push_back(tmpPacket);
			} else {
				LOG_ERROR("Session " << session->GetId() << " - Invalid packet: " << tmpPacket->GetMsg()->messagetype());
			}
		} else {
			dataAvailable = false;
		}
	} while(dataAvailable);
}

void
AsioReceiveBuffer::ProcessPackets(boost::shared_ptr<SessionData> session)
{
	while (!receivedPackets.empty()) {
		boost::shared_ptr<NetPacket> p = receivedPackets.front();
		receivedPackets.pop_front();
		session->HandlePacket(p);
	}
	if (recvBufUsed >= RECV_BUF_SIZE) {
		LOG_ERROR("Session " << session->GetId() << " - Receive buf full: " << recvBufUsed);
		recvBufUsed = 0;
	}
}

