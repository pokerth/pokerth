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
#ifndef MYSTATUSLABEL_H
#define MYSTATUSLABEL_H

#include <iostream>

#include <QtGui>
#include <QtCore>

class gameTableImpl;

class MyStatusLabel : public QLabel
{
Q_OBJECT
public:
    MyStatusLabel(QGroupBox*);

    ~MyStatusLabel();

	
	void setMyW ( gameTableImpl* theValue ) { myW = theValue; }

	void mousePressEvent ( QMouseEvent *);
	void mouseReleaseEvent ( QMouseEvent *);

	
private: 

	gameTableImpl *myW;
	bool mousePress;
	
};

#endif
