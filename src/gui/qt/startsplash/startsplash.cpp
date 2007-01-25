/***************************************************************************
 *   Copyright (C) 2006 by FThauer FHammer   *
 *   f.thauer@web.de   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "startsplash.h"

StartSplash::StartSplash()
 : QWidget()
{
	QFontDatabase::addApplicationFont ("src/gui/qt/fonts/c059013l.pfb");
	

	frameNo = 0;
	
	QTimer *timer = new QTimer;
	connect(timer, SIGNAL(timeout()), this, SLOT(nextAnimationFrame()));
	timer->start(40);
}


StartSplash::~StartSplash()
{
}

void StartSplash::nextAnimationFrame() {
     
	++frameNo;
     	update();
 }

void StartSplash::paintEvent(QPaintEvent * event) {

	QPainter painter(this);

	if(frameNo < 40) {
	
		painter.setBrush(QColor(0+frameNo,40+frameNo,6+frameNo));
		painter.drawRect(0,0,299,199);

	}

	if(frameNo >= 40 && frameNo < 65) {
	
		painter.setBrush(QColor(40,80,46));
		painter.drawRect(0,0,299,199);
	}

	if(frameNo >= 65 && frameNo < 90) {
	
		painter.setBrush(QColor(40,80,46));
		painter.drawRect(0,0,299,199);
	
		QFont font1("Century Schoolbook L",20);	
		painter.setFont(font1);
		painter.setPen(QColor(255,255,255));
		painter.drawText(20,20,140,40,1,"Welcome");
		
	}

}

