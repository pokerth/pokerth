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

class Game;
class Session;
class Log;
class ConfigFile;

class BoardInterface;
// class HandInterface;
class PlayerInterface;
class MyCardsPixmapLabel;
class joinNetworkGameDialogImpl;
class connectToServerDialogImpl;


class QColor;


class mainWindowImpl: public QMainWindow, public Ui::mainWindow {
Q_OBJECT

public:
	mainWindowImpl(QMainWindow *parent = 0 );

	~mainWindowImpl();

	void setGame(Game*);
	void setHand(HandInterface*);
	void setSession(Session*);
	void setLog(Log*);

	int getMaxQuantityPlayers() const { return maxQuantityPlayers; }
	
	void setActualHand(HandInterface* theValue) { actualHand = theValue;}
	HandInterface* getActualHand() const {  return actualHand;}


	//refresh-Funktionen
	void refreshSet();
	void refreshButton();
	void refreshPlayerName();
	void refreshAction();
	void refreshCash();
	void refreshGroupbox();
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
	void SignalNetClientSuccess(int actionID);
	void SignalNetClientError(int errorID, int osErrorID);

public slots:

	void setGameSpeed(const int theValue) { guiGameSpeed = theValue; setSpeeds(); } // Achtung Faktor 10!!!

	void callNewGameDialog() ;
	void callAboutPokerthDialog();
	void callSettingsDialog();
	void callJoinNetworkGameDialog();

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

	void flipHolecardsAllIn();

	void handSwitchRounds();

	void startNewHand();

	void stopTimer();
	
	void nextRoundCleanGui();
	
	void userWidgetsBackgroudColor();
	void breakButtonClicked();

	void keyPressEvent ( QKeyEvent * event );
	void switchToolBox();

	void paintStartSplash();

	void refreshConnectToServerDialog(int actionID);
	void errorConnectToServerDialog(int errorID, int osErrorID);

private: 
	
	Game *actualGame;
	HandInterface *actualHand;
	Session *mySession;
	Log *myLog;
	ConfigFile *myConfig;

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
	
	QWidget *userWidgetsArray[8];
	QLabel *buttonLabelArray[5];
	QLabel *cashLabelArray[5];
	QLabel *cashTopLabelArray[5];
	QLabel *setLabelArray[5];
	QLabel *actionLabelArray[5];
	QGroupBox *groupBoxArray[5];
	MyCardsPixmapLabel *boardCardsArray[5];
	MyCardsPixmapLabel *holeCardsArray[5][2];

	QPixmap *flipside;

	//Dialoge
	joinNetworkGameDialogImpl *myJoinNetworkGameDialog;
	connectToServerDialogImpl *myConnectToServerDialog;
	int maxQuantityPlayers;
	
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

friend class GuiWrapper;
};

#endif
