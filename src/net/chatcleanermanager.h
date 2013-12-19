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
/* Manager which acts as client for the chat cleaner service. */

#ifndef _CHATCLEANERMANAGER_H_
#define _CHATCLEANERMANAGER_H_

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <string>
#include <net/chatcleanercallback.h>

#define CLEANER_NET_HEADER_SIZE		4
#define MAX_CLEANER_PACKET_SIZE		512
#define CLEANER_PROTOCOL_VERSION	2

class AsioSendBuffer;
class ChatCleanerMessage;

class ChatCleanerManager : public boost::enable_shared_from_this<ChatCleanerManager>
{
public:
	ChatCleanerManager(ChatCleanerCallback &cb, boost::shared_ptr<boost::asio::io_service> ioService);
	virtual ~ChatCleanerManager();

	void Init(const std::string &serverAddr, int port, bool ipv6,
			  const std::string &clientSecret, const std::string &serverSecret);
	void ReInit();
	void HandleLobbyChatText(unsigned playerId, const std::string &name, const std::string &text);
	void HandleGameChatText(unsigned gameId, unsigned playerId, const std::string &name, const std::string &text);

protected:

	void HandleResolve(const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::iterator endpoint_iterator);
	void HandleConnect(const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::iterator endpoint_iterator);
	void HandleRead(const boost::system::error_code &ec, size_t bytesRead);
	bool HandleMessage(ChatCleanerMessage &msg);

	void SendMessageToServer(ChatCleanerMessage &msg);
	unsigned GetNextRequestId();

private:

	ChatCleanerCallback &m_callback;
	boost::shared_ptr<boost::asio::io_service> m_ioService;
	boost::shared_ptr<boost::asio::ip::tcp::resolver> m_resolver;
	boost::shared_ptr<boost::asio::ip::tcp::socket> m_socket;
	boost::shared_ptr<AsioSendBuffer> m_sendManager;

	bool m_connected;
	unsigned m_curRequestId;
	std::string m_serverAddr;
	int m_serverPort;
	bool m_useIpv6;
	std::string m_clientSecret;
	std::string m_serverSecret;
	unsigned char m_recvBuf[2*MAX_CLEANER_PACKET_SIZE];
	size_t m_recvBufUsed;
};

#endif
