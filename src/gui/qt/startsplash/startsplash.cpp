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

StartSplash::StartSplash(mainWindowImpl *parent)
 : QDialog(), myW(parent)
{
	QFontDatabase::addApplicationFont ("src/gui/qt/fonts/c059013l.pfb");
	QFontDatabase::addApplicationFont ("src/gui/qt/fonts/andybold.ttf");	

	frameNo = 52;
	opacityCounter = 13;
	opacity = 1.0;
	
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
	QFont welcomeFont("Century Schoolbook L",29);	
	QFont haveFont("Andy MT",30);
	QPixmap logo(":/graphics/graphics/logo-140-100.png");

	if(frameNo >= 52 && frameNo < 65) {
	
		painter.setBrush(QColor(40,80,46));
		painter.drawRect(0,0,399,249);
	}

	if(frameNo >= 65 && frameNo < 68) {
	
		painter.setBrush(QColor(40,80,46));
		painter.drawRect(0,0,399,249);
	
		
		painter.setRenderHint(QPainter::Antialiasing, TRUE);
		painter.setFont(welcomeFont);
		painter.setPen(QColor(255,255,255));
		painter.drawText(48,30,300,40,4,"W");
		
	}

	if(frameNo >= 68 && frameNo < 71) {
	
		painter.setBrush(QColor(40,80,46));
		painter.drawRect(0,0,399,249);
		
		painter.setRenderHint(QPainter::Antialiasing, TRUE);
		painter.setFont(welcomeFont);
		painter.setPen(QColor(255,255,255));
		painter.drawText(48,30,300,40,4,"We");
		
	}

	if(frameNo >= 71 && frameNo < 74) {
	
		painter.setBrush(QColor(40,80,46));
		painter.drawRect(0,0,399,249);
	
		painter.setRenderHint(QPainter::Antialiasing, TRUE);
		painter.setFont(welcomeFont);
		painter.setPen(QColor(255,255,255));
		painter.drawText(48,30,300,40,4,"Wel");
		
	}

	if(frameNo >= 74 && frameNo < 77) {
	
		painter.setBrush(QColor(40,80,46));
		painter.drawRect(0,0,399,249);
		
		painter.setRenderHint(QPainter::Antialiasing, TRUE);
		painter.setFont(welcomeFont);
		painter.setPen(QColor(255,255,255));
		painter.drawText(48,30,300,40,4,"Welc");
		
	}

	if(frameNo >= 77 && frameNo < 80) {
	
		painter.setBrush(QColor(40,80,46));
		painter.drawRect(0,0,399,249);
		
		painter.setRenderHint(QPainter::Antialiasing, TRUE);
		painter.setFont(welcomeFont);
		painter.setPen(QColor(255,255,255));
		painter.drawText(48,30,300,40,4,"Welco");
		
	}

	if(frameNo >= 80 && frameNo < 83) {
	
		painter.setBrush(QColor(40,80,46));
		painter.drawRect(0,0,399,249);
		
		painter.setRenderHint(QPainter::Antialiasing, TRUE);
		painter.setFont(welcomeFont);
		painter.setPen(QColor(255,255,255));
		painter.drawText(48,30,300,40,4,"Welcom");
		
	}

	if(frameNo >= 83 && frameNo < 92) {
	
		painter.setBrush(QColor(40,80,46));
		painter.drawRect(0,0,399,249);
		
		painter.setRenderHint(QPainter::Antialiasing, TRUE);
		painter.setFont(welcomeFont);
		painter.setPen(QColor(255,255,255));
		painter.drawText(48,30,300,40,4,"Welcome");
		
	}

	if(frameNo >= 92 && frameNo < 95) {
	
		painter.setBrush(QColor(40,80,46));
		painter.drawRect(0,0,399,249);
		
		painter.setRenderHint(QPainter::Antialiasing, TRUE);
		painter.setFont(welcomeFont);
		painter.setPen(QColor(255,255,255));
		painter.drawText(48,30,300,40,4,"Welcome t");
		
	}

	if(frameNo >= 95 && frameNo < 120 ) {
	
		painter.setBrush(QColor(40,80,46));
		painter.drawRect(0,0,399,249);
		
		painter.setRenderHint(QPainter::Antialiasing, TRUE);
		painter.setFont(welcomeFont);
		painter.setPen(QColor(255,255,255));
		painter.drawText(48,30,300,40,4,"Welcome to");
		
	}

	if(frameNo >= 120 && frameNo < 132) {
// 	
		painter.setBrush(QColor(40,80,46));
		painter.drawRect(0,0,399,249);
		
		painter.setRenderHint(QPainter::Antialiasing, TRUE);
		painter.setFont(welcomeFont);
		painter.setPen(QColor(255,255,255));
		painter.drawText(48,30,300,40,4,"Welcome to");

		--opacityCounter;
// 			std::cout << opacity << " " << opacityCounter << "\n";
		opacity = 1.0/opacityCounter;
// 		std::cout << opacity << "\n";
		painter.setOpacity(opacity);
		painter.drawPixmap(130,85,140,100,logo);
		
	}

	if(frameNo >= 132 && frameNo < 155) {
// 	
		painter.setBrush(QColor(40,80,46));
		painter.drawRect(0,0,399,249);
		
		painter.setRenderHint(QPainter::Antialiasing, TRUE);
		painter.setFont(welcomeFont);
		painter.setPen(QColor(255,255,255));
		painter.drawText(48,30,300,40,4,"Welcome to");

		painter.drawPixmap(130,85,140,100,logo);
		
	}

	if(frameNo >= 155 && frameNo < 195) {
// 	
		painter.setBrush(QColor(40,80,46));
		painter.drawRect(0,0,399,249);
		
		painter.setRenderHint(QPainter::Antialiasing, TRUE);
		painter.setFont(welcomeFont);
		painter.setPen(QColor(255,255,255));
		painter.drawText(48,30,300,40,4,"Welcome to");
	
		painter.drawPixmap(130,85,140,100,logo);
		
		painter.setFont(haveFont);
		painter.setPen(QColor(255,255,255));
		painter.drawText(48,190,300,40,4,"Have a lot of fun!");

	}

	if(frameNo >= 195 && frameNo < 245) {
// 	
		painter.setBrush(QColor(40,80,46));
		painter.drawRect(0,0,399,249);
		
		painter.setRenderHint(QPainter::Antialiasing, TRUE);
		painter.setFont(welcomeFont);
		painter.setPen(QColor(255,255,255));
		painter.drawText(48,30,300,40,4,"Welcome to");

		painter.drawPixmap(130,85,140,100,logo);
		
		painter.setFont(haveFont);
		painter.setPen(QColor(255,255,255));
		painter.drawText(48,190,300,40,4,"Have a lot of fun! ;-)");

	}

	if(frameNo >= 245) { 
		this->hide();
		myW->label_logo->show(); 

	}

}

