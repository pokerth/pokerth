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

enum PlayerType
{
	PLAYER_TYPE_COMPUTER,
	PLAYER_TYPE_HUMAN
};

enum PlayerRights
{
	PLAYER_RIGHTS_NORMAL,
	PLAYER_RIGHTS_ADMIN
};

enum AvatarFileType
{
	AVATAR_FILE_TYPE_UNKNOWN = 0,
	AVATAR_FILE_TYPE_PNG,
	AVATAR_FILE_TYPE_JPG,
	AVATAR_FILE_TYPE_GIF
};

struct AvatarFile
{
	AvatarFile() : fileType(AVATAR_FILE_TYPE_UNKNOWN), reportedSize(0) {}
	std::vector<unsigned char>	fileData;
	AvatarFileType				fileType;
	unsigned					reportedSize;
};

struct PlayerInfo
{
	PlayerInfo() : ptype(PLAYER_TYPE_HUMAN), hasAvatar(false), avatarType(AVATAR_FILE_TYPE_UNKNOWN) {}
	std::string		playerName;
	PlayerType		ptype;
	bool			hasAvatar;
	MD5Buf			avatar;
	AvatarFileType	avatarType;
};

class PlayerData
{
public:
	PlayerData(unsigned uniqueId, int number, PlayerType type, PlayerRights rights);
	PlayerData(const PlayerData &other);
	~PlayerData();

	std::string GetName() const;
	void SetName(const std::string &name);
	std::string GetPassword() const;
	void SetPassword(const std::string &password);
	std::string GetAvatarFile() const;
	void SetAvatarFile(const std::string &avatarFile);
	MD5Buf GetAvatarMD5() const;
	void SetAvatarMD5(const MD5Buf &avatarMD5);
	boost::shared_ptr<SessionData> GetNetSessionData() const;
	void SetNetSessionData(boost::shared_ptr<SessionData> session);
	boost::shared_ptr<AvatarFile> GetNetAvatarFile() const;
	void SetNetAvatarFile(boost::shared_ptr<AvatarFile> AvatarFile);
	PlayerType GetType() const;
	void SetType(PlayerType type);
	PlayerRights GetRights() const;
	void SetRights(PlayerRights rights);
	unsigned GetUniqueId() const;
	int GetNumber() const;
	void SetNumber(int number);
	DB_id GetDBId() const;
	void SetDBId(DB_id id);

	bool operator<(const PlayerData &other) const;

private:
	const unsigned					m_uniqueId;
	DB_id							m_dbId;
	int								m_number;
	std::string						m_name;
	std::string						m_password;
	std::string						m_avatarFile;
	MD5Buf							m_avatarMD5;
	PlayerType						m_type;
	PlayerRights					m_rights;
	boost::shared_ptr<SessionData>	m_netSessionData;
	boost::shared_ptr<AvatarFile>	m_netAvatarFile;

	mutable boost::mutex			m_dataMutex;
};

typedef std::list<std::pair<unsigned, unsigned> > RemovePlayerList;
typedef std::list<boost::shared_ptr<PlayerData> > PlayerDataList;
typedef std::map<unsigned, boost::shared_ptr<PlayerData> > PlayerDataMap;

#endif

