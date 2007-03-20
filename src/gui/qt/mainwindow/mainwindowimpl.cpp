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
#include "mainwindowimpl.h"

#include "newgamedialogimpl.h"
#include "aboutpokerthimpl.h"
#include "settingsdialogimpl.h"
#include "joinnetworkgamedialogimpl.h"
#include "connecttoserverdialogimpl.h"
#include "createnetworkgamedialogimpl.h"
#include "startnetworkgamedialogimpl.h"
#include "waitforservertostartgamedialogimpl.h"

#include "startsplash.h"
#include "mycardspixmaplabel.h"

#include "playerinterface.h"
#include "boardinterface.h"

#include "game.h"
#include "session.h"

#include "log.h"
#include "configfile.h"

#define FORMATLEFT(X) "<p align='center'>(X)"
#define FORMATRIGHT(X) "(X)</p>"

// muss mit GUI übereinstimmen
const int maxQuantityPlayersConst = 5;
// !!! in game.h ebenfalls bei playerArray[?] setzen !!!
// !!! in hand.h ebenfalls bei roundStartCashArray[?] setzen !!!


using namespace std;

mainWindowImpl::mainWindowImpl(QMainWindow *parent)
     : QMainWindow(parent), actualGame(0), actualHand(0), mySession(0), maxQuantityPlayers(maxQuantityPlayersConst), gameSpeed(0), debugMode(0), breakAfterActualHand(FALSE)
{	
	int i;

	myConfig = new ConfigFile;

// Resourcen abladen 
	QFile preflopValuesFile(":data/resources/data/preflopValues");
	QFile preflopValuesFileDest(QString::fromStdString(myConfig->readConfigString("DataDir")+"preflopValues"));
	if(!preflopValuesFileDest.exists()) { preflopValuesFile.copy(QString::fromStdString(myConfig->readConfigString("DataDir")+"preflopValues")); }

// 	Schriftart laden 
#ifdef _WIN32

	QFont tmpFont;
	tmpFont.setFamily("Arial");
	tmpFont.setPixelSize(12);

// 	if(this->logicalDpiX() > 105) { tmpFont.setFont("Arial",8); }
// 	else { QFont tmpFont("Arial",9); }

#else 
	QFontDatabase::addApplicationFont (":fonts/resources/fonts/n019003l.pfb");
// 	QFont tmpFont("Nimbus Sans L",9);
	QFont tmpFont;
	tmpFont.setFamily("Nimbus Sans L");
	tmpFont.setPixelSize(12);

#endif
	QApplication::setFont(tmpFont);

	setupUi(this);

	//Schriftgrößen festlegen 
#ifdef _WIN32
	tmpFont.setPixelSize(11);
	textBrowser_Log->setFont(tmpFont);
#else
	tmpFont.setPixelSize(10);
	textBrowser_Log->setFont(tmpFont);
#endif
	tmpFont.setPixelSize(18);
	label_Pot->setFont(tmpFont);
	tmpFont.setPixelSize(13);
	label_Sets->setFont(tmpFont);
	tmpFont.setPixelSize(14);
	textLabel_handNumber->setFont(tmpFont);
	textLabel_gameNumber->setFont(tmpFont);
	textLabel_Sets->setFont(tmpFont);
	textLabel_Cash0->setFont(tmpFont);
	tmpFont.setPixelSize(15);
	textLabel_Pot->setFont(tmpFont);
	tmpFont.setPixelSize(21);
	tmpFont.setBold(TRUE);
	textLabel_handLabel->setFont(tmpFont);

	//Logos
	label_logoleft = new QLabel;
	label_logoleft->setPixmap(QPixmap(QString::fromUtf8(":/graphics/resources/graphics/logo-140-100.png")));
	label_logoleft->hide();
 	vboxLayout->addWidget(label_logoleft);

	label_logoright = new QLabel;
	label_logoright->setPixmap(QPixmap(QString::fromUtf8(":/graphics/resources/graphics/logo-140-100.png")));
	label_logoright->setAlignment(Qt::AlignRight);
	label_logoright->hide();
 	vboxLayout3->addWidget(label_logoright);

	//pixmapCardsLabel erstellen und ins Layout einfügen!
	pixmapLabel_cardBoard0 = new MyCardsPixmapLabel(frame_5);
	pixmapLabel_cardBoard0->setObjectName(QString::fromUtf8("pixmapLabel_cardBoard0"));
    	pixmapLabel_cardBoard0->setMinimumSize(QSize(57, 80));
    	pixmapLabel_cardBoard0->setMaximumSize(QSize(57, 80));
	pixmapLabel_cardBoard0->setScaledContents(true);
	gridLayout37->addWidget(pixmapLabel_cardBoard0, 0, 0, 1, 1);

	pixmapLabel_cardBoard1 = new MyCardsPixmapLabel(frame_5);
    	pixmapLabel_cardBoard1->setObjectName(QString::fromUtf8("pixmapLabel_cardBoard1"));
    	pixmapLabel_cardBoard1->setMinimumSize(QSize(57, 80));
    	pixmapLabel_cardBoard1->setMaximumSize(QSize(57, 80));
	pixmapLabel_cardBoard1->setScaledContents(true);
    	gridLayout37->addWidget(pixmapLabel_cardBoard1, 0, 1, 1, 1);

	pixmapLabel_cardBoard2 = new MyCardsPixmapLabel(frame_5);
    	pixmapLabel_cardBoard2->setObjectName(QString::fromUtf8("pixmapLabel_cardBoard2"));
    	pixmapLabel_cardBoard2->setMinimumSize(QSize(57, 80));
    	pixmapLabel_cardBoard2->setMaximumSize(QSize(57, 80));
	pixmapLabel_cardBoard2->setScaledContents(true);
	gridLayout37->addWidget(pixmapLabel_cardBoard2, 0, 2, 1, 1);

	pixmapLabel_cardBoard3 = new MyCardsPixmapLabel(frame_5);
    	pixmapLabel_cardBoard3->setObjectName(QString::fromUtf8("pixmapLabel_cardBoard3"));
    	pixmapLabel_cardBoard3->setMinimumSize(QSize(57, 80));
    	pixmapLabel_cardBoard3->setMaximumSize(QSize(57, 80));
	pixmapLabel_cardBoard3->setScaledContents(true);
    	gridLayout37->addWidget(pixmapLabel_cardBoard3, 0, 3, 1, 1);
	
	pixmapLabel_cardBoard4 = new MyCardsPixmapLabel(frame_5);
    	pixmapLabel_cardBoard4->setObjectName(QString::fromUtf8("pixmapLabel_cardBoard4"));
    	pixmapLabel_cardBoard4->setMinimumSize(QSize(57, 80));
    	pixmapLabel_cardBoard4->setMaximumSize(QSize(57, 80));
	pixmapLabel_cardBoard4->setScaledContents(true);
    	gridLayout37->addWidget(pixmapLabel_cardBoard4, 0, 4, 1, 1);

	pixmapLabel_card0a = new MyCardsPixmapLabel(frame5);
	pixmapLabel_card0a->setObjectName(QString::fromUtf8("pixmapLabel_card0a"));
	pixmapLabel_card0a->setMinimumSize(QSize(80, 112));
	pixmapLabel_card0a->setMaximumSize(QSize(80, 112));
	pixmapLabel_card0a->setScaledContents(true);
	gridLayout4->addWidget(pixmapLabel_card0a, 0, 0, 1, 1);
	
	pixmapLabel_card0b = new MyCardsPixmapLabel(frame5);
	pixmapLabel_card0b->setObjectName(QString::fromUtf8("pixmapLabel_card0b"));
	pixmapLabel_card0b->setMinimumSize(QSize(80, 112));
	pixmapLabel_card0b->setMaximumSize(QSize(80, 112));
	pixmapLabel_card0b->setScaledContents(true);
	gridLayout4->addWidget(pixmapLabel_card0b, 0, 1, 1, 1);

	pixmapLabel_card1a = new MyCardsPixmapLabel(frame8_2_2_2_5);
    	pixmapLabel_card1a->setObjectName(QString::fromUtf8("pixmapLabel_card1a"));
	pixmapLabel_card1a->setMinimumSize(QSize(57, 80));
    	pixmapLabel_card1a->setMaximumSize(QSize(57, 80));
	pixmapLabel_card1a->setScaledContents(true);
	gridLayout13->addWidget(pixmapLabel_card1a, 0, 0, 1, 1);

   	pixmapLabel_card1b = new MyCardsPixmapLabel(frame8_2_2_2_5);
    	pixmapLabel_card1b->setObjectName(QString::fromUtf8("pixmapLabel_card1b"));
	pixmapLabel_card1b->setMinimumSize(QSize(57, 80));
    	pixmapLabel_card1b->setMaximumSize(QSize(57, 80));
	pixmapLabel_card1b->setScaledContents(true);
	gridLayout13->addWidget(pixmapLabel_card1b, 0, 1, 1, 1);

	pixmapLabel_card2a = new MyCardsPixmapLabel(frame8_2_2_2_4);
    	pixmapLabel_card2a->setObjectName(QString::fromUtf8("pixmapLabel_card2a"));
	pixmapLabel_card2a->setMinimumSize(QSize(57, 80));
    	pixmapLabel_card2a->setMaximumSize(QSize(57, 80));
	pixmapLabel_card2a->setScaledContents(true);
   	gridLayout19->addWidget(pixmapLabel_card2a, 0, 0, 1, 1);

	pixmapLabel_card2b = new MyCardsPixmapLabel(frame8_2_2_2_4);
    	pixmapLabel_card2b->setObjectName(QString::fromUtf8("pixmapLabel_card2b"));
	pixmapLabel_card2b->setMinimumSize(QSize(57, 80));
    	pixmapLabel_card2b->setMaximumSize(QSize(57, 80));	
	pixmapLabel_card2b->setScaledContents(true);
	gridLayout19->addWidget(pixmapLabel_card2b, 0, 1, 1, 1);

	pixmapLabel_card3a = new MyCardsPixmapLabel(frame8_2_2_2_6);
    	pixmapLabel_card3a->setObjectName(QString::fromUtf8("pixmapLabel_card3a"));
	pixmapLabel_card3a->setMinimumSize(QSize(57, 80));
    	pixmapLabel_card3a->setMaximumSize(QSize(57, 80));
	pixmapLabel_card3a->setScaledContents(true);
   	gridLayout25->addWidget(pixmapLabel_card3a, 0, 0, 1, 1);

	pixmapLabel_card3b = new MyCardsPixmapLabel(frame8_2_2_2_6);
    	pixmapLabel_card3b->setObjectName(QString::fromUtf8("pixmapLabel_card3b"));
	pixmapLabel_card3b->setMinimumSize(QSize(57, 80));
    	pixmapLabel_card3b->setMaximumSize(QSize(57, 80));
	pixmapLabel_card3b->setScaledContents(true);
	gridLayout25->addWidget(pixmapLabel_card3b, 0, 1, 1, 1);

	pixmapLabel_card4a = new MyCardsPixmapLabel(frame8_2_2_2_7);
    	pixmapLabel_card4a->setObjectName(QString::fromUtf8("pixmapLabel_card4a"));
	pixmapLabel_card4a->setMinimumSize(QSize(57, 80));
    	pixmapLabel_card4a->setMaximumSize(QSize(57, 80));
	pixmapLabel_card4a->setScaledContents(true);
   	gridLayout31->addWidget(pixmapLabel_card4a, 0, 0, 1, 1);

	pixmapLabel_card4b = new MyCardsPixmapLabel(frame8_2_2_2_7);
    	pixmapLabel_card4b->setObjectName(QString::fromUtf8("pixmapLabel_card4b"));
	pixmapLabel_card4b->setMinimumSize(QSize(57, 80));
    	pixmapLabel_card4b->setMaximumSize(QSize(57, 80));
	pixmapLabel_card4b->setScaledContents(true);
	gridLayout31->addWidget(pixmapLabel_card4b, 0, 1, 1, 1);

// 	pixmapLabel_card4b->setPixmap(QPixmap(":/cards/resources/graphics/cards/33.png"), FALSE);
	
	//Flipside festlegen;
	flipside = new QPixmap(":/cards/resources/graphics/cards/flipside.png");
	
	if (myConfig->readConfigInt("FlipsideOwn") && myConfig->readConfigString("FlipsideOwnFile") != "") {
		QPixmap tmpFlipside(QString::fromStdString(myConfig->readConfigString("FlipsideOwnFile")));
		flipside = new QPixmap(tmpFlipside.scaled(QSize(57, 80)));
	}
	else { flipside->load(":/cards/resources/graphics/cards/flipside.png"); }

	//Flipside Animation noch nicht erledigt
	flipHolecardsAllInAlreadyDone = FALSE;


	//Toolboxen verstecken?				
	if (!myConfig->readConfigInt("ShowRightToolBox")) { 
		groupBox_right_tools->hide(); 
		label_logoright->show();
	}
	if (!myConfig->readConfigInt("ShowLeftToolBox")) { 
		groupBox_left_tools->hide(); 
		label_logoleft->show();
	}

	//Intro abspielen?
	if (myConfig->readConfigInt("ShowIntro")) { 
// 		label_logo->hide();
		QTimer::singleShot(100, this, SLOT( paintStartSplash() )); }

		
	pushButton_raise->setDisabled(TRUE);
	pushButton_call->setDisabled(TRUE);
	pushButton_bet->setDisabled(TRUE);
	pushButton_set->setDisabled(TRUE);
	pushButton_fold->setDisabled(TRUE);
	pushButton_allin->setDisabled(TRUE);
	pushButton_check->setDisabled(TRUE);
	
	spinBox_set->setDisabled(TRUE);
	
	// userWidgetsArray init
	userWidgetsArray[0] = pushButton_raise;
	userWidgetsArray[1] = pushButton_call;
	userWidgetsArray[2] = pushButton_bet;
	userWidgetsArray[3] = pushButton_set;
	userWidgetsArray[4] = pushButton_fold;
	userWidgetsArray[5] = pushButton_allin;
	userWidgetsArray[6] = pushButton_check;
	userWidgetsArray[7] = spinBox_set;

	//Hintergrundfarbe der userWidgets anpassen;
	for(i=0; i<=7; i++) {
		QPalette tempPalette = userWidgetsArray[i]->palette();
		QColor tempColor = tempPalette.color(QPalette::Window);
		tempPalette.setColor(QPalette::Button, tempColor);
		userWidgetsArray[i]->setPalette(tempPalette);
	}

	//Timer Objekt erstellen
	dealFlopCards0Timer = new QTimer(this);
	dealFlopCards1Timer = new QTimer(this);
	dealFlopCards2Timer = new QTimer(this);
	dealFlopCards3Timer = new QTimer(this);
	dealFlopCards4Timer = new QTimer(this);
	dealFlopCards5Timer = new QTimer(this);
	dealFlopCards6Timer = new QTimer(this);
	dealTurnCards0Timer = new QTimer(this);
	dealTurnCards1Timer = new QTimer(this);
	dealTurnCards2Timer = new QTimer(this);
	dealRiverCards0Timer = new QTimer(this);
	dealRiverCards1Timer = new QTimer(this);
	dealRiverCards2Timer = new QTimer(this);

	nextPlayerAnimationTimer = new QTimer(this);
	preflopAnimation1Timer = new QTimer(this);
	preflopAnimation2Timer = new QTimer(this);
	flopAnimation1Timer = new QTimer(this);
	flopAnimation2Timer = new QTimer(this);
	turnAnimation1Timer = new QTimer(this);
	turnAnimation2Timer = new QTimer(this);
	riverAnimation1Timer = new QTimer(this);
	riverAnimation2Timer = new QTimer(this);

	postRiverAnimation1Timer = new QTimer(this);
	postRiverRunAnimation1Timer = new QTimer(this);
	postRiverRunAnimation2Timer = new QTimer(this);
	postRiverRunAnimation3Timer = new QTimer(this);
	postRiverRunAnimation5Timer = new QTimer(this);
	potDistributeTimer = new QTimer(this);
	postRiverRunAnimation6Timer = new QTimer(this);

	dealFlopCards0Timer->setSingleShot(TRUE);
	dealFlopCards1Timer->setSingleShot(TRUE);
	dealFlopCards2Timer->setSingleShot(TRUE);
	dealFlopCards3Timer->setSingleShot(TRUE);
	dealFlopCards4Timer->setSingleShot(TRUE);
	dealFlopCards5Timer->setSingleShot(TRUE);
	dealFlopCards6Timer->setSingleShot(TRUE);
	dealTurnCards0Timer->setSingleShot(TRUE);
	dealTurnCards1Timer->setSingleShot(TRUE);
	dealTurnCards2Timer->setSingleShot(TRUE);
	dealRiverCards0Timer->setSingleShot(TRUE);
	dealRiverCards1Timer->setSingleShot(TRUE);
	dealRiverCards2Timer->setSingleShot(TRUE);

	nextPlayerAnimationTimer->setSingleShot(TRUE);
	preflopAnimation1Timer->setSingleShot(TRUE);
	preflopAnimation2Timer->setSingleShot(TRUE);
	flopAnimation1Timer->setSingleShot(TRUE);
	flopAnimation2Timer->setSingleShot(TRUE);
	turnAnimation1Timer->setSingleShot(TRUE);
	turnAnimation2Timer->setSingleShot(TRUE);
	riverAnimation1Timer->setSingleShot(TRUE);
	riverAnimation2Timer->setSingleShot(TRUE);

	postRiverAnimation1Timer->setSingleShot(TRUE);
	postRiverRunAnimation1Timer->setSingleShot(TRUE);
	postRiverRunAnimation2Timer->setSingleShot(TRUE);
	postRiverRunAnimation3Timer->setSingleShot(TRUE);
	postRiverRunAnimation5Timer->setSingleShot(TRUE);
	postRiverRunAnimation6Timer->setSingleShot(TRUE);

	// buttonLabelArray init
	buttonLabelArray[0] = textLabel_Button0;
	buttonLabelArray[1] = textLabel_Button1;
	buttonLabelArray[2] = textLabel_Button2;
	buttonLabelArray[3] = textLabel_Button3;
	buttonLabelArray[4] = textLabel_Button4;

	// cashLabelArray init
	cashLabelArray[0] = textLabel_Cash0;
	cashLabelArray[1] = textLabel_Cash1;
	cashLabelArray[2] = textLabel_Cash2;
	cashLabelArray[3] = textLabel_Cash3;
	cashLabelArray[4] = textLabel_Cash4;

	// cashTopLabelArray init
	cashTopLabelArray[0] = textLabel_TopCash0;
	cashTopLabelArray[1] = textLabel_TopCash1;
	cashTopLabelArray[2] = textLabel_TopCash2;
	cashTopLabelArray[3] = textLabel_TopCash3;
	cashTopLabelArray[4] = textLabel_TopCash4;

	// setLabelArray init
	setLabelArray[0] = textLabel_Set0;
	setLabelArray[1] = textLabel_Set1;
	setLabelArray[2] = textLabel_Set2;
	setLabelArray[3] = textLabel_Set3;
	setLabelArray[4] = textLabel_Set4;

	// statusLabelArray init
	actionLabelArray[0] = textLabel_Status0;
	actionLabelArray[1] = textLabel_Status1;
	actionLabelArray[2] = textLabel_Status2;
	actionLabelArray[3] = textLabel_Status3;
	actionLabelArray[4] = textLabel_Status4;

	// GroupBoxArray init
	groupBoxArray[0] = groupBox0;
	groupBoxArray[1] = groupBox1;
	groupBoxArray[2] = groupBox2;
	groupBoxArray[3] = groupBox3;
	groupBoxArray[4] = groupBox4;

	// boardCardsArray init
	boardCardsArray[0] = pixmapLabel_cardBoard0;
	boardCardsArray[1] = pixmapLabel_cardBoard1;
	boardCardsArray[2] = pixmapLabel_cardBoard2;
	boardCardsArray[3] = pixmapLabel_cardBoard3;
	boardCardsArray[4] = pixmapLabel_cardBoard4;

	// holeCardsArray int
	holeCardsArray[0][0] = pixmapLabel_card0a;
	holeCardsArray[0][1] = pixmapLabel_card0b;
	holeCardsArray[1][0] = pixmapLabel_card1a;
	holeCardsArray[1][1] = pixmapLabel_card1b;
	holeCardsArray[2][0] = pixmapLabel_card2a;
	holeCardsArray[2][1] = pixmapLabel_card2b;
	holeCardsArray[3][0] = pixmapLabel_card3a;
	holeCardsArray[3][1] = pixmapLabel_card3b;
	holeCardsArray[4][0] = pixmapLabel_card4a;
	holeCardsArray[4][1] = pixmapLabel_card4b;

	// Farben initalisieren
	active.setRgb(86,170,86);
	inactive.setRgb(83,141,107);
	highlight.setRgb(151,214,109);

	//ShortCuts 
// 	QShortcut *startNewGameKeys = new QShortcut(QKeySequence(Qt::Key_Control + Qt::Key_N), this);
// 	connect( startNewGameKeys, SIGNAL(activated() ), actionNewGame, SLOT( trigger() ) );

	//Statusbar 
	statusBar()->showMessage(tr("Ctrl+N to start a new Game"));

	//Dialoge 
	myJoinNetworkGameDialog = new joinNetworkGameDialogImpl(this);
	myConnectToServerDialog = new connectToServerDialogImpl(this);
	myStartNetworkGameDialog = new startNetworkGameDialogImpl(this);
	myCreateNetworkGameDialog = new createNetworkGameDialogImpl(this);
	myWaitingForServerGameDialog = new waitForServerToStartGameDialogImpl(this);

	//Connects
	connect(dealFlopCards0Timer, SIGNAL(timeout()), this, SLOT( dealFlopCards1() ));
	connect(dealFlopCards1Timer, SIGNAL(timeout()), this, SLOT( dealFlopCards2() ));
	connect(dealFlopCards2Timer, SIGNAL(timeout()), this, SLOT( dealFlopCards3() ));
	connect(dealFlopCards3Timer, SIGNAL(timeout()), this, SLOT( dealFlopCards4() ));
	connect(dealFlopCards4Timer, SIGNAL(timeout()), this, SLOT( dealFlopCards5() ));
	connect(dealFlopCards5Timer, SIGNAL(timeout()), this, SLOT( dealFlopCards6() ));
	connect(dealFlopCards6Timer, SIGNAL(timeout()), this, SLOT( handSwitchRounds() ));
	connect(dealTurnCards0Timer, SIGNAL(timeout()), this, SLOT( dealTurnCards1() ));
	connect(dealTurnCards1Timer, SIGNAL(timeout()), this, SLOT( dealTurnCards2() ));
	connect(dealTurnCards2Timer, SIGNAL(timeout()), this, SLOT( handSwitchRounds() ));
	connect(dealRiverCards0Timer, SIGNAL(timeout()), this, SLOT( dealRiverCards1() ));
	connect(dealRiverCards1Timer, SIGNAL(timeout()), this, SLOT( dealRiverCards2() ));
	connect(dealRiverCards2Timer, SIGNAL(timeout()), this, SLOT( handSwitchRounds() ));
	
	connect(nextPlayerAnimationTimer, SIGNAL(timeout()), this, SLOT( handSwitchRounds() ));
	connect(preflopAnimation1Timer, SIGNAL(timeout()), this, SLOT( preflopAnimation1Action() ));
	connect(preflopAnimation2Timer, SIGNAL(timeout()), this, SLOT( preflopAnimation2Action() ));
	connect(flopAnimation1Timer, SIGNAL(timeout()), this, SLOT( flopAnimation1Action() ));
	connect(flopAnimation2Timer, SIGNAL(timeout()), this, SLOT( flopAnimation2Action() ));
	connect(turnAnimation1Timer, SIGNAL(timeout()), this, SLOT( turnAnimation1Action() ));
	connect(turnAnimation2Timer, SIGNAL(timeout()), this, SLOT( turnAnimation2Action() ));
	connect(riverAnimation1Timer, SIGNAL(timeout()), this, SLOT( riverAnimation1Action() ));
	connect(riverAnimation2Timer, SIGNAL(timeout()), this, SLOT( riverAnimation2Action() ));
	
	connect(postRiverAnimation1Timer, SIGNAL(timeout()), this, SLOT( postRiverAnimation1Action() ));
	connect(postRiverRunAnimation1Timer, SIGNAL(timeout()), this, SLOT( postRiverRunAnimation2() ));
	connect(postRiverRunAnimation2Timer, SIGNAL(timeout()), this, SLOT( postRiverRunAnimation3() ));
	connect(postRiverRunAnimation3Timer, SIGNAL(timeout()), this, SLOT( postRiverRunAnimation4() ));
	connect(potDistributeTimer, SIGNAL(timeout()), this, SLOT(postRiverRunAnimation5()));
	connect(postRiverRunAnimation5Timer, SIGNAL(timeout()), this, SLOT( postRiverRunAnimation6() ));
	connect(postRiverRunAnimation6Timer, SIGNAL(timeout()), this, SLOT( startNewHand() ));

	connect( actionNewGame, SIGNAL( triggered() ), this, SLOT( callNewGameDialog() ) );
	connect( actionAboutPokerth, SIGNAL( triggered() ), this, SLOT( callAboutPokerthDialog() ) );
	connect( actionSettings, SIGNAL( triggered() ), this, SLOT( callSettingsDialog() ) );
	connect( actionJoin_network_Game, SIGNAL( triggered() ), this, SLOT( callJoinNetworkGameDialog() ) );
	connect( actionCreate_network_Game, SIGNAL( triggered() ), this, SLOT( callCreateNetworkGameDialog() ) );
	connect( actionQuit, SIGNAL( triggered() ), qApp, SLOT( quit() ) );
	connect( actionFullScreen, SIGNAL( triggered() ), this, SLOT( switchFullscreen() ) );

	connect( pushButton_raise, SIGNAL( clicked() ), this, SLOT( myRaise() ) );
	connect( pushButton_call, SIGNAL( clicked() ), this, SLOT( myCall() ) );
	connect( pushButton_bet, SIGNAL( clicked() ), this, SLOT( myBet() ) );
	connect( pushButton_set, SIGNAL( clicked() ), this, SLOT( mySet() ) );
	connect( pushButton_fold, SIGNAL( clicked() ), this, SLOT( myFold() ) );
	connect( pushButton_check, SIGNAL( clicked() ), this, SLOT( myCheck() ) );
	connect( pushButton_allin, SIGNAL( clicked() ), this, SLOT( myAllIn() ) );

	connect ( horizontalSlider_speed, SIGNAL( valueChanged(int)), this, SLOT ( setGameSpeed(int) ) );
	connect ( pushButton_break, SIGNAL( clicked()), this, SLOT ( breakButtonClicked() ) ); // auch wieder starten!!!!

	//Nachrichten Thread-Save
	connect(this, SIGNAL(SignalNetClientConnect(int)), myConnectToServerDialog, SLOT(refresh(int)));
	connect(this, SIGNAL(SignalNetClientGameInfo(int)), myWaitingForServerGameDialog, SLOT(refresh(int)));
	// TODO Fix, errors MUST be global, not within one dialog.
	connect(this, SIGNAL(SignalNetClientError(int, int)), myConnectToServerDialog, SLOT(error(int, int)));

// 	textBrowser_Log->append(QString::number(this->pos().x(),10)+" "+QString::number(this->pos().y(),10));	
// 	textBrowser_Log->append(QString::number(this->x(),10)+" "+QString::number(this->y(),10));

}

mainWindowImpl::~mainWindowImpl() {}

void mainWindowImpl::callNewGameDialog() {

	

	//wenn Dialogfenster gezeigt werden soll

	if(myConfig->readConfigInt("ShowGameSettingsDialogOnNewGame")){

		newGameDialogImpl *v = new newGameDialogImpl(this);
		v->exec();
	
		if (v->result() == QDialog::Accepted ) {

		
			if(actualGame) {
				mySession->deleteGame();
				actualGame = 0;
			}
			
			guiGameSpeed = v->spinBox_gameSpeed->value();
	// 		debugMode = v->checkBox_debugMode->isChecked();
			//Speeds 
			setSpeeds();
	
			//restliche Singleshots killen!!!
			stopTimer();
				
			label_Pot->setText("<p align='center'><span style='font-weight:bold'>Pot Total</span></p>");
			label_Sets->setText("<p align='center'><span style='font-weight:bold'>Sets:</span></p>");
	
			
			//Tools und Board aufhellen und enablen
			QPalette tempPalette = groupBox_board->palette();
			tempPalette.setColor(QPalette::Window, active);
			groupBox_board->setPalette(tempPalette);
			groupBox_right_tools->setDisabled(FALSE);
			groupBox_left_tools->setDisabled(FALSE);	
		
	
			//positioning Slider
			horizontalSlider_speed->setValue(guiGameSpeed);
			
			//Start Game!!!
			mySession->startGame(v->spinBox_quantityPlayers->value(), v->spinBox_startCash->value(), v->spinBox_smallBlind->value());
	
		}
	}
	// sonst mit gespeicherten Werten starten
	else {
		
		if(actualGame) {
			mySession->deleteGame();
			actualGame = 0;
		}
	
		guiGameSpeed = myConfig->readConfigInt("GameSpeed");
		//Speeds 
		setSpeeds();
				
		//restliche Singleshots killen!!!
		stopTimer();
	
		label_Pot->setText("<p align='center'><span style='font-weight:bold'>Pot Total</span></p>");
		label_Sets->setText("<p align='center'><span style='font-weight:bold'>Sets:</span></p>");
	
	
	// 	debugMode = v->checkBox_debugMode->isChecked();
			
		//Tools und Board aufhellen und enablen
		QPalette tempPalette = groupBox_board->palette();
		tempPalette.setColor(QPalette::Window, active);
		groupBox_board->setPalette(tempPalette);
		groupBox_right_tools->setDisabled(FALSE);	
	 	groupBox_left_tools->setDisabled(FALSE);	
	
		
		//positioning Slider
		horizontalSlider_speed->setValue(guiGameSpeed);

		//Start Game!!!
		mySession->startGame(myConfig->readConfigInt("NumberOfPlayers"), myConfig->readConfigInt("StartCash"), myConfig->readConfigInt("SmallBlind"));


	}

}

void mainWindowImpl::callAboutPokerthDialog() {

	aboutPokerthImpl *v = new aboutPokerthImpl(this);
	v->exec();
}

void mainWindowImpl::callCreateNetworkGameDialog() {
	
	myCreateNetworkGameDialog->exec();
// 
	if (myCreateNetworkGameDialog->result() == QDialog::Accepted ) {

		mySession->terminateNetworkServer();
		mySession->startNetworkServer();

		myStartNetworkGameDialog->exec();

		if (myStartNetworkGameDialog->result() == QDialog::Accepted ) {
			mySession->initiateNetworkServerGame();
		}
	}

}

void mainWindowImpl::callJoinNetworkGameDialog() {
	
	myJoinNetworkGameDialog->exec();

	if (myJoinNetworkGameDialog->result() == QDialog::Accepted ) {

		mySession->terminateNetworkClient();

		// Maybe use QUrl::toPunycode.
		mySession->startNetworkClient(
			myJoinNetworkGameDialog->lineEdit_ipAddress->text().toUtf8().constData(),
			myJoinNetworkGameDialog->spinBox_port->value(),
			myJoinNetworkGameDialog->checkBox_ipv6->isChecked(),
			myJoinNetworkGameDialog->lineEdit_password->text().toUtf8().constData());

		//Dialog mit Statusbalken
		myConnectToServerDialog->exec();

		if (myConnectToServerDialog->result() == QDialog::Rejected ) {
			mySession->terminateNetworkClient();
			actionJoin_network_Game->trigger(); // re-trigger
		}
		else {
			myWaitingForServerGameDialog->exec();
		}

	}

}

void mainWindowImpl::callSettingsDialog() {

	settingsDialogImpl *v = new settingsDialogImpl(this);
	v->exec();
		
	if (v->getSettingsCorrect()) {
		
		//Toolbox verstecken?
		if (myConfig->readConfigInt("ShowLeftToolBox")) { 
			label_logoleft->hide();
			groupBox_left_tools->show(); 
		}
		else { 
			groupBox_left_tools->hide(); 
			label_logoleft->show();
		}

		if (myConfig->readConfigInt("ShowRightToolBox")) { 
			label_logoright->hide();
			groupBox_right_tools->show(); 
		}
		else { 
			groupBox_right_tools->hide(); 
			label_logoright->show();
		}


		
		//Falls Spielernamen geändert wurden --> neu zeichnen --> erst beim nächsten Neustart neu ausgelesen
		if (v->getPlayerNickIsChanged() && actualGame) { 

			actualHand->getPlayerArray()[0]->setMyName(v->lineEdit_humanPlayerName->text().toStdString());
			actualHand->getPlayerArray()[1]->setMyName(v->lineEdit_Opponent1Name->text().toStdString());
			actualHand->getPlayerArray()[2]->setMyName(v->lineEdit_Opponent2Name->text().toStdString());
			actualHand->getPlayerArray()[3]->setMyName(v->lineEdit_Opponent3Name->text().toStdString());
			actualHand->getPlayerArray()[4]->setMyName(v->lineEdit_Opponent4Name->text().toStdString());
			v->setPlayerNickIsChanged(FALSE);

			refreshPlayerName();
		}
	
		//Flipside refresh
		if (myConfig->readConfigInt("FlipsideOwn") && myConfig->readConfigString("FlipsideOwnFile") != "") {

			QPixmap tmpFlipside(QString::fromStdString(myConfig->readConfigString("FlipsideOwnFile")));
			flipside = new QPixmap(tmpFlipside.scaled(QSize(57, 80)));
		}
		else { flipside->load(":/cards/resources/graphics/cards/flipside.png"); }

		int i,j;

		for (i=1; i<=4; i++ ) { 
			for ( j=0; j<=1; j++ ) {
				if (holeCardsArray[i][j]->getIsFlipside()) {
					holeCardsArray[i][j]->setPixmap(*flipside, TRUE);
				}
			}
		}
	}
}

void mainWindowImpl::setGame(Game *g) { actualGame = g; }

void mainWindowImpl::setHand(HandInterface* br) { actualHand = br; }

void mainWindowImpl::setSession(Session* s) { mySession = s; }

void mainWindowImpl::setLog(Log* l) { myLog = l; }

//refresh-Funktionen
void mainWindowImpl::refreshSet() {

	
	int i;
 	for (i=0; i<maxQuantityPlayers; i++) { 
		if(actualHand->getPlayerArray()[i]->getMySet() == 0) setLabelArray[i]->setText("");
		else setLabelArray[i]->setText("<p align='center'><b>Set:</b> "+QString::number(actualHand->getPlayerArray()[i]->getMySet(),10)+" $</p>"); 
	}
}

void mainWindowImpl::refreshButton() {

	QPixmap dealerButton(":/graphics/resources/graphics/dealerbutton.png");
	QPixmap onePix(":/graphics/resources/graphics/1px.png");

	int i;
	int k;
	//Aktive Spieler zählen
	int activePlayersCounter = 0;
	for (k=0; k<maxQuantityPlayers; k++) { 
		if (actualHand->getPlayerArray()[k]->getMyActiveStatus() == 1) activePlayersCounter++;
	}

	for (i=0; i<actualHand->getStartQuantityPlayers(); i++) { 
		
		if(activePlayersCounter > 2) {
			if (actualHand->getPlayerArray()[i]->getMyButton()==1) {
				buttonLabelArray[i]->setPixmap(dealerButton); 
			}	
			else {
				buttonLabelArray[i]->setPixmap(onePix);
			}
		}
		else {
			if (actualHand->getPlayerArray()[i]->getMyButton()==3) {
				buttonLabelArray[i]->setPixmap(dealerButton); 
			}	
			else {
				buttonLabelArray[i]->setPixmap(onePix);
			}
		}
		
	}
	
}

void mainWindowImpl::refreshPlayerName() {

	int i;
	for (i=1; i<maxQuantityPlayers; i++) { 	groupBoxArray[i]->setTitle(QString::fromStdString(actualHand->getPlayerArray()[i]->getMyName())); }
}

void mainWindowImpl::refreshAction() {

	QStringList actionArray;
	actionArray << "" << "Fold" << "Check" << "Call" << "Bet" << "Raise" << "All-In";

	int i;
	for (i=0; i<maxQuantityPlayers; i++) { 

		actionLabelArray[i]->setText("<p align='center'>"+actionArray[actualHand->getPlayerArray()[i]->getMyAction()]+"</p>"); 
		
		if (actualHand->getPlayerArray()[i]->getMyAction()==1) { 
			groupBoxArray[i]->setDisabled(TRUE);
			QPixmap onePix(":/graphics/resources/graphics/1px.png");
			if(i != 0) {
				holeCardsArray[i][0]->setPixmap(onePix, FALSE);
				holeCardsArray[i][1]->setPixmap(onePix, FALSE);
			}
			
			QColor c(199,199,199);	
			QPalette labelPalette = cashTopLabelArray[i]->palette();
			labelPalette.setColor(QPalette::WindowText, c);
			cashTopLabelArray[i]->setPalette(labelPalette);
			cashTopLabelArray[i]->setAutoFillBackground(FALSE);
		}
// 		else {
// 			QColor c(0,0,0);	
// 			QPalette labelPalette = cashTopLabelArray[i]->palette();
// 			labelPalette.setColor(QPalette::WindowText, c);
// 			cashTopLabelArray[i]->setPalette(labelPalette);
// 			cashTopLabelArray[i]->setAutoFillBackground(FALSE);
// 		}
	}

}

void mainWindowImpl::refreshCash() {

	int i;
	for (i=0; i<maxQuantityPlayers; i++) { 
		if(actualHand->getPlayerArray()[i]->getMyActiveStatus()) { 

			if (i==0) {
				cashLabelArray[0]->setText(QString::number(actualHand->getPlayerArray()[i]->getMyCash(),10)+" $"); 
				cashTopLabelArray[0]->setText("<p align='center'><span style='font-size:15px; font-weight:bold'>Cash:</span></p>"); 
			}
			else {
				cashLabelArray[i]->setText(QString::number(actualHand->getPlayerArray()[i]->getMyCash(),10)+" $"); 
				cashTopLabelArray[i]->setText("<p align='center'><b>Cash:</b></p>"); 
			}
		} else {
			cashLabelArray[i]->setText(""); 
			cashTopLabelArray[i]->setText("");
		}
	}

}

void mainWindowImpl::refreshGroupbox() {

	int i;
	for (i=0; i<maxQuantityPlayers; i++) { 

		groupBoxArray[i]->setEnabled(actualHand->getPlayerArray()[i]->getMyActiveStatus());
		if (actualHand->getPlayerArray()[i]->getMyAction() == 1) groupBoxArray[i]->setDisabled(TRUE);		

		if(actualHand->getPlayerArray()[i]->getMyTurn()) {
			//Groupbox aufhellen wenn der Spiele dran ist. 
			QPalette tempPalette = groupBoxArray[i]->palette();
			tempPalette.setColor(QPalette::Window, highlight);
			groupBoxArray[i]->setPalette(tempPalette);

		} else {
			//Groupbox auf Hintergrundfarbe setzen wenn der Spiele nicht dran aber aktiv ist. 
			if(actualHand->getPlayerArray()[i]->getMyActiveStatus()) {
				QPalette tempPalette = groupBoxArray[i]->palette();
				tempPalette.setColor(QPalette::Window, active);
				groupBoxArray[i]->setPalette(tempPalette);
			}
			//Groupbox verdunkeln wenn der Spiele inactive ist.  
			else {
				QPalette tempPalette = groupBoxArray[i]->palette();
				tempPalette.setColor(QPalette::Window, inactive);
				groupBoxArray[i]->setPalette(tempPalette);
			}
		}
	}

	userWidgetsBackgroudColor();


}

void mainWindowImpl::highlightRoundLabel(string tempround) { 

	QString round (QString::fromStdString(tempround));

	// für PostRiverRun (alte Runden stehen lassen)
	if(round != "") {

		QPalette tempPalette = frame_handLabel->palette();
		tempPalette.setColor(QPalette::Window, highlight);
		frame_handLabel->setPalette(tempPalette);
		
		textLabel_handLabel->setText(round);
		textLabel_handNumber->setText("Hand: "+QString::number(actualHand->getMyID(),10));
		textLabel_gameNumber->setText("Game: "+QString::number(actualGame->getMyGameID(),10));


	}

}

void mainWindowImpl::refreshAll() {

	refreshSet();
	refreshButton();
	refreshAction();
	refreshCash();
	refreshGroupbox();

}

void mainWindowImpl::refreshChangePlayer() {

	refreshSet();
	refreshButton();
	refreshAction();
	refreshCash();

}

void mainWindowImpl::refreshPot() {

	textLabel_Sets->setText("<p align='left'>"+QString::number(actualHand->getBoard()->getSets(),10)+" $</p>");
	textLabel_Pot->setText("<p align='center'><span style='font-weight:bold'>"+QString::number(actualHand->getBoard()->getPot(),10)+" $</span></p>");

}

void mainWindowImpl::dealHoleCards() {

	QPixmap onePix(":/graphics/resources/graphics/1px.png");

	//TempArrays
	QPixmap tempCardsPixmapArray[2];
	int tempCardsIntArray[2];
	
	// Karten der Gegner austeilen
	int i, j;
	for(i=1; i<maxQuantityPlayers; i++) {
		actualHand->getPlayerArray()[i]->getMyCards(tempCardsIntArray);	
		for(j=0; j<2; j++) {
			if(actualHand->getPlayerArray()[i]->getMyActiveStatus()) { 
				if (debugMode) {
					tempCardsPixmapArray[j].load(":/cards/resources/graphics/cards/"+QString::number(tempCardsIntArray[j], 10)+".png");
					holeCardsArray[i][j]->setPixmap(tempCardsPixmapArray[j],FALSE);
					
				} 
				else holeCardsArray[i][j]->setPixmap(*flipside, TRUE);

			}
			else {
				
				holeCardsArray[i][j]->setPixmap(onePix, FALSE);
// 				holeCardsArray[i][j]->repaint();
			}
	
		}
	}

	// eigenen Karten austeilen
	
	actualHand->getPlayerArray()[0]->getMyCards(tempCardsIntArray);
	for(i=0; i<2; i++) {

		if(actualHand->getPlayerArray()[0]->getMyActiveStatus()) { 
			
			tempCardsPixmapArray[i].load(":/cards/resources/graphics/cards/"+QString::number(tempCardsIntArray[i], 10)+".png");
			holeCardsArray[0][i]->setPixmap(tempCardsPixmapArray[i], FALSE);
						
		}
		else {
				
			holeCardsArray[0][i]->setPixmap(onePix, FALSE);
// 			holeCardsArray[i][j]->repaint();
		}
	}

	//Groupbox repaint() hline-bug?;
	refreshGroupbox();
}

void mainWindowImpl::dealFlopCards0() {	dealFlopCards0Timer->start(preDealCardsSpeed); }

void mainWindowImpl::dealFlopCards1() {

	boardCardsArray[0]->setPixmap(*flipside, TRUE);

	dealFlopCards1Timer->start(dealCardsSpeed);
}

void mainWindowImpl::dealFlopCards2() {

	boardCardsArray[1]->setPixmap(*flipside, TRUE);

	dealFlopCards2Timer->start(dealCardsSpeed);

}

void mainWindowImpl::dealFlopCards3() {

	
	boardCardsArray[2]->setPixmap(*flipside, TRUE);

	dealFlopCards3Timer->start(dealCardsSpeed);

}

void mainWindowImpl::dealFlopCards4() {

	int tempBoardCardsArray[5];
	QPixmap tempCardsPixmap;
	actualHand->getBoard()->getMyCards(tempBoardCardsArray);
	tempCardsPixmap.load(":/cards/resources/graphics/cards/"+QString::number(tempBoardCardsArray[0], 10)+".png");
	QPixmap card(tempCardsPixmap);

	//Config? mit oder ohne Eye-Candy?
	if(myConfig->readConfigInt("ShowFlipCardsAnimation")) { 
		//mit Eye-Candy
		boardCardsArray[0]->startFlipCards(guiGameSpeed, card, flipside);
	}
	else {
		//ohne Eye-Candy
		boardCardsArray[0]->setPixmap(card, FALSE);
	}
	dealFlopCards4Timer->start(dealCardsSpeed);
}

void mainWindowImpl::dealFlopCards5() {

	int tempBoardCardsArray[5];
	QPixmap tempCardsPixmap;
	actualHand->getBoard()->getMyCards(tempBoardCardsArray);
	tempCardsPixmap.load(":/cards/resources/graphics/cards/"+QString::number(tempBoardCardsArray[1], 10)+".png");
	QPixmap card(tempCardsPixmap);
	
	//Config? mit oder ohne Eye-Candy?
	if(myConfig->readConfigInt("ShowFlipCardsAnimation")) { 
		//mit Eye-Candy
		boardCardsArray[1]->startFlipCards(guiGameSpeed, card, flipside);
	}
	else {
		//ohne Eye-Candy
		boardCardsArray[1]->setPixmap(card, FALSE);
	}
	dealFlopCards5Timer->start(dealCardsSpeed);

}

void mainWindowImpl::dealFlopCards6() {

	int tempBoardCardsArray[5];
	QPixmap tempCardsPixmap;
	actualHand->getBoard()->getMyCards(tempBoardCardsArray);
	tempCardsPixmap.load(":/cards/resources/graphics/cards/"+QString::number(tempBoardCardsArray[2], 10)+".png");
	QPixmap card(tempCardsPixmap);
	
	//Config? mit oder ohne Eye-Candy?
	if(myConfig->readConfigInt("ShowFlipCardsAnimation")) { 
		//mit Eye-Candy
		boardCardsArray[2]->startFlipCards(guiGameSpeed, card, flipside);
	}
	else {
		//ohne Eye-Candy
		boardCardsArray[2]->setPixmap(card, FALSE);
	}
	
	myLog->logDealBoardCardsMsg(1, tempBoardCardsArray[0], tempBoardCardsArray[1], tempBoardCardsArray[2]);
	// stable
	// wenn alle All In
	if(actualHand->getAllInCondition()) { dealFlopCards6Timer->start(AllInDealCardsSpeed); }
	// sonst normale Variante
	else { dealFlopCards6Timer->start(postDealCardsSpeed);}

}

void mainWindowImpl::dealTurnCards0() { dealTurnCards0Timer->start(preDealCardsSpeed); }

void mainWindowImpl::dealTurnCards1() {

	boardCardsArray[3]->setPixmap(*flipside, TRUE);
	dealTurnCards1Timer->start(dealCardsSpeed);

}

void mainWindowImpl::dealTurnCards2() {

	int tempBoardCardsArray[5];
	QPixmap tempCardsPixmap;
	actualHand->getBoard()->getMyCards(tempBoardCardsArray);
	tempCardsPixmap.load(":/cards/resources/graphics/cards/"+QString::number(tempBoardCardsArray[3], 10)+".png");
	QPixmap card(tempCardsPixmap);

	//Config? mit oder ohne Eye-Candy?
	if(myConfig->readConfigInt("ShowFlipCardsAnimation")) { 
		//mit Eye-Candy
		boardCardsArray[3]->startFlipCards(guiGameSpeed, card, flipside);
	}
	else {
		//ohne Eye-Candy
		boardCardsArray[3]->setPixmap(card, FALSE);
	}
	
	myLog->logDealBoardCardsMsg(2, tempBoardCardsArray[0], tempBoardCardsArray[1], tempBoardCardsArray[2], tempBoardCardsArray[3]);
	// stable
	// wenn alle All In
	if(actualHand->getAllInCondition()) { dealTurnCards2Timer->start(AllInDealCardsSpeed);
	}
	// sonst normale Variante
	else { dealTurnCards2Timer->start(postDealCardsSpeed); }
}

void mainWindowImpl::dealRiverCards0() { dealRiverCards0Timer->start(preDealCardsSpeed); }

void mainWindowImpl::dealRiverCards1() {

	boardCardsArray[4]->setPixmap(*flipside, TRUE);
// 	QTimer::singleShot(dealCardsSpeed, this, SLOT( dealRiverCards2() ));
	dealRiverCards1Timer->start(dealCardsSpeed);

}

void mainWindowImpl::dealRiverCards2() {

	int tempBoardCardsArray[5];
	QPixmap tempCardsPixmap;
	actualHand->getBoard()->getMyCards(tempBoardCardsArray);
	tempCardsPixmap.load(":/cards/resources/graphics/cards/"+QString::number(tempBoardCardsArray[4], 10)+".png");
	QPixmap card(tempCardsPixmap);

	//Config? mit oder ohne Eye-Candy?
	if(myConfig->readConfigInt("ShowFlipCardsAnimation")) { 
		//mit Eye-Candy
		boardCardsArray[4]->startFlipCards(guiGameSpeed, card, flipside);
	}
	else {
		//ohne Eye-Candy
		boardCardsArray[4]->setPixmap(card, FALSE);
	}

	myLog->logDealBoardCardsMsg(3, tempBoardCardsArray[0], tempBoardCardsArray[1], tempBoardCardsArray[2], tempBoardCardsArray[3], tempBoardCardsArray[4]);
	// stable
	// wenn alle All In
	if(actualHand->getAllInCondition()) { dealRiverCards2Timer->start(AllInDealCardsSpeed);	}
	// sonst normale Variante
	else {
		dealRiverCards2Timer->start(postDealCardsSpeed);
	}


}

void mainWindowImpl::meInAction() {

	
	switch (actualHand->getActualRound()) {

	case 0: {
		if (actualHand->getPlayerArray()[0]->getMyCash()+actualHand->getPlayerArray()[0]->getMySet() > actualHand->getPreflop()->getHighestSet()) { pushButton_raise->setEnabled(TRUE); }

		if ( actualHand->getPlayerArray()[0]->getMySet()== actualHand->getPreflop()->getHighestSet() &&  actualHand->getPlayerArray()[0]->getMyButton() == 3) { pushButton_check->setEnabled(TRUE); }
		else { pushButton_call->setEnabled(TRUE); }
		pushButton_fold->setEnabled(TRUE);
	}
	break;
	case 1: {
	
		pushButton_fold->setEnabled(TRUE);

// 		cout << "highestSet in meInAction " << actualHand->getFlop()->getHighestSet()  << endl;
		if (actualHand->getFlop()->getHighestSet() == 0) { 
			pushButton_check->setEnabled(TRUE); 
			pushButton_bet->setEnabled(TRUE); 
		}
		if (actualHand->getFlop()->getHighestSet() > 0 && actualHand->getFlop()->getHighestSet() > actualHand->getPlayerArray()[0]->getMySet()) {
			pushButton_call->setEnabled(TRUE); 
			if (actualHand->getPlayerArray()[0]->getMyCash()+actualHand->getPlayerArray()[0]->getMySet() > actualHand->getPreflop()->getHighestSet()) { pushButton_raise->setEnabled(TRUE); }
		}
		


	}
	break;
	case 2: {
	
		pushButton_fold->setEnabled(TRUE);

// 		cout << "highestSet in meInAction " << actualHand->getFlop()->getHighestSet()  << endl;
		if (actualHand->getTurn()->getHighestSet() == 0) { 
			pushButton_check->setEnabled(TRUE); 
			pushButton_bet->setEnabled(TRUE); 
		}
		if (actualHand->getTurn()->getHighestSet() > 0 && actualHand->getTurn()->getHighestSet() > actualHand->getPlayerArray()[0]->getMySet()) {
			pushButton_call->setEnabled(TRUE); 
			if (actualHand->getPlayerArray()[0]->getMyCash()+actualHand->getPlayerArray()[0]->getMySet() > actualHand->getPreflop()->getHighestSet()) { pushButton_raise->setEnabled(TRUE); }
		}
		


	}
	break;
	case 3: {
	
		pushButton_fold->setEnabled(TRUE);

// 		cout << "highestSet in meInAction " << actualHand->getFlop()->getHighestSet()  << endl;
		if (actualHand->getRiver()->getHighestSet() == 0) { 
			pushButton_check->setEnabled(TRUE); 
			pushButton_bet->setEnabled(TRUE); 
		}
		if (actualHand->getRiver()->getHighestSet() > 0 && actualHand->getRiver()->getHighestSet() > actualHand->getPlayerArray()[0]->getMySet()) {
			pushButton_call->setEnabled(TRUE); 
			if (actualHand->getPlayerArray()[0]->getMyCash()+actualHand->getPlayerArray()[0]->getMySet() > actualHand->getPreflop()->getHighestSet()) { pushButton_raise->setEnabled(TRUE); }
		}
		


	}
	break;
	default: {}

	}

	//Hintergrundfarbe der userWidgets anpassen;
	userWidgetsBackgroudColor();

}

void mainWindowImpl::disableMyButtons() {

	pushButton_raise->setDisabled(TRUE);
	pushButton_call->setDisabled(TRUE);
	pushButton_bet->setDisabled(TRUE);
	pushButton_set->setDisabled(TRUE);
	pushButton_fold->setDisabled(TRUE);
	pushButton_allin->setDisabled(TRUE);
	pushButton_check->setDisabled(TRUE);
	spinBox_set->setMinimum(0);
	spinBox_set->setValue(0);
	spinBox_set->setDisabled(TRUE);
	
	userWidgetsBackgroudColor();
}

void mainWindowImpl::myFold(){

	actualHand->getPlayerArray()[0]->setMyAction(1);
	actualHand->getPlayerArray()[0]->setMyTurn(0);

	//Action in LogWindow
	myLog->logPlayerActionMsg(actualHand->getPlayerArray()[0]->getMyName(), actualHand->getPlayerArray()[0]->getMyAction(), actualHand->getPlayerArray()[0]->getMySet());

	disableMyButtons();
	
	//Spiel läuft weiter
	nextPlayerAnimation();
}

void mainWindowImpl::myCheck() {

	actualHand->getPlayerArray()[0]->setMyAction(2);
	actualHand->getPlayerArray()[0]->setMyTurn(0);

	//Action in LogWindow
	myLog->logPlayerActionMsg(actualHand->getPlayerArray()[0]->getMyName(), actualHand->getPlayerArray()[0]->getMyAction(), actualHand->getPlayerArray()[0]->getMySet());

	disableMyButtons();

	//Spiel läuft weiter
	nextPlayerAnimation();
}

void mainWindowImpl::myCall(){

	int tempHighestSet = 0;
	switch (actualHand->getActualRound()) {

		case 0: {tempHighestSet = actualHand->getPreflop()->getHighestSet();}
		break;
		case 1: {tempHighestSet = actualHand->getFlop()->getHighestSet();}
		break;
		case 2: {tempHighestSet = actualHand->getTurn()->getHighestSet();}
		break;
		case 3: {tempHighestSet = actualHand->getRiver()->getHighestSet();}
		break;
		default: {}	
	}

	if (actualHand->getPlayerArray()[0]->getMyCash()+actualHand->getPlayerArray()[0]->getMySet() <= tempHighestSet) {

		actualHand->getPlayerArray()[0]->setMyAction(6);
		actualHand->getPlayerArray()[0]->setMySet(actualHand->getPlayerArray()[0]->getMyCash());
		actualHand->getPlayerArray()[0]->setMyCash(0);
	}
	else {	
		actualHand->getPlayerArray()[0]->setMySet(tempHighestSet - actualHand->getPlayerArray()[0]->getMySet());
		actualHand->getPlayerArray()[0]->setMyAction(3);
	}
	actualHand->getPlayerArray()[0]->setMyTurn(0);

	actualHand->getBoard()->collectSets();
	refreshPot();

	//Action in LogWindow
	myLog->logPlayerActionMsg(actualHand->getPlayerArray()[0]->getMyName(), actualHand->getPlayerArray()[0]->getMyAction(), actualHand->getPlayerArray()[0]->getMySet());

	disableMyButtons();

	//Spiel läuft weiter
	nextPlayerAnimation();
}

void mainWindowImpl::myBet(){ 

	pushButton_bet->setDisabled(TRUE);

	pushButton_set->setEnabled(TRUE);	
	pushButton_allin->setEnabled(TRUE);
	spinBox_set->setEnabled(TRUE);

	//Hintergrundfarbe anpassen;
	userWidgetsBackgroudColor();

	
	if (actualHand->getActualRound() <= 2 ) { spinBox_set->setMinimum(actualHand->getSmallBlind()*2); }
	else { spinBox_set->setMinimum(actualHand->getSmallBlind()*4); }
	
	spinBox_set->setMaximum(actualHand->getPlayerArray()[0]->getMyCash());
	spinBox_set->setValue(spinBox_set->minimum());
	spinBox_set->setFocus();
	spinBox_set->selectAll();

	actualHand->getPlayerArray()[0]->setMyAction(4);

}

void mainWindowImpl::myRaise(){ 

	pushButton_raise->setDisabled(TRUE);

	pushButton_set->setEnabled(TRUE);	
	pushButton_allin->setEnabled(TRUE);
	spinBox_set->setEnabled(TRUE);

	//Hintergrundfarbe der userWidgets anpassen;
	userWidgetsBackgroudColor();


	int tempHighestSet = 0;
	switch (actualHand->getActualRound()) {

		case 0: {tempHighestSet = actualHand->getPreflop()->getHighestSet();}
		break;
		case 1: {tempHighestSet = actualHand->getFlop()->getHighestSet();}
		break;
		case 2: {tempHighestSet = actualHand->getTurn()->getHighestSet();}
		break;
		case 3: {tempHighestSet = actualHand->getRiver()->getHighestSet();}
		break;
		default: {}	
	}

	spinBox_set->setMinimum(tempHighestSet*2 - actualHand->getPlayerArray()[0]->getMySet());
	spinBox_set->setMaximum(actualHand->getPlayerArray()[0]->getMyCash());
	spinBox_set->setValue(spinBox_set->minimum());
	spinBox_set->setFocus();
	spinBox_set->selectAll();

	actualHand->getPlayerArray()[0]->setMyAction(5);

}

void mainWindowImpl::mySet(){
	
	int tempCash = actualHand->getPlayerArray()[0]->getMyCash();

// 	cout << "Set-Value " << spinBox_set->value() << endl; 
	actualHand->getPlayerArray()[0]->setMySet(spinBox_set->value());
// 	cout << "MySET " << actualHand->getPlayerArray()[0]->getMySet() << endl;
	if (spinBox_set->value() >= tempCash ) {

		actualHand->getPlayerArray()[0]->setMyAction(6);
		actualHand->getPlayerArray()[0]->setMySet(actualHand->getPlayerArray()[0]->getMyCash());
		actualHand->getPlayerArray()[0]->setMyCash(0);
		
	}

	switch (actualHand->getActualRound()) {

		case 0: {actualHand->getPreflop()->setHighestSet(actualHand->getPlayerArray()[0]->getMySet());}
		break;
		case 1: {actualHand->getFlop()->setHighestSet( actualHand->getPlayerArray()[0]->getMySet());}
		break;
		case 2: {actualHand->getTurn()->setHighestSet( actualHand->getPlayerArray()[0]->getMySet());}
		break;
		case 3: {actualHand->getRiver()->setHighestSet( actualHand->getPlayerArray()[0]->getMySet());}
		break;
		default: {}	
	}

	
	actualHand->getPlayerArray()[0]->setMyTurn(0);

	actualHand->getBoard()->collectSets();
	refreshPot();

	//Action in LogWindow
	myLog->logPlayerActionMsg(actualHand->getPlayerArray()[0]->getMyName(), actualHand->getPlayerArray()[0]->getMyAction(), actualHand->getPlayerArray()[0]->getMySet());

	disableMyButtons();

	//Spiel läuft weiter
	nextPlayerAnimation();
}

void mainWindowImpl::myAllIn(){

	actualHand->getPlayerArray()[0]->setMyAction(6);
	actualHand->getPlayerArray()[0]->setMySet(actualHand->getPlayerArray()[0]->getMyCash());
	actualHand->getPlayerArray()[0]->setMyCash(0);
	
	switch (actualHand->getActualRound()) {

		case 0: {if(actualHand->getPlayerArray()[0]->getMySet() > actualHand->getPreflop()->getHighestSet()) actualHand->getPreflop()->setHighestSet(actualHand->getPlayerArray()[0]->getMySet());}
		break;
		case 1: {if(actualHand->getPlayerArray()[0]->getMySet() > actualHand->getFlop()->getHighestSet()) actualHand->getFlop()->setHighestSet(actualHand->getPlayerArray()[0]->getMySet());}
		break;
		case 2: {if(actualHand->getPlayerArray()[0]->getMySet() > actualHand->getTurn()->getHighestSet()) actualHand->getTurn()->setHighestSet(actualHand->getPlayerArray()[0]->getMySet());}
		break;
		case 3: {if(actualHand->getPlayerArray()[0]->getMySet() > actualHand->getRiver()->getHighestSet()) actualHand->getRiver()->setHighestSet(actualHand->getPlayerArray()[0]->getMySet());}
		break;
		default: {}	
	}

	actualHand->getPlayerArray()[0]->setMyTurn(0);

	actualHand->getBoard()->collectSets();
	refreshPot();

	//Action in LogWindow
	myLog->logPlayerActionMsg(actualHand->getPlayerArray()[0]->getMyName(), actualHand->getPlayerArray()[0]->getMyAction(), actualHand->getPlayerArray()[0]->getMySet());
	
	disableMyButtons();

	//Spiel läuft weiter
	nextPlayerAnimation();
}

void mainWindowImpl::nextPlayerAnimation() {

	refreshChangePlayer();
	nextPlayerAnimationTimer->start(nextPlayerSpeed1);
}

void mainWindowImpl::preflopAnimation1() { preflopAnimation1Timer->start(nextPlayerSpeed2); }
void mainWindowImpl::preflopAnimation1Action() { actualHand->getPreflop()->preflopRun(); }

void mainWindowImpl::preflopAnimation2() { preflopAnimation2Timer->start(preflopNextPlayerSpeed); }
void mainWindowImpl::preflopAnimation2Action() { actualHand->getPreflop()->nextPlayer2(); }


void mainWindowImpl::flopAnimation1() { flopAnimation1Timer->start(nextPlayerSpeed2); }
void mainWindowImpl::flopAnimation1Action() { actualHand->getFlop()->flopRun(); }

void mainWindowImpl::flopAnimation2() { flopAnimation2Timer->start(nextPlayerSpeed3); }
void mainWindowImpl::flopAnimation2Action() { actualHand->getFlop()->nextPlayer2(); }


void mainWindowImpl::turnAnimation1() { turnAnimation1Timer->start(nextPlayerSpeed2); }
void mainWindowImpl::turnAnimation1Action() { actualHand->getTurn()->turnRun(); }

void mainWindowImpl::turnAnimation2() { turnAnimation2Timer->start(nextPlayerSpeed3); }
void mainWindowImpl::turnAnimation2Action() { actualHand->getTurn()->nextPlayer2(); }


void mainWindowImpl::riverAnimation1() { riverAnimation1Timer->start(nextPlayerSpeed2); }
void mainWindowImpl::riverAnimation1Action() { actualHand->getRiver()->riverRun(); }

void mainWindowImpl::riverAnimation2() { riverAnimation2Timer->start(nextPlayerSpeed3); }
void mainWindowImpl::riverAnimation2Action() { actualHand->getRiver()->nextPlayer2(); }


void mainWindowImpl::postRiverAnimation1() { postRiverAnimation1Timer->start(nextPlayerSpeed2); }
void mainWindowImpl::postRiverAnimation1Action() { actualHand->getRiver()->postRiverRun(); }


void mainWindowImpl::postRiverRunAnimation1() {

	//RoundsLabel mit "River" dunkel machen
	QPalette tempPalette = frame_handLabel->palette();
	tempPalette.setColor(QPalette::Window, active);
	frame_handLabel->setPalette(tempPalette);

	//PotLabel aufhellen!
	tempPalette = frame_Pot->palette();
	tempPalette.setColor(QPalette::Window, highlight);
	frame_Pot->setPalette(tempPalette);



	postRiverRunAnimation1Timer->start(postRiverRunAnimationSpeed);

}

void mainWindowImpl::postRiverRunAnimation2() {

	int i;

	//Aktive Spieler zählen --> wenn nur noch einer nicht-folded dann keine Karten umdrehen
	int activePlayersCounter = 0;
	for (i=0; i<maxQuantityPlayers; i++) { 
		if (actualHand->getPlayerArray()[i]->getMyAction() != 1 && actualHand->getPlayerArray()[i]->getMyActiveStatus() == 1) activePlayersCounter++;
	}

	if(activePlayersCounter!=1) { 
		 
		if(!flipHolecardsAllInAlreadyDone) {

			//Config? mit oder ohne Eye-Candy?
			if(myConfig->readConfigInt("ShowFlipCardsAnimation")) { 
				// mit Eye-Candy
		
				//TempArrays
				int tempCardsIntArray[2];
		
				int i, j;
				
				for(i=0; i<maxQuantityPlayers; i++) {
					actualHand->getPlayerArray()[i]->getMyCards(tempCardsIntArray);	
					if(actualHand->getPlayerArray()[i]->getMyActiveStatus() && actualHand->getPlayerArray()[i]->getMyAction() != 1) { 
						if(i) {
							for(j=0; j<2; j++) {
												
								holeCardsArray[i][j]->startFlipCards(guiGameSpeed, QPixmap(":/cards/resources/graphics/cards/"+QString::number(tempCardsIntArray[j], 10)+".png"), flipside);
							}	
						}
						//Karten umdrehen Loggen 
						myLog->logFlipHoleCardsMsg(actualHand->getPlayerArray()[i]->getMyName(), tempCardsIntArray[0], tempCardsIntArray[1], actualHand->getPlayerArray()[i]->getMyCardsValueInt());
			
						actualHand->getPlayerArray()[i]->setMyCardsFlip(1);
					}
				}	
			}
			else {
				//Ohne Eye-Candy		
			
				//Karten der aktiven Spieler umdrehen
				QPixmap onePix(":/graphics/cards/1px.png");
			
				//TempArrays
				QPixmap tempCardsPixmapArray[2];
				int tempCardsIntArray[2];
			
				int i, j;
				for(i=0; i<maxQuantityPlayers; i++) {
					actualHand->getPlayerArray()[i]->getMyCards(tempCardsIntArray);	
					if(actualHand->getPlayerArray()[i]->getMyActiveStatus() && actualHand->getPlayerArray()[i]->getMyAction() != 1) { 
						if(i) {
							for(j=0; j<2; j++) {		
								tempCardsPixmapArray[j].load(":/cards/resources/graphics/cards/"+QString::number(tempCardsIntArray[j], 10)+".png");
								holeCardsArray[i][j]->setPixmap(tempCardsPixmapArray[j], FALSE);
								
							}	
						}
						//Karten umdrehen Loggen 
						myLog->logFlipHoleCardsMsg(actualHand->getPlayerArray()[i]->getMyName(), tempCardsIntArray[0], tempCardsIntArray[1], actualHand->getPlayerArray()[i]->getMyCardsValueInt() );

						actualHand->getPlayerArray()[i]->setMyCardsFlip(1);
					}
				}
			}

		//Wenn einmal umgedreht dann fertig!!	
		flipHolecardsAllInAlreadyDone = TRUE;
		}
		else {
			int tempCardsIntArray[2];
			int i;
			for(i=0; i<maxQuantityPlayers; i++) {
				actualHand->getPlayerArray()[i]->getMyCards(tempCardsIntArray);	
				if(actualHand->getPlayerArray()[i]->getMyActiveStatus() && actualHand->getPlayerArray()[i]->getMyAction() != 1) { 
				
					//Kartenwerte Loggen 
					myLog->logFlipHoleCardsMsg(actualHand->getPlayerArray()[i]->getMyName(), tempCardsIntArray[0], tempCardsIntArray[1], actualHand->getPlayerArray()[i]->getMyCardsValueInt(), "has");
			
	
				}
			}	
		}

		postRiverRunAnimation2Timer->start(postRiverRunAnimationSpeed);
	}
	else { postRiverRunAnimation3(); }
	
}

void mainWindowImpl::postRiverRunAnimation3() {

	int i;

// 	cout << "Neue Runde" << endl;

	//Alle Winner erhellen und "Winner" schreiben
	for(i=0; i<maxQuantityPlayers; i++) {
		if(actualHand->getPlayerArray()[i]->getMyActiveStatus() && actualHand->getPlayerArray()[i]->getMyAction() != 1 && actualHand->getPlayerArray()[i]->getMyCardsValueInt() == actualHand->getRiver()->getHighestCardsValue() ) { 

			QPalette tempPalette = groupBoxArray[i]->palette();
			tempPalette.setColor(QPalette::Window, highlight);
			groupBoxArray[i]->setPalette(tempPalette);
			actionLabelArray[i]->setText("<p align='center'><b>- Winner -</b></p>");

			//nicht gewonnene Karten ausblenden
			if ( actualHand->getActivePlayersCounter() != 1 && myConfig->readConfigInt("ShowFadeOutCardsAnimation")) {

				int j;
	
				//index 0 testen --> Karte darf nicht im MyBestHand Position Array drin sein, es darf nicht nur ein Spieler Aktiv sein, die Config fordert die Animation
				bool index0 = TRUE;
				for(j=0; j<=4; j++) {			
	// 				cout <<  (actualHand->getPlayerArray()[i]->getMyBestHandPosition())[j] << endl;
					if ((actualHand->getPlayerArray()[i]->getMyBestHandPosition())[j] == 0 ) { index0 = FALSE; }
				}
				if (index0) { holeCardsArray[i][0]->startFadeOut(guiGameSpeed); /*cout << "Fade Out index0" << endl;*/}
				//index 1 testen
				bool index1 = TRUE;
				for(j=0; j<=4; j++) {
					if ((actualHand->getPlayerArray()[i]->getMyBestHandPosition())[j] == 1 ) { index1 = FALSE; }
				}
				if (index1) { holeCardsArray[i][1]->startFadeOut(guiGameSpeed); /*cout << "Fade Out index1" << endl;*/}
				//index 2 testen
				bool index2 = TRUE;
				for(j=0; j<=4; j++) {
					if ((actualHand->getPlayerArray()[i]->getMyBestHandPosition())[j] == 2 ) { index2 = FALSE; }
				}
				if (index2) { boardCardsArray[0]->startFadeOut(guiGameSpeed); /*cout << "Fade Out index2" << endl;*/}
				//index 3 testen
				bool index3 = TRUE;
				for(j=0; j<=4; j++) {
					if ((actualHand->getPlayerArray()[i]->getMyBestHandPosition())[j] == 3 ) { index3 = FALSE; }
				}
				if (index3) { boardCardsArray[1]->startFadeOut(guiGameSpeed); /*cout << "Fade Out index3" << endl;*/}
				//index 4 testen
				bool index4 = TRUE;
				for(j=0; j<=4; j++) {
					if ((actualHand->getPlayerArray()[i]->getMyBestHandPosition())[j] == 4 ) { index4 = FALSE; }
				}
				if (index4) { boardCardsArray[2]->startFadeOut(guiGameSpeed); /*cout << "Fade Out index4" << endl;*/}
				//index 5 testen
				bool index5 = TRUE;
				for(j=0; j<=4; j++) {
					if ((actualHand->getPlayerArray()[i]->getMyBestHandPosition())[j] == 5 ) { index5 = FALSE; }
				}
				if (index5) { boardCardsArray[3]->startFadeOut(guiGameSpeed); /*cout << "Fade Out index5" << endl;*/}
				//index 6 testen
				bool index6 = TRUE;
				for(j=0; j<=4; j++) {
					if ((actualHand->getPlayerArray()[i]->getMyBestHandPosition())[j] == 6 ) { index6 = FALSE; }
				}
				if (index6) { boardCardsArray[4]->startFadeOut(guiGameSpeed); /*cout << "Fade Out index6" << endl;*/}
			}	

			//Pot-Verteilung Loggen 
			//Pro Spieler den Cash aus dem Player und dem Label auslesen. Player_cash - Label_cash = Gewinnsumme
			bool toIntBool = TRUE;
			int pot =  actualHand->getPlayerArray()[i]->getMyCash() - cashLabelArray[i]->text().remove(" $").toInt(&toIntBool,10) ;
			//Wenn River dann auch das Blatt loggen!
// 			if (textLabel_handLabel->text() == "River") {
			myLog->logPlayerWinsMsg(i, pot);	
// 			}
// 			else {
// 				myLog->logPlayerWinsMsg(i, pot);
// 			}
// 			
			
		}
		else {
			
			if( actualHand->getActivePlayersCounter() != 1 && actualHand->getPlayerArray()[i]->getMyAction() != 1 &&  actualHand->getPlayerArray()[i]->getMyActiveStatus() && myConfig->readConfigInt("ShowFadeOutCardsAnimation") ) {
    	
			//aufgedeckte Gegner auch ausblenden
				holeCardsArray[i][0]->startFadeOut(guiGameSpeed);
				holeCardsArray[i][1]->startFadeOut(guiGameSpeed);
			}
		
		

		}
	}
	
	
	//Hintergrundfarbe der userWidgets anpassen;
	userWidgetsBackgroudColor();

	postRiverRunAnimation3Timer->start(postRiverRunAnimationSpeed/2);

}

void mainWindowImpl::postRiverRunAnimation4() {

	distributePotAnimCounter=0;
	potDistributeTimer->start(winnerBlinkSpeed);
}

void mainWindowImpl::postRiverRunAnimation5() {

	int i;

	//CashTopLabel und PotLabel blinken lassen
	if (distributePotAnimCounter<10) {
		
		if (distributePotAnimCounter==0 || distributePotAnimCounter==2 || distributePotAnimCounter==4 || distributePotAnimCounter==6 || distributePotAnimCounter==8) { 

			QPalette labelPalette = label_Pot->palette();
			labelPalette.setColor(QPalette::WindowText, highlight);
			label_Pot->setPalette(labelPalette);
	
			for(i=0; i<maxQuantityPlayers; i++) {
				if(actualHand->getPlayerArray()[i]->getMyActiveStatus() && actualHand->getPlayerArray()[i]->getMyAction() != 1 && actualHand->getPlayerArray()[i]->getMyCardsValueInt() == actualHand->getRiver()->getHighestCardsValue() ) { 

					cashTopLabelArray[i]->setAutoFillBackground(TRUE);
					QPalette labelPalette = cashTopLabelArray[i]->palette();
					labelPalette.setColor(QPalette::WindowText, highlight);
					cashTopLabelArray[i]->setPalette(labelPalette);
					
				}
			}
		}
		else { 

			QColor c(0,0,0);
			QPalette labelPalette = label_Pot->palette();
			labelPalette.setColor(QPalette::WindowText, c);
			label_Pot->setPalette(labelPalette);
			
			for(i=0; i<maxQuantityPlayers; i++) {
				if(actualHand->getPlayerArray()[i]->getMyActiveStatus() && actualHand->getPlayerArray()[i]->getMyAction() != 1 && actualHand->getPlayerArray()[i]->getMyCardsValueInt() == actualHand->getRiver()->getHighestCardsValue() ) { 

					QPalette labelPalette = cashTopLabelArray[i]->palette();
					labelPalette.setColor(QPalette::WindowText, c);
					cashTopLabelArray[i]->setPalette(labelPalette);
					cashTopLabelArray[i]->setAutoFillBackground(FALSE);
				}
			}
		}
		
		distributePotAnimCounter++;
	}
	else {
		potDistributeTimer->stop();
		postRiverRunAnimation5Timer->start(gameSpeed);	
	}
	
}

void mainWindowImpl::postRiverRunAnimation6() {

	
	int i;
	refreshCash();
	refreshPot();

	// wenn nur noch ein Spieler aktive "neues Spiel"-Dialog anzeigen
	int playersPositiveCashCounter = 0;
	for (i=0; i<actualGame->getStartQuantityPlayers(); i++) { 

		if (actualHand->getPlayerArray()[i]->getMyCash() > 0) playersPositiveCashCounter++;
	}
	if (playersPositiveCashCounter==1) {
		
		callNewGameDialog();	
		//Bei Cancel nichts machen!!!
		return;
	} 
	
	postRiverRunAnimation6Timer->start(newRoundSpeed);
}

void mainWindowImpl::flipHolecardsAllIn() {


	int i;

	if(!flipHolecardsAllInAlreadyDone) {
		//Aktive Spieler zählen --> wenn nur noch einer nicht-folded dann keine Karten umdrehen
		int activePlayersCounter = 0;
		for (i=0; i<maxQuantityPlayers; i++) { 
			if (actualHand->getPlayerArray()[i]->getMyAction() != 1 && actualHand->getPlayerArray()[i]->getMyActiveStatus() == 1) activePlayersCounter++;
		}
		
		if(activePlayersCounter!=1) { 
			
			//Config? mit oder ohne Eye-Candy?
			if(myConfig->readConfigInt("ShowFlipCardsAnimation")) { 
				// mit Eye-Candy
	
				//TempArrays
				int tempCardsIntArray[2];
	
				int i, j;
	
				for(i=0; i<maxQuantityPlayers; i++) {
					actualHand->getPlayerArray()[i]->getMyCards(tempCardsIntArray);	
					if(actualHand->getPlayerArray()[i]->getMyActiveStatus() && actualHand->getPlayerArray()[i]->getMyAction() != 1) { 
						if(i) {
							for(j=0; j<2; j++) {
								holeCardsArray[i][j]->startFlipCards(guiGameSpeed, QPixmap(":/cards/resources/graphics/cards/"+QString::number(tempCardsIntArray[j], 10)+".png"), flipside);
							}
						}
						//Karten umdrehen Loggen 
						myLog->logFlipHoleCardsMsg(actualHand->getPlayerArray()[i]->getMyName(), tempCardsIntArray[0], tempCardsIntArray[1]);

						actualHand->getPlayerArray()[i]->setMyCardsFlip(1);
						
					}
				}
			}
			else {
				//Ohne Eye-Candy		
		
				//Karten der aktiven Spieler umdrehen
				QPixmap onePix(":/graphics/cards/1px.png");
				
				//TempArrays
				QPixmap tempCardsPixmapArray[2];
				int temp2CardsIntArray[2];
				
				int i, j;
				for(i=0; i<maxQuantityPlayers; i++) {
					actualHand->getPlayerArray()[i]->getMyCards(temp2CardsIntArray);	
					if(actualHand->getPlayerArray()[i]->getMyActiveStatus() && actualHand->getPlayerArray()[i]->getMyAction() != 1) { 
						if(i) {
							for(j=0; j<2; j++) {
								
								tempCardsPixmapArray[j].load(":/cards/resources/graphics/cards/"+QString::number(temp2CardsIntArray[j], 10)+".png");
								holeCardsArray[i][j]->setPixmap(tempCardsPixmapArray[j], FALSE);
							}	
						}
						//Karten umdrehen Loggen 
						myLog->logFlipHoleCardsMsg(actualHand->getPlayerArray()[i]->getMyName(), temp2CardsIntArray[0], temp2CardsIntArray[1] );
				
						actualHand->getPlayerArray()[i]->setMyCardsFlip(1);
					}
				}
			}
		}
	}	
	
	//Wenn einmal umgedreht dann fertig!!	
	flipHolecardsAllInAlreadyDone = TRUE;
}

void mainWindowImpl::showMyCards() {

	//TempArrays
	int tempCardsIntArray[2];
	
	actualHand->getPlayerArray()[0]->getMyCards(tempCardsIntArray);	
	if( actualHand->getPlayerArray()[0]->getMyCardsFlip() == 0 &&  actualHand->getActualRound() == 4 && actualHand->getPlayerArray()[0]->getMyActiveStatus() && actualHand->getPlayerArray()[0]->getMyAction() != 1) { 
		//Karten umdrehen Loggen 
		myLog->logFlipHoleCardsMsg(actualHand->getPlayerArray()[0]->getMyName(), tempCardsIntArray[0], tempCardsIntArray[1], actualHand->getPlayerArray()[0]->getMyCardsValueInt() );

		actualHand->getPlayerArray()[0]->setMyCardsFlip(1);
	}
}


void mainWindowImpl::startNewHand() {

if( !breakAfterActualHand){ actualGame->startHand();
}
else { 
	pushButton_break->setDisabled(FALSE);
	pushButton_break->setText("Start");
	breakAfterActualHand=FALSE;
}


}

void mainWindowImpl::handSwitchRounds() { actualHand->switchRounds(); }

void mainWindowImpl::nextRoundCleanGui() {

	int i,j;

	// GUI bereinigen - Bilder löschen, Animationen unterbrechen
	QPixmap onePix(":/graphics/cards/1px.png");
	for (i=0; i<5; i++ ) { 
		boardCardsArray[i]->setPixmap(onePix, FALSE); 
		boardCardsArray[i]->setFadeOutAction(FALSE); 
		
	}
	for (i=1; i<=4; i++ ) { 
		for ( j=0; j<=1; j++ ) { holeCardsArray[i][j]->setFadeOutAction(FALSE);}
	}
	

		
	QColor c(0,0,0);
	for(i=0; i<maxQuantityPlayers; i++) {
		QPalette labelPalette = cashTopLabelArray[i]->palette();
		labelPalette.setColor(QPalette::WindowText, c);
		cashTopLabelArray[i]->setPalette(labelPalette);
	}

	QPalette labelPalette = label_Pot->palette();
	labelPalette.setColor(QPalette::WindowText, c);
	label_Pot->setPalette(labelPalette);

	disableMyButtons();
	
	QPalette tempPalette = frame_Pot->palette();
	tempPalette.setColor(QPalette::Window, active);
	frame_Pot->setPalette(tempPalette);

	textLabel_handLabel->setText("");
	
	tempPalette = frame_handLabel->palette();
	tempPalette.setColor(QPalette::Window, active);
	frame_handLabel->setPalette(tempPalette);

	refreshAll();

	flipHolecardsAllInAlreadyDone = FALSE;

	//Wenn Pause zwischen den Hands in der Konfiguration steht den Stop Button drücken!
	if (myConfig->readConfigInt("PauseBetweenHands")) { pushButton_break->click(); }
	else { 
		//FIX STRG+N Bug
		pushButton_break->setEnabled(TRUE); 
		breakAfterActualHand=FALSE;
	}

	//Clear Statusbarmessage
	statusBar()->clearMessage();
}

void mainWindowImpl::stopTimer() {

	dealFlopCards0Timer->stop();
	dealFlopCards1Timer->stop();
	dealFlopCards2Timer->stop();
	dealFlopCards3Timer->stop();
	dealFlopCards4Timer->stop();
	dealFlopCards5Timer->stop();
	dealFlopCards6Timer->stop();
	dealTurnCards0Timer->stop();
	dealTurnCards1Timer->stop();
	dealTurnCards2Timer->stop();
	dealRiverCards0Timer->stop();
	dealRiverCards1Timer->stop();
	dealRiverCards2Timer->stop();
			
	nextPlayerAnimationTimer->stop();
	preflopAnimation1Timer->stop();
	preflopAnimation2Timer->stop();
	flopAnimation1Timer->stop();
	flopAnimation2Timer->stop();
	turnAnimation1Timer->stop();
	turnAnimation2Timer->stop();
	riverAnimation1Timer->stop();
	riverAnimation2Timer->stop();
			
	postRiverAnimation1Timer->stop();
	postRiverRunAnimation1Timer->stop();
	postRiverRunAnimation2Timer->stop();
	postRiverRunAnimation3Timer->stop();
	postRiverRunAnimation5Timer->stop();
	postRiverRunAnimation6Timer->stop();	
	potDistributeTimer->stop();

}

void mainWindowImpl::userWidgetsBackgroudColor() {
	
	
	int i;
	for(i=0; i<=7; i++) {

		QPalette tempPalette = userWidgetsArray[i]->palette();
	
		if (actualHand->getPlayerArray()[0]->getMyTurn() && userWidgetsArray[i]->isEnabled()) {
			//Farbe der Buttons wenn der Spieler dran ist.
			QColor tempColor(203,237,180);
			tempPalette.setColor(QPalette::Button, tempColor);
		}
		else {
			QColor tempColor = tempPalette.color(QPalette::Window);
			tempPalette.setColor(QPalette::Button, tempColor);

		}
		userWidgetsArray[i]->setPalette(tempPalette);
	}
}

void mainWindowImpl::setSpeeds() {

	gameSpeed = (11-guiGameSpeed)*10;
	dealCardsSpeed = (gameSpeed/2)*10; //milliseconds
	preDealCardsSpeed = dealCardsSpeed*2; //Zeit for Karten aufdecken auf dem Board (Flop, Turn, River)
	postDealCardsSpeed = dealCardsSpeed*2; //Zeit nach Karten aufdecken auf dem Board (Flop, Turn, River)
	AllInDealCardsSpeed = dealCardsSpeed*4; //Zeit nach Karten aufdecken auf dem Board (Flop, Turn, River) bei AllIn
	postRiverRunAnimationSpeed = gameSpeed*18; 
	winnerBlinkSpeed = gameSpeed*3; //milliseconds
	newRoundSpeed = gameSpeed*35; 
	nextPlayerSpeed1 = gameSpeed*10; // Zeit zwischen dem Setzen des Spielers und dem Verdunkeln
	nextPlayerSpeed2 = gameSpeed*4; // Zeit zwischen Verdunkeln des einen und aufhellen des anderen Spielers
	nextPlayerSpeed3 = gameSpeed*7; // Zeit bis zwischen Aufhellen und Aktion
	preflopNextPlayerSpeed = gameSpeed*10; // Zeit bis zwischen Aufhellen und Aktion im Preflop (etwas langsamer da nicht gerechnet wird. )
}

void mainWindowImpl::breakButtonClicked() {

	if (pushButton_break->text() == "Stop") {
		pushButton_break->setDisabled(TRUE);
		breakAfterActualHand=TRUE;
	}
	else { 
             	pushButton_break->setText("Stop");
		startNewHand();
	}
}

void mainWindowImpl::paintStartSplash() {

	StartSplash *mySplash = new StartSplash(this);	
				
// 	mySplash->setMaximumSize(400,250);
// 	mySplash->setMinimumSize(400,250);
	
	mySplash->setGeometry(this->pos().x()+237,this->pos().y()+210,400,250);
// 	mySplash->setWindowFlags(Qt::SplashScreen);
	mySplash->show();
}

void mainWindowImpl::keyPressEvent ( QKeyEvent * event ) {

// 	cout << event->key() << endl;
	
	bool ctrlPressed = FALSE;

	if (event->key() == 16777220) { if(spinBox_set->hasFocus()) pushButton_set->click(); } //ENTER 
	if (event->key() == 16777265) { switchLeftToolBox(); } //F2	
	if (event->key() == 16777266) { switchRightToolBox(); } //F3
	if (event->key() == Qt::Key_F) { switchFullscreen(); } //f
	if (event->key() == Qt::Key_S) { showMyCards(); } //f	
	if (event->key() == 16777249) { 
		pushButton_break->click(); 
		ctrlPressed = TRUE;
// 		QTimer::SingleShot
	} //CTRL
// 	if (event->key() == 65) {  paintStartSplash();
		
	     
// 	if (event->key() == 66) {  label_logo->hide();	}
	
}

void mainWindowImpl::switchLeftToolBox() {

	if (groupBox_left_tools->isHidden()) { 
		label_logoleft->hide();
		groupBox_left_tools->show(); 
	}
	else { 
		groupBox_left_tools->hide(); 
		label_logoleft->show();
	}

}

void mainWindowImpl::switchRightToolBox() {

	if (groupBox_right_tools->isHidden()) { 
		label_logoright->hide();
		groupBox_right_tools->show(); 
	}
	else { 
		groupBox_right_tools->hide(); 
		label_logoright->show();
	}

}

void mainWindowImpl::switchFullscreen() {

	if (this->isFullScreen()) {
		this->showNormal();
	}
	else {
		this->showFullScreen();
	}

}
