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

class MyRightTabWidget : public QTabWidget
{
Q_OBJECT
public:
    MyRightTabWidget(QGroupBox*);

    ~MyRightTabWidget();


	void paintEvent(QPaintEvent * event);

	QTabBar* getMyTabBar() const { return myTabBar; }
	

public slots:
	

private: 
	QTabBar *myTabBar;

};

#endif
