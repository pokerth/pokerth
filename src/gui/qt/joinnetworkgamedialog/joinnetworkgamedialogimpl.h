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
 *****************************************************************************/
#ifndef JOINNETWORKGAMEDIALOGIMPL_H
#define JOINNETWORKGAMEDIALOGIMPL_H

#ifdef GUI_800x480
#include "ui_joinnetworkgamedialog_800x480.h"
#else
#include "ui_joinnetworkgamedialog.h"
#endif

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
