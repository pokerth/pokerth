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
#include "mycardspixmaplabel.h"

MyCardsPixmapLabel::MyCardsPixmapLabel(QFrame* parent)
 : QLabel(parent)
{
	
	frameOpacity = 0.0;

	timer = new QTimer;
	connect(timer, SIGNAL(timeout()), this, SLOT(nextFadeOutFrame()));
	
}


MyCardsPixmapLabel::~MyCardsPixmapLabel()
{
}

void MyCardsPixmapLabel::startFadeOut() { timer->start(40); }


void MyCardsPixmapLabel::nextFadeOutFrame() {

	if (frameOpacity < 0.7) {
		frameOpacity += 0.01;
     		update();
	}
	else { timer->stop(); }

}

void MyCardsPixmapLabel::paintEvent(QPaintEvent * event) {

	QLabel::paintEvent(event);

	QPainter painter(this);
	painter.setPen(QColor(74,131,83));
	painter.setBrush(QColor(74,131,83));
//  	painter.setBrush(QColor(0,0,0));

	painter.setOpacity(frameOpacity);
// 	painter.drawRect(this->geometry());
	painter.drawRect(0,0,57,80);

}
