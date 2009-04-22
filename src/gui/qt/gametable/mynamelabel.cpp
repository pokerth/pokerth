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
#include "mynamelabel.h"

MyNameLabel::MyNameLabel(QGroupBox* parent)
 : QLabel(parent), transparent(FALSE)
{
}


MyNameLabel::~MyNameLabel()
{
}

void MyNameLabel::setText ( const QString &text, const bool trans) {

        myText = text;
        transparent = trans;
        update();

}

void MyNameLabel::paintEvent(QPaintEvent*) {

        QPainter painter(this);
        if(transparent)
                painter.setOpacity(0.4);
        else
                painter.setOpacity(1.0);

        painter.drawText(0,11,myText);
}

