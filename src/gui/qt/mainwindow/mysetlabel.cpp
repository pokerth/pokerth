//
// C++ Implementation: mycardspixmaplabel
//
// Description: 
//
//
// Author: FThauer FHammer <f.thauer@web.de>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "mysetlabel.h"

#include "mainwindowimpl.h"

using namespace std;

MySetLabel::MySetLabel(QGroupBox* parent)
 : QLabel(parent), timeOutAnimation(FALSE), timeOutValue(0), timeOutFrame(0)
{

	timeOutAnimationTimer = new QTimer;
	timeOutAnimationKickOnTimer = new QTimer;
	timeOutAnimationKickOnTimer->setSingleShot(TRUE);

	connect(timeOutAnimationKickOnTimer, SIGNAL(timeout()), this, SLOT(startTimeOutAnimationNow()));
	connect(timeOutAnimationTimer, SIGNAL(timeout()), this, SLOT(nextTimeOutAnimationFrame()));
// 	flipCardsTimer = new QTimer;
// 	connect(flipCardsTimer, SIGNAL(timeout()), this, SLOT(nextFlipCardsFrame()));
}


MySetLabel::~MySetLabel()
{
}

void MySetLabel::startTimeOutAnimation(int secs) {

	timeOutValue = secs-3;
	timeOutFrame = 1;
	timeOutAnimationWidth = 118;
	
	timeOutAnimationKickOnTimer->start(3000);
}

void MySetLabel::startTimeOutAnimationNow() {

	timeOutAnimation = TRUE;
	timeOutAnimationTimer->start(83);

}

void MySetLabel::stopTimeOutAnimation() {

	timeOutAnimationKickOnTimer->stop();
	timeOutAnimationTimer->stop();
	timeOutAnimation = FALSE;
	update();
}

void MySetLabel::nextTimeOutAnimationFrame() {

	if(timeOutAnimationWidth >=0) {
		if(timeOutFrame%(timeOutValue*12/118) == 0) 
			timeOutAnimationWidth--;
		timeOutFrame++;
		update();
	}	
	else { 
		stopTimeOutAnimation();
		// no callback is called here.
		// the server initiates any action required.
	}
}


void MySetLabel::paintEvent(QPaintEvent * event) {

	if(!timeOutAnimation) {
		QLabel::paintEvent(event);
	}

	if(timeOutAnimation) {

		QPainter painter(this);
	
		QLinearGradient linearGrad(QPointF(0, 10), QPointF(113, 10));
		linearGrad.setColorAt(0, Qt::red);
		linearGrad.setColorAt(0.5, Qt::yellow);
		linearGrad.setColorAt(1, Qt::green);
	
		painter.setBrush(linearGrad);
		
		painter.setPen(QColor(0,0,0));
		painter.drawRect(0,6,timeOutAnimationWidth,8);
	}
}
