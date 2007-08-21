//
// C++ Interface: gamelobbydialogimpl
//
// Description: 
//
//
// Author: FThauer FHammer <webmaster@pokerth.net>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef GAMELOBBYDIALOGIMPL_H
#define GAMELOBBYDIALOGIMPL_H

#include <ui_gamelobbydialog.h>

#include <QtGui>
#include <QtCore>

#include "createinternetgamedialogimpl.h"

class Session;
class ConfigFile;

/**
	@author FThauer FHammer <webmaster@pokerth.net>
*/
class gameLobbyDialogImpl: public QDialog, public Ui::gameLobbyDialog {
Q_OBJECT
public:
	gameLobbyDialogImpl(QWidget *parent = 0, ConfigFile* = 0);

	~gameLobbyDialogImpl();

	void exec();

	void setSession(Session *session);

public slots:

	void createGame();
	void joinGame();

	void gameSelected(QTreeWidgetItem*, int);

	void addGame(QString gameName);
	void removeGame(QString gameName);

private:

	ConfigFile *myConfig;	
	Session *mySession;

	createInternetGameDialogImpl *myCreateInternetGameDialog;
};

#endif
