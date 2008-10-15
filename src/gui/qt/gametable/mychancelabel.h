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
	void paintEvent(QPaintEvent * event);
	void refreshChance(int*);
	
private: 

	gameTableImpl *myW;
	int RFChance;
	int SFChance;
	int FOAKChance;
	int FHChance;
	int FLChance;
	int STRChance;
	int TOAKChance;
	int TPChance;
	int OPChance;
	int HCChance;
};

#endif
