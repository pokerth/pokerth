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

#include <gamedata.h>
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

	LobbyChat *getLobbyChat() { return myChat; }

	void setSession(Session *session);
	Session& getSession() { return *mySession; }

	void setMyW ( mainWindowImpl* theValue ) { myW = theValue; }

public slots:

	void createGame();
	void joinGame();

	void gameSelected(QTreeWidgetItem*, QTreeWidgetItem*);
	void updateGameItem(QTreeWidgetItem *item, unsigned gameId);

	void addGame(unsigned gameId);
	void updateGameMode(unsigned gameId, int newMode);
	void removeGame(unsigned gameId);
	void gameAddPlayer(unsigned gameId, unsigned playerId);
	void gameRemovePlayer(unsigned gameId, unsigned playerId);

	void setCurrentGameName ( const QString& theValue ) { currentGameName = theValue; }
	QString getCurrentGameName() const { return currentGameName; }	

	mainWindowImpl* getMyW() const { return myW; }
	void checkPlayerQuantity();

	void joinedNetworkGame(unsigned, QString, int);
	void addConnectedPlayer(unsigned, QString, int);
	void updatePlayer(unsigned, QString);
	void removePlayer(unsigned, QString);
	void newGameAdmin(unsigned, QString);

	void playerSelected(QTreeWidgetItem*, QTreeWidgetItem*);
	void refresh(int actionID);
	void removedFromGame(int reason);
	void startGame();
	void leaveGame();
	void kickPlayer();

	void joinedGameDialogUpdate();
	void leftGameDialogUpdate();
	void updateDialogBlinds(const GameData &gameData);
	void clearDialog();

	void sendChatMessage();
	void checkChatInputLength(QString string);
	
	void keyPressEvent(QKeyEvent * keyEvent); 
	bool event(QEvent * event); 
	void hideShowGameDescription(bool show);


private:
	
	mainWindowImpl* myW;
	ConfigFile *myConfig;	
	Session *mySession;
	createInternetGameDialogImpl *myCreateInternetGameDialog;
	QString currentGameName;
	unsigned myPlayerId;
	bool isAdmin;
	bool inGame;
	LobbyChat *myChat;
	QString myAppDataPath;
	int keyUpCounter;

 protected:
         bool eventFilter(QObject *obj, QEvent *event);
	
};

#endif
