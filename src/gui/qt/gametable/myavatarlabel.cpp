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

MyAvatarLabel::MyAvatarLabel(QGroupBox* parent)
 : QLabel(parent)
{

	myContextMenu = new QMenu;
	myContextMenu->addAction(tr("Vote for kick this user"));

}


MyAvatarLabel::~MyAvatarLabel()
{
}

void MyAvatarLabel::contextMenuEvent ( QContextMenuEvent * /*event*/ ) {

// 	showContextMenu(event->globalPos());
}

void MyAvatarLabel::showContextMenu(const QPoint &/*pos*/) {

// 	myContextMenu->popup(pos);
}
