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

#include <db/serverdbgeneric.h>

using namespace std;


ServerDBGeneric::ServerDBGeneric()
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

async_handle
ServerDBGeneric::AsyncPlayerLogin(const string &/*playerName*/, const string &/*secretString*/)
{
	return ASYNC_HANDLE_INVALID;
}

bool
ServerDBGeneric::PlayerLogout(db_id /*playerId*/)
{
	return false;
}

async_handle
ServerDBGeneric::AsyncCreateGame(const db_list &/*players*/)
{
	return ASYNC_HANDLE_INVALID;
}

bool
ServerDBGeneric::SetGamePlayerPlace(db_id /*gameId*/, db_id /*playerId*/, unsigned /*place*/)
{
	return false;
}

bool
ServerDBGeneric::EndGame(db_id /*gameId*/)
{
	return false;
}
