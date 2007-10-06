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
