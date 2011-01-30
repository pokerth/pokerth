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

#include <net/chatcleanermanager.h>
#include <net/internalchatcleanerpacket.h>
#include <net/encodedpacket.h>
#include <boost/bind.hpp>
#include <core/loghelper.h>
#include <third_party/asn1/ChatCleanerMessage.h>

#include <sstream>

using namespace std;
using boost::asio::ip::tcp;


ChatCleanerManager::ChatCleanerManager(ChatCleanerCallback &cb, boost::shared_ptr<boost::asio::io_service> ioService)
: m_callback(cb), m_ioService(ioService), m_connected(false), m_curRequestId(0), m_recvBufUsed(0)
{
	m_resolver.reset(
		new boost::asio::ip::tcp::resolver(*m_ioService));
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
	if (m_connected)
	{
		InternalChatCleanerPacket tmpChat;
		tmpChat.GetMsg()->present = ChatCleanerMessage_PR_cleanerChatRequestMessage;
		CleanerChatRequestMessage_t *netRequest = &tmpChat.GetMsg()->choice.cleanerChatRequestMessage;
		netRequest->requestId = GetNextRequestId();
		if (gameId)
		{
			netRequest->cleanerChatType.present = CleanerChatType_PR_cleanerChatTypeGame;
			netRequest->cleanerChatType.choice.cleanerChatTypeGame.gameId = gameId;
		}
		else
		{
			netRequest->cleanerChatType.present = CleanerChatType_PR_cleanerChatTypeLobby;
		}
		netRequest->playerId = playerId;
		OCTET_STRING_fromBuf(&netRequest->playerName,
							 name.c_str(),
							 name.length());
		OCTET_STRING_fromBuf(&netRequest->chatMessage,
							 text.c_str(),
							 text.length());
		SendMessageToServer(tmpChat);
	}
}

void
ChatCleanerManager::HandleResolve(const boost::system::error_code& ec,
								  boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
{
	if (!ec)
	{
		boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
		m_socket->async_connect(
			endpoint,
			boost::bind(&ChatCleanerManager::HandleConnect,
				shared_from_this(),
				boost::asio::placeholders::error,
				++endpoint_iterator));
	}
	else if (ec != boost::asio::error::operation_aborted)
	{
		LOG_ERROR("Could not resolve chat cleaner server.");
	}
}

void
ChatCleanerManager::HandleConnect(const boost::system::error_code& ec,
								  boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
{
	if (!ec)
	{
		InternalChatCleanerPacket tmpInit;
		tmpInit.GetMsg()->present = ChatCleanerMessage_PR_cleanerInitMessage;
		CleanerInitMessage_t *netInit = &tmpInit.GetMsg()->choice.cleanerInitMessage;
		netInit->requestedVersion = CLEANER_PROTOCOL_VERSION;
		OCTET_STRING_fromBuf(&netInit->clientSecret,
							 m_clientSecret.c_str(),
							 m_clientSecret.length());
		SendMessageToServer(tmpInit);
		m_socket->async_read_some(
			boost::asio::buffer(m_recvBuf, sizeof(m_recvBuf)),
			boost::bind(
				&ChatCleanerManager::HandleRead,
				shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	}
	else if (ec != boost::asio::error::operation_aborted)
	{
		if (endpoint_iterator != boost::asio::ip::tcp::resolver::iterator())
		{
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
		}
		else
			LOG_ERROR("Could not connect to chat cleaner server.");
	}
}

void
ChatCleanerManager::HandleWrite(const boost::system::error_code &ec,
								boost::shared_ptr<EncodedPacket> /*tmpPacket*/)
{
	if (ec && ec != boost::asio::error::operation_aborted)
	{
		LOG_ERROR("Error sending message to chat cleaner.");
		boost::system::error_code ec;
		m_socket->close(ec);
		m_connected = false;
	}
}

void
ChatCleanerManager::HandleRead(const boost::system::error_code &ec, size_t bytesRead)
{
	if (!ec)
	{
		bool error = false;
		m_recvBufUsed += bytesRead;

		asn_dec_rval_t retVal;
		do
		{
			InternalChatCleanerPacket recvMsg;
			retVal = ber_decode(0, &asn_DEF_ChatCleanerMessage, (void **)recvMsg.GetMsgPtr(), m_recvBuf, m_recvBufUsed);
			if(retVal.code == RC_OK)
			{
				// Consume the bytes.
				if (retVal.consumed < m_recvBufUsed)
				{
					m_recvBufUsed -= retVal.consumed;
					memmove(m_recvBuf, m_recvBuf + retVal.consumed, m_recvBufUsed);
				}
				else
					m_recvBufUsed = 0;

				if (asn_check_constraints(&asn_DEF_ChatCleanerMessage, recvMsg.GetMsg(), NULL, NULL) == 0)
					error = HandleMessage(recvMsg);
				else
					LOG_ERROR("Received invalid chat cleaner packet.");
			}
		} while (!error && retVal.code == RC_OK);

		if (!error)
		{
			m_socket->async_read_some(
				boost::asio::buffer(m_recvBuf + m_recvBufUsed, sizeof(m_recvBuf) - m_recvBufUsed),
				boost::bind(
					&ChatCleanerManager::HandleRead,
					shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
		else
		{
			boost::system::error_code ec;
			m_socket->close(ec);
			m_connected = false;
		}
	}
	else if (ec != boost::asio::error::operation_aborted)
	{
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
ChatCleanerManager::HandleMessage(InternalChatCleanerPacket &msg)
{
	bool error = true;
	if (msg.GetMsg()->present == ChatCleanerMessage_PR_cleanerInitAckMessage)
	{
		CleanerInitAckMessage_t *netAck = &msg.GetMsg()->choice.cleanerInitAckMessage;
		if (netAck->serverVersion == CLEANER_PROTOCOL_VERSION)
		{
			string tmpSecret((const char *)netAck->serverSecret.buf, netAck->serverSecret.size);
			if (m_serverSecret == tmpSecret)
			{
				m_connected = true;
				error = false;
				LOG_MSG("Successfully connected to chat cleaner.");
			}
		}
		if (!m_connected)
			LOG_ERROR("Chat cleaner handshake failed.");
	}
	else if (msg.GetMsg()->present == ChatCleanerMessage_PR_cleanerChatReplyMessage)
	{
		CleanerChatReplyMessage_t *netReply = &msg.GetMsg()->choice.cleanerChatReplyMessage;
		if (netReply->cleanerText)
		{
			if (netReply->cleanerChatType.present == CleanerChatType_PR_cleanerChatTypeLobby)
			{
				m_callback.SignalChatBotMessage(string((const char *)netReply->cleanerText->buf, netReply->cleanerText->size));
			}
			else if (netReply->cleanerChatType.present == CleanerChatType_PR_cleanerChatTypeGame)
			{
				m_callback.SignalChatBotMessage(netReply->cleanerChatType.choice.cleanerChatTypeGame.gameId, string((const char *)netReply->cleanerText->buf, netReply->cleanerText->size));
			}
		}
		if (netReply->cleanerActionType == cleanerActionType_cleanerActionKick)
			m_callback.SignalKickPlayer(netReply->playerId);
		else if (netReply->cleanerActionType == cleanerActionType_cleanerActionBan)
			m_callback.SignalBanPlayer(netReply->playerId);
		error = false;
	}
	return error;
}

void
ChatCleanerManager::SendMessageToServer(InternalChatCleanerPacket &msg)
{
	unsigned char buf[MAX_CLEANER_PACKET_SIZE];
	asn_enc_rval_t e = der_encode_to_buffer(&asn_DEF_ChatCleanerMessage, msg.GetMsg(), buf, MAX_CLEANER_PACKET_SIZE);

	if (e.encoded == -1)
		LOG_ERROR("Failed to encode chat cleaner packet: " << msg.GetMsg()->present);
	else
	{
		boost::shared_ptr<EncodedPacket> tmpPacket(new EncodedPacket(buf, e.encoded));
		// Actually, this should not be done (parallel async_write calls might break data).
		// But we do not want to create additional buffers here.
		boost::asio::async_write(
			*m_socket,
			boost::asio::buffer(tmpPacket->GetData(), tmpPacket->GetSize()),
			boost::bind(&ChatCleanerManager::HandleWrite,
				shared_from_this(),
				boost::asio::placeholders::error,
				tmpPacket));
	}
}

unsigned
ChatCleanerManager::GetNextRequestId()
{
	m_curRequestId++;
	if (m_curRequestId == 0) // 0 is an invalid id.
		m_curRequestId++;

	return m_curRequestId;
}

