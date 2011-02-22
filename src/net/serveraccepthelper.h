/***************************************************************************
 *   Copyright (C) 2007-2009 by Lothar May                                 *
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
/* Network server helper to accept connections. */

#ifndef _SERVERACCEPTHELPER_H_
#define _SERVERACCEPTHELPER_H_

#include <boost/asio.hpp>
#include <string>

#include <net/serverlobbythread.h>
#include <net/serverexception.h>
#include <net/socket_msg.h>
#include <core/loghelper.h>
#include <game_defs.h>
#include <gui/guiinterface.h>

class ServerAcceptInterface
{
public:
	virtual ~ServerAcceptInterface();

	virtual void Listen(unsigned serverPort, bool ipv6, const std::string &logDir,
						boost::shared_ptr<ServerLobbyThread> lobbyThread) = 0;

	virtual void Close() = 0;
};

template <typename P>
class ServerAcceptHelper : public ServerAcceptInterface
{
public:
	typedef typename P::acceptor P_acceptor;
	typedef typename P::endpoint P_endpoint;

	ServerAcceptHelper(ServerCallback &serverCallback, boost::shared_ptr<boost::asio::io_service> ioService)
		: m_ioService(ioService), m_serverCallback(serverCallback)
	{
		m_acceptor.reset(new P_acceptor(*m_ioService));
	}

	virtual ~ServerAcceptHelper()
	{
	}

	// Set the parameters.
	virtual void Listen(unsigned serverPort, bool ipv6, const std::string &/*logDir*/,
						boost::shared_ptr<ServerLobbyThread> lobbyThread)
	{
		m_lobbyThread = lobbyThread;

		try {
			InternalListen(serverPort, ipv6);
		} catch (const PokerTHException &e) {
			LOG_ERROR(e.what());
			GetCallback().SignalNetServerError(e.GetErrorId(), e.GetOsErrorCode());
		} catch (...) {
			// This is probably an asio exception. Assume that bind failed,
			// which is the most frequent case.
			LOG_ERROR("Cannot bind/listen on port.");
			GetCallback().SignalNetServerError(ERR_SOCK_BIND_FAILED, 0);
		}
	}

	virtual void Close()
	{
		boost::system::error_code ec;
		m_acceptor->close(ec);
		// Ignore any error, because we are terminating.
	}
protected:

	void InternalListen(unsigned serverPort, bool ipv6)
	{
		if (serverPort < 1024)
			throw ServerException(__FILE__, __LINE__, ERR_SOCK_INVALID_PORT, 0);

		// TODO consider sctp
		// Prepare Listen.
		if (ipv6) {
			m_endpoint.reset(new P_endpoint(P::v6(), serverPort));
		} else {
			m_endpoint.reset(new P_endpoint(P::v4(), serverPort));
		}

		m_acceptor->open(m_endpoint->protocol());
		m_acceptor->set_option(typename P::acceptor::reuse_address(true));
		if (ipv6) { // In IPv6 mode: Be compatible with IPv4.
			m_acceptor->set_option(boost::asio::ip::v6_only(false));
		}
		m_acceptor->bind(*m_endpoint);
		m_acceptor->listen();

		// Start first asynchronous Accept.
		boost::shared_ptr<typename P::socket> newSocket(new typename P::socket(*m_ioService));
		m_acceptor->async_accept(
			*newSocket,
			boost::bind(&ServerAcceptHelper::HandleAccept, this, newSocket,
						boost::asio::placeholders::error)
		);
	}

	void HandleAccept(boost::shared_ptr<typename P::socket> acceptedSocket,
					  const boost::system::error_code &error)
	{
		if (!error) {
			boost::asio::socket_base::non_blocking_io command(true);
			acceptedSocket->io_control(command);
			acceptedSocket->set_option(typename P::no_delay(true));
			acceptedSocket->set_option(boost::asio::socket_base::keep_alive(true));
			GetLobbyThread().AddConnection(acceptedSocket);

			boost::shared_ptr<typename P::socket> newSocket(new typename P::socket(*m_ioService));
			m_acceptor->async_accept(
				*newSocket,
				boost::bind(&ServerAcceptHelper::HandleAccept, this, newSocket,
							boost::asio::placeholders::error)
			);
		} else {
			// Accept failed. This is a fatal error.
			LOG_ERROR("In boost::asio handler: Accept failed.");
			GetCallback().SignalNetServerError(ERR_SOCK_ACCEPT_FAILED, 0);
		}
	}

	ServerCallback &GetCallback()
	{
		return m_serverCallback;
	}

	ServerLobbyThread &GetLobbyThread()
	{
		return *m_lobbyThread;
	}

private:
	boost::shared_ptr<boost::asio::io_service> m_ioService;
	boost::shared_ptr<P_acceptor> m_acceptor;
	boost::shared_ptr<P_endpoint> m_endpoint;
	ServerCallback &m_serverCallback;

	boost::shared_ptr<ServerLobbyThread> m_lobbyThread;
};

#endif
