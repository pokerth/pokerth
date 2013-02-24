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
#include <playerdata.h>

using namespace std;

PlayerData::PlayerData(unsigned uniqueId, int number, PlayerType type, PlayerRights rights, bool isGameAdmin)
	: m_uniqueId(uniqueId), m_dbId(DB_ID_INVALID), m_number(number), m_startCash(-1), m_type(type), m_rights(rights), m_isGameAdmin(isGameAdmin)
{
}

PlayerData::PlayerData(const PlayerData &other)
	: m_uniqueId(other.GetUniqueId()), m_dbId(other.GetDBId()), m_number(other.GetNumber()), m_startCash(other.GetStartCash()),
	  m_guid(other.GetGuid()), m_oldGuid(other.GetOldGuid()), m_name(other.GetName()), m_password(), m_country(other.GetCountry()),
	  m_avatarFile(other.GetAvatarFile()), m_avatarMD5(other.GetAvatarMD5()), m_type(other.GetType()), m_rights(other.GetRights()),
	  m_isGameAdmin(other.IsGameAdmin()), m_netAvatarFile(), m_dataMutex()
{
}

PlayerData::~PlayerData()
{
}

string
PlayerData::GetName() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_name;
}

void
PlayerData::SetName(const string &name)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_name = name;
}

std::string
PlayerData::GetCountry() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_country;
}

void
PlayerData::SetCountry(const std::string &country)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_country = country;
}

string
PlayerData::GetAvatarFile() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_avatarFile;
}

void
PlayerData::SetAvatarFile(const std::string &avatarFile)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_avatarFile = avatarFile;
}

MD5Buf
PlayerData::GetAvatarMD5() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_avatarMD5;
}
void
PlayerData::SetAvatarMD5(const MD5Buf &avatarMD5)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_avatarMD5 = avatarMD5;
}

boost::shared_ptr<AvatarFile>
PlayerData::GetNetAvatarFile() const
{
	return m_netAvatarFile;
}

void
PlayerData::SetNetAvatarFile(boost::shared_ptr<AvatarFile> AvatarFile)
{
	m_netAvatarFile = AvatarFile;
}

PlayerType
PlayerData::GetType() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_type;
}

void
PlayerData::SetType(PlayerType type)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_type = type;
}

PlayerRights
PlayerData::GetRights() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_rights;
}

void
PlayerData::SetRights(PlayerRights rights)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_rights = rights;
}

bool
PlayerData::IsGameAdmin() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_isGameAdmin;
}

void
PlayerData::SetGameAdmin(bool isAdmin)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_isGameAdmin = isAdmin;
}

unsigned
PlayerData::GetUniqueId() const
{
	// const value - no mutex needed.
	return m_uniqueId;
}

int
PlayerData::GetNumber() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_number;
}

void
PlayerData::SetNumber(int number)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_number = number;
}

std::string
PlayerData::GetGuid() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_guid;
}

void
PlayerData::SetGuid(const std::string &guid)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_guid = guid;
}

std::string
PlayerData::GetOldGuid() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_oldGuid;
}

void
PlayerData::SetOldGuid(const std::string &guid)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_oldGuid = guid;
}

DB_id
PlayerData::GetDBId() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_dbId;
}

void
PlayerData::SetDBId(DB_id id)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_dbId = id;
}

int
PlayerData::GetStartCash() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_startCash;
}

void
PlayerData::SetStartCash(int cash)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_startCash = cash;
}

bool
PlayerData::operator<(const PlayerData &other) const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_number < other.GetNumber();
}

