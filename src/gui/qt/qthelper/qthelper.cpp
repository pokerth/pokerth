//
// C++ Implementation: qthelper
//
// Description: 
//
//
// Author: FThauer FHammer <webmaster@pokerth.net>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "qthelper.h"

QtHelper::QtHelper()
{
}


QtHelper::~QtHelper()
{
}

std::string QtHelper::stringToUtf8(const std::string &myString) {

	QString tmpString = QString::fromStdString(myString);
	std::string myUtf8String = tmpString.toUtf8().constData();
	
	return myUtf8String;
}

