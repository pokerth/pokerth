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
#include "myactionbutton.h"


using namespace std;

MyActionButton::MyActionButton(QGroupBox* parent)
 : QPushButton(parent)
{
}


MyActionButton::~MyActionButton()
{
}


void MyActionButton::paintEvent(QPaintEvent * event) {

	QPushButton::paintEvent(event);

	QPainter painter(this);
	painter.setPen(QColor(240,240,240));
	painter.setOpacity(0.5);
	QFont f= painter.font();
	f.setPixelSize(9);
	painter.setFont(f);
	painter.drawText(8,11,15,15,Qt::AlignLeft,fKeyText);
}
