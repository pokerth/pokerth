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
#include "waitforservertostartgamedialogimpl.h"
#include "session.h"
// #include "configfile.h"

waitForServerToStartGameDialogImpl::waitForServerToStartGameDialogImpl(QWidget *parent)
      : QDialog(parent)
{

    setupUi(this);

	connect( pushButton_cancel, SIGNAL( clicked() ), this, SLOT( cancel() ) );

}

void waitForServerToStartGameDialogImpl::refresh(int actionID) {
	QTimer::singleShot(1000, this, SLOT(accept()));
}

void waitForServerToStartGameDialogImpl::cancel() {
	
}

void waitForServerToStartGameDialogImpl::keyPressEvent ( QKeyEvent * event ) {


// 	if (event->key() == 16777220) { pushButton_connect->click(); } //ENTER 
	
}

void waitForServerToStartGameDialogImpl::addConnectedPlayer(QString playerName) {

	QTreeWidgetItem *item = new QTreeWidgetItem(treeWidget,0);
	item->setData(0, 0, playerName);
}

void waitForServerToStartGameDialogImpl::removePlayer(QString playerName) {

	QList<QTreeWidgetItem *> list = treeWidget->findItems(playerName, Qt::MatchExactly, 0);
	if(!list.empty()) { 
		treeWidget->takeTopLevelItem(treeWidget->indexOfTopLevelItem(list[0]));
	}
}
