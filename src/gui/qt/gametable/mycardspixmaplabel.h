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
#ifndef MYCARDSPIXMAPLABEL_H
#define MYCARDSPIXMAPLABEL_H

#include <iostream>

#include <QtGui>
#include <QtCore>
#if QT_VERSION >= 0x050000
#include <QtWidgets>
#endif

class gameTableImpl;

class MyCardsPixmapLabel : public QLabel
{
	Q_OBJECT
public:
	MyCardsPixmapLabel(QGroupBox*);

	~MyCardsPixmapLabel();

	void setMyW ( gameTableImpl* theValue ) {
		myW = theValue;
	}

	void setIsFlipside(bool theValue) {
		isFlipside = theValue;
	}
	bool getIsFlipside() const {
		return isFlipside;
	}

	void setFadeOutAction(bool theValue) {
		fadeOutAction = theValue;
	}
	bool getFadeOutAction() const {
		return fadeOutAction;
	}

	void startFadeOut(int);
	void stopFadeOut();
	void startFlipCards(int, const QPixmap & , const QPixmap &);
	void stopFlipCardsAnimation();

	void setFlipsidePix(QPixmap p) {
		flipside = p;
	}

	void paintEvent(QPaintEvent * event);

signals:
	void signalFastFlipCards(bool);


public slots:

	void setPixmap ( const QPixmap &, const bool );
	void setHiddenFrontPixmap ( const QPixmap &);

	void nextFadeOutFrame();
	void nextFlipCardsFrame();

	void fastFlipCards(bool front);

// 	void mouseMoveEvent ( QMouseEvent *);

	void mousePressEvent ( QMouseEvent *);
	void mouseReleaseEvent ( QMouseEvent *);

	void setFront ( const QPixmap& theValue );


private:

	gameTableImpl* myW;

	qreal frameOpacity;
	qreal opacityRaiseInterval;

	qreal flipCardsScaleIntervall;
	qreal frameFlipCardsAction1Size;
	qreal frameFlipCardsAction2Size;

	QTimer *fadeOutTimer;
	QTimer *flipCardsTimer;

	bool isFlipside;
	bool fadeOutAction;
	bool flipCardsAction1;
	bool flipCardsAction2;
	bool stopFlipCards;

	bool mousePress;
	bool fastFlipCardsFront;

	QPixmap front;
	QPixmap flipside;
	QPixmap myHiddenFront;

	friend class gameTableImpl;
};

#endif
