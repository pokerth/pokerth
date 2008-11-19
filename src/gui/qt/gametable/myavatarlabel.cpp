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
#include "session.h"
#include "playerinterface.h"
#include "game.h"

using namespace std;

MyAvatarLabel::MyAvatarLabel(QGroupBox* parent)
 : QLabel(parent)
{

	myContextMenu = new QMenu;
	action_VoteForKick = new QAction(QIcon(":/gfx/list_remove_user.png"), tr("Start vote to kick this user"), myContextMenu);
	myContextMenu->addAction(action_VoteForKick);

	connect( action_VoteForKick, SIGNAL ( triggered() ), this, SLOT ( sendTriggerVoteOnKickSignal() ) );
}


MyAvatarLabel::~MyAvatarLabel()
{
}

void MyAvatarLabel::contextMenuEvent ( QContextMenuEvent *event ) {

	
	Game *currentGame = myW->getSession()->getCurrentGame();

	PlayerListConstIterator it_c;
	int i=0;
 	for (it_c=currentGame->getSeatsList()->begin(); it_c!=currentGame->getSeatsList()->end(); it_c++) { 
		
		if(myContextMenuEnabled && myId != 0 && myId == i && (*it_c)->getMyActiveStatus() )
			showContextMenu(event->globalPos());

		i++;
	}
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

void MyAvatarLabel::setVoteOnKickContextMenuEnabled(bool b)
{
	action_VoteForKick->setEnabled(b);
}