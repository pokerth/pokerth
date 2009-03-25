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

MyStatusLabel::MyStatusLabel(QGroupBox* parent)
 : QLabel(parent) {

	mousePress = FALSE;

}

MyStatusLabel::~MyStatusLabel()
{
}

void MyStatusLabel::mousePressEvent(QMouseEvent * event) {

	if (!mousePress && objectName().contains("textLabel_Status0")) {
			mousePress = TRUE;	
			myW->mouseOverFlipCards(TRUE);
			
	}

	QLabel::mousePressEvent(event);
}

void MyStatusLabel::mouseReleaseEvent(QMouseEvent * event) {

	if (mousePress && objectName().contains("textLabel_Status0")) {
		mousePress = FALSE;	
		myW->mouseOverFlipCards(FALSE);
	}	

	QLabel::mouseReleaseEvent(event);
}
