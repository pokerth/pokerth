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
#include "mymessagedialogimpl.h"
#include "chattools.h"

using namespace std;

MyAvatarLabel::MyAvatarLabel(QGroupBox* parent)
    : QLabel(parent), voteRunning(FALSE), transparent(FALSE)
{

    myContextMenu = new QMenu;
    action_VoteForKick = new QAction(QIcon(":/gfx/list_remove_user.png"), tr("Start vote to kick this player"), myContextMenu);
    myContextMenu->addAction(action_VoteForKick);
    action_IgnorePlayer = new QAction(QIcon(":/gfx/im-ban-user.png"), tr("Ignore Player"), myContextMenu);
    myContextMenu->addAction(action_IgnorePlayer);
    action_ReportBadAvatar = new QAction(QIcon(":/gfx/emblem-important.png"), tr("Report inappropriate avatar"), myContextMenu);
    myContextMenu->addAction(action_ReportBadAvatar);


    connect( action_VoteForKick, SIGNAL ( triggered() ), this, SLOT ( sendTriggerVoteOnKickSignal() ) );
    connect( action_IgnorePlayer, SIGNAL ( triggered() ), this, SLOT ( putPlayerOnIgnoreList() ) );
    connect( action_ReportBadAvatar, SIGNAL ( triggered() ), this, SLOT ( reportBadAvatar() ) );
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
            GameInfo info(myW->getSession()->getClientGameInfo(myW->getSession()->getClientCurrentGameId()));

            if(activePlayerCounter > 2 && !voteRunning && info.data.gameType != GAME_TYPE_RANKING && !myW->getGuestMode()) {
                setVoteOnKickContextMenuEnabled(TRUE);
            }
            else {
                setVoteOnKickContextMenuEnabled(FALSE);
            }

            action_IgnorePlayer->setEnabled(true);

            int j=0;
            for (it_c=currentGame->getSeatsList()->begin(); it_c!=currentGame->getSeatsList()->end(); it_c++) {

                if(myId == j) {

                    if(myW->getSession()->getClientPlayerInfo((*it_c)->getMyUniqueID()).isGuest) {
                        action_IgnorePlayer->setDisabled(true);
                    }

                    if(myW->getSession()->getGameType() == Session::GAME_TYPE_INTERNET && !((*it_c)->getMyAvatar().empty()) ) {
                        action_ReportBadAvatar->setVisible(TRUE);
                    }
                    else {
                        action_ReportBadAvatar->setVisible(FALSE);
                    }
                }
                j++;
            }

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

void MyAvatarLabel::setPixmapAndCountry ( const QPixmap &pix,QString countryString, int seatPlace, const bool trans) {
    
    QPixmap resultAvatar(pix.scaled(50,50, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    QPainter painter(&resultAvatar);
    int showCountryFlags = myW->getMyConfig()->readConfigInt("ShowCountryFlagInAvatar");
    if(showCountryFlags && !countryString.isEmpty())
    {
        if(seatPlace<=2||seatPlace>=8)
	{
            painter.drawPixmap(resultAvatar.width()-(QPixmap(countryString)).width(),resultAvatar.height()-(QPixmap(countryString)).height(),QPixmap(countryString));
    	}
	else
	{
            painter.drawPixmap(resultAvatar.width()-(QPixmap(countryString)).width(),0,QPixmap(countryString));
	}
    }
    painter.end();
    myPixmap = resultAvatar; 
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

bool MyAvatarLabel::playerIsOnIgnoreList(QString playerName) {

    list<std::string> playerIgnoreList = myW->getMyConfig()->readConfigStringList("PlayerIgnoreList");
    list<std::string>::iterator it1;
    for(it1= playerIgnoreList.begin(); it1 != playerIgnoreList.end(); it1++) {

        if(playerName == QString::fromUtf8(it1->c_str())) {
            return true;
        }
    }
    return false;
}


void MyAvatarLabel::putPlayerOnIgnoreList() {

    QStringList list;
    PlayerListConstIterator it_c;

    for (it_c=myW->getSession()->getCurrentGame()->getSeatsList()->begin(); it_c!=myW->getSession()->getCurrentGame()->getSeatsList()->end(); it_c++) {
        list << QString::fromUtf8((*it_c)->getMyName().c_str());
    }

    if(!playerIsOnIgnoreList(list.at(myId))) {

        myMessageDialogImpl dialog(myW->getMyConfig(), this);
        if(dialog.exec(4, tr("You will no longer receive chat messages or game invitations from this user.<br>Do you really want to put player <b>%1</b> on ignore list?").arg(list.at(myId)), tr("PokerTH - Question"), QPixmap(":/gfx/im-ban-user_64.png"), QDialogButtonBox::Yes|QDialogButtonBox::No, false ) == QDialog::Accepted) {

            std::list<std::string> playerIgnoreList = myW->getMyConfig()->readConfigStringList("PlayerIgnoreList");
            playerIgnoreList.push_back(list.at(myId).toUtf8().constData());
            myW->getMyConfig()->writeConfigStringList("PlayerIgnoreList", playerIgnoreList);
            myW->getMyConfig()->writeBuffer();

            myW->getMyChat()->refreshIgnoreList();
        }
    }
}

void MyAvatarLabel::reportBadAvatar() {

    Game *currentGame = myW->getSession()->getCurrentGame();
    int j=0;
    PlayerListConstIterator it_c;
    for (it_c=currentGame->getSeatsList()->begin(); it_c!=currentGame->getSeatsList()->end(); it_c++) {

        if(myId == j) {

            QString avatar = QString::fromUtf8((*it_c)->getMyAvatar().c_str());
            if(!avatar.isEmpty()) {

                QString nick = QString::fromUtf8((*it_c)->getMyName().c_str());
                int ret = QMessageBox::question(this, tr("PokerTH - Question"),
                                                tr("Are you sure you want to report the avatar of \"%1\" as inappropriate?").arg(nick), QMessageBox::Yes | QMessageBox::No);

                if(ret == QMessageBox::Yes) {
                    QFileInfo fi(avatar);
                    myW->getSession()->reportBadAvatar((*it_c)->getMyUniqueID(), fi.baseName().toStdString());
                }
            }
            break;
        }
        j++;
    }
}
