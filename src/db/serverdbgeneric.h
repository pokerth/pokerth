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
/* Generic server database implementation. Basically does nothing. */

#ifndef _SERVERDBGENERIC_H_
#define _SERVERDBGENERIC_H_

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <db/serverdbinterface.h>


class ServerDBGeneric : public ServerDBInterface
{
public:
	ServerDBGeneric(ServerDBCallback &cb, boost::shared_ptr<boost::asio::io_service> ioService);
	virtual ~ServerDBGeneric();

	virtual void Init(const std::string &host, const std::string &user, const std::string &pwd,
					  const std::string &database, const std::string &encryptionKey);

	virtual void Start();
	virtual void Stop();

	virtual void AsyncPlayerLogin(unsigned requestId, const std::string &playerName, const std::string &secretString);
	virtual void PlayerLogout(db_id playerId);

	virtual void AsyncCreateGame(unsigned requestId, const std::string &gameName);
	virtual void SetGamePlayerPlace(db_id gameId, db_id playerId, unsigned place);
	virtual void EndGame(db_id gameId);

private:
	boost::shared_ptr<boost::asio::io_service> m_ioService;
	ServerDBCallback &m_callback;
};

#endif
