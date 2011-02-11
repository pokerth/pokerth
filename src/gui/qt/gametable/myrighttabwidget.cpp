//
// C++ Implementation: MyRightTabWidget
//
// Description:
//
//
// Author: FThauer FHammer <f.thauer@web.de>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "myrighttabwidget.h"
#include <iostream>

using namespace std;

MyRightTabWidget::MyRightTabWidget(QGroupBox *parent)
	: QTabWidget(parent), myTabBar(0)
{
	myTabBar = this->tabBar();
}

MyRightTabWidget::~MyRightTabWidget()
{
}

void MyRightTabWidget::paintEvent(QPaintEvent * event)
{

	QTabWidget::paintEvent(event);

}



