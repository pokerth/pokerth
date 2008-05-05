//
// C++ Interface: mycardspixmaplabel
//
// Description: 
//
//
// Author: FThauer FHammer <f.thauer@web.de>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef MYSETLABEL_H
#define MYSETLABEL_H

#include <iostream>

#include <QtGui>
#include <QtCore>
#include <third_party/boost/timers.hpp>
#include "sdlplayer.h"

class gameTableImpl;

class MySetLabel : public QLabel
{
Q_OBJECT
public:
    MySetLabel(QGroupBox*);

    ~MySetLabel();

	
	void setMyW ( gameTableImpl* theValue ) { myW = theValue; }
	
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
