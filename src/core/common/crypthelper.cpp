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

#include "crypthelper.h"


#include <openssl/md5.h>
#include <cstring>
#include <cstdio>

using namespace std;

// Helper function.
static int
fromHex(int ch)
{
	int retVal = -1;

	if (ch >= '0' && ch <= '9')
		retVal = ch - '0';
	else if (ch >= 'a' && ch <= 'f')
		retVal = ch - 'a' + 10;
	else if (ch >= 'A' && ch <= 'F')
		retVal = ch - 'A' + 10;

	return retVal;
}


MD5Buf::MD5Buf()
{
	memset(data, 0, sizeof(data));
}


std::string
MD5Buf::ToString() const
{
	// Create a hex-based string from the MD5 data.
	string retValue;
	char tmpBuf[2 + 1];
	tmpBuf[sizeof(tmpBuf) - 1] = 0;
	for (int i = 0; i < MD5_DATA_SIZE; i++)
	{
		sprintf(tmpBuf, "%02x", data[i]);
		retValue += tmpBuf;
	}
	return retValue;
}

bool
MD5Buf::FromString(const std::string &text)
{
	// Convert hex-based string to MD5 data.
	bool retVal = false;
	if (text.size() == 2 * MD5_DATA_SIZE)
	{
		unsigned char *tmpData = data;
		const char *t = text.c_str();
		int i = 0;
		for (; i < MD5_DATA_SIZE; i++) {
			int part1 = fromHex(*t++);
			if (part1 == -1)
				break;
			int part2 = fromHex(*t++);
			if (part2 == -1)
				break;
			*tmpData++ = (part1<<4) + part2;
		}
		retVal = i == MD5_DATA_SIZE;
	}
	return retVal;
}

bool
MD5Buf::operator==(const MD5Buf &other) const
{
	return memcmp(data, other.data, MD5_DATA_SIZE) == 0;
}

bool
MD5Buf::operator<(const MD5Buf &other) const
{
	return memcmp(data, other.data, MD5_DATA_SIZE) < 0;
}

bool
CryptHelper::MD5Sum(const std::string &fileName, MD5Buf &buf)
{
	bool retVal = false;
	FILE *file = fopen(fileName.c_str(), "rb");

	if (file)
	{
		// Calculate MD5 sum of file.
		unsigned char readBuf[8192];
		MD5_CTX context;
		int numBytes;

		MD5_Init(&context);
		while ((numBytes = fread(readBuf, 1, sizeof(readBuf), file)) > 0)
			MD5_Update(&context, readBuf, numBytes);
		MD5_Final(buf.data, &context);
		retVal = ferror(file) == 0;

		fclose(file);
	}
	return retVal;
}

