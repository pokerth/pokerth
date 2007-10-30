//
// C++ Implementation: mycardspixmaplabel
//
// Description: 
//
//
// Author: FThauer FHammer <f.thauer@web.de>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "mygamelisttreewidget.h"
#include <QDebug>
#include <iostream>

using namespace std;

MyGameListTreeWidget::MyGameListTreeWidget(QDialog* parent)
 : QTreeWidget(parent)
{
	
	//set the pixmap cache size in the main function only one time
	QPixmapCache::setCacheLimit(1024);
}


MyGameListTreeWidget::~MyGameListTreeWidget()
{
}

void MyGameListTreeWidget::setGameListBackgroundImage(QString pmString) {
       
	gameListBGPixmap.load(pmString);
}

void MyGameListTreeWidget::paintEvent(QPaintEvent *event) {

        QPainter painter(viewport());
        painter.drawPixmap(50, 5, gameListBGPixmap);

        QTreeWidget::paintEvent(event);
}

// bool MyGameListTreeWidget::event(QEvent *event)
// {
// 	cout << event->type() << endl;
// // 	if(event->type() == 12) paintEvent(event);
// 
// 	QTreeWidget::event(event);
// }

void MyGameListTreeWidget::scrollContentsBy ( int dx, int dy ) {

	viewport()->update();
	
	QTreeWidget::scrollContentsBy ( dx,dy );
}