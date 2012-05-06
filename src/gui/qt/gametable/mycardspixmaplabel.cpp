/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2011 Felix Hammer, Florian Thauer, Lothar May          *
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
 *****************************************************************************/
#include "mycardspixmaplabel.h"
#include "gametableimpl.h"
// using namespace std;

MyCardsPixmapLabel::MyCardsPixmapLabel(QGroupBox* parent)
	: QLabel(parent), myW(NULL)
{

	this->setMouseTracking(TRUE);

	fadeOutAction = FALSE;
	flipCardsAction1 = FALSE;
	flipCardsAction2 = FALSE;

	mousePress = FALSE;
	fastFlipCardsFront = FALSE;

// 	rotationIntervall = 0.03;

	isFlipside = FALSE;

	fadeOutTimer = new QTimer;
	connect(fadeOutTimer, SIGNAL(timeout()), this, SLOT(nextFadeOutFrame()));
	flipCardsTimer = new QTimer;
	connect(flipCardsTimer, SIGNAL(timeout()), this, SLOT(nextFlipCardsFrame()));

	connect(this, SIGNAL(signalFastFlipCards(bool)), this, SLOT(fastFlipCards(bool)));
}


MyCardsPixmapLabel::~MyCardsPixmapLabel()
{
}

void MyCardsPixmapLabel::startFadeOut(int speed)
{

	frameOpacity = 1.0;

	if(speed <= 4) {
		opacityRaiseInterval = 0.01;
	}
	if(speed > 4 && speed <= 7) {
		opacityRaiseInterval = 0.02;
	}
	if(speed > 7 && speed <= 10) {
		opacityRaiseInterval = 0.04;
	}

	if(speed != 11) {
		fadeOutAction = TRUE;
		frameOpacity = 1.0;
		fadeOutTimer->start(40);
	}

}

void MyCardsPixmapLabel::stopFadeOut()
{

	fadeOutTimer->stop();
	fadeOutAction = FALSE;
	frameOpacity = 1.0;
	update();
}



void MyCardsPixmapLabel::nextFadeOutFrame()
{

	if (frameOpacity > 0.25) {
		frameOpacity -= opacityRaiseInterval;
		update();
	} else {

		fadeOutTimer->stop();
// 		fadeOutAction = FALSE;
	}

}

void MyCardsPixmapLabel::startFlipCards(int speed, const QPixmap &frontPix, const QPixmap &flipsidePix)
{

	stopFadeOut();

	stopFlipCards = FALSE;
	isFlipside = FALSE;

	QLabel::setPixmap(frontPix);

	frameFlipCardsAction1Size = 1.0;
	frameFlipCardsAction2Size = 0.0;

	front = frontPix.scaled(width(), height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);;
	flipside = flipsidePix.scaled(width(), height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);;

	if(speed <= 4) {
		flipCardsScaleIntervall = 0.1;
	}
	if(speed > 4 && speed <= 6) {
		flipCardsScaleIntervall = 0.20;
	}
	if(speed > 6 && speed <= 8) {
		flipCardsScaleIntervall = 0.25;
	}
	if(speed > 8 && speed <= 10) {
		flipCardsScaleIntervall = 0.5;
        }

	if(speed != 11) {
		flipCardsAction1 = TRUE;
		flipCardsTimer->start(40);
	}

}

void MyCardsPixmapLabel::stopFlipCardsAnimation()
{

	flipCardsTimer->stop();
	flipCardsAction1 = FALSE;
	flipCardsAction2 = FALSE;
	stopFlipCards = TRUE;
	update();
}

void MyCardsPixmapLabel::nextFlipCardsFrame()
{

	if (frameFlipCardsAction1Size > 0.1	) {
		//erst flipside verkleinern
		frameFlipCardsAction1Size -= flipCardsScaleIntervall;
		update();
	} else {
		if(flipCardsAction1) {
			flipCardsAction1 = FALSE;
			flipCardsAction2 = TRUE;
		} else {
			//dann front vergrößern
			if (frameFlipCardsAction2Size < 0.9 ) {
				frameFlipCardsAction2Size += flipCardsScaleIntervall;
                                update();
			} else {
				flipCardsAction2 = FALSE;
				flipCardsTimer->stop();
			}

		}
	}
}

void MyCardsPixmapLabel::setPixmap(const QPixmap &pic, const bool flipsideIs)
{

	QLabel::setPixmap(pic);
	isFlipside = flipsideIs;
}

void MyCardsPixmapLabel::setHiddenFrontPixmap ( const QPixmap &pic )
{

	myHiddenFront = pic.scaled(width(), height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);;
}

void MyCardsPixmapLabel::paintEvent(QPaintEvent * event)
{

	if (!(flipCardsAction1 || flipCardsAction2 || fadeOutAction)) {
		QLabel::paintEvent(event);
	}

	if (fadeOutAction && !fastFlipCardsFront) {
		QPainter painter(this);
		painter.setOpacity(frameOpacity);
		if(isFlipside)
			painter.drawPixmap(0,0, flipside);
		else
			painter.drawPixmap(0,0, front);
	}

	if (flipCardsAction1) {
		QPainter painter2(this);
		QPointF center(flipside.width()/2.0, flipside.height()/2.0);
		painter2.translate(center);
		painter2.scale(frameFlipCardsAction1Size ,1);
		painter2.translate(-center);
		painter2.drawPixmap(0,0, flipside);
	}

	if (flipCardsAction2) {
		QPainter painter3(this);
		QPointF center(front.width()/2.0, front.height()/2.0);
		painter3.translate(center);
		painter3.scale(frameFlipCardsAction2Size ,1);
		painter3.translate(-center);
		painter3.drawPixmap(0,0, front);
	}

	if (fastFlipCardsFront && !flipCardsAction1 && !flipCardsAction2) {
		if(objectName().contains("pixmapLabel_card0")) {
			QPainter painter4(this);
			if(fadeOutAction) {
				painter4.setOpacity(0.25);
			}
			painter4.drawPixmap(0,0, myHiddenFront);
		}
	}
}

void MyCardsPixmapLabel::fastFlipCards(bool front)
{

	if (front) {
		fastFlipCardsFront = TRUE;
		update();
	} else {
		fastFlipCardsFront = FALSE;
		update();
	}
}

void MyCardsPixmapLabel::mousePressEvent(QMouseEvent * event)
{

	if (!mousePress && objectName().contains("pixmapLabel_card0")) {
		mousePress = TRUE;
		myW->mouseOverFlipCards(TRUE);

	}

	QLabel::mousePressEvent(event);
}

void MyCardsPixmapLabel::mouseReleaseEvent(QMouseEvent * event)
{

	if (mousePress && objectName().contains("pixmapLabel_card0")) {
		mousePress = FALSE;
		myW->mouseOverFlipCards(FALSE);
	}

	QLabel::mouseReleaseEvent(event);
}
