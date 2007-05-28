//
// C++ Implementation: MyLeftTabWidget
//
// Description: 
//
//
// Author: FThauer FHammer <f.thauer@web.de>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "mylefttabwidget.h"

// using namespace std;

MyLeftTabWidget::MyLeftTabWidget(QGroupBox *parent)
 : QTabWidget(parent), chatBlinkTimer(0), myTabBar(0)
{
// 	fadeOutTimer = new QTimer;
// 	connect(flipCardsTimer, SIGNAL(timeout()), this, SLOT(nextFlipCardsFrame()));
	myTabBar = this->tabBar();

	chatBlinkTimer = new QTimer;
	connect(chatBlinkTimer, SIGNAL(timeout()), this, SLOT( blinkChatTab() ));
}


MyLeftTabWidget::~MyLeftTabWidget()
{
}

void MyLeftTabWidget::startBlinkChatTab() { chatBlinkTimer->start(500); }
void MyLeftTabWidget::stopBlinkChatTab() { chatBlinkTimer->stop(); }
void MyLeftTabWidget::showDefaultChatTab() { myTabBar->setTabTextColor(1, QColor(240,240,240)); }
void MyLeftTabWidget::disableTab(int tabIndex, bool yesNo) { myTabBar->setTabEnabled(tabIndex, !yesNo); }

void MyLeftTabWidget::blinkChatTab() {

	if(myTabBar->tabTextColor(1).red() == 240) myTabBar->setTabTextColor(1, QColor(113,162,0));
	else myTabBar->setTabTextColor(1, QColor(240,240,240));
}

void MyLeftTabWidget::paintEvent(QPaintEvent * event) {

	QTabWidget::paintEvent(event);
	
}



