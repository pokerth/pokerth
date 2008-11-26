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

	this->setStyleSheet("QTabWidget::pane { border: 2px solid #286400; border-radius: 2px; background-color: #145300; }  QTabWidget::tab-bar { left: 5px; } ");

#ifdef _WIN32
	QString font1String("font-family: \"Arial\";");
	int paddingTop = 1;
	int paddingSide = 7;
#else 
	#ifdef __APPLE__	
		QString font1String("font-family: \"Lucida Grande\";");
		int paddingTop = 1;
		int paddingSide = 9;
	#else 
		QString font1String("font-family: \"Nimbus Sans L\";");
		int paddingTop = 0;
		int paddingSide = 9;
	#endif
#endif

	QTabBar *myTabBar = this->tabBar();

	myTabBar->setStyleSheet("QTabBar::tab{ "+ font1String +" font-size: 11px; color: #99D500; background-color: #145300; border: 2px solid #286400; border-bottom-color: #286400; border-top-left-radius: 4px; border-top-right-radius: 4px; padding-top: "+QString::number(paddingTop,10)+"px; padding-left:"+QString::number(paddingSide,10)+"px; padding-right:"+QString::number(paddingSide,10)+"px;} QTabBar::tab:selected, QTabBar::tab:hover { background-color: #145300; padding-top: "+QString::number(paddingTop,10)+"px; padding-left:"+QString::number(paddingSide,10)+"px; padding-right:"+QString::number(paddingSide,10)+"px;} QTabBar::tab:selected { border-color: #286400; border-bottom-color: #145300; padding-top: "+QString::number(paddingTop,10)+"px; padding-left:"+QString::number(paddingSide+5,10)+"px; padding-right:"+QString::number(paddingSide+5,10)+"px;}  QTabBar::tab:!selected { margin-top: 2px; padding-top: "+QString::number(paddingTop,10)+"px; padding-left:"+QString::number(paddingSide,10)+"px; padding-right:"+QString::number(paddingSide,10)+"px;} QTabBar::tab:selected { margin-left: -4px; margin-right: -4px; padding-top: "+QString::number(paddingTop,10)+"px; padding-left:"+QString::number(paddingSide+5,10)+"px; padding-right:"+QString::number(paddingSide+5,10)+"px;} QTabBar::tab:first:selected { margin-left: 0; padding-top: "+QString::number(paddingTop,10)+"px; padding-left:"+QString::number(paddingSide,10)+"px; padding-right:"+QString::number(paddingSide,10)+"px;} QTabBar::tab:last:selected { margin-right: 0; padding-top: "+QString::number(paddingTop,10)+"px; padding-left:"+QString::number(paddingSide,10)+"px; padding-right:"+QString::number(paddingSide,10)+"px;} QTabBar::tab:only-one { margin: 0; } QTabBar::tab:disabled { font-size: 11px; font-weight: 900; color: #144D03; background-color: #144D03; border-left-color: #255704; border-right-color: #255704; border-top-color: #255704; }");

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
