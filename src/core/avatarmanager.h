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
/* A manager for avatar files, MD5sums and a cache. */

#ifndef _AVATARMANAGER_H_
#define _AVATARMANAGER_H_

#include <playerdata.h>
#include <net/netpacket.h>
#include <core/crypthelper.h>
#include <map>

#include <boost/shared_ptr.hpp>

#define MAX_AVATAR_FILE_SIZE	30720

struct AvatarFileState;

class AvatarManager
{
public:

	AvatarManager();
	~AvatarManager();

	bool Init(const std::string &dataDir, const std::string &cacheDir);

	boost::shared_ptr<AvatarFileState> OpenAvatarFileForChunkRead(const std::string &fileName, unsigned &outFileSize, AvatarFileType &outFileType);
	unsigned ChunkReadAvatarFile(boost::shared_ptr<AvatarFileState> fileState, unsigned char *data, unsigned chunkSize);

	bool AvatarFileToNetPackets(const std::string &fileName, unsigned requestId, NetPacketList &packets);

	bool GetHashForAvatar(const std::string &fileName, MD5Buf &md5buf);
	bool GetAvatarFileName(const MD5Buf &md5buf, std::string &fileName) const;
	bool StoreAvatarInCache(const MD5Buf &md5buf, AvatarFileType avatarFileType, const unsigned char *data, unsigned size);

protected:
	typedef std::map<MD5Buf, std::string> AvatarMap;

	void InternalReadDirectory(const std::string &dir);

private:
	AvatarMap			m_avatars;

	std::string			m_cacheDir;
};

#endif
