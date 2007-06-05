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
// #include "configfile.h"

startNetworkGameDialogImpl::startNetworkGameDialogImpl(QWidget *parent)
      : QDialog(parent)
{

    setupUi(this);

	connect( pushButton_cancel, SIGNAL( clicked() ), this, SLOT( cancel() ) );
	connect( pushButton_startGame, SIGNAL( clicked() ), this, SLOT( startGame() ) );
	connect( pushButton_Kick, SIGNAL( clicked() ), this, SLOT( kickPlayer() ) );
	connect( treeWidget, SIGNAL( itemClicked ( QTreeWidgetItem*, int) ), this, SLOT( playerSelected(QTreeWidgetItem*, int) ) );

	pushButton_Kick->setEnabled(FALSE);
}

void startNetworkGameDialogImpl::startGame() {
	
}

void startNetworkGameDialogImpl::cancel() {
	
}


void startNetworkGameDialogImpl::addConnectedPlayer(QString playerName) {

	QTreeWidgetItem *item = new QTreeWidgetItem(treeWidget,0);
	item->setData(0, 0, playerName);
	
	checkPlayerQuantity();
}

void startNetworkGameDialogImpl::removePlayer(QString playerName) {

	QList<QTreeWidgetItem *> list = treeWidget->findItems(playerName, Qt::MatchExactly, 0);
	if(!list.empty()) { 
		treeWidget->takeTopLevelItem(treeWidget->indexOfTopLevelItem(list[0]));
	}

	checkPlayerQuantity();
}

void startNetworkGameDialogImpl::playerSelected(QTreeWidgetItem*, int) {

	pushButton_Kick->setEnabled(TRUE);
}

void startNetworkGameDialogImpl::kickPlayer() {

// 	if()
	QTreeWidgetItem *item = treeWidget->currentItem();
	QString playerName = item->text(0);
// 	kickplayerFunktion(playerName.toStdString());

	pushButton_Kick->setEnabled(FALSE);

}

void startNetworkGameDialogImpl::checkPlayerQuantity() {

	if(treeWidget->topLevelItemCount() == maxPlayerNumber) pushButton_startGame->setEnabled(TRUE);
	else pushButton_startGame->setDisabled(TRUE);

}

void startNetworkGameDialogImpl::keyPressEvent ( QKeyEvent * event ) {


	if (event->key() == 16777220) { pushButton_startGame->click(); } //ENTER 
	
}
