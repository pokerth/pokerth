//
// C++ Interface: qthelper
//
// Description: 
//
//
// Author: FThauer FHammer <webmaster@pokerth.net>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef QTHELPER_H
#define QTHELPER_H

#include <QtCore>
/**
	@author FThauer FHammer <webmaster@pokerth.net>
*/
class QtHelper{
public:
    QtHelper();

    ~QtHelper();

    std::string stringToUtf8(const std::string &);
    std::string getDefaultLanguage();

};

#endif
