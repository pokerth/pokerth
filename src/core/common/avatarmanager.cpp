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
#include <net/net_helper.h>
#include <net/socket_msg.h>
#include <net/uploaderthread.h>
#include <core/loghelper.h>
#include <core/crypthelper.h>

#include <boost/filesystem.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <core/openssl_wrapper.h>

#include <fstream>
#include <cstring>

#define MAX_NUMBER_OF_FILES			GetMaxNumberOfAvatarFiles()
#define MAX_AVATAR_CACHE_AGE		GetMaxAvatarCacheAgeSec()

#define PNG_HEADER "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a"
#define PNG_HEADER_SIZE (sizeof(PNG_HEADER) - 1)
#define JPG_HEADER "\xff\xd8"
#define JPG_HEADER_SIZE (sizeof(JPG_HEADER) - 1)
#define GIF_HEADER_1 "GIF87a"
#define GIF_HEADER_2 "GIF89a"
#define GIF_HEADER_SIZE (sizeof(GIF_HEADER_1) - 1)
#define MAX_HEADER_SIZE PNG_HEADER_SIZE


using namespace std;
using namespace boost::filesystem;

struct AvatarFileState
{
	ifstream		inputStream;
};

AvatarManager::AvatarManager(bool useExternalServer, const std::string &externalServerAddress,
		const string &externalServerUser, const string &externalServerPassword)
: m_useExternalServer(useExternalServer), m_externalServerAddress(externalServerAddress),
  m_externalServerUser(externalServerUser), m_externalServerPassword(externalServerPassword)
{
	m_uploader.reset(new UploaderThread());
}

AvatarManager::~AvatarManager()
{
	m_uploader->SignalTermination();
	m_uploader->Join(UPLOADER_THREAD_TERMINATE_TIMEOUT);
}

bool
AvatarManager::Init(const string &dataDir, const string &cacheDir)
{
	bool retVal = true;
	bool tmpRet;
	path tmpCachePath(cacheDir);
	path tmpDataPath(dataDir);
	{
		boost::mutex::scoped_lock lock(m_cacheDirMutex);
		m_cacheDir = tmpCachePath.directory_string();
	}
	{
		boost::mutex::scoped_lock lock(m_avatarsMutex);
		tmpRet = InternalReadDirectory((tmpDataPath / "gfx/avatars/default/people/").directory_string(), m_avatars);
		retVal = retVal && tmpRet;
		tmpRet = InternalReadDirectory((tmpDataPath / "gfx/avatars/default/misc/").directory_string(), m_avatars);
		retVal = retVal && tmpRet;
	}
	if (cacheDir.empty() || tmpCachePath.empty())
		LOG_ERROR("Cache directory was not set!");
	else
	{
		boost::mutex::scoped_lock lock(m_cachedAvatarsMutex);
		tmpRet = InternalReadDirectory(tmpCachePath.directory_string(), m_cachedAvatars);
		retVal = retVal && tmpRet;
	}

	m_uploader->Run();
	return retVal;
}

void
AvatarManager::AddSingleAvatar(const std::string &fileName)
{
	path filePath(fileName);
	string tmpFileName(filePath.file_string());

	if (!fileName.empty() && !tmpFileName.empty())
	{
		unsigned outFileSize;
		AvatarFileType outFileType;
		boost::shared_ptr<AvatarFileState> tmpFileState = OpenAvatarFileForChunkRead(tmpFileName, outFileSize, outFileType);

		// Check whether the avatar file is valid.
		if (tmpFileState.get())
		{
			tmpFileState.reset();

			MD5Buf md5buf;
			if (CryptHelper::MD5Sum(tmpFileName, md5buf))
			{
				boost::mutex::scoped_lock lock(m_avatarsMutex);
				m_avatars.insert(AvatarMap::value_type(md5buf, tmpFileName));
			}
		}
	}
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
		if (boost::algorithm::iequals(ext, ".png"))
			outFileType = AVATAR_FILE_TYPE_PNG;
		else if (boost::algorithm::iequals(ext, ".jpg") || boost::algorithm::iequals(ext, ".jpeg"))
			outFileType = AVATAR_FILE_TYPE_JPG;
		else if (boost::algorithm::iequals(ext, ".gif"))
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
		LOG_ERROR("Exception caught when trying to open avatar.");
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
			LOG_ERROR("Exception caught when trying to read avatar.");
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
AvatarManager::StoreAvatarInCache(const MD5Buf &md5buf, AvatarFileType avatarFileType, const unsigned char *data, unsigned size, bool upload)
{
	bool retVal = false;
	string cacheDir;
	{
		boost::mutex::scoped_lock lock(m_cacheDirMutex);
		cacheDir = m_cacheDir;
	}
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
		if (!ext.empty() && !cacheDir.empty())
		{
			// Check header before storing file.
			if (IsValidAvatarFileType(avatarFileType, data, size))
			{
				path tmpPath(cacheDir);
				tmpPath /= (md5buf.ToString() + ext);
				string fileName(tmpPath.file_string());
				ofstream o(fileName.c_str(), ios_base::out | ios_base::binary | ios_base::trunc);
				if (!o.fail())
				{
					o.write((const char *)data, size);
					o.close();
					if (upload && m_useExternalServer)
					{
						m_uploader->QueueUpload(m_externalServerAddress, m_externalServerUser, m_externalServerPassword, fileName, size);
					}

					{
						boost::mutex::scoped_lock lock(m_cachedAvatarsMutex);
						m_cachedAvatars.insert(AvatarMap::value_type(md5buf, fileName));
					}
					retVal = true;
				}
			}
		}
	} catch (...)
	{
		LOG_ERROR("Exception caught when trying to store avatar.");
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

void
AvatarManager::RemoveOldAvatarCacheEntries()
{
	string cacheDir;
	{
		boost::mutex::scoped_lock lock(m_cacheDirMutex);
		cacheDir = m_cacheDir;
	}
	try
	{
		path cachePath(cacheDir);
		cacheDir = cachePath.directory_string();
		// Never delete anything if we do not have a special cache dir set.
		if (!cacheDir.empty())
		{
			boost::mutex::scoped_lock lock(m_cachedAvatarsMutex);

			// First pass: Remove files which no longer exist.
			// Count files and record age.
			AvatarList removeList;
			TimeAvatarMap timeMap;
			unsigned fileCount = 0;
			{
				AvatarMap::const_iterator i = m_cachedAvatars.begin();
				AvatarMap::const_iterator end = m_cachedAvatars.end();
				while (i != end)
				{
					bool keepFile = false;
					path filePath(i->second);
					string fileString(filePath.file_string());
					// Only consider files which are definitely in the cache dir.
					if (fileString.size() > cacheDir.size() && fileString.substr(0, cacheDir.size()) == cacheDir)
					{
						// Only consider files with MD5 as file name.
						MD5Buf tmpBuf;
						if (exists(filePath) && tmpBuf.FromString(basename(filePath)))
						{
							++fileCount;
							timeMap.insert(TimeAvatarMap::value_type(last_write_time(filePath), i->first));
							keepFile = true;
						}
					}
					if (!keepFile)
						removeList.push_back(i->first);

					++i;
				}
			}

			{
				AvatarList::const_iterator i = removeList.begin();
				AvatarList::const_iterator end = removeList.end();
				while (i != end)
				{
					m_cachedAvatars.erase(*i);
					++i;
				}
				removeList.clear();
			}

			// Remove and physically delete files in one of the
			// following cases:
			// 1. More than MAX_NUMBER_OF_FILES files are present
			//    - delete until only MAX_NUMBER_OF_FILES/2 are left.
			// 2. Files are older than 30 days.

			if (m_cachedAvatars.size() > MAX_NUMBER_OF_FILES)
			{
				while (!timeMap.empty() && m_cachedAvatars.size() > MAX_NUMBER_OF_FILES / 2)
				{
					TimeAvatarMap::iterator i = timeMap.begin();
					AvatarMap::iterator pos = m_cachedAvatars.find(i->second);
					if (pos != m_cachedAvatars.end())
					{
						path tmpPath(pos->second);
						remove(tmpPath);
						m_cachedAvatars.erase(pos);
					}
					timeMap.erase(i);
				}
			}

			// Get reference time.
			time_t curTime = time(NULL);
			while (!timeMap.empty() && !m_cachedAvatars.empty())
			{
				TimeAvatarMap::iterator i = timeMap.begin();
				if (curTime - i->first < MAX_AVATAR_CACHE_AGE)
					break;
				AvatarMap::iterator pos = m_cachedAvatars.find(i->second);
				if (pos != m_cachedAvatars.end())
				{
					path tmpPath(pos->second);
					remove(tmpPath);
					m_cachedAvatars.erase(pos);
				}
				timeMap.erase(i);
			}
		}
	} catch (...)
	{
		LOG_ERROR("Exception caught while cleaning up cache.");
	}
}

bool
AvatarManager::InternalReadDirectory(const std::string &dir, AvatarMap &avatars)
{
	bool retVal = true;
	path tmpPath(dir);

	if (exists(tmpPath) && is_directory(tmpPath))
	{
		try
		{
			// This method is not thread safe. Only call after locking the map.
			directory_iterator i(tmpPath);
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
			LOG_ERROR("Exception caught when trying to scan avatar directory.");
			retVal = false;
		}
	}
	else
	{
		LOG_ERROR("Avatar directory does not exist.");
		retVal = false;
	}
	return retVal;
}

