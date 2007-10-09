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
#include "mychatinputlineedit.h"

using namespace std;

MyChatInputLineEdit::MyChatInputLineEdit(QGroupBox* parent)
 : QLineEdit(parent)
{

}


MyChatInputLineEdit::~MyChatInputLineEdit()
{
}

void MyChatInputLineEdit::keyPressEvent( QKeyEvent * event ) {

	if (event->key() == Qt::Key_Tab && this->hasSelectedText()) {  }

	QLineEdit::event(event);
}
