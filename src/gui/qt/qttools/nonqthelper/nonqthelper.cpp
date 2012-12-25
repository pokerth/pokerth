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

#include "nonqthelper.h"

#include <core/convhelper.h>
#include <boost/filesystem.hpp>

NonQtHelper::NonQtHelper()
{
}


NonQtHelper::~NonQtHelper()
{
}

std::string
NonQtHelper::stringToUtf8(const std::string &myString)
{
	return ConvHelper::NativeToUtf8(myString);
}

std::string
NonQtHelper::stringFromUtf8(const std::string &myString)
{
	return ConvHelper::Utf8ToNative(myString);
}

std::string
NonQtHelper::getDefaultLanguage()
{
	return "en";
}

std::string
NonQtHelper::getDataPathStdString(const char *argv0)
{
	boost::filesystem::path startPath(argv0);
	startPath = startPath.remove_leaf();
	startPath /= "data";
	return stringToUtf8(startPath.directory_string());
}
