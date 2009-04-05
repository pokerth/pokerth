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
#include "serverlistdialogimpl.h"
#include "configfile.h"
#include "startwindowimpl.h"
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
	

	connect( buttonBox, SIGNAL( accepted() ), this, SLOT( connectToServer() ));
	
}

void serverListDialogImpl::exec() {

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
}

void serverListDialogImpl::connectToServer()
{
	QTreeWidgetItem *item = treeWidget_serverList->currentItem();
	if (item) {
		mySw->getSession()->selectServer(item->data(0, Qt::UserRole).toUInt());
	}
}
