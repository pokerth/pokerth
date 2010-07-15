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
#include "gametablestylereader.h"

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
    painter.setPen(QColor("#"+myStyle->getFKeyIndicatorColor()));
    painter.setOpacity(0.5);
    QFont f= painter.font();
    f.setPixelSize(9);
    painter.setFont(f);
    if(objectName()==("pushButton_AllIn")) {
        painter.drawText(6,6,15,15,Qt::AlignLeft,fKeyText);
    }
    else if(objectName()==("pushButton_showMyCards")){
        painter.drawText(6,6,15,15,Qt::AlignLeft,QString("F5"));
    }
    else {
        painter.drawText(8,15,15,15,Qt::AlignLeft,fKeyText);
    }
}
