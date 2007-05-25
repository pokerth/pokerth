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

// using namespace std;

MyRightTabWidget::MyRightTabWidget(QGroupBox *parent)
 : QTabWidget(parent), chatBlinkTimer(0), myTabBar(0)
{
// 	fadeOutTimer = new QTimer;
// 	connect(flipCardsTimer, SIGNAL(timeout()), this, SLOT(nextFlipCardsFrame()));
	myTabBar = this->tabBar();

	chatBlinkTimer = new QTimer;
	connect(chatBlinkTimer, SIGNAL(timeout()), this, SLOT( blinkChatTab() ));
}


MyRightTabWidget::~MyRightTabWidget()
{
}

void MyRightTabWidget::startBlinkChatTab() { chatBlinkTimer->start(500); }
void MyRightTabWidget::stopBlinkChatTab() { chatBlinkTimer->stop(); }
void MyRightTabWidget::showDefaultChatTab() { myTabBar->setTabTextColor(1, QColor(240,240,240)); }

void MyRightTabWidget::blinkChatTab() {

	if(myTabBar->tabTextColor(1).red() == 240) myTabBar->setTabTextColor(1, QColor(113,162,0));
	else myTabBar->setTabTextColor(1, QColor(240,240,240));
}

void MyRightTabWidget::paintEvent(QPaintEvent * event) {

	QTabWidget::paintEvent(event);
	
}



