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

// void MyGameListTreeWidget::paintEvent(QPaintEvent *event) {
// 
// /*        QPainter painter(viewport());
//  	QVector<QRect> rects = event->region().rects();
//    	for (int i = 0; i < rects.count(); i++) {
// 		if (rects[i].width() > gameListBGPixmap.width())
// 			rects[i].setWidth(gameListBGPixmap.width());
// 		if (rects[i].height() > gameListBGPixmap.height())
// 			rects[i].setHeight(gameListBGPixmap.height());
// 		painter.drawPixmap(rects[i].translated(50,5), gameListBGPixmap, rects[i]);
// 	}
//    	painter.end();
//        QTreeWidget::paintEvent(event);*/
// 
//        QPainter painter(viewport());
//        painter.drawPixmap(50, 5, gameListBGPixmap);
// 
//        QTreeWidget::paintEvent(event);
// }

void MyGameListTreeWidget::scrollContentsBy ( int dx, int dy ) {

	viewport()->update();
	
	QTreeWidget::scrollContentsBy ( dx,dy );
}
