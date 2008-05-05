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

class gameTableImpl;
class ConfigFile;
class StartSplash : public QSplashScreen
{
Q_OBJECT
public:
    StartSplash(gameTableImpl *, ConfigFile*);

    ~StartSplash();
	
	int frameNo;
	int opacityCounter;	
	qreal opacity;

	void paintEvent(QPaintEvent * event);

public slots:

	void nextAnimationFrame();

private:

	gameTableImpl *myW;
	QString myAppDataPath;
	QPixmap logo;	
	ConfigFile *myConfig;

};

#endif
