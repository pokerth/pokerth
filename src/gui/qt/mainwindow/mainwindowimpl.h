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
#ifndef MAINWINDOWIMPL_H
#define MAINWINDOWIMPL_H

#include "ui_mainwindow.h"
#include <iostream>
#include <string>

#include <QtGui>
#include <QtCore>

#include "handinterface.h"
#include "game_defs.h"
#include <gamedata.h>

class Session;
class Game;
class Log;
class ConfigFile;

class BoardInterface;
// class HandInterface;
class PlayerInterface;
class MyCardsPixmapLabel;
class newGameDialogImpl;
class joinNetworkGameDialogImpl;
class connectToServerDialogImpl;
class createNetworkGameDialogImpl;
class startNetworkGameDialogImpl;
class waitForServerToStartGameDialogImpl;

class QColor;


class mainWindowImpl: public QMainWindow, public Ui::mainWindow {
Q_OBJECT

public:
	mainWindowImpl(QMainWindow *parent = 0 );

	~mainWindowImpl();

	void initGui(int speed);

	void setGame(Game*);
	void setHand(HandInterface*);
	void setSession(Session*);
	void setLog(Log*);

	void setActualHand(HandInterface* theValue) { actualHand = theValue;}
	HandInterface* getActualHand() const {  return actualHand;}


	//refresh-Funktionen
	void refreshSet();
	void refreshButton();
	void refreshPlayerName();
	void refreshPlayerAvatar();
	void refreshAction(int =-1, int=-1);
	void refreshCash();
	void refreshGroupbox(int =-1, int =-1);
	void refreshAll();
	void refreshChangePlayer();
	void refreshPot();

	// Karten-Funktionen
	void dealHoleCards();

	//Spieler-Funktionen
	void meInAction();
	void disableMyButtons();

	void highlightRoundLabel(std::string);

	void setSpeeds();

signals:
	void SignalNetClientConnect(int actionID);
	void SignalNetClientGameInfo(int actionID);
	void SignalNetClientError(int errorID, int osErrorID);
	void SignalNetClientGameStart(int numberOfPlayers, int startCash, int smallBlind, int handsBeforeRaise);
	void SignalNetServerPlayerJoined(QString playerName);
	void SignalNetServerPlayerLeft(QString playerName);

public slots:

	void setGameSpeed(const int theValue) { guiGameSpeed = theValue; setSpeeds(); } // Achtung Faktor 10!!!

	void callNewGameDialog() ;
	void callAboutPokerthDialog();
	void callSettingsDialog();
	void callCreateNetworkGameDialog();
	void callJoinNetworkGameDialog();

	void myBetRaise();
	void myFoldAllin();
	void myCallCheckSet();

	void myFold();
	void myCheck();
	void myCall();
	void myBet();
	void mySet();
	void myRaise();
	void myAllIn();

	void dealFlopCards0();
	void dealFlopCards1();
	void dealFlopCards2();
	void dealFlopCards3();
	void dealFlopCards4();
	void dealFlopCards5();
	void dealFlopCards6();

	void dealTurnCards0();
	void dealTurnCards1();
	void dealTurnCards2();

	void dealRiverCards0();
	void dealRiverCards1();
	void dealRiverCards2();

	void nextPlayerAnimation();

	void preflopAnimation1();
	void preflopAnimation1Action();
	void preflopAnimation2();
	void preflopAnimation2Action();

	void flopAnimation1();
	void flopAnimation1Action();
	void flopAnimation2();
	void flopAnimation2Action();

	void turnAnimation1();
	void turnAnimation1Action();
	void turnAnimation2();
	void turnAnimation2Action();

	void riverAnimation1();
	void riverAnimation1Action();
	void riverAnimation2();
	void riverAnimation2Action();

	void postRiverAnimation1();
	void postRiverAnimation1Action();

	void postRiverRunAnimation1();
	void postRiverRunAnimation2();
	void postRiverRunAnimation3();	
	void postRiverRunAnimation4();	
	void postRiverRunAnimation5();	
	void postRiverRunAnimation6();

	void blinkingStartButtonAnimationAction();

	void flipHolecardsAllIn();
	void showMyCards();

	void handSwitchRounds();

	void startNewHand();
	void startNewLocalGame(newGameDialogImpl* =0);

	void stopTimer();
	
	void nextRoundCleanGui();
	
	void breakButtonClicked();

	void keyPressEvent ( QKeyEvent*);
// 	bool event ( QEvent * );

	void switchLeftToolBox();
	void switchRightToolBox();
	void switchFullscreen();

	void paintStartSplash();

	void networkError(int, int);
	void networkStart(int, int, int, int);

private: 
	
	Game *actualGame;
	HandInterface *actualHand;
	Session *mySession;
	Log *myLog;
	ConfigFile *myConfig;

	//Logo 
	QLabel *label_logoleft;
	QLabel *label_logoright;

	//MyPixmapCardsLabel
	MyCardsPixmapLabel *pixmapLabel_cardBoard0;
	MyCardsPixmapLabel *pixmapLabel_cardBoard1;
	MyCardsPixmapLabel *pixmapLabel_cardBoard2;
	MyCardsPixmapLabel *pixmapLabel_cardBoard3;
	MyCardsPixmapLabel *pixmapLabel_cardBoard4;
	MyCardsPixmapLabel *pixmapLabel_card0a;
	MyCardsPixmapLabel *pixmapLabel_card0b;
	MyCardsPixmapLabel *pixmapLabel_card1a;
	MyCardsPixmapLabel *pixmapLabel_card1b;
	MyCardsPixmapLabel *pixmapLabel_card2a;
	MyCardsPixmapLabel *pixmapLabel_card2b;
	MyCardsPixmapLabel *pixmapLabel_card3a;
	MyCardsPixmapLabel *pixmapLabel_card3b;
	MyCardsPixmapLabel *pixmapLabel_card4a;
	MyCardsPixmapLabel *pixmapLabel_card4b;
	MyCardsPixmapLabel *pixmapLabel_card5a;
	MyCardsPixmapLabel *pixmapLabel_card5b;
	MyCardsPixmapLabel *pixmapLabel_card6a;
	MyCardsPixmapLabel *pixmapLabel_card6b;
	//Timer
	QTimer *potDistributeTimer;
	QTimer *timer;
	QTimer *dealFlopCards0Timer;
	QTimer *dealFlopCards1Timer;
	QTimer *dealFlopCards2Timer;
	QTimer *dealFlopCards3Timer;
	QTimer *dealFlopCards4Timer;
	QTimer *dealFlopCards5Timer;
	QTimer *dealFlopCards6Timer;
	QTimer *dealTurnCards0Timer;
	QTimer *dealTurnCards1Timer;
	QTimer *dealTurnCards2Timer;
	QTimer *dealRiverCards0Timer;
	QTimer *dealRiverCards1Timer;
	QTimer *dealRiverCards2Timer;

	QTimer *nextPlayerAnimationTimer;
	QTimer *preflopAnimation1Timer;
	QTimer *preflopAnimation2Timer;
	QTimer *flopAnimation1Timer;
	QTimer *flopAnimation2Timer;
	QTimer *turnAnimation1Timer;
	QTimer *turnAnimation2Timer;
	QTimer *riverAnimation1Timer;
	QTimer *riverAnimation2Timer;

	QTimer *postRiverAnimation1Timer;
	QTimer *postRiverRunAnimation1Timer;
	QTimer *postRiverRunAnimation2Timer;
	QTimer *postRiverRunAnimation3Timer;
	QTimer *postRiverRunAnimation5Timer;
	QTimer *postRiverRunAnimation6Timer;

	QTimer *blinkingStartButtonAnimationTimer;
	

	QWidget *userWidgetsArray[4];
	QLabel *buttonLabelArray[MAX_NUMBER_OF_PLAYERS];
	QLabel *cashLabelArray[MAX_NUMBER_OF_PLAYERS];
	QLabel *cashTopLabelArray[MAX_NUMBER_OF_PLAYERS];
	QLabel *setLabelArray[MAX_NUMBER_OF_PLAYERS];
	QLabel *actionLabelArray[MAX_NUMBER_OF_PLAYERS];
	QLabel *playerNameLabelArray[MAX_NUMBER_OF_PLAYERS];
	QLabel *playerAvatarLabelArray[MAX_NUMBER_OF_PLAYERS];

	QGroupBox *groupBoxArray[MAX_NUMBER_OF_PLAYERS];
	MyCardsPixmapLabel *boardCardsArray[5];
	MyCardsPixmapLabel *holeCardsArray[MAX_NUMBER_OF_PLAYERS][2];

	QPixmap *flipside;

	//Dialoge
	joinNetworkGameDialogImpl *myJoinNetworkGameDialog;
	connectToServerDialogImpl *myConnectToServerDialog;
	startNetworkGameDialogImpl *myStartNetworkGameDialog;
	createNetworkGameDialogImpl *myCreateNetworkGameDialog;
	waitForServerToStartGameDialogImpl *myWaitingForServerGameDialog;

	int distributePotAnimCounter;

	//Speed
	int guiGameSpeed;
	int gameSpeed;
	int dealCardsSpeed;
	int preDealCardsSpeed;
	int postDealCardsSpeed;
	int AllInDealCardsSpeed;
	int postRiverRunAnimationSpeed;
	int winnerBlinkSpeed; 
	int newRoundSpeed;
	int nextPlayerSpeed1;
	int nextPlayerSpeed2;
	int nextPlayerSpeed3;
	int preflopNextPlayerSpeed;
	int nextOpponentSpeed;	


	bool debugMode;
	bool breakAfterActualHand;
	
	bool flipHolecardsAllInAlreadyDone;

	QColor active;
	QColor inactive;
	QColor highlight;

	// statistic testing
	int statisticArray[15];

friend class GuiWrapper;
};

#endif
