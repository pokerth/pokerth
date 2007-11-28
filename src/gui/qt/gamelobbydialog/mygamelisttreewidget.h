//
// C++ Interface: MyGameListTreeWidget
//
// Description: 
//
//
// Author: FThauer FHammer <f.thauer@web.de>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef MYGAMELISTTREEWIDGET_H
#define MYGAMELISTTREEWIDGET_H

#include <QtGui>
#include <QtCore>

class MyGameListTreeWidget : public QTreeWidget
{
Q_OBJECT
public:
    MyGameListTreeWidget(QDialog*);

    ~MyGameListTreeWidget();


public slots:
	
	void paintEvent(QPaintEvent *);
	void setGameListBackgroundImage(QString pmString);
	void scrollContentsBy ( int dx, int dy );

private: 

	QPixmap gameListBGPixmap;

};

#endif
