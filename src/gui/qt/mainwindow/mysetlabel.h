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
#include <core/boost/timer.hpp>
#include "sdlplayer.h"

class mainWindowImpl;

class MySetLabel : public QLabel
{
Q_OBJECT
public:
    MySetLabel(QGroupBox*);

    ~MySetLabel();

	
	void setMyW ( mainWindowImpl* theValue ) { myW = theValue; }
	
	void startTimeOutAnimation(int secs, bool beep);
	void stopTimeOutAnimation();
	
	void paintEvent(QPaintEvent * event);

	
public slots:
	void startTimeOutAnimationNow();
	void nextTimeOutAnimationFrame();

	
private: 

	mainWindowImpl *myW;
	QTimer *timeOutAnimationTimer;
	QTimer *timeOutAnimationKickOnTimer;
	
// 	boost::shared_ptr<SDLPlayer> mySDLPlayer;

	boost::microsec_timer realTimer;

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
