/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2011 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *****************************************************************************/ #include "serverlistdialogimpl.h"
#include "configfile.h"
#include "startwindowimpl.h"
#include "connecttoserverdialogimpl.h"
#include "serverdata.h"
#include "session.h"

serverListDialogImpl::serverListDialogImpl(startWindowImpl *sw, QMainWindow *parent, ConfigFile *c)
	: QDialog(parent), myConfig(c), mySw(sw)
{
#ifdef __APPLE__
	setWindowModality(Qt::ApplicationModal);
	setWindowFlags(Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint | Qt::Dialog);
#endif
	setupUi(this);

	connect( treeWidget_serverList, SIGNAL( itemDoubleClicked ( QTreeWidgetItem*, int) ), this, SLOT( connectToServer() ));
	connect( buttonBox, SIGNAL( accepted() ), this, SLOT( connectToServer() ));
	connect( buttonBox, SIGNAL( rejected() ), this, SLOT( closeNetworkClient() ));
	connect( this, SIGNAL( rejected() ), this, SLOT( closeNetworkClient() ));
}

void serverListDialogImpl::exec()
{

	QDialog::exec();
}

void serverListDialogImpl::clearList()
{
	treeWidget_serverList->clear();
}

void serverListDialogImpl::addServerItem(unsigned serverId)
{
	ServerInfo info = mySw->getSession()->getClientServerInfo(serverId);
	QTreeWidgetItem *item = new QTreeWidgetItem(treeWidget_serverList);
	item->setData(0, Qt::DisplayRole, QString::fromUtf8(info.name.c_str()));
	item->setData(0, Qt::UserRole, serverId);
	item->setData(1, Qt::DisplayRole, QString::fromUtf8(info.country.c_str()));

	treeWidget_serverList->resizeColumnToContents(0);
	treeWidget_serverList->setCurrentItem(treeWidget_serverList->topLevelItem(0));
}

void serverListDialogImpl::connectToServer()
{
	QTreeWidgetItem *item = treeWidget_serverList->currentItem();
	if (item) {
		mySw->getSession()->selectServer(item->data(0, Qt::UserRole).toUInt());
	}
	this->hide();
}

void serverListDialogImpl::closeNetworkClient()
{

	mySw->getSession()->terminateNetworkClient();
	mySw->getMyConnectToServerDialog()->reject();
}
