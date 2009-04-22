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
#include "myavatarlabel.h"

#include "gametableimpl.h"
#include "session.h"
#include "playerinterface.h"
#include "game.h"

using namespace std;

MyAvatarLabel::MyAvatarLabel(QGroupBox* parent)
 : QLabel(parent), voteRunning(FALSE), transparent(FALSE)
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

        assert(myW->getSession()->getCurrentGame());
        if (myW->getSession()->isNetworkClientRunning()) {

                PlayerListIterator it = myW->getSession()->getCurrentGame()->getSeatsList()->begin();
                //only active players are allowed to start a vote
                if((*it)->getMyActiveStatus()) {

                        Game *currentGame = myW->getSession()->getCurrentGame();

                        PlayerListConstIterator it_c;
                        int activePlayerCounter=0;
                        for (it_c=currentGame->getSeatsList()->begin(); it_c!=currentGame->getSeatsList()->end(); it_c++) {
                                if((*it_c)->getMyActiveStatus()) activePlayerCounter++;
                        }
                        if(activePlayerCounter > 2 && !voteRunning ) setVoteOnKickContextMenuEnabled(TRUE);
                        else setVoteOnKickContextMenuEnabled(FALSE);

                        int i=0;
                        for (it_c=currentGame->getSeatsList()->begin(); it_c!=currentGame->getSeatsList()->end(); it_c++) {

                                //also inactive player which stays on table can be voted to kick
                                if(myContextMenuEnabled && myId != 0 && myId == i && (*it_c)->getMyType() != PLAYER_TYPE_COMPUTER && ( (*it_c)->getMyActiveStatus() || (*it_c)->getMyStayOnTableStatus() ) )
                                        showContextMenu(event->globalPos());

                                i++;
                        }
                }
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

void MyAvatarLabel::setPixmap ( const QPixmap &pix, const bool trans) {

        myPixmap = pix.scaled(50,50, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        transparent = trans;
        update();

}

void MyAvatarLabel::paintEvent(QPaintEvent*) {

        QPainter painter(this);
        if(transparent)
                painter.setOpacity(0.4);
        else
                painter.setOpacity(1.0);

        painter.drawPixmap(0,0,myPixmap);
}

