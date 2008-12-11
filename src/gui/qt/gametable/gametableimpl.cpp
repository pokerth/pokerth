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
#include "gametableimpl.h"

#include "mymessagedialogimpl.h"
#include "settingsdialogimpl.h"
#include "startwindowimpl.h"

#include "startsplash.h"
#include "mycardspixmaplabel.h"
#include "mysetlabel.h"
#include "myavatarlabel.h"
#include "mychancelabel.h"
#include "log.h"
#include "chat.h"

#include "playerinterface.h"
#include "boardinterface.h"
#include "handinterface.h"
#include "game.h"
#include "session.h"
#include "cardsvalue.h"

#include "configfile.h"
#include "sdlplayer.h"
// #include "stylesheetreader.h"

#include <gamedata.h>
#include <generic/serverguiwrapper.h>

#include <net/socket_msg.h>

#include "math.h"

#define FORMATLEFT(X) "<p align='center'>(X)"
#define FORMATRIGHT(X) "(X)</p>"

using namespace std;

gameTableImpl::gameTableImpl(ConfigFile *c, QMainWindow *parent)
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

	//Sound
	mySDLPlayer = new SDLPlayer(myConfig);

	//styleSheetReader
// 	myStyleSheetReader = new StyleSheetReader(QString(myAppDataPath+"gfx/stylesheet.xml").toStdString());

	//Player0 pixmapCardsLabel needs Myw
	pixmapLabel_card0b->setMyW(this);
	pixmapLabel_card0a->setMyW(this);

	//Flipside festlegen;
	if (myConfig->readConfigInt("FlipsideOwn") && myConfig->readConfigString("FlipsideOwnFile") != "") {
		flipside = new QPixmap();
		*flipside = QPixmap::fromImage(QImage(QString::fromUtf8(myConfig->readConfigString("FlipsideOwnFile").c_str())));
	}
	else { 
		flipside = new QPixmap();
		*flipside = QPixmap::fromImage(QImage(myAppDataPath +"gfx/cards/default/flipside.png"));
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

	//CardsChanceMonitor show/hide
	if (!myConfig->readConfigInt("ShowCardsChanceMonitor")) { 
		tabWidget_Right->removeTab(2);
		tabWidget_Right->setCurrentIndex(0);
	}
			
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
	voteOnKickTimeoutTimer = new QTimer(this);

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
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) { 
		playerAvatarLabelArray[i]->setMyW(this); 
		playerAvatarLabelArray[i]->setMyId(i); 
	}

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
	font2String = "font-family: \"Nimbus Sans L\";";
	QString textBrowserFontsize= "11";
	QString cashFontSize = "11";
	QString setLabelFontSize = "13";
	QString playerNameLabelFontSize = "14";
	QString mediumTableFontSize = "13";
	QString handLabelFontSize = "18";
	QString labelPotFontSize = "19";
	QString humanPlayerButtonFontSize = "13";
	QString betValueFontSize = "11";
#else 
	#ifdef __APPLE__	
		font1String = "font-family: \"Lucida Grande\";";
		font2String = "font-family: \"Lucida Grande\";";
	#else 
		font1String = "font-family: \"Nimbus Sans L\";";
		font2String = "font-family: \"Bitstream Vera Sans\";";
	#endif
	QString textBrowserFontsize= "10";
	QString cashFontSize = "10";
	QString setLabelFontSize = "12";
	QString playerNameLabelFontSize = "13";
	QString mediumTableFontSize = "13";
	QString handLabelFontSize = "17";	
	QString labelPotFontSize = "18";
	QString humanPlayerButtonFontSize = "12";
	QString betValueFontSize = "10";
#endif


	textBrowser_Log->setStyleSheet("QTextBrowser { "+ font1String +" font-size: "+textBrowserFontsize+"px; color: #F0F0F0; background-color: #1D3B00; border:none; } QScrollBar:vertical { border: 1px solid #104600; background: #135000; width: 15px; margin: 17px -1px 17px 0px; } QScrollBar::handle:vertical { border-radius: 1px; border: 1px solid #1B7200; background: #176400; min-height: 20px; } QScrollBar::add-line:vertical { margin-right: 0px; margin-left: 1px; border-bottom-right-radius: 2px; border-bottom-left-radius: 2px; border-top-right-radius: 1px; border-top-left-radius: 1px; border: 1px solid #1E7F00; background: #1A6F00; height: 15px; subcontrol-position: bottom; subcontrol-origin: margin; } QScrollBar::sub-line:vertical { margin-right: 0px; margin-left: 1px; border-bottom-right-radius: 1px; border-bottom-left-radius: 1px; border-top-right-radius: 2px; border-top-left-radius: 2px; border: 1px solid #1E7F00; background: #1A6F00; height: 15px; subcontrol-position: top; subcontrol-origin: margin; } QScrollBar:up-arrow:vertical, QScrollBar::down-arrow:vertical { border: 1px solid #208A00; height: 3px; width: 3px; background: #27A800; } QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }");
	textBrowser_Chat->setStyleSheet("QTextBrowser { "+ font1String +" font-size: "+textBrowserFontsize+"px; color: #F0F0F0; background-color: #1D3B00; border:none; } QScrollBar:vertical { border: 1px solid #104600; background: #135000; width: 15px; margin: 17px -1px 17px 0px; } QScrollBar::handle:vertical { border-radius: 1px; border: 1px solid #1B7200; background: #176400; min-height: 20px; } QScrollBar::add-line:vertical { margin-right: 0px; margin-left: 1px; border-bottom-right-radius: 2px; border-bottom-left-radius: 2px; border-top-right-radius: 1px; border-top-left-radius: 1px; border: 1px solid #1E7F00; background: #1A6F00; height: 15px; subcontrol-position: bottom; subcontrol-origin: margin; } QScrollBar::sub-line:vertical { margin-right: 0px; margin-left: 1px; border-bottom-right-radius: 1px; border-bottom-left-radius: 1px; border-top-right-radius: 2px; border-top-left-radius: 2px; border: 1px solid #1E7F00; background: #1A6F00; height: 15px; subcontrol-position: top; subcontrol-origin: margin; } QScrollBar:up-arrow:vertical, QScrollBar::down-arrow:vertical { border: 1px solid #208A00; height: 3px; width: 3px; background: #27A800; } QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }");
	lineEdit_ChatInput->setStyleSheet("QLineEdit { "+ font1String +" font-size: "+textBrowserFontsize+"px; color: #F0F0F0; background-color: #1D3B00; border-top: 2px solid #286400; }");


#ifdef __APPLE__
// 	tabWidget_Right->setStyleSheet("QTabWidget { "+ font1String +" font-size: 11px; background-color: #145300; }");
// 	tabWidget_Left->setStyleSheet("QTabWidget { "+ font1String +" font-size: 11px; background-color: #145300;}");
#else
// 	tabWidget_Right->setStyleSheet("QTabWidget::pane { "+ font1String +" font-size: 10px; background-color: #145300; border: 1px solid #286400; border-radius: 2px; }");
// 	tabWidget_Left->setStyleSheet("QTabWidget::pane { border: 1px solid #286400; border-radius: 2px; background-color: #145300; }");
#endif


	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {

		cashTopLabelArray[i]->setStyleSheet("QLabel { "+ font2String +" font-size: "+cashFontSize+"px; font-weight: bold; color: #F0F0F0; }");
		cashLabelArray[i]->setStyleSheet("QLabel { "+ font2String +" font-size: "+cashFontSize+"px; font-weight: bold; color: #F0F0F0; }");
	}
	
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {

		setLabelArray[i]->setStyleSheet("QLabel { "+ font2String +" font-size: "+setLabelFontSize+"px; font-weight: bold; color: #F0F0F0; }");
	}

	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {

		playerNameLabelArray[i]->setStyleSheet("QLabel { "+ font2String +" font-size: "+playerNameLabelFontSize+"px; font-weight: bold; color: #F0F0F0; }");
	}

	label_Sets->setStyleSheet("QLabel { "+ font2String +" font-size: "+mediumTableFontSize+"px; font-weight: bold; color: #669900;  }");
	label_Total->setStyleSheet("QLabel { "+ font2String +" font-size: "+mediumTableFontSize+"px; font-weight: bold; color: #669900; }");
	textLabel_Sets->setStyleSheet("QLabel { "+ font2String +" font-size: "+mediumTableFontSize+"px; font-weight: bold; color: #669900;  }");
	textLabel_Pot->setStyleSheet("QLabel { "+ font2String +" font-size: "+mediumTableFontSize+"px; font-weight: bold; color: #669900;  }");
	label_handNumber->setStyleSheet("QLabel { "+ font2String +" font-size: "+mediumTableFontSize+"px; font-weight: bold; color: #669900;  }");
	label_gameNumber->setStyleSheet("QLabel { "+ font2String +" font-size: "+mediumTableFontSize+"px; font-weight: bold; color: #669900;  }");
	label_handNumberValue->setStyleSheet("QLabel { "+ font2String +" font-size: "+mediumTableFontSize+"px; font-weight: bold; color: #669900;  }");
	label_gameNumberValue->setStyleSheet("QLabel { "+ font2String +" font-size: "+mediumTableFontSize+"px; font-weight: bold; color: #669900;  }");

	textLabel_handLabel->setStyleSheet("QLabel { "+ font2String +" font-size: "+handLabelFontSize+"px; font-weight: bold; color: #669900;  }");
	label_Pot->setStyleSheet("QLabel { "+ font2String +" font-size: "+labelPotFontSize+"px; font-weight: bold; color: #669900;   }");


	//Widgets Grafiken setzen
	label_CardHolder0->setPixmap(myAppDataPath + "gfx/gui/table/default/cardholder_flop.png");
	label_CardHolder1->setPixmap(myAppDataPath + "gfx/gui/table/default/cardholder_flop.png");
	label_CardHolder2->setPixmap(myAppDataPath + "gfx/gui/table/default/cardholder_flop.png");
	label_CardHolder3->setPixmap(myAppDataPath + "gfx/gui/table/default/cardholder_turn.png");
	label_CardHolder4->setPixmap(myAppDataPath + "gfx/gui/table/default/cardholder_river.png");

	label_Handranking->setPixmap(myAppDataPath + "gfx/gui/misc/handRanking.png");

	//Widgets Grafiken per Stylesheets setzen
	this->setStyleSheet("QMainWindow { background-image: url(" + myAppDataPath +"gfx/gui/table/default/table.png); background-position: bottom center; background-origin: content;}");

	menubar->setStyleSheet("QMenuBar { background-color: #145300; } QMenuBar::item { color: #99D500; }");

	pushButton_break->setStyleSheet("QPushButton:enabled { background-color: #145300; color: #99D500;} QPushButton:disabled { background-color: #145300; color: #486F3E; font-weight: 900;}");
	label_speedString->setStyleSheet("QLabel { color: #99D500;}");
	label_speedValue->setStyleSheet("QLabel { color: #99D500;}");	
	
	pushButton_voteOnKickYes->setStyleSheet("QPushButton:enabled { background-color: #1C7000; color: #99D500;} QPushButton:disabled { background-color: #145300; color: #486F3E; font-weight: 900;}");
	pushButton_voteOnKickNo->setStyleSheet("QPushButton:enabled { background-color: #1C7000; color: #99D500;} QPushButton:disabled { background-color: #145300; color: #486F3E; font-weight: 900;}");
	label_timeout->setStyleSheet("QLabel { color: #99D500; font-size: 11px;}");
	label_kickVoteTimeout->setStyleSheet("QLabel { color: #99D500; font-size: 11px;}");
	label_kickUser->setStyleSheet("QLabel { color: #99D500; font-size: 11px;}");	
	label_votesMonitor->setStyleSheet("QLabel { color: #99D500; font-size: 11px;}");	
	label_voteStarterNick->setStyleSheet("QLabel { color: #99D500; font-size: 11px;}");	
	label_votestartedby->setStyleSheet("QLabel { color: #99D500; font-size: 11px;}");	

	statusbar->setStyleSheet(" QStatusBar { "+ font1String +" font-size: 12px; color: #B7FF00; }");

	//Groupbox Background 
	for (i=1; i<MAX_NUMBER_OF_PLAYERS; i++) {

		groupBoxArray[i]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/opponentBoxInactiveGlow.png) }"); 
	}
	groupBoxArray[0]->setStyleSheet("QGroupBox { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/playerBoxInactiveGlow_0.6.png) }"); 

	//Human player button
	pushButton_BetRaise->setStyleSheet("QPushButton { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_green.png); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #F0F0F0;} QPushButton:unchecked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_green.png); } QPushButton:checked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_green_checked.png);} QPushButton:hover { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_green_hover.png); } QPushButton:checked:hover { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_green_checked_hover.png);}");
	pushButton_CallCheck->setStyleSheet("QPushButton { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_blue.png); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #F0F0F0;} QPushButton:unchecked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_blue.png); } QPushButton:checked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_blue_checked.png);} QPushButton:hover { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_blue_hover.png); } QPushButton:checked:hover { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_blue_checked_hover.png);}");
	pushButton_Fold->setStyleSheet("QPushButton { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_red.png); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #F0F0F0;}  QPushButton:unchecked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_red.png); } QPushButton:checked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_red_checked.png);} QPushButton:hover { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_red_hover.png); } QPushButton:checked:hover { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_red_checked_hover.png);}");

	lineEdit_betValue->setStyleSheet("QLineEdit { "+ font2String +" font-size: "+betValueFontSize+"px; font-weight: bold; background-color: #1D3B00; color: #F0F0F0; } QLineEdit:disabled { background-color: #316300; color: #6d7b5f }");

	pushButton_AllIn->setStyleSheet("QPushButton:enabled { background-color: #145300; color: #99D500;} QPushButton:disabled { background-color: #145300; color: #486F3E; font-weight: 900;}");

// 	away radiobuttons
// 	QString radioButtonString("QRadioButton { color: #99D500; } QRadioButton::indicator { width: 13px; height: 13px; } QRadioButton::indicator::checked { image: url("+myAppDataPath+"gfx/gui/misc/radiobutton_checked.png); }");
	QString radioButtonString("QRadioButton { color: #99D500; } QRadioButton::indicator { width: 13px; height: 13px;} QRadioButton::indicator::checked { image: url("+myAppDataPath+"gfx/gui/misc/radiobutton_checked.png);}  QRadioButton::indicator::unchecked { image: url("+myAppDataPath+"gfx/gui/misc/radiobutton_unchecked.png);}      QRadioButton::indicator:unchecked:hover { image: url("+myAppDataPath+"gfx/gui/misc/radiobutton_unchecked_hover.png);} QRadioButton::indicator:unchecked:pressed { image: url("+myAppDataPath+"gfx/gui/misc/radiobutton_pressed.png);} QRadioButton::indicator::checked { image: url("+myAppDataPath+"gfx/gui/misc/radiobutton_checked.png);}      QRadioButton::indicator:checked:hover { image: url("+myAppDataPath+"gfx/gui/misc/radiobutton_checked_hover.png);} QRadioButton::indicator:checked:pressed { image: url("+myAppDataPath+"gfx/gui/misc/radiobutton_pressed.png);}");

	radioButton_manualAction->setStyleSheet(radioButtonString);
	radioButton_autoCheckFold->setStyleSheet(radioButtonString);
	radioButton_autoCheckCallAny->setStyleSheet(radioButtonString);

	groupBox_RightToolBox->setStyleSheet("QGroupBox { border:none; background-image: url(" + myAppDataPath +"gfx/gui/misc/toolboxFrameBG.png) }");
	groupBox_LeftToolBox->setStyleSheet("QGroupBox { border:none; background-image: url(" + myAppDataPath +"gfx/gui/misc/toolboxFrameBG.png) }");

	//raise actionLable above just inserted mypixmaplabel
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) { actionLabelArray[i]->raise(); }

	//fix for away string bug in righttabwidget on windows
 #if (defined _WIN32) || (defined __APPLE__)
	tabWidget_Right->setTabText(0, " "+tabWidget_Right->tabText(0)+" ");
	tabWidget_Right->setTabText(1, " "+tabWidget_Right->tabText(1)+" ");
	tabWidget_Left->setTabText(0, " "+tabWidget_Left->tabText(0)+" ");
	tabWidget_Left->setTabText(1, " "+tabWidget_Left->tabText(1)+" ");
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

	//set Focus to gametable
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

// 	Dialogs
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
	connect(voteOnKickTimeoutTimer, SIGNAL(timeout()), this, SLOT(nextVoteOnKickTimeoutAnimationFrame()));

	connect( actionConfigure_PokerTH, SIGNAL( triggered() ), this, SLOT( callSettingsDialog() ) );
	connect( actionClose, SIGNAL( triggered() ), this, SLOT( closeGameTable()) );
	connect( actionFullScreen, SIGNAL( triggered() ), this, SLOT( switchFullscreen() ) );
	connect( actionShowHideChat, SIGNAL( triggered() ), this, SLOT( switchChatWindow() ) );
	connect( actionShowHideHelp, SIGNAL( triggered() ), this, SLOT( switchHelpWindow() ) );
	connect( actionShowHideLog, SIGNAL( triggered() ), this, SLOT( switchLogWindow() ) );
	connect( actionShowHideAway, SIGNAL( triggered() ), this, SLOT( switchAwayWindow() ) );
	connect( actionShowHideChance, SIGNAL( triggered() ), this, SLOT( switchChanceWindow() ) );

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

	connect( pushButton_voteOnKickYes, SIGNAL( clicked() ), this, SLOT( voteOnKickYes() ) );
	connect( pushButton_voteOnKickNo, SIGNAL( clicked() ), this, SLOT( voteOnKickNo() ) );

	//Nachrichten Thread-Save
	connect(this, SIGNAL(signalInitGui(int)), this, SLOT(initGui(int)));
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
	connect(this, SIGNAL(signalStartVoteOnKick(unsigned, unsigned, int, int)), this, SLOT(startVoteOnKick(unsigned, unsigned, int, int)));
	connect(this, SIGNAL(signalChangeVoteOnKickButtonsState(bool)), this, SLOT(changeVoteOnKickButtonsState(bool)));
	connect(this, SIGNAL(signalEndVoteOnKick()), this, SLOT(endVoteOnKick()));

}

gameTableImpl::~gameTableImpl() {

	delete myConfig;
	myConfig = 0;

}

void gameTableImpl::callSettingsDialog() { myStartWindow->callSettingsDialog(); }

void gameTableImpl::applySettings(settingsDialogImpl* mySettingsDialog) {

	//Toolbox verstecken?
	if (myConfig->readConfigInt("ShowLeftToolBox")) { groupBox_LeftToolBox->show(); }
	else { groupBox_LeftToolBox->hide(); }

	if (myConfig->readConfigInt("ShowRightToolBox")) { groupBox_RightToolBox->show(); }
	else { groupBox_RightToolBox->hide(); }

	//cardschancemonitor show/hide
	if (!myConfig->readConfigInt("ShowCardsChanceMonitor")) { 
		tabWidget_Right->removeTab(2);
		tabWidget_Right->setCurrentIndex(0);
	}
	else {
		if(tabWidget_Right->widget(2) != tab_Chance) 
		tabWidget_Right->insertTab(2, tab_Chance, QString(tr("Chance")));
	}


	//Add avatar (if set)
	myStartWindow->getSession()->addOwnAvatar(myConfig->readConfigString("MyAvatar"));

	//Falls Spielernamen geändert wurden --> neu zeichnen --> erst beim nächsten Neustart neu ausgelesen
	if (mySettingsDialog->getPlayerNickIsChanged() && myStartWindow->getSession()->getCurrentGame() && !myStartWindow->getSession()->isNetworkClientRunning()) { 

		Game *currentGame = myStartWindow->getSession()->getCurrentGame();
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

	if(myStartWindow->getSession()->getCurrentGame() && !myStartWindow->getSession()->isNetworkClientRunning()) {

		Game *currentGame = myStartWindow->getSession()->getCurrentGame();
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

		flipside = new QPixmap();
		*flipside = QPixmap::fromImage(QImage(QString::fromUtf8(myConfig->readConfigString("FlipsideOwnFile").c_str())));
	}
	else { 
		flipside = new QPixmap();
		*flipside = QPixmap::fromImage(QImage(myAppDataPath +"gfx/cards/default/flipside.png"));
	}

	//Check for anti-peek mode
	if(myStartWindow->getSession()->getCurrentGame()) {
		QPixmap tempCardsPixmapArray[2];
		int tempCardsIntArray[2];
		
		myStartWindow->getSession()->getCurrentGame()->getSeatsList()->front()->getMyCards(tempCardsIntArray);	
		if(myConfig->readConfigInt("AntiPeekMode")) {
			holeCardsArray[0][0]->setPixmap(*flipside, TRUE);
			tempCardsPixmapArray[0] = QPixmap::fromImage(QImage(myAppDataPath +"gfx/cards/default/"+QString::number(tempCardsIntArray[0], 10)+".png"));
			holeCardsArray[0][0]->setFrontPixmap(tempCardsPixmapArray[0]);
			holeCardsArray[0][1]->setPixmap(*flipside, TRUE);
			tempCardsPixmapArray[1]= QPixmap::fromImage(QImage(myAppDataPath +"gfx/cards/default/"+QString::number(tempCardsIntArray[1], 10)+".png"));
			holeCardsArray[0][1]->setFrontPixmap(tempCardsPixmapArray[1]);
		}
		else {
			tempCardsPixmapArray[0]= QPixmap::fromImage(QImage(myAppDataPath +"gfx/cards/default/"+QString::number(tempCardsIntArray[0], 10)+".png"));
			holeCardsArray[0][0]->setPixmap(tempCardsPixmapArray[0],FALSE);
			tempCardsPixmapArray[1]= QPixmap::fromImage(QImage(myAppDataPath +"gfx/cards/default/"+QString::number(tempCardsIntArray[1], 10)+".png"));
			holeCardsArray[0][1]->setPixmap(tempCardsPixmapArray[1],FALSE);
		}
	}

	if(myStartWindow->getSession()->getCurrentGame()) {
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

void gameTableImpl::initGui(int speed)
{
	//kill running Singleshots!!!
	stopTimer();
		
	label_Pot->setText("Pot");
	label_Total->setText("Total:");
	label_Sets->setText("Bets:");
	label_handNumber->setText("Hand:");
	label_gameNumber->setText("Game:");
	
	groupBox_RightToolBox->setDisabled(FALSE);
	groupBox_LeftToolBox->setDisabled(FALSE);	
		
	//show human player buttons
	for(int i=0; i<6; i++) {
		userWidgetsArray[i]->show();
	}
	
	//set speeds for local game and for first network game
	if( !myStartWindow->getSession()->isNetworkClientRunning() || (myStartWindow->getSession()->isNetworkClientRunning() && !myStartWindow->getSession()->getCurrentGame()) ) {
	
		guiGameSpeed = speed;
		//positioning Slider
		horizontalSlider_speed->setValue(guiGameSpeed);
		setSpeeds();
	}
}

boost::shared_ptr<Session> gameTableImpl::getSession() { assert(myStartWindow->getSession().get()); return myStartWindow->getSession(); }
// void gameTableImpl::setSession(boost::shared_ptr<Session> session) { mySession = session; }


//refresh-Funktionen
void gameTableImpl::refreshSet() {
	
	Game *currentGame = myStartWindow->getSession()->getCurrentGame();

	PlayerListConstIterator it_c;
 	for (it_c=currentGame->getSeatsList()->begin(); it_c!=currentGame->getSeatsList()->end(); it_c++) { 
		if( (*it_c)->getMySet() == 0 )
			setLabelArray[(*it_c)->getMyID()]->setText("");
		else
			setLabelArray[(*it_c)->getMyID()]->setText("Bet: $"+QString::number((*it_c)->getMySet(),10)); 
	}
}

void gameTableImpl::refreshButton() {

	QPixmap dealerButton = QPixmap::fromImage(QImage(myAppDataPath +"gfx/gui/table/default/dealerPuck.png"));
	QPixmap smallblindButton = QPixmap::fromImage(QImage(myAppDataPath +"gfx/gui/table/default/smallblindPuck.png"));
	QPixmap bigblindButton = QPixmap::fromImage(QImage(myAppDataPath +"gfx/gui/table/default/bigblindPuck.png"));
	QPixmap onePix = QPixmap::fromImage(QImage(myAppDataPath +"gfx/gui/misc/1px.png"));

	Game *currentGame = myStartWindow->getSession()->getCurrentGame();

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

void gameTableImpl::refreshPlayerName() {

	HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();

	PlayerListConstIterator it_c;
	for (it_c=currentHand->getSeatsList()->begin(); it_c!=currentHand->getSeatsList()->end(); it_c++) {
		if((*it_c)->getMyActiveStatus()) { 
			playerNameLabelArray[(*it_c)->getMyID()]->setText(QString::fromUtf8((*it_c)->getMyName().c_str()));
			
		} else {
			playerNameLabelArray[(*it_c)->getMyID()]->setText(""); 
		
		}
	}
}

void gameTableImpl::refreshPlayerAvatar() {

	QPixmap onePix = QPixmap::fromImage(QImage(myAppDataPath +"gfx/gui/misc/1px.png"));

	HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();

	PlayerListConstIterator it_c;
	for (it_c=currentHand->getSeatsList()->begin(); it_c!=currentHand->getSeatsList()->end(); it_c++) {
		if((*it_c)->getMyActiveStatus()) { 

			if((*it_c)->getMyAvatar() == "" || !QFile::QFile(QString::fromUtf8((*it_c)->getMyAvatar().c_str())).exists()) {
				playerAvatarLabelArray[(*it_c)->getMyID()]->setPixmap(QPixmap::fromImage(QImage(myAppDataPath +"gfx/gui/table/default/genereticAvatar.png")));
			}
			else {
				playerAvatarLabelArray[(*it_c)->getMyID()]->setPixmap(QPixmap::fromImage(QImage(QString::fromUtf8((*it_c)->getMyAvatar().c_str()))));
			}
		}	
		else {
			playerAvatarLabelArray[(*it_c)->getMyID()]->setPixmap(onePix);
		}		
	}
}

void gameTableImpl::setPlayerAvatar(int myID, QString myAvatar) {

	if(myStartWindow->getSession()->getCurrentGame()) {

		boost::shared_ptr<PlayerInterface> tmpPlayer = myStartWindow->getSession()->getCurrentGame()->getPlayerByUniqueId(myID);
		if (tmpPlayer.get()) {

			if(QFile::QFile(myAvatar).exists()) {
				playerAvatarLabelArray[tmpPlayer->getMyID()]->setPixmap(myAvatar);
				tmpPlayer->setMyAvatar(myAvatar.toUtf8().constData());
			}
			else {
				playerAvatarLabelArray[tmpPlayer->getMyID()]->setPixmap(QPixmap::fromImage(QImage(myAppDataPath +"gfx/gui/table/default/genereticAvatar.png")));
				tmpPlayer->setMyAvatar("");
			}	
		}	
	}
}


void gameTableImpl::refreshAction(int playerID, int playerAction) {

	QPixmap onePix = QPixmap::fromImage(QImage(myAppDataPath +"gfx/gui/misc/1px.png"));
	QPixmap action;

	QStringList actionArray;
	actionArray << "" << "fold" << "check" << "call" << "bet" << "raise" << "allin";

	HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();

	if(playerID == -1 || playerAction == -1) {

		PlayerListConstIterator it_c;
		for (it_c=currentHand->getSeatsList()->begin(); it_c!=currentHand->getSeatsList()->end(); it_c++) { 
			
			//if no action --> clear Pixmap 
			if( (*it_c)->getMyAction() == 0) {
				actionLabelArray[(*it_c)->getMyID()]->setPixmap(onePix);	
			}
			else {
					//paint action pixmap
					actionLabelArray[(*it_c)->getMyID()]->setPixmap(QPixmap::fromImage(QImage(myAppDataPath +"gfx/gui/table/default/action_"+actionArray[(*it_c)->getMyAction()]+".png")));
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
			actionLabelArray[playerID]->setPixmap(QPixmap::fromImage(QImage(myAppDataPath +"gfx/gui/table/default/action_"+actionArray[playerAction]+".png")));			

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

void gameTableImpl::refreshCash() {

	HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();

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

void gameTableImpl::refreshGroupbox(int playerID, int status) {

	int j;

	if(playerID == -1 || status == -1) {

		HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();
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

void gameTableImpl::refreshGameLabels(int gameState) { 

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

	label_handNumberValue->setText(QString::number(myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getMyID(),10));
	label_gameNumberValue->setText(QString::number(myStartWindow->getSession()->getCurrentGame()->getMyGameID(),10));
}

void gameTableImpl::refreshAll() {
	
	refreshSet();
	refreshButton();

	HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();
	PlayerListConstIterator it_c;
	for (it_c=currentHand->getSeatsList()->begin(); it_c!=currentHand->getSeatsList()->end(); it_c++) { 
		refreshAction( (*it_c)->getMyID(), (*it_c)->getMyAction());
	}

	refreshCash();
	refreshGroupbox();
	refreshPlayerName();
	refreshPlayerAvatar();
}

void gameTableImpl::refreshChangePlayer() {

	refreshSet();

	HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();
	PlayerListConstIterator it_c;
	for (it_c=currentHand->getSeatsList()->begin(); it_c!=currentHand->getSeatsList()->end(); it_c++) { 
		refreshAction( (*it_c)->getMyID(), (*it_c)->getMyAction());
	}

	refreshCash();
}

void gameTableImpl::refreshPot() {
	HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();

	textLabel_Sets->setText("$"+QString::number(currentHand->getBoard()->getSets(),10));
	textLabel_Pot->setText("$"+QString::number(currentHand->getBoard()->getPot(),10));
}

void gameTableImpl::guiUpdateDone() {
	guiUpdateSemaphore.release();
}

void gameTableImpl::waitForGuiUpdateDone() {
	guiUpdateSemaphore.acquire();
}

void gameTableImpl::dealHoleCards() {

	QPixmap onePix = QPixmap::fromImage(QImage(myAppDataPath +"gfx/gui/misc/1px.png"));

	//TempArrays
	QPixmap tempCardsPixmapArray[2];
	int tempCardsIntArray[2];
	
	// Karten der Gegner und eigene Karten austeilen
	int j;
	Game *currentGame = myStartWindow->getSession()->getCurrentGame();

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

	//refresh CardsChanceMonitor Tool
	refreshCardsChance(GAME_STATE_PREFLOP);
}

void gameTableImpl::dealBeRoCards(int myBeRoID) {	

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


void gameTableImpl::dealFlopCards0() {	dealFlopCards0Timer->start(preDealCardsSpeed); }

void gameTableImpl::dealFlopCards1() {

	boardCardsArray[0]->setPixmap(*flipside, TRUE);
	dealFlopCards1Timer->start(dealCardsSpeed);
}

void gameTableImpl::dealFlopCards2() {

	boardCardsArray[1]->setPixmap(*flipside, TRUE);
	dealFlopCards2Timer->start(dealCardsSpeed);
}

void gameTableImpl::dealFlopCards3() {
	
	boardCardsArray[2]->setPixmap(*flipside, TRUE);
	dealFlopCards3Timer->start(dealCardsSpeed);
}

void gameTableImpl::dealFlopCards4() {

	int tempBoardCardsArray[5];
	
	myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getBoard()->getMyCards(tempBoardCardsArray);
	QPixmap card = QPixmap::fromImage(QImage(myAppDataPath +"gfx/cards/default/"+QString::number(tempBoardCardsArray[0], 10)+".png"));

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

void gameTableImpl::dealFlopCards5() {

	int tempBoardCardsArray[5];
	myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getBoard()->getMyCards(tempBoardCardsArray);
	QPixmap card = QPixmap::fromImage(QImage(myAppDataPath +"gfx/cards/default/"+QString::number(tempBoardCardsArray[1], 10)+".png"));
	
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

void gameTableImpl::dealFlopCards6() {

	int tempBoardCardsArray[5];
	myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getBoard()->getMyCards(tempBoardCardsArray);
	QPixmap card = QPixmap::fromImage(QImage(myAppDataPath +"gfx/cards/default/"+QString::number(tempBoardCardsArray[2], 10)+".png"));
	
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
	if(myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getAllInCondition()) { dealFlopCards6Timer->start(AllInDealCardsSpeed); }
	// sonst normale Variante
	else { 
		updateMyButtonsState(0);  //mode 0 == called from dealberocards
		dealFlopCards6Timer->start(postDealCardsSpeed);
	}

	//refresh CardsChanceMonitor Tool
	refreshCardsChance(GAME_STATE_FLOP);
}

void gameTableImpl::dealTurnCards0() { dealTurnCards0Timer->start(preDealCardsSpeed); }

void gameTableImpl::dealTurnCards1() {

	boardCardsArray[3]->setPixmap(*flipside, TRUE);
	dealTurnCards1Timer->start(dealCardsSpeed);
}

void gameTableImpl::dealTurnCards2() {

	int tempBoardCardsArray[5];
	myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getBoard()->getMyCards(tempBoardCardsArray);
	QPixmap card = QPixmap::fromImage(QImage(myAppDataPath +"gfx/cards/default/"+QString::number(tempBoardCardsArray[3], 10)+".png"));

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
	if(myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getAllInCondition()) { dealTurnCards2Timer->start(AllInDealCardsSpeed);
	}
	// sonst normale Variante
	else { 
		updateMyButtonsState(0);  //mode 0 == called from dealberocards
		dealTurnCards2Timer->start(postDealCardsSpeed); 
	}
	//refresh CardsChanceMonitor Tool
	refreshCardsChance(GAME_STATE_TURN);
}

void gameTableImpl::dealRiverCards0() { dealRiverCards0Timer->start(preDealCardsSpeed); }

void gameTableImpl::dealRiverCards1() {

	boardCardsArray[4]->setPixmap(*flipside, TRUE);

// 	QTimer::singleShot(dealCardsSpeed, this, SLOT( dealRiverCards2() ));
	dealRiverCards1Timer->start(dealCardsSpeed);
}

void gameTableImpl::dealRiverCards2() {

	int tempBoardCardsArray[5];
	myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getBoard()->getMyCards(tempBoardCardsArray);
	QPixmap card = QPixmap::fromImage(QImage(myAppDataPath +"gfx/cards/default/"+QString::number(tempBoardCardsArray[4], 10)+".png"));

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
	if(myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getAllInCondition()) { dealRiverCards2Timer->start(AllInDealCardsSpeed);	}
	// sonst normale Variante
	else {		
		updateMyButtonsState(0);  //mode 0 == called from dealberocards
		dealRiverCards2Timer->start(postDealCardsSpeed);
	}
	//refresh CardsChanceMonitor Tool
	refreshCardsChance(GAME_STATE_RIVER);
}

void gameTableImpl::provideMyActions(int mode) {

	QString pushButtonFoldString;
	QString pushButtonBetRaiseString;
	QString lastPushButtonBetRaiseString = pushButton_BetRaise->text();
	QString pushButtonCallCheckString;
	QString lastPushButtonCallCheckString = pushButton_CallCheck->text();
	
	
	Game *currentGame = myStartWindow->getSession()->getCurrentGame();
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
				if( (currentGame->getActivePlayerList()->size() > 2 && currentGame->getSeatsList()->front()->getMyButton() == BUTTON_SMALL_BLIND ) || ( currentGame->getActivePlayerList()->size() <= 2 && currentGame->getSeatsList()->front()->getMyButton() == BUTTON_BIG_BLIND)) { pushButtonFoldString = "Fold"; }
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

		if((myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_INTERNET || myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_NETWORK) && !lineEdit_ChatInput->hasFocus() && myConfig->readConfigInt("EnableBetInputFocusSwitch")) { 
			lineEdit_betValue->setFocus();
			lineEdit_betValue->selectAll();
		}

		if(myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_LOCAL) { 
			lineEdit_betValue->setFocus();
			lineEdit_betValue->selectAll();
		}

	}
}

void gameTableImpl::meInAction() {

	myButtonsCheckable(FALSE);
	
	horizontalSlider_bet->setEnabled(TRUE);
	lineEdit_betValue->setEnabled(TRUE);

	if((myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_INTERNET || myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_NETWORK) && lineEdit_ChatInput->text() == "" && myConfig->readConfigInt("EnableBetInputFocusSwitch")) { 
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

void gameTableImpl::startTimeoutAnimation(int playerId, int timeoutSec) {
	assert(playerId >= 0 && playerId < myStartWindow->getSession()->getCurrentGame()->getStartQuantityPlayers());
	
	//beep for player 0
	if(playerId) { setLabelArray[playerId]->startTimeOutAnimation(timeoutSec, FALSE); }
	else { setLabelArray[playerId]->startTimeOutAnimation(timeoutSec, TRUE); }
}

void gameTableImpl::stopTimeoutAnimation(int playerId) {
	assert(playerId >= 0 && playerId < myStartWindow->getSession()->getCurrentGame()->getStartQuantityPlayers());
	setLabelArray[playerId]->stopTimeOutAnimation();
}

void gameTableImpl::disableMyButtons() {

	HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();

	clearMyButtons();

	//clear userWidgets
	pushButton_AllIn->setDisabled(TRUE);
	horizontalSlider_bet->setDisabled(TRUE);
	lineEdit_betValue->setDisabled(TRUE);
	horizontalSlider_bet->setMinimum(0);
	horizontalSlider_bet->setMaximum(currentHand->getSeatsList()->front()->getMyCash());
	lineEdit_betValue->clear();
	horizontalSlider_bet->setValue(0);
	
#ifdef _WIN32
	QString humanPlayerButtonFontSize = "13";
#else 
	QString humanPlayerButtonFontSize = "12";
#endif
}

void gameTableImpl::myCallCheck() {

	if(pushButton_CallCheck->text().startsWith("Call")) { myCall(); }
	if(pushButton_CallCheck->text() == "Check") { myCheck(); }	
}

void gameTableImpl::myFold(){

	if(pushButton_Fold->text() == "Fold") {

		HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();
		currentHand->getSeatsList()->front()->setMyAction(1);
		currentHand->getSeatsList()->front()->setMyTurn(0);
	
		//set that i was the last active player. need this for unhighlighting groupbox
		currentHand->setLastPlayersTurn(0);
		
		statusBar()->clearMessage();
	
		//Spiel läuft weiter
		myActionDone();
	}
}

void gameTableImpl::myCheck() {
	HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();
	
	currentHand->getSeatsList()->front()->setMyTurn(0);
	currentHand->getSeatsList()->front()->setMyAction(2);

	//set that i was the last active player. need this for unhighlighting groupbox
	currentHand->setLastPlayersTurn(0);

	statusBar()->clearMessage();

	//Spiel läuft weiter
	myActionDone();
}

int gameTableImpl::getMyCallAmount() {
        int tempHighestSet = 0;
        HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();

        tempHighestSet = currentHand->getCurrentBeRo()->getHighestSet();

        if (currentHand->getSeatsList()->front()->getMyCash()+currentHand->getSeatsList()->front()->getMySet() <= tempHighestSet) {

                return currentHand->getSeatsList()->front()->getMyCash();
        }
        else {
                return tempHighestSet - currentHand->getSeatsList()->front()->getMySet();
        }
}

int gameTableImpl::getBetRaisePushButtonValue() {

	QString betValueString = pushButton_BetRaise->text().section(" ",1 ,1);
	int index = betValueString.indexOf("$");
	betValueString.remove(index,1);
	bool ok;
	int betValue = betValueString.toInt(&ok,10);
	
	return betValue;
}

int gameTableImpl::getMyBetAmount() {

	HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();
	
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

void gameTableImpl::myCall(){

	int tempHighestSet = 0;
	HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();
	
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

void gameTableImpl::mySet(){
	
	if(pushButton_BetRaise->text() != "") {

		HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();
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

void gameTableImpl::myAllIn(){

	HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();

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


void gameTableImpl::pushButtonBetRaiseClicked(bool checked) {

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

void gameTableImpl::pushButtonCallCheckClicked(bool checked) {

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

void gameTableImpl::pushButtonFoldClicked(bool checked) {

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

void gameTableImpl::pushButtonAllInClicked(bool checked) {

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

void gameTableImpl::myActionDone() {

	// If a network client is running, we need
	// to transfer the action to the server.
	myStartWindow->getSession()->sendClientPlayerAction();

	// TODO: Should not call in networking game.
	disableMyButtons();

	if (!myStartWindow->getSession()->isNetworkClientRunning())
		nextPlayerAnimation();

	//prevent escape button working while allIn
	myActionIsRaise = 0;
	myActionIsBet = 0;
}

void gameTableImpl::nextPlayerAnimation() {

	HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();

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

void gameTableImpl::beRoAnimation2(int myBeRoID) {

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


void gameTableImpl::preflopAnimation1() { preflopAnimation1Timer->start(nextPlayerSpeed2); }
void gameTableImpl::preflopAnimation1Action() { myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getCurrentBeRo()->run(); }

void gameTableImpl::preflopAnimation2() { preflopAnimation2Timer->start(preflopNextPlayerSpeed); }
void gameTableImpl::preflopAnimation2Action() { myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getCurrentBeRo()->nextPlayer(); }


void gameTableImpl::flopAnimation1() { flopAnimation1Timer->start(nextPlayerSpeed2); }
void gameTableImpl::flopAnimation1Action() { myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getCurrentBeRo()->run(); }

void gameTableImpl::flopAnimation2() { flopAnimation2Timer->start(nextPlayerSpeed3); }
void gameTableImpl::flopAnimation2Action() { myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getCurrentBeRo()->nextPlayer(); }

void gameTableImpl::turnAnimation1() { turnAnimation1Timer->start(nextPlayerSpeed2); }
void gameTableImpl::turnAnimation1Action() { myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getCurrentBeRo()->run(); }

void gameTableImpl::turnAnimation2() { turnAnimation2Timer->start(nextPlayerSpeed3); }
void gameTableImpl::turnAnimation2Action() { myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getCurrentBeRo()->nextPlayer(); }

void gameTableImpl::riverAnimation1() { riverAnimation1Timer->start(nextPlayerSpeed2); }
void gameTableImpl::riverAnimation1Action() { myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getCurrentBeRo()->run(); }

void gameTableImpl::riverAnimation2() { riverAnimation2Timer->start(nextPlayerSpeed3); }
void gameTableImpl::riverAnimation2Action() { myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getCurrentBeRo()->nextPlayer(); }

void gameTableImpl::postRiverAnimation1() { postRiverAnimation1Timer->start(nextPlayerSpeed2); }
void gameTableImpl::postRiverAnimation1Action() { myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getCurrentBeRo()->postRiverRun(); }

void gameTableImpl::postRiverRunAnimation1() {	postRiverRunAnimation1Timer->start(postRiverRunAnimationSpeed); }

void gameTableImpl::postRiverRunAnimation2() {
	
	uncheckMyButtons();
	myButtonsCheckable(FALSE);
	clearMyButtons();
	resetMyButtonsCheckStateMemory();

	pushButton_AllIn->setDisabled(TRUE);
	horizontalSlider_bet->setDisabled(TRUE);
	lineEdit_betValue->setDisabled(TRUE);

	HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();

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
												
								holeCardsArray[(*it_c)->getMyID()][j]->startFlipCards(guiGameSpeed, QPixmap::fromImage(QImage(myAppDataPath +"gfx/cards/default/"+QString::number(tempCardsIntArray[j], 10)+".png")), *flipside);
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
				QPixmap onePix = QPixmap::fromImage(QImage(myAppDataPath +"gfx/gui/misc/1px.png"));
			
				//TempArrays
				QPixmap tempCardsPixmapArray[2];
				int tempCardsIntArray[2];
			
				int j;
					for (it_c=currentHand->getActivePlayerList()->begin(); it_c!=currentHand->getActivePlayerList()->end(); it_c++) { 
					(*it_c)->getMyCards(tempCardsIntArray);	
					if((*it_c)->getMyAction() != PLAYER_ACTION_FOLD) { 
						if((*it_c)->getMyID() || ((*it_c)->getMyID()==0 && myConfig->readConfigInt("AntiPeekMode")) ) {
							for(j=0; j<2; j++) {		
								tempCardsPixmapArray[j] = QPixmap::fromImage(QImage(myAppDataPath +"gfx/cards/default/"+QString::number(tempCardsIntArray[j], 10)+".png"));
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
void gameTableImpl::postRiverRunAnimation2_flipHoleCards1() {

	HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();

	currentHand->getCurrentBeRo()->setPlayersTurn(currentHand->getCurrentBeRo()->getLastActionPlayer());

	postRiverRunAnimation2_flipHoleCards2Timer->start(nextPlayerSpeed2);
}


void gameTableImpl::postRiverRunAnimation2_flipHoleCards2() {

// 	if() {
// 		postRiverRunAnimation2_flipHoleCards1Timer->start(nextPlayerSpeed2);
// 	}
// 	else {
// 		postRiverRunAnimation2Timer->start(postRiverRunAnimationSpeed);
// 	}
}


void gameTableImpl::postRiverRunAnimation3() {

	HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();
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
			actionLabelArray[(*it_c)->getMyID()]->setPixmap(QPixmap::fromImage(QImage(myAppDataPath +"gfx/gui/table/default/action_winner.png")));

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

void gameTableImpl::postRiverRunAnimation4() {

	distributePotAnimCounter=0;
	potDistributeTimer->start(winnerBlinkSpeed);
}

void gameTableImpl::postRiverRunAnimation5() {

	HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();

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

void gameTableImpl::postRiverRunAnimation6() {

// 	int i;
	HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();

	refreshCash();
	refreshPot();

	// TODO HACK
	// Check for network client, do not start new hand if client is running.
	if (myStartWindow->getSession()->isNetworkClientRunning())
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
				currentHand->getGuiInterface()->logPlayerWinGame((*it_c)->getMyName(),  myStartWindow->getSession()->getCurrentGame()->getMyGameID());
			}
		}

		if( !DEBUG_MODE ) {

			if(myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_LOCAL) {
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
			myStartWindow->callNewGameDialog();	
			//Bei Cancel nichts machen!!!
		}
		return;
	} 
	
	postRiverRunAnimation6Timer->start(newRoundSpeed);
}

void gameTableImpl::flipHolecardsAllIn() {

	HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();

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
								holeCardsArray[(*it_c)->getMyID()][j]->startFlipCards(guiGameSpeed, QPixmap::fromImage(QImage(myAppDataPath +"gfx/cards/default/"+QString::number(tempCardsIntArray[j], 10)+".png")), *flipside);
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
				QPixmap onePix = QPixmap::fromImage(QImage(myAppDataPath +"gfx/gui/misc/1px.png"));
				
				//TempArrays
				QPixmap tempCardsPixmapArray[2];
				int temp2CardsIntArray[2];
				
				int j;
				for (it_c=currentHand->getActivePlayerList()->begin(); it_c!=currentHand->getActivePlayerList()->end(); it_c++) { 
					(*it_c)->getMyCards(temp2CardsIntArray);	
					if((*it_c)->getMyAction() != PLAYER_ACTION_FOLD) { 
						if((*it_c)->getMyID() || ((*it_c)->getMyID()==0 && myConfig->readConfigInt("AntiPeekMode")) ) {
							for(j=0; j<2; j++) {
								
								tempCardsPixmapArray[j] = QPixmap::fromImage(QImage(myAppDataPath +"gfx/cards/default/"+QString::number(temp2CardsIntArray[j], 10)+".png"));
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

void gameTableImpl::showMyCards() {

	//TempArrays
	int tempCardsIntArray[2];
	HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();

	currentHand->getSeatsList()->front()->getMyCards(tempCardsIntArray);	
	if( currentHand->getSeatsList()->front()->getMyCardsFlip() == 0 &&  currentHand->getCurrentRound() == 4 && currentHand->getSeatsList()->front()->getMyActiveStatus() && currentHand->getSeatsList()->front()->getMyAction() != PLAYER_ACTION_FOLD) { 

		//set Player value (logging)	
		currentHand->getSeatsList()->front()->setMyCardsFlip(1,1);
	}
}


void gameTableImpl::startNewHand() {

	if( !breakAfterCurrentHand){
		myStartWindow->getSession()->getCurrentGame()->initHand();
		myStartWindow->getSession()->getCurrentGame()->startHand();
	}
	else { 

		if(myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_LOCAL) {
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

void gameTableImpl::handSwitchRounds() { myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->switchRounds(); 
}

void gameTableImpl::nextRoundCleanGui() {

	int i,j;

	// GUI bereinigen - Bilder löschen, Animationen unterbrechen
	QPixmap onePix = QPixmap::fromImage(QImage(myAppDataPath +"gfx/gui/misc/1px.png"));
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
	if(myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getSeatsList()->front()->getMyActiveStatus() == 1) {
		disableMyButtons();
	}

	textLabel_handLabel->setText("");
	
	refreshAll();

	flipHolecardsAllInAlreadyDone = FALSE;

	//Wenn Pause zwischen den Hands in der Konfiguration steht den Stop Button drücken!
	if (myConfig->readConfigInt("PauseBetweenHands") && blinkingStartButtonAnimationTimer->isActive() == FALSE && myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_LOCAL) { 
		pushButton_break->click(); 
	}
	else { 
		//FIX STRG+N Bug
		pushButton_break->setEnabled(TRUE); 
		breakAfterCurrentHand=FALSE;
	}
	
	//Clean breakbutton
	if(myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_LOCAL) {
		blinkingStartButtonAnimationTimer->stop();
		pushButton_break->setStyleSheet("QPushButton { background-color: #145300; color: #99D500;}");
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

void gameTableImpl::stopTimer() {

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

void gameTableImpl::setSpeeds() {

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

void gameTableImpl::breakButtonClicked() {

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
		pushButton_break->setStyleSheet("QPushButton { background-color: #145300; color: #99D500;}");
		QFontMetrics tempMetrics = this->fontMetrics();
		int width = tempMetrics.width(tr("Stop"));
		pushButton_break->setMinimumSize(width+10,20);

		pushButton_break->setText(tr("Stop"));

		if(currentGameOver) {
			currentGameOver = FALSE;
			myStartWindow->callNewGameDialog();	
			//Bei Cancel nichts machen!!!
		}
		else {
			startNewHand();
		}
	}
}

void gameTableImpl::keyPressEvent ( QKeyEvent * event ) {

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
		if(myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_LOCAL) {
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
	
	//TESTING UNIT
	//if (event->key() == Qt::Key_M) { startVoteOnKick(3,60, 6); }
	//if (event->key() == Qt::Key_N) { endVoteOnKick(); }
}

void gameTableImpl::changePlayingMode() {

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

bool gameTableImpl::eventFilter(QObject *obj, QEvent *event)
{
	QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

	if (/*obj == lineEdit_ChatInput && lineEdit_ChatInput->text() != "" && */event->type() == QEvent::KeyPress && keyEvent->key() == Qt::Key_Tab) {
		myChat->nickAutoCompletition();
		return true;
	}
	else if (event->type() == QEvent::Close) {
		event->ignore();
		closeGameTable();
		return true; 
	}
	else {
		// pass the event on to the parent class
		return QMainWindow::eventFilter(obj, event);
	}
}

void gameTableImpl::switchChatWindow() {

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

void gameTableImpl::switchHelpWindow() {

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

void gameTableImpl::switchLogWindow() {

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

void gameTableImpl::switchAwayWindow() {

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

void gameTableImpl::switchChanceWindow() {

	int tab = 2;
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

void gameTableImpl::switchFullscreen() {

	if (this->isFullScreen()) {
		this->showNormal();
	}
	else {
		this->showFullScreen();
	}
}

void gameTableImpl::blinkingStartButtonAnimationAction() {
	
	QString style = pushButton_break->styleSheet();

	if(style.contains("QPushButton { background-color: #145300;")) {
		pushButton_break->setStyleSheet("QPushButton { background-color: #6E9E00; color: black;}");
	}
	else {
		pushButton_break->setStyleSheet("QPushButton { background-color: #145300; color: #99D500;}");
	}
}

void gameTableImpl::sendChatMessage() { myChat->sendMessage(); }
void gameTableImpl::checkChatInputLength(QString string) { myChat->checkInputLength(string); }


void gameTableImpl::tabSwitchAction() { 
	
	switch(tabWidget_Left->currentIndex()) {

		case 1: { lineEdit_ChatInput->setFocus();
			  myChat->checkInvisible();				
			}
		break;
		default: { lineEdit_ChatInput->clearFocus(); }
	
	}
}


void gameTableImpl::localGameModification() {
	
	tabWidget_Left->setCurrentIndex(0);
	tabWidget_Left->removeTab(1);
	tabWidget_Left->removeTab(1);

	int i;
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++ ) { 
		setLabelArray[i]->stopTimeOutAnimation();
		playerAvatarLabelArray[i]->setEnabledContextMenu(FALSE);
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

void gameTableImpl::networkGameModification() {
	
	if(tabWidget_Left->widget(1) != tab_Chat) 
		tabWidget_Left->insertTab(1, tab_Chat, QString(tr("Chat"))); /*TODO text abgeschnitten --> stylesheets*/
	
	tabWidget_Left->removeTab(2);
	
	tabWidget_Left->setCurrentIndex(1);
	myChat->clearNewGame();
	
	int i;
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++ ) { 
		playerAvatarLabelArray[i]->setEnabledContextMenu(TRUE);
		playerAvatarLabelArray[i]->setVoteOnKickContextMenuEnabled(TRUE);
	}

	if(myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_INTERNET) {
		pushButton_break->show();
		QFontMetrics tempMetrics = this->fontMetrics();
		int width = tempMetrics.width(tr("Lobby"));
		pushButton_break->setText(tr("Lobby"));
		pushButton_break->setMinimumSize(width+10,20);
	}
	if(myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_NETWORK) {

		pushButton_break->hide();
	}
	//Set the playing mode to "manual"
	radioButton_manualAction->click();

	//clear log
	textBrowser_Log->clear();
}

void gameTableImpl::mouseOverFlipCards(bool front) {

	if(myStartWindow->getSession()->getCurrentGame()) {
		if(myConfig->readConfigInt("AntiPeekMode") && myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getSeatsList()->front()->getMyActiveStatus() && myStartWindow->getSession()->getCurrentGame()->getSeatsList()->front()->getMyAction() != PLAYER_ACTION_FOLD) {
			holeCardsArray[0][0]->signalFastFlipCards(front);
			holeCardsArray[0][1]->signalFastFlipCards(front);
		}
	}
}

void gameTableImpl::updateMyButtonsState(int mode) {
	
	HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();

	if(currentHand->getLastPlayersTurn() == 0) {
		myButtonsCheckable(FALSE);
		clearMyButtons();
	}
	else {	
		if(myStartWindow->getSession()->getCurrentGame()->getSeatsList()->front()->getMyAction() != PLAYER_ACTION_ALLIN) { // dont show pre-actions after flip cards when allin
			myButtonsCheckable(TRUE);
			provideMyActions(mode);
		}
	}
}

void gameTableImpl::uncheckMyButtons() {

	pushButton_BetRaise->setChecked(FALSE);
	pushButton_CallCheck->setChecked(FALSE);
	pushButton_Fold->setChecked(FALSE);
	pushButton_AllIn->setChecked(FALSE);

}

void gameTableImpl::resetMyButtonsCheckStateMemory() {

	pushButtonCallCheckIsChecked = FALSE;
	pushButtonFoldIsChecked = FALSE;
	pushButtonAllInIsChecked = FALSE;
	pushButtonBetRaiseIsChecked = FALSE;
}

void gameTableImpl::clearMyButtons() {

	pushButton_BetRaise->setText("");
	pushButton_CallCheck->setText("");
	pushButton_Fold->setText("");
}

void gameTableImpl::myButtonsCheckable(bool state) {

#ifdef _WIN32
	QString humanPlayerButtonFontSize = "13";
#else 
	QString humanPlayerButtonFontSize = "12";
#endif
	
	if(state) {
		//checkable

		pushButton_BetRaise->setCheckable(TRUE);
		pushButton_CallCheck->setCheckable(TRUE);
		pushButton_Fold->setCheckable(TRUE);
		pushButton_AllIn->setCheckable(TRUE);

		//design
		pushButton_BetRaise->setStyleSheet("QPushButton { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_green.png); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #87FF97;} QPushButton:unchecked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_green.png);} QPushButton:checked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_green_checked.png);} QPushButton:hover { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_green_hover.png);} QPushButton:checked:hover { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_green_checked_hover.png);}");
		pushButton_CallCheck->setStyleSheet("QPushButton { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_blue.png); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #87CDFF;} QPushButton:unchecked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_blue.png); } QPushButton:checked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_blue_checked.png);} QPushButton:hover { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_blue_hover.png); } QPushButton:checked:hover { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_blue_checked_hover.png); }");
		pushButton_Fold->setStyleSheet("QPushButton { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_red.png); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #FF8787;} QPushButton:unchecked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_red.png); } QPushButton:checked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_red_checked.png);} QPushButton:hover { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_red_hover.png); } QPushButton:checked:hover { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_red_checked_hover.png); }");

		myButtonsAreCheckable = TRUE;
	}
	else {
		//not checkable

		pushButton_BetRaise->setCheckable(FALSE);
		pushButton_CallCheck->setCheckable(FALSE);
		pushButton_Fold->setCheckable(FALSE);
		pushButton_AllIn->setCheckable(FALSE);
	
		QString hover;
		if(pushButton_AllIn->isEnabled()) { hover = "_hover"; }
	
		//design
		pushButton_BetRaise->setStyleSheet("QPushButton { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_green.png); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #F0F0F0;} QPushButton:unchecked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_green.png); } QPushButton:checked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_green_checked.png);} QPushButton:hover { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_green"+hover+".png); } QPushButton:checked:hover { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_green_checked"+hover+".png);}");
		pushButton_CallCheck->setStyleSheet("QPushButton { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_blue.png); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #F0F0F0;} QPushButton:unchecked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_blue.png); } QPushButton:checked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_blue_checked.png);} QPushButton:hover { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_blue"+hover+".png); } QPushButton:checked:hover { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_blue_checked"+hover+".png);}");
		pushButton_Fold->setStyleSheet("QPushButton { border:none; background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_red.png); "+ font2String +" font-size: "+humanPlayerButtonFontSize+"px; font-weight: bold; color: #F0F0F0;}  QPushButton:unchecked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_red.png); } QPushButton:checked { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_red_checked.png);} QPushButton:hover { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_red"+hover+".png); } QPushButton:checked:hover { background-image: url(" + myAppDataPath +"gfx/gui/table/default/playeraction_red_checked"+hover+".png);}");

		myButtonsAreCheckable = FALSE;
	}

}

// // void gameTableImpl::closeEvent(QCloseEvent* /*event*/) { closeGameTable(); }

void gameTableImpl::showMaximized () {
	this->showFullScreen ();
}

void gameTableImpl::closeGameTable() {

	if (myStartWindow->getMyServerGuiInterface().get() && myStartWindow->getMyServerGuiInterface()->getSession()->isNetworkServerRunning()) {

		QMessageBox msgBox(QMessageBox::Warning, tr("Closing PokerTH during network game"),
	                   	tr("You are the hosting server. Do you want to close PokerTH anyway?"), QMessageBox::Yes | QMessageBox::No, this);

		if (msgBox.exec() == QMessageBox::Yes ) {
			myStartWindow->getSession()->terminateNetworkClient();
			stopTimer();
			if (myStartWindow->getMyServerGuiInterface().get()) myStartWindow->getMyServerGuiInterface()->getSession()->terminateNetworkServer();
			myStartWindow->show();
			this->hide();
		}
	}
	else {
		myStartWindow->getSession()->terminateNetworkClient();
		stopTimer();
		myStartWindow->show();
		this->hide();
	}
}

void gameTableImpl::changeLineEditBetValue(int value) {

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

void gameTableImpl::lineEditBetValueChanged(QString valueString) {

	if(horizontalSlider_bet->isEnabled()) {

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
}

void gameTableImpl::leaveCurrentNetworkGame() {

	if (myStartWindow->getSession()->isNetworkClientRunning()) {

		if(myConfig->readConfigInt("DisableBackToLobbyWarning")) {

			assert(myStartWindow->getSession());
			myStartWindow->getSession()->sendLeaveCurrentGame();
		}
		else {
			myMessageDialogImpl dialog(this);
			dialog.setWindowTitle(tr("PokerTH - Internet Game Message"));
			dialog.label_icon->setPixmap(QPixmap::fromImage(QImage(myAppDataPath +"gfx/gui/misc/logoChip3D.png")).scaled(50,50,Qt::KeepAspectRatio,Qt::SmoothTransformation));
			dialog.label->setText(tr("Attention! Do you really want to leave the current game\nand go back to the lobby?"));
				
			if (dialog.exec() == QDialog::Accepted ) {
			
				if(dialog.checkBox->isChecked()) {
					myConfig->writeConfigInt("DisableBackToLobbyWarning",1);
					myConfig->writeBuffer();
				}
				assert(myStartWindow->getSession());
				myStartWindow->getSession()->sendLeaveCurrentGame();
			}
		}
	}
}

void gameTableImpl::triggerVoteOnKick(int id) { 

	assert(myStartWindow->getSession()->getCurrentGame());
	int playerCount = static_cast<int>(myStartWindow->getSession()->getCurrentGame()->getSeatsList()->size());
	if (id < playerCount)
	{
		PlayerListIterator pos = myStartWindow->getSession()->getCurrentGame()->getSeatsList()->begin();
		advance(pos, id);
		myStartWindow->getSession()->startVoteKickPlayer((*pos)->getMyUniqueID());
	}
}

void gameTableImpl::startVoteOnKick(unsigned playerId, unsigned voteStarterPlayerId, int timeoutSec, int numVotesNeededToKick)
{
	if(tabWidget_Left->widget(2) != tab_Kick) 
		tabWidget_Left->insertTab(2, tab_Kick, QString(tr("Kick")));
	
	tabWidget_Left->setCurrentIndex(2);
	pushButton_voteOnKickNo->hide();
	pushButton_voteOnKickYes->hide();
	label_kickUser->clear();
	label_kickVoteTimeout->clear();

	voteOnKickTimeoutSecs = timeoutSec;
	
	playerAboutToBeKickedId = playerId;
	refreshVotesMonitor(1, numVotesNeededToKick);

	PlayerInfo info(myStartWindow->getSession()->getClientPlayerInfo(voteStarterPlayerId));
	label_voteStarterNick->setText("<b>"+QString::fromUtf8(info.playerName.c_str())+"</b>");

	startVoteOnKickTimeout();
	
	int i;
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++ ) { 
		playerAvatarLabelArray[i]->setVoteRunning(TRUE);
	}
}

void gameTableImpl::changeVoteOnKickButtonsState(bool showHide)
{
	if(showHide) {
		
		PlayerInfo info(myStartWindow->getSession()->getClientPlayerInfo(playerAboutToBeKickedId));
		label_kickUser->setText(tr("Do you want to kick <b>%1</b><br>from this game?").arg(QString::fromUtf8(info.playerName.c_str())));

		pushButton_voteOnKickNo->show();
		pushButton_voteOnKickYes->show();
	}
	else {
		label_kickUser->clear();
		pushButton_voteOnKickNo->hide();
		pushButton_voteOnKickYes->hide();
	}
}

void gameTableImpl::endVoteOnKick()
{
	stopVoteOnKickTimeout();
	tabWidget_Left->removeTab(2);
	
	int i;
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++ ) { 
		playerAvatarLabelArray[i]->setVoteRunning(FALSE);
	}
}

void gameTableImpl::voteOnKickYes()
{
	changeVoteOnKickButtonsState(false);
	myStartWindow->getSession()->voteKick(true);
}

void gameTableImpl::voteOnKickNo()
{
	changeVoteOnKickButtonsState(false);
	myStartWindow->getSession()->voteKick(false);
}

void gameTableImpl::startVoteOnKickTimeout()
{
	voteOnKickRealTimer.reset();
	voteOnKickRealTimer.start();
	voteOnKickTimeoutTimer->start(1000);
}

void gameTableImpl::stopVoteOnKickTimeout()
{
	voteOnKickRealTimer.reset();
	voteOnKickTimeoutTimer->stop();
}

void gameTableImpl::nextVoteOnKickTimeoutAnimationFrame()
{
	if(voteOnKickTimeoutSecs-voteOnKickRealTimer.elapsed().total_seconds() > 0)
		label_kickVoteTimeout->setText(tr("<b>%1</b> secs left").arg(voteOnKickTimeoutSecs-voteOnKickRealTimer.elapsed().total_seconds()));
	else 
		label_kickVoteTimeout->setText(tr("<b>%1</b> secs left").arg(0));
}

void gameTableImpl::refreshVotesMonitor(int currentVotes, int numVotesNeededToKick)
{
	QString currentVotesString;
	if(currentVotes == 1) currentVotesString = tr("vote");
	else currentVotesString = tr("votes");
	
	if((*myStartWindow->getSession()->getCurrentGame()->getSeatsList()->begin())->getMyUniqueID() != playerAboutToBeKickedId) {
		PlayerInfo info(myStartWindow->getSession()->getClientPlayerInfo(playerAboutToBeKickedId));
		label_votesMonitor->setText(tr("Player <b>%1</b> has <b>%2</b> %3<br>against him. <b>%4</b> votes needed to kick.").arg(QString::fromUtf8(info.playerName.c_str())).arg(currentVotes).arg(currentVotesString).arg(numVotesNeededToKick));
	}
	else {
		label_votesMonitor->setText(tr("You have <b>%1</b> %2 against you.<br><b>%3</b> votes needed to kick.").arg(currentVotes).arg(currentVotesString).arg(numVotesNeededToKick));
	}
}

void gameTableImpl::refreshCardsChance(GameState bero)
{

	CardsValue* myCardsValue = new CardsValue;

	if(myConfig->readConfigInt("ShowCardsChanceMonitor")) {

		if((*myStartWindow->getSession()->getCurrentGame()->getSeatsList()->begin())->getMyActiveStatus() && (*myStartWindow->getSession()->getCurrentGame()->getSeatsList()->begin())->getMyAction() != PLAYER_ACTION_FOLD) {
			int boardCards[5];
			int holeCards[2];
		
			(*myStartWindow->getSession()->getCurrentGame()->getSeatsList()->begin())->getMyCards(holeCards);
			myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getBoard()->getMyCards(boardCards);
		
			label_chance->refreshChance(myCardsValue->calcCardsChance(bero, holeCards, boardCards));
		}
		else { 
			label_chance->resetChance();
		}
	}

	delete myCardsValue;
}
