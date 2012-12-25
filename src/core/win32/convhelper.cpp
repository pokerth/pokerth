/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/

#include "convhelper.h"

#ifndef _WIN32
#error This file is Windows only.
#endif

#include <windows.h>

using namespace std;

static string
Convert(const std::string &inStr, int fromCP, int toCP)
{
	// convert str from current Windows source to target charset
	string retStr(inStr);

	if (!inStr.empty()) {
		int len = (int)inStr.length() + 1;
		int reqLen = ::MultiByteToWideChar(fromCP, 0, inStr.c_str(), len, NULL, 0);

		if (reqLen) {
			wchar_t *wstr = new wchar_t[reqLen];
			wstr[0] = L'\0';
			if (::MultiByteToWideChar(fromCP, 0, inStr.c_str(), len, wstr, reqLen) == (int)reqLen) {
				len = reqLen;
				reqLen = ::WideCharToMultiByte(toCP, 0, wstr, len, NULL, 0, NULL, NULL);

				if (reqLen) {
					char *str = new char[reqLen];
					if (::WideCharToMultiByte(toCP, 0, wstr, len, str, reqLen, NULL, NULL) == (int)reqLen)
						retStr = str;
					delete[] str;
				}
			}

			delete[] wstr;
		}
	}
	return retStr;
}

string
ConvHelper::NativeToUtf8(const std::string &inStr)
{
	return Convert(inStr, CP_ACP, CP_UTF8);
}

string
ConvHelper::Utf8ToNative(const std::string &inStr)
{
	return Convert(inStr, CP_UTF8, CP_ACP);
}

