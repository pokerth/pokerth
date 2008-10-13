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
#ifndef MYCHANCELABEL_H
#define MYCHANCELABEL_H

#include <iostream>

#include <QtGui>
#include <QtCore>

class gameTableImpl;

class MyChanceLabel : public QLabel
{
Q_OBJECT
public:
    MyChanceLabel(QWidget*);

    ~MyChanceLabel();
	
	void setMyW ( gameTableImpl* theValue ) { myW = theValue; }

	
private: 

	gameTableImpl *myW;
	
};

#endif
