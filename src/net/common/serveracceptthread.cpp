/***************************************************************************
 *   Copyright (C) 2007 by Lothar May                                      *
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

#include <net/serveracceptthread.h>
#include <net/serverlobbythread.h>
#include <net/serverexception.h>
#include <net/socket_msg.h>
#include <core/loghelper.h>


#define NET_SERVER_LISTEN_BACKLOG	5

using namespace std;
using boost::asio::ip::tcp;

ServerAcceptThread::ServerAcceptThread(ServerCallback &serverCallback, boost::shared_ptr<boost::asio::io_service> ioService)
: m_ioService(ioService), m_serverCallback(serverCallback)
{
	m_acceptor.reset(new tcp::acceptor(*m_ioService));
}

ServerAcceptThread::~ServerAcceptThread()
{
}

void
ServerAcceptThread::Listen(unsigned serverPort, bool ipv6, bool sctp, const string &pwd, const string &logDir, boost::shared_ptr<ServerLobbyThread> lobbyThread)
{
	m_lobbyThread = lobbyThread;

	try
	{
		InternalListen(serverPort, ipv6, sctp);
	}
	catch (const PokerTHException &e)
	{
		LOG_ERROR(e.what());
		GetCallback().SignalNetServerError(e.GetErrorId(), e.GetOsErrorCode());
	}
	catch (...)
	{
		// This is probably an asio exception. Assume that bind failed,
		// which is the most frequent case.
		LOG_ERROR("Cannot bind/listen on TCP port.");
		GetCallback().SignalNetServerError(ERR_SOCK_BIND_FAILED, 0);
	}
}

void
ServerAcceptThread::InternalListen(unsigned serverPort, bool ipv6, bool sctp)
{
	if (serverPort < 1024)
		throw ServerException(__FILE__, __LINE__, ERR_SOCK_INVALID_PORT, 0);

	// TODO consider sctp
	// Prepare Listen.
	if (ipv6)
		m_endpoint.reset(new tcp::endpoint(tcp::v6(), serverPort));
	else
		m_endpoint.reset(new tcp::endpoint(tcp::v4(), serverPort));

	// TODO use non blocking I/O
	//boost::asio::socket_base::non_blocking_io command(true);
	//m_acceptor->io_control(command);
	m_acceptor->open(m_endpoint->protocol());
	m_acceptor->set_option(tcp::acceptor::reuse_address(true));
	if (ipv6) // In IPv6 mode: Be compatible with IPv4.
		m_acceptor->set_option(boost::asio::ip::v6_only(false));
	m_acceptor->bind(*m_endpoint);
	m_acceptor->listen(NET_SERVER_LISTEN_BACKLOG);

	// Start first asynchronous Accept.
	boost::shared_ptr<tcp::socket> newSocket(new tcp::socket(*m_ioService));
	m_acceptor->async_accept(
		*newSocket,
		boost::bind(&ServerAcceptThread::HandleAccept, this, newSocket,
			boost::asio::placeholders::error)
		);
}

void
ServerAcceptThread::HandleAccept(boost::shared_ptr<boost::asio::ip::tcp::socket> acceptedSocket,
								 const boost::system::error_code& error)
{
	if (!error)
	{
		boost::asio::socket_base::non_blocking_io command(true);
		acceptedSocket->io_control(command);
		acceptedSocket->set_option(tcp::no_delay(true));
		acceptedSocket->set_option(boost::asio::socket_base::keep_alive(true));
		GetLobbyThread().AddConnection(acceptedSocket);

		boost::shared_ptr<tcp::socket> newSocket(new tcp::socket(*m_ioService));
		m_acceptor->async_accept(
			*newSocket,
			boost::bind(&ServerAcceptThread::HandleAccept, this, newSocket,
				boost::asio::placeholders::error)
			);
	}
	else
	{
		// Accept failed. This is a fatal error.
		LOG_ERROR("In boost::asio handler: Accept failed.");
		GetCallback().SignalNetServerError(ERR_SOCK_ACCEPT_FAILED, 0);
	}
}

ServerCallback &
ServerAcceptThread::GetCallback()
{
	return m_serverCallback;
}

ServerLobbyThread &
ServerAcceptThread::GetLobbyThread()
{
	assert(m_lobbyThread.get());
	return *m_lobbyThread;
}

