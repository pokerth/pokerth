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

#include "avatarmanager.h"

#include <boost/filesystem.hpp>
#include <boost/lambda/lambda.hpp>
#include <openssl/md5.h>
#include <net/socket_msg.h>

#include <fstream>
#include <cstring>

#define PNG_HEADER "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a"
#define PNG_HEADER_SIZE (sizeof(PNG_HEADER) - 1)
#define JPG_HEADER "\xff\xd8"
#define JPG_HEADER_SIZE (sizeof(JPG_HEADER) - 1)
#define GIF_HEADER_1 "GIF87a"
#define GIF_HEADER_2 "GIF89a"
#define GIF_HEADER_SIZE (sizeof(GIF_HEADER_1) - 1)
#define MAX_HEADER_SIZE PNG_HEADER_SIZE

// Not using boost::algorithm here because of STL issues.
#ifdef _MSC_VER
#define STRCASECMP _stricmp
#else
#define STRCASECMP strcasecmp
#endif

using namespace std;
using namespace boost::filesystem;

struct AvatarFileState
{
	ifstream		inputStream;
};

AvatarManager::AvatarManager()
{
}

AvatarManager::~AvatarManager()
{
}

bool
AvatarManager::Init(const std::string &dataDir, const std::string &cacheDir)
{
	bool retVal = true;
	bool tmpRet;
	{
		boost::mutex::scoped_lock lock(m_cacheDirMutex);
		m_cacheDir = cacheDir;
	}
	{
		boost::mutex::scoped_lock lock(m_avatarsMutex);
		tmpRet = InternalReadDirectory(dataDir + "gfx/avatars/default/people/", m_avatars);
		retVal = retVal && tmpRet;
		tmpRet = InternalReadDirectory(dataDir + "gfx/avatars/default/misc/", m_avatars);
		retVal = retVal && tmpRet;
	}
	{
		boost::mutex::scoped_lock lock(m_cachedAvatarsMutex);
		tmpRet = InternalReadDirectory(cacheDir, m_cachedAvatars);
		retVal = retVal && tmpRet;
	}
	return retVal;
}

boost::shared_ptr<AvatarFileState>
AvatarManager::OpenAvatarFileForChunkRead(const std::string &fileName, unsigned &outFileSize, AvatarFileType &outFileType)
{
	outFileSize = 0;
	outFileType = AVATAR_FILE_TYPE_UNKNOWN;
	boost::shared_ptr<AvatarFileState> retVal;
	try
	{
		path filePath(fileName);
		string ext(extension(filePath));
		if (STRCASECMP(ext.c_str(), ".png") == 0)
			outFileType = AVATAR_FILE_TYPE_PNG;
		else if (STRCASECMP(ext.c_str(), ".jpg") == 0 || STRCASECMP(ext.c_str(), ".jpeg") == 0)
			outFileType = AVATAR_FILE_TYPE_JPG;
		else if (STRCASECMP(ext.c_str(), ".gif") == 0)
			outFileType = AVATAR_FILE_TYPE_GIF;
		boost::shared_ptr<AvatarFileState> fileState(new AvatarFileState);
		fileState->inputStream.open(fileName.c_str(), ios_base::in | ios_base::binary);
		if (!fileState->inputStream.fail())
		{
			// Find out file size.
			// Not fully portable, but works on win/linux/mac.
			fileState->inputStream.seekg(0, ios_base::beg);
			std::streampos startPos = fileState->inputStream.tellg();
			fileState->inputStream.seekg(0, ios_base::end);
			std::streampos endPos = fileState->inputStream.tellg();
			fileState->inputStream.seekg(0, ios_base::beg);
			std::streamoff posDiff(endPos - startPos);
			outFileSize = (unsigned)posDiff;
			if (outFileSize >= MIN_AVATAR_FILE_SIZE && outFileSize <= MAX_AVATAR_FILE_SIZE)
			{
				// Validate type of file by verifying image header.
				unsigned char fileHeader[MAX_HEADER_SIZE];
				fileState->inputStream.read((char *)fileHeader, sizeof(fileHeader));
				fileState->inputStream.seekg(0, ios_base::beg);

				if (IsValidAvatarFileType(outFileType, fileHeader, sizeof(fileHeader)))
					retVal = fileState;
			}
		}
	} catch (...)
	{
	}
	return retVal;
}

unsigned
AvatarManager::ChunkReadAvatarFile(boost::shared_ptr<AvatarFileState> fileState, unsigned char *data, unsigned chunkSize)
{
	unsigned retVal = 0;
	if (fileState.get())
	{
		try
		{
			if (!fileState->inputStream.fail() && !fileState->inputStream.eof())
			{
				fileState->inputStream.read((char *)data, chunkSize);
				retVal = fileState->inputStream.gcount();
			}
		} catch (...)
		{
		}
	}
	return retVal;
}

int
AvatarManager::AvatarFileToNetPackets(const string &fileName, unsigned requestId, NetPacketList &packets)
{
	int retVal = ERR_NET_INVALID_AVATAR_FILE;
	unsigned fileSize;
	AvatarFileType fileType;
	boost::shared_ptr<AvatarFileState> tmpState = OpenAvatarFileForChunkRead(fileName, fileSize, fileType);
	if (tmpState.get() && fileSize && fileType != AVATAR_FILE_TYPE_UNKNOWN)
	{
		boost::shared_ptr<NetPacket> avatarHeader(new NetPacketAvatarHeader);
		NetPacketAvatarHeader::Data avatarHeaderData;
		avatarHeaderData.requestId = requestId;
		avatarHeaderData.avatarFileSize = fileSize;
		avatarHeaderData.avatarFileType = fileType;
		static_cast<NetPacketAvatarHeader *>(avatarHeader.get())->SetData(avatarHeaderData);
		packets.push_back(avatarHeader);

		unsigned numBytes = 0;
		unsigned totalBytesRead = 0;
		do
		{
			boost::shared_ptr<NetPacketAvatarFile> avatarFile(new NetPacketAvatarFile);
			NetPacketAvatarFile::Data avatarFileData;
			avatarFileData.fileData.resize(MAX_FILE_DATA_SIZE);
			numBytes = ChunkReadAvatarFile(tmpState, &avatarFileData.fileData[0], MAX_FILE_DATA_SIZE);
			if (numBytes)
			{
				avatarFileData.fileData.resize(numBytes);
				avatarFileData.requestId = requestId;
				totalBytesRead += numBytes;
				static_cast<NetPacketAvatarFile *>(avatarFile.get())->SetData(avatarFileData);
				packets.push_back(avatarFile);
			}
		} while (numBytes);

		if (fileSize != totalBytesRead)
			retVal = ERR_NET_WRONG_AVATAR_SIZE;
		else
		{
			boost::shared_ptr<NetPacket> avatarEnd(new NetPacketAvatarEnd);
			NetPacketAvatarEnd::Data avatarEndData;
			avatarEndData.requestId = requestId;
			static_cast<NetPacketAvatarEnd *>(avatarEnd.get())->SetData(avatarEndData);
			packets.push_back(avatarEnd);
			retVal = 0;
		}
	}
	return retVal;
}

bool
AvatarManager::GetHashForAvatar(const std::string &fileName, MD5Buf &md5buf) const
{
	bool found = false;
	if (exists(fileName))
	{
		// Scan default avatars first.
		{
			boost::mutex::scoped_lock lock(m_avatarsMutex);
			AvatarMap::const_iterator i = m_avatars.begin();
			AvatarMap::const_iterator end = m_avatars.end();
			while (i != end)
			{
				if (i->second == fileName)
				{
					md5buf = i->first;
					found = true;
					break;
				}
				++i;
			}
		}
		// Check cached avatars next.
		if (!found)
		{
			boost::mutex::scoped_lock lock(m_cachedAvatarsMutex);
			AvatarMap::const_iterator i = m_cachedAvatars.begin();
			AvatarMap::const_iterator end = m_cachedAvatars.end();
			while (i != end)
			{
				if (i->second == fileName)
				{
					md5buf = i->first;
					found = true;
					break;
				}
				++i;
			}
		}

		// Calculate md5 sum if not found.
		if (!found)
		{
			if (CryptHelper::MD5Sum(fileName, md5buf))
				found = true;
		}
	}
	return found;
}

bool
AvatarManager::GetAvatarFileName(const MD5Buf &md5buf, std::string &fileName) const
{
	bool retVal = false;
	{
		boost::mutex::scoped_lock lock(m_avatarsMutex);
		AvatarMap::const_iterator pos = m_avatars.find(md5buf);
		if (pos != m_avatars.end())
		{
			fileName = pos->second;
			retVal = true;
		}
	}
	if (!retVal)
	{
		boost::mutex::scoped_lock lock(m_cachedAvatarsMutex);
		AvatarMap::const_iterator pos = m_cachedAvatars.find(md5buf);
		if (pos != m_cachedAvatars.end())
		{
			fileName = pos->second;
			retVal = true;
		}
	}
	return retVal;
}

bool
AvatarManager::HasAvatar(const MD5Buf &md5buf) const
{
	string tmpFile;
	return GetAvatarFileName(md5buf, tmpFile);
}

bool
AvatarManager::StoreAvatarInCache(const MD5Buf &md5buf, AvatarFileType avatarFileType, const unsigned char *data, unsigned size)
{
	bool retVal = false;
	try
	{
		string ext;
		switch (avatarFileType)
		{
			case AVATAR_FILE_TYPE_PNG:
				ext = ".png";
				break;
			case AVATAR_FILE_TYPE_JPG:
				ext = ".jpg";
				break;
			case AVATAR_FILE_TYPE_GIF:
				ext = ".gif";
				break;
			case AVATAR_FILE_TYPE_UNKNOWN:
				break;
		}
		if (!ext.empty())
		{
			// Check header before storing file.
			if (IsValidAvatarFileType(avatarFileType, data, size))
			{
				path tmpPath(m_cacheDir);
				tmpPath /= (md5buf.ToString() + ext);
				string fileName(tmpPath.file_string());
				ofstream o(fileName.c_str(), ios_base::out | ios_base::binary);
				o.write((const char *)data, size);
				{
					boost::mutex::scoped_lock lock(m_cachedAvatarsMutex);
					m_cachedAvatars.insert(AvatarMap::value_type(md5buf, fileName));
				}
				retVal = true;
			}
		}
	} catch (...)
	{
	}
	return retVal;
}

bool
AvatarManager::IsValidAvatarFileType(AvatarFileType avatarFileType, const unsigned char *fileHeader, unsigned fileHeaderSize)
{
	bool validType = false;

	switch (avatarFileType)
	{
		case AVATAR_FILE_TYPE_PNG:
			if (fileHeaderSize >= PNG_HEADER_SIZE
				&& memcmp(fileHeader, PNG_HEADER, PNG_HEADER_SIZE) == 0)
			{
				validType = true;
			}
			break;
		case AVATAR_FILE_TYPE_JPG:
			if (fileHeaderSize >= JPG_HEADER_SIZE
				&& memcmp(fileHeader, JPG_HEADER, JPG_HEADER_SIZE) == 0)
			{
				validType = true;
			}
			break;
		case AVATAR_FILE_TYPE_GIF:
			if (fileHeaderSize >= GIF_HEADER_SIZE
				&& (memcmp(fileHeader, GIF_HEADER_1, GIF_HEADER_SIZE) == 0
					|| memcmp(fileHeader, GIF_HEADER_2, GIF_HEADER_SIZE) == 0))
			{
				validType = true;
			}
			break;
		case AVATAR_FILE_TYPE_UNKNOWN:
			break;
	}
	return validType;
}

bool
AvatarManager::InternalReadDirectory(const std::string &dir, AvatarMap &avatars)
{
	bool retVal = true;
	try
	{
		// This method is not thread safe. Only call after locking the map.
		directory_iterator i(dir);
		directory_iterator end;

		while (i != end)
		{
			if (is_regular(i->status()))
			{
				string md5sum(basename(i->path()));
				MD5Buf md5buf;
				string fileName(i->path().file_string());
				if (md5buf.FromString(md5sum))
				{
					// Only consider files with md5sum as name.
					avatars.insert(AvatarMap::value_type(md5buf, fileName));
				}
			}
			++i;
		}
	} catch (...)
	{
		retVal = false;
	}
	return retVal;
}

