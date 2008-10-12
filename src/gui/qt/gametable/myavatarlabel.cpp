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
	action_VoteForKick = new QAction(tr("Start vote on kick this user"), myContextMenu);
	myContextMenu->addAction(action_VoteForKick);

	connect( action_VoteForKick, SIGNAL ( triggered() ), this, SLOT ( sendTriggerVoteOnKickSignal() ) );
}


MyAvatarLabel::~MyAvatarLabel()
{
}

void MyAvatarLabel::contextMenuEvent ( QContextMenuEvent *event ) {

	if(myContextMenuEnabled)
		showContextMenu(event->globalPos());
}

void MyAvatarLabel::showContextMenu(const QPoint &pos) {

	myContextMenu->popup(pos);
}

void MyAvatarLabel::sendTriggerVoteOnKickSignal()
{
	myW->triggerVoteOnKick(myId);
}

void MyAvatarLabel::setEnabledContextMenu(bool b)
{
	myContextMenuEnabled = b;
}
