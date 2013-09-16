/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2013 Felix Hammer, Florian Thauer, Lothar May          *
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

#include <net/chatcleanermanager.h>
#include <net/asiosendbuffer.h>
#include <boost/bind.hpp>
#include <core/loghelper.h>
#include <third_party/protobuf/chatcleaner.pb.h>

#include <sstream>

using namespace std;
using boost::asio::ip::tcp;


ChatCleanerManager::ChatCleanerManager(ChatCleanerCallback &cb, boost::shared_ptr<boost::asio::io_service> ioService)
	: m_callback(cb), m_ioService(ioService), m_connected(false), m_curRequestId(0), m_serverPort(0), m_useIpv6(false),
	  m_recvBufUsed(0)
{
	m_recvBuf[0] = 0;
	m_resolver.reset(
		new boost::asio::ip::tcp::resolver(*m_ioService));
	m_sendManager.reset(
		new AsioSendBuffer);
}

ChatCleanerManager::~ChatCleanerManager()
{
}

void
ChatCleanerManager::Init(const string &serverAddr, int port, bool ipv6,
						 const string &clientSecret, const string &serverSecret)
{
	m_serverAddr = serverAddr;
	m_serverPort = port;
	m_useIpv6 = ipv6;
	m_clientSecret = clientSecret;
	m_serverSecret = serverSecret;
	ReInit();
}

void
ChatCleanerManager::ReInit()
{
	if (m_useIpv6)
		m_socket.reset(new boost::asio::ip::tcp::socket(*m_ioService, tcp::v6()));
	else
		m_socket.reset(new boost::asio::ip::tcp::socket(*m_ioService, tcp::v4()));

	ostringstream portStr;
	portStr << m_serverPort;
	boost::asio::ip::tcp::resolver::query q(m_serverAddr, portStr.str());

	m_resolver->async_resolve(
		q,
		boost::bind(&ChatCleanerManager::HandleResolve,
					shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::iterator));
}

void
ChatCleanerManager::HandleLobbyChatText(unsigned playerId, const std::string &name, const std::string &text)
{
	HandleGameChatText(0, playerId, name, text);
}

void
ChatCleanerManager::HandleGameChatText(unsigned gameId, unsigned playerId, const std::string &name, const std::string &text)
{
	if (m_connected) {
		boost::shared_ptr<ChatCleanerMessage> tmpChat(ChatCleanerMessage::default_instance().New());
		tmpChat->set_messagetype(ChatCleanerMessage::Type_CleanerChatRequestMessage);
		CleanerChatRequestMessage *netRequest = tmpChat->mutable_cleanerchatrequestmessage();
		netRequest->set_requestid(GetNextRequestId());
		if (gameId) {
			netRequest->set_cleanerchattype(cleanerChatTypeGame);
			netRequest->set_gameid(gameId);
		} else {
			netRequest->set_cleanerchattype(cleanerChatTypeLobby);
		}
		netRequest->set_playerid(playerId);
		netRequest->set_playername(name);
		netRequest->set_chatmessage(text);
		SendMessageToServer(*tmpChat);
	}
}

void
ChatCleanerManager::HandleResolve(const boost::system::error_code& ec,
								  boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
{
	if (!ec) {
		boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
		m_socket->async_connect(
			endpoint,
			boost::bind(&ChatCleanerManager::HandleConnect,
						shared_from_this(),
						boost::asio::placeholders::error,
						++endpoint_iterator));
	} else if (ec != boost::asio::error::operation_aborted) {
		LOG_ERROR("Could not resolve chat cleaner server.");
	}
}

void
ChatCleanerManager::HandleConnect(const boost::system::error_code& ec,
								  boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
{
	if (!ec) {
		boost::shared_ptr<ChatCleanerMessage> tmpInit(ChatCleanerMessage::default_instance().New());
		tmpInit->set_messagetype(ChatCleanerMessage::Type_CleanerInitMessage);
		CleanerInitMessage *netInit = tmpInit->mutable_cleanerinitmessage();
		netInit->set_requestedversion(CLEANER_PROTOCOL_VERSION);
		netInit->set_clientsecret(m_clientSecret);
		SendMessageToServer(*tmpInit);
		m_socket->async_read_some(
			boost::asio::buffer(m_recvBuf, sizeof(m_recvBuf)),
			boost::bind(
				&ChatCleanerManager::HandleRead,
				shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	} else if (ec != boost::asio::error::operation_aborted) {
		if (endpoint_iterator != boost::asio::ip::tcp::resolver::iterator()) {
			// Try next resolve entry.
			boost::system::error_code ec;
			m_socket->close(ec);
			boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
			m_socket->async_connect(
				endpoint,
				boost::bind(&ChatCleanerManager::HandleConnect,
							shared_from_this(),
							boost::asio::placeholders::error,
							++endpoint_iterator));
		} else
			LOG_ERROR("Could not connect to chat cleaner server.");
	}
}

void
ChatCleanerManager::HandleRead(const boost::system::error_code &ec, size_t bytesRead)
{
	if (!ec) {
		bool error = false;
		m_recvBufUsed += bytesRead;

		bool valid;
		do {
			valid = false;
			if (m_recvBufUsed >= CLEANER_NET_HEADER_SIZE) {
				// Read the size of the packet (first 4 bytes in network byte order).
				uint32_t nativeVal;
				memcpy(&nativeVal, &m_recvBuf[0], sizeof(uint32_t));
				size_t packetSize = ntohl(nativeVal);
				if (packetSize > MAX_CLEANER_PACKET_SIZE) {
					m_recvBufUsed = 0;
					LOG_ERROR("Invalid packet size: " << packetSize);
				} else if (m_recvBufUsed >= packetSize + CLEANER_NET_HEADER_SIZE) {
					try {
						// Try to decode the packet.
						boost::shared_ptr<ChatCleanerMessage> recvMsg(ChatCleanerMessage::default_instance().New());
						if (recvMsg->ParseFromArray(&m_recvBuf[CLEANER_NET_HEADER_SIZE], static_cast<int>(packetSize))) {
							m_recvBufUsed -= (packetSize + CLEANER_NET_HEADER_SIZE);
							if (m_recvBufUsed) {
								memmove(m_recvBuf, m_recvBuf + packetSize + CLEANER_NET_HEADER_SIZE, m_recvBufUsed);
							}
						}
						// Handle the packet.
						error = HandleMessage(*recvMsg);
						valid = true;
					} catch (const exception &e) {
						// Reset buffer on error.
						m_recvBufUsed = 0;
						LOG_ERROR("Exception while decoding packet: " << e.what());
					}
				}
			}
		} while (valid && !error);

		if (!error) {
			m_socket->async_read_some(
				boost::asio::buffer(m_recvBuf + m_recvBufUsed, sizeof(m_recvBuf) - m_recvBufUsed),
				boost::bind(
					&ChatCleanerManager::HandleRead,
					shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		} else {
			boost::system::error_code ec;
			m_socket->close(ec);
			m_connected = false;
		}
	} else if (ec != boost::asio::error::operation_aborted) {
		LOG_ERROR("Error receiving data from chat cleaner.");
		bool wasConnected = m_connected;
		boost::system::error_code ec;
		m_socket->close(ec);
		m_connected = false;
		if (wasConnected)
			ReInit(); // Try to reconnect once if disconnected.
	}
}

bool
ChatCleanerManager::HandleMessage(ChatCleanerMessage &msg)
{
	bool error = true;
	if (msg.messagetype() == ChatCleanerMessage::Type_CleanerInitAckMessage) {
		const CleanerInitAckMessage &netAck = msg.cleanerinitackmessage();
		if (netAck.serverversion() == CLEANER_PROTOCOL_VERSION) {
			if (m_serverSecret == netAck.serversecret()) {
				m_connected = true;
				error = false;
				LOG_MSG("Successfully connected to chat cleaner.");
			}
		}
		if (!m_connected)
			LOG_ERROR("Chat cleaner handshake failed.");
	} else if (msg.messagetype() == ChatCleanerMessage::Type_CleanerChatReplyMessage) {
		const CleanerChatReplyMessage &netReply = msg.cleanerchatreplymessage();
		if (!netReply.cleanertext().empty()) {
			if (netReply.cleanerchattype() == cleanerChatTypeLobby) {
				m_callback.SignalChatBotMessage(netReply.cleanertext());
			} else if (netReply.cleanerchattype() == cleanerChatTypeGame) {
				m_callback.SignalChatBotMessage(netReply.gameid(), netReply.cleanertext());
			}
		}
		if (netReply.cleaneractiontype() == CleanerChatReplyMessage_CleanerActionType_cleanerActionKick)
			m_callback.SignalKickPlayer(netReply.playerid());
		else if (netReply.cleaneractiontype() == CleanerChatReplyMessage_CleanerActionType_cleanerActionBan)
			m_callback.SignalBanPlayer(netReply.playerid());
		else if (netReply.cleaneractiontype() == CleanerChatReplyMessage_CleanerActionType_cleanerActionMute)
			m_callback.SignalMutePlayer(netReply.playerid());
		error = false;
	}
	return error;
}

void
ChatCleanerManager::SendMessageToServer(ChatCleanerMessage &msg)
{
	uint32_t packetSize = msg.ByteSize();
	google::protobuf::uint8 *buf = new google::protobuf::uint8[packetSize + CLEANER_NET_HEADER_SIZE];
	*((uint32_t *)buf) = htonl(packetSize);
	msg.SerializeWithCachedSizesToArray(&buf[CLEANER_NET_HEADER_SIZE]);
	m_sendManager->EncodeToBuf(buf, packetSize + CLEANER_NET_HEADER_SIZE);
	delete[] buf;

	m_sendManager->AsyncSendNextPacket(m_socket);
}

unsigned
ChatCleanerManager::GetNextRequestId()
{
	m_curRequestId++;
	if (m_curRequestId == 0) // 0 is an invalid id.
		m_curRequestId++;

	return m_curRequestId;
}

