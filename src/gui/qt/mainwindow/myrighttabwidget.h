//
// C++ Interface: MyRightTabWidget
//
// Description: 
//
//
// Author: FThauer FHammer <f.thauer@web.de>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef MYRIGHTTABWIDGET_H
#define MYRIGHTTABWIDGET_H

#include <QtGui>
#include <QtCore>

#include <iostream>

class MyRightTabWidget : public QTabWidget
{
Q_OBJECT
public:
    MyRightTabWidget(QGroupBox*);

    ~MyRightTabWidget();


	void paintEvent(QPaintEvent * event);
	void startBlinkChatTab();
	void stopBlinkChatTab();

public slots:
	
	void blinkChatTab();

private: 

	QTimer *chatBlinkTimer;
	QTabBar *myTabBar;

};

#endif
