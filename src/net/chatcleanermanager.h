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
/* Manager which acts as client for the chat cleaner service. */

#ifndef _CHATCLEANERMANAGER_H_
#define _CHATCLEANERMANAGER_H_

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <string>

#define CLEANER_PROTOCOL_VERSION		1
#define MAX_CLEANER_PACKET_SIZE			384

class InternalChatCleanerMessage;
class EncodedPacket;

class ChatCleanerManager : public boost::enable_shared_from_this<ChatCleanerManager>
{
public:
	ChatCleanerManager(boost::shared_ptr<boost::asio::io_service> ioService);
	virtual ~ChatCleanerManager();

	void Init(const std::string &serverAddr, int port, bool ipv6,
			  const std::string &clientSecret, const std::string &serverSecret);
	void HandleChatText(unsigned playerId, const std::string &name, const std::string &text);

protected:

	void HandleResolve(const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::iterator endpoint_iterator);
	void HandleConnect(const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::iterator endpoint_iterator);
	void HandleWrite(const boost::system::error_code &ec, boost::shared_ptr<EncodedPacket> tmpPacket);
	void HandleRead(const boost::system::error_code &ec, size_t bytesRead);
	bool HandleMessage(InternalChatCleanerMessage &msg);

	void SendMessageToServer(InternalChatCleanerMessage &msg);
	unsigned GetNextRequestId();

private:

	boost::shared_ptr<boost::asio::io_service> m_ioService;
	boost::shared_ptr<boost::asio::ip::tcp::resolver> m_resolver;
	boost::shared_ptr<boost::asio::ip::tcp::socket> m_socket;

	bool m_connected;
	unsigned m_curRequestId;
	std::string m_clientSecret;
	std::string m_serverSecret;
	unsigned char m_recvBuf[2*MAX_CLEANER_PACKET_SIZE];
	unsigned m_recvBufUsed;
};

#endif
