//
// C++ Interface: startsplash
//
// Description:
//
//
// Author: FThauer FHammer <f.thauer@web.de>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef STARTSPLASH_H
#define STARTSPLASH_H

#include <QtGui>
#include <QtCore>


class StartSplash : public QSplashScreen
{
	Q_OBJECT
public:
	StartSplash(QPixmap& p);

	~StartSplash();

public slots:

	void closeThis();

};

#endif
