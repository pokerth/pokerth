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

#include <boost/bind.hpp>
#include <db/serverdbgeneric.h>

using namespace std;


ServerDBGeneric::ServerDBGeneric(ServerDBCallback &cb, boost::shared_ptr<boost::asio::io_service> ioService)
: m_ioService(ioService), m_callback(cb)
{
}

ServerDBGeneric::~ServerDBGeneric()
{
}

void
ServerDBGeneric::Init(const string &/*host*/, const string &/*user*/, const string &/*pwd*/,
					  const string &/*database*/, const string &/*encryptionKey*/)
{
}

void
ServerDBGeneric::Start()
{
}

void
ServerDBGeneric::Stop()
{
}

void
ServerDBGeneric::AsyncPlayerLogin(unsigned requestId, const string &/*playerName*/)
{
	m_ioService->post(boost::bind(&ServerDBCallback::PlayerLoginFailed, &m_callback, requestId));
}

void
ServerDBGeneric::PlayerPostLogin(DB_id /*playerId*/, const std::string &/*avatarHash*/, const std::string &/*avatarType*/)
{
}

void
ServerDBGeneric::PlayerLogout(DB_id /*playerId*/)
{
}

void
ServerDBGeneric::AsyncCreateGame(unsigned requestId, const string &/*gameName*/)
{
	m_ioService->post(boost::bind(&ServerDBCallback::CreateGameFailed, &m_callback, requestId));
}

void
ServerDBGeneric::SetGamePlayerPlace(DB_id /*gameId*/, DB_id /*playerId*/, unsigned /*place*/)
{
}

void
ServerDBGeneric::EndGame(DB_id /*gameId*/)
{
}
