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
#include <boost/bind/bind.hpp>

#include <net/asioreceivebuffer.h>
#include <net/sessiondata.h>
#include <net/sendbuffer.h>
#include <playerdata.h>
#include <core/loghelper.h>
#include <core/pokerthexception.h>
#include <QDebug>
#include <sstream>

using namespace std;

AsioReceiveBuffer::AsioReceiveBuffer()
	: recvBufUsed(0), unparsablePackets(0)
{
	recvBuf[0] = 0;
}

void
AsioReceiveBuffer::StartAsyncRead(boost::shared_ptr<SessionData> session)
{
	// Check whether the session is already closed
	if (session->GetState() == SessionData::Closed) {
		return;  // No async_read on closed sessions
	}

	if (session->IsSsl()) {
		auto sslStream = session->GetSslStream();
		if (!sslStream) {
			LOG_ERROR("Session " << session->GetId() << " - SSL stream is null, cannot start async read");
			return;
		}

		// Check whether the socket is still open
		boost::system::error_code ec;
		if (!sslStream->lowest_layer().is_open()) {
			LOG_ERROR("Session " << session->GetId() << " - SSL socket is closed, cannot start async read");
			return;
		}

		sslStream->async_read_some(
			boost::asio::buffer(recvBuf + recvBufUsed, RECV_BUF_SIZE - recvBufUsed),
			boost::bind(
				&ReceiveBuffer::HandleRead,
				shared_from_this(),
				session,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	} else {
		auto socket = session->GetAsioSocket();
		if (!socket) {
			LOG_ERROR("Session " << session->GetId() << " - Socket is null, cannot start async read");
			return;
		}

		// Check whether the socket is still open
		boost::system::error_code ec;
		if (!socket->is_open()) {
			LOG_ERROR("Session " << session->GetId() << " - Socket is closed, cannot start async read");
			return;
		}

		socket->async_read_some(
			boost::asio::buffer(recvBuf + recvBufUsed, RECV_BUF_SIZE - recvBufUsed),
			boost::bind(
				&ReceiveBuffer::HandleRead,
				shared_from_this(),
				session,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	}
}

void
AsioReceiveBuffer::HandleRead(boost::shared_ptr<SessionData> session, const boost::system::error_code &error, size_t bytesRead)
{
	if (error == boost::asio::error::operation_aborted) {
		// A locally cancelled read is only legitimate if the session was
		// closed intentionally (state Closed; on shutdown handlers no longer
		// run at all). If the session is still officially open, the socket
		// handle was closed behind our back (e.g. after a write error) -
		// swallowing the abort here would leave the session owner unaware of
		// the disconnect: the client would hang "connected" in a dead game
		// forever. Treat it as a connection loss instead.
		if (session && session->GetState() != SessionData::Closed) {
			LOG_ERROR("Session " << session->GetId() << " - read aborted while session still open, closing session");
			session->Close(); // client: throws -> GUI error; server: cleanup
		}
		return;
	}
	{
		try {
			// Check whether the session is still valid and not closed
			if (!session || session->GetState() == SessionData::Closed) {
				LOG_VERBOSE("Session " << (session ? session->GetId() : 0) << " - HandleRead on closed session, ignoring");
				return;
			}

			if (!error) {
				// Sanity check: bytesRead should not be larger than the buffer allows
				if (bytesRead > RECV_BUF_SIZE - recvBufUsed) {
					LOG_ERROR("Session " << session->GetId() << " - Buffer overflow prevented: bytesRead="
							  << bytesRead << " available=" << (RECV_BUF_SIZE - recvBufUsed));
					session->Close();
					return;
				}
				recvBufUsed += bytesRead;
				ScanPackets(session);
				// Check again whether the session is still open after ScanPackets
				if (session->GetState() != SessionData::Closed) {
					ProcessPackets(session);
					if (session->GetState() != SessionData::Closed) {
						StartAsyncRead(session);
					}
				}
			} else if (error == boost::asio::error::interrupted
					   || error == boost::asio::error::try_again
					   || error == boost::asio::error::would_block) {
				// Transient errors: interrupted (EINTR), try_again (EAGAIN),
				// would_block (EWOULDBLOCK / WSAEWOULDBLOCK on Windows).
				// On Windows + WiFi, would_block can appear when the network
				// stack is temporarily overwhelmed after a power-save resume.
				// Retry the async read instead of closing the connection.
				LOG_ERROR("Session " << session->GetId() << " - recv transient error, retrying: " << error);
				if (session->GetState() != SessionData::Closed) {
					StartAsyncRead(session);
				}
			} else {
				std::string playerInfo;
				if (session->GetPlayerData()) {
					playerInfo = " player=\"" + session->GetPlayerData()->GetName() + "\"";
				}
				// Log the unsent backlog as well. It separates the two very
				// different causes behind an otherwise identical error: a
				// large backlog means we produced data the peer never took
				// (peer stalled, or our egress is saturated), a backlog near
				// zero means the connection itself died.
				LOG_ERROR("Session " << session->GetId() << " (" << session->GetClientAddr() << playerInfo
						  << ") - Connection closed: " << error
						  << " (unsent: " << session->GetSendBuffer().GetPendingBytes() << " bytes)");
				// Keep the cause for the activity log - by the time the lobby
				// closes the session, the error is no longer in reach.
				{
					std::ostringstream reasonStream;
					reasonStream << error;
					session->SetCloseReason(reasonStream.str());
				}
				session->Close();
			}
		} catch (const PokerTHException &) {
			// Re-throw PokerTH exceptions (ClientException, etc.) so they
			// propagate to ClientThread::Main() / ServerLobbyThread which
			// translate them into proper GUI error messages via
			// SignalNetClientError.
			throw;
		} catch (const exception &e) {
			LOG_ERROR("Session " << session->GetId() << " - unhandled exception in HandleRead: " << e.what());
			try {
				session->Close();
			} catch (...) {}
		} catch (...) {
			LOG_ERROR("Session " << (session ? session->GetId() : 0) << " - unknown exception in HandleRead");
			try {
				if (session) session->Close();
			} catch (...) {}
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
		// Was a completely framed packet removed from the buffer? (Also
		// when its content could not be parsed – see below.)
		bool packetConsumed = false;
		// This is necessary, because we use TCP.
		// Packets may be received in multiple chunks or
		// several packets may be received at once.
		if (recvBufUsed >= NET_HEADER_SIZE) {
			// Read the size of the packet (first 4 bytes in network byte order).
			uint32_t nativeVal;
			memcpy(&nativeVal, &recvBuf[0], sizeof(uint32_t));
			size_t packetSize = ntohl(nativeVal);

			// Server hardening: validate the packet size for ALL connections (SSL and non-SSL)
			// Invalid packet sizes point to faulty clients or attacks
			if (packetSize > MAX_PACKET_SIZE || packetSize == 0) {
				LOG_ERROR(session->GetClientAddr() << "Session " << session->GetId()
						  << " - Invalid packet size: " << packetSize << " (max: " << MAX_PACKET_SIZE << ") - closing connection");
				recvBufUsed = 0;
				session->Close();
				return;  // Stop processing immediately
			} else if (recvBufUsed >= packetSize + NET_HEADER_SIZE) {
				try {
					tmpPacket = NetPacket::Create(&recvBuf[NET_HEADER_SIZE], packetSize);
				} catch (const exception &e) {
					// Reset buffer on error.
					LOG_ERROR(session->GetClientAddr() << "Session " << session->GetId() << " - Packet parse error: " << e.what());
					recvBufUsed = 0;
					// On protocol errors: close the session to avoid corrupt states
					session->Close();
					return;
				}
				if (!tmpPacket) {
					// The frame is intact (length checked, packet completely there),
					// only the content could not be parsed – typical for a kind of
					// message that only a newer version knows. That must not
					// paralyse the connection: if the packet stayed in the buffer,
					// every following scan would fail on the same bytes and the
					// session would process nothing at all any more (until the timeout).
					// So discard it and carry on – as in the WebSocket path.
					++unparsablePackets;
					LOG_ERROR(session->GetClientAddr() << "Session " << session->GetId()
							  << " - Unparsable packet of size " << packetSize << " - skipped ("
							  << unparsablePackets << "/" << MAX_UNPARSABLE_PACKETS << ").");
					if (unparsablePackets >= MAX_UNPARSABLE_PACKETS) {
						// Permanently unreadable data is no longer a version difference
						// but a broken client or an attack.
						LOG_ERROR(session->GetClientAddr() << "Session " << session->GetId()
								  << " - Too many unparsable packets - closing connection");
						recvBufUsed = 0;
						session->Close();
						return;
					}
				}
				// Take a validly framed packet out of the buffer in any case.
				recvBufUsed -= (packetSize + NET_HEADER_SIZE);
				if (recvBufUsed) {
					memmove(recvBuf, recvBuf + packetSize + NET_HEADER_SIZE, recvBufUsed);
				}
				packetConsumed = true;
			}
		}
		if (tmpPacket) {
			if (validator.IsValidPacket(*tmpPacket)) {
				receivedPackets.push_back(tmpPacket);
			} else {
				LOG_ERROR(session->GetClientAddr() << "Session " << session->GetId() << " - Invalid packet: " << tmpPacket->GetMsg()->messagetype());
				// On invalid packets: close the session (potential attack or broken implementation)
				recvBufUsed = 0;
				session->Close();
				return;
			}
		} else if (!packetConsumed) {
			// No further complete packet in the buffer. (If one was discarded,
			// scanning continues – behind it there may be valid packets that
			// would otherwise stay there until the next read.)
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

