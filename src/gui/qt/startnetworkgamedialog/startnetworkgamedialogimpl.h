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
#ifndef STARTNETWORKGAMEDIALOGIMPL_H
#define STARTNETWORKGAMEDIALOGIMPL_H

#include "ui_startnetworkgamedialog.h"

#include <QtGui>
#include <QtCore>

class Session;
class ConfigFile;

class startNetworkGameDialogImpl: public QDialog, public Ui::startNetworkGameDialog {
Q_OBJECT
public:
    startNetworkGameDialogImpl(QWidget *parent = 0, ConfigFile *config = 0);

public slots:

	void startGame();
	void cancel();
	void addConnectedPlayer(QString playerName);
	void removePlayer(QString playerName);
	void playerSelected(QTreeWidgetItem*, int);
	void kickPlayer();
	void checkPlayerQuantity();

	void setSession(Session *session);

	void keyPressEvent ( QKeyEvent*);

	void setMaxPlayerNumber ( int theValue ) { maxPlayerNumber = theValue; label_maxPlayerNumber->setText(QString::number(theValue,10)); }
	int getMaxPlayerNumber() const { return maxPlayerNumber; }
	

private: 

	int maxPlayerNumber;
	ConfigFile *myConfig;
	Session *mySession;
};

#endif
