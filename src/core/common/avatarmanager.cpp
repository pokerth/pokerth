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
#include <openssl/md5.h>

#include <fstream>

using namespace std;
using namespace boost::filesystem;


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
		ofstream o(fileName.c_str());
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

