//
// C++ Implementation: startsplash
//
// Description: 
//
//
// Author: FThauer FHammer <f.thauer@web.de>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "startsplash.h"

StartSplash::StartSplash()
 : QWidget()
{

	frameNo = 0;
	
	QTimer *timer = new QTimer;
	connect(timer, SIGNAL(timeout()), this, SLOT(nextAnimationFrame()));
	timer->start(40);
}


StartSplash::~StartSplash()
{
}

void StartSplash::nextAnimationFrame() {
     
	++frameNo;
     	update();
 }

void StartSplash::paintEvent(QPaintEvent * event) {

	QPainter painter(this);

	if(frameNo < 51) {
	
		painter.setBrush(QColor(0+frameNo,40+frameNo,6+frameNo));
		painter.drawRect(0,0,299,199);

	}

	if(frameNo >= 51) {
	
		painter.setBrush(QColor(51,91,57));
		painter.drawRect(0,0,299,199);
	}
}

