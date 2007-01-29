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
	opacityRaiseIntervall = 0.01;

	isFlipside = FALSE;

	fadeOutTimer = new QTimer;
	connect(timer, SIGNAL(timeout()), this, SLOT(nextFadeOutFrame()));
	
}


MyCardsPixmapLabel::~MyCardsPixmapLabel()
{
}

void MyCardsPixmapLabel::startFadeOut(int speed) { 
	
	if(speed <= 4) { opacityRaiseIntervall = 0.01; }
	if(speed > 4 && speed <= 7) { opacityRaiseIntervall = 0.02; }
	if(speed > 7 && speed <= 10) { opacityRaiseIntervall = 0.04; }

	if(speed != 11) {
		fadeOutAction = TRUE;
		frameOpacity = 0.0;
		fadeOutTimer->start(40);
	} 
	
}


void MyCardsPixmapLabel::nextFadeOutFrame() {

	if (frameOpacity < 0.65) {
		frameOpacity += opacityRaiseIntervall;
     		update();
	}
	else { 
		fadeOutTimer->stop(); 
		fadeOutAction = FALSE;
	}

}

void MyCardsPixmapLabel::startFlipCards(int speed) { 
	
// 	if(speed <= 4) { opacityRaiseIntervall = 0.01; }
// 	if(speed > 4 && speed <= 7) { opacityRaiseIntervall = 0.02; }
// 	if(speed > 7 && speed <= 10) { opacityRaiseIntervall = 0.04; }
// 
// 	if(speed != 11) {
// 		fadeOutAction = TRUE;
// 		frameOpacity = 0.0;
// 		timer->start(40);
// 	} 
	
}

void MyCardsPixmapLabel::nextFlipCardsFrame() {

// 	if (frameOpacity < 0.65) {
// 		frameOpacity += opacityRaiseIntervall;
//      		update();
// 	}
// 	else { 
// 		timer->stop(); 
// 		fadeOutAction = FALSE;
// 	}

}

void MyCardsPixmapLabel::setPixmap(const QPixmap &pic, const bool flipside) {
	
	QLabel::setPixmap(pic);
	isFlipside = flipside;

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
