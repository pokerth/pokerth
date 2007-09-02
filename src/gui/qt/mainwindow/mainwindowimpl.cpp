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
#include "selectavatardialogimpl.h"
#include "joinnetworkgamedialogimpl.h"
#include "connecttoserverdialogimpl.h"
#include "createnetworkgamedialogimpl.h"
#include "startnetworkgamedialogimpl.h"
#include "changehumanplayernamedialogimpl.h"
#include "gamelobbydialogimpl.h"

#include "startsplash.h"
#include "mycardspixmaplabel.h"
#include "mysetlabel.h"
#include "log.h"
#include "chat.h"

#include "playerinterface.h"
#include "boardinterface.h"
#include "handinterface.h"
#include "game.h"
#include "session.h"
#include "tools.h"
#include "qthelper.h"

#include "configfile.h"
#include "sdlplayer.h"

#include <gamedata.h>
#include <generic/serverguiwrapper.h>

#include <net/socket_msg.h>

#define FORMATLEFT(X) "<p align='center'>(X)"
#define FORMATRIGHT(X) "(X)</p>"

using namespace std;

mainWindowImpl::mainWindowImpl(ConfigFile *c, QMainWindow *parent)
     : QMainWindow(parent), myConfig(c), gameSpeed(0), myActionIsBet(0), myActionIsRaise(0), breakAfterActualHand(FALSE)
{
	int i;

//	this->setStyle(new QPlastiqueStyle);

	//for statistic development
	for(i=0; i<15; i++) {
		statisticArray[i] = 0;
	}
	////////////////////////////

	myQtHelper = new QtHelper;

// Resourcen abladen 
	QFile preflopValuesFile(myQtHelper->getDataPath() +"misc/preflopValues");
	QFile preflopValuesFileDest(QString::fromUtf8(myConfig->readConfigString("DataDir").c_str())+QString("preflopValues"));
// 	if(!preflopValuesFileDest.exists()) {
	preflopValuesFile.copy(QString::fromUtf8(myConfig->readConfigString("DataDir").c_str())+QString("preflopValues"));
// 	}

	QFile flopValuesFile(myQtHelper->getDataPath() +"misc/flopValues");
	QFile flopValuesFileDest(QString::fromUtf8(myConfig->readConfigString("DataDir").c_str())+QString("flopValues"));
// 	if(!flopValuesFileDest.exists()) {
	flopValuesFile.copy(QString::fromUtf8(myConfig->readConfigString("DataDir").c_str())+QString("flopValues"));
// 	}

// 	Schriftart laden und für Dialoge setzen
#ifdef _WIN32
	QFont tmpFont1;
	tmpFont1.setFamily("Arial");
	tmpFont1.setPixelSize(12);
// 	if(this->logicalDpiX() > 105) { tmpFont.setFont("Arial",8); }
// 	else { QFont tmpFont("Arial",9); }

#else 
	QFontDatabase::addApplicationFont (myQtHelper->getDataPath() +"fonts/n019003l.pfb");
	QFontDatabase::addApplicationFont (myQtHelper->getDataPath() +"fonts/VeraBd.ttf");
// 	QFont tmpFont("Nimbus Sans L",9);
	QFont tmpFont1;
	tmpFont1.setFamily("Nimbus Sans L");
	tmpFont1.setPixelSize(12);
#endif
	QApplication::setFont(tmpFont1);

	setupUi(this);

	//pixmapCardsLabel erstellen und ins Layout einfügen!
	pixmapLabel_cardBoard0 = new MyCardsPixmapLabel(frame_Board);
	pixmapLabel_cardBoard0->setObjectName(QString::fromUtf8("pixmapLabel_BoardCard0"));
	pixmapLabel_cardBoard0->setScaledContents(true);
	pixmapLabel_cardBoard0->setGeometry(QRect(6, 64, 79, 114));

	pixmapLabel_cardBoard1 = new MyCardsPixmapLabel(frame_Board);
    	pixmapLabel_cardBoard1->setObjectName(QString::fromUtf8("pixmapLabel_BoardCard1"));
    	pixmapLabel_cardBoard1->setScaledContents(true);
	pixmapLabel_cardBoard1->setGeometry(QRect(95, 64, 79, 114));

	pixmapLabel_cardBoard2 = new MyCardsPixmapLabel(frame_Board);
    	pixmapLabel_cardBoard2->setObjectName(QString::fromUtf8("pixmapLabel_BoardCard2"));
    	pixmapLabel_cardBoard2->setScaledContents(true);
	pixmapLabel_cardBoard2->setGeometry(QRect(184, 64, 79, 114));

	pixmapLabel_cardBoard3 = new MyCardsPixmapLabel(frame_Board);
    	pixmapLabel_cardBoard3->setObjectName(QString::fromUtf8("pixmapLabel_BoardCard3"));
    	pixmapLabel_cardBoard3->setScaledContents(true);
	pixmapLabel_cardBoard3->setGeometry(QRect(280, 64, 79, 114));
	
	pixmapLabel_cardBoard4 = new MyCardsPixmapLabel(frame_Board);
    	pixmapLabel_cardBoard4->setObjectName(QString::fromUtf8("pixmapLabel_BoardCard4"));
   	pixmapLabel_cardBoard4->setScaledContents(true);
	pixmapLabel_cardBoard4->setGeometry(QRect(376, 64, 79, 114));


	pixmapLabel_card0a = new MyCardsPixmapLabel(frame_Cards0);
    	pixmapLabel_card0a->setObjectName(QString::fromUtf8("pixmapLabel_card0a"));
	pixmapLabel_card0a->setScaledContents(true);
	pixmapLabel_card0a->setGeometry(QRect(0, 0, 80, 111));
// 	pixmapLabel_card0a->setFocusPolicy(Qt::NoFocus);

   	pixmapLabel_card0b = new MyCardsPixmapLabel(frame_Cards0);
    	pixmapLabel_card0b->setObjectName(QString::fromUtf8("pixmapLabel_card0b"));
	pixmapLabel_card0b->setScaledContents(true);
	pixmapLabel_card0b->setGeometry(QRect(39, 0, 80, 111));	

	pixmapLabel_card0b->setMyW(this);
	pixmapLabel_card0a->setMyW(this);

	pixmapLabel_card1a = new MyCardsPixmapLabel(frame_Cards1);
    	pixmapLabel_card1a->setObjectName(QString::fromUtf8("pixmapLabel_card1a"));
	pixmapLabel_card1a->setScaledContents(true);
	pixmapLabel_card1a->setGeometry(QRect(0, 0, 80, 111));

   	pixmapLabel_card1b = new MyCardsPixmapLabel(frame_Cards1);
    	pixmapLabel_card1b->setObjectName(QString::fromUtf8("pixmapLabel_card1b"));
	pixmapLabel_card1b->setScaledContents(true);
	pixmapLabel_card1b->setGeometry(QRect(39, 0, 80, 111));

	pixmapLabel_card2a = new MyCardsPixmapLabel(frame_Cards2);
    	pixmapLabel_card2a->setObjectName(QString::fromUtf8("pixmapLabel_card2a"));
	pixmapLabel_card2a->setScaledContents(true);
	pixmapLabel_card2a->setGeometry(QRect(0, 0, 80, 111));

	pixmapLabel_card2b = new MyCardsPixmapLabel(frame_Cards2);
    	pixmapLabel_card2b->setObjectName(QString::fromUtf8("pixmapLabel_card2b"));
	pixmapLabel_card2b->setScaledContents(true);
	pixmapLabel_card2b->setGeometry(QRect(39, 0, 80, 111));

	pixmapLabel_card3a = new MyCardsPixmapLabel(frame_Cards3);
    	pixmapLabel_card3a->setObjectName(QString::fromUtf8("pixmapLabel_card3a"));
	pixmapLabel_card3a->setScaledContents(true);
	pixmapLabel_card3a->setGeometry(QRect(0, 0, 80, 111));

	pixmapLabel_card3b = new MyCardsPixmapLabel(frame_Cards3);
    	pixmapLabel_card3b->setObjectName(QString::fromUtf8("pixmapLabel_card3b"));
	pixmapLabel_card3b->setScaledContents(true);
	pixmapLabel_card3b->setGeometry(QRect(39, 0, 80, 111));

	pixmapLabel_card4a = new MyCardsPixmapLabel(frame_Cards4);
    	pixmapLabel_card4a->setObjectName(QString::fromUtf8("pixmapLabel_card4a"));
	pixmapLabel_card4a->setScaledContents(true);
	pixmapLabel_card4a->setGeometry(QRect(0, 0, 80, 111));

	pixmapLabel_card4b = new MyCardsPixmapLabel(frame_Cards4);
    	pixmapLabel_card4b->setObjectName(QString::fromUtf8("pixmapLabel_card4b"));
	pixmapLabel_card4b->setScaledContents(true);
	pixmapLabel_card4b->setGeometry(QRect(39, 0, 80, 111));

	pixmapLabel_card5a = new MyCardsPixmapLabel(frame_Cards5);
    	pixmapLabel_card5a->setObjectName(QString::fromUtf8("pixmapLabel_card5a"));
	pixmapLabel_card5a->setScaledContents(true);
	pixmapLabel_card5a->setGeometry(QRect(0, 0, 80, 111));

	pixmapLabel_card5b = new MyCardsPixmapLabel(frame_Cards5);
    	pixmapLabel_card5b->setObjectName(QString::fromUtf8("pixmapLabel_card5b"));
	pixmapLabel_card5b->setScaledContents(true);
	pixmapLabel_card5b->setGeometry(QRect(39, 0, 80, 111));

	pixmapLabel_card6a = new MyCardsPixmapLabel(frame_Cards6);
    	pixmapLabel_card6a->setObjectName(QString::fromUtf8("pixmapLabel_card6a"));
	pixmapLabel_card6a->setScaledContents(true);
	pixmapLabel_card6a->setGeometry(QRect(0, 0, 80, 111));

	pixmapLabel_card6b = new MyCardsPixmapLabel(frame_Cards6);
    	pixmapLabel_card6b->setObjectName(QString::fromUtf8("pixmapLabel_card6b"));
	pixmapLabel_card6b->setScaledContents(true);
	pixmapLabel_card6b->setGeometry(QRect(39, 0, 80, 111));


	//Flipside festlegen;
	flipside = new QPixmap(myQtHelper->getDataPath() +"gfx/cards/default/flipside.png");
	
	if (myConfig->readConfigInt("FlipsideOwn") && myConfig->readConfigString("FlipsideOwnFile") != "") {
		QPixmap tmpFlipside(QString::fromUtf8(myConfig->readConfigString("FlipsideOwnFile").c_str()));
		flipside = new QPixmap(tmpFlipside.scaled(QSize(80, 111),Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
	}
	else { flipside->load(myQtHelper->getDataPath() +"gfx/cards/default/flipside.png"); }

	//Flipside Animation noch nicht erledigt
	flipHolecardsAllInAlreadyDone = FALSE;

	//Toolboxen verstecken?				
	if (!myConfig->readConfigInt("ShowRightToolBox")) { 
		groupBox_RightToolBox->hide(); 
	}
	if (!myConfig->readConfigInt("ShowLeftToolBox")) { 
		groupBox_LeftToolBox->hide(); 
	}

	//Intro abspielen?
	if (myConfig->readConfigInt("ShowIntro")) { 
// 		label_logo->hide();
		QTimer::singleShot(100, this, SLOT( paintStartSplash() )); }
			
	// userWidgetsArray init
	userWidgetsArray[0] = pushButton_BetRaise;
	userWidgetsArray[1] = pushButton_CallCheckSet;
	userWidgetsArray[2] = pushButton_FoldAllin;
	userWidgetsArray[3] = spinBox_set;

	//hide userWidgets
	for(i=0; i<4; i++) {
		userWidgetsArray[i]->hide();
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
	postRiverRunAnimation2_flipHoleCards1Timer = new QTimer(this);
	postRiverRunAnimation2_flipHoleCards2Timer = new QTimer(this);
	postRiverRunAnimation3Timer = new QTimer(this);
	postRiverRunAnimation5Timer = new QTimer(this);
	potDistributeTimer = new QTimer(this);
	postRiverRunAnimation6Timer = new QTimer(this);

	blinkingStartButtonAnimationTimer = new QTimer(this);

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
	postRiverRunAnimation2_flipHoleCards1Timer->setSingleShot(TRUE);
	postRiverRunAnimation2_flipHoleCards2Timer->setSingleShot(TRUE);
	postRiverRunAnimation3Timer->setSingleShot(TRUE);
	postRiverRunAnimation5Timer->setSingleShot(TRUE);
	postRiverRunAnimation6Timer->setSingleShot(TRUE);

	// buttonLabelArray init
	buttonLabelArray[0] = textLabel_Button0;
	buttonLabelArray[1] = textLabel_Button1;
	buttonLabelArray[2] = textLabel_Button2;
	buttonLabelArray[3] = textLabel_Button3;
	buttonLabelArray[4] = textLabel_Button4;
	buttonLabelArray[5] = textLabel_Button5;
	buttonLabelArray[6] = textLabel_Button6;

	// cashLabelArray init
	cashLabelArray[0] = textLabel_Cash0;
	cashLabelArray[1] = textLabel_Cash1;
	cashLabelArray[2] = textLabel_Cash2;
	cashLabelArray[3] = textLabel_Cash3;
	cashLabelArray[4] = textLabel_Cash4;
	cashLabelArray[5] = textLabel_Cash5;
	cashLabelArray[6] = textLabel_Cash6;

	// cashTopLabelArray init
	cashTopLabelArray[0] = textLabel_TopCash0;
	cashTopLabelArray[1] = textLabel_TopCash1;
	cashTopLabelArray[2] = textLabel_TopCash2;
	cashTopLabelArray[3] = textLabel_TopCash3;
	cashTopLabelArray[4] = textLabel_TopCash4;
	cashTopLabelArray[5] = textLabel_TopCash5;
	cashTopLabelArray[6] = textLabel_TopCash6;

	// playerNameLabelArray init
	playerNameLabelArray[0] = label_PlayerName0;
	playerNameLabelArray[1] = label_PlayerName1;
	playerNameLabelArray[2] = label_PlayerName2;
	playerNameLabelArray[3] = label_PlayerName3;
	playerNameLabelArray[4] = label_PlayerName4;
	playerNameLabelArray[5] = label_PlayerName5;
	playerNameLabelArray[6] = label_PlayerName6;

	// playerAvatarLabelArray init
	playerAvatarLabelArray[0] = label_Avatar0;
	playerAvatarLabelArray[1] = label_Avatar1;
	playerAvatarLabelArray[2] = label_Avatar2;
	playerAvatarLabelArray[3] = label_Avatar3;
	playerAvatarLabelArray[4] = label_Avatar4;
	playerAvatarLabelArray[5] = label_Avatar5;
	playerAvatarLabelArray[6] = label_Avatar6;

	// setLabelArray init
	setLabelArray[0] = textLabel_Set0;
	setLabelArray[1] = textLabel_Set1;
	setLabelArray[2] = textLabel_Set2;
	setLabelArray[3] = textLabel_Set3;
	setLabelArray[4] = textLabel_Set4;
	setLabelArray[5] = textLabel_Set5;
	setLabelArray[6] = textLabel_Set6;
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) { setLabelArray[i]->setMyW(this); }

	// statusLabelArray init
	actionLabelArray[0] = textLabel_Status0;
	actionLabelArray[1] = textLabel_Status1;
	actionLabelArray[2] = textLabel_Status2;
	actionLabelArray[3] = textLabel_Status3;
	actionLabelArray[4] = textLabel_Status4;
	actionLabelArray[5] = textLabel_Status5;
	actionLabelArray[6] = textLabel_Status6;

	textLabel_Status0->setMyW(this);

	// GroupBoxArray init
	groupBoxArray[0] = groupBox0;
	groupBoxArray[1] = groupBox1;
	groupBoxArray[2] = groupBox2;
	groupBoxArray[3] = groupBox3;
	groupBoxArray[4] = groupBox4;
	groupBoxArray[5] = groupBox5;
	groupBoxArray[6] = groupBox6;

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
	holeCardsArray[5][0] = pixmapLabel_card5a;
	holeCardsArray[5][1] = pixmapLabel_card5b;
	holeCardsArray[6][0] = pixmapLabel_card6a;
	holeCardsArray[6][1] = pixmapLabel_card6b;

	// Farben initalisieren
	active.setRgb(86,170,86);
	inactive.setRgb(83,141,107);
	highlight.setRgb(151,214,109);

	//Schriftart und Schriftgrößen für Widgets festlegen 
#ifdef _WIN32
	tmpFont1.setPixelSize(11);
	textBrowser_Log->setStyleSheet("QTextBrowser { font-family: \"Nimbus Sans L\"; font-size: 11px; font-color: white; background-color: #285200;}");
#else
	tmpFont1.setPixelSize(10);
	textBrowser_Log->setStyleSheet("QTextBrowser { font-family: \"Nimbus Sans L\"; font-size: 10px; font-color: white; background-color: #285200; }");
	textBrowser_Chat->setStyleSheet("QTextBrowser { font-family: \"Nimbus Sans L\"; font-size: 10px; font-color: white; background-color: #285200;  }");
	lineEdit_ChatInput->setStyleSheet("QLineEdit { font-family: \"Nimbus Sans L\"; font-size: 10px; font-color: white; background-color: #285200;  }");
#endif

#ifdef __APPLE__
	tmpFont1.setPixelSize(11);
	tabWidget_Right->setStyleSheet("QTabWidget { font-family: \"Nimbus Sans L\"; font-size: 11px; font-color: white; background-color: #285200;  }");
	tabWidget_Left->setStyleSheet("QTabWidget { font-family: \"Nimbus Sans L\"; font-size: 11px; font-color: white; background-color: #285200;  }");
#else
	tmpFont1.setPixelSize(10);
	tabWidget_Right->setStyleSheet("QTabWidget { font-family: \"Nimbus Sans L\"; font-size: 10px; font-color: white; background-color: #285200;  }");
	tabWidget_Left->setStyleSheet("QTabWidget { font-family: \"Nimbus Sans L\"; font-size: 10px; font-color: white; background-color: #285200;  }");
#endif


	QFont tmpFont2;
	tmpFont2.setFamily("Bitstream Vera Sans");

	tmpFont2.setPixelSize(10);
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {

		cashTopLabelArray[i]->setStyleSheet("QLabel { font-family: \"Bitstream Vera Sans\"; font-size: 10px }");
		cashLabelArray[i]->setStyleSheet("QLabel { font-family: \"Bitstream Vera Sans\"; font-size: 10px }");
	}

	spinBox_set->setFont(tmpFont2);


	tmpFont2.setPixelSize(12);
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {

		setLabelArray[i]->setStyleSheet("QLabel { font-family: \"Bitstream Vera Sans\"; font-size: 12px }");
	}

	tmpFont2.setPixelSize(13);
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {

		playerNameLabelArray[i]->setStyleSheet("QLabel { font-family: \"Bitstream Vera Sans\"; font-size: 13px }");
	}

	label_Sets->setStyleSheet("QLabel { font-family: \"Bitstream Vera Sans\"; font-size: 13px }");
	label_Total->setStyleSheet("QLabel { font-family: \"Bitstream Vera Sans\"; font-size: 13px }");
	textLabel_Sets->setStyleSheet("QLabel { font-family: \"Bitstream Vera Sans\"; font-size: 13px }");
	textLabel_Pot->setStyleSheet("QLabel { font-family: \"Bitstream Vera Sans\"; font-size: 13px }");
	label_handNumber->setStyleSheet("QLabel { font-family: \"Bitstream Vera Sans\"; font-size: 13px }");
	label_gameNumber->setStyleSheet("QLabel { font-family: \"Bitstream Vera Sans\"; font-size: 13px }");
	textLabel_handNumber->setStyleSheet("QLabel { font-family: \"Bitstream Vera Sans\"; font-size: 13px }");
	textLabel_gameNumber->setStyleSheet("QLabel { font-family: \"Bitstream Vera Sans\"; font-size: 13px }");

// 	tmpFont2.setPixelSize(15);
	tmpFont2.setPixelSize(17);
	tmpFont2.setBold(TRUE);
	textLabel_handLabel->setStyleSheet("QLabel { font-family: \"Bitstream Vera Sans\"; font-size: 17px }");

	tmpFont2.setPixelSize(18);
	label_Pot->setStyleSheet("QLabel { font-family: \"Bitstream Vera Sans\"; font-size: 18px }");


	//Widgets Grafiken setzen
	label_CardHolder0->setPixmap(myQtHelper->getDataPath() + "gfx/gui/table/default/cardholder_flop.png");
	label_CardHolder1->setPixmap(myQtHelper->getDataPath() + "gfx/gui/table/default/cardholder_flop.png");
	label_CardHolder2->setPixmap(myQtHelper->getDataPath() + "gfx/gui/table/default/cardholder_flop.png");
	label_CardHolder3->setPixmap(myQtHelper->getDataPath() + "gfx/gui/table/default/cardholder_turn.png");
	label_CardHolder4->setPixmap(myQtHelper->getDataPath() + "gfx/gui/table/default/cardholder_river.png");

	label_Handranking->setPixmap(myQtHelper->getDataPath() + "gfx/gui/misc/handRanking.png");

	//Widgets Grafiken per Stylesheets setzen
	this->setStyleSheet("QMainWindow { background-image: url(" + myQtHelper->getDataPath() +"gfx/gui/table/default/table.png); background-position: bottom center; background-origin: content;}");

	//Groupbox Background 
	for (i=1; i<MAX_NUMBER_OF_PLAYERS; i++) {

		groupBoxArray[i]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myQtHelper->getDataPath() +"gfx/gui/table/default/opponentBoxInactiveGlow.png) }"); 
	}
	groupBoxArray[0]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myQtHelper->getDataPath() +"gfx/gui/table/default/playerBoxInactiveGlow.png) }"); 

		//Human player button
	pushButton_BetRaise->setStyleSheet("QPushButton { border:none; background-image: url(" + myQtHelper->getDataPath() +"gfx/gui/table/default/playeraction_03.png); font-family: \"Bitstream Vera Sans\"; font-size: 11px }");
	pushButton_CallCheckSet->setStyleSheet("QPushButton { border:none; background-image: url(" + myQtHelper->getDataPath() +"gfx/gui/table/default/playeraction_05.png); font-family: \"Bitstream Vera Sans\"; font-size: 11px }"); 
	pushButton_FoldAllin->setStyleSheet("QPushButton { border:none; background-image: url(" + myQtHelper->getDataPath() +"gfx/gui/table/default/playeraction_07.png); font-family: \"Bitstream Vera Sans\"; font-size: 11px }"); 

	groupBox_RightToolBox->setStyleSheet("QGroupBox { border:none; background-image: url(" + myQtHelper->getDataPath() +"gfx/gui/misc/toolboxFrameBG.png) }");
	groupBox_LeftToolBox->setStyleSheet("QGroupBox { border:none; background-image: url(" + myQtHelper->getDataPath() +"gfx/gui/misc/toolboxFrameBG.png) }");


	//raise actionLable above just inserted mypixmaplabel
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) { actionLabelArray[i]->raise(); }


	//resize stop-button depending on translation
	QFontMetrics tempMetrics = this->fontMetrics();
	int width = tempMetrics.width(tr("Stop"));
	pushButton_break->setMinimumSize(width+10,20);


	//Clear Focus
	groupBox_LeftToolBox->clearFocus();

	//set Focus to mainwindow
	this->setFocus();

	//windowicon
// 	QString windowIconString();
	this->setWindowIcon(QIcon(myQtHelper->getDataPath()+"gfx/gui/misc/windowicon.png")); 

	//Statusbar 
	if(myConfig->readConfigInt("ShowStatusbarMessages")) {
 
#ifdef __APPLE__
                statusBar()->showMessage(tr("Cmd+N to start a new game"));
#else
                statusBar()->showMessage(tr("Ctrl+N to start a new game"));
#endif
        }

	//Dialoge 
	myNewGameDialog = new newGameDialogImpl(this, myConfig);
	mySelectAvatarDialog = new selectAvatarDialogImpl(this, myConfig);
	mySettingsDialog = new settingsDialogImpl(this, myConfig, mySelectAvatarDialog);	
	myChangeHumanPlayerNameDialog = new changeHumanPlayerNameDialogImpl(this, myConfig);
	myJoinNetworkGameDialog = new joinNetworkGameDialogImpl(this, myConfig);
	myConnectToServerDialog = new connectToServerDialogImpl(this);
	myStartNetworkGameDialog = new startNetworkGameDialogImpl(this, myConfig);
	myCreateNetworkGameDialog = new createNetworkGameDialogImpl(this, myConfig);
	myAboutPokerthDialog = new aboutPokerthImpl(this);
	myGameLobbyDialog = new gameLobbyDialogImpl(this, myConfig);
	
	myStartNetworkGameDialog->setMyW(this);
	myGameLobbyDialog->setMyW(this);

// 	//ShortCuts 
// 	QShortcut *quitPokerTHKeys = new QShortcut(QKeySequence(Qt::Key_Control + Qt::Key_Q), this);
// 	connect( quitPokerTHKeys, SIGNAL(activated() ), actionQuit, SLOT( trigger() ) );


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
	connect(postRiverRunAnimation2_flipHoleCards1Timer, SIGNAL(timeout()), this, SLOT( postRiverRunAnimation2_flipHoleCards1() ));
	connect(postRiverRunAnimation2_flipHoleCards2Timer, SIGNAL(timeout()), this, SLOT( postRiverRunAnimation2_flipHoleCards2() ));
	connect(postRiverRunAnimation3Timer, SIGNAL(timeout()), this, SLOT( postRiverRunAnimation4() ));
	connect(potDistributeTimer, SIGNAL(timeout()), this, SLOT(postRiverRunAnimation5()));
	connect(postRiverRunAnimation5Timer, SIGNAL(timeout()), this, SLOT( postRiverRunAnimation6() ));
	connect(postRiverRunAnimation6Timer, SIGNAL(timeout()), this, SLOT( startNewHand() ));

	connect(blinkingStartButtonAnimationTimer, SIGNAL(timeout()), this, SLOT( blinkingStartButtonAnimationAction()));

	connect( actionNewGame, SIGNAL( triggered() ), this, SLOT( callNewGameDialog() ) );
	connect( actionAboutPokerth, SIGNAL( triggered() ), this, SLOT( callAboutPokerthDialog() ) );
	connect( actionSettings, SIGNAL( triggered() ), this, SLOT( callSettingsDialog() ) );
	connect( actionJoin_network_Game, SIGNAL( triggered() ), this, SLOT( callJoinNetworkGameDialog() ) );
	connect( actionCreate_network_Game, SIGNAL( triggered() ), this, SLOT( callCreateNetworkGameDialog() ) );
	connect( actionInternet_Game, SIGNAL( triggered() ), this, SLOT( callGameLobbyDialog() ) );
	connect( actionQuit, SIGNAL( triggered() ), this, SLOT( quitPokerTH() ) );
	connect( actionFullScreen, SIGNAL( triggered() ), this, SLOT( switchFullscreen() ) );
	connect( actionShowHideChat, SIGNAL( triggered() ), this, SLOT( switchChatWindow() ) );
	connect( actionShowHideHelp, SIGNAL( triggered() ), this, SLOT( switchHelpWindow() ) );
	connect( actionShowHideLog, SIGNAL( triggered() ), this, SLOT( switchLogWindow() ) );

	connect( pushButton_BetRaise, SIGNAL( clicked() ), this, SLOT( myBetRaise() ) );
	connect( pushButton_FoldAllin, SIGNAL( clicked() ), this, SLOT( myFoldAllin() ) );
	connect( pushButton_CallCheckSet, SIGNAL( clicked() ), this, SLOT( myCallCheckSet() ) );

	connect ( horizontalSlider_speed, SIGNAL( valueChanged(int)), this, SLOT ( setGameSpeed(int) ) );
	connect ( pushButton_break, SIGNAL( clicked()), this, SLOT ( breakButtonClicked() ) ); // auch wieder starten!!!!

	connect( tabWidget_Left, SIGNAL( currentChanged(int) ), this, SLOT( tabSwitchAction() ) );
	connect( lineEdit_ChatInput, SIGNAL( returnPressed () ), this, SLOT( sendChatMessage() ) );
	connect( lineEdit_ChatInput, SIGNAL( textChanged (QString) ), this, SLOT( checkChatInputLength(QString) ) );

	//Nachrichten Thread-Save
	connect(this, SIGNAL(signalInitGui(int)), this, SLOT(initGui(int)));

	connect(this, SIGNAL(signalShowNetworkStartDialog()), this, SLOT(showNetworkStartDialog()));

	connect(this, SIGNAL(signalRefreshSet()), this, SLOT(refreshSet()));
	connect(this, SIGNAL(signalRefreshCash()), this, SLOT(refreshCash()));
	connect(this, SIGNAL(signalRefreshAction(int, int)), this, SLOT(refreshAction(int, int)));
	connect(this, SIGNAL(signalRefreshChangePlayer()), this, SLOT(refreshChangePlayer()));
	connect(this, SIGNAL(signalRefreshPot()), this, SLOT(refreshPot()));
	connect(this, SIGNAL(signalRefreshGroupbox(int, int)), this, SLOT(refreshGroupbox(int, int)));
	connect(this, SIGNAL(signalRefreshAll()), this, SLOT(refreshAll()));
	connect(this, SIGNAL(signalRefreshPlayerName()), this, SLOT(refreshPlayerName()));
	connect(this, SIGNAL(signalRefreshButton()), this, SLOT(refreshButton()));
	connect(this, SIGNAL(signalRefreshGameLabels(int)), this, SLOT(refreshGameLabels(int)));

	connect(this, SIGNAL(signalGuiUpdateDone()), this, SLOT(guiUpdateDone()));

	connect(this, SIGNAL(signalMeInAction()), this, SLOT(meInAction()));
	connect(this, SIGNAL(signalDisableMyButtons()), this, SLOT(disableMyButtons()));
	connect(this, SIGNAL(signalStartTimeoutAnimation(int, int)), this, SLOT(startTimeoutAnimation(int, int)));
	connect(this, SIGNAL(signalStopTimeoutAnimation(int)), this, SLOT(stopTimeoutAnimation(int)));

	connect(this, SIGNAL(signalDealBeRoCards(int)), this, SLOT(dealBeRoCards(int)));
	connect(this, SIGNAL(signalDealHoleCards()), this, SLOT(dealHoleCards()));
	connect(this, SIGNAL(signalDealFlopCards0()), this, SLOT(dealFlopCards0()));
	connect(this, SIGNAL(signalDealTurnCards0()), this, SLOT(dealTurnCards0()));
	connect(this, SIGNAL(signalDealRiverCards0()), this, SLOT(dealRiverCards0()));

	connect(this, SIGNAL(signalNextPlayerAnimation()), this, SLOT(nextPlayerAnimation()));

		;
	connect(this, SIGNAL(signalBeRoAnimation2(int)), this, SLOT(beRoAnimation2(int)));

	connect(this, SIGNAL(signalPreflopAnimation1()), this, SLOT(preflopAnimation1()));
	connect(this, SIGNAL(signalPreflopAnimation2()), this, SLOT(preflopAnimation2()));
	connect(this, SIGNAL(signalFlopAnimation1()), this, SLOT(flopAnimation1()));
	connect(this, SIGNAL(signalFlopAnimation2()), this, SLOT(flopAnimation2()));
	connect(this, SIGNAL(signalTurnAnimation1()), this, SLOT(turnAnimation1()));
	connect(this, SIGNAL(signalTurnAnimation2()), this, SLOT(turnAnimation2()));
	connect(this, SIGNAL(signalRiverAnimation1()), this, SLOT(riverAnimation1()));
	connect(this, SIGNAL(signalRiverAnimation2()), this, SLOT(riverAnimation2()));
	connect(this, SIGNAL(signalPostRiverAnimation1()), this, SLOT(postRiverAnimation1()));
	connect(this, SIGNAL(signalPostRiverRunAnimation1()), this, SLOT(postRiverRunAnimation1()));

	connect(this, SIGNAL(signalFlipHolecardsAllIn()), this, SLOT(flipHolecardsAllIn()));

	connect(this, SIGNAL(signalNextRoundCleanGui()), this, SLOT(nextRoundCleanGui()));

	connect(this, SIGNAL(signalNetClientConnect(int)), myConnectToServerDialog, SLOT(refresh(int)));
	connect(this, SIGNAL(signalNetClientGameInfo(int)), myStartNetworkGameDialog, SLOT(refresh(int)));
	connect(this, SIGNAL(signalNetClientSelfJoined(QString, int)), myStartNetworkGameDialog, SLOT(joinedNetworkGame(QString, int)));
	connect(this, SIGNAL(signalNetClientPlayerJoined(QString, int)), myStartNetworkGameDialog, SLOT(addConnectedPlayer(QString, int)));
	connect(this, SIGNAL(signalNetClientPlayerChanged(QString, QString)), myStartNetworkGameDialog, SLOT(updatePlayer(QString, QString)));
	connect(this, SIGNAL(signalNetClientPlayerLeft(QString)), myStartNetworkGameDialog, SLOT(removePlayer(QString)));

	connect(this, SIGNAL(signalNetClientSelfJoined(QString, int)), myGameLobbyDialog, SLOT(joinedNetworkGame(QString, int)));
	connect(this, SIGNAL(signalNetClientPlayerJoined(QString, int)), myGameLobbyDialog, SLOT(addConnectedPlayer(QString, int)));
	connect(this, SIGNAL(signalNetClientPlayerChanged(QString, QString)), myGameLobbyDialog, SLOT(updatePlayer(QString, QString)));
	connect(this, SIGNAL(signalNetClientPlayerLeft(QString)), myGameLobbyDialog, SLOT(removePlayer(QString)));
	connect(this, SIGNAL(signalNetClientGameListNew(QString)), myGameLobbyDialog, SLOT(addGame(QString)));
	connect(this, SIGNAL(signalNetClientGameListRemove(QString)), myGameLobbyDialog, SLOT(removeGame(QString)));

	// Errors are handled globally, not within one dialog.
	connect(this, SIGNAL(signalNetClientError(int, int)), this, SLOT(networkError(int, int)));
	connect(this, SIGNAL(signalNetServerError(int, int)), this, SLOT(networkError(int, int)));
	connect(this, SIGNAL(signalNetClientGameStart(boost::shared_ptr<Game>)), this, SLOT(networkStart(boost::shared_ptr<Game>)));

	//Sound
	mySDLPlayer = new SDLPlayer(myConfig);

}

mainWindowImpl::~mainWindowImpl() {

	delete myConfig;
	myConfig = 0;

}

void mainWindowImpl::callNewGameDialog() {

	//wenn Dialogfenster gezeigt werden soll
	if(myConfig->readConfigInt("ShowGameSettingsDialogOnNewGame")){

		myNewGameDialog->exec();
		if (myNewGameDialog->result() == QDialog::Accepted ) { startNewLocalGame(myNewGameDialog);	}
// 		else { startNewLocalGame(); }
	}
	// sonst mit gespeicherten Werten starten
	else { startNewLocalGame(); }
}

void mainWindowImpl::startNewLocalGame(newGameDialogImpl *v) {

	// Start new local game - terminate existing network game.
	mySession->terminateNetworkClient();
	if (myServerGuiInterface.get())
		myServerGuiInterface->getSession().terminateNetworkServer();

	//get values from local game dialog
	GameData gameData;
	if(v) {
		// Set Game Data
		gameData.maxNumberOfPlayers = v->spinBox_quantityPlayers->value();
		gameData.startMoney = v->spinBox_startCash->value();
		gameData.smallBlind = v->spinBox_smallBlind->value();
		gameData.handsBeforeRaise = v->spinBox_handsBeforeRaiseSmallBlind->value();
		//Speeds 
		gameData.guiSpeed = v->spinBox_gameSpeed->value();
	}
	// start with default values
	else {
		// Set Game Data
		gameData.maxNumberOfPlayers = myConfig->readConfigInt("NumberOfPlayers");
		gameData.startMoney = myConfig->readConfigInt("StartCash");
		gameData.smallBlind = myConfig->readConfigInt("SmallBlind");
		gameData.handsBeforeRaise = myConfig->readConfigInt("HandsBeforeRaiseSmallBlind");
		//Speeds 
		gameData.guiSpeed = myConfig->readConfigInt("GameSpeed");
	}
	// Set dealer pos.
	StartData startData;
	int tmpDealerPos = 0;
	startData.numberOfPlayers = gameData.maxNumberOfPlayers;
	Tools::getRandNumber(0, startData.numberOfPlayers-1, 1, &tmpDealerPos, 0);
	if(DEBUG_MODE) {
		tmpDealerPos = 2;
	}
	startData.startDealerPlayerId = static_cast<unsigned>(tmpDealerPos);

	//some gui modifications
	localGameModification();

	//Start Game!!!
	mySession->startLocalGame(gameData, startData);
}


void mainWindowImpl::callAboutPokerthDialog() {	myAboutPokerthDialog->exec(); }

void mainWindowImpl::callCreateNetworkGameDialog() {
	
	myCreateNetworkGameDialog->exec();
// 
	if (myCreateNetworkGameDialog->result() == QDialog::Accepted ) {

		if (!myServerGuiInterface.get())
		{
			// Create pseudo Gui Wrapper for the server.
			myServerGuiInterface.reset(new ServerGuiWrapper(myConfig, mySession->getGui(), mySession->getGui()));
			{
				boost::shared_ptr<Session> session(new Session(myServerGuiInterface.get(), myConfig));
				myServerGuiInterface->setSession(session);
			}
		}

		// Terminate existing network games.
		mySession->terminateNetworkClient();
		myServerGuiInterface->getSession().terminateNetworkServer();

		GameData gameData;
		gameData.maxNumberOfPlayers = myCreateNetworkGameDialog->spinBox_quantityPlayers->value();
		gameData.startMoney = myCreateNetworkGameDialog->spinBox_startCash->value();
		gameData.smallBlind = myCreateNetworkGameDialog->spinBox_smallBlind->value();
		gameData.handsBeforeRaise = myCreateNetworkGameDialog->spinBox_handsBeforeRaiseSmallBlind->value();
		gameData.guiSpeed = myCreateNetworkGameDialog->spinBox_gameSpeed->value();
		gameData.playerActionTimeoutSec = myCreateNetworkGameDialog->spinBox_netTimeOutPlayerAction->value();

		myGameLobbyDialog->setSession(&getSession());
		myGameLobbyDialog->treeWidget_GameList->clear();
		myStartNetworkGameDialog->setSession(&getSession());
		myStartNetworkGameDialog->treeWidget->clear();

		myServerGuiInterface->getSession().startNetworkServer();
		mySession->startNetworkClientForLocalServer(gameData);

		myStartNetworkGameDialog->setMaxPlayerNumber(gameData.maxNumberOfPlayers);

		myStartNetworkGameDialog->setWindowTitle("Start Network Game");

		showNetworkStartDialog();
	}

}

void mainWindowImpl::callJoinNetworkGameDialog() {

	myJoinNetworkGameDialog->exec();

	if (myJoinNetworkGameDialog->result() == QDialog::Accepted ) {

		mySession->terminateNetworkClient();
		if (myServerGuiInterface.get())
			myServerGuiInterface->getSession().terminateNetworkServer();

		myGameLobbyDialog->setSession(&getSession());
		myGameLobbyDialog->treeWidget_GameList->clear();
		myStartNetworkGameDialog->setSession(&getSession());
		myStartNetworkGameDialog->treeWidget->clear();
		// Maybe use QUrl::toPunycode.
		mySession->startNetworkClient(
			myJoinNetworkGameDialog->lineEdit_ipAddress->text().toUtf8().constData(),
			myJoinNetworkGameDialog->spinBox_port->value(),
			myJoinNetworkGameDialog->checkBox_ipv6->isChecked(),
			myJoinNetworkGameDialog->checkBox_sctp->isChecked(),
			myJoinNetworkGameDialog->lineEdit_password->text().toUtf8().constData());

		//Dialog mit Statusbalken
		myConnectToServerDialog->exec();

		if (myConnectToServerDialog->result() == QDialog::Rejected ) {
			mySession->terminateNetworkClient();
			actionJoin_network_Game->trigger(); // re-trigger
		}
		else {
			//needed for join and ready sounds - TODO
			//myStartNetworkGameDialog->setMaxPlayerNumber(gameData.maxNumberOfPlayers);
			myStartNetworkGameDialog->setWindowTitle("Start Network Game");

			showNetworkStartDialog();
		}
	}
}

void mainWindowImpl::callGameLobbyDialog() {

	mySession->terminateNetworkClient();
	if (myServerGuiInterface.get())
		myServerGuiInterface->getSession().terminateNetworkServer();

	myGameLobbyDialog->setSession(&getSession());
	myGameLobbyDialog->treeWidget_GameList->clear();
	myStartNetworkGameDialog->treeWidget->clear();
	myStartNetworkGameDialog->setSession(&getSession());

	// Start client for dedicated server.
	mySession->startInternetClient();

	myGameLobbyDialog->exec(); 

	if (myGameLobbyDialog->result() == QDialog::Accepted)
	{
		if(myGameLobbyDialog->getCurrentGameName() != "") {	
		myStartNetworkGameDialog->setWindowTitle(myGameLobbyDialog->getCurrentGameName());
	}
		showNetworkStartDialog();
	}
	else
	{
		mySession->terminateNetworkClient();
	}
}

void mainWindowImpl::callSettingsDialog() {

	mySettingsDialog->exec();
	
	if (mySettingsDialog->result() == QDialog::Accepted && mySettingsDialog->getSettingsCorrect()) {
		//Toolbox verstecken?
		if (myConfig->readConfigInt("ShowLeftToolBox")) { groupBox_LeftToolBox->show(); }
		else { groupBox_LeftToolBox->hide(); }

		if (myConfig->readConfigInt("ShowRightToolBox")) { groupBox_RightToolBox->show(); }
		else { groupBox_RightToolBox->hide(); }
		
		//Falls Spielernamen geändert wurden --> neu zeichnen --> erst beim nächsten Neustart neu ausgelesen
		if (mySettingsDialog->getPlayerNickIsChanged() && mySession->getCurrentGame() && !mySession->isNetworkClientRunning()) { 

			HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();
			currentHand->getPlayerArray()[0]->setMyName(mySettingsDialog->lineEdit_HumanPlayerName->text().toUtf8().constData());
			currentHand->getPlayerArray()[1]->setMyName(mySettingsDialog->lineEdit_Opponent1Name->text().toUtf8().constData());
			currentHand->getPlayerArray()[2]->setMyName(mySettingsDialog->lineEdit_Opponent2Name->text().toUtf8().constData());
			currentHand->getPlayerArray()[3]->setMyName(mySettingsDialog->lineEdit_Opponent3Name->text().toUtf8().constData());
			currentHand->getPlayerArray()[4]->setMyName(mySettingsDialog->lineEdit_Opponent4Name->text().toUtf8().constData());
			currentHand->getPlayerArray()[5]->setMyName(mySettingsDialog->lineEdit_Opponent5Name->text().toUtf8().constData());
			currentHand->getPlayerArray()[6]->setMyName(mySettingsDialog->lineEdit_Opponent6Name->text().toUtf8().constData());
			mySettingsDialog->setPlayerNickIsChanged(FALSE);

			refreshPlayerName();
		}
	
		if(mySession->getCurrentGame() && !mySession->isNetworkClientRunning()) {

			HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();
			currentHand->getPlayerArray()[0]->setMyAvatar(mySettingsDialog->pushButton_HumanPlayerAvatar->getMyLink().toUtf8().constData());
			currentHand->getPlayerArray()[1]->setMyAvatar(mySettingsDialog->pushButton_Opponent1Avatar->getMyLink().toUtf8().constData());
			currentHand->getPlayerArray()[2]->setMyAvatar(mySettingsDialog->pushButton_Opponent2Avatar->getMyLink().toUtf8().constData());
			currentHand->getPlayerArray()[3]->setMyAvatar(mySettingsDialog->pushButton_Opponent3Avatar->getMyLink().toUtf8().constData());
			currentHand->getPlayerArray()[4]->setMyAvatar(mySettingsDialog->pushButton_Opponent4Avatar->getMyLink().toUtf8().constData());
			currentHand->getPlayerArray()[5]->setMyAvatar(mySettingsDialog->pushButton_Opponent5Avatar->getMyLink().toUtf8().constData());
			currentHand->getPlayerArray()[6]->setMyAvatar(mySettingsDialog->pushButton_Opponent6Avatar->getMyLink().toUtf8().constData());

			//avatar refresh
			refreshPlayerAvatar();		
		}

		//Flipside refresh
		if (myConfig->readConfigInt("FlipsideOwn") && myConfig->readConfigString("FlipsideOwnFile") != "") {

			QPixmap tmpFlipside(QString::fromUtf8(myConfig->readConfigString("FlipsideOwnFile").c_str()));
			flipside = new QPixmap(tmpFlipside.scaled(QSize(57, 80)));
		}
		else { flipside->load(myQtHelper->getDataPath() +"gfx/cards/default/flipside.png"); }

		if(mySession->getCurrentGame()) {
			//blind buttons refresh
			refreshButton();
		}
		int i,j;

		for (i=1; i<MAX_NUMBER_OF_PLAYERS; i++ ) { 
			for ( j=0; j<=1; j++ ) {
				if (holeCardsArray[i][j]->getIsFlipside()) {
					holeCardsArray[i][j]->setPixmap(*flipside, TRUE);
				}
			}
		}
		// Re-init audio.
		mySDLPlayer->audioDone();
		mySDLPlayer->initAudio();
	}
}

void mainWindowImpl::initGui(int speed)
{
	//restliche Singleshots killen!!!
	stopTimer();
		
	label_Pot->setText("<span style='font-weight:bold'>Pot</span>");
	label_Total->setText("<span style='font-weight:bold'>Total:</span>");
	label_Sets->setText("<span style='font-weight:bold'>Sets:</span>");
	label_handNumber->setText("<span style='font-weight:bold'>Hand:</span>");
	label_gameNumber->setText("<span style='font-weight:bold'>Game:</span>");
	
	//Tools und Board aufhellen und enablen
// 	QPalette tempPalette = groupBox_board->palette();
// 	tempPalette.setColor(QPalette::Window, active);
// 	groupBox_board->setPalette(tempPalette);
	groupBox_RightToolBox->setDisabled(FALSE);
	groupBox_LeftToolBox->setDisabled(FALSE);	
		
	//show human player buttons
	for(int i=0; i<3; i++) {
		userWidgetsArray[i]->show();
	}
	
	//set speeds for local game and for first network game
	if( !mySession->isNetworkClientRunning() || (mySession->isNetworkClientRunning() && !mySession->getCurrentGame()) ) {
	
		guiGameSpeed = speed;
		//positioning Slider
		horizontalSlider_speed->setValue(guiGameSpeed);
		setSpeeds();
	}
}

void mainWindowImpl::showNetworkStartDialog()
{
	myStartNetworkGameDialog->exec();

	if (myStartNetworkGameDialog->result() == QDialog::Accepted ) {
		
		//some gui modifications
		networkGameModification();
	}
	else {
		mySession->terminateNetworkClient();
		if (myServerGuiInterface)
			myServerGuiInterface->getSession().terminateNetworkServer();
	}
}

Session &mainWindowImpl::getSession() { assert(mySession.get()); return *mySession; }
void mainWindowImpl::setSession(boost::shared_ptr<Session> session) { mySession = session; }


//refresh-Funktionen
void mainWindowImpl::refreshSet() {
	
	int i;
 	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) { 
		if(mySession->getCurrentGame()->getCurrentHand()->getPlayerArray()[i]->getMySet() == 0) setLabelArray[i]->setText("");
		else setLabelArray[i]->setText("<p align='center'><b>Set:</b> "+QString::number(mySession->getCurrentGame()->getCurrentHand()->getPlayerArray()[i]->getMySet(),10)+" $</p>"); 
	}
}

void mainWindowImpl::refreshButton() {

	QPixmap dealerButton(myQtHelper->getDataPath() +"gfx/gui/table/default/dealerPuck.png");
	QPixmap smallblindButton(myQtHelper->getDataPath() +"gfx/gui/table/default/smallblindPuck.png");
	QPixmap bigblindButton(myQtHelper->getDataPath() +"gfx/gui/table/default/bigblindPuck.png");
	QPixmap onePix(myQtHelper->getDataPath() +"gfx/gui/misc/1px.png");

	int i;
	int k;
	//Aktive Spieler zählen
	int activePlayersCounter = 0;
	for (k=0; k<MAX_NUMBER_OF_PLAYERS; k++) { 
		if (mySession->getCurrentGame()->getCurrentHand()->getPlayerArray()[k]->getMyActiveStatus() == 1) activePlayersCounter++;
	}

	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();

	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) { 
		if(currentHand->getPlayerArray()[i]->getMyActiveStatus()) { 
			if(activePlayersCounter > 2) {
				switch (currentHand->getPlayerArray()[i]->getMyButton()) {
				
				case 1 : buttonLabelArray[i]->setPixmap(dealerButton); 
				break;
				case 2 : { 	
						if ( myConfig->readConfigInt("ShowBlindButtons")) buttonLabelArray[i]->setPixmap(smallblindButton); 
						else { buttonLabelArray[i]->setPixmap(onePix); }					  
					 }
				break;
				case 3 : { 
						if (myConfig->readConfigInt("ShowBlindButtons")) buttonLabelArray[i]->setPixmap(bigblindButton); 				
						else { buttonLabelArray[i]->setPixmap(onePix); }					  
					 }
				break;
				default: buttonLabelArray[i]->setPixmap(onePix);
				
				}
			}
			else {
				switch (currentHand->getPlayerArray()[i]->getMyButton()) {
			
				case 2 : buttonLabelArray[i]->setPixmap(dealerButton); 
				break;
				case 3 : { 
						if (myConfig->readConfigInt("ShowBlindButtons")) buttonLabelArray[i]->setPixmap(bigblindButton); 				
						else { buttonLabelArray[i]->setPixmap(onePix); }					  
					 }
				break;
				default: buttonLabelArray[i]->setPixmap(onePix);
				
				}
			}	
		}
		else { buttonLabelArray[i]->setPixmap(onePix); }
	}
}

void mainWindowImpl::refreshPlayerName() {

	int i;
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
		if(mySession->getCurrentGame()->getCurrentHand()->getPlayerArray()[i]->getMyActiveStatus()) { 
			playerNameLabelArray[i]->setText(QString::fromUtf8(mySession->getCurrentGame()->getCurrentHand()->getPlayerArray()[i]->getMyName().c_str()));
			
		} else {
			playerNameLabelArray[i]->setText(""); 
		
		}
		
	}
}

void mainWindowImpl::refreshPlayerAvatar() {

	QPixmap onePix(myQtHelper->getDataPath() +"gfx/gui/misc/1px.png");
	int i;

	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();

	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
		if(currentHand->getPlayerArray()[i]->getMyActiveStatus()) { 

			if(!i) {
				if(currentHand->getPlayerArray()[0]->getMyAvatar() == "" || !QFile::QFile(QString::fromUtf8(currentHand->getPlayerArray()[0]->getMyAvatar().c_str())).exists()) {
					playerAvatarLabelArray[0]->setPixmap(QPixmap(myQtHelper->getDataPath() +"gfx/gui/table/default/genereticAvatar.png"));
				}
				else {
					playerAvatarLabelArray[0]->setPixmap(QString::fromUtf8(currentHand->getPlayerArray()[0]->getMyAvatar().c_str()));
				}
			}
			else {				
				if(currentHand->getPlayerArray()[i]->getMyAvatar() == ""  || !QFile::QFile(QString::fromUtf8(currentHand->getPlayerArray()[i]->getMyAvatar().c_str())).exists()) {
					playerAvatarLabelArray[i]->setPixmap(QPixmap(myQtHelper->getDataPath() +"gfx/gui/table/default/genereticAvatar.png"));
				}
				else {
					playerAvatarLabelArray[i]->setPixmap(QString::fromUtf8(currentHand->getPlayerArray()[i]->getMyAvatar().c_str()));
				}
			}
		}	
		else {
			playerAvatarLabelArray[i]->setPixmap(onePix);
		}		
	}
}

void mainWindowImpl::refreshAction(int playerID, int playerAction) {

	QPixmap onePix(myQtHelper->getDataPath() +"gfx/gui/misc/1px.png");
	QPixmap action;

	QStringList actionArray;
	actionArray << "" << "fold" << "check" << "call" << "bet" << "raise" << "allin";

	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();

	if(playerID == -1 || playerAction == -1) {

		int i;
		for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) { 
			
			//if no action --> clear Pixmap 
			if(currentHand->getPlayerArray()[i]->getMyAction() == 0) {
				actionLabelArray[i]->setPixmap(onePix);	
			}
			else {
// 				if(i!=0 || ( i==0 && currentHand->getPlayerArray()[0]->getMyAction() != 1) ) {
					//paint action pixmap
					actionLabelArray[i]->setPixmap(QPixmap(myQtHelper->getDataPath() +"gfx/gui/table/default/action_"+actionArray[currentHand->getPlayerArray()[i]->getMyAction()]+".png"));			
// 				}		
			}
					
			if (currentHand->getPlayerArray()[i]->getMyAction()==1) { 
	// 			groupBoxArray[i]->setDisabled(TRUE);
				
				if(i != 0) {
					holeCardsArray[i][0]->setPixmap(onePix, FALSE);
					holeCardsArray[i][1]->setPixmap(onePix, FALSE);
				}
			}
		}
	} 
	else {
		//if no action --> clear Pixmap 
		if(playerAction == 0) {
			actionLabelArray[playerID]->setPixmap(onePix);	
		}
		else {
		
	// 		paint action pixmap and raise
			actionLabelArray[playerID]->setPixmap(QPixmap(myQtHelper->getDataPath() +"gfx/gui/table/default/action_"+actionArray[playerAction]+".png"));			

			//play sounds if exist
			mySDLPlayer->playSound(actionArray[playerAction].toStdString(), playerID);
		}

		if (playerAction == 1) { // FOLD

			if (playerID == 0 && !myConfig->readConfigInt("AntiPeekMode")) {
				holeCardsArray[0][0]->startFadeOut(10); 
				holeCardsArray[0][1]->startFadeOut(10); 
			}
			else {
				holeCardsArray[playerID][0]->setPixmap(onePix, FALSE);
				holeCardsArray[playerID][1]->setPixmap(onePix, FALSE);
			}
		}
	}
}

void mainWindowImpl::refreshCash() {

	int i;
	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();

	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) { 
		if(currentHand->getPlayerArray()[i]->getMyActiveStatus()) { 

			cashLabelArray[i]->setText(QString::number(currentHand->getPlayerArray()[i]->getMyCash(),10)+" $"); 
			cashTopLabelArray[i]->setText("<b>Cash:</b>"); 
			
		} else {
			cashLabelArray[i]->setText(""); 
			cashTopLabelArray[i]->setText("");
		}
	}
}

void mainWindowImpl::refreshGroupbox(int playerID, int status) {

	int i,j;

	if(playerID == -1 || status == -1) {

		HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();
		for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) { 
	
			if(currentHand->getPlayerArray()[i]->getMyTurn()) {
				//Groupbox glow wenn der Spiele dran ist. 
				if(i==0) {
					groupBoxArray[0]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myQtHelper->getDataPath() +"gfx/gui/table/default/playerBoxActiveGlow.png) }"); 
				}
				else {
					groupBoxArray[i]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myQtHelper->getDataPath() +"gfx/gui/table/default/opponentBoxActiveGlow.png) }"); 
				}
	
			} else {
				//Groupbox auf Hintergrundfarbe setzen wenn der Spiele nicht dran aber aktiv ist. 
				if(currentHand->getPlayerArray()[i]->getMyActiveStatus()) {
					if(i==0) {
						groupBoxArray[0]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myQtHelper->getDataPath() +"gfx/gui/table/default/playerBoxInactiveGlow.png) }"); 
						//show buttons
						for(j=0; j<3; j++) {
							userWidgetsArray[j]->show();
						}
					}
					else {
						groupBoxArray[i]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myQtHelper->getDataPath() +"gfx/gui/table/default/opponentBoxInactiveGlow.png) }"); 
					}	
				}
				//Groupbox verdunkeln wenn der Spiele inactive ist.  
				else {
					if(i==0) {
						groupBoxArray[0]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myQtHelper->getDataPath() +"gfx/gui/table/default/playerBoxInactiveGlow.png) }"); 
						//hide buttons
						for(j=0; j<4; j++) {
							userWidgetsArray[j]->hide();
						}
					}
					else {
						groupBoxArray[i]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myQtHelper->getDataPath() +"gfx/gui/table/default/opponentBoxInactiveGlow.png) }"); 
					}
				}
			}
		}
	}
	else {
		switch(status) {
			
		//inactive
		case 0: { 
				if (!playerID) {
					groupBoxArray[playerID]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myQtHelper->getDataPath() +"gfx/gui/table/default/playerBoxInactiveGlow.png) }"); 	
					//hide buttons
					for(j=0; j<4; j++) {
						userWidgetsArray[j]->hide();
					}					
				}
				else {
					groupBoxArray[playerID]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myQtHelper->getDataPath() +"gfx/gui/table/default/opponentBoxInactiveGlow.png) }"); 
				}
			}
		break;
		//active but fold
		case 1: {
				if (!playerID) {
					groupBoxArray[playerID]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myQtHelper->getDataPath() +"gfx/gui/table/default/playerBoxInactiveGlow.png) }"); 	
					//show buttons
					for(j=0; j<3; j++) {
						userWidgetsArray[j]->show();
					}		
				}
				else {
					groupBoxArray[playerID]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myQtHelper->getDataPath() +"gfx/gui/table/default/opponentBoxInactiveGlow.png) }"); 
				}
			}
		break;
		//active in action
		case 2:  {
				if (!playerID) {
					groupBoxArray[playerID]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myQtHelper->getDataPath() +"gfx/gui/table/default/playerBoxActiveGlow.png) }"); 
				}
				else {
					groupBoxArray[playerID]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myQtHelper->getDataPath() +"gfx/gui/table/default/opponentBoxActiveGlow.png) }"); 
				}
			}
		break;
		//active not in action
		case 3:  {
				if (!playerID) {
					groupBoxArray[playerID]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myQtHelper->getDataPath() +"gfx/gui/table/default/playerBoxInactiveGlow.png) }"); 	
				}
				else {
					groupBoxArray[playerID]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myQtHelper->getDataPath() +"gfx/gui/table/default/opponentBoxInactiveGlow.png) }"); 
				}
			}
		break;
		default: {}
		}
	}
}

void mainWindowImpl::refreshGameLabels(int gameState) { 

	switch(gameState) {
		case 0: {
			textLabel_handLabel->setText("Preflop");
		} break;
		case 1: {
			textLabel_handLabel->setText("Flop");
		} break;
		case 2: {
			textLabel_handLabel->setText("Turn");
		} break;
		case 3: {
			textLabel_handLabel->setText("River");
		} break;
		case 4: {
			textLabel_handLabel->setText("");
		} break;
		default: {
			textLabel_handLabel->setText("!!! ERROR !!!");
		}
	}

	textLabel_handNumber->setText(QString::number(mySession->getCurrentGame()->getCurrentHand()->getMyID(),10));
	textLabel_gameNumber->setText(QString::number(mySession->getCurrentGame()->getMyGameID(),10));
}

void mainWindowImpl::refreshAll() {
	
	int i;
	
	refreshSet();
	refreshButton();
	for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
		refreshAction( i, mySession->getCurrentGame()->getPlayerArray()[i]->getMyAction());
	}
	refreshCash();
	refreshGroupbox();
	refreshPlayerName();
	refreshPlayerAvatar();
}

void mainWindowImpl::refreshChangePlayer() {

	int i;

	refreshSet();
	for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
		refreshAction( i, mySession->getCurrentGame()->getPlayerArray()[i]->getMyAction());
	}
	refreshCash();
}

void mainWindowImpl::refreshPot() {
	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();

	textLabel_Sets->setText("<span style='font-weight:bold'>"+QString::number(currentHand->getBoard()->getSets(),10)+" $</span>");
	textLabel_Pot->setText("<span style='font-weight:bold'>"+QString::number(currentHand->getBoard()->getPot(),10)+" $</span>");
}

void mainWindowImpl::guiUpdateDone() {
	guiUpdateSemaphore.release();
}

void mainWindowImpl::waitForGuiUpdateDone() {
	guiUpdateSemaphore.acquire();
}

void mainWindowImpl::dealHoleCards() {

	QPixmap onePix(myQtHelper->getDataPath() +"gfx/gui/misc/1px.png");

	//TempArrays
	QPixmap tempCardsPixmapArray[2];
	int tempCardsIntArray[2];
	
	// Karten der Gegner und eigene Karten austeilen
	int i, j;
	Game *currentGame = mySession->getCurrentGame();
	for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
		currentGame->getPlayerArray()[i]->getMyCards(tempCardsIntArray);	
		for(j=0; j<2; j++) {
			if(currentGame->getPlayerArray()[i]->getMyActiveStatus()) { 
				if ((i == 0) || DEBUG_MODE) {
					if(myConfig->readConfigInt("AntiPeekMode")) {
						holeCardsArray[i][j]->setPixmap(*flipside, TRUE);
						tempCardsPixmapArray[j].load(myQtHelper->getDataPath() +"gfx/cards/default/"+QString::number(tempCardsIntArray[j], 10)+".png");
						holeCardsArray[i][j]->setFrontPixmap(tempCardsPixmapArray[j]);
					}
					else {
						tempCardsPixmapArray[j].load(myQtHelper->getDataPath() +"gfx/cards/default/"+QString::number(tempCardsIntArray[j], 10)+".png");
						holeCardsArray[i][j]->setPixmap(tempCardsPixmapArray[j],FALSE);
					}
				} 
				else {
					holeCardsArray[i][j]->setPixmap(*flipside, TRUE);
/*					holeCardsArray[i][j]->setStyleSheet("QLabel:hover { background-image:url(:/cards/resources/graphics/cards/"+QString::number(tempCardsIntArray[j], 10)+".png");*/
				}
			}
			else {
				
				holeCardsArray[i][j]->setPixmap(onePix, FALSE);
//					holeCardsArray[i][j]->repaint();
			}
		}
	}
	//fix press mouse button during bankrupt with anti-peek-mode
	this->mouseOverFlipCards(FALSE);
}

void mainWindowImpl::dealBeRoCards(int myBeRoID) {	

	switch(myBeRoID) {

		case 1: { dealFlopCards0(); }
		break;
		case 2: { dealTurnCards0(); }
		break;
		case 3: { dealRiverCards0(); }
		break;
		default: { cout << "dealBeRoCards() Error" << endl; }
	}
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
	mySession->getCurrentGame()->getCurrentHand()->getBoard()->getMyCards(tempBoardCardsArray);
	tempCardsPixmap.load(myQtHelper->getDataPath() +"gfx/cards/default/"+QString::number(tempBoardCardsArray[0], 10)+".png");
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
	mySession->getCurrentGame()->getCurrentHand()->getBoard()->getMyCards(tempBoardCardsArray);
	tempCardsPixmap.load(myQtHelper->getDataPath() +"gfx/cards/default/"+QString::number(tempBoardCardsArray[1], 10)+".png");
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
	mySession->getCurrentGame()->getCurrentHand()->getBoard()->getMyCards(tempBoardCardsArray);
	tempCardsPixmap.load(myQtHelper->getDataPath() +"gfx/cards/default/"+QString::number(tempBoardCardsArray[2], 10)+".png");
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
	
	// stable
	// wenn alle All In
	if(mySession->getCurrentGame()->getCurrentHand()->getAllInCondition()) { dealFlopCards6Timer->start(AllInDealCardsSpeed); }
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
	mySession->getCurrentGame()->getCurrentHand()->getBoard()->getMyCards(tempBoardCardsArray);
	tempCardsPixmap.load(myQtHelper->getDataPath() +"gfx/cards/default/"+QString::number(tempBoardCardsArray[3], 10)+".png");
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
	
	// stable
	// wenn alle All In
	if(mySession->getCurrentGame()->getCurrentHand()->getAllInCondition()) { dealTurnCards2Timer->start(AllInDealCardsSpeed);
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
	mySession->getCurrentGame()->getCurrentHand()->getBoard()->getMyCards(tempBoardCardsArray);
	tempCardsPixmap.load(myQtHelper->getDataPath() +"gfx/cards/default/"+QString::number(tempBoardCardsArray[4], 10)+".png");
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

	// stable
	// wenn alle All In
	if(mySession->getCurrentGame()->getCurrentHand()->getAllInCondition()) { dealRiverCards2Timer->start(AllInDealCardsSpeed);	}
	// sonst normale Variante
	else {
		dealRiverCards2Timer->start(postDealCardsSpeed);
	}
}

void mainWindowImpl::meInAction() {

	//fix buttons if escape is pressed during raise or bet
	spinBox_set->hide();
	pushButton_BetRaise->show();
	myActionIsRaise = 0;
	myActionIsBet = 0;
	
	if(myConfig->readConfigInt("ShowStatusbarMessages")) {
		statusBar()->showMessage(tr("F1 - Fold/All-In | F2 - Check/Call | F3 - Bet/Raise"), 15000);
	}
	Game *currentGame = mySession->getCurrentGame();
	HandInterface *currentHand = currentGame->getCurrentHand();

	switch (currentHand->getActualRound()) {

	case 0: {
		if (currentGame->getPlayerArray()[0]->getMyCash()+currentGame->getPlayerArray()[0]->getMySet() > currentHand->getCurrentBeRo()->getHighestSet()) { pushButton_BetRaise->setText("Raise"); }

		if (currentGame->getPlayerArray()[0]->getMySet()== currentHand->getCurrentBeRo()->getHighestSet() &&  currentGame->getPlayerArray()[0]->getMyButton() == 3) { pushButton_CallCheckSet->setText("Check"); }
		else { pushButton_CallCheckSet->setText("Call"); }
		pushButton_FoldAllin->setText("Fold"); 
	}
	break;
	case 1: {
	
		pushButton_FoldAllin->setText("Fold"); 

// 		cout << "highestSet in meInAction " << currentHand->getCurrentBeRo()->getHighestSet()  << endl;
		if (currentHand->getCurrentBeRo()->getHighestSet() == 0) { 
			pushButton_CallCheckSet->setText("Check");
			pushButton_BetRaise->setText("Bet"); 
		}
		if (currentHand->getCurrentBeRo()->getHighestSet() > 0 && currentHand->getCurrentBeRo()->getHighestSet() > currentGame->getPlayerArray()[0]->getMySet()) {
			pushButton_CallCheckSet->setText("Call");
			if (currentGame->getPlayerArray()[0]->getMyCash()+currentGame->getPlayerArray()[0]->getMySet() > currentHand->getCurrentBeRo()->getHighestSet()) { pushButton_BetRaise->setText("Raise"); }
		}
	}
	break;
	case 2: {
	
		pushButton_FoldAllin->setText("Fold"); 

// 		cout << "highestSet in meInAction " << currentHand->getCurrentBeRo()->getHighestSet()  << endl;
		if (currentHand->getCurrentBeRo()->getHighestSet() == 0) { 
			pushButton_CallCheckSet->setText("Check");
			pushButton_BetRaise->setText("Bet"); 
		}
		if (currentHand->getCurrentBeRo()->getHighestSet() > 0 && currentHand->getCurrentBeRo()->getHighestSet() > currentGame->getPlayerArray()[0]->getMySet()) {
			pushButton_CallCheckSet->setText("Call");
			if (currentGame->getPlayerArray()[0]->getMyCash()+currentGame->getPlayerArray()[0]->getMySet() > currentHand->getCurrentBeRo()->getHighestSet()) { pushButton_BetRaise->setText("Raise"); }
		}
	}
	break;
	case 3: {
	
		pushButton_FoldAllin->setText("Fold"); 

// 		cout << "highestSet in meInAction " << currentHand->getCurrentBeRo()->getHighestSet()  << endl;
		if (currentHand->getCurrentBeRo()->getHighestSet() == 0) { 
			pushButton_CallCheckSet->setText("Check");
			pushButton_BetRaise->setText("Bet");
		}
		if (currentHand->getCurrentBeRo()->getHighestSet() > 0 && currentHand->getCurrentBeRo()->getHighestSet() > currentGame->getPlayerArray()[0]->getMySet()) {
			pushButton_CallCheckSet->setText("Call");
			if (currentGame->getPlayerArray()[0]->getMyCash()+currentGame->getPlayerArray()[0]->getMySet() > currentHand->getCurrentBeRo()->getHighestSet()) { pushButton_BetRaise->setText("Raise"); }
		}
	}
	break;
	default: {}
	}
}

void mainWindowImpl::startTimeoutAnimation(int playerId, int timeoutSec) {
	assert(playerId >= 0 && playerId < mySession->getCurrentGame()->getStartQuantityPlayers());
	
	//beep for player 0
	if(playerId) { setLabelArray[playerId]->startTimeOutAnimation(timeoutSec, FALSE); }
	else { setLabelArray[playerId]->startTimeOutAnimation(timeoutSec, TRUE); }
}

void mainWindowImpl::stopTimeoutAnimation(int playerId) {
	assert(playerId >= 0 && playerId < mySession->getCurrentGame()->getStartQuantityPlayers());
	setLabelArray[playerId]->stopTimeOutAnimation();
}

void mainWindowImpl::disableMyButtons() {

	//clear userWidgets
	spinBox_set->setMinimum(0);
	spinBox_set->setValue(0);
	spinBox_set->hide();
	pushButton_BetRaise->show();
	pushButton_BetRaise->setText("");
	pushButton_CallCheckSet->setText("");
	pushButton_FoldAllin->setText("");
}

void mainWindowImpl::myBetRaise() {
	
	if(pushButton_BetRaise->text() == "Raise") { myRaise(); }
	if(pushButton_BetRaise->text() == "Bet") { myBet(); }
}

void mainWindowImpl::myFoldAllin() {
	if(pushButton_FoldAllin->text() == "Fold") { myFold(); }
	if(pushButton_FoldAllin->text() == "All-In") { myAllIn(); }
}

void mainWindowImpl::myCallCheckSet() {
	if(pushButton_CallCheckSet->text() == "Call") { myCall(); }
	if(pushButton_CallCheckSet->text() == "Check") { myCheck(); }
	if(pushButton_CallCheckSet->text() == "Set") { mySet(); }
}


void mainWindowImpl::myFold(){

	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();
	currentHand->getPlayerArray()[0]->setMyAction(1);
	currentHand->getPlayerArray()[0]->setMyTurn(0);

	//set that i was the last active player. need this for unhighlighting groupbox
	currentHand->setLastPlayersTurn(0);
	
	statusBar()->clearMessage();

	//Spiel läuft weiter
	myActionDone();
}

void mainWindowImpl::myCheck() {
	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();
	
	currentHand->getPlayerArray()[0]->setMyTurn(0);
	currentHand->getPlayerArray()[0]->setMyAction(2);

	//set that i was the last active player. need this for unhighlighting groupbox
	currentHand->setLastPlayersTurn(0);

	statusBar()->clearMessage();

	//Spiel läuft weiter
	myActionDone();
}

void mainWindowImpl::myCall(){

	int tempHighestSet = 0;
	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();
	
	tempHighestSet = currentHand->getCurrentBeRo()->getHighestSet();

	if (currentHand->getPlayerArray()[0]->getMyCash()+currentHand->getPlayerArray()[0]->getMySet() <= tempHighestSet) {

		currentHand->getPlayerArray()[0]->setMySet(currentHand->getPlayerArray()[0]->getMyCash());
		currentHand->getPlayerArray()[0]->setMyCash(0);
		currentHand->getPlayerArray()[0]->setMyAction(6);
	}
	else {	
		currentHand->getPlayerArray()[0]->setMySet(tempHighestSet - currentHand->getPlayerArray()[0]->getMySet());
		currentHand->getPlayerArray()[0]->setMyAction(3);
	}
	currentHand->getPlayerArray()[0]->setMyTurn(0);

	currentHand->getBoard()->collectSets();
	refreshPot();

	//set that i was the last active player. need this for unhighlighting groupbox
	currentHand->setLastPlayersTurn(0);

	statusBar()->clearMessage();

	//Spiel läuft weiter
	myActionDone();
}

void mainWindowImpl::myBet(){ 

	pushButton_BetRaise->hide();
	pushButton_CallCheckSet->setText("Set");
	pushButton_FoldAllin->setText("All-In"); 
	spinBox_set->show();

	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();

	if (currentHand->getActualRound() <= 2 ) { spinBox_set->setMinimum(currentHand->getSmallBlind()*2); }
	else { spinBox_set->setMinimum(currentHand->getSmallBlind()*4); }
	
	spinBox_set->setMaximum(currentHand->getPlayerArray()[0]->getMyCash());
	spinBox_set->setValue(spinBox_set->minimum());
	spinBox_set->setFocus();
	spinBox_set->selectAll();

	myActionIsBet = 1;
}

void mainWindowImpl::myRaise(){ 

	pushButton_BetRaise->hide();
	pushButton_CallCheckSet->setText("Set");
	pushButton_FoldAllin->setText("All-In"); 
	spinBox_set->show();

	int tempHighestSet = 0;
	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();

	tempHighestSet = currentHand->getCurrentBeRo()->getHighestSet();

	spinBox_set->setMinimum(tempHighestSet*2 - currentHand->getPlayerArray()[0]->getMySet());
	spinBox_set->setMaximum(currentHand->getPlayerArray()[0]->getMyCash());
	spinBox_set->setValue(spinBox_set->minimum());
	spinBox_set->setFocus();
	spinBox_set->selectAll();
	
	myActionIsRaise = 1;
}

void mainWindowImpl::mySet(){
	
	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();
	int tempCash = currentHand->getPlayerArray()[0]->getMyCash();

// 	cout << "Set-Value " << spinBox_set->value() << endl; 
	currentHand->getPlayerArray()[0]->setMySet(spinBox_set->value());
// 	cout << "MySET " << currentHand->getPlayerArray()[0]->getMySet() << endl;
	if (spinBox_set->value() >= tempCash ) {

		currentHand->getPlayerArray()[0]->setMySet(currentHand->getPlayerArray()[0]->getMyCash());
		currentHand->getPlayerArray()[0]->setMyCash(0);
		currentHand->getPlayerArray()[0]->setMyAction(6);
	}
	
	currentHand->getCurrentBeRo()->setHighestSet(currentHand->getPlayerArray()[0]->getMySet());
	
	if(myActionIsRaise) {
		//do not if allIn
		if(currentHand->getPlayerArray()[0]->getMyAction() != 6) {
			currentHand->getPlayerArray()[0]->setMyAction(5);
		}
		myActionIsRaise = 0;
	}
	
	if(myActionIsBet) {
		//do not if allIn
		if(currentHand->getPlayerArray()[0]->getMyAction() != 6) {
			currentHand->getPlayerArray()[0]->setMyAction(4);
		}		
		myActionIsBet = 0;
	}

	currentHand->getPlayerArray()[0]->setMyTurn(0);

	currentHand->getBoard()->collectSets();
	refreshPot();

	statusBar()->clearMessage();

	//set that i was the last active player. need this for unhighlighting groupbox
	currentHand->setLastPlayersTurn(0);

	//Spiel läuft weiter
	myActionDone();
}

void mainWindowImpl::myAllIn(){

	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();

	currentHand->getPlayerArray()[0]->setMySet(currentHand->getPlayerArray()[0]->getMyCash());
	currentHand->getPlayerArray()[0]->setMyCash(0);
	currentHand->getPlayerArray()[0]->setMyAction(6);
	
	if(currentHand->getPlayerArray()[0]->getMySet() > currentHand->getCurrentBeRo()->getHighestSet()) { currentHand->getCurrentBeRo()->setHighestSet(currentHand->getPlayerArray()[0]->getMySet());}

	currentHand->getPlayerArray()[0]->setMyTurn(0);

	currentHand->getBoard()->collectSets();
	refreshPot();
	
	statusBar()->clearMessage();

	//set that i was the last active player. need this for unhighlighting groupbox
	currentHand->setLastPlayersTurn(0);

	//Spiel läuft weiter
	myActionDone();
}

void mainWindowImpl::myActionDone() {

	// If a network client is running, we need
	// to transfer the action to the server.
	mySession->sendClientPlayerAction();

	// TODO: Should not call in networking game.
	disableMyButtons();

	if (!mySession->isNetworkClientRunning())
		nextPlayerAnimation();

	//prevent escape button working while allIn
	myActionIsRaise = 0;
	myActionIsBet = 0;
}

void mainWindowImpl::nextPlayerAnimation() {

	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();

	//refresh Change Player
	refreshSet();
	refreshAction(currentHand->getLastPlayersTurn(), currentHand->getPlayerArray()[currentHand->getLastPlayersTurn()]->getMyAction());
	refreshCash();

	nextPlayerAnimationTimer->start(nextPlayerSpeed1);
}

void mainWindowImpl::beRoAnimation2(int myBeRoID) {

	switch(myBeRoID) {

		case 0: { preflopAnimation2(); }
		break;
		case 1: { flopAnimation2(); }
		break;
		case 2: { turnAnimation2(); }
		break;
		case 3: { riverAnimation2(); }
		break;
		default: { cout << "beRoAnimation2() Error" << endl; }
	}
}


void mainWindowImpl::preflopAnimation1() { preflopAnimation1Timer->start(nextPlayerSpeed2); }
void mainWindowImpl::preflopAnimation1Action() { mySession->getCurrentGame()->getCurrentHand()->getCurrentBeRo()->run(); }

void mainWindowImpl::preflopAnimation2() { preflopAnimation2Timer->start(preflopNextPlayerSpeed); }
void mainWindowImpl::preflopAnimation2Action() { mySession->getCurrentGame()->getCurrentHand()->getCurrentBeRo()->nextPlayer(); }


void mainWindowImpl::flopAnimation1() { flopAnimation1Timer->start(nextPlayerSpeed2); }
void mainWindowImpl::flopAnimation1Action() { mySession->getCurrentGame()->getCurrentHand()->getCurrentBeRo()->run(); }

void mainWindowImpl::flopAnimation2() { flopAnimation2Timer->start(nextPlayerSpeed3); }
void mainWindowImpl::flopAnimation2Action() { mySession->getCurrentGame()->getCurrentHand()->getCurrentBeRo()->nextPlayer(); }

void mainWindowImpl::turnAnimation1() { turnAnimation1Timer->start(nextPlayerSpeed2); }
void mainWindowImpl::turnAnimation1Action() { mySession->getCurrentGame()->getCurrentHand()->getCurrentBeRo()->run(); }

void mainWindowImpl::turnAnimation2() { turnAnimation2Timer->start(nextPlayerSpeed3); }
void mainWindowImpl::turnAnimation2Action() { mySession->getCurrentGame()->getCurrentHand()->getCurrentBeRo()->nextPlayer(); }

void mainWindowImpl::riverAnimation1() { riverAnimation1Timer->start(nextPlayerSpeed2); }
void mainWindowImpl::riverAnimation1Action() { mySession->getCurrentGame()->getCurrentHand()->getCurrentBeRo()->run(); }

void mainWindowImpl::riverAnimation2() { riverAnimation2Timer->start(nextPlayerSpeed3); }
void mainWindowImpl::riverAnimation2Action() { mySession->getCurrentGame()->getCurrentHand()->getCurrentBeRo()->nextPlayer(); }

void mainWindowImpl::postRiverAnimation1() { postRiverAnimation1Timer->start(nextPlayerSpeed2); }
void mainWindowImpl::postRiverAnimation1Action() { mySession->getCurrentGame()->getCurrentHand()->getCurrentBeRo()->postRiverRun(); }

void mainWindowImpl::postRiverRunAnimation1() {	postRiverRunAnimation1Timer->start(postRiverRunAnimationSpeed); }

void mainWindowImpl::postRiverRunAnimation2() {

	int i;
	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();

	//Aktive Spieler zählen --> wenn nur noch einer nicht-folded dann keine Karten umdrehen
	int activePlayersCounter = 0;
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) { 
		if (currentHand->getPlayerArray()[i]->getMyAction() != 1 && currentHand->getPlayerArray()[i]->getMyActiveStatus()) activePlayersCounter++;
	}

	if(activePlayersCounter!=1) { 
		 
		if(!flipHolecardsAllInAlreadyDone) {

//TODO - Turn cards like in the rules

// 			postRiverRunAnimation2_flipHoleCards1Timer->start(nextPlayerSpeed2);

// 			//Config? mit oder ohne Eye-Candy?
			if(myConfig->readConfigInt("ShowFlipCardsAnimation")) { 
				// mit Eye-Candy
		
				//TempArrays
				int tempCardsIntArray[2];
		
				int i, j;
				
				for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
					currentHand->getPlayerArray()[i]->getMyCards(tempCardsIntArray);	
					if(currentHand->getPlayerArray()[i]->getMyActiveStatus() && currentHand->getPlayerArray()[i]->getMyAction() != 1) { 
						if(i || (i==0 && myConfig->readConfigInt("AntiPeekMode")) ) {
							for(j=0; j<2; j++) {
												
								holeCardsArray[i][j]->startFlipCards(guiGameSpeed, QPixmap(myQtHelper->getDataPath() +"gfx/cards/default/"+QString::number(tempCardsIntArray[j], 10)+".png"), flipside);
							}	
						}
						//set Player value (logging)
						currentHand->getPlayerArray()[i]->setMyCardsFlip(1,1);
					}
				}	
			}
			else {
				//Ohne Eye-Candy		
			
				//Karten der aktiven Spieler umdrehen
				QPixmap onePix(myQtHelper->getDataPath() +"gfx/gui/misc/1px.png");
			
				//TempArrays
				QPixmap tempCardsPixmapArray[2];
				int tempCardsIntArray[2];
			
				int i, j;
				for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
					currentHand->getPlayerArray()[i]->getMyCards(tempCardsIntArray);	
					if(currentHand->getPlayerArray()[i]->getMyActiveStatus() && currentHand->getPlayerArray()[i]->getMyAction() != 1) { 
						if(i || (i==0 && myConfig->readConfigInt("AntiPeekMode")) ) {
							for(j=0; j<2; j++) {		
								tempCardsPixmapArray[j].load(myQtHelper->getDataPath() +"gfx/cards/default/"+QString::number(tempCardsIntArray[j], 10)+".png");
								holeCardsArray[i][j]->setPixmap(tempCardsPixmapArray[j], FALSE);
								
							}	
						}
						//set Player value (logging)
						currentHand->getPlayerArray()[i]->setMyCardsFlip(1,1);
					}
				}
			}
		//Wenn einmal umgedreht dann fertig!!	
		flipHolecardsAllInAlreadyDone = TRUE;
		}
		else {
			int tempCardsIntArray[2];
			int i;
			for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
				currentHand->getPlayerArray()[i]->getMyCards(tempCardsIntArray);	
				if(currentHand->getPlayerArray()[i]->getMyActiveStatus() && currentHand->getPlayerArray()[i]->getMyAction() != 1) { 
				
					//set Player value (logging)
					currentHand->getPlayerArray()[i]->setMyCardsFlip(1,3);
				}
			}	
		}
		postRiverRunAnimation2Timer->start(postRiverRunAnimationSpeed);
	}
	else { postRiverRunAnimation3(); }
}

// TODO
void mainWindowImpl::postRiverRunAnimation2_flipHoleCards1() {

	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();

	currentHand->getCurrentBeRo()->setPlayersTurn(currentHand->getCurrentBeRo()->getLastActionPlayer());

	postRiverRunAnimation2_flipHoleCards2Timer->start(nextPlayerSpeed2);
}


void mainWindowImpl::postRiverRunAnimation2_flipHoleCards2() {

// 	if() {
// 		postRiverRunAnimation2_flipHoleCards1Timer->start(nextPlayerSpeed2);
// 	}
// 	else {
// 		postRiverRunAnimation2Timer->start(postRiverRunAnimationSpeed);
// 	}
}


void mainWindowImpl::postRiverRunAnimation3() {

	int i;
	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();
	//Alle Winner erhellen und "Winner" schreiben
	for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {

		if(currentHand->getPlayerArray()[i]->getMyActiveStatus() && currentHand->getPlayerArray()[i]->getMyAction() != 1 && currentHand->getPlayerArray()[i]->getMyCardsValueInt() == currentHand->getCurrentBeRo()->getHighestCardsValue() ) { 

// 			QPalette tempPalette = groupBoxArray[i]->palette();
// 			tempPalette.setColor(QPalette::Window, highlight);
// 			groupBoxArray[i]->setPalette(tempPalette);
			actionLabelArray[i]->setPixmap(QPixmap(myQtHelper->getDataPath() +"gfx/gui/table/default/action_winner.png"));

			//show winnercards if more than one player is active
			if ( currentHand->getActivePlayersCounter() != 1 && myConfig->readConfigInt("ShowFadeOutCardsAnimation")) {

				int j;
				int bestHandPos[5];
				currentHand->getPlayerArray()[i]->getMyBestHandPosition(bestHandPos);

				//index 0 testen --> Karte darf nicht im MyBestHand Position Array drin sein, es darf nicht nur ein Spieler Aktiv sein, die Config fordert die Animation
				bool index0 = TRUE;
				for(j=0; j<5; j++) {			
	// 				cout <<  (currentHand->getPlayerArray()[i]->getMyBestHandPosition())[j] << endl;
					if (bestHandPos[j] == 0 ) { index0 = FALSE; }
				}
				if (index0) { holeCardsArray[i][0]->startFadeOut(guiGameSpeed); /*cout << "Fade Out index0" << endl;*/}
				//index 1 testen
				bool index1 = TRUE;
				for(j=0; j<5; j++) {
					if (bestHandPos[j] == 1 ) { index1 = FALSE; }
				}
				if (index1) { holeCardsArray[i][1]->startFadeOut(guiGameSpeed); /*cout << "Fade Out index1" << endl;*/}
				//index 2 testen
				bool index2 = TRUE;
				for(j=0; j<5; j++) {
					if (bestHandPos[j] == 2 ) { index2 = FALSE; }
				}
				if (index2) { boardCardsArray[0]->startFadeOut(guiGameSpeed); /*cout << "Fade Out index2" << endl;*/}
				//index 3 testen
				bool index3 = TRUE;
				for(j=0; j<5; j++) {
					if (bestHandPos[j] == 3 ) { index3 = FALSE; }
				}
				if (index3) { boardCardsArray[1]->startFadeOut(guiGameSpeed); /*cout << "Fade Out index3" << endl;*/}
				//index 4 testen
				bool index4 = TRUE;
				for(j=0; j<5; j++) {
					if (bestHandPos[j] == 4 ) { index4 = FALSE; }
				}
				if (index4) { boardCardsArray[2]->startFadeOut(guiGameSpeed); /*cout << "Fade Out index4" << endl;*/}
				//index 5 testen
				bool index5 = TRUE;
				for(j=0; j<5; j++) {
					if (bestHandPos[j] == 5 ) { index5 = FALSE; }
				}
				if (index5) { boardCardsArray[3]->startFadeOut(guiGameSpeed); /*cout << "Fade Out index5" << endl;*/}
				//index 6 testen
				bool index6 = TRUE;
				for(j=0; j<5; j++) {
					if (bestHandPos[j] == 6 ) { index6 = FALSE; }
				}
				if (index6) { boardCardsArray[4]->startFadeOut(guiGameSpeed); /*cout << "Fade Out index6" << endl;*/}
			}	
			//Pot-Verteilung Loggen 
			//Pro Spieler den Cash aus dem Player und dem Label auslesen. Player_cash - Label_cash = Gewinnsumme
			bool toIntBool = TRUE;
			int pot =  currentHand->getPlayerArray()[i]->getMyCash() - cashLabelArray[i]->text().remove(" $").toInt(&toIntBool,10) ;
			//Wenn River dann auch das Blatt loggen!
// 			if (textLabel_handLabel->text() == "River") {

			//set Player value (logging)
			currentHand->getPlayerArray()[i]->setMyWinnerState(1, pot);

// 			}
// 			else {
// 				myLog->logPlayerWinsMsg(i, pot);
// 			}
		}
		else {
			
			if( currentHand->getActivePlayersCounter() != 1 && currentHand->getPlayerArray()[i]->getMyAction() != 1 &&  currentHand->getPlayerArray()[i]->getMyActiveStatus() && myConfig->readConfigInt("ShowFadeOutCardsAnimation") ) {
    	
			//aufgedeckte Gegner auch ausblenden
				holeCardsArray[i][0]->startFadeOut(guiGameSpeed);
				holeCardsArray[i][1]->startFadeOut(guiGameSpeed);
			}
		}
	}
	
	postRiverRunAnimation3Timer->start(postRiverRunAnimationSpeed/2);
}

void mainWindowImpl::postRiverRunAnimation4() {

	distributePotAnimCounter=0;
	potDistributeTimer->start(winnerBlinkSpeed);
}

void mainWindowImpl::postRiverRunAnimation5() {

	int i;
	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();

	//CashTopLabel und PotLabel blinken lassen
	if (distributePotAnimCounter<10) {
		
		if (distributePotAnimCounter==0 || distributePotAnimCounter==2 || distributePotAnimCounter==4 || distributePotAnimCounter==6 || distributePotAnimCounter==8) { 

			label_Pot->setText("");
	
			for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
				if(currentHand->getPlayerArray()[i]->getMyActiveStatus() && currentHand->getPlayerArray()[i]->getMyAction() != 1 && currentHand->getPlayerArray()[i]->getMyCardsValueInt() == currentHand->getCurrentBeRo()->getHighestCardsValue() ) { 

					cashTopLabelArray[i]->setText("");
				}
			}
		}
		else { 
			label_Pot->setText("<span style='font-weight:bold'>Pot</span>");

			for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
				if(currentHand->getPlayerArray()[i]->getMyActiveStatus() && currentHand->getPlayerArray()[i]->getMyAction() != 1 && currentHand->getPlayerArray()[i]->getMyCardsValueInt() == currentHand->getCurrentBeRo()->getHighestCardsValue() ) { 

					cashTopLabelArray[i]->setText("<b>Cash:</b>"); 
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
	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();

	refreshCash();
	refreshPot();

	// TODO HACK
	// Check for network client, do not start new hand if client is running.
	if (mySession->isNetworkClientRunning())
		return;

	// wenn nur noch ein Spieler aktive "neues Spiel"-Dialog anzeigen
	int playersPositiveCashCounter = 0;
	for (i=0; i<mySession->getCurrentGame()->getStartQuantityPlayers(); i++) { 
// 		cout << "player 0 cash: " << currentHand->getPlayerArray()[0]->getMyCash()

		if (currentHand->getPlayerArray()[i]->getMyCash() > 0) 
		playersPositiveCashCounter++;
	}
	if (playersPositiveCashCounter==1) {

// 		for (i=0; i<mySession->getCurrentGame()->getStartQuantityPlayers(); i++) { 
// // 		cout << "player 0 cash: " << currentHand->getPlayerArray()[0]->getMyCash()
// 			if (currentHand->getPlayerArray()[i]->getMyCash() > 0) {
// 				(statisticArray[currentHand->getPlayerArray()[i]->getMyDude4()+7])++;
// 			}
// 		}

// 		for(i=0; i<15; i++) {
// 			cout << i-7 << ": " << statisticArray[i] << " | ";
// 		}
// 		cout << endl;
		
		callNewGameDialog();	
		//Bei Cancel nichts machen!!!
		return;
	} 
	
	postRiverRunAnimation6Timer->start(newRoundSpeed);
}

void mainWindowImpl::flipHolecardsAllIn() {

	int i;
	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();

	if(!flipHolecardsAllInAlreadyDone) {
		//Aktive Spieler zählen --> wenn nur noch einer nicht-folded dann keine Karten umdrehen
		int activePlayersCounter = 0;
		for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) { 
			if (currentHand->getPlayerArray()[i]->getMyAction() != 1 && currentHand->getPlayerArray()[i]->getMyActiveStatus() == 1) activePlayersCounter++;
		}
		
		if(activePlayersCounter!=1) { 
			
			//Config? mit oder ohne Eye-Candy?
			if(myConfig->readConfigInt("ShowFlipCardsAnimation")) { 
				// mit Eye-Candy
	
				//TempArrays
				int tempCardsIntArray[2];
	
				int i, j;
	
				for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
					currentHand->getPlayerArray()[i]->getMyCards(tempCardsIntArray);	
					if(currentHand->getPlayerArray()[i]->getMyActiveStatus() && currentHand->getPlayerArray()[i]->getMyAction() != 1) { 
						if(i || (i==0 && myConfig->readConfigInt("AntiPeekMode")) ) {
							for(j=0; j<2; j++) {
								holeCardsArray[i][j]->startFlipCards(guiGameSpeed, QPixmap(myQtHelper->getDataPath() +"gfx/cards/default/"+QString::number(tempCardsIntArray[j], 10)+".png"), flipside);
							}
						}
						//set Player value (logging)
						currentHand->getPlayerArray()[i]->setMyCardsFlip(1,2);
						
					}
				}
			}
			else {
				//Ohne Eye-Candy		
		
				//Karten der aktiven Spieler umdrehen
				QPixmap onePix(myQtHelper->getDataPath() +"gfx/gui/misc/1px.png");
				
				//TempArrays
				QPixmap tempCardsPixmapArray[2];
				int temp2CardsIntArray[2];
				
				int i, j;
				for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
					currentHand->getPlayerArray()[i]->getMyCards(temp2CardsIntArray);	
					if(currentHand->getPlayerArray()[i]->getMyActiveStatus() && currentHand->getPlayerArray()[i]->getMyAction() != 1) { 
						if(i || (i==0 && myConfig->readConfigInt("AntiPeekMode")) ) {
							for(j=0; j<2; j++) {
								
								tempCardsPixmapArray[j].load(myQtHelper->getDataPath() +"gfx/cards/default/"+QString::number(temp2CardsIntArray[j], 10)+".png");
								holeCardsArray[i][j]->setPixmap(tempCardsPixmapArray[j], FALSE);
							}	
						}
						//set Player value (logging)
						currentHand->getPlayerArray()[i]->setMyCardsFlip(1,2);
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
	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();

	currentHand->getPlayerArray()[0]->getMyCards(tempCardsIntArray);	
	if( currentHand->getPlayerArray()[0]->getMyCardsFlip() == 0 &&  currentHand->getActualRound() == 4 && currentHand->getPlayerArray()[0]->getMyActiveStatus() && currentHand->getPlayerArray()[0]->getMyAction() != 1) { 

		//set Player value (logging)	
		currentHand->getPlayerArray()[0]->setMyCardsFlip(1,1);
	}
}


void mainWindowImpl::startNewHand() {

	if( !breakAfterActualHand){
		mySession->getCurrentGame()->initHand();
		mySession->getCurrentGame()->startHand();
	}
	else { 
		pushButton_break->setDisabled(FALSE);
		
		QFontMetrics tempMetrics = this->fontMetrics();
		int width = tempMetrics.width(tr("Start"));
		pushButton_break->setMinimumSize(width+10,20);

		pushButton_break->setText(tr("Start"));
		breakAfterActualHand=FALSE;

		blinkingStartButtonAnimationTimer->start(500);		
	}
}

void mainWindowImpl::handSwitchRounds() { mySession->getCurrentGame()->getCurrentHand()->switchRounds(); }

void mainWindowImpl::nextRoundCleanGui() {

	int i,j;

	// GUI bereinigen - Bilder löschen, Animationen unterbrechen
	QPixmap onePix(myQtHelper->getDataPath() +"gfx/gui/misc/1px.png");
	for (i=0; i<5; i++ ) { 
		boardCardsArray[i]->setPixmap(onePix, FALSE); 
		boardCardsArray[i]->setFadeOutAction(FALSE); 
		boardCardsArray[i]->stopFlipCardsAnimation();
		setLabelArray[i]->stopTimeOutAnimation();
		
	}
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++ ) { 
		for ( j=0; j<=1; j++ ) { holeCardsArray[i][j]->setFadeOutAction(FALSE);}
	}
		
// 	QPalette labelPalette = label_Pot->palette();
// 	labelPalette.setColor(QPalette::WindowText, c);
// 	label_Pot->setPalette(labelPalette);

	// for startNewGame during human player is active
	if(mySession->getCurrentGame()->getCurrentHand()->getPlayerArray()[0]->getMyActiveStatus() == 1) {
		disableMyButtons();
	}
// 	QPalette tempPalette = frame_Pot->palette();
// 	tempPalette.setColor(QPalette::Window, active);
// 	frame_Pot->setPalette(tempPalette);

	textLabel_handLabel->setText("");
	
// 	tempPalette = frame_handLabel->palette();
// 	tempPalette.setColor(QPalette::Window, active);
// 	frame_handLabel->setPalette(tempPalette);

	refreshAll();

	flipHolecardsAllInAlreadyDone = FALSE;

	//Wenn Pause zwischen den Hands in der Konfiguration steht den Stop Button drücken!
	if (myConfig->readConfigInt("PauseBetweenHands") && blinkingStartButtonAnimationTimer->isActive() == FALSE ) { 
		pushButton_break->click(); 
	}
	else { 
		//FIX STRG+N Bug
		pushButton_break->setEnabled(TRUE); 
		breakAfterActualHand=FALSE;
	}
	
	//Clean breakbutton
	blinkingStartButtonAnimationTimer->stop();
	QPalette tempPalette = pushButton_break->palette();
	tempPalette.setColor(QPalette::Button, QColor(40,82,0));
	tempPalette.setColor(QPalette::ButtonText, QColor(240,240,240));
	pushButton_break->setPalette(tempPalette);
	blinkingStartButtonAnimationTimer->stop();
	QFontMetrics tempMetrics = this->fontMetrics();
	int width = tempMetrics.width(tr("Stop"));
	pushButton_break->setMinimumSize(width+10,20);
       	pushButton_break->setText(tr("Stop"));
	
	//Clear Statusbarmessage
	statusBar()->clearMessage();

	//fix press mouse button during bankrupt with anti-peek-mode
	this->mouseOverFlipCards(FALSE);
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

void mainWindowImpl::setSpeeds() {

	gameSpeed = (11-guiGameSpeed)*10;
	dealCardsSpeed = (gameSpeed/2)*10; //milliseconds
	preDealCardsSpeed = dealCardsSpeed*2; //Zeit for Karten aufdecken auf dem Board (Flop, Turn, River)
	postDealCardsSpeed = dealCardsSpeed*3; //Zeit nach Karten aufdecken auf dem Board (Flop, Turn, River)
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

	if (pushButton_break->text() == tr("Stop")) {
		pushButton_break->setDisabled(TRUE);
		breakAfterActualHand=TRUE;
	}
	else { 
		blinkingStartButtonAnimationTimer->stop();
		//Set default Color
		QPalette tempPalette = pushButton_break->palette();
		tempPalette.setColor(QPalette::Button, QColor(40,82,0));
		tempPalette.setColor(QPalette::ButtonText, QColor(240,240,240));
		pushButton_break->setPalette(tempPalette);

		QFontMetrics tempMetrics = this->fontMetrics();
		int width = tempMetrics.width(tr("Stop"));
		pushButton_break->setMinimumSize(width+10,20);

		pushButton_break->setText(tr("Stop"));
		startNewHand();
	}
}

void mainWindowImpl::paintStartSplash() {

	StartSplash *mySplash = new StartSplash(this);	

#ifdef __APPLE__
  int offset = 305;
#else
  int offset = 237;
#endif
        mySplash->setGeometry(this->pos().x()+offset,this->pos().y()+210,400,250);
//         mySplash->setWindowFlags(Qt::SplashScreen);
        mySplash->show();
}


void mainWindowImpl::networkError(int errorID, int osErrorID) {

	switch (errorID) {

		case ERR_SOCK_SERVERADDR_NOT_SET:
			{QMessageBox::warning(this, tr("Network Error"),
				tr("Server address was not set."),
				QMessageBox::Close); }
		break;
		case ERR_SOCK_INVALID_PORT:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("An invalid port was set (ports 0-1023 are not allowed)."),
				QMessageBox::Close); }
		break;
		case ERR_SOCK_CREATION_FAILED:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("Could not create a socket for TCP communication."),
				QMessageBox::Close); }
		break;
		case ERR_SOCK_SET_ADDR_FAILED:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("Could not set the IP address."),
				QMessageBox::Close); }
		break;
		case ERR_SOCK_SET_PORT_FAILED:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("Could not set the port for this type of address."),
				QMessageBox::Close); }
		break;
		case ERR_SOCK_RESOLVE_FAILED:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("The server name could not be resolved."),
				QMessageBox::Close); }
		break;
		case ERR_SOCK_BIND_FAILED:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("Bind failed - please choose a different port."),
				QMessageBox::Close); }
		break;
		case ERR_SOCK_LISTEN_FAILED:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("Internal network error: \"listen\" failed."),
				QMessageBox::Close); }
		break;
		case ERR_SOCK_ACCEPT_FAILED:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("Server execution was terminated."),
				QMessageBox::Close); }
		break;
		case ERR_SOCK_CONNECT_FAILED:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("Could not connect to the server."),
				QMessageBox::Close); }
		break;
		case ERR_SOCK_CONNECT_TIMEOUT:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("Connection timed out.\nPlease check the server address.\n\nIf the server is behind a NAT-Router, make sure port forwarding has been set up on server side."),
				QMessageBox::Close); }
		break;
		case ERR_SOCK_SELECT_FAILED:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("Internal network error: \"select\" failed."),
				QMessageBox::Close); }
		break;
		case ERR_SOCK_SEND_FAILED:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("Internal network error: \"send\" failed."),
				QMessageBox::Close); }
		break;
		case ERR_SOCK_RECV_FAILED: // Sometimes windows reports recv failed on close.
		case ERR_SOCK_CONN_RESET:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("Connection was closed by the server."),
				QMessageBox::Close); }
		break;
		case ERR_SOCK_CONN_EXISTS:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("Internal network error: Duplicate TCP connection."),
				QMessageBox::Close); }
		break;
		case ERR_SOCK_INVALID_PACKET:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("An invalid network packet was received.\nPlease make sure that all players use the same version of PokerTH."),
				QMessageBox::Close); }
		break;
		case ERR_SOCK_INVALID_STATE:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("Internal state error.\nPlease make sure that all players use the same version of PokerTH."),
				QMessageBox::Close); }
		break;
		case ERR_NET_VERSION_NOT_SUPPORTED:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("The PokerTH server does not support this version of the game.\nPlease update PokerTH."),
				QMessageBox::Close); }
		break;
		case ERR_NET_SERVER_FULL:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("Sorry, this server is already full."),
				QMessageBox::Close); }
		break;
		case ERR_NET_GAME_ALREADY_RUNNING:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("Unable to join - the server has already started the game."),
				QMessageBox::Close); }
		break;
		case ERR_NET_INVALID_PASSWORD:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("Invalid password when joining the game.\nPlease reenter the password and try again."),
				QMessageBox::Close); }
		break;
		case ERR_NET_INVALID_PASSWORD_STR:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("The password is too long. Please choose another one."),
				QMessageBox::Close); }
		break;
		case ERR_NET_PLAYER_NAME_IN_USE:
			{ myChangeHumanPlayerNameDialog->label_Message->setText(tr("Your player name is already used by another player.\nPlease choose a different name."));
			  myChangeHumanPlayerNameDialog->exec(); }
		break;
		case ERR_NET_INVALID_PLAYER_NAME:
			{ myChangeHumanPlayerNameDialog->label_Message->setText(tr("The player name is too short, too long or reserved. Please choose another one."));
			  myChangeHumanPlayerNameDialog->exec(); }
		break;
		case ERR_NET_INVALID_GAME_NAME:
			{ myChangeHumanPlayerNameDialog->label_Message->setText(tr("The game name is either too short or too long. Please choose another one."));
			  myChangeHumanPlayerNameDialog->exec(); }
		break;
		case ERR_NET_UNKNOWN_GAME:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("The game could not be found."),
				QMessageBox::Close); }
		break;
		case ERR_NET_INVALID_CHAT_TEXT:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("The chat text is invalid."),
				QMessageBox::Close); }
		break;
		case ERR_NET_UNKNOWN_PLAYER_ID:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("The server referred to an unknown player. Aborting."),
				QMessageBox::Close); }
		break;
		case ERR_NET_PLAYER_KICKED:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("You were kicked from the server."),
				QMessageBox::Close); }
		break;
		case ERR_NET_INVALID_PLAYER_COUNT:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("The client player count is invalid."),
				QMessageBox::Close); }
		break;
		default:  { QMessageBox::warning(this, tr("Network Error"),
				tr("An internal error occured."),
				QMessageBox::Close); }
	}
	// close dialogs
	myGameLobbyDialog->reject();
	myConnectToServerDialog->reject();
	myStartNetworkGameDialog->reject();
}

void mainWindowImpl::networkStart(boost::shared_ptr<Game> game)
{
	mySession->startClientGame(game);
}

void mainWindowImpl::keyPressEvent ( QKeyEvent * event ) {

// 	cout << event->key() << endl;
	
	bool ctrlPressed = FALSE;

	if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return ) { if(spinBox_set->hasFocus()) pushButton_CallCheckSet->click(); } //ENTER 
	if (event->key() == Qt::Key_F1) { pushButton_FoldAllin->click(); } 
	if (event->key() == Qt::Key_F2) { pushButton_CallCheckSet->click(); } 
	if (event->key() == Qt::Key_F3) { pushButton_BetRaise->click(); } 
// 	if (event->key() == Qt::Key_S) { setLabelArray[0]->startTimeOutAnimation(myConfig->readConfigInt("NetTimeOutPlayerAction"),TRUE); } //s	
	if (event->key() == 16777249) { 
		pushButton_break->click(); 
		ctrlPressed = TRUE;
// 		QTimer::SingleShot
	} //CTRL
// 	if (event->key() == 65) {  pixmapLabel_card0a->setUpdatesEnabled(FALSE); }     
// 	if (event->key() == 66) {  label_logo->hide();	}

	if (event->key() == Qt::Key_Escape && (myActionIsBet || myActionIsRaise)) { 
		meInAction(); 
	} 
}

// bool mainWindowImpl::event ( QEvent * event )  { 

// 	if(event->type() == QEvent::MouseMove) { event->setAccepted ( FALSE );  }
// }

void mainWindowImpl::switchChatWindow() {

	int tab = 1;
	if (groupBox_LeftToolBox->isHidden()) { 
		tabWidget_Left->setCurrentIndex(tab);
		groupBox_LeftToolBox->show(); 
	}	else {
		if (tabWidget_Left->currentIndex() == tab) {
			groupBox_LeftToolBox->hide(); 				
		} else {
			tabWidget_Left->setCurrentIndex(tab);			
		}
	}
}

void mainWindowImpl::switchHelpWindow() {

	int tab = 0;
	if (groupBox_LeftToolBox->isHidden()) { 
		tabWidget_Left->setCurrentIndex(tab);
		groupBox_LeftToolBox->show(); 
	}	else {
		if (tabWidget_Left->currentIndex() == tab) {
			groupBox_LeftToolBox->hide(); 				
		} else {
			tabWidget_Left->setCurrentIndex(tab);			
		}
	}
}

void mainWindowImpl::switchLogWindow() {

	if (groupBox_RightToolBox->isHidden()) { 
		groupBox_RightToolBox->show(); 
	}	else { 	
		groupBox_RightToolBox->hide(); 
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

void mainWindowImpl::blinkingStartButtonAnimationAction() {
	
	QPalette tempPalette = pushButton_break->palette();

	if(tempPalette.color(QPalette::Button).red()==40) {
		tempPalette.setColor(QPalette::Button, QColor(113,162,0));
		tempPalette.setColor(QPalette::ButtonText, QColor(0,0,0));
	}
	else {
		tempPalette.setColor(QPalette::Button, QColor(40,82,0));
		tempPalette.setColor(QPalette::ButtonText, QColor(240,240,240));
	}
	pushButton_break->setPalette(tempPalette);
}

void mainWindowImpl::sendChatMessage() { myChat->sendMessage(); }
void mainWindowImpl::checkChatInputLength(QString string) { myChat->checkInputLength(string); }


void mainWindowImpl::tabSwitchAction() { 
	
	switch(tabWidget_Left->currentIndex()) {

		case 1: { lineEdit_ChatInput->setFocus();
			  myChat->checkInvisible();				
			}
		break;
		default: { lineEdit_ChatInput->clearFocus(); }
	
	}
}


void mainWindowImpl::localGameModification() {
	
	tabWidget_Left->setCurrentIndex(0);
	tabWidget_Left->disableTab(1, TRUE);
	
	int i;
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++ ) { 
		setLabelArray[i]->stopTimeOutAnimation();
	}
}

void mainWindowImpl::networkGameModification() {
	
	tabWidget_Left->disableTab(1, FALSE);	
	tabWidget_Left->setCurrentIndex(1);
	myChat->clearNewGame();

}

void mainWindowImpl::mouseOverFlipCards(bool front) {

	if(mySession->getCurrentGameID()) {
		if(myConfig->readConfigInt("AntiPeekMode") && mySession->getCurrentGame()->getCurrentHand()->getPlayerArray()[0]->getMyActiveStatus() && mySession->getCurrentGame()->getPlayerArray()[0]->getMyAction() != PLAYER_ACTION_FOLD) {
			holeCardsArray[0][0]->signalFastFlipCards(front);
			holeCardsArray[0][1]->signalFastFlipCards(front);
		}
	}
}

void mainWindowImpl::closeEvent(QCloseEvent *event) { quitPokerTH(); }

void mainWindowImpl::quitPokerTH() {

	mySession->terminateNetworkClient();
	if (myServerGuiInterface.get()) myServerGuiInterface->getSession().terminateNetworkServer();


// 	cout << "PokerTH finished" << endl;
	qApp->quit();
}

