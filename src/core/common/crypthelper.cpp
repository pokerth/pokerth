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


#include <core/openssl_wrapper.h>
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

HashBuf::~HashBuf()
{
}

std::string
HashBuf::ToString() const
{
	// Create a hex-based string from the MD5 data.
	string retValue;
	char tmpBuf[2 + 1];
	tmpBuf[sizeof(tmpBuf) - 1] = 0;
	const unsigned char *tmpData = GetData();
	for (int i = 0; i < GetDataSize(); i++)
	{
		sprintf(tmpBuf, "%02x", tmpData[i]);
		retValue += tmpBuf;
	}
	return retValue;
}

bool
HashBuf::FromString(const std::string &text)
{
	// Convert hex-based string to MD5 data.
	bool retVal = false;
	int tmpSize = GetDataSize();
	if (text.size() == 2 * (unsigned)tmpSize)
	{
		unsigned char *tmpData = GetData();
		const char *t = text.c_str();
		int i = 0;
		for (; i < tmpSize; i++) {
			int part1 = fromHex(*t++);
			if (part1 == -1)
				break;
			int part2 = fromHex(*t++);
			if (part2 == -1)
				break;
			*tmpData++ = (part1<<4) + part2;
		}
		retVal = i == tmpSize;
	}
	return retVal;
}

bool
HashBuf::IsZero() const
{
	int dataSize = GetDataSize();
	const unsigned char *tmpData = GetData();
	int i;
	for (i = 0; i < dataSize; i++)
	{
		if (tmpData[i] != 0)
			break;
	}
	return i == dataSize;
}

bool
HashBuf::operator==(const HashBuf &other) const
{
	return GetDataSize() == other.GetDataSize() && memcmp(GetData(), other.GetData(), GetDataSize()) == 0;
}

bool
HashBuf::operator<(const HashBuf &other) const
{
	int smallestDataSize = GetDataSize() < other.GetDataSize() ? GetDataSize() : other.GetDataSize();
	return memcmp(GetData(), other.GetData(), smallestDataSize) < 0;
}


MD5Buf::MD5Buf()
{
	memset(m_data, 0, sizeof(m_data));
}

unsigned char *
MD5Buf::GetData()
{
	return m_data;
}

const unsigned char *
MD5Buf::GetData() const
{
	return m_data;
}

int
MD5Buf::GetDataSize() const
{
	return sizeof(m_data);
}

SHA1Buf::SHA1Buf()
{
	memset(m_data, 0, sizeof(m_data));
}

unsigned char *
SHA1Buf::GetData()
{
	return m_data;
}

const unsigned char *
SHA1Buf::GetData() const
{
	return m_data;
}

int
SHA1Buf::GetDataSize() const
{
	return sizeof(m_data);
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
		MD5_Final(buf.GetData(), &context);
		retVal = ferror(file) == 0;

		fclose(file);
	}
	return retVal;
}

bool
CryptHelper::SHA1Hash(unsigned char *data, unsigned dataSize, SHA1Buf &buf)
{
	bool retVal = false;
#ifdef HAVE_OPENSSL
	if (SHA1(data, dataSize, buf.GetData()) != NULL)
		retVal = true;
#else
	// TODO
#endif
	return retVal;
}

bool
CryptHelper::AES128Encrypt(unsigned char *keyData, unsigned keySize, unsigned char *plainData, unsigned plainSize, std::vector<unsigned char> &outCipher)
{
	bool retVal = false;
//#ifdef HAVE_OPENSSL
//	int errCode = EVP_BytesToKey(EVP_aes_128_cbc(), EVP_sha1(), NULL, keyData, keySize,
	return retVal;
}
