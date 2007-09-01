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

#include <net/socket_helper.h>

#include <windows.h>
#include <string>
#include <cstring>

using namespace std;

typedef int  (WSAAPI * getaddrinfo_ptr_t)  (const char *, const char* , const struct addrinfo *, struct addrinfo **);
typedef void (WSAAPI * freeaddrinfo_ptr_t) (struct addrinfo*);

static wstring
utf8ToWchar(const char *str)
{
	// convert str from UTF-8 to UTF-16 (Win32 byte order)
	wstring retStr;
	if (str)
	{
		size_t len = strlen(str) + 1;
		if (len > 1)
		{
			size_t reqLen = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, len, NULL, 0);

			if (reqLen)
			{
				wchar_t *wstr = new wchar_t[reqLen];
				wstr[0] = L'\0';
				if (::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, len, wstr, reqLen) == reqLen)
					retStr = wstr;
				delete[] wstr;
			}
		}
	}
	return retStr;
}

bool
socket_string_to_addr(const char *str, int addrFamily, struct sockaddr * addr, int addrLen)
{
	bool retVal = false;
#ifdef UNICODE
	// convert str from UTF-8 to UTF-16 (Win32 byte order)
	wstring wstr(utf8ToWchar(str));
	if (!wstr.empty())
		retVal = (WSAStringToAddress((LPWSTR)wstr.c_str(), addrFamily, NULL, addr, &addrLen) != SOCKET_ERROR);
#else
	if (str && *str != 0)
		retVal = (WSAStringToAddress(str, addrFamily, NULL, addr, &addrLen) != SOCKET_ERROR);
#endif

	return retVal;
}

bool
socket_resolve(const char *str, const char *port, int addrFamily, int sockType, int protocol, struct sockaddr *addr, int addrLen)
{
	bool retVal = false;
	bool useGetaddrinfo = false;

	if (str && *str != 0)
	{
		HMODULE hWsock = ::LoadLibraryA("ws2_32");

		if (hWsock)
		{
			// Determine functions at runtime, because windows systems < XP do not
			// support getaddrinfo.
			getaddrinfo_ptr_t getaddrinfo_ptr = (getaddrinfo_ptr_t)::GetProcAddress(hWsock, "getaddrinfo");
			freeaddrinfo_ptr_t freeaddrinfo_ptr = (freeaddrinfo_ptr_t)::GetProcAddress(hWsock, "freeaddrinfo");

			if (getaddrinfo_ptr && freeaddrinfo_ptr)
			{
				useGetaddrinfo = true;

				struct addrinfo aiHints;
				struct addrinfo *aiList = NULL;

				memset(&aiHints, 0, sizeof(aiHints));
				aiHints.ai_family = addrFamily;
				aiHints.ai_socktype = sockType;
				aiHints.ai_protocol = protocol;

				// Try to resolve the name.
				bool success = (getaddrinfo_ptr(str, port, &aiHints, &aiList) == 0);

				if (success && aiList)
				{
					if ((int)aiList->ai_addrlen <= addrLen)
					{
						memcpy(addr, aiList->ai_addr, aiList->ai_addrlen);
						retVal = true;
					}
					freeaddrinfo_ptr(aiList);
				}
			}
			::FreeLibrary(hWsock);
		}
		// If we cannot use getaddrinfo (OS older than XP),
		// we call the "classic" gethostbyname.
		if (!useGetaddrinfo && protocol == AF_INET)
		{
			struct hostent *host = gethostbyname(str);
			if (host && host->h_addr_list)
			{
				memcpy(addr, host->h_addr_list, host->h_length);
				retVal = true;
			}
		}
	}
	return retVal;
}

