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
#ifndef JOINNETWORKGAMEDIALOGIMPL_H
#define JOINNETWORKGAMEDIALOGIMPL_H

#include "ui_joinnetworkgamedialog.h"

#include <string>
#include "configfile.h"
#include <QtGui>
#include <QtCore>

class Session;
class ConfigFile;

class joinNetworkGameDialogImpl: public QDialog, public Ui::joinNetworkGameDialog
{
	Q_OBJECT
public:
	joinNetworkGameDialogImpl(QWidget *parent = 0, ConfigFile *c = 0);

	ConfigFile *myConfig;
	std::string myServerProfilesFile;

	void exec();

public slots:

	void startClient();
	void fillServerProfileList();
	void itemFillForm (QTreeWidgetItem* item, int column);
	void saveServerProfile();
	void deleteServerProfile();
	void keyPressEvent ( QKeyEvent * event );
	void checkIp();
	void connectButtonTest();
};

#endif
