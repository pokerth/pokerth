/***************************************************************************
 *   Copyright (C) 2006 by FThauer FHammer   *
 *   f.thauer@web.de   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "startnetworkgamedialogimpl.h"
#include "session.h"
#include "configfile.h"
#include <net/socket_msg.h>

startNetworkGameDialogImpl::startNetworkGameDialogImpl(QWidget *parent, ConfigFile *config)
      : QDialog(parent), myW(NULL), isAdmin(false), myConfig(config), mySession(NULL)
{
	setupUi(this);

	connect( pushButton_cancel, SIGNAL( clicked() ), this, SLOT( cancel() ) );
	connect( pushButton_startGame, SIGNAL( clicked() ), this, SLOT( startGame() ) );
	connect( pushButton_Kick, SIGNAL( clicked() ), this, SLOT( kickPlayer() ) );
	connect( treeWidget, SIGNAL( itemClicked ( QTreeWidgetItem*, int) ), this, SLOT( playerSelected(QTreeWidgetItem*, int) ) );
	connect( treeWidget, SIGNAL( clear () ), this, SLOT( clearPlayers() ) );

	pushButton_Kick->setEnabled(false);
	pushButton_startGame->setEnabled(false);
}

void startNetworkGameDialogImpl::startGame() {
	assert(mySession);
	mySession->sendStartEvent();
}

void startNetworkGameDialogImpl::cancel() {
	
}

void startNetworkGameDialogImpl::refresh(int actionID) {

	if (actionID == MSG_NET_GAME_CLIENT_START)
	{
		QTimer::singleShot(500, this, SLOT(accept()));
	}
}

void startNetworkGameDialogImpl::joinedNetworkGame(QString playerName, int rights) {

	isAdmin = rights == PLAYER_RIGHTS_ADMIN;
	addConnectedPlayer(playerName, rights);
}

void startNetworkGameDialogImpl::addConnectedPlayer(QString playerName, int rights) {

	QTreeWidgetItem *item = new QTreeWidgetItem(treeWidget,0);
	item->setData(0, 0, playerName);
	
	if(treeWidget->topLevelItemCount() != maxPlayerNumber) {
		myW->getMySDLPlayer()->playSound("playerconnected", 0);
	}
	else {
		myW->getMySDLPlayer()->playSound("onlinegameready", 0);
	}

	checkPlayerQuantity();
}

void startNetworkGameDialogImpl::updatePlayer(QString oldPlayerName, QString newPlayerName)
{
	QList<QTreeWidgetItem *> list = treeWidget->findItems(oldPlayerName, Qt::MatchExactly, 0);
	if(!list.empty()) { 
		list[0]->setText(0, newPlayerName);
	}
}

void startNetworkGameDialogImpl::removePlayer(QString playerName) {

	QList<QTreeWidgetItem *> list = treeWidget->findItems(playerName, Qt::MatchExactly, 0);
	if(!list.empty()) { 
		treeWidget->takeTopLevelItem(treeWidget->indexOfTopLevelItem(list[0]));
	}

	checkPlayerQuantity();
}

void startNetworkGameDialogImpl::playerSelected(QTreeWidgetItem*, int) {

	pushButton_Kick->setEnabled(isAdmin);
}

void startNetworkGameDialogImpl::kickPlayer() {

	
	QTreeWidgetItem *item = treeWidget->currentItem();
	if (item)
	{
		QString playerName = item->text(0);
		if(playerName == QString::fromUtf8(myConfig->readConfigString("MyName").c_str())) {
			{ QMessageBox::warning(this, tr("Server Error"),
					tr("You should not kick yourself from this game!"),
					QMessageBox::Close); }
		}
		else {
			assert(mySession);
			mySession->kickPlayer(playerName.toUtf8().constData());
		}
	}
	pushButton_Kick->setEnabled(false);
}

void startNetworkGameDialogImpl::checkPlayerQuantity() {

	if (treeWidget->topLevelItemCount() >= 2 && isAdmin) {
		pushButton_startGame->setEnabled(true);
	}
	else {
		pushButton_startGame->setEnabled(false);
	}

}

void startNetworkGameDialogImpl::clearPlayers()
{
	pushButton_Kick->setEnabled(false);
}

void startNetworkGameDialogImpl::setSession(Session *session)
{
	mySession = session;
}

void startNetworkGameDialogImpl::keyPressEvent ( QKeyEvent * event ) {


	if (event->key() == 16777220) { pushButton_startGame->click(); } //ENTER 
	
}
