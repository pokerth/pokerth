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
 *****************************************************************************/
#ifndef MYTIMEOUTLABEL_H
#define MYTIMEOUTLABEL_H

#include <iostream>

#include <QtGui>
#include <QtCore>
#include <third_party/boost/timers.hpp>
#include "sdlplayer.h"

class gameTableImpl;

class MyTimeoutLabel : public QLabel
{
	Q_OBJECT
public:
	MyTimeoutLabel(QGroupBox*);

	~MyTimeoutLabel();


	void setMyW ( gameTableImpl* theValue ) {
		myW = theValue;
	}

	void startTimeOutAnimation(int secs, bool beep);
	void stopTimeOutAnimation();

	void paintEvent(QPaintEvent * event);


public slots:
	void startTimeOutAnimationNow();
	void nextTimeOutAnimationFrame();


private:

	gameTableImpl *myW;
	QTimer *timeOutAnimationTimer;
	QTimer *timeOutAnimationKickOnTimer;

// 	boost::shared_ptr<SDLPlayer> mySDLPlayer;

	boost::timers::portable::microsec_timer realTimer;

	bool timeOutAnimation;

	int timeOutAnimationWidth;
	int timeOutValue;
	int timeOutFrame;
	int waitFrames;
	int decreaseWidthIntervall;
	int timerIntervall;
	bool isBeep;
	bool isBeepPlayed;


};

#endif
