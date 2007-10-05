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
#include <net/netpacket.h>

#include <boost/filesystem.hpp>
#include <openssl/md5.h>

#include <fstream>

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
	bool retVal = false;
	try
	{
		InternalReadDirectory(dataDir + "gfx/avatars/default/people/");
		InternalReadDirectory(dataDir + "gfx/avatars/default/misc/");
		InternalReadDirectory(cacheDir);
		m_cacheDir = cacheDir;
		retVal = true;
	} catch (...)
	{
	}
	return retVal;
}

boost::shared_ptr<AvatarFileState>
AvatarManager::OpenAvatarFileForChunkRead(const std::string &fileName, unsigned &outFileSize)
{
	outFileSize = 0;
	boost::shared_ptr<AvatarFileState> retVal;
	try
	{
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
			if (outFileSize <= MAX_AVATAR_FILE_SIZE)
				retVal = fileState;
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

bool
AvatarManager::SendAvatarFile(const string &fileName, unsigned requestId, boost::function<void (boost::shared_ptr<NetPacket>)> sender)
{
	bool retVal = false;
	unsigned fileSize;
	boost::shared_ptr<AvatarFileState> tmpState = OpenAvatarFileForChunkRead(fileName, fileSize);
	if (tmpState.get() && fileSize)
	{
		boost::shared_ptr<NetPacket> avatarHeader(new NetPacketAvatarHeader);
		NetPacketAvatarHeader::Data avatarHeaderData;
		avatarHeaderData.requestId = requestId;
		avatarHeaderData.avatarFileSize = fileSize;
		avatarHeaderData.avatarFileType = AVATAR_TYPE_PNG; // TODO
		static_cast<NetPacketAvatarHeader *>(avatarHeader.get())->SetData(avatarHeaderData);
		sender(avatarHeader);

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
				sender(avatarFile);
			}
		} while (numBytes);
		// TODO error handling if numBytes != totalBytesRead
		boost::shared_ptr<NetPacket> avatarEnd(new NetPacketAvatarEnd);
		NetPacketAvatarEnd::Data avatarEndData;
		avatarEndData.requestId = requestId;
		static_cast<NetPacketAvatarEnd *>(avatarEnd.get())->SetData(avatarEndData);
		sender(avatarEnd);
		retVal = true;
	}
	// else TODO error handling
	return retVal;
}

bool
AvatarManager::GetHashForAvatar(const std::string &fileName, MD5Buf &md5buf)
{
	bool found = false;

	if (exists(fileName))
	{
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
		if (!found)
		{
			if (CryptHelper::MD5Sum(fileName, md5buf))
			{
				m_avatars.insert(AvatarMap::value_type(md5buf, fileName));
				found = true;
			}
		}
	}
	return found;
}

bool
AvatarManager::GetAvatarFileName(const MD5Buf &md5buf, std::string &fileName) const
{
	bool retVal = false;
	AvatarMap::const_iterator pos = m_avatars.find(md5buf);
	if (pos != m_avatars.end())
	{
		fileName = pos->second;
		retVal = true;
	}
	return retVal;
}

bool
AvatarManager::StoreAvatarInCache(const MD5Buf &md5buf, const std::string &fileExtension, const unsigned char *data, unsigned size)
{
	bool retVal = false;
	try
	{
		path tmpPath(m_cacheDir);
		tmpPath /= (md5buf.ToString() + "." + fileExtension);
		string fileName(tmpPath.file_string());
		ofstream o(fileName.c_str(), ios_base::out | ios_base::binary);
		o.write((const char *)data, size);
		m_avatars.insert(AvatarMap::value_type(md5buf, fileName));
		retVal = true;
	} catch (...)
	{
	}
	return retVal;
}

void
AvatarManager::InternalReadDirectory(const std::string &dir)
{
	directory_iterator i(dir);
	directory_iterator end;

	while (i != end)
	{
		if (is_regular(i->status()))
		{
			string md5sum(basename(i->path()));
			MD5Buf md5buf;
			string fileName(i->path().file_string());
			bool success = true;
			if (!md5buf.FromString(md5sum))
			{
				// sigh. File name is not an md5 sum. Calculate on our own...
				if (!CryptHelper::MD5Sum(fileName, md5buf))
					success = false;
			}
			if (success)
				m_avatars.insert(AvatarMap::value_type(md5buf, fileName));
		}
		++i;
	}
}

