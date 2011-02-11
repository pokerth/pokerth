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
#ifndef SERVERLISTDIALOGIMPL_H
#define SERVERLISTDIALOGIMPL_H

#include "ui_serverlistdialog.h"

#include <QtGui>
#include <QtCore>


class ConfigFile;
class startWindowImpl;

class serverListDialogImpl: public QDialog, public Ui::ServerListDialog
{
	Q_OBJECT
public:
	serverListDialogImpl(startWindowImpl *sw, QMainWindow *parent = 0, ConfigFile* = 0 );

public slots:

	void exec();
	void clearList();
	void addServerItem(unsigned);
	void connectToServer();
	void closeNetworkClient();

private:

	ConfigFile *myConfig;
	startWindowImpl *mySw;

};

#endif
void exec();