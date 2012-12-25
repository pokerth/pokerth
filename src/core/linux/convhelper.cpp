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
#include <core/loghelper.h>

#ifdef _WIN32
#error This file is not for Windows.
#endif

#if defined(__FreeBSD__) || defined(__NetBSD__)
#define HAVE_ICONV_CONST
#endif

#include <string>
#include <cstring>

#include <iconv.h>
#include <errno.h>

using namespace std;

string
ConvHelper::NativeToUtf8(const std::string &inStr)
{
	string retStr(inStr);
	size_t insize = inStr.length();
#ifdef HAVE_ICONV_CONST
	const char *inbuf = inStr.data();
#else
	char *inbuf = const_cast<char *>(inStr.data());
#endif

	const size_t c_outsize = insize * 6; // max size of utf-8 char is 6 per input char
	size_t outsize = c_outsize;
	char *c_outbuf = new char[outsize];
	char *outbuf = c_outbuf;

	//nl_langinfo(CODESET)
	iconv_t conversion = iconv_open("UTF-8", "ISO-8859-1");

	if (conversion == (iconv_t)(-1))
		LOG_ERROR("iconv_open() failed: " << strerror(errno));
	else {
		size_t retval = iconv(conversion, &inbuf, &insize, &outbuf, &outsize);

		if (retval == (size_t)-1)
			LOG_ERROR("iconv() failed: " << strerror(errno));
		retStr = string(c_outbuf, c_outsize - outsize);
	}
	delete[] c_outbuf;

	iconv_close(conversion);
	return retStr;
}

string
ConvHelper::Utf8ToNative(const std::string &inStr)
{
	string retStr(inStr);
	size_t insize = inStr.length();
#ifdef HAVE_ICONV_CONST
	const char *inbuf = inStr.data();
#else
	char *inbuf = const_cast<char *>(inStr.data());
#endif

	const size_t c_outsize = insize;
	size_t outsize = c_outsize;
	char *c_outbuf = new char[outsize];
	char *outbuf = c_outbuf;

	iconv_t conversion = iconv_open("ISO-8859-1", "UTF-8");

	if (conversion == (iconv_t)(-1))
		LOG_ERROR("iconv_open() failed: " << strerror(errno));
	else {
		size_t retval = iconv(conversion, &inbuf, &insize, &outbuf, &outsize);

		if (retval == (size_t)-1)
			LOG_ERROR("iconv() failed: " << strerror(errno));
		retStr = string(c_outbuf, c_outsize - outsize);
	}
	delete[] c_outbuf;

	iconv_close(conversion);
	return retStr;
}


