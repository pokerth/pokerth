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
	flipCardsAction1 = FALSE;
	flipCardsAction2 = FALSE;


	
// 	rotationIntervall = 0.03;
	
	isFlipside = FALSE;

	front = new QPixmap;
	flipside = new QPixmap;

	fadeOutTimer = new QTimer;
	connect(fadeOutTimer, SIGNAL(timeout()), this, SLOT(nextFadeOutFrame()));
	flipCardsTimer = new QTimer;
	connect(flipCardsTimer, SIGNAL(timeout()), this, SLOT(nextFlipCardsFrame()));
}


MyCardsPixmapLabel::~MyCardsPixmapLabel()
{
}

void MyCardsPixmapLabel::startFadeOut(int speed) { 
	
	
	frameOpacity = 0.0;

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

void MyCardsPixmapLabel::startFlipCards(int speed, QPixmap frontPix, QPixmap *flipsidePix) { 
	
	QLabel::setPixmap(frontPix);

	frameFlipCardsAction1Size = 1.0;
	frameFlipCardsAction2Size = 0.0;

	*front = frontPix;
	flipside = flipsidePix;

	if(speed <= 4) { flipCardsScaleIntervall = 0.1; }
	if(speed > 4 && speed <= 6) { flipCardsScaleIntervall = 0.20; }
	if(speed > 6 && speed <= 8) { flipCardsScaleIntervall = 0.25; }
	if(speed > 8 && speed <= 10) { flipCardsScaleIntervall = 0.5; }
// 
	
	if(speed != 11) {
		flipCardsAction1 = TRUE;
		flipCardsTimer->start(40);
	} 
	
}

void MyCardsPixmapLabel::nextFlipCardsFrame() {

	if (frameFlipCardsAction1Size > 0.1	) {
		//erst flipside verkleinern
		frameFlipCardsAction1Size -= flipCardsScaleIntervall;
     		update();
	}
	else { 
		if(flipCardsAction1) { 
			flipCardsAction1 = FALSE;
			flipCardsAction2 = TRUE;
		}
		else {
			//dann front vergrößern
			if (frameFlipCardsAction2Size < 0.9 ) {
				
				frameFlipCardsAction2Size += flipCardsScaleIntervall;
				update();
			}
			else {
				flipCardsAction2 = FALSE;
				flipCardsTimer->stop(); 
			}

		}
	}
}

void MyCardsPixmapLabel::setPixmap(const QPixmap &pic, const bool flipsideIs) {
	
	QLabel::setPixmap(pic);
	isFlipside = flipsideIs;

}

void MyCardsPixmapLabel::paintEvent(QPaintEvent * event) {

	if (!(flipCardsAction1 || flipCardsAction2)) {
		QLabel::paintEvent(event);
	}

	if (fadeOutAction) {

		QPainter painter(this);
// 		painter.setPen(QColor(74,131,83));
		painter.setBrush(QColor(74,131,83));
// 		painter.setBrush(palette().color(QPalette::Background));
	//  	painter.setBrush(QColor(0,0,0));
	
		painter.setOpacity(frameOpacity);
	// 	painter.drawRect(this->geometry());
		if(objectName()=="pixmapLabel_card0b" || objectName()=="pixmapLabel_card0a")
		painter.drawRect(-1,-1,81,113);
		else
		painter.drawRect(-1,-1,58,81);
		
	}

	if (flipCardsAction1) {
		QPainter painter2(this);
		QPixmap tmpFlipside = flipside->scaled(width(), height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
		QPointF center(tmpFlipside.width()/2.0, tmpFlipside.height()/2.0);

		painter2.translate(center);
		painter2.scale(frameFlipCardsAction1Size ,1);
		painter2.translate(-center);
		painter2.drawPixmap(0,0, tmpFlipside);
	}
	
	if (flipCardsAction2) {
		QPainter painter3(this);
		QPixmap tmpFront = front->scaled(width(), height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
		QPointF center(tmpFront.width()/2.0, tmpFront.height()/2.0);

		painter3.translate(center);
		painter3.scale(frameFlipCardsAction2Size ,1);
		painter3.translate(-center);

		painter3.drawPixmap(0,0, tmpFront);
	}

	
}
