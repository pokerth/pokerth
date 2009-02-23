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
#include <QtCore>
#include <iostream>


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

std::string QtHelper::stringFromUtf8(const std::string &myString) {
	QString tmpString = QString::fromUtf8(myString.c_str());
	
	return tmpString.toStdString();
}

std::string QtHelper::getDefaultLanguage() { return QLocale::system().name().toStdString(); }

std::string QtHelper::getDataPathStdString(const char * /*argv0*/)
{
	QString path(QCoreApplication::instance()->applicationDirPath());
	
#ifdef _WIN32 
	path += "/data/";
#else
	#ifdef __APPLE__
	if (QRegExp("Contents/MacOS/?$").indexIn(path) != -1) {
		// pointing into an macosx application bundle
		path += "/../Resources/data/";
	} else { path += "/data/"; }
	#else //Unix
	if (QRegExp("pokerth/?$").indexIn(path) != -1) {
		// there is an own application directory
		path += "/data/";
	} else if (QRegExp("usr/games/bin/?$").indexIn(path) != -1) {
		// we are in /usr/games/bin (like gentoo linux does)
		path += "/../../share/games/pokerth/data/";
	} else if (QRegExp("usr/games/?$").indexIn(path) != -1) {
		// we are in /usr/games (like Debian linux does)
		path += "/../share/games/pokerth/";
	} else if (QRegExp("bin/?$").indexIn(path) != -1) {
		// we are in a bin directory. e.g. /usr/bin
		path += "/../share/pokerth/data/";
	
	} else { path += "/data/"; }
	#endif
#endif
	return (QDir::cleanPath(path) + "/").toUtf8().constData();
}
// [01:09] <Zhenech> doitux|mob, mach den pfad als define, und nur wenns nich gesetzt is wildes raten
// [01:10] <Zhenech> dann compilieren die distries mit -DDATAPTH="/usr/share/games/pokerth" o.ä.
// [01:10] <Zhenech> und du suchst eine liste ab:
// [01:10] <Zhenech> ist es in [/usr/share/pokerth, /usr/share/games/pokerth/, /usr/local/..., $PWD/data]



