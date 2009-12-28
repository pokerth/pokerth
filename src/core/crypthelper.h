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
/* Helper class for crypt functions. */

#ifndef _CRYPTHELPER_H_
#define _CRYPTHELPER_H_

#include <string>
#include <vector>

#define MD5_DATA_SIZE		16
#define SHA1_DATA_SIZE		20

class HashBuf
{
public:
	virtual ~HashBuf();

	std::string ToString() const;
	bool FromString(const std::string &text);
	bool IsZero() const;

	bool operator==(const HashBuf &other) const;
	bool operator<(const HashBuf &other) const;

	virtual unsigned char *GetData() = 0;
	virtual const unsigned char *GetData() const = 0;
	virtual int GetDataSize() const = 0;
};

class MD5Buf : public HashBuf
{
public:
	MD5Buf();

	virtual unsigned char *GetData();
	virtual const unsigned char *GetData() const;
	virtual int GetDataSize() const;

private:
	unsigned char m_data[MD5_DATA_SIZE];
};

class SHA1Buf : public HashBuf
{
public:
	SHA1Buf();

	virtual unsigned char *GetData();
	virtual const unsigned char *GetData() const;
	virtual int GetDataSize() const;

	unsigned char m_data[SHA1_DATA_SIZE];
};

class CryptHelper
{
public:

	static bool MD5Sum(const std::string &fileName, MD5Buf &buf);
	static bool SHA1Hash(unsigned char *data, unsigned dataSize, SHA1Buf &buf);
	static bool HMACSha1(unsigned char *keyData, unsigned keySize, unsigned char *plainData, unsigned plainSize, SHA1Buf &buf);
	static bool AES128Encrypt(unsigned char *keyData, unsigned keySize, unsigned char *plainData, unsigned plainSize, std::vector<unsigned char> &outCipher);
};

#endif
