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
#include <vector>

#include <QtGui>
#include <QtCore>

using namespace std;

class gameTableImpl;

class MyChanceLabel : public QLabel
{
Q_OBJECT
public:
    MyChanceLabel(QWidget*);

    ~MyChanceLabel();
	
	void setMyW ( gameTableImpl* theValue ) { myW = theValue; }
	void paintEvent(QPaintEvent * event);
	void refreshChance(vector< vector<int> >);
	void resetChance();
	
private: 

	gameTableImpl *myW;
	int RFChance[2];
	int SFChance[2];
	int FOAKChance[2];
	int FHChance[2];
	int FLChance[2];
	int STRChance[2];
	int TOAKChance[2];
	int TPChance[2];
	int OPChance[2];
	int HCChance[2];
};

#endif
