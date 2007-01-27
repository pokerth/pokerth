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

class StartSplash : public QPainter
{
Q_OBJECT
public:
    StartSplash(QMainWindow *parent = 0);

    ~StartSplash();
	
	int frameNo;
	int opacityCounter;	
	qreal opacity;

	void aanimateStartSplash();
	void setFrameNo(int frameNumber) { frameNo = frameNumber+51; }
};

#endif
