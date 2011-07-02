/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2011 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *****************************************************************************/ #include "mytimeoutlabel.h"

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

void MyTimeoutLabel::startTimeOutAnimation(int secs, bool beep)
{

	if (secs >= 4) { // smaller timeouts may lead to errors/endless loops below
		isBeepPlayed = FALSE;
		isBeep = beep;

		timeOutValue = secs;
		timeOutFrame = 1;
		timeOutAnimationWidth = 52;

		int preTimerIntervall = ((timeOutValue-3) * 1000)/timeOutAnimationWidth;

		timerIntervall = preTimerIntervall;

		//save gfx ressources and never play more the 10 pps
		while(timerIntervall < 100) {
			if(secs <= 9 && secs >= 6) {
				//fix inaccuracies caused by integer division
				timerIntervall = timerIntervall + preTimerIntervall + 5;
			} else {
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
}

void MyTimeoutLabel::startTimeOutAnimationNow()
{

	timeOutAnimation = TRUE;
	timeOutAnimationTimer->start(83);

}

void MyTimeoutLabel::stopTimeOutAnimation()
{

// 	timeOutAnimationKickOnTimer->stop();
	timeOutAnimationTimer->stop();
	timeOutAnimation = FALSE;
	update();
}

void MyTimeoutLabel::nextTimeOutAnimationFrame()
{

	if(timeOutAnimationWidth >=0) {
		if(timeOutFrame > waitFrames) {
			//play beep after waitFrames one time
			if(isBeep && !isBeepPlayed) {
				myW->getMySDLPlayer()->playSound("yourturn",0);
				isBeepPlayed = TRUE;
			}
			//save gfx ressources and never play more the 10 pps
			unsigned int realTimerValue = realTimer.elapsed().total_milliseconds();
			timeOutAnimationWidth = 52-(((realTimerValue-3000)*52)/((timeOutValue-3)*1000));

		}
		timeOutFrame++;
		update();
	} else {
		stopTimeOutAnimation();
		// no callback is called here.
		// the server initiates any action required.
	}
}


void MyTimeoutLabel::paintEvent(QPaintEvent * event)
{

	if(!timeOutAnimation || timeOutFrame <= waitFrames) {
		QLabel::paintEvent(event);
	}

	if(timeOutAnimation && timeOutFrame > waitFrames) {

		QPainter painter(this);

		QLinearGradient linearGrad(QPointF(0, 10), QPointF(47, 10));
		linearGrad.setColorAt(0, Qt::red);
		linearGrad.setColorAt(0.5, Qt::yellow);
		linearGrad.setColorAt(1, Qt::green);

		painter.setBrush(linearGrad);

		painter.setPen(QColor(0,0,0));
		painter.drawRect(0,1,timeOutAnimationWidth-1,8);
	}
}
