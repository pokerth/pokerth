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
#ifndef _WIN32
#error This source code is Win32 only.
#endif

#include <windows.h>
#include <wincrypt.h>

#include <core/rand.h>

class CryptData
{
public:
	CryptData()
	{
		if (!::CryptAcquireContext(
			&cryptProvider,
			NULL,
			NULL,
			PROV_RSA_FULL,
			CRYPT_VERIFYCONTEXT | CRYPT_SILENT))
		{
			cryptProvider = 0;
		}
	}
	~CryptData()
	{
		if (cryptProvider)
			CryptReleaseContext(cryptProvider, 0);
	}

	HCRYPTPROV cryptProvider;
};

static CryptData g_cryptData;

void
RandomBytes(unsigned char *buf, unsigned size)
{
	HCRYPTPROV provider = g_cryptData.cryptProvider;

	if (provider)
		::CryptGenRandom(provider, size, buf);
}


