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
#include "myactionbutton.h"
#include "gametablestylereader.h"

using namespace std;

MyActionButton::MyActionButton(QGroupBox* parent)
	: QPushButton(parent), myStyle(NULL), eatMyEvents(false)
{
}


MyActionButton::~MyActionButton()
{
}


void MyActionButton::paintEvent(QPaintEvent * event)
{

	QPushButton::paintEvent(event);

	QPainter painter(this);
	painter.setPen(QColor("#"+myStyle->getFKeyIndicatorColor()));
	painter.setOpacity(0.5);
	QFont f= painter.font();
	f.setPixelSize(9);
	painter.setFont(f);
	if(objectName()==("pushButton_AllIn")) {
		painter.drawText(6,6,15,15,Qt::AlignLeft,fKeyText);
	} else if(objectName()==("pushButton_showMyCards")) {
#ifndef GUI_800x480
		painter.drawText(6,6,15,15,Qt::AlignLeft,QString("F5"));
#endif
	} else {
		painter.drawText(8,15,15,15,Qt::AlignLeft,fKeyText);
	}
}

bool MyActionButton::event(QEvent *event)
{
	if ((event->type() == QEvent::KeyPress || event->type() == QEvent::MouseButtonPress) && eatMyEvents ) {
		return true;
	}
	return QWidget::event(event);
}

void MyActionButton::click()
{
	if (!eatMyEvents) {
		QPushButton::click();
	}
}
