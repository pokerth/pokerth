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
#include "mymessagedialogimpl.h"
#include "settingsdialogimpl.h"
#include "selectavatardialogimpl.h"
#include "joinnetworkgamedialogimpl.h"
#include "connecttoserverdialogimpl.h"
#include "createnetworkgamedialogimpl.h"
#include "startnetworkgamedialogimpl.h"
#include "changehumanplayernamedialogimpl.h"
#include "changecompleteblindsdialogimpl.h"
#include "gamelobbydialogimpl.h"
#include "lobbychat.h"

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

#include "configfile.h"
#include "sdlplayer.h"

#include <gamedata.h>
#include <generic/serverguiwrapper.h>

#include <net/socket_msg.h>

#include "math.h"

#define FORMATLEFT(X) "<p align='center'>(X)"
#define FORMATRIGHT(X) "(X)</p>"

using namespace std;

mainWindowImpl::mainWindowImpl(ConfigFile *c, QMainWindow *parent)
     : QMainWindow(parent), myChat(NULL), myConfig(c), gameSpeed(0), myActionIsBet(0), myActionIsRaise(0), pushButtonBetRaiseIsChecked(FALSE), pushButtonCallCheckIsChecked(FALSE), pushButtonFoldIsChecked(FALSE), pushButtonAllInIsChecked(FALSE), myButtonsAreCheckable(FALSE), breakAfterCurrentHand(FALSE), currentGameOver(FALSE), betSliderChangedByInput(FALSE), myLastPreActionBetValue(0)
{
	int i;

//	this->setStyle(new QPlastiqueStyle);

	//for statistic development
	for(i=0; i<15; i++) {
		statisticArray[i] = 0;
	}
	////////////////////////////

	myAppDataPath = QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str());

	setupUi(this);

	//Player0 pixmapCardsLabel needs Myw
	pixmapLabel_card0b->setMyW(this);
	pixmapLabel_card0a->setMyW(this);

	//Flipside festlegen;
	if (myConfig->readConfigInt("FlipsideOwn") && myConfig->readConfigString("FlipsideOwnFile") != "") {
		flipside = new QPixmap(QString::fromUtf8(myConfig->readConfigString("FlipsideOwnFile").c_str()));
	}
	else { 
		flipside = new QPixmap(myAppDataPath +"gfx/cards/default/flipside.png");
	}

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
	userWidgetsArray[1] = pushButton_CallCheck;
	userWidgetsArray[2] = pushButton_Fold;
	userWidgetsArray[3] = lineEdit_betValue;
	userWidgetsArray[4] = horizontalSlider_bet;
	userWidgetsArray[5] = pushButton_AllIn;

	//hide userWidgets
	for(i=0; i<6; i++) {
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



// 	Schriftart laden und für Dialoge setzen
#ifdef _WIN32
	font1String = "font-family: \"Arial\";";
	font2String = "font-family: \"Bitstream Vera Sans\";";
	QString fontsize= "11";
#else 
	#ifdef __APPLE__	
		font1String = "font-family: \"Lucida Grande\";";
		font2String = "font-family: \"Lucida Grande\";";
		QString fontsize= "10";

	#else 
		font1String = "font-family: \"Nimbus Sans L\";";
		font2String = "font-family: \"Bitstream Vera Sans\";";
		QString fontsize= "10";

	#endif
#endif


	textBrowser_Log->setStyleSheet("QTextBrowser { "+ font1String +" font-size: "+fontsize+"px; color: #F0F0F0; background-color: #1D3B00; border:none; } QScrollBar:vertical { border: 1px solid #104600; background: #135000; width: 15px; margin: 17px -1px 17px 0px; } QScrollBar::handle:vertical { border-radius: 1px; border: 1px solid #1B7200; background: #176400; min-height: 20px; } QScrollBar::add-line:vertical { margin-right: 0px; margin-left: 1px; border-bottom-right-radius: 2px; border-bottom-left-radius: 2px; border-top-right-radius: 1px; border-top-left-radius: 1px; border: 1px solid #1E7F00; background: #1A6F00; height: 15px; subcontrol-position: bottom; subcontrol-origin: margin; } QScrollBar::sub-line:vertical { margin-right: 0px; margin-left: 1px; border-bottom-right-radius: 1px; border-bottom-left-radius: 1px; border-top-right-radius: 2px; border-top-left-radius: 2px; border: 1px solid #1E7F00; background: #1A6F00; height: 15px; subcontrol-position: top; subcontrol-origin: margin; } QScrollBar:up-arrow:vertical, QScrollBar::down-arrow:vertical { border: 1px solid #208A00; height: 3px; width: 3px; background: #27A800; } QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }");
	textBrowser_Chat->setStyleSheet("QTextBrowser { "+ font1String +" font-size: "+fontsize+"px; color: #F0F0F0; background-color: #1D3B00; border:none; } QScrollBar:vertical { border: 1px solid #104600; background: #135000; width: 15px; margin: 17px -1px 17px 0px; } QScrollBar::handle:vertical { border-radius: 1px; border: 1px solid #1B7200; background: #176400; min-height: 20px; } QScrollBar::add-line:vertical { margin-right: 0px; margin-left: 1px; border-bottom-right-radius: 2px; border-bottom-left-radius: 2px; border-top-right-radius: 1px; border-top-left-radius: 1px; border: 1px solid #1E7F00; background: #1A6F00; height: 15px; subcontrol-position: bottom; subcontrol-origin: margin; } QScrollBar::sub-line:vertical { margin-right: 0px; margin-left: 1px; border-bottom-right-radius: 1px; border-bottom-left-radius: 1px; border-top-right-radius: 2px; border-top-left-radius: 2px; border: 1px solid #1E7F00; background: #1A6F00; height: 15px; subcontrol-position: top; subcontrol-origin: margin; } QScrollBar:up-arrow:vertical, QScrollBar::down-arrow:vertical { border: 1px solid #208A00; height: 3px; width: 3px; background: #27A800; } QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }");
	lineEdit_ChatInput->setStyleSheet("QLineEdit { "+ font1String +" font-size: "+fontsize+"px; color: #F0F0F0; background-color: #1D3B00; border-top: 2px solid #286400; }");


#ifdef __APPLE__
// 	tabWidget_Right->setStyleSheet("QTabWidget { "+ font1String +" font-size: 11px; background-color: #145300; }");
// 	tabWidget_Left->setStyleSheet("QTabWidget { "+ font1String +" font-size: 11px; background-color: #145300;}");
#else
// 	tabWidget_Right->setStyleSheet("QTabWidget::pane { "+ font1String +" font-size: 10px; background-color: #145300; border: 1px solid #286400; border-radius: 2px; }");
// 	tabWidget_Left->setStyleSheet("QTabWidget::pane { border: 1px solid #286400; border-radius: 2px; background-color: #145300; }");
#endif

	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {

		cashTopLabelArray[i]->setStyleSheet("QLabel { "+ font2String +" font-size: 10px; font-weight: bold; color: #F0F0F0; }");
		cashLabelArray[i]->setStyleSheet("QLabel { "+ font2String +" font-size: 10px; font-weight: bold; color: #F0F0F0; }");
	}
	
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {

		setLabelArray[i]->setStyleSheet("QLabel { "+ font2String +" font-size: 12px; font-weight: bold; color: #F0F0F0; }");
	}

	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {

		playerNameLabelArray[i]->setStyleSheet("QLabel { "+ font2String +" font-size: 13px; font-weight: bold; color: #F0F0F0; }");
	}

	label_Sets->setStyleSheet("QLabel { "+ font2String +" font-size: 13px; font-weight: bold; color: #669900;  }");
	label_Total->setStyleSheet("QLabel { "+ font2String +" font-size: 13px; font-weight: bold; color: #669900; }");
	textLabel_Sets->setStyleSheet("QLabel { "+ font2String +" font-size: 13px; font-weight: bold; color: #669900;  }");
	textLabel_Pot->setStyleSheet("QLabel { "+ font2String +" font-size: 13px; font-weight: bold; color: #669900;  }");
	label_handNumber->setStyleSheet("QLabel { "+ font2String +" font-size: 13px; font-weight: bold; color: #669900;  }");
	label_gameNumber->setStyleSheet("QLabel { "+ font2String +" font-size: 13px; font-weight: bold; color: #669900;  }");
	label_handNumberValue->setStyleSheet("QLabel { "+ font2String +" font-size: 13px; font-weight: bold; color: #669900;  }");
	label_gameNumberValue->setStyleSheet("QLabel { "+ font2String +" font-size: 13px; font-weight: bold; color: #669900;  }");

	textLabel_handLabel->setStyleSheet("QLabel { "+ font2String +" font-size: 17px; font-weight: bold; color: #669900;  }");

	label_Pot->setStyleSheet("QLabel { "+ font2String +" font-size: 18px; font-weight: bold; color: #669900;   }");


	//Widgets Grafiken setzen
	label_CardHolder0->setPixmap(myAppDataPath + "gfx/gui/table/default/cardholder_flop.png");
	label_CardHolder1->setPixmap(myAppDataPath + "gfx/gui/table/default/cardholder_flop.png");
	label_CardHolder2->setPixmap(myAppDataPath + "gfx/gui/table/default/cardholder_flop.png");
	label_CardHolder3->setPixmap(myAppDataPath + "gfx/gui/table/default/cardholder_turn.png");
	label_CardHolder4->setPixmap(myAppDataPath + "gfx/gui/table/default/cardholder_river.png");

	label_Handranking->setPixmap(myAppDataPath + "gfx/gui/misc/handRanking.png");

	//Widgets Grafiken per Stylesheets setzen
	this->setStyleSheet("QMainWindow { background-image: url(" + myAppDataPath +"gfx/gui/table/default/table.png); background-position: bottom center; background-origin: content;}");

	menubar->setStyleSheet("QMenuBar { background-color: #145300; } QMenuBar::item { color: #669800; }");

	pushButton_break->setStyleSheet("QPushButton:enabled { background-color: #145300; color: white;} QPushButton:disabled { background-color: #145300; color: #486F3E; font-weight: 900;}");

	statusbar->setStyleSheet(" QStatusBar { "+ font1String +" font-size: 12px; color: white; }");

	//Groupbox Background 
	for (i=1; i<MAX_NUMBER_OF_PLAYERS; i++) {

		groupBoxArray[i]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/opponentBoxInactiveGlow.png) }"); 
	}
	groupBoxArray[0]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/playerBoxInactiveGlow_0.6.png) }"); 

	//Human player button
	pushButton_BetRaise->setStyleSheet("QPushButton { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_03_0.6.png); "+ font2String +" font-size: 11px; font-weight: bold; color: #F0F0F0;} QPushButton:unchecked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_03_0.6.png); } QPushButton:checked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_03_0.6_checked.png); }");
	pushButton_CallCheck->setStyleSheet("QPushButton { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_05_0.6.png); "+ font2String +" font-size: 11px; font-weight: bold; color: #F0F0F0;} QPushButton:unchecked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_05_0.6.png); } QPushButton:checked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_05_0.6_checked.png); }");
	pushButton_Fold->setStyleSheet("QPushButton { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_07_0.6.png); "+ font2String +" font-size: 11px; font-weight: bold; color: #F0F0F0;}  QPushButton:unchecked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_07_0.6.png); } QPushButton:checked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_07_0.6_checked.png); }");

	lineEdit_betValue->setStyleSheet("QLineEdit { "+ font2String +" font-size: 10px; font-weight: bold; background-color: #1D3B00; color: #F0F0F0; } QLineEdit:disabled { background-color: #316300; color: #6d7b5f }");

	pushButton_AllIn->setStyleSheet("QPushButton:enabled { background-color: #145300; color: white;} QPushButton:disabled { background-color: #145300; color: #486F3E; font-weight: 900;}");

// 	away radiobuttons
	QString radioButtonString("QRadioButton { color: #F0F0F0; } QRadioButton::indicator { width: 13px; height: 13px; } QRadioButton::indicator::checked { image: url("+myAppDataPath+"gfx/gui/misc/radiobutton_checked.png); }");

	radioButton_manualAction->setStyleSheet(radioButtonString);
	radioButton_autoCheckFold->setStyleSheet(radioButtonString);
	radioButton_autoCheckCallAny->setStyleSheet(radioButtonString);

	groupBox_RightToolBox->setStyleSheet("QGroupBox { border:none; background-image: url(" + myAppDataPath +"gfx/gui/misc/toolboxFrameBG.png) }");
	groupBox_LeftToolBox->setStyleSheet("QGroupBox { border:none; background-image: url(" + myAppDataPath +"gfx/gui/misc/toolboxFrameBG.png) }");


	//raise actionLable above just inserted mypixmaplabel
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) { actionLabelArray[i]->raise(); }

	//fix for away string bug in righttabwidget on windows
#ifdef _WIN32
	tabWidget_Right->setTabText(0, " "+tabWidget_Right->tabText(0)+" ");
	tabWidget_Right->setTabText(1, " "+tabWidget_Right->tabText(1)+" ");
#endif

	//resize stop-button depending on translation
	QFontMetrics tempMetrics = this->fontMetrics();
	int width = tempMetrics.width(tr("Stop"));
	pushButton_break->setMinimumSize(width+10,20);

	//set inputvalidator for lineeditbetvalue
	QRegExp rx("[1-9]\\d{0,4}");
 	QValidator *validator = new QRegExpValidator(rx, this);
 	lineEdit_betValue->setValidator(validator);

	//Clear Focus
	groupBox_LeftToolBox->clearFocus();
	groupBox_RightToolBox->clearFocus();

	//set Focus to mainwindow
	this->setFocus();

	//windowicon
// 	QString windowIconString();
	this->setWindowIcon(QIcon(myAppDataPath+"gfx/gui/misc/windowicon.png")); 

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
	myAboutPokerthDialog = new aboutPokerthImpl(this, myConfig);
	myGameLobbyDialog = new gameLobbyDialogImpl(this, myConfig);

	myStartNetworkGameDialog->setMyW(this);
	myGameLobbyDialog->setMyW(this);

	myChat = new Chat(this, myConfig);

	lineEdit_ChatInput->installEventFilter(this);
	this->installEventFilter(this);

	//set WindowTitle dynamically
	this->setWindowTitle(QString(tr("PokerTH %1 - The Open-Source Texas Holdem Engine").arg(POKERTH_BETA_RELEASE_STRING)));

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
	connect( actionShowHideAway, SIGNAL( triggered() ), this, SLOT( switchAwayWindow() ) );


	connect( pushButton_BetRaise, SIGNAL( clicked(bool) ), this, SLOT( pushButtonBetRaiseClicked(bool) ) );
	connect( pushButton_Fold, SIGNAL( clicked(bool) ), this, SLOT( pushButtonFoldClicked(bool) ) );
	connect( pushButton_CallCheck, SIGNAL( clicked(bool) ), this, SLOT( pushButtonCallCheckClicked(bool) ) );
	connect( pushButton_AllIn, SIGNAL( clicked(bool) ), this, SLOT(pushButtonAllInClicked(bool) ) );
	connect( horizontalSlider_bet, SIGNAL( valueChanged(int)), this, SLOT ( changeLineEditBetValue(int) ) );
	connect( lineEdit_betValue, SIGNAL( textChanged(QString)), this, SLOT ( lineEditBetValueChanged(QString) ) );
	
	connect( horizontalSlider_speed, SIGNAL( valueChanged(int)), this, SLOT ( setGameSpeed(int) ) );
	connect( pushButton_break, SIGNAL( clicked()), this, SLOT ( breakButtonClicked() ) ); // auch wieder starten!!!!

	connect( tabWidget_Left, SIGNAL( currentChanged(int) ), this, SLOT( tabSwitchAction() ) );
	connect( lineEdit_ChatInput, SIGNAL( returnPressed () ), this, SLOT( sendChatMessage() ) );
	connect( lineEdit_ChatInput, SIGNAL( textChanged (QString) ), this, SLOT( checkChatInputLength(QString) ) );
	connect( lineEdit_ChatInput, SIGNAL( textEdited (QString) ), myChat, SLOT( setChatTextEdited() ) );

	connect( radioButton_manualAction, SIGNAL( clicked() ) , this, SLOT( changePlayingMode() ) );
	connect( radioButton_autoCheckFold, SIGNAL( clicked() ) , this, SLOT( changePlayingMode() ) );
	connect( radioButton_autoCheckCallAny, SIGNAL( clicked() ), this, SLOT( changePlayingMode() ) );

	//Nachrichten Thread-Save
	connect(this, SIGNAL(signalInitGui(int)), this, SLOT(initGui(int)));

	connect(this, SIGNAL(signalShowClientDialog()), this, SLOT(showClientDialog()));

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
	connect(this, SIGNAL(signalSetPlayerAvatar(int, QString)), this, SLOT(setPlayerAvatar(int, QString))); 
	connect(this, SIGNAL(signalSetPlayerAvatar(int, QString)), myGameLobbyDialog, SLOT(refreshConnectedPlayerAvatars())); 

	connect(this, SIGNAL(signalGuiUpdateDone()), this, SLOT(guiUpdateDone()));

	connect(this, SIGNAL(signalMeInAction()), this, SLOT(meInAction()));
	connect(this, SIGNAL(signalDisableMyButtons()), this, SLOT(disableMyButtons()));
	connect(this, SIGNAL(signalUpdateMyButtonsState()), this, SLOT(updateMyButtonsState()));
	connect(this, SIGNAL(signalStartTimeoutAnimation(int, int)), this, SLOT(startTimeoutAnimation(int, int)));
	connect(this, SIGNAL(signalStopTimeoutAnimation(int)), this, SLOT(stopTimeoutAnimation(int)));

	connect(this, SIGNAL(signalDealBeRoCards(int)), this, SLOT(dealBeRoCards(int)));
	connect(this, SIGNAL(signalDealHoleCards()), this, SLOT(dealHoleCards()));
	connect(this, SIGNAL(signalDealFlopCards0()), this, SLOT(dealFlopCards0()));
	connect(this, SIGNAL(signalDealTurnCards0()), this, SLOT(dealTurnCards0()));
	connect(this, SIGNAL(signalDealRiverCards0()), this, SLOT(dealRiverCards0()));

	connect(this, SIGNAL(signalNextPlayerAnimation()), this, SLOT(nextPlayerAnimation()));

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
	connect(this, SIGNAL(signalNetClientGameInfo(int)), myGameLobbyDialog, SLOT(refresh(int)));

	connect(this, SIGNAL(signalNetClientSelfJoined(unsigned, QString, int)), myStartNetworkGameDialog, SLOT(joinedNetworkGame(unsigned, QString, int)));
	connect(this, SIGNAL(signalNetClientPlayerJoined(unsigned, QString, int)), myStartNetworkGameDialog, SLOT(addConnectedPlayer(unsigned, QString, int)));
	connect(this, SIGNAL(signalNetClientPlayerChanged(unsigned, QString)), myStartNetworkGameDialog, SLOT(updatePlayer(unsigned, QString)));
	connect(this, SIGNAL(signalNetClientPlayerLeft(unsigned, QString)), myStartNetworkGameDialog, SLOT(removePlayer(unsigned, QString)));
	connect(this, SIGNAL(signalNetClientNewGameAdmin(unsigned, QString)), myStartNetworkGameDialog, SLOT(newGameAdmin(unsigned, QString)));
	connect(this, SIGNAL(signalNetClientGameListNew(unsigned)), myStartNetworkGameDialog, SLOT(gameCreated(unsigned)));

	connect(this, SIGNAL(signalNetClientSelfJoined(unsigned, QString, int)), myGameLobbyDialog, SLOT(joinedNetworkGame(unsigned, QString, int)));
	connect(this, SIGNAL(signalNetClientPlayerJoined(unsigned, QString, int)), myGameLobbyDialog, SLOT(addConnectedPlayer(unsigned, QString, int)));
	connect(this, SIGNAL(signalNetClientPlayerChanged(unsigned, QString)), myGameLobbyDialog, SLOT(updatePlayer(unsigned, QString)));
	connect(this, SIGNAL(signalNetClientPlayerLeft(unsigned, QString)), myGameLobbyDialog, SLOT(removePlayer(unsigned, QString)));
	connect(this, SIGNAL(signalNetClientNewGameAdmin(unsigned, QString)), myGameLobbyDialog, SLOT(newGameAdmin(unsigned, QString)));

	connect(this, SIGNAL(signalNetClientGameListNew(unsigned)), myGameLobbyDialog, SLOT(addGame(unsigned)));
	connect(this, SIGNAL(signalNetClientGameListRemove(unsigned)), myGameLobbyDialog, SLOT(removeGame(unsigned)));
	connect(this, SIGNAL(signalNetClientGameListUpdateMode(unsigned, int)), myGameLobbyDialog, SLOT(updateGameMode(unsigned, int)));
	connect(this, SIGNAL(signalNetClientGameListUpdateAdmin(unsigned, unsigned)), myGameLobbyDialog, SLOT(updateGameAdmin(unsigned, unsigned)));
	connect(this, SIGNAL(signalNetClientGameListPlayerJoined(unsigned, unsigned)), myGameLobbyDialog, SLOT(gameAddPlayer(unsigned, unsigned)));
	connect(this, SIGNAL(signalNetClientGameListPlayerLeft(unsigned, unsigned)), myGameLobbyDialog, SLOT(gameRemovePlayer(unsigned, unsigned)));
	connect(this, SIGNAL(signalNetClientRemovedFromGame(int)), myGameLobbyDialog, SLOT(removedFromGame(int)));
	connect(this, SIGNAL(signalNetClientStatsUpdate(ServerStats)), myGameLobbyDialog, SLOT(updateStats(ServerStats)));

	// Errors are handled globally, not within one dialog.
	connect(this, SIGNAL(signalNetClientError(int, int)), this, SLOT(networkError(int, int)));
	connect(this, SIGNAL(signalNetClientNotification(int)), this, SLOT(networkNotification(int)));
	connect(this, SIGNAL(signalNetClientRemovedFromGame(int)), this, SLOT(networkNotification(int)));
	connect(this, SIGNAL(signalNetServerError(int, int)), this, SLOT(networkError(int, int)));
	connect(this, SIGNAL(signalNetClientGameStart(boost::shared_ptr<Game>)), this, SLOT(networkStart(boost::shared_ptr<Game>)));

	//Chat Messages	
	connect(this, SIGNAL(signalNetClientChatMsg(QString, QString)), myChat, SLOT(receiveMessage(QString, QString)));
	connect(this, SIGNAL(signalNetClientChatMsg(QString, QString)), myStartNetworkGameDialog, SLOT(receiveChatMsg(QString, QString)));

	connect(this, SIGNAL(signalIrcConnect(QString)), myGameLobbyDialog->getLobbyChat(), SLOT(connected(QString)));
	connect(this, SIGNAL(signalIrcSelfJoined(QString, QString)), myGameLobbyDialog->getLobbyChat(), SLOT(selfJoined(QString, QString)));
	connect(this, SIGNAL(signalIrcPlayerJoined(QString)), myGameLobbyDialog->getLobbyChat(), SLOT(playerJoined(QString)));
	connect(this, SIGNAL(signalIrcPlayerChanged(QString, QString)), myGameLobbyDialog->getLobbyChat(), SLOT(playerChanged(QString, QString)));
	connect(this, SIGNAL(signalIrcPlayerKicked(QString, QString, QString)), myGameLobbyDialog->getLobbyChat(), SLOT(playerKicked(QString, QString, QString)));
	connect(this, SIGNAL(signalIrcPlayerLeft(QString)), myGameLobbyDialog->getLobbyChat(), SLOT(playerLeft(QString)));
	connect(this, SIGNAL(signalIrcChatMessage(QString, QString)), myGameLobbyDialog->getLobbyChat(), SLOT(displayMessage(QString, QString)));
	connect(this, SIGNAL(signalIrcError(int)), myGameLobbyDialog->getLobbyChat(), SLOT(chatError(int)));
	connect(this, SIGNAL(signalIrcServerError(int)), myGameLobbyDialog->getLobbyChat(), SLOT(chatServerError(int)));

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
		if (myNewGameDialog->result() == QDialog::Accepted ) { startNewLocalGame(myNewGameDialog); }
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
		gameData.firstSmallBlind = v->getChangeCompleteBlindsDialog()->spinBox_firstSmallBlind->value();
		
		if(v->getChangeCompleteBlindsDialog()->radioButton_raiseBlindsAtHands->isChecked()) { 
			gameData.raiseIntervalMode = RAISE_ON_HANDNUMBER;
			gameData.raiseSmallBlindEveryHandsValue = v->getChangeCompleteBlindsDialog()->spinBox_raiseSmallBlindEveryHands->value();
		}
		else { 
			gameData.raiseIntervalMode = RAISE_ON_MINUTES; 
			gameData.raiseSmallBlindEveryMinutesValue = v->getChangeCompleteBlindsDialog()->spinBox_raiseSmallBlindEveryMinutes->value();
		}
		
		if(v->getChangeCompleteBlindsDialog()->radioButton_alwaysDoubleBlinds->isChecked()) { 
			gameData.raiseMode = DOUBLE_BLINDS; 
		}
		else { 
			gameData.raiseMode = MANUAL_BLINDS_ORDER;
			list<int> tempBlindList;
			int i;
			bool ok = TRUE;
			for(i=0; i<v->getChangeCompleteBlindsDialog()->listWidget_blinds->count(); i++) {
				tempBlindList.push_back(v->getChangeCompleteBlindsDialog()->listWidget_blinds->item(i)->text().toInt(&ok,10));		
			}
			gameData.manualBlindsList = tempBlindList;
			
			if(v->getChangeCompleteBlindsDialog()->radioButton_afterThisAlwaysDoubleBlinds->isChecked()) { gameData.afterManualBlindsMode = AFTERMB_DOUBLE_BLINDS; }
			else {
				if(v->getChangeCompleteBlindsDialog()->radioButton_afterThisAlwaysRaiseAbout->isChecked()) {
					gameData.afterManualBlindsMode = AFTERMB_RAISE_ABOUT;
					gameData.afterMBAlwaysRaiseValue = v->getChangeCompleteBlindsDialog()->spinBox_afterThisAlwaysRaiseValue->value();
				}
				else { gameData.afterManualBlindsMode = AFTERMB_STAY_AT_LAST_BLIND; }	
			}
		}
		
		//Speeds 
		gameData.guiSpeed = v->spinBox_gameSpeed->value();
	}
	// start with default values
	else {
		// Set Game Data
		gameData.maxNumberOfPlayers = myConfig->readConfigInt("NumberOfPlayers");
		gameData.startMoney = myConfig->readConfigInt("StartCash");
		gameData.firstSmallBlind =  myConfig->readConfigInt("FirstSmallBlind");
	
		if(myConfig->readConfigInt("RaiseBlindsAtHands")) { 
			gameData.raiseIntervalMode = RAISE_ON_HANDNUMBER;
			gameData.raiseSmallBlindEveryHandsValue = myConfig->readConfigInt("RaiseSmallBlindEveryHands");
		}
		else { 
			gameData.raiseIntervalMode = RAISE_ON_MINUTES; 
			gameData.raiseSmallBlindEveryMinutesValue = myConfig->readConfigInt("RaiseSmallBlindEveryMinutes");
		}
		
		if(myConfig->readConfigInt("AlwaysDoubleBlinds")) { 
			gameData.raiseMode = DOUBLE_BLINDS; 
		}
		else { 
			gameData.raiseMode = MANUAL_BLINDS_ORDER;
			gameData.manualBlindsList = myConfig->readConfigIntList("ManualBlindsList");
			
			if(myConfig->readConfigInt("AfterMBAlwaysDoubleBlinds")) { gameData.afterManualBlindsMode = AFTERMB_DOUBLE_BLINDS; }
			else {
				if(myConfig->readConfigInt("AfterMBAlwaysRaiseAbout")) {
					gameData.afterManualBlindsMode = AFTERMB_RAISE_ABOUT;
					gameData.afterMBAlwaysRaiseValue = myConfig->readConfigInt("AfterMBAlwaysRaiseValue");
				}
				else { gameData.afterManualBlindsMode = AFTERMB_STAY_AT_LAST_BLIND; }	
			}
		}
		//Speeds 
		gameData.guiSpeed = myConfig->readConfigInt("GameSpeed");
	}
	// Set dealer pos.
	StartData startData;
	int tmpDealerPos = 0;
	startData.numberOfPlayers = gameData.maxNumberOfPlayers;
	Tools::getRandNumber(0, startData.numberOfPlayers-1, 1, &tmpDealerPos, 0);
	if(DEBUG_MODE) {
		tmpDealerPos = 1;
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

		// Stop local game.
		stopTimer();

		if (!myServerGuiInterface.get())
		{
			// Create pseudo Gui Wrapper for the server.
			myServerGuiInterface.reset(new ServerGuiWrapper(myConfig, mySession->getGui(), mySession->getGui(), mySession->getGui()));
			{
				boost::shared_ptr<Session> session(new Session(myServerGuiInterface.get(), myConfig));
				session->init(mySession->getAvatarManager());
				myServerGuiInterface->setSession(session);
			}
		}

		// Terminate existing network games.
		mySession->terminateNetworkClient();
		myServerGuiInterface->getSession().terminateNetworkServer();

		GameData gameData;
		gameData.maxNumberOfPlayers = myCreateNetworkGameDialog->spinBox_quantityPlayers->value();
		gameData.startMoney = myCreateNetworkGameDialog->spinBox_startCash->value();
		gameData.firstSmallBlind = myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->spinBox_firstSmallBlind->value();
		
		if(myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->radioButton_raiseBlindsAtHands->isChecked()) { 
			gameData.raiseIntervalMode = RAISE_ON_HANDNUMBER;
			gameData.raiseSmallBlindEveryHandsValue = myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->spinBox_raiseSmallBlindEveryHands->value();
		}
		else { 
			gameData.raiseIntervalMode = RAISE_ON_MINUTES; 
			gameData.raiseSmallBlindEveryMinutesValue = myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->spinBox_raiseSmallBlindEveryMinutes->value();
		}
		
		if(myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->radioButton_alwaysDoubleBlinds->isChecked()) { 
			gameData.raiseMode = DOUBLE_BLINDS; 
		}
		else { 
			gameData.raiseMode = MANUAL_BLINDS_ORDER;
			list<int> tempBlindList;
			int i;
			bool ok = TRUE;
			for(i=0; i<myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->listWidget_blinds->count(); i++) {
				tempBlindList.push_back(myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->listWidget_blinds->item(i)->text().toInt(&ok,10));		
			}
			gameData.manualBlindsList = tempBlindList;
			
			if(myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->radioButton_afterThisAlwaysDoubleBlinds->isChecked()) { gameData.afterManualBlindsMode = AFTERMB_DOUBLE_BLINDS; }
			else {
				if(myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->radioButton_afterThisAlwaysRaiseAbout->isChecked()) {
					gameData.afterManualBlindsMode = AFTERMB_RAISE_ABOUT;
					gameData.afterMBAlwaysRaiseValue = myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->spinBox_afterThisAlwaysRaiseValue->value();
				}
				else { gameData.afterManualBlindsMode = AFTERMB_STAY_AT_LAST_BLIND; }	
			}
		}

		gameData.guiSpeed = myCreateNetworkGameDialog->spinBox_gameSpeed->value();
		gameData.playerActionTimeoutSec = myCreateNetworkGameDialog->spinBox_netTimeOutPlayerAction->value();

		myGameLobbyDialog->setSession(&getSession());
		myStartNetworkGameDialog->setSession(&getSession());

		// Clear network game dialog.
		myStartNetworkGameDialog->clearDialog();

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

		// Stop local game.
		stopTimer();

		mySession->terminateNetworkClient();
		if (myServerGuiInterface.get())
			myServerGuiInterface->getSession().terminateNetworkServer();

		myGameLobbyDialog->setSession(&getSession());
		myStartNetworkGameDialog->setSession(&getSession());
		// Clear network game dialog
		myStartNetworkGameDialog->clearDialog();
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

	//Avoid join Lobby with "Human Player" nick
	if(QString::fromUtf8(myConfig->readConfigString("MyName").c_str()) == QString("Human Player")) {
		myChangeHumanPlayerNameDialog->label_Message->setText(tr("You cannot join Internet-Game-Lobby with \"Human Player\" as nickname.\nPlease choose another one."));
		myChangeHumanPlayerNameDialog->exec();

		if(myChangeHumanPlayerNameDialog->result() == QDialog::Accepted) {
			joinGameLobby();
		}
	}
	else {	
		joinGameLobby();
	}
}

void mainWindowImpl::joinGameLobby() {

	// Stop local game.
	stopTimer();

// Join Lobby
	mySession->terminateNetworkClient();
	if (myServerGuiInterface.get())
		myServerGuiInterface->getSession().terminateNetworkServer();
		myGameLobbyDialog->setSession(&getSession());
		myStartNetworkGameDialog->setSession(&getSession());

	// Clear Lobby dialog.
	myGameLobbyDialog->clearDialog();

	//set clean irc nick
	QString myNick(QString::fromUtf8(myConfig->readConfigString("MyName").c_str()));
	myNick.replace(QString::fromUtf8("ä"),"ae");
	myNick.replace(QString::fromUtf8("Ä"),"Ae");
	myNick.replace(QString::fromUtf8("ü"),"ue");
	myNick.replace(QString::fromUtf8("Ü"),"Ue");
	myNick.replace(QString::fromUtf8("ö"),"oe");
	myNick.replace(QString::fromUtf8("Ö"),"Oe");
	myNick.replace(QString::fromUtf8("é"),"e");
	myNick.replace(QString::fromUtf8("è"),"e");
	myNick.replace(QString::fromUtf8("á"),"a");
	myNick.replace(QString::fromUtf8("à"),"a");	
	myNick.replace(QString::fromUtf8("ó"),"o");
	myNick.replace(QString::fromUtf8("ò"),"o");
	myNick.replace(QString::fromUtf8("ú"),"u");
	myNick.replace(QString::fromUtf8("ù"),"u");
	myNick.replace(QString::fromUtf8("É"),"E");
	myNick.replace(QString::fromUtf8("È"),"E");
	myNick.replace(QString::fromUtf8("Á"),"A");
	myNick.replace(QString::fromUtf8("À"),"A");	
	myNick.replace(QString::fromUtf8("Ó"),"O");
	myNick.replace(QString::fromUtf8("Ò"),"O");
	myNick.replace(QString::fromUtf8("Ú"),"U");
	myNick.replace(QString::fromUtf8("Ù"),"U");
	myNick.remove(QRegExp("[^A-Z^a-z^0-9|\\-_\\\\^`]*"));
	myNick = myNick.mid(0,16);

 	mySession->setIrcNick(myNick.toUtf8().constData());
	
	// Start client for dedicated server.
	mySession->startInternetClient();
	
	//Dialog mit Statusbalken
	myConnectToServerDialog->exec();
	
	if (myConnectToServerDialog->result() == QDialog::Rejected ) {
		mySession->terminateNetworkClient();
	}
	else
	{
		showLobbyDialog();
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

		//Add avatar (if set)
		mySession->addOwnAvatar(myConfig->readConfigString("MyAvatar"));

		//Falls Spielernamen geändert wurden --> neu zeichnen --> erst beim nächsten Neustart neu ausgelesen
		if (mySettingsDialog->getPlayerNickIsChanged() && mySession->getCurrentGame() && !mySession->isNetworkClientRunning()) { 

			Game *currentGame = mySession->getCurrentGame();
			PlayerListIterator it = currentGame->getSeatsList()->begin();
			(*it)->setMyName(mySettingsDialog->lineEdit_HumanPlayerName->text().toUtf8().constData());
			(*(++it))->setMyName(mySettingsDialog->lineEdit_Opponent1Name->text().toUtf8().constData());
			(*(++it))->setMyName(mySettingsDialog->lineEdit_Opponent2Name->text().toUtf8().constData());
			(*(++it))->setMyName(mySettingsDialog->lineEdit_Opponent3Name->text().toUtf8().constData());
			(*(++it))->setMyName(mySettingsDialog->lineEdit_Opponent4Name->text().toUtf8().constData());
			(*(++it))->setMyName(mySettingsDialog->lineEdit_Opponent5Name->text().toUtf8().constData());
			(*(++it))->setMyName(mySettingsDialog->lineEdit_Opponent6Name->text().toUtf8().constData());
			mySettingsDialog->setPlayerNickIsChanged(FALSE);

			refreshPlayerName();
		}
	
		if(mySession->getCurrentGame() && !mySession->isNetworkClientRunning()) {

			Game *currentGame = mySession->getCurrentGame();
			PlayerListIterator it = currentGame->getSeatsList()->begin();
			(*it)->setMyAvatar(mySettingsDialog->pushButton_HumanPlayerAvatar->getMyLink().toUtf8().constData());
			(*(++it))->setMyAvatar(mySettingsDialog->pushButton_Opponent1Avatar->getMyLink().toUtf8().constData());
			(*(++it))->setMyAvatar(mySettingsDialog->pushButton_Opponent2Avatar->getMyLink().toUtf8().constData());
			(*(++it))->setMyAvatar(mySettingsDialog->pushButton_Opponent3Avatar->getMyLink().toUtf8().constData());
			(*(++it))->setMyAvatar(mySettingsDialog->pushButton_Opponent4Avatar->getMyLink().toUtf8().constData());
			(*(++it))->setMyAvatar(mySettingsDialog->pushButton_Opponent5Avatar->getMyLink().toUtf8().constData());
			(*(++it))->setMyAvatar(mySettingsDialog->pushButton_Opponent6Avatar->getMyLink().toUtf8().constData());

			//avatar refresh
			refreshPlayerAvatar();		
		}

		//Flipside refresh
		if (myConfig->readConfigInt("FlipsideOwn") && myConfig->readConfigString("FlipsideOwnFile") != "") {
			flipside = new QPixmap(QString::fromUtf8(myConfig->readConfigString("FlipsideOwnFile").c_str()));
		}
		else { 
			flipside = new QPixmap(myAppDataPath +"gfx/cards/default/flipside.png");
		}

		//Check for anti-peek mode
		if(mySession->getCurrentGame()) {
			QPixmap tempCardsPixmapArray[2];
			int tempCardsIntArray[2];
			
			mySession->getCurrentGame()->getSeatsList()->front()->getMyCards(tempCardsIntArray);	
			if(myConfig->readConfigInt("AntiPeekMode")) {
				holeCardsArray[0][0]->setPixmap(*flipside, TRUE);
				tempCardsPixmapArray[0].load(myAppDataPath +"gfx/cards/default/"+QString::number(tempCardsIntArray[0], 10)+".png");
				holeCardsArray[0][0]->setFrontPixmap(tempCardsPixmapArray[0]);
				holeCardsArray[0][1]->setPixmap(*flipside, TRUE);
				tempCardsPixmapArray[1].load(myAppDataPath +"gfx/cards/default/"+QString::number(tempCardsIntArray[1], 10)+".png");
				holeCardsArray[0][1]->setFrontPixmap(tempCardsPixmapArray[1]);
			}
			else {
				tempCardsPixmapArray[0].load(myAppDataPath +"gfx/cards/default/"+QString::number(tempCardsIntArray[0], 10)+".png");
				holeCardsArray[0][0]->setPixmap(tempCardsPixmapArray[0],FALSE);
				tempCardsPixmapArray[1].load(myAppDataPath +"gfx/cards/default/"+QString::number(tempCardsIntArray[1], 10)+".png");
				holeCardsArray[0][1]->setPixmap(tempCardsPixmapArray[1],FALSE);
			}
		}

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
	//kill running Singleshots!!!
	stopTimer();
		
	label_Pot->setText("Pot");
	label_Total->setText("Total:");
	label_Sets->setText("Sets:");
	label_handNumber->setText("Hand:");
	label_gameNumber->setText("Game:");
	
	groupBox_RightToolBox->setDisabled(FALSE);
	groupBox_LeftToolBox->setDisabled(FALSE);	
		
	//show human player buttons
	for(int i=0; i<6; i++) {
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

void mainWindowImpl::showClientDialog()
{
	if (mySession->getGameType() == Session::GAME_TYPE_NETWORK)
	{
		if (!myStartNetworkGameDialog->isVisible())
			showNetworkStartDialog();
	}
	else if (mySession->getGameType() == Session::GAME_TYPE_INTERNET)
	{
		if (!myGameLobbyDialog->isVisible())
			showLobbyDialog();
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

void mainWindowImpl::showLobbyDialog()
{
	myGameLobbyDialog->exec(); 

	if (myGameLobbyDialog->result() == QDialog::Accepted)
	{
		//some gui modifications
		networkGameModification();
	}
	else
	{
		mySession->terminateNetworkClient();
	}
}

Session &mainWindowImpl::getSession() { assert(mySession.get()); return *mySession; }
void mainWindowImpl::setSession(boost::shared_ptr<Session> session) { mySession = session; }


//refresh-Funktionen
void mainWindowImpl::refreshSet() {
	
	Game *currentGame = mySession->getCurrentGame();

	PlayerListConstIterator it_c;
 	for (it_c=currentGame->getSeatsList()->begin(); it_c!=currentGame->getSeatsList()->end(); it_c++) { 
		if( (*it_c)->getMySet() == 0 )
			setLabelArray[(*it_c)->getMyID()]->setText("");
		else
			setLabelArray[(*it_c)->getMyID()]->setText("Bet: $"+QString::number((*it_c)->getMySet(),10)); 
	}


}

void mainWindowImpl::refreshButton() {

	QPixmap dealerButton(myAppDataPath +"gfx/gui/table/default/dealerPuck.png");
	QPixmap smallblindButton(myAppDataPath +"gfx/gui/table/default/smallblindPuck.png");
	QPixmap bigblindButton(myAppDataPath +"gfx/gui/table/default/bigblindPuck.png");
	QPixmap onePix(myAppDataPath +"gfx/gui/misc/1px.png");

	Game *currentGame = mySession->getCurrentGame();

	PlayerListConstIterator it_c;

	for (it_c=currentGame->getSeatsList()->begin(); it_c!=currentGame->getSeatsList()->end(); it_c++) { 
		if( (*it_c)->getMyActiveStatus() ) { 
			if( currentGame->getActivePlayerList()->size() > 2 ) {
				switch ( (*it_c)->getMyButton() ) {
				
					case 1 : buttonLabelArray[(*it_c)->getMyID()]->setPixmap(dealerButton); 
					break;
					case 2 : { 	
						if(myConfig->readConfigInt("ShowBlindButtons"))
							buttonLabelArray[(*it_c)->getMyID()]->setPixmap(smallblindButton); 
						else
							buttonLabelArray[(*it_c)->getMyID()]->setPixmap(onePix);				  
					}
					break;
					case 3 : { 
						if(myConfig->readConfigInt("ShowBlindButtons"))
							buttonLabelArray[(*it_c)->getMyID()]->setPixmap(bigblindButton); 				
						else
							buttonLabelArray[(*it_c)->getMyID()]->setPixmap(onePix);				  
					}
					break;
					default: buttonLabelArray[(*it_c)->getMyID()]->setPixmap(onePix);
				
				}
			}
			else {
				switch ((*it_c)->getMyButton()) {
			
					case 2 : buttonLabelArray[(*it_c)->getMyID()]->setPixmap(dealerButton); 
					break;
					case 3 : { 
						if(myConfig->readConfigInt("ShowBlindButtons"))
							buttonLabelArray[(*it_c)->getMyID()]->setPixmap(bigblindButton);
						else
							buttonLabelArray[(*it_c)->getMyID()]->setPixmap(onePix);
					}
					break;
					default: buttonLabelArray[(*it_c)->getMyID()]->setPixmap(onePix);
				
				}
			}	
		}
		else { buttonLabelArray[(*it_c)->getMyID()]->setPixmap(onePix); }
	}



}

void mainWindowImpl::refreshPlayerName() {

	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();

	PlayerListConstIterator it_c;
	for (it_c=currentHand->getSeatsList()->begin(); it_c!=currentHand->getSeatsList()->end(); it_c++) {
		if((*it_c)->getMyActiveStatus()) { 
			playerNameLabelArray[(*it_c)->getMyID()]->setText(QString::fromUtf8((*it_c)->getMyName().c_str()));
			
		} else {
			playerNameLabelArray[(*it_c)->getMyID()]->setText(""); 
		
		}
	}
}

QStringList mainWindowImpl::getPlayerNicksList() {

	QStringList list;
	PlayerListConstIterator it_c;
	
	for (it_c=mySession->getCurrentGame()->getSeatsList()->begin(); it_c!=mySession->getCurrentGame()->getSeatsList()->end(); it_c++) {
		list << QString::fromUtf8((*it_c)->getMyName().c_str());		
	}
	
	return list;
}

void mainWindowImpl::refreshPlayerAvatar() {

	QPixmap onePix(myAppDataPath +"gfx/gui/misc/1px.png");

	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();

	PlayerListConstIterator it_c;
	for (it_c=currentHand->getSeatsList()->begin(); it_c!=currentHand->getSeatsList()->end(); it_c++) {
		if((*it_c)->getMyActiveStatus()) { 

			if((*it_c)->getMyAvatar() == "" || !QFile::QFile(QString::fromUtf8((*it_c)->getMyAvatar().c_str())).exists()) {
				playerAvatarLabelArray[(*it_c)->getMyID()]->setPixmap(QPixmap(myAppDataPath +"gfx/gui/table/default/genereticAvatar.png"));
			}
			else {
				playerAvatarLabelArray[(*it_c)->getMyID()]->setPixmap(QString::fromUtf8((*it_c)->getMyAvatar().c_str()));
			}
		}	
		else {
			playerAvatarLabelArray[(*it_c)->getMyID()]->setPixmap(onePix);
		}		
	}
}

void mainWindowImpl::setPlayerAvatar(int myID, QString myAvatar) {

	if(mySession->getCurrentGame()) {

		boost::shared_ptr<PlayerInterface> tmpPlayer = mySession->getCurrentGame()->getPlayerByUniqueId(myID);
		if (tmpPlayer.get()) {

			if(QFile::QFile(myAvatar).exists()) {
				playerAvatarLabelArray[tmpPlayer->getMyID()]->setPixmap(myAvatar);
				tmpPlayer->setMyAvatar(myAvatar.toUtf8().constData());
			}
			else {
				playerAvatarLabelArray[tmpPlayer->getMyID()]->setPixmap(QPixmap(myAppDataPath +"gfx/gui/table/default/genereticAvatar.png"));
				tmpPlayer->setMyAvatar("");
			}	
		}	
	}
}


void mainWindowImpl::refreshAction(int playerID, int playerAction) {

	QPixmap onePix(myAppDataPath +"gfx/gui/misc/1px.png");
	QPixmap action;

	QStringList actionArray;
	actionArray << "" << "fold" << "check" << "call" << "bet" << "raise" << "allin";

	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();

	if(playerID == -1 || playerAction == -1) {

		PlayerListConstIterator it_c;
		for (it_c=currentHand->getSeatsList()->begin(); it_c!=currentHand->getSeatsList()->end(); it_c++) { 
			
			//if no action --> clear Pixmap 
			if( (*it_c)->getMyAction() == 0) {
				actionLabelArray[(*it_c)->getMyID()]->setPixmap(onePix);	
			}
			else {
					//paint action pixmap
					actionLabelArray[(*it_c)->getMyID()]->setPixmap(QPixmap(myAppDataPath +"gfx/gui/table/default/action_"+actionArray[(*it_c)->getMyAction()]+".png"));
			}
					
			if ((*it_c)->getMyAction()==1) { 
				
				if((*it_c)->getMyID() != 0) {
					holeCardsArray[(*it_c)->getMyID()][0]->setPixmap(onePix, FALSE);
					holeCardsArray[(*it_c)->getMyID()][1]->setPixmap(onePix, FALSE);
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
			actionLabelArray[playerID]->setPixmap(QPixmap(myAppDataPath +"gfx/gui/table/default/action_"+actionArray[playerAction]+".png"));			

			//play sounds if exist
			if(myConfig->readConfigInt("PlayGameActions"))
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

	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();

	PlayerListConstIterator it_c;
	for (it_c=currentHand->getSeatsList()->begin(); it_c!=currentHand->getSeatsList()->end(); it_c++) { 
		if((*it_c)->getMyActiveStatus()) { 

			cashLabelArray[(*it_c)->getMyID()]->setText("$"+QString::number((*it_c)->getMyCash(),10)); 
			cashTopLabelArray[(*it_c)->getMyID()]->setText("Cash:"); 
			
		} else {
			cashLabelArray[(*it_c)->getMyID()]->setText(""); 
			cashTopLabelArray[(*it_c)->getMyID()]->setText("");
		}
	}
}

void mainWindowImpl::refreshGroupbox(int playerID, int status) {

	int j;

	if(playerID == -1 || status == -1) {

		HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();
		PlayerListConstIterator it_c;
		for (it_c=currentHand->getSeatsList()->begin(); it_c!=currentHand->getSeatsList()->end(); it_c++) { 
	
			if((*it_c)->getMyTurn()) {
				//Groupbox glow wenn der Spiele dran ist. 
				if((*it_c)->getMyID()==0) {
					groupBoxArray[0]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/playerBoxActiveGlow_0.6.png) }"); 
				}
				else {
					groupBoxArray[(*it_c)->getMyID()]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/opponentBoxActiveGlow.png) }"); 
				}
	
			} else {
				//Groupbox auf Hintergrundfarbe setzen wenn der Spiele nicht dran aber aktiv ist. 
				if((*it_c)->getMyActiveStatus()) {
					if((*it_c)->getMyID()==0) {
						groupBoxArray[0]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/playerBoxInactiveGlow_0.6.png) }"); 
						//show buttons
						for(j=0; j<6; j++) {
							userWidgetsArray[j]->show();
						}
					}
					else {
						groupBoxArray[(*it_c)->getMyID()]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/opponentBoxInactiveGlow.png) }"); 
					}	
				}
				//Groupbox verdunkeln wenn der Spiele inactive ist.  
				else {
					if((*it_c)->getMyID()==0) {
						groupBoxArray[0]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/playerBoxInactiveGlow_0.6.png) }"); 
						//hide buttons
						for(j=0; j<6; j++) {
							userWidgetsArray[j]->hide();
						}	
						//disable anti-peek front after player is out
						holeCardsArray[0][0]->signalFastFlipCards(FALSE);
						holeCardsArray[0][1]->signalFastFlipCards(FALSE);						
					}
					else {
						groupBoxArray[(*it_c)->getMyID()]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/opponentBoxInactiveGlow.png) }"); 
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
					groupBoxArray[playerID]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/playerBoxInactiveGlow_0.6.png) }"); 	
					//hide buttons
					for(j=0; j<6; j++) {
						userWidgetsArray[j]->hide();
					}					
					//disable anti-peek front after player is out
					holeCardsArray[0][0]->signalFastFlipCards(FALSE);
					holeCardsArray[0][1]->signalFastFlipCards(FALSE);						
				}
				else {
					groupBoxArray[playerID]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/opponentBoxInactiveGlow.png) }"); 
				}
			}
		break;
		//active but fold
		case 1: {
				if (!playerID) {
					groupBoxArray[playerID]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/playerBoxInactiveGlow_0.6.png) }"); 	
					//show buttons
					for(j=0; j<6; j++) {
						userWidgetsArray[j]->show();
					}		
				}
				else {
					groupBoxArray[playerID]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/opponentBoxInactiveGlow.png) }"); 
				}
			}
		break;
		//active in action
		case 2:  {
				if (!playerID) {
					groupBoxArray[playerID]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/playerBoxActiveGlow_0.6.png) }"); 
				}
				else {
					groupBoxArray[playerID]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/opponentBoxActiveGlow.png) }"); 
				}
			}
		break;
		//active not in action
		case 3:  {
				if (!playerID) {
					groupBoxArray[playerID]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/playerBoxInactiveGlow_0.6.png) }"); 	
				}
				else {
					groupBoxArray[playerID]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/opponentBoxInactiveGlow.png) }"); 
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

	label_handNumberValue->setText(QString::number(mySession->getCurrentGame()->getCurrentHand()->getMyID(),10));
	label_gameNumberValue->setText(QString::number(mySession->getCurrentGame()->getMyGameID(),10));
}

void mainWindowImpl::refreshAll() {
	
	refreshSet();
	refreshButton();

	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();
	PlayerListConstIterator it_c;
	for (it_c=currentHand->getSeatsList()->begin(); it_c!=currentHand->getSeatsList()->end(); it_c++) { 
		refreshAction( (*it_c)->getMyID(), (*it_c)->getMyAction());
	}

	refreshCash();
	refreshGroupbox();
	refreshPlayerName();
	refreshPlayerAvatar();
}

void mainWindowImpl::refreshChangePlayer() {

	refreshSet();

	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();
	PlayerListConstIterator it_c;
	for (it_c=currentHand->getSeatsList()->begin(); it_c!=currentHand->getSeatsList()->end(); it_c++) { 
		refreshAction( (*it_c)->getMyID(), (*it_c)->getMyAction());
	}

	refreshCash();
}

void mainWindowImpl::refreshPot() {
	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();

	textLabel_Sets->setText("$"+QString::number(currentHand->getBoard()->getSets(),10));
	textLabel_Pot->setText("$"+QString::number(currentHand->getBoard()->getPot(),10));
}

void mainWindowImpl::guiUpdateDone() {
	guiUpdateSemaphore.release();
}

void mainWindowImpl::waitForGuiUpdateDone() {
	guiUpdateSemaphore.acquire();
}

void mainWindowImpl::dealHoleCards() {

	QPixmap onePix(myAppDataPath +"gfx/gui/misc/1px.png");

	//TempArrays
	QPixmap tempCardsPixmapArray[2];
	int tempCardsIntArray[2];
	
	// Karten der Gegner und eigene Karten austeilen
	int j;
	Game *currentGame = mySession->getCurrentGame();

	PlayerListConstIterator it_c;
	for(it_c=currentGame->getSeatsList()->begin(); it_c!=currentGame->getSeatsList()->end(); it_c++) {
		(*it_c)->getMyCards(tempCardsIntArray);	
		for(j=0; j<2; j++) {
			if((*it_c)->getMyActiveStatus()) { 
				if (( (*it_c)->getMyID() == 0) || DEBUG_MODE) {
					if(myConfig->readConfigInt("AntiPeekMode")) {
						holeCardsArray[(*it_c)->getMyID()][j]->setPixmap(*flipside, TRUE);
						tempCardsPixmapArray[j].load(myAppDataPath +"gfx/cards/default/"+QString::number(tempCardsIntArray[j], 10)+".png");
						holeCardsArray[(*it_c)->getMyID()][j]->setFrontPixmap(tempCardsPixmapArray[j]);
					}
					else {
						tempCardsPixmapArray[j].load(myAppDataPath +"gfx/cards/default/"+QString::number(tempCardsIntArray[j], 10)+".png");
						holeCardsArray[(*it_c)->getMyID()][j]->setPixmap(tempCardsPixmapArray[j],FALSE);
					}
				} 
				else {
					holeCardsArray[(*it_c)->getMyID()][j]->setPixmap(*flipside, TRUE);
/*					holeCardsArray[i][j]->setStyleSheet("QLabel:hover { background-image:url(:/cards/resources/graphics/cards/"+QString::number(tempCardsIntArray[j], 10)+".png");*/
				}
			}
			else {
				
				holeCardsArray[(*it_c)->getMyID()][j]->setPixmap(onePix, FALSE);
//					holeCardsArray[i][j]->repaint();
			}
		}
	}

	//fix press mouse button during bankrupt with anti-peek-mode
	this->mouseOverFlipCards(FALSE);
}

void mainWindowImpl::dealBeRoCards(int myBeRoID) {	

	uncheckMyButtons();
	myButtonsCheckable(FALSE);
	resetMyButtonsCheckStateMemory();
	clearMyButtons();

	pushButton_AllIn->setDisabled(TRUE);
	horizontalSlider_bet->setDisabled(TRUE);
	lineEdit_betValue->setDisabled(TRUE);

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
	tempCardsPixmap.load(myAppDataPath +"gfx/cards/default/"+QString::number(tempBoardCardsArray[0], 10)+".png");
	QPixmap card(tempCardsPixmap);

	//Config? mit oder ohne Eye-Candy?
	if(myConfig->readConfigInt("ShowFlipCardsAnimation")) { 
		//with Eye-Candy
		boardCardsArray[0]->startFlipCards(guiGameSpeed, card, *flipside);
	}
	else {
		//without Eye-Candy
		boardCardsArray[0]->setPixmap(card, FALSE);
	}
	dealFlopCards4Timer->start(dealCardsSpeed);
}

void mainWindowImpl::dealFlopCards5() {

	int tempBoardCardsArray[5];
	QPixmap tempCardsPixmap;
	mySession->getCurrentGame()->getCurrentHand()->getBoard()->getMyCards(tempBoardCardsArray);
	tempCardsPixmap.load(myAppDataPath +"gfx/cards/default/"+QString::number(tempBoardCardsArray[1], 10)+".png");
	QPixmap card(tempCardsPixmap);
	
	//Config? mit oder ohne Eye-Candy?
	if(myConfig->readConfigInt("ShowFlipCardsAnimation")) { 
		//with Eye-Candy
		boardCardsArray[1]->startFlipCards(guiGameSpeed, card, *flipside);
	}
	else {
		//without Eye-Candy
		boardCardsArray[1]->setPixmap(card, FALSE);
	}
	dealFlopCards5Timer->start(dealCardsSpeed);
}

void mainWindowImpl::dealFlopCards6() {

	int tempBoardCardsArray[5];
	QPixmap tempCardsPixmap;
	mySession->getCurrentGame()->getCurrentHand()->getBoard()->getMyCards(tempBoardCardsArray);
	tempCardsPixmap.load(myAppDataPath +"gfx/cards/default/"+QString::number(tempBoardCardsArray[2], 10)+".png");
	QPixmap card(tempCardsPixmap);
	
	//Config? mit oder ohne Eye-Candy?
	if(myConfig->readConfigInt("ShowFlipCardsAnimation")) { 
		//with Eye-Candy
		boardCardsArray[2]->startFlipCards(guiGameSpeed, card, *flipside);
	}
	else {
		//without Eye-Candy
		boardCardsArray[2]->setPixmap(card, FALSE);
	}
	
	// stable
	// wenn alle All In
	if(mySession->getCurrentGame()->getCurrentHand()->getAllInCondition()) { dealFlopCards6Timer->start(AllInDealCardsSpeed); }
	// sonst normale Variante
	else { 
		updateMyButtonsState(0);  //mode 0 == called from dealberocards
		dealFlopCards6Timer->start(postDealCardsSpeed);
	}
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
	tempCardsPixmap.load(myAppDataPath +"gfx/cards/default/"+QString::number(tempBoardCardsArray[3], 10)+".png");
	QPixmap card(tempCardsPixmap);

	//Config? mit oder ohne Eye-Candy?
	if(myConfig->readConfigInt("ShowFlipCardsAnimation")) { 
		//with Eye-Candy
		boardCardsArray[3]->startFlipCards(guiGameSpeed, card, *flipside);
	}
	else {
		//without Eye-Candy
		boardCardsArray[3]->setPixmap(card, FALSE);
	}
	
	// stable
	// wenn alle All In
	if(mySession->getCurrentGame()->getCurrentHand()->getAllInCondition()) { dealTurnCards2Timer->start(AllInDealCardsSpeed);
	}
	// sonst normale Variante
	else { 
		updateMyButtonsState(0);  //mode 0 == called from dealberocards
		dealTurnCards2Timer->start(postDealCardsSpeed); 

	}
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
	tempCardsPixmap.load(myAppDataPath +"gfx/cards/default/"+QString::number(tempBoardCardsArray[4], 10)+".png");
	QPixmap card(tempCardsPixmap);

	//Config? mit oder ohne Eye-Candy?
	if(myConfig->readConfigInt("ShowFlipCardsAnimation")) { 
		//with Eye-Candy
		boardCardsArray[4]->startFlipCards(guiGameSpeed, card, *flipside);
	}
	else {
		//without Eye-Candy
		boardCardsArray[4]->setPixmap(card, FALSE);
	}

	// stable
	// wenn alle All In
	if(mySession->getCurrentGame()->getCurrentHand()->getAllInCondition()) { dealRiverCards2Timer->start(AllInDealCardsSpeed);	}
	// sonst normale Variante
	else {		
		updateMyButtonsState(0);  //mode 0 == called from dealberocards
		dealRiverCards2Timer->start(postDealCardsSpeed);
	}
}

void mainWindowImpl::provideMyActions(int mode) {

	QString pushButtonFoldString;
	QString pushButtonBetRaiseString;
	QString lastPushButtonBetRaiseString = pushButton_BetRaise->text();
	QString pushButtonCallCheckString;
	QString lastPushButtonCallCheckString = pushButton_CallCheck->text();
	
	
	Game *currentGame = mySession->getCurrentGame();
	HandInterface *currentHand = currentGame->getCurrentHand();

// 	cout << "provideMyActions get myAction" << currentHand->getSeatsList()->front()->getMyAction() << endl;
// 	cout << "provideMyActions get mySet" << currentGame->getSeatsList()->front()->getMySet() << endl;
// 	cout << "provideMyActions get HighestSet" << currentHand->getCurrentBeRo()->getHighestSet() << endl;

	//really disabled buttons if human player is fold/all-in or ... and not called from dealberocards
	if(/*pushButton_BetRaise->isCheckable() && */mode != 0 && (currentHand->getSeatsList()->front()->getMyAction() == PLAYER_ACTION_ALLIN || currentHand->getSeatsList()->front()->getMyAction() == PLAYER_ACTION_FOLD || (currentGame->getSeatsList()->front()->getMySet() == currentHand->getCurrentBeRo()->getHighestSet() && (currentGame->getSeatsList()->front()->getMyAction() != PLAYER_ACTION_NONE)))) {
	
		pushButton_BetRaise->setText("");
		pushButton_CallCheck->setText("");
		pushButton_Fold->setText("");	

		pushButton_AllIn->setDisabled(TRUE);
		horizontalSlider_bet->setDisabled(TRUE);
		lineEdit_betValue->setDisabled(TRUE);

		myButtonsCheckable(FALSE);
	}
	else {	
		pushButton_AllIn->setEnabled(TRUE);		
		horizontalSlider_bet->setEnabled(TRUE);
		lineEdit_betValue->setEnabled(TRUE);	

		//show available actions on buttons
		if(currentHand->getCurrentRound() == 0) { // preflop
			
			if (currentGame->getSeatsList()->front()->getMyCash()+currentGame->getSeatsList()->front()->getMySet() > currentHand->getCurrentBeRo()->getHighestSet()) { 
				pushButtonBetRaiseString = "Raise $"+QString::number(getMyBetAmount()); 
			}
	
			if (currentGame->getSeatsList()->front()->getMySet()== currentHand->getCurrentBeRo()->getHighestSet() &&  currentGame->getSeatsList()->front()->getMyButton() == 3) { pushButtonCallCheckString = "Check"; }
			else { pushButtonCallCheckString = "Call $"+QString::number(getMyCallAmount()); }
			
			pushButtonFoldString = "Fold"; 

		}
		else { // flop,turn,river

			if (currentHand->getCurrentBeRo()->getHighestSet() == 0 && pushButton_Fold->isCheckable() ) { 
				pushButtonFoldString = "Check / Fold"; 
			}
			else { pushButtonFoldString = "Fold"; }
			if (currentHand->getCurrentBeRo()->getHighestSet() == 0) { 
	
				pushButtonCallCheckString = "Check";
				pushButtonBetRaiseString = "Bet $"+QString::number(getMyBetAmount());
			}
			if (currentHand->getCurrentBeRo()->getHighestSet() > 0 && currentHand->getCurrentBeRo()->getHighestSet() > currentGame->getSeatsList()->front()->getMySet()) {
				pushButtonCallCheckString = "Call $"+QString::number(getMyCallAmount());
				if (currentGame->getSeatsList()->front()->getMyCash()+currentGame->getSeatsList()->front()->getMySet() > currentHand->getCurrentBeRo()->getHighestSet()) {
					pushButtonBetRaiseString = "Raise $"+QString::number(getMyBetAmount());
				}
			}
		}
		
		if(mode == 0) {
			if( currentHand->getSeatsList()->front()->getMyAction() != PLAYER_ACTION_FOLD ) {
				pushButtonBetRaiseString = "Bet $"+QString::number(getMyBetAmount());
				pushButtonCallCheckString = "Check"; 
				if( currentGame->getActivePlayerList()->size() > 2 && currentGame->getSeatsList()->front()->getMyButton() == BUTTON_SMALL_BLIND || ( currentGame->getActivePlayerList()->size() <= 2 && currentGame->getSeatsList()->front()->getMyButton() == BUTTON_BIG_BLIND)) { pushButtonFoldString = "Fold"; }
				else { pushButtonFoldString = "Check / Fold"; }
			}
			else {
				pushButtonBetRaiseString = "";
				pushButtonCallCheckString = ""; 		
				pushButtonFoldString = "";
				pushButton_AllIn->setDisabled(TRUE);
				horizontalSlider_bet->setDisabled(TRUE);
				lineEdit_betValue->setDisabled(TRUE);
		
				myButtonsCheckable(FALSE);
			}
		}
				
		//if text changed on checked button --> uncheck to prevent unwanted actions
		if((pushButtonCallCheckString != lastPushButtonCallCheckString && pushButton_CallCheck->isChecked())) { 
		
//			cout << "jo" << endl;
			uncheckMyButtons(); 
			resetMyButtonsCheckStateMemory();
		}

		if(pushButtonBetRaiseString == "") {

			pushButton_AllIn->setDisabled(TRUE);
			horizontalSlider_bet->setDisabled(TRUE);
			lineEdit_betValue->setDisabled(TRUE);
		}

		pushButton_Fold->setText(pushButtonFoldString);
		pushButton_BetRaise->setText(pushButtonBetRaiseString);
		pushButton_CallCheck->setText(pushButtonCallCheckString);
		pushButton_AllIn->setText("All-In");

// 		myBetRaise();
		if(pushButton_BetRaise->text().startsWith("Raise")) { 
				
			horizontalSlider_bet->setMinimum(currentHand->getCurrentBeRo()->getHighestSet() - currentHand->getSeatsList()->front()->getMySet() + currentHand->getCurrentBeRo()->getMinimumRaise());
			horizontalSlider_bet->setMaximum(currentHand->getSeatsList()->front()->getMyCash());
			horizontalSlider_bet->setSingleStep(10);
		
			myActionIsRaise = 1;
		}
		else if(pushButton_BetRaise->text().startsWith("Bet")) { 
			
			horizontalSlider_bet->setMinimum(currentHand->getSmallBlind()*2);
			horizontalSlider_bet->setMaximum(currentHand->getSeatsList()->front()->getMyCash());
			horizontalSlider_bet->setSingleStep(10);
		
			myActionIsBet = 1;
		}
		else {}

		
		//if value changed on bet/raise button --> uncheck to prevent unwanted actions
		QString lastBetValueString = lastPushButtonBetRaiseString.section(" ",1 ,1);
		int index = lastBetValueString.indexOf("$");
		lastBetValueString.remove(index,1);
		bool ok;
		int lastBetValue = lastBetValueString.toInt(&ok,10);
		
		if(lastBetValue < horizontalSlider_bet->minimum() && pushButton_BetRaise->isChecked()) {

			uncheckMyButtons(); 
			resetMyButtonsCheckStateMemory();
		}

		if((mySession->getGameType() == Session::GAME_TYPE_INTERNET || mySession->getGameType() == Session::GAME_TYPE_NETWORK) && !lineEdit_ChatInput->hasFocus() && myConfig->readConfigInt("EnableBetInputFocusSwitch")) { 
			lineEdit_betValue->setFocus();
			lineEdit_betValue->selectAll();
		}

		if(mySession->getGameType() == Session::GAME_TYPE_LOCAL) { 
			lineEdit_betValue->setFocus();
			lineEdit_betValue->selectAll();
		}

	}
}

void mainWindowImpl::meInAction() {

	myButtonsCheckable(FALSE);
	
	horizontalSlider_bet->setEnabled(TRUE);
	lineEdit_betValue->setEnabled(TRUE);

	if((mySession->getGameType() == Session::GAME_TYPE_INTERNET || mySession->getGameType() == Session::GAME_TYPE_NETWORK) && lineEdit_ChatInput->text() == "" && myConfig->readConfigInt("EnableBetInputFocusSwitch")) { 
		lineEdit_betValue->setFocus();
		lineEdit_betValue->selectAll();
	}
	
	myActionIsRaise = 0;
	myActionIsBet = 0;
	
	if(myConfig->readConfigInt("ShowStatusbarMessages")) {
		if ( myConfig->readConfigInt("AlternateFKeysUserActionMode") == 0 ) {
			statusBar()->showMessage(tr("F1 - Fold | F2 - Check/Call | F3 - Bet/Raise | F4 - All-In"), 15000);
		} else {
			statusBar()->showMessage(tr("F1 - All-In | F2 - Bet/Raise | F3 - Check/Call | F4 - Fold"), 15000);
		}
	}
		
	QString lastPushButtonFoldString = pushButton_Fold->text();

	//paint actions on buttons
	provideMyActions();
	
	//do remembered action
	if( pushButtonBetRaiseIsChecked ) { pushButton_BetRaise->click(); pushButtonBetRaiseIsChecked = FALSE;}
	if( pushButtonCallCheckIsChecked )  { pushButton_CallCheck->click(); pushButtonCallCheckIsChecked = FALSE;}
	if( pushButtonFoldIsChecked ) { 
		if(lastPushButtonFoldString == "Check / Fold" && pushButton_CallCheck->text() == "Check") {
			pushButton_CallCheck->click();
		}
		else {
			pushButton_Fold->click(); 
		}
		pushButtonFoldIsChecked = FALSE;
	}
	if( pushButtonAllInIsChecked ) { pushButton_AllIn->click(); pushButtonAllInIsChecked = FALSE;}

	//automatic mode
	switch (playingMode) {
		case 0: // Manual mode
			break;
		case 1: // Auto check / call all
			myCallCheck();
			break;
		case 2: // Auto check / fold all
			if (pushButton_CallCheck->text() == "Check") { 
				myCheck();
			} else {
				myFold();
			}
			break;
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

	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();

	//clear userWidgets
	horizontalSlider_bet->setMinimum(0);
	horizontalSlider_bet->setMaximum(currentHand->getSeatsList()->front()->getMyCash());
	lineEdit_betValue->clear();
	horizontalSlider_bet->setValue(0);
	pushButton_AllIn->setDisabled(TRUE);
	horizontalSlider_bet->setDisabled(TRUE);
	lineEdit_betValue->setDisabled(TRUE);
}

void mainWindowImpl::myCallCheck() {

	if(pushButton_CallCheck->text().startsWith("Call")) { myCall(); }
	if(pushButton_CallCheck->text() == "Check") { myCheck(); }	
}

void mainWindowImpl::myFold(){

	if(pushButton_Fold->text() == "Fold") {

		HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();
		currentHand->getSeatsList()->front()->setMyAction(1);
		currentHand->getSeatsList()->front()->setMyTurn(0);
	
		//set that i was the last active player. need this for unhighlighting groupbox
		currentHand->setLastPlayersTurn(0);
		
		statusBar()->clearMessage();
	
		//Spiel läuft weiter
		myActionDone();
	}
}

void mainWindowImpl::myCheck() {
	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();
	
	currentHand->getSeatsList()->front()->setMyTurn(0);
	currentHand->getSeatsList()->front()->setMyAction(2);

	//set that i was the last active player. need this for unhighlighting groupbox
	currentHand->setLastPlayersTurn(0);

	statusBar()->clearMessage();

	//Spiel läuft weiter
	myActionDone();
}

int mainWindowImpl::getMyCallAmount() {
        int tempHighestSet = 0;
        HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();

        tempHighestSet = currentHand->getCurrentBeRo()->getHighestSet();

        if (currentHand->getSeatsList()->front()->getMyCash()+currentHand->getSeatsList()->front()->getMySet() <= tempHighestSet) {

                return currentHand->getSeatsList()->front()->getMyCash();
        }
        else {
                return tempHighestSet - currentHand->getSeatsList()->front()->getMySet();
        }
}

int mainWindowImpl::getBetRaisePushButtonValue() {

	QString betValueString = pushButton_BetRaise->text().section(" ",1 ,1);
	int index = betValueString.indexOf("$");
	betValueString.remove(index,1);
	bool ok;
	int betValue = betValueString.toInt(&ok,10);
	
	return betValue;
}

int mainWindowImpl::getMyBetAmount() {

	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();
	
	int betValue = getBetRaisePushButtonValue();
	int minimum;
	
	minimum = currentHand->getCurrentBeRo()->getHighestSet() - currentHand->getSeatsList()->front()->getMySet() + currentHand->getCurrentBeRo()->getMinimumRaise();

	if(betValue < minimum) {
		return minimum;
	}
	else {
		return betValue;
	}
}

void mainWindowImpl::myCall(){

	int tempHighestSet = 0;
	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();
	
	tempHighestSet = currentHand->getCurrentBeRo()->getHighestSet();

	if (currentHand->getSeatsList()->front()->getMyCash()+currentHand->getSeatsList()->front()->getMySet() <= tempHighestSet) {

		currentHand->getSeatsList()->front()->setMySet(currentHand->getSeatsList()->front()->getMyCash());
		currentHand->getSeatsList()->front()->setMyCash(0);
		currentHand->getSeatsList()->front()->setMyAction(6);
	}
	else {	
		currentHand->getSeatsList()->front()->setMySet(tempHighestSet - currentHand->getSeatsList()->front()->getMySet());
		currentHand->getSeatsList()->front()->setMyAction(3);
	}
	currentHand->getSeatsList()->front()->setMyTurn(0);

	currentHand->getBoard()->collectSets();
	refreshPot();

	//set that i was the last active player. need this for unhighlighting groupbox
	currentHand->setLastPlayersTurn(0);

	statusBar()->clearMessage();

	//Spiel läuft weiter
	myActionDone();
}

void mainWindowImpl::mySet(){
	
	if(pushButton_BetRaise->text() != "") {

		HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();
		int tempCash = currentHand->getSeatsList()->front()->getMyCash();
	
// 		cout << "Set-Value " << getBetRaisePushButtonValue() << endl; 
		currentHand->getSeatsList()->front()->setMySet(getBetRaisePushButtonValue());

		if (getBetRaisePushButtonValue() >= tempCash ) {
	
			currentHand->getSeatsList()->front()->setMySet(currentHand->getSeatsList()->front()->getMyCash());
			currentHand->getSeatsList()->front()->setMyCash(0);
			currentHand->getSeatsList()->front()->setMyAction(6);
		}
		
		if(myActionIsRaise) {
			//do not if allIn
			if(currentHand->getSeatsList()->front()->getMyAction() != 6) {
				currentHand->getSeatsList()->front()->setMyAction(5);
			}
			myActionIsRaise = 0;
	
			currentHand->getCurrentBeRo()->setMinimumRaise(currentHand->getSeatsList()->front()->getMySet() - currentHand->getCurrentBeRo()->getHighestSet());
		}
		
		if(myActionIsBet) {
			//do not if allIn
			if(currentHand->getSeatsList()->front()->getMyAction() != 6) {
				currentHand->getSeatsList()->front()->setMyAction(4);
			}		
			myActionIsBet = 0;
	
			currentHand->getCurrentBeRo()->setMinimumRaise(currentHand->getSeatsList()->front()->getMySet());
		}
	
		currentHand->getCurrentBeRo()->setHighestSet(currentHand->getSeatsList()->front()->getMySet());
	
		currentHand->getSeatsList()->front()->setMyTurn(0);
	
		currentHand->getBoard()->collectSets();
		refreshPot();
	
		statusBar()->clearMessage();
	
		//set that i was the last active player. need this for unhighlighting groupbox
		currentHand->setLastPlayersTurn(0);
	
		//Spiel läuft weiter
		myActionDone();
	}
}

void mainWindowImpl::myAllIn(){

	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();

	currentHand->getSeatsList()->front()->setMySet(currentHand->getSeatsList()->front()->getMyCash());
	currentHand->getSeatsList()->front()->setMyCash(0);
	currentHand->getSeatsList()->front()->setMyAction(6);
	
	if(currentHand->getSeatsList()->front()->getMySet() > currentHand->getCurrentBeRo()->getHighestSet()) {
		currentHand->getCurrentBeRo()->setMinimumRaise(currentHand->getSeatsList()->front()->getMySet() - currentHand->getCurrentBeRo()->getHighestSet());

		currentHand->getCurrentBeRo()->setHighestSet(currentHand->getSeatsList()->front()->getMySet());

	}

	currentHand->getSeatsList()->front()->setMyTurn(0);

	currentHand->getBoard()->collectSets();
	refreshPot();
	
	statusBar()->clearMessage();

	//set that i was the last active player. need this for unhighlighting groupbox
	currentHand->setLastPlayersTurn(0);

	//Spiel läuft weiter
	myActionDone();
}


void mainWindowImpl::pushButtonBetRaiseClicked(bool checked) {

	if (pushButton_BetRaise->isCheckable()) {
		if(checked) {
			pushButton_CallCheck->setChecked(FALSE);
			pushButton_Fold->setChecked(FALSE);
			pushButton_AllIn->setChecked(FALSE);
			
			pushButtonCallCheckIsChecked = FALSE;
			pushButtonFoldIsChecked = FALSE;
			pushButtonAllInIsChecked = FALSE;
		
			pushButtonBetRaiseIsChecked = TRUE;

			if(!radioButton_manualAction->isChecked())
				radioButton_manualAction->click();

// 			myLastPreActionBetValue = lineEdit_betValue->value();

		}
		else {
			pushButtonBetRaiseIsChecked = FALSE;
			myLastPreActionBetValue = 0;
		}
	}
	else {
		mySet();
	}
}

void mainWindowImpl::pushButtonCallCheckClicked(bool checked) {

	if (pushButton_CallCheck->isCheckable()) {
		if(checked) {
			pushButton_Fold->setChecked(FALSE);
			pushButton_BetRaise->setChecked(FALSE);
			pushButton_AllIn->setChecked(FALSE);

			pushButtonAllInIsChecked = FALSE;
			pushButtonFoldIsChecked = FALSE;
			pushButtonBetRaiseIsChecked = FALSE;
		
			pushButtonCallCheckIsChecked = TRUE;
			
			if(!radioButton_manualAction->isChecked())
				radioButton_manualAction->click();
		}
		else {
			pushButtonCallCheckIsChecked = FALSE;
		}
	}
	else {
		myCallCheck();
	}
}

void mainWindowImpl::pushButtonFoldClicked(bool checked) {

	if (pushButton_Fold->isCheckable()) {
		if(checked) {
			pushButton_CallCheck->setChecked(FALSE);
			pushButton_BetRaise->setChecked(FALSE);
			pushButton_AllIn->setChecked(FALSE);
			
			pushButtonAllInIsChecked = FALSE;
			pushButtonCallCheckIsChecked = FALSE;
			pushButtonBetRaiseIsChecked = FALSE;
		
			pushButtonFoldIsChecked = TRUE;
			
			if(!radioButton_manualAction->isChecked())
				radioButton_manualAction->click();
		}
		else {
			pushButtonFoldIsChecked = FALSE;
		}
	}
	else {
		myFold();
	}
}

void mainWindowImpl::pushButtonAllInClicked(bool checked) {

	if (pushButton_AllIn->isCheckable()) {
		if(checked) {
			pushButton_CallCheck->setChecked(FALSE);
			pushButton_BetRaise->setChecked(FALSE);
			pushButton_Fold->setChecked(FALSE);

			pushButtonFoldIsChecked = FALSE;
			pushButtonCallCheckIsChecked = FALSE;
			pushButtonBetRaiseIsChecked = FALSE;
		
			pushButtonAllInIsChecked = TRUE;

			if(!radioButton_manualAction->isChecked())
				radioButton_manualAction->click();
		}
		else {
			pushButtonAllInIsChecked = FALSE;
		}
	}
	else {
		myAllIn();
	}
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

	PlayerListConstIterator it_c;
	for(it_c=currentHand->getSeatsList()->begin(); it_c!=currentHand->getSeatsList()->end(); it_c++) {
		if((*it_c)->getMyID() == currentHand->getLastPlayersTurn()) break;
	}

	if(currentHand->getLastPlayersTurn() != -1) {
		refreshAction(currentHand->getLastPlayersTurn(), (*it_c)->getMyAction());
	}
	refreshCash();

	//refresh actions for human player
	updateMyButtonsState();

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
	
	uncheckMyButtons();
	myButtonsCheckable(FALSE);
	clearMyButtons();
	resetMyButtonsCheckStateMemory();

	pushButton_AllIn->setDisabled(TRUE);
	horizontalSlider_bet->setDisabled(TRUE);
	lineEdit_betValue->setDisabled(TRUE);

	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();

	int nonfoldPlayersCounter = 0;
	PlayerListConstIterator it_c;
	for (it_c=currentHand->getActivePlayerList()->begin(); it_c!=currentHand->getActivePlayerList()->end(); it_c++) { 
		if ((*it_c)->getMyAction() != PLAYER_ACTION_FOLD)
			nonfoldPlayersCounter++;
	}

	if(nonfoldPlayersCounter!=1) { 
		 
		if(!flipHolecardsAllInAlreadyDone) {

//TODO - Turn cards like in the rules

// 			postRiverRunAnimation2_flipHoleCards1Timer->start(nextPlayerSpeed2);

// 			//Config? mit oder ohne Eye-Candy?
			if(myConfig->readConfigInt("ShowFlipCardsAnimation")) { 
				// mit Eye-Candy
		
				//TempArrays
				int tempCardsIntArray[2];
		
				int j;
				
				for (it_c=currentHand->getActivePlayerList()->begin(); it_c!=currentHand->getActivePlayerList()->end(); it_c++) { 
					(*it_c)->getMyCards(tempCardsIntArray);	
					if((*it_c)->getMyAction() != PLAYER_ACTION_FOLD) { 
						if((*it_c)->getMyID() || ((*it_c)->getMyID()==0 && myConfig->readConfigInt("AntiPeekMode")) ) {
							for(j=0; j<2; j++) {
												
								holeCardsArray[(*it_c)->getMyID()][j]->startFlipCards(guiGameSpeed, QPixmap(myAppDataPath +"gfx/cards/default/"+QString::number(tempCardsIntArray[j], 10)+".png"), *flipside);
							}	
						}
						//set Player value (logging)
						(*it_c)->setMyCardsFlip(1,1);
					}
				}	
			}
			else {
				//without Eye-Candy		
			
				//Karten der aktiven Spieler umdrehen
				QPixmap onePix(myAppDataPath +"gfx/gui/misc/1px.png");
			
				//TempArrays
				QPixmap tempCardsPixmapArray[2];
				int tempCardsIntArray[2];
			
				int j;
					for (it_c=currentHand->getActivePlayerList()->begin(); it_c!=currentHand->getActivePlayerList()->end(); it_c++) { 
					(*it_c)->getMyCards(tempCardsIntArray);	
					if((*it_c)->getMyAction() != PLAYER_ACTION_FOLD) { 
						if((*it_c)->getMyID() || ((*it_c)->getMyID()==0 && myConfig->readConfigInt("AntiPeekMode")) ) {
							for(j=0; j<2; j++) {		
								tempCardsPixmapArray[j].load(myAppDataPath +"gfx/cards/default/"+QString::number(tempCardsIntArray[j], 10)+".png");
								holeCardsArray[(*it_c)->getMyID()][j]->setPixmap(tempCardsPixmapArray[j], FALSE);
								
							}	
						}
						//set Player value (logging)
						(*it_c)->setMyCardsFlip(1,1);
					}
				}
			}
		//Wenn einmal umgedreht dann fertig!!	
		flipHolecardsAllInAlreadyDone = TRUE;
		}
		else {
			int tempCardsIntArray[2];
// 			int i;
				for (it_c=currentHand->getActivePlayerList()->begin(); it_c!=currentHand->getActivePlayerList()->end(); it_c++) { 
				(*it_c)->getMyCards(tempCardsIntArray);	
					if((*it_c)->getMyAction() != PLAYER_ACTION_FOLD) { 
				
					//set Player value (logging)
					(*it_c)->setMyCardsFlip(1,3);
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

	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();
	//Alle Winner erhellen und "Winner" schreiben

	int nonfoldPlayerCounter = 0;
	PlayerListConstIterator it_c;

	for(it_c=currentHand->getActivePlayerList()->begin(); it_c!=currentHand->getActivePlayerList()->end(); it_c++) {
		if((*it_c)->getMyAction() != PLAYER_ACTION_FOLD) {
			nonfoldPlayerCounter++;
		}
	} 


	list<unsigned> winners = currentHand->getBoard()->getWinners();


	for(it_c=currentHand->getActivePlayerList()->begin(); it_c!=currentHand->getActivePlayerList()->end(); it_c++) {
		if((*it_c)->getMyAction() != PLAYER_ACTION_FOLD && (*it_c)->getMyCardsValueInt() == currentHand->getCurrentBeRo()->getHighestCardsValue() ) { 

// 			QPalette tempPalette = groupBoxArray[i]->palette();
// 			tempPalette.setColor(QPalette::Window, highlight);
// 			groupBoxArray[i]->setPalette(tempPalette);
			actionLabelArray[(*it_c)->getMyID()]->setPixmap(QPixmap(myAppDataPath +"gfx/gui/table/default/action_winner.png"));

			//show winnercards if more than one player is active TODO
			if ( nonfoldPlayerCounter != 1 && myConfig->readConfigInt("ShowFadeOutCardsAnimation")) {

				int j;
				int bestHandPos[5];
				(*it_c)->getMyBestHandPosition(bestHandPos);

				//index 0 testen --> Karte darf nicht im MyBestHand Position Array drin sein, es darf nicht nur ein Spieler Aktiv sein, die Config fordert die Animation
				bool index0 = TRUE;
				for(j=0; j<5; j++) {			
					if (bestHandPos[j] == 0 ) { index0 = FALSE; }
				}
				if (index0) { holeCardsArray[(*it_c)->getMyID()][0]->startFadeOut(guiGameSpeed); /*cout << "Fade Out index0" << endl;*/}
				//index 1 testen
				bool index1 = TRUE;
				for(j=0; j<5; j++) {
					if (bestHandPos[j] == 1 ) { index1 = FALSE; }
				}
				if (index1) { holeCardsArray[(*it_c)->getMyID()][1]->startFadeOut(guiGameSpeed); /*cout << "Fade Out index1" << endl;*/}
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
			int pot = (*it_c)->getLastMoneyWon();
			//Wenn River dann auch das Blatt loggen!
// 			if (textLabel_handLabel->text() == "River") {

			//set Player value (logging)
			(*it_c)->setMyWinnerState(true, pot);

// 			}
// 			else {
// 				myLog->logPlayerWinsMsg(i, pot);
// 			}
		}
		else {

			if( currentHand->getActivePlayerList()->size() != 1 && (*it_c)->getMyAction() != PLAYER_ACTION_FOLD && myConfig->readConfigInt("ShowFadeOutCardsAnimation") ) {
    	
			//aufgedeckte Gegner auch ausblenden
				holeCardsArray[(*it_c)->getMyID()][0]->startFadeOut(guiGameSpeed);
				holeCardsArray[(*it_c)->getMyID()][1]->startFadeOut(guiGameSpeed);
			}
		}
	}

	// log side pot winners -> TODO
	list<unsigned>::iterator it_int;
	for(it_c=currentHand->getActivePlayerList()->begin(); it_c!=currentHand->getActivePlayerList()->end(); it_c++) {
		if((*it_c)->getMyAction() != PLAYER_ACTION_FOLD && (*it_c)->getMyCardsValueInt() != currentHand->getCurrentBeRo()->getHighestCardsValue() ) { 

			for(it_int = winners.begin(); it_int != winners.end(); it_int++) {
				if((*it_int) == (*it_c)->getMyUniqueID()) {
					int pot = (*it_c)->getLastMoneyWon();
					(*it_c)->setMyWinnerState(false, pot);
				}
			}

		}
	}

	for(it_c=currentHand->getActivePlayerList()->begin(); it_c!=currentHand->getActivePlayerList()->end(); it_c++) {
		if((*it_c)->getMyCash() == 0) {
			currentHand->getGuiInterface()->logPlayerSitsOut((*it_c)->getMyName());
		}
	}

	textBrowser_Log->append("");

	postRiverRunAnimation3Timer->start(postRiverRunAnimationSpeed/2);
}

void mainWindowImpl::postRiverRunAnimation4() {

	distributePotAnimCounter=0;
	potDistributeTimer->start(winnerBlinkSpeed);
}

void mainWindowImpl::postRiverRunAnimation5() {

	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();

	PlayerListConstIterator it_c;

	if (distributePotAnimCounter<10) {
		
		if (distributePotAnimCounter==0 || distributePotAnimCounter==2 || distributePotAnimCounter==4 || distributePotAnimCounter==6 || distributePotAnimCounter==8) { 

			label_Pot->setText("");
	
			for(it_c=currentHand->getActivePlayerList()->begin(); it_c!=currentHand->getActivePlayerList()->end(); it_c++) {
				if((*it_c)->getMyAction() != PLAYER_ACTION_FOLD && (*it_c)->getMyCardsValueInt() == currentHand->getCurrentBeRo()->getHighestCardsValue() ) { 

					cashTopLabelArray[(*it_c)->getMyID()]->setText("");
				}
			}
		}
		else { 
			label_Pot->setText("Pot");

			for(it_c=currentHand->getActivePlayerList()->begin(); it_c!=currentHand->getActivePlayerList()->end(); it_c++) {
				if((*it_c)->getMyAction() != PLAYER_ACTION_FOLD && (*it_c)->getMyCardsValueInt() == currentHand->getCurrentBeRo()->getHighestCardsValue() ) { 

					cashTopLabelArray[(*it_c)->getMyID()]->setText("Cash:"); 
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

// 	int i;
	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();

	refreshCash();
	refreshPot();

	// TODO HACK
	// Check for network client, do not start new hand if client is running.
	if (mySession->isNetworkClientRunning())
		return;

	// wenn nur noch ein Spieler aktive "neues Spiel"-Dialog anzeigen
	int playersPositiveCashCounter = 0;

	PlayerListConstIterator it_c;
	for (it_c=currentHand->getActivePlayerList()->begin(); it_c!=currentHand->getActivePlayerList()->end(); it_c++) { 

		if ((*it_c)->getMyCash() > 0) playersPositiveCashCounter++;
	}

	if (playersPositiveCashCounter==1) {

		for (it_c=currentHand->getActivePlayerList()->begin(); it_c!=currentHand->getActivePlayerList()->end(); it_c++) { 
	
			if ((*it_c)->getMyCash() > 0) {
				currentHand->getGuiInterface()->logPlayerWinGame((*it_c)->getMyName(),  mySession->getCurrentGame()->getMyGameID());
			}
		}

		if( !DEBUG_MODE ) {

			if(mySession->getGameType() == Session::GAME_TYPE_LOCAL) {
				currentGameOver = TRUE;
				pushButton_break->setDisabled(FALSE);
				QFontMetrics tempMetrics = this->fontMetrics();
				int width = tempMetrics.width(tr("Start"));
				pushButton_break->setMinimumSize(width+10,20);
				pushButton_break->setText(tr("Start"));
				blinkingStartButtonAnimationTimer->start(500);		
			}
		}
		else {
			callNewGameDialog();	
			//Bei Cancel nichts machen!!!
		}
		return;
	} 
	
	postRiverRunAnimation6Timer->start(newRoundSpeed);
}

void mainWindowImpl::flipHolecardsAllIn() {

	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();

	if(!flipHolecardsAllInAlreadyDone) {
		//Aktive Spieler zählen --> wenn nur noch einer nicht-folded dann keine Karten umdrehen
		int nonfoldPlayersCounter = 0;
		PlayerListConstIterator it_c;
		for (it_c=currentHand->getActivePlayerList()->begin(); it_c!=currentHand->getActivePlayerList()->end(); it_c++) { 
			if ((*it_c)->getMyAction() != PLAYER_ACTION_FOLD) nonfoldPlayersCounter++;
		}
		
		if(nonfoldPlayersCounter!=1) { 
			
			//Config? mit oder ohne Eye-Candy?
			if(myConfig->readConfigInt("ShowFlipCardsAnimation")) { 
				// mit Eye-Candy
	
				//TempArrays
				int tempCardsIntArray[2];
	
				int j;
	
				for (it_c=currentHand->getActivePlayerList()->begin(); it_c!=currentHand->getActivePlayerList()->end(); it_c++) { 
					(*it_c)->getMyCards(tempCardsIntArray);	
					if((*it_c)->getMyAction() != PLAYER_ACTION_FOLD) { 
						if((*it_c)->getMyID() || ((*it_c)->getMyID()==0 && myConfig->readConfigInt("AntiPeekMode")) ) {
							for(j=0; j<2; j++) {
								holeCardsArray[(*it_c)->getMyID()][j]->startFlipCards(guiGameSpeed, QPixmap(myAppDataPath +"gfx/cards/default/"+QString::number(tempCardsIntArray[j], 10)+".png"), *flipside);
							}
						}
						//set Player value (logging)
						(*it_c)->setMyCardsFlip(1,2);
						
					}
				}
			}
			else {
				//without Eye-Candy		
		
				//Karten der aktiven Spieler umdrehen
				QPixmap onePix(myAppDataPath +"gfx/gui/misc/1px.png");
				
				//TempArrays
				QPixmap tempCardsPixmapArray[2];
				int temp2CardsIntArray[2];
				
				int j;
				for (it_c=currentHand->getActivePlayerList()->begin(); it_c!=currentHand->getActivePlayerList()->end(); it_c++) { 
					(*it_c)->getMyCards(temp2CardsIntArray);	
					if((*it_c)->getMyAction() != PLAYER_ACTION_FOLD) { 
						if((*it_c)->getMyID() || ((*it_c)->getMyID()==0 && myConfig->readConfigInt("AntiPeekMode")) ) {
							for(j=0; j<2; j++) {
								
								tempCardsPixmapArray[j].load(myAppDataPath +"gfx/cards/default/"+QString::number(temp2CardsIntArray[j], 10)+".png");
								holeCardsArray[(*it_c)->getMyID()][j]->setPixmap(tempCardsPixmapArray[j], FALSE);
							}	
						}
						//set Player value (logging)
						(*it_c)->setMyCardsFlip(1,2);
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

	currentHand->getSeatsList()->front()->getMyCards(tempCardsIntArray);	
	if( currentHand->getSeatsList()->front()->getMyCardsFlip() == 0 &&  currentHand->getCurrentRound() == 4 && currentHand->getSeatsList()->front()->getMyActiveStatus() && currentHand->getSeatsList()->front()->getMyAction() != PLAYER_ACTION_FOLD) { 

		//set Player value (logging)	
		currentHand->getSeatsList()->front()->setMyCardsFlip(1,1);
	}
}


void mainWindowImpl::startNewHand() {

	if( !breakAfterCurrentHand){
		mySession->getCurrentGame()->initHand();
		mySession->getCurrentGame()->startHand();
	}
	else { 

		if(mySession->getGameType() == Session::GAME_TYPE_LOCAL) {
			pushButton_break->setDisabled(FALSE);
			
			QFontMetrics tempMetrics = this->fontMetrics();
			int width = tempMetrics.width(tr("Start"));
			pushButton_break->setMinimumSize(width+10,20);
	
			pushButton_break->setText(tr("Start"));
			breakAfterCurrentHand=FALSE;
	
			blinkingStartButtonAnimationTimer->start(500);		
		}
	}
}

void mainWindowImpl::handSwitchRounds() { mySession->getCurrentGame()->getCurrentHand()->switchRounds(); 
}

void mainWindowImpl::nextRoundCleanGui() {

	int i,j;

	// GUI bereinigen - Bilder löschen, Animationen unterbrechen
	QPixmap onePix(myAppDataPath +"gfx/gui/misc/1px.png");
	for (i=0; i<5; i++ ) { 
		boardCardsArray[i]->setPixmap(onePix, FALSE); 
		boardCardsArray[i]->setFadeOutAction(FALSE); 
		boardCardsArray[i]->stopFlipCardsAnimation();
		setLabelArray[i]->stopTimeOutAnimation();
		
	}
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++ ) { 
		for ( j=0; j<=1; j++ ) { holeCardsArray[i][j]->setFadeOutAction(FALSE);}
	}

	// for startNewGame during human player is active
	if(mySession->getCurrentGame()->getCurrentHand()->getSeatsList()->front()->getMyActiveStatus() == 1) {
		disableMyButtons();
	}

	textLabel_handLabel->setText("");
	
	refreshAll();

	flipHolecardsAllInAlreadyDone = FALSE;

	//Wenn Pause zwischen den Hands in der Konfiguration steht den Stop Button drücken!
	if (myConfig->readConfigInt("PauseBetweenHands") && blinkingStartButtonAnimationTimer->isActive() == FALSE && mySession->getGameType() == Session::GAME_TYPE_LOCAL) { 
		pushButton_break->click(); 
	}
	else { 
		//FIX STRG+N Bug
		pushButton_break->setEnabled(TRUE); 
		breakAfterCurrentHand=FALSE;
	}
	
	//Clean breakbutton
	if(mySession->getGameType() == Session::GAME_TYPE_LOCAL) {
		blinkingStartButtonAnimationTimer->stop();
		pushButton_break->setStyleSheet("QPushButton { background-color: #145300; color: white;}");
		blinkingStartButtonAnimationTimer->stop();
		QFontMetrics tempMetrics = this->fontMetrics();
		int width = tempMetrics.width(tr("Stop"));
		pushButton_break->setMinimumSize(width+10,20);
		pushButton_break->setText(tr("Stop"));
	}
	//Clear Statusbarmessage
	statusBar()->clearMessage();

	//fix press mouse button during bankrupt with anti-peek-mode
	this->mouseOverFlipCards(FALSE);

	pushButton_AllIn->setDisabled(TRUE);
	horizontalSlider_bet->setDisabled(TRUE);
	lineEdit_betValue->setDisabled(TRUE);

	uncheckMyButtons();
	myButtonsCheckable(FALSE);
	resetMyButtonsCheckStateMemory();
	clearMyButtons();
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
		breakAfterCurrentHand=TRUE;
	}
	else if (pushButton_break->text() == tr("Lobby")) {
		leaveCurrentNetworkGame();
	}
	else if (pushButton_break->text() == tr("Start")) { 

		blinkingStartButtonAnimationTimer->stop();
		//Set default Color
		pushButton_break->setStyleSheet("QPushButton { background-color: #145300; color: white;}");
		QFontMetrics tempMetrics = this->fontMetrics();
		int width = tempMetrics.width(tr("Stop"));
		pushButton_break->setMinimumSize(width+10,20);

		pushButton_break->setText(tr("Stop"));

		if(currentGameOver) {
			currentGameOver = FALSE;
			callNewGameDialog();	
			//Bei Cancel nichts machen!!!
		}
		else {
			startNewHand();
		}
	}
}

void mainWindowImpl::paintStartSplash() {

	StartSplash *mySplash = new StartSplash(this, myConfig);	

#ifdef __APPLE__
  int offset = 305;
#else
  int offset = 237;
#endif
        mySplash->setGeometry(this->pos().x()+offset,this->pos().y()+210,400,250);
//         mySplash->setWindowFlags(Qt::SplashScreen);
        mySplash->show();
}


void mainWindowImpl::networkError(int errorID, int /*osErrorID*/) {

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
			{	QMessageBox msgBox(QMessageBox::Warning, tr("Network Error"),
				tr("The PokerTH server does not support this version of the game.<br>Please go to <a href=\"http://www.pokerth.net/\" target=\"_blank\">http://www.pokerth.net</a> and download the latest version."),
				QMessageBox::Close, this); 
				msgBox.setTextFormat(Qt::RichText);
				msgBox.exec();
			}
		break;
		case ERR_NET_SERVER_FULL:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("Sorry, this server is already full."),
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
			{ myChangeHumanPlayerNameDialog->label_Message->setText(tr("The player name is too short, too long or invalid. Please choose another one."));
			  myChangeHumanPlayerNameDialog->exec(); }
		break;
		case ERR_NET_INVALID_GAME_NAME:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("The game name is either too short or too long. Please choose another one."),
				QMessageBox::Close); }
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
		case ERR_NET_NO_CURRENT_PLAYER:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("Internal error: The current player could not be found."),
				QMessageBox::Close); }
		break;
		case ERR_NET_PLAYER_NOT_ACTIVE:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("Internal error: The current player is not active."),
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
		case ERR_NET_TOO_MANY_MANUAL_BLINDS:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("Too many manual blinds were set. Please reconfigure the manual blinds."),
				QMessageBox::Close); }
		break;
		case ERR_NET_INVALID_AVATAR_FILE:
		case ERR_NET_WRONG_AVATAR_SIZE:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("An invalid avatar file was configured. Please choose a different avatar."),
				QMessageBox::Close); }
		break;
		case ERR_NET_AVATAR_TOO_LARGE:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("The selected avatar file is too large. Please choose a different avatar."),
				QMessageBox::Close); }
		break;
		case ERR_NET_INVALID_REQUEST_ID:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("An internal avatar error occured. Please report this to an admin in the lobby chat."),
				QMessageBox::Close); }
		break;
		case ERR_NET_START_TIMEOUT:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("Could not start game: Synchronization failed."),
				QMessageBox::Close); }
		break;
		case ERR_NET_SERVER_MAINTENANCE:
			{ QMessageBox::warning(this, tr("Network Error"),
				tr("The server is down for maintenance. Please try again later."),
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

void mainWindowImpl::networkNotification(int notificationId)
{
	switch (notificationId)
	{
		case NTF_NET_REMOVED_KICKED:
			{ QMessageBox::warning(this, tr("Network Notification"),
				tr("You were kicked from the game."),
				QMessageBox::Close); }
		break;
		case NTF_NET_REMOVED_GAME_FULL:
		case NTF_NET_JOIN_GAME_FULL:
			{ QMessageBox::warning(this, tr("Network Notification"),
				tr("Sorry, this game is already full."),
				QMessageBox::Close); }
		break;
		case NTF_NET_REMOVED_ALREADY_RUNNING:
		case NTF_NET_JOIN_ALREADY_RUNNING:
			{ QMessageBox::warning(this, tr("Network Notification"),
				tr("Unable to join - the server has already started the game."),
				QMessageBox::Close); }
		break;
		case NTF_NET_JOIN_INVALID_PASSWORD:
			{ QMessageBox::warning(this, tr("Network Notification"),
				tr("Invalid password when joining the game.\nPlease reenter the password and try again."),
				QMessageBox::Close); }
		break;
		case NTF_NET_NEW_RELEASE_AVAILABLE:
			{	QMessageBox msgBox(QMessageBox::Information, tr("Network Notification"),
				tr("A new release of PokerTH is available.<br>Please go to <a href=\"http://www.pokerth.net/\" target=\"_blank\">http://www.pokerth.net</a> and download the latest version."),
				QMessageBox::Close, this); 
				msgBox.setTextFormat(Qt::RichText);
				msgBox.exec();
			}
		break;
		case NTF_NET_OUTDATED_BETA:
			{	QMessageBox msgBox(QMessageBox::Information, tr("Network Notification"),
				tr("This beta release of PokerTH is outdated.<br>Please go to <a href=\"http://www.pokerth.net/\" target=\"_blank\">http://www.pokerth.net</a> and download the latest version."),
				QMessageBox::Close, this); 
				msgBox.setTextFormat(Qt::RichText);
				msgBox.exec();
			}
		break;
	}
}

void mainWindowImpl::networkStart(boost::shared_ptr<Game> game)
{
	mySession->startClientGame(game);

	//send playerNicksList to chat for nick-autocompletition
	myChat->setPlayerNicksList(getPlayerNicksList());
}

void mainWindowImpl::keyPressEvent ( QKeyEvent * event ) {

// 	cout << event->key() << endl;
	
	bool ctrlPressed = FALSE;

	if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return )  /*ENTER*/  { 
		if(lineEdit_betValue->hasFocus()) {
			pushButton_BetRaise->click();
		}
	}
        if (event->key() == Qt::Key_F1) {
		if (myConfig->readConfigInt("AlternateFKeysUserActionMode") == 0) {
			pushButton_Fold->click();
		} else {
			pushButton_AllIn->click();
		}
	}
	if (event->key() == Qt::Key_F2) { 
		if (myConfig->readConfigInt("AlternateFKeysUserActionMode") == 0) {
			pushButton_CallCheck->click();
		} else {
			pushButton_BetRaise->click();
		}

	} 
	if (event->key() == Qt::Key_F3 ) {
		if (myConfig->readConfigInt("AlternateFKeysUserActionMode") == 0) {
			pushButton_BetRaise->click();
		} else {
			pushButton_CallCheck->click();
		}
	}
	if (event->key() == Qt::Key_F4) {
		if (myConfig->readConfigInt("AlternateFKeysUserActionMode") == 0) {
			pushButton_AllIn->click();
		} else {
			pushButton_Fold->click();
		}
	}
	if (event->key() == Qt::Key_F5) { radioButton_manualAction->click(); }
 	if (event->key() == Qt::Key_F6) { radioButton_autoCheckFold->click(); }
  	if (event->key() == Qt::Key_F7) { radioButton_autoCheckCallAny->click(); }
	if (event->key() == 16777249) {  //CTRL
		if(mySession->getGameType() == Session::GAME_TYPE_LOCAL) {
			pushButton_break->click(); 
			ctrlPressed = TRUE;
		}
	} 
	if (event->key() == Qt::Key_Escape && (myActionIsBet || myActionIsRaise)) { 
		meInAction(); 
	} 
	if (event->key() == Qt::Key_Up && lineEdit_ChatInput->hasFocus()) { 
		if((keyUpDownChatCounter + 1) <= myChat->getChatLinesHistorySize()) { keyUpDownChatCounter++; }
// 		std::cout << "Up keyUpDownChatCounter: " << keyUpDownChatCounter << "\n";
		myChat->showChatHistoryIndex(keyUpDownChatCounter); 
	}
	else if(event->key() == Qt::Key_Down && lineEdit_ChatInput->hasFocus()) { 
		if((keyUpDownChatCounter - 1) >= 0) { keyUpDownChatCounter--; }
// 		std::cout << "Down keyUpDownChatCounter: " << keyUpDownChatCounter << "\n";
		myChat->showChatHistoryIndex(keyUpDownChatCounter); 
	}
	else { keyUpDownChatCounter = 0; }
	

}

void mainWindowImpl::changePlayingMode() {

	int mode = -1;

	if(radioButton_manualAction->isChecked()) { mode=0; }
	if(radioButton_autoCheckFold->isChecked()) { mode=2; }
	if(radioButton_autoCheckCallAny->isChecked()) { mode=1; }
	

	switch (mode) {

		case 0: { statusBar()->showMessage(tr("Manual mode set. You've got to choose yourself now."), 5000); }
		break;
		case 1: { statusBar()->showMessage(tr("Auto mode set: Check or call any."), 5000); }
		break;
		case 2: { statusBar()->showMessage(tr("Auto mode set: Check or fold."), 5000); }
		break;
		default: { cout << "changePlayingMode ERROR!!!!" << endl; }

	}

	playingMode = mode;
}

bool mainWindowImpl::eventFilter(QObject *obj, QEvent *event)
{
	QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

	if (/*obj == lineEdit_ChatInput && lineEdit_ChatInput->text() != "" && */event->type() == QEvent::KeyPress && keyEvent->key() == Qt::Key_Tab) {
		myChat->nickAutoCompletition();
		return true;
	}
	else if (event->type() == QEvent::Close) {
		event->ignore();
		quitPokerTH();
		return true; 
	}
	else {
		// pass the event on to the parent class
		return QMainWindow::eventFilter(obj, event);
	}
}

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

	int tab = 0;
	if (groupBox_RightToolBox->isHidden()) { 
		tabWidget_Right->setCurrentIndex(tab);
		groupBox_RightToolBox->show(); 
	}	else {
		if (tabWidget_Right->currentIndex() == tab) {
			groupBox_RightToolBox->hide(); 				
		} else {
			tabWidget_Right->setCurrentIndex(tab);			
		}
	}
}

void mainWindowImpl::switchAwayWindow() {

	int tab = 1;
	if (groupBox_RightToolBox->isHidden()) { 
		tabWidget_Right->setCurrentIndex(tab);
		groupBox_RightToolBox->show(); 
	}	else {
		if (tabWidget_Right->currentIndex() == tab) {
			groupBox_RightToolBox->hide(); 				
		} else {
			tabWidget_Right->setCurrentIndex(tab);			
		}
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
	
	QString style = pushButton_break->styleSheet();

	if(style.contains("QPushButton { background-color: #145300;")) {
		pushButton_break->setStyleSheet("QPushButton { background-color: #6E9E00; color: black;}");
	}
	else {
		pushButton_break->setStyleSheet("QPushButton { background-color: #145300; color: white;}");
	}
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
	tabWidget_Left->removeTab(1);

	int i;
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++ ) { 
		setLabelArray[i]->stopTimeOutAnimation();
	}

	pushButton_break->show();
	QFontMetrics tempMetrics = this->fontMetrics();
	int width = tempMetrics.width(tr("Stop"));
	pushButton_break->setText(tr("Stop"));
	pushButton_break->setMinimumSize(width+10,20);

	//Set the playing mode to "manual"
	radioButton_manualAction->click();

	//clear log
	textBrowser_Log->clear();
}

void mainWindowImpl::networkGameModification() {
	
	if(tabWidget_Left->widget(1) != tab_Chat) 
		tabWidget_Left->insertTab(1, tab_Chat, QString(tr("Chat")));
	
	tabWidget_Left->setCurrentIndex(1);
	myChat->clearNewGame();

	if(mySession->getGameType() == Session::GAME_TYPE_INTERNET) {
		pushButton_break->show();
		QFontMetrics tempMetrics = this->fontMetrics();
		int width = tempMetrics.width(tr("Lobby"));
		pushButton_break->setText(tr("Lobby"));
		pushButton_break->setMinimumSize(width+10,20);
	}
	if(mySession->getGameType() == Session::GAME_TYPE_NETWORK) {

		pushButton_break->hide();
	}
	//Set the playing mode to "manual"
	radioButton_manualAction->click();

	//clear log
	textBrowser_Log->clear();
}

void mainWindowImpl::mouseOverFlipCards(bool front) {

	if(mySession->getCurrentGame()) {
		if(myConfig->readConfigInt("AntiPeekMode") && mySession->getCurrentGame()->getCurrentHand()->getSeatsList()->front()->getMyActiveStatus() && mySession->getCurrentGame()->getSeatsList()->front()->getMyAction() != PLAYER_ACTION_FOLD) {
			holeCardsArray[0][0]->signalFastFlipCards(front);
			holeCardsArray[0][1]->signalFastFlipCards(front);
		}
	}
}

void mainWindowImpl::updateMyButtonsState(int mode) {
	
	HandInterface *currentHand = mySession->getCurrentGame()->getCurrentHand();

	if(currentHand->getLastPlayersTurn() == 0) {
		myButtonsCheckable(FALSE);
		clearMyButtons();
	}
	else {	
		if(mySession->getCurrentGame()->getSeatsList()->front()->getMyAction() != PLAYER_ACTION_ALLIN) { // dont show pre-actions after flip cards when allin
			myButtonsCheckable(TRUE);
			provideMyActions(mode);
		}
	}
}

void mainWindowImpl::uncheckMyButtons() {

	pushButton_BetRaise->setChecked(FALSE);
	pushButton_CallCheck->setChecked(FALSE);
	pushButton_Fold->setChecked(FALSE);
	pushButton_AllIn->setChecked(FALSE);

}

void mainWindowImpl::resetMyButtonsCheckStateMemory() {

	pushButtonCallCheckIsChecked = FALSE;
	pushButtonFoldIsChecked = FALSE;
	pushButtonAllInIsChecked = FALSE;
	pushButtonBetRaiseIsChecked = FALSE;
}

void mainWindowImpl::clearMyButtons() {

	pushButton_BetRaise->setText("");
	pushButton_CallCheck->setText("");
	pushButton_Fold->setText("");
}

void mainWindowImpl::myButtonsCheckable(bool state) {

	if(state) {
		//checkable

		pushButton_BetRaise->setCheckable(TRUE);
		pushButton_CallCheck->setCheckable(TRUE);
		pushButton_Fold->setCheckable(TRUE);
		pushButton_AllIn->setCheckable(TRUE);

		//design
		pushButton_BetRaise->setStyleSheet("QPushButton { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_03_0.6.png); "+ font2String +" font-size: 11px; font-weight: bold; color: #7DFF95;} QPushButton:unchecked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_03_0.6.png); } QPushButton:checked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_03_0.6_checked.png); }");
		pushButton_CallCheck->setStyleSheet("QPushButton { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_05_0.6.png); "+ font2String +" font-size: 11px; font-weight: bold; color: #7DCDFF;} QPushButton:unchecked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_05_0.6.png); } QPushButton:checked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_05_0.6_checked.png); }");
		pushButton_Fold->setStyleSheet("QPushButton { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_07_0.6.png); "+ font2String +" font-size: 11px; font-weight: bold; color: #FF7D7D;}  QPushButton:unchecked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_07_0.6.png); } QPushButton:checked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_07_0.6_checked.png); }");

		myButtonsAreCheckable = TRUE;
	}
	else {
		//not checkable

		pushButton_BetRaise->setCheckable(FALSE);
		pushButton_CallCheck->setCheckable(FALSE);
		pushButton_Fold->setCheckable(FALSE);
		pushButton_AllIn->setCheckable(FALSE);
		
		//design
		pushButton_BetRaise->setStyleSheet("QPushButton { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_03_0.6.png); "+ font2String +" font-size: 11px; font-weight: bold; color: #F0F0F0;} QPushButton:unchecked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_03_0.6.png); } QPushButton:checked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_03_0.6_checked.png); }");
		pushButton_CallCheck->setStyleSheet("QPushButton { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_05_0.6.png); "+ font2String +" font-size: 11px; font-weight: bold; color: #F0F0F0;} QPushButton:unchecked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_05_0.6.png); } QPushButton:checked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_05_0.6_checked.png); }");
		pushButton_Fold->setStyleSheet("QPushButton { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_07_0.6.png); "+ font2String +" font-size: 11px; font-weight: bold; color: #F0F0F0;}  QPushButton:unchecked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_07_0.6.png); } QPushButton:checked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_07_0.6_checked.png); }");

		myButtonsAreCheckable = FALSE;
	}

}

void mainWindowImpl::closeEvent(QCloseEvent* /*event*/) { quitPokerTH(); }

void mainWindowImpl::showMaximized () {
	this->showFullScreen ();
}

void mainWindowImpl::quitPokerTH() {

	if (myServerGuiInterface.get() && myServerGuiInterface->getSession().isNetworkServerRunning()) {

		QMessageBox msgBox(QMessageBox::Warning, tr("Closing PokerTH during network game"),
	                   	tr("You are the hosting server. Do you want to close PokerTH anyway?"), QMessageBox::Yes | QMessageBox::No, this);

		if (msgBox.exec() == QMessageBox::Yes ) {
			mySession->terminateNetworkClient();
			if (myServerGuiInterface.get()) myServerGuiInterface->getSession().terminateNetworkServer();
			qApp->quit();
		}		
	}
	else { 
		stopTimer();
		qApp->quit();
	}
}

void mainWindowImpl::changeLineEditBetValue(int value) {

	int temp;
	if(betSliderChangedByInput) {
		//prevent interval cutting of lineEdit_betValue input from code below
		betSliderChangedByInput = FALSE;
	}
	else {
		if(horizontalSlider_bet->maximum() <= 1000 ) {
			temp = (int)((value/10)*10);
			if(temp < horizontalSlider_bet->minimum())
				lineEdit_betValue->setText(QString::number(horizontalSlider_bet->minimum()));
			else
				lineEdit_betValue->setText(QString::number(temp));
		}
		else {
			temp = (int)((value/50)*50);
			if(temp < horizontalSlider_bet->minimum())
				lineEdit_betValue->setText(QString::number(horizontalSlider_bet->minimum()));
			else
				lineEdit_betValue->setText(QString::number(temp));
		}
	}
}

void mainWindowImpl::lineEditBetValueChanged(QString valueString) {

	bool ok;
	int betValue = QString(valueString).toInt(&ok, 10);
	QString betRaise = pushButton_BetRaise->text().section(" ",0 ,0);

	if(betValue >= horizontalSlider_bet->minimum()) {

		if(betValue > horizontalSlider_bet->maximum()) { // print the maximum
			pushButton_BetRaise->setText(betRaise + " $" + QString::number(horizontalSlider_bet->maximum()));
			betSliderChangedByInput = TRUE;
			horizontalSlider_bet->setValue(horizontalSlider_bet->maximum());
		}
		else { // really print the value
			pushButton_BetRaise->setText(betRaise + " $" + valueString);
			betSliderChangedByInput = TRUE;
			horizontalSlider_bet->setValue(betValue);
		}
	}
	else { // print the minimum
		pushButton_BetRaise->setText(betRaise + " $" + QString::number(horizontalSlider_bet->minimum()));
		betSliderChangedByInput = TRUE;
		horizontalSlider_bet->setValue(horizontalSlider_bet->minimum());
	}
}

void mainWindowImpl::leaveCurrentNetworkGame() {

	if (mySession->isNetworkClientRunning()) {

		if(myConfig->readConfigInt("DisableBackToLobbyWarning")) {

			assert(mySession);
			mySession->sendLeaveCurrentGame();
		}
		else {
			myMessageDialogImpl dialog(this);
			dialog.setWindowTitle(tr("PokerTH - Internet Game Message"));
			dialog.label_icon->setPixmap(QPixmap(myAppDataPath +"gfx/gui/misc/logoChip3D.png").scaled(50,50,Qt::KeepAspectRatio,Qt::SmoothTransformation));
			dialog.label->setText(tr("Attention! Do you really want to leave the current game\nand go back to the lobby?"));
				
			if (dialog.exec() == QDialog::Accepted ) {
			
				if(dialog.checkBox->isChecked()) {
					myConfig->writeConfigInt("DisableBackToLobbyWarning",1);
					myConfig->writeBuffer();
				}
				assert(mySession);
				mySession->sendLeaveCurrentGame();
			}
		}
	}
}
