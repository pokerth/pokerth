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
	
	void setFadeOutAction(bool theValue) { fadeOutAction = theValue; }
	bool getFadeOutAction() const { return fadeOutAction;}
	
	void startFadeOut(int);
	void startFlipCards(int, QPixmap, QPixmap*);
	
	void paintEvent(QPaintEvent * event);

	QPixmap *front;
	QPixmap *flipside;


public slots:
	
	void setPixmap ( const QPixmap &, const bool );
	void nextFadeOutFrame();
	void nextFlipCardsFrame();

// 	bool event ( QEvent * );
	

private: 

	qreal frameOpacity;
	qreal opacityRaiseIntervall;

	qreal flipCardsScaleIntervall;
	qreal frameFlipCardsAction1Size;
	qreal frameFlipCardsAction2Size;

	QTimer *fadeOutTimer;
	QTimer *flipCardsTimer;	

	
	bool isFlipside;
	bool fadeOutAction;
	bool flipCardsAction1;
	bool flipCardsAction2;
};

#endif
