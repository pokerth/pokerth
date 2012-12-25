/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/
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

void MyGameListTreeWidget::setGameListBackgroundImage(QString pmString)
{

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

void MyGameListTreeWidget::scrollContentsBy ( int dx, int dy )
{

	viewport()->update();

	QTreeWidget::scrollContentsBy ( dx,dy );
}
