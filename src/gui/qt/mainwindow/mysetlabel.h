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

#include <QtGui>
#include <QtCore>

#include <iostream>

class MySetLabel : public QLabel
{
Q_OBJECT
public:
    MySetLabel(QGroupBox*);

    ~MySetLabel();

	void startTimeOutAnimation(int secs);
	void stopTimeOutAnimation();
	
	void paintEvent(QPaintEvent * event);

public slots:
	
	void nextTimeOutAnimationFrame();
	
private: 

	QTimer *timeOutAnimationTimer;
	bool timeOutAnimation;

	int timeOutAnimationWidth;
	int timeOutValue;
	int timeOutFrame;
};

#endif
