/***************************************************************************
 *   Copyright (C) 2006 by FThauer FHammer LMay   *
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
	connect( treeWidget, SIGNAL( currentItemChanged ( QTreeWidgetItem*, QTreeWidgetItem*) ), this, SLOT( playerSelected(QTreeWidgetItem*, QTreeWidgetItem*) ) );
	connect( lineEdit_ChatInput, SIGNAL( returnPressed () ), this, SLOT( sendChatMessage() ) );


	clearDialog();
}

void startNetworkGameDialogImpl::exec() {

	GameInfo info = mySession->getClientGameInfo(0);
	label_maxPlayerNumber->setText(QString::number(info.data.maxNumberOfPlayers));

	QDialog::exec();
	clearDialog();
}

void startNetworkGameDialogImpl::startGame() {
	assert(mySession);
	mySession->sendStartEvent(checkBox_fillUpWithComputerOpponents->isChecked());
}

void startNetworkGameDialogImpl::cancel() {
	
}

void startNetworkGameDialogImpl::refresh(int actionID) {

	if (actionID == MSG_NET_GAME_CLIENT_START)
	{
		QTimer::singleShot(500, this, SLOT(accept()));
	}
}

void startNetworkGameDialogImpl::joinedNetworkGame(unsigned playerId, QString playerName, int rights) {

	isAdmin = rights == PLAYER_RIGHTS_ADMIN;
	addConnectedPlayer(playerId, playerName, rights);
}

void startNetworkGameDialogImpl::addConnectedPlayer(unsigned playerId, QString playerName, int rights) {

	QTreeWidgetItem *item = new QTreeWidgetItem(treeWidget, 0);
	item->setData(0, Qt::UserRole, playerId);
	item->setData(0, Qt::DisplayRole, playerName);

	if(treeWidget->topLevelItemCount() != maxPlayerNumber) {
		myW->getMySDLPlayer()->playSound("playerconnected", 0);
	}
	else {
		myW->getMySDLPlayer()->playSound("onlinegameready", 0);
	}

	checkPlayerQuantity();
}

void startNetworkGameDialogImpl::updatePlayer(unsigned playerId, QString newPlayerName)
{
	QTreeWidgetItemIterator it(treeWidget);
	while (*it) {
		if ((*it)->data(0, Qt::UserRole) == playerId)
		{
			(*it)->setData(0, Qt::DisplayRole, newPlayerName);
			break;
		}
		++it;
	}
}

void startNetworkGameDialogImpl::removePlayer(unsigned playerId, QString) {

	QTreeWidgetItemIterator it(treeWidget);
	while (*it) {
		if ((*it)->data(0, Qt::UserRole) == playerId)
		{
			treeWidget->takeTopLevelItem(treeWidget->indexOfTopLevelItem(*it));
			break;
		}
		++it;
	}

	checkPlayerQuantity();
}

void startNetworkGameDialogImpl::playerSelected(QTreeWidgetItem* item, QTreeWidgetItem*) {

	if (item)
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
			mySession->kickPlayer(item->data(0, Qt::UserRole).toUInt());
		}
	}
	pushButton_Kick->setEnabled(false);
}

void startNetworkGameDialogImpl::checkPlayerQuantity() {

	if(isAdmin){
		pushButton_Kick->show();
		pushButton_startGame->show();
		checkBox_fillUpWithComputerOpponents->show();
		
		if (treeWidget->topLevelItemCount() >= 2) {
			pushButton_startGame->setEnabled(true);
		}
		else {
			pushButton_startGame->setEnabled(false);
		}
	}
}

void startNetworkGameDialogImpl::clearDialog()
{
	pushButton_Kick->hide();
	pushButton_startGame->hide();
	pushButton_Kick->setEnabled(false);
	pushButton_startGame->setEnabled(false);
	treeWidget->clear();
	checkBox_fillUpWithComputerOpponents->hide();
}

void startNetworkGameDialogImpl::setSession(Session *session)
{
	mySession = session;
}

void startNetworkGameDialogImpl::keyPressEvent ( QKeyEvent * event ) {


	if (event->key() == 16777220) { pushButton_startGame->click(); } //ENTER 
	
}

void startNetworkGameDialogImpl::sendChatMessage() { /*myChat->sendMessage();*/ }
