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

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

#define MIN_AVATAR_FILE_SIZE	32
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

	int AvatarFileToNetPackets(const std::string &fileName, unsigned requestId, NetPacketList &packets);

	bool GetHashForAvatar(const std::string &fileName, MD5Buf &md5buf) const;
	bool GetAvatarFileName(const MD5Buf &md5buf, std::string &fileName) const;
	bool HasAvatar(const MD5Buf &md5buf) const;
	bool StoreAvatarInCache(const MD5Buf &md5buf, AvatarFileType avatarFileType, const unsigned char *data, unsigned size);

	static bool IsValidAvatarFileType(AvatarFileType avatarFileType, const unsigned char *fileHeader, unsigned fileHeaderSize);

protected:
	typedef std::map<MD5Buf, std::string> AvatarMap;

	bool InternalReadDirectory(const std::string &dir, AvatarMap &avatars);

private:
	mutable boost::mutex	m_avatarsMutex;
	AvatarMap				m_avatars;

	mutable boost::mutex	m_cachedAvatarsMutex;
	AvatarMap				m_cachedAvatars;

	mutable boost::mutex	m_cacheDirMutex;
	std::string				m_cacheDir;
};

#endif
