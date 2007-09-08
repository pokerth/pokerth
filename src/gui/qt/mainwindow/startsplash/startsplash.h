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

class mainWindowImpl;
class ConfigFile;
class StartSplash : public QSplashScreen
{
Q_OBJECT
public:
    StartSplash(mainWindowImpl *, ConfigFile*);

    ~StartSplash();
	
	int frameNo;
	int opacityCounter;	
	qreal opacity;

	void paintEvent(QPaintEvent * event);

public slots:

	void nextAnimationFrame();

private:

	mainWindowImpl *myW;
	QString myAppDataPath;
	QPixmap logo;	
	ConfigFile *myConfig;

};

#endif
