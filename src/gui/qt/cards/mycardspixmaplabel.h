//
// C++ Interface: mycardspixmaplabel
//
// Description: 
//
//
// Author: FThauer FHammer <f.thauer@web.de>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef MYCARDSPIXMAPLABEL_H
#define MYCARDSPIXMAPLABEL_H

#include <QtGui>
#include <QtCore>

#include <iostream>

class MyCardsPixmapLabel : public QLabel
{
Q_OBJECT
public:
    MyCardsPixmapLabel(QFrame*);

    ~MyCardsPixmapLabel();

	qreal frameOpacity;
	QTimer *timer;


	void startFadeOut();
	void paintEvent(QPaintEvent * event);

public slots:
	
	void nextFadeOutFrame();

};

#endif
