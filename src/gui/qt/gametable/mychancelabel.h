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
	void refreshChance(double*);
	
private: 

	gameTableImpl *myW;
	double RFChance;
	double SFChance;
	double FOAKChance;
	double FHChance;
	double FLChance;
	double STRChance;
	double TOAKChance;
	double TPChance;
	double OPChance;
	double HCChance;
};

#endif
