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

public:
    StartSplash(QMainWindow *parent = 0, int =0);

    virtual ~StartSplash();
	
	int frameNo;
	int opacityCounter;	
	qreal opacity;

	void animateStartSplash();
};

#endif
