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

class gameTableImpl;

class MySetLabel : public QLabel
{
Q_OBJECT
public:
    MySetLabel(QGroupBox*);

    ~MySetLabel();

	
	void setMyW ( gameTableImpl* theValue ) { myW = theValue; }
        void paintEvent(QPaintEvent * event);
	
public slots:

private: 

        gameTableImpl *myW;

};

#endif
