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

using namespace std;

MyCardsPixmapLabel::MyCardsPixmapLabel(QFrame* parent)
 : QLabel(parent)
{
	fadeOutAction = FALSE;
	frameOpacity = 0.0;

	timer = new QTimer;
	connect(timer, SIGNAL(timeout()), this, SLOT(nextFadeOutFrame()));
	
}


MyCardsPixmapLabel::~MyCardsPixmapLabel()
{
}

void MyCardsPixmapLabel::startFadeOut() { 
	fadeOutAction = TRUE;
	frameOpacity = 0.0;
	timer->start(40); 
	
}


void MyCardsPixmapLabel::nextFadeOutFrame() {

	if (frameOpacity < 0.7) {
		frameOpacity += 0.01;
     		update();
	}
	else { 
		timer->stop(); 
		fadeOutAction = FALSE;
	}

}

void MyCardsPixmapLabel::paintEvent(QPaintEvent * event) {

	QLabel::paintEvent(event);

	if (fadeOutAction) {

		QPainter painter(this);
// 		painter.setPen(QColor(74,131,83));
		painter.setBrush(QColor(74,131,83));
	//  	painter.setBrush(QColor(0,0,0));
	
		painter.setOpacity(frameOpacity);
	// 	painter.drawRect(this->geometry());
		if(objectName()=="pixmapLabel_card0b" || objectName()=="pixmapLabel_card0a")
		painter.drawRect(-1,-1,81,113);
		else
		painter.drawRect(-1,-1,58,81);
		
	}
}
