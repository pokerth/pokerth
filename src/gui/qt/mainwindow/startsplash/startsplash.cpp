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

#include "configfile.h"
#include "mainwindowimpl.h"

StartSplash::StartSplash(mainWindowImpl *parent, ConfigFile *c)
 : QSplashScreen(parent), myW(parent), myConfig(c)
{
	
	myAppDataPath = QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str());

	QFontDatabase::addApplicationFont (myAppDataPath +"fonts/c059013l.pfb");

	logo = myAppDataPath+"gfx/gui/misc/logoChip3D.png";


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

void StartSplash::paintEvent(QPaintEvent * /*event*/) {

	QPainter painter(this);

#ifdef _WIN32
        QFont welcomeFont;
        welcomeFont.setFamily("Times New Roman");
        welcomeFont.setPixelSize(42);        
#else
#ifdef __APPLE__ 
        QFont welcomeFont;
        welcomeFont.setFamily("Century Schoolbook L");
        welcomeFont.setPixelSize(42);
#else
        QFont welcomeFont;
        welcomeFont.setFamily("Century Schoolbook L");
        welcomeFont.setPixelSize(35);
#endif        
#endif
#ifdef __APPLE__
        QFont haveFont;
        haveFont.setFamily("Century Schoolbook L");
        haveFont.setPixelSize(24);
#else
        QFont haveFont;
        haveFont.setFamily("Century Schoolbook L");
        haveFont.setPixelSize(24);
#endif
	
	QFont versionFont;
	versionFont.setFamily("Nimbus Sans L");
	versionFont.setPixelSize(12);

	if(frameNo >= 52 && frameNo < 55) {
	
		painter.setBrush(QColor(35,71,0));
		painter.drawRect(0,0,399,249);
	}

	if(frameNo >= 55 && frameNo < 57) {
	
		painter.setBrush(QColor(35,71,0));
		painter.drawRect(0,0,399,249);
	
		
		painter.setRenderHint(QPainter::Antialiasing, TRUE);
		painter.setFont(welcomeFont);
		painter.setPen(QColor(255,255,255));
		painter.drawText(48,27,300,45,4,"W");
		
	}

	if(frameNo >= 57 && frameNo < 59) {
	
		painter.setBrush(QColor(35,71,0));
		painter.drawRect(0,0,399,249);
		
		painter.setRenderHint(QPainter::Antialiasing, TRUE);
		painter.setFont(welcomeFont);
		painter.setPen(QColor(255,255,255));
		painter.drawText(48,27,300,45,4,"We");
		
	}

	if(frameNo >= 59 && frameNo < 61) {
	
		painter.setBrush(QColor(35,71,0));
		painter.drawRect(0,0,399,249);
	
		painter.setRenderHint(QPainter::Antialiasing, TRUE);
		painter.setFont(welcomeFont);
		painter.setPen(QColor(255,255,255));
		painter.drawText(48,27,300,45,4,"Wel");
		
	}

	if(frameNo >= 61 && frameNo < 63) {
	
		painter.setBrush(QColor(35,71,0));
		painter.drawRect(0,0,399,249);
		
		painter.setRenderHint(QPainter::Antialiasing, TRUE);
		painter.setFont(welcomeFont);
		painter.setPen(QColor(255,255,255));
		painter.drawText(48,27,300,45,4,"Welc");
		
	}

	if(frameNo >= 63 && frameNo < 65) {
	
		painter.setBrush(QColor(35,71,0));
		painter.drawRect(0,0,399,249);
		
		painter.setRenderHint(QPainter::Antialiasing, TRUE);
		painter.setFont(welcomeFont);
		painter.setPen(QColor(255,255,255));
		painter.drawText(48,27,300,45,4,"Welco");
		
	}

	if(frameNo >= 65 && frameNo < 67) {
	
		painter.setBrush(QColor(35,71,0));
		painter.drawRect(0,0,399,249);
		
		painter.setRenderHint(QPainter::Antialiasing, TRUE);
		painter.setFont(welcomeFont);
		painter.setPen(QColor(255,255,255));
		painter.drawText(48,27,300,45,4,"Welcom");
		
	}

	if(frameNo >= 67 && frameNo < 72) {
	
		painter.setBrush(QColor(35,71,0));
		painter.drawRect(0,0,399,249);
		
		painter.setRenderHint(QPainter::Antialiasing, TRUE);
		painter.setFont(welcomeFont);
		painter.setPen(QColor(255,255,255));
		painter.drawText(48,27,300,45,4,"Welcome");
		
	}

	if(frameNo >= 72 && frameNo < 74) {
	
		painter.setBrush(QColor(35,71,0));
		painter.drawRect(0,0,399,249);
		
		painter.setRenderHint(QPainter::Antialiasing, TRUE);
		painter.setFont(welcomeFont);
		painter.setPen(QColor(255,255,255));
		painter.drawText(48,27,300,45,4,"Welcome t");
		
	}

	if(frameNo >= 74 && frameNo < 79 ) {
	
		painter.setBrush(QColor(35,71,0));
		painter.drawRect(0,0,399,249);
		
		painter.setRenderHint(QPainter::Antialiasing, TRUE);
		painter.setFont(welcomeFont);
		painter.setPen(QColor(255,255,255));
		painter.drawText(48,27,300,45,4,"Welcome to");
		
	}

	if(frameNo >= 79 && frameNo < 91) {
// 	
		painter.setBrush(QColor(35,71,0));
		painter.drawRect(0,0,399,249);
		
		painter.setRenderHint(QPainter::Antialiasing, TRUE);
		painter.setFont(welcomeFont);
		painter.setPen(QColor(255,255,255));
		painter.drawText(48,27,300,45,4,"Welcome to");

		--opacityCounter;
// 			std::cout << opacity << " " << opacityCounter << "\n";
		opacity = 1.0/opacityCounter;
// 		std::cout << opacity << "\n";
		painter.setOpacity(opacity);
		painter.drawPixmap(150,85,100,100,logo);
		
	}

	if(frameNo >= 91 && frameNo < 106) {
// 	
		painter.setBrush(QColor(35,71,0));
		painter.drawRect(0,0,399,249);
		
		painter.setRenderHint(QPainter::Antialiasing, TRUE);
		painter.setFont(welcomeFont);
		painter.setPen(QColor(255,255,255));
		painter.drawText(48,27,300,45,4,"Welcome to");

		painter.drawPixmap(150,85,100,100,logo);
		
	}

	if(frameNo >= 106 && frameNo < 136) {
// 	
		painter.setBrush(QColor(35,71,0));
		painter.drawRect(0,0,399,249);
		
		painter.setRenderHint(QPainter::Antialiasing, TRUE);
		painter.setFont(welcomeFont);
		painter.setPen(QColor(255,255,255));
		painter.drawText(48,27,300,45,4,"Welcome to");
	
		painter.drawPixmap(150,85,100,100,logo);
		
		painter.setFont(haveFont);
		painter.setPen(QColor(255,255,255));
		painter.drawText(3,190,390,40,4,"Have a lot of fun!");

	}

	if(frameNo >= 136 && frameNo < 190) {
// 	
		painter.setBrush(QColor(35,71,0));
		painter.drawRect(0,0,399,249);
		
		painter.setRenderHint(QPainter::Antialiasing, TRUE);
		painter.setFont(welcomeFont);
		painter.setPen(QColor(255,255,255));
		painter.drawText(48,27,300,45,4,"Welcome to");

		painter.drawPixmap(150,85,100,100,logo);
		
		painter.setFont(haveFont);
		painter.setPen(QColor(255,255,255));
		painter.drawText(3,190,390,40,4,"Have a lot of fun! ;-)");

	}

	//even draw version number
	painter.setOpacity(1.0);
	painter.setFont(versionFont);
	painter.setPen(QColor(102,153,0));
	painter.drawText(295,230,100,20,4,"Version 0.6-beta");
	painter.setOpacity(opacity);

	if(frameNo >=190) { 
		this->close();
// 		myW->label_logo->show(); 

	}
}

