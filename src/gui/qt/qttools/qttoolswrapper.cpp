/***************************************************************************
 *   Copyright (C) 2006 by FThauer FHammer   *
 *   f.thauer@web.de   *
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

#include "qttoolswrapper.h"

using namespace std;


QtToolsWrapper::QtToolsWrapper() : myQtHelper(0)
{

	myQtHelper = new QtHelper();
}


QtToolsWrapper::~QtToolsWrapper()
{
	delete myQtHelper;
	myQtHelper = 0;
}

std::string QtToolsWrapper::stringToUtf8(const std::string &myString) { return myQtHelper->stringToUtf8(myString); }
std::string QtToolsWrapper::getDefaultLanguage() { return myQtHelper->getDefaultLanguage(); }
