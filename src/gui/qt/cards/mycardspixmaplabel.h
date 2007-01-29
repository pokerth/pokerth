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

	void setIsFlipside(bool theValue){ isFlipside = theValue;}
	bool getIsFlipside() const{ return isFlipside;}

	void startFadeOut(int);
	
	void paintEvent(QPaintEvent * event);


public slots:
	
	void setPixmap ( const QPixmap &, const bool );
	void nextFadeOutFrame();

private: 

	qreal frameOpacity;
	qreal opacityRaiseIntervall;
	QTimer *timer;
	
	bool isFlipside;
	bool fadeOutAction;
};

#endif
