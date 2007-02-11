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

typedef int (WSAAPI * getaddrinfow_ptr_t)(const wchar_t *nodename, const wchar_t* servname,
			const ADDRINFOW *hints, PADDRINFOW *res);
typedef void (WSAAPI * freeaddrinfow_ptr_t)(PADDRINFOW ai);

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
	bool useUnicodeCall = false;

	if (str && *str != 0)
	{
		HMODULE hWsock = ::LoadLibraryA("ws2_32");

		if (hWsock)
		{
			// Determine functions at runtime, because some operating system do not
			// support the unicode version of getaddrinfo.
			getaddrinfow_ptr_t getaddrinfow_ptr = (getaddrinfow_ptr_t)::GetProcAddress(hWsock, "GetAddrInfoW");
			freeaddrinfow_ptr_t freeaddrinfow_ptr = (freeaddrinfow_ptr_t)::GetProcAddress(hWsock, "FreeAddrInfoW");

			if (getaddrinfow_ptr && freeaddrinfow_ptr)
			{
				useUnicodeCall = true;

				// convert str from UTF-8 to UTF-16 (Win32 byte order)
				wstring wstr(utf8ToWchar(str));
				wstring wport(utf8ToWchar(port));
				ADDRINFOW aiHints;
				ADDRINFOW *aiList = NULL;

				memset(&aiHints, 0, sizeof(aiHints));
				aiHints.ai_family = addrFamily;
				aiHints.ai_socktype = sockType;
				aiHints.ai_protocol = protocol;

				bool success = (getaddrinfow_ptr(wstr.c_str(), wport.c_str(), &aiHints, &aiList) == 0);

				if (success && aiList)
				{
					if ((int)aiList->ai_addrlen <= addrLen)
					{
						memcpy(addr, aiList->ai_addr, aiList->ai_addrlen);
						retVal = true;
					}
					freeaddrinfow_ptr(aiList);
				}
			}
			::FreeLibrary(hWsock);
		}
		// If we cannot use the unicode function (OS older than XP SP 2),
		// we call the "classic" getaddrinfo.
		if (!useUnicodeCall)
			retVal = internal_socket_resolve(str, port, addrFamily, sockType, protocol, addr, addrLen);
	}
	return retVal;
}

