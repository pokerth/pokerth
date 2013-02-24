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
/* Player data. */

#ifndef _PLAYERDATA_H_
#define _PLAYERDATA_H_

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <core/crypthelper.h>
#include <db/serverdbcallback.h>

class SessionData;

enum PlayerType {
	PLAYER_TYPE_COMPUTER,
	PLAYER_TYPE_HUMAN
};

enum PlayerRights {
	PLAYER_RIGHTS_GUEST = 1,
	PLAYER_RIGHTS_NORMAL,
	PLAYER_RIGHTS_ADMIN
};

enum AvatarFileType {
	AVATAR_FILE_TYPE_UNKNOWN = 0,
	AVATAR_FILE_TYPE_PNG,
	AVATAR_FILE_TYPE_JPG,
	AVATAR_FILE_TYPE_GIF
};

struct AvatarFile {
	AvatarFile() : fileType(AVATAR_FILE_TYPE_UNKNOWN), reportedSize(0) {}
	std::vector<unsigned char>	fileData;
	AvatarFileType				fileType;
	size_t						reportedSize;
};

struct PlayerInfo {
	PlayerInfo() : ptype(PLAYER_TYPE_HUMAN), isGuest(false), isAdmin(false), hasAvatar(false), avatarType(AVATAR_FILE_TYPE_UNKNOWN) {}
	std::string		playerName;
	PlayerType		ptype;
	bool			isGuest;
	bool			isAdmin;
	std::string		countryCode;
	bool			hasAvatar;
	MD5Buf			avatar;
	AvatarFileType	avatarType;
};

class PlayerData
{
public:
	PlayerData(unsigned uniqueId, int number, PlayerType type, PlayerRights rights, bool isGameAdmin);
	PlayerData(const PlayerData &other);
	~PlayerData();

	std::string GetName() const;
	void SetName(const std::string &name);
	std::string GetCountry() const;
	void SetCountry(const std::string &country);
	std::string GetAvatarFile() const;
	void SetAvatarFile(const std::string &avatarFile);
	MD5Buf GetAvatarMD5() const;
	void SetAvatarMD5(const MD5Buf &avatarMD5);
	boost::shared_ptr<AvatarFile> GetNetAvatarFile() const;
	void SetNetAvatarFile(boost::shared_ptr<AvatarFile> AvatarFile);
	PlayerType GetType() const;
	void SetType(PlayerType type);
	PlayerRights GetRights() const;
	void SetRights(PlayerRights rights);
	bool IsGameAdmin() const;
	void SetGameAdmin(bool isAdmin);
	unsigned GetUniqueId() const;
	int GetNumber() const;
	void SetNumber(int number);
	std::string GetGuid() const;
	void SetGuid(const std::string &guid);
	std::string GetOldGuid() const;
	void SetOldGuid(const std::string &guid);
	DB_id GetDBId() const;
	void SetDBId(DB_id id);
	int GetStartCash() const;
	void SetStartCash(int cash);

	bool operator<(const PlayerData &other) const;

private:
	const unsigned					m_uniqueId;
	DB_id							m_dbId;
	int								m_number;
	int								m_startCash; // only used if > 0
	std::string						m_guid;
	std::string						m_oldGuid;
	std::string						m_name;
	std::string						m_password;
	std::string						m_country;
	std::string						m_avatarFile;
	MD5Buf							m_avatarMD5;
	PlayerType						m_type;
	PlayerRights					m_rights;
	bool							m_isGameAdmin;
	boost::shared_ptr<AvatarFile>	m_netAvatarFile;

	mutable boost::mutex			m_dataMutex;
};

typedef std::list<std::pair<unsigned, unsigned> > RemovePlayerList;
typedef std::list<boost::shared_ptr<PlayerData> > PlayerDataList;
typedef std::map<unsigned, boost::shared_ptr<PlayerData> > PlayerDataMap;

#endif

