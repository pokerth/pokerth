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

#include <iostream>

#include <QtGui>
#include <QtCore>

class StartSplash : public QWidget
{
Q_OBJECT
public:
    StartSplash(const QRect&);

    ~StartSplash();
	
	int frameNo;
	int opacityCounter;	
	qreal opacity;

	QRect parentsGeo;

	void paintEvent(QPaintEvent * event);

public slots:

	void nextAnimationFrame();

};

#endif
