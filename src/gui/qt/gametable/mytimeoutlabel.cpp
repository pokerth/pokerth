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
#include "mytimeoutlabel.h"

#include "gametableimpl.h"

using namespace std;

MyTimeoutLabel::MyTimeoutLabel(QGroupBox* parent)
 : QLabel(parent), timeOutAnimation(FALSE), timeOutValue(0), timeOutFrame(0), waitFrames(0), timerIntervall(0), isBeep(0), isBeepPlayed(0)
{

	timeOutAnimationTimer = new QTimer;

	connect(timeOutAnimationTimer, SIGNAL(timeout()), this, SLOT(nextTimeOutAnimationFrame()));
}


MyTimeoutLabel::~MyTimeoutLabel()
{
}

void MyTimeoutLabel::startTimeOutAnimation(int secs, bool beep) {

	isBeepPlayed = FALSE;
	isBeep = beep;

	timeOutValue = secs;
	timeOutFrame = 1;
        timeOutAnimationWidth = 50;
	
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

void MyTimeoutLabel::startTimeOutAnimationNow() {

	timeOutAnimation = TRUE;
	timeOutAnimationTimer->start(83);

}

void MyTimeoutLabel::stopTimeOutAnimation() {

// 	timeOutAnimationKickOnTimer->stop();
	timeOutAnimationTimer->stop();
	timeOutAnimation = FALSE;
	update();
}

void MyTimeoutLabel::nextTimeOutAnimationFrame() {

	if(timeOutAnimationWidth >=0) {
		if(timeOutFrame > waitFrames) { 
			//play beep after waitFrames one time
			if(isBeep && !isBeepPlayed) { 
				myW->getMySDLPlayer()->playSound("yourturn",0);
				isBeepPlayed = TRUE;
			}
			//save gfx ressources and never play more the 10 pps
			unsigned int realTimerValue = realTimer.elapsed().total_milliseconds();
                        timeOutAnimationWidth = 50-(((realTimerValue-3000)*50)/((timeOutValue-3)*1000));

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


void MyTimeoutLabel::paintEvent(QPaintEvent * event) {

	if(!timeOutAnimation || timeOutFrame <= waitFrames) {
		QLabel::paintEvent(event);
	}

	if(timeOutAnimation && timeOutFrame > waitFrames) {

		QPainter painter(this);
	
                QLinearGradient linearGrad(QPointF(0, 10), QPointF(45, 10));
		linearGrad.setColorAt(0, Qt::red);
		linearGrad.setColorAt(0.5, Qt::yellow);
		linearGrad.setColorAt(1, Qt::green);
	
		painter.setBrush(linearGrad);
		
		painter.setPen(QColor(0,0,0));
		painter.drawRect(0,2,timeOutAnimationWidth-1,8);
	}
}
