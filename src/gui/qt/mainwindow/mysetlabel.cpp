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
 : QLabel(parent), timeOutAnimation(FALSE), timeOutValue(0), timeOutFrame(0), waitFrames(0), timerIntervall(0)
{

	timeOutAnimationTimer = new QTimer;

	connect(timeOutAnimationTimer, SIGNAL(timeout()), this, SLOT(nextTimeOutAnimationFrame()));
}


MySetLabel::~MySetLabel()
{
}

void MySetLabel::startTimeOutAnimation(int secs) {

	timeOutValue = secs;
	timeOutFrame = 1;
	timeOutAnimationWidth = 118;
	
	int preTimerIntervall = ((timeOutValue-3) * 1000)/timeOutAnimationWidth;
	
	timerIntervall = preTimerIntervall;

	//save gfx ressources and never play more the 10 pps
	while(timerIntervall < 100) {
		if(secs <= 9 && secs >= 6) {
			//fix inaccuracies caused by integer division
			timerIntervall = timerIntervall + preTimerIntervall + 5;
		}
		else {
			timerIntervall = timerIntervall + preTimerIntervall;
		}
	}

	waitFrames = 3000/timerIntervall;
	
	//start the real timer
	realTimer.reset();
	realTimer.start();

// 	std::cout << timerIntervall << endl;
	timeOutAnimation = TRUE;	
	timeOutAnimationTimer->start(timerIntervall);
}

void MySetLabel::startTimeOutAnimationNow() {

	timeOutAnimation = TRUE;
	timeOutAnimationTimer->start(83);

}

void MySetLabel::stopTimeOutAnimation() {

// 	timeOutAnimationKickOnTimer->stop();
	timeOutAnimationTimer->stop();
	timeOutAnimation = FALSE;
	update();
}

void MySetLabel::nextTimeOutAnimationFrame() {

	if(timeOutAnimationWidth >=0) {
		if(timeOutFrame > waitFrames) { 
			//save gfx ressources and never play more the 10 pps
			unsigned int realTimerValue = realTimer.elapsed().total_milliseconds();
			timeOutAnimationWidth = 118-(((realTimerValue-3000)*118)/((timeOutValue-3)*1000));		

		}
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

	if(!timeOutAnimation || timeOutFrame <= waitFrames) {
		QLabel::paintEvent(event);
	}

	if(timeOutAnimation && timeOutFrame > waitFrames) {

		QPainter painter(this);
	
		QLinearGradient linearGrad(QPointF(0, 10), QPointF(113, 10));
		linearGrad.setColorAt(0, Qt::red);
		linearGrad.setColorAt(0.5, Qt::yellow);
		linearGrad.setColorAt(1, Qt::green);
	
		painter.setBrush(linearGrad);
		
		painter.setPen(QColor(0,0,0));
		painter.drawRect(0,6,timeOutAnimationWidth-1,8);
	}
}
