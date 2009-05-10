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

#include <game_defs.h>
#include <gui/guiinterface.h>

class ServerLobbyThread;

class ServerAcceptHelper
{
public:
	ServerAcceptHelper(ServerCallback &serverCallback, boost::shared_ptr<boost::asio::io_service> ioService);
	virtual ~ServerAcceptHelper();

	// Set the parameters.
	void Listen(unsigned serverPort, bool ipv6, bool sctp, const std::string &pwd, const std::string &logDir,
		boost::shared_ptr<ServerLobbyThread> lobbyThread);

protected:

	void InternalListen(unsigned serverPort, bool ipv6, bool sctp);
	void HandleAccept(boost::shared_ptr<boost::asio::ip::tcp::socket> acceptedSocket,
		const boost::system::error_code& error);

	ServerCallback &GetCallback();

	ServerLobbyThread &GetLobbyThread();

private:
	boost::shared_ptr<boost::asio::io_service> m_ioService;
	boost::shared_ptr<boost::asio::ip::tcp::acceptor> m_acceptor;
	boost::shared_ptr<boost::asio::ip::tcp::endpoint> m_endpoint;
	ServerCallback &m_serverCallback;

	boost::shared_ptr<ServerLobbyThread> m_lobbyThread;
};

#endif
