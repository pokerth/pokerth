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
/* A manager for avatar files, MD5sums and a cache. */

#ifndef _AVATARMANAGER_H_
#define _AVATARMANAGER_H_

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

#include <map>
#include <list>

#include <playerdata.h>
#include <core/crypthelper.h>
#include <net/netpacket.h>

#define MIN_AVATAR_FILE_SIZE	32
#define MAX_AVATAR_FILE_SIZE	30720

struct AvatarFileState;
class UploaderThread;

class AvatarManager
{
public:

	AvatarManager(bool useExternalServer = false, const std::string &externalServerAddress = "",
				  const std::string &externalServerUser = "", const std::string &externalServerPassword = "");
	~AvatarManager();

	bool Init(const std::string &dataDir, const std::string &cacheDir);

	bool AddSingleAvatar(const std::string &fileName);

	static boost::shared_ptr<AvatarFileState> OpenAvatarFileForChunkRead(const std::string &fileName, unsigned &outFileSize, AvatarFileType &outFileType);
	static unsigned ChunkReadAvatarFile(boost::shared_ptr<AvatarFileState> fileState, unsigned char *data, unsigned chunkSize);

	static int AvatarFileToNetPackets(const std::string &fileName, unsigned requestId, NetPacketList &packets);
	static AvatarFileType GetAvatarFileType(const std::string &fileName);
	static std::string GetAvatarFileExtension(AvatarFileType fileType);

	bool GetHashForAvatar(const std::string &fileName, MD5Buf &md5buf) const;
	bool GetAvatarFileName(const MD5Buf &md5buf, std::string &fileName) const;
	bool HasAvatar(const MD5Buf &md5buf) const;
	bool StoreAvatarInCache(const MD5Buf &md5buf, AvatarFileType avatarFileType, const unsigned char *data, size_t size, bool upload);

	static bool IsValidAvatarFileType(AvatarFileType avatarFileType, const unsigned char *fileHeader, size_t fileHeaderSize);

	void RemoveOldAvatarCacheEntries();

protected:
	typedef std::map<MD5Buf, std::string> AvatarMap;
	typedef std::list<MD5Buf> AvatarList;
	typedef std::map<std::time_t, MD5Buf> TimeAvatarMap;

	bool InternalReadDirectory(const std::string &dir, AvatarMap &avatars);

private:
	mutable boost::mutex	m_avatarsMutex;
	AvatarMap				m_avatars;

	mutable boost::mutex	m_cachedAvatarsMutex;
	AvatarMap				m_cachedAvatars;

	mutable boost::mutex	m_cacheDirMutex;
	std::string				m_cacheDir;

	const bool				m_useExternalServer;
	const std::string		m_externalServerAddress;
	const std::string		m_externalServerUser;
	const std::string		m_externalServerPassword;

	boost::shared_ptr<UploaderThread> m_uploader;
};

#endif
