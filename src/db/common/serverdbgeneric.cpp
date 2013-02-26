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
ServerDBGeneric::AsyncCheckAvatarBlacklist(unsigned requestId, const std::string &/*avatarHash*/)
{
	m_ioService->post(boost::bind(&ServerDBCallback::AvatarIsBlacklisted, &m_callback, requestId));
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
ServerDBGeneric::SetGamePlayerPlace(unsigned /*requestId*/, DB_id /*playerId*/, unsigned /*place*/)
{
}

void
ServerDBGeneric::EndGame(unsigned /*requestId*/)
{
}

void
ServerDBGeneric::AsyncReportAvatar(unsigned requestId, unsigned replyId, DB_id /*reportedPlayerId*/, const std::string &/*avatarHash*/, const std::string &/*avatarType*/, DB_id * /*byPlayerId*/)
{
	m_ioService->post(boost::bind(&ServerDBCallback::ReportAvatarFailed, &m_callback, requestId, replyId));
}

void
ServerDBGeneric::AsyncReportGame(unsigned requestId, unsigned replyId, DB_id * /*creatorPlayerId*/, unsigned /*gameId*/, const std::string &/*gameName*/, DB_id * /*byPlayerId*/)
{
	m_ioService->post(boost::bind(&ServerDBCallback::ReportGameFailed, &m_callback, requestId, replyId));
}

void
ServerDBGeneric::AsyncQueryAdminPlayers(unsigned /*requestId*/)
{
}

void
ServerDBGeneric::AsyncBlockPlayer(unsigned /*requestId*/, unsigned /*replyId*/, DB_id /*playerId*/, int /*valid*/, int /*active*/)
{
}

