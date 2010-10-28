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
#include "mysetlabel.h"

#include "gametableimpl.h"

using namespace std;

MySetLabel::MySetLabel(QGroupBox* parent)
 : QLabel(parent), myW(0)
{
}


MySetLabel::~MySetLabel()
{
}

void MySetLabel::paintEvent(QPaintEvent * event) {

		QLabel::paintEvent(event);
}
