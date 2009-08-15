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


using namespace std;
using boost::asio::ip::tcp;


ChatCleanerManager::ChatCleanerManager(boost::shared_ptr<boost::asio::io_service> ioService)
: m_ioService(ioService), m_connected(false), m_curRequestId(0), m_recvBufUsed(0)
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
	m_clientSecret = clientSecret;
	m_serverSecret = serverSecret;
	if (ipv6)
		m_socket.reset(new boost::asio::ip::tcp::socket(*m_ioService, tcp::v6()));
	else
		m_socket.reset(new boost::asio::ip::tcp::socket(*m_ioService, tcp::v4()));

	ostringstream portStr;
	portStr << port;
	boost::asio::ip::tcp::resolver::query q(serverAddr, portStr.str());

	m_resolver->async_resolve(
		q,
		boost::bind(&ChatCleanerManager::HandleResolve,
		shared_from_this(),
		boost::asio::placeholders::error,
		boost::asio::placeholders::iterator));
}

void
ChatCleanerManager::HandleChatText(unsigned playerId, const std::string &name, const std::string &text)
{
	if (m_connected)
	{
		InternalChatCleanerPacket tmpChat;
		tmpChat.GetMsg()->present = ChatCleanerMessage_PR_cleanerChatRequestMessage;
		CleanerChatRequestMessage_t *netRequest = &tmpChat.GetMsg()->choice.cleanerChatRequestMessage;
		netRequest->requestId = GetNextRequestId();
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
	else if (endpoint_iterator != boost::asio::ip::tcp::resolver::iterator())
	{
		// Try next resolve entry.
		m_socket->close();
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
		m_socket->close();
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

				error = HandleMessage(recvMsg);
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
			m_socket->close();
			m_connected = false;
		}
	}
	else if (ec != boost::asio::error::operation_aborted)
	{
		LOG_ERROR("Error receiving data from chat cleaner.");
		m_socket->close();
		m_connected = false;
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
			}
		}
		if (!m_connected)
			LOG_ERROR("Chat cleaner handshake failed.");
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
