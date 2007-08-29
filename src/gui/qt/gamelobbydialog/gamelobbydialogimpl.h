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
#include "sdlplayer.h"
#include "mainwindowimpl.h"


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
	void setMyW ( mainWindowImpl* theValue ) { myW = theValue; }
	
public slots:

	void createGame();
	void joinGame();

	void gameSelected(QTreeWidgetItem*, int);

	void addGame(QString gameName);
	void removeGame(QString gameName);

	void setCurrentGameName ( const QString& theValue ) { currentGameName = theValue; }
	QString getCurrentGameName() const { return currentGameName; }	

	void clearGames();
	void checkPlayerQuantity();

	void joinedNetworkGame(QString, int);
	void addConnectedPlayer(QString, int);
	void updatePlayer(QString, QString);
	void removePlayer(QString);

private:
	
	mainWindowImpl* myW;
	ConfigFile *myConfig;	
	Session *mySession;
	createInternetGameDialogImpl *myCreateInternetGameDialog;

	QString currentGameName;
	bool isAdmin;
};

#endif
