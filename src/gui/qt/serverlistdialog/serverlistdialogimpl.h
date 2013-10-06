/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
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
 *                                                                           *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/
#ifndef SERVERLISTDIALOGIMPL_H
#define SERVERLISTDIALOGIMPL_H

#ifdef GUI_800x480
#include "ui_serverlistdialog_800x480.h"
#else
#include "ui_serverlistdialog.h"
#endif

#include <QtGui>
#include <QtCore>
#if QT_VERSION >= 0x050000
#include <QtWidgets>
#endif

class ConfigFile;
class startWindowImpl;

class serverListDialogImpl: public QDialog, public Ui::ServerListDialog
{
	Q_OBJECT
public:
	serverListDialogImpl(startWindowImpl *sw, QMainWindow *parent = 0, ConfigFile* = 0 );

public slots:

	int exec();
	void clearList();
	void addServerItem(unsigned);
	void connectToServer();
	void closeNetworkClient();

private:

	ConfigFile *myConfig;
	startWindowImpl *mySw;

};

#endif
int exec();
