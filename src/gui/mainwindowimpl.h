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

#include <QtGui>
#include <QtCore>

/*#include "newgamedialogimpl.h"
#include "aboutpokerthimpl.h"
#include "game.h"
#include "localhand.h"
#include "session.h"*/
//#include "player.h"
//#include "board.h"

class Game;
class LocalHand;
class Session;
class QColor;

class mainWindowImpl: public QMainWindow, public Ui::mainWindow {
Q_OBJECT
public:
	mainWindowImpl(QMainWindow *parent = 0, const char *name = 0 );
	void setGame(Game*);
	void setHand(LocalHand*);
	void setSession(Session*);


	int getMaxQuantityPlayers() const { return maxQuantityPlayers; }
	
	int getGameSpeed() const { return gameSpeed; }
	int getPreflopNextPlayerSpeed() const { return preflopNextPlayerSpeed; }
	int getNextPlayerSpeed1() const { return nextPlayerSpeed1; }
	int getNextPlayerSpeed2() const { return nextPlayerSpeed2; }	
	int getNextPlayerSpeed3() const { return nextPlayerSpeed3; }
	
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

	void highlightRoundLabel(QString);

public slots:

	void callNewGameDialog() ;
	void callAboutPokerthDialog();
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

	void handSwitchRounds();

	void startNewHand();
	void nextRoundCleanGui();
	
	void userWidgetsBackgroudColor();
	void timerBlockerFalse();


private: 
	
	Game *actualGame;
	LocalHand *actualHand;
	Session *mySession;


	QTimer *potDistributeTimer;

	QWidget *userWidgetsArray[8];
	QLabel *buttonLabelArray[5];
	QLabel *cashLabelArray[5];
	QLabel *cashTopLabelArray[5];
	QLabel *setLabelArray[5];
	QLabel *actionLabelArray[5];
	QGroupBox *groupBoxArray[5];
	QLabel *boardCardsArray[5];
	QLabel *holeCardsArray[5][2];

	int maxQuantityPlayers;
	
	int distributePotAnimCounter;

	//Speed
	int gameSpeed;
	int dealCardsSpeed;
	int postRiverRunAnimationSpeed;
	int winnerBlinkSpeed; 
	int newRoundSpeed;
	int nextPlayerSpeed1;
	int nextPlayerSpeed2;
	int nextPlayerSpeed3;
	int preflopNextPlayerSpeed;
	int nextOpponentSpeed;	

	bool firstCallNewGame;
	bool newRoundTimerBlock;
	bool debugMode;

	QColor active;
	QColor inactive;
	QColor highlight;
};

#endif
