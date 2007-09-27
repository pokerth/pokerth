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

#include <iostream>

#include "createinternetgamedialogimpl.h"
#include "sdlplayer.h"
#include "mainwindowimpl.h"


class Session;
class ConfigFile;
class LobbyChat;

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
	Session& getSession() { return *mySession; }

	void setMyW ( mainWindowImpl* theValue ) { myW = theValue; }

signals:
	void signalChatConnect(QString server);
	void signalChatSelfJoined(QString nickName, QString channel);
	void signalChatPlayerJoined(QString nickName);
	void signalChatPlayerLeft(QString nickName);
	void signalChatMessage(QString nickName, QString msg);

public slots:

	void createGame();
	void joinGame();

	void gameSelected(QTreeWidgetItem*, QTreeWidgetItem*);
	void updateGameItem(QTreeWidgetItem *item, unsigned gameId);

	void addGame(unsigned gameId);
	void removeGame(unsigned gameId);
	void gameAddPlayer(unsigned gameId, unsigned playerId);
	void gameRemovePlayer(unsigned gameId, unsigned playerId);

	void setCurrentGameName ( const QString& theValue ) { currentGameName = theValue; }
	QString getCurrentGameName() const { return currentGameName; }	

	void checkPlayerQuantity();

	void joinedNetworkGame(unsigned, QString, int);
	void addConnectedPlayer(unsigned, QString, int);
	void updatePlayer(unsigned, QString);
	void removePlayer(unsigned, QString);

	void playerSelected(QTreeWidgetItem*, QTreeWidgetItem*);
	void refresh(int actionID);
	void removedFromGame(int reason);
	void startGame();
	void leaveGame();
	void kickPlayer();

	void gameModeDialogUpdate();
	void clearDialog();

	void sendChatMessage();
	void checkChatInputLength(QString string);
	
	void keyPressEvent(QKeyEvent * event); 

private:
	
	mainWindowImpl* myW;
	ConfigFile *myConfig;	
	Session *mySession;
	createInternetGameDialogImpl *myCreateInternetGameDialog;
	LobbyChat *myChat;

	QString currentGameName;
	bool isAdmin;
};

#endif
