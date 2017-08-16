/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/
#include "gametableimpl.h"

#include "mymessagedialogimpl.h"
#include "settingsdialogimpl.h"
#include "startwindowimpl.h"

#include "startsplash.h"
#include "mycardspixmaplabel.h"
#include "mysetlabel.h"
#include "myavatarlabel.h"
#include "myactionbutton.h"
#include "mycashlabel.h"
#include "mynamelabel.h"
#include "mychancelabel.h"
#include "mytimeoutlabel.h"
#include "guilog.h"
#include "chattools.h"

#include "playerinterface.h"
#include "boardinterface.h"
#include "handinterface.h"
#include "game.h"
#include "session.h"
#include "cardsvalue.h"

#include "configfile.h"
#include "soundevents.h"
#include "gametablestylereader.h"
#include "carddeckstylereader.h"
#include <gamedata.h>
#include <generic/serverguiwrapper.h>

#include <net/socket_msg.h>

#include <cmath>

#define FORMATLEFT(X) "<p align='center'>(X)"
#define FORMATRIGHT(X) "(X)</p>"

#ifdef ANDROID
#ifndef ANDROID_TEST
#include "QtGui/5.7.1/QtGui/qpa/qplatformnativeinterface.h"
#include <jni.h>
#endif
#endif

using namespace std;

gameTableImpl::gameTableImpl(ConfigFile *c, QMainWindow *parent)
	: QMainWindow(parent), myChat(NULL), myConfig(c), gameSpeed(0), myActionIsBet(0), myActionIsRaise(0), pushButtonBetRaiseIsChecked(false), pushButtonCallCheckIsChecked(false), pushButtonFoldIsChecked(false), pushButtonAllInIsChecked(false), myButtonsAreCheckable(false), breakAfterCurrentHand(false), currentGameOver(false), betSliderChangedByInput(false), guestMode(false), myLastPreActionBetValue(0)
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
	mySoundEventHandler = new SoundEvents(myConfig);

	// 	Init game table style
	myGameTableStyle = new GameTableStyleReader(myConfig, this);
	myGameTableStyle->readStyleFile(QString::fromUtf8(myConfig->readConfigString("CurrentGameTableStyle").c_str()));

	// 	Init card deck style
	myCardDeckStyle = new CardDeckStyleReader(myConfig, this);
	myCardDeckStyle->readStyleFile(QString::fromUtf8(myConfig->readConfigString("CurrentCardDeckStyle").c_str()));

	//Player0 pixmapCardsLabel needs Myw
	pixmapLabel_card0b->setMyW(this);
	pixmapLabel_card0a->setMyW(this);

	//set myStyle to widgets wich needs it
#ifdef GUI_800x480
	tabsDiag = new QDialog(this);
	tabs.setupUi(tabsDiag);
	textLabel_handLabel->hide();
#ifdef ANDROID
	tabsDiag->setStyleSheet("QObject { font: 26px; } QDialog { background-image: url(:/android/android-data/gfx/gui/table/default_800x480/table_dark.png); background-position: bottom center; background-origin: content;  background-repeat: no-repeat;}");
	this->setWindowState(Qt::WindowFullScreen);
#else
	tabs.pushButton_settings->hide();
#endif
	tabs.label_chance->setMyStyle(myGameTableStyle);
#else
	label_chance->setMyStyle(myGameTableStyle);
#endif

	//Flipside festlegen;
	if (myConfig->readConfigInt("FlipsideOwn") && myConfig->readConfigString("FlipsideOwnFile") != "") {
		flipside = QPixmap::fromImage(QImage(QString::fromUtf8(myConfig->readConfigString("FlipsideOwnFile").c_str())));
	} else {
		flipside = QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+"flipside.png"));
	}

	//Flipside Animation noch nicht erledigt
	flipHolecardsAllInAlreadyDone = false;

#ifndef GUI_800x480
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
#endif

	// userWidgetsArray init
	userWidgetsArray[0] = pushButton_BetRaise;
	userWidgetsArray[1] = pushButton_CallCheck;
	userWidgetsArray[2] = pushButton_Fold;
	userWidgetsArray[3] = spinBox_betValue;
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
	enableCallCheckPushButtonTimer = new QTimer(this);

	dealFlopCards0Timer->setSingleShot(true);
	dealFlopCards1Timer->setSingleShot(true);
	dealFlopCards2Timer->setSingleShot(true);
	dealFlopCards3Timer->setSingleShot(true);
	dealFlopCards4Timer->setSingleShot(true);
	dealFlopCards5Timer->setSingleShot(true);
	dealFlopCards6Timer->setSingleShot(true);
	dealTurnCards0Timer->setSingleShot(true);
	dealTurnCards1Timer->setSingleShot(true);
	dealTurnCards2Timer->setSingleShot(true);
	dealRiverCards0Timer->setSingleShot(true);
	dealRiverCards1Timer->setSingleShot(true);
	dealRiverCards2Timer->setSingleShot(true);

	nextPlayerAnimationTimer->setSingleShot(true);
	preflopAnimation1Timer->setSingleShot(true);
	preflopAnimation2Timer->setSingleShot(true);
	flopAnimation1Timer->setSingleShot(true);
	flopAnimation2Timer->setSingleShot(true);
	turnAnimation1Timer->setSingleShot(true);
	turnAnimation2Timer->setSingleShot(true);
	riverAnimation1Timer->setSingleShot(true);
	riverAnimation2Timer->setSingleShot(true);

	postRiverAnimation1Timer->setSingleShot(true);
	postRiverRunAnimation1Timer->setSingleShot(true);
	postRiverRunAnimation2Timer->setSingleShot(true);
	postRiverRunAnimation3Timer->setSingleShot(true);
	postRiverRunAnimation5Timer->setSingleShot(true);
	postRiverRunAnimation6Timer->setSingleShot(true);

	enableCallCheckPushButtonTimer->setSingleShot(true);

	playerStarsArray[1][0]=label_Star10;
	playerStarsArray[2][0]=label_Star20;
	playerStarsArray[3][0]=label_Star30;
	playerStarsArray[4][0]=label_Star40;
	playerStarsArray[5][0]=label_Star50;
	playerStarsArray[1][1]=label_Star11;
	playerStarsArray[2][1]=label_Star21;
	playerStarsArray[3][1]=label_Star31;
	playerStarsArray[4][1]=label_Star41;
	playerStarsArray[5][1]=label_Star51;
	playerStarsArray[1][2]=label_Star12;
	playerStarsArray[2][2]=label_Star22;
	playerStarsArray[3][2]=label_Star32;
	playerStarsArray[4][2]=label_Star42;
	playerStarsArray[5][2]=label_Star52;
	playerStarsArray[1][3]=label_Star13;
	playerStarsArray[2][3]=label_Star23;
	playerStarsArray[3][3]=label_Star33;
	playerStarsArray[4][3]=label_Star43;
	playerStarsArray[5][3]=label_Star53;
	playerStarsArray[1][4]=label_Star14;
	playerStarsArray[2][4]=label_Star24;
	playerStarsArray[3][4]=label_Star34;
	playerStarsArray[4][4]=label_Star44;
	playerStarsArray[5][4]=label_Star54;
	playerStarsArray[1][5]=label_Star15;
	playerStarsArray[2][5]=label_Star25;
	playerStarsArray[3][5]=label_Star35;
	playerStarsArray[4][5]=label_Star45;
	playerStarsArray[5][5]=label_Star55;
	playerStarsArray[1][6]=label_Star16;
	playerStarsArray[2][6]=label_Star26;
	playerStarsArray[3][6]=label_Star36;
	playerStarsArray[4][6]=label_Star46;
	playerStarsArray[5][6]=label_Star56;
	playerStarsArray[1][7]=label_Star17;
	playerStarsArray[2][7]=label_Star27;
	playerStarsArray[3][7]=label_Star37;
	playerStarsArray[4][7]=label_Star47;
	playerStarsArray[5][7]=label_Star57;
	playerStarsArray[1][8]=label_Star18;
	playerStarsArray[2][8]=label_Star28;
	playerStarsArray[3][8]=label_Star38;
	playerStarsArray[4][8]=label_Star48;
	playerStarsArray[5][8]=label_Star58;
	playerStarsArray[1][9]=label_Star19;
	playerStarsArray[2][9]=label_Star29;
	playerStarsArray[3][9]=label_Star39;
	playerStarsArray[4][9]=label_Star49;
	playerStarsArray[5][9]=label_Star59;

	// buttonLabelArray init
	buttonLabelArray[0] = textLabel_Button0;
	buttonLabelArray[1] = textLabel_Button1;
	buttonLabelArray[2] = textLabel_Button2;
	buttonLabelArray[3] = textLabel_Button3;
	buttonLabelArray[4] = textLabel_Button4;
	buttonLabelArray[5] = textLabel_Button5;
	buttonLabelArray[6] = textLabel_Button6;
	buttonLabelArray[7] = textLabel_Button7;
	buttonLabelArray[8] = textLabel_Button8;
	buttonLabelArray[9] = textLabel_Button9;

	// cashLabelArray init
	cashLabelArray[0] = textLabel_Cash0;
	cashLabelArray[1] = textLabel_Cash1;
	cashLabelArray[2] = textLabel_Cash2;
	cashLabelArray[3] = textLabel_Cash3;
	cashLabelArray[4] = textLabel_Cash4;
	cashLabelArray[5] = textLabel_Cash5;
	cashLabelArray[6] = textLabel_Cash6;
	cashLabelArray[7] = textLabel_Cash7;
	cashLabelArray[8] = textLabel_Cash8;
	cashLabelArray[9] = textLabel_Cash9;
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
		cashLabelArray[i]->setMyW(this);
	}

	playerTipLabelArray[0] = label_playerTip0;
	playerTipLabelArray[1] = label_playerTip1;
	playerTipLabelArray[2] = label_playerTip2;
	playerTipLabelArray[3] = label_playerTip3;
	playerTipLabelArray[4] = label_playerTip4;
	playerTipLabelArray[5] = label_playerTip5;
	playerTipLabelArray[6] = label_playerTip6;
	playerTipLabelArray[7] = label_playerTip7;
	playerTipLabelArray[8] = label_playerTip8;
	playerTipLabelArray[9] = label_playerTip9;

#ifdef GUI_800x480
	int j;
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; ++i) {
		for (j=1; j<=5; ++j) {
			playerStarsArray[j][i]->hide();
		}
		playerTipLabelArray[i]->hide();
	}

#endif

	// playerNameLabelArray init
	playerNameLabelArray[0] = label_PlayerName0;
	playerNameLabelArray[1] = label_PlayerName1;
	playerNameLabelArray[2] = label_PlayerName2;
	playerNameLabelArray[3] = label_PlayerName3;
	playerNameLabelArray[4] = label_PlayerName4;
	playerNameLabelArray[5] = label_PlayerName5;
	playerNameLabelArray[6] = label_PlayerName6;
	playerNameLabelArray[7] = label_PlayerName7;
	playerNameLabelArray[8] = label_PlayerName8;
	playerNameLabelArray[9] = label_PlayerName9;
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
		playerNameLabelArray[i]->setMyW(this);
	}

	// playerAvatarLabelArray init
	playerAvatarLabelArray[0] = label_Avatar0;
	playerAvatarLabelArray[1] = label_Avatar1;
	playerAvatarLabelArray[2] = label_Avatar2;
	playerAvatarLabelArray[3] = label_Avatar3;
	playerAvatarLabelArray[4] = label_Avatar4;
	playerAvatarLabelArray[5] = label_Avatar5;
	playerAvatarLabelArray[6] = label_Avatar6;
	playerAvatarLabelArray[7] = label_Avatar7;
	playerAvatarLabelArray[8] = label_Avatar8;
	playerAvatarLabelArray[9] = label_Avatar9;
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
		playerAvatarLabelArray[i]->setMyW(this);
		playerAvatarLabelArray[i]->setMyId(i);
	}

	// timeoutLabelArray init
	timeoutLabelArray[0] = label_Timeout0;
	timeoutLabelArray[1] = label_Timeout1;
	timeoutLabelArray[2] = label_Timeout2;
	timeoutLabelArray[3] = label_Timeout3;
	timeoutLabelArray[4] = label_Timeout4;
	timeoutLabelArray[5] = label_Timeout5;
	timeoutLabelArray[6] = label_Timeout6;
	timeoutLabelArray[7] = label_Timeout7;
	timeoutLabelArray[8] = label_Timeout8;
	timeoutLabelArray[9] = label_Timeout9;
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
		timeoutLabelArray[i]->setMyW(this);
	}

	// setLabelArray init
	setLabelArray[0] = textLabel_Set0;
	setLabelArray[1] = textLabel_Set1;
	setLabelArray[2] = textLabel_Set2;
	setLabelArray[3] = textLabel_Set3;
	setLabelArray[4] = textLabel_Set4;
	setLabelArray[5] = textLabel_Set5;
	setLabelArray[6] = textLabel_Set6;
	setLabelArray[7] = textLabel_Set7;
	setLabelArray[8] = textLabel_Set8;
	setLabelArray[9] = textLabel_Set9;
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
		setLabelArray[i]->setMyW(this);
	}
	label_Timeout0->setMyW(this);

	// statusLabelArray init
	actionLabelArray[0] = textLabel_Status0;
	actionLabelArray[1] = textLabel_Status1;
	actionLabelArray[2] = textLabel_Status2;
	actionLabelArray[3] = textLabel_Status3;
	actionLabelArray[4] = textLabel_Status4;
	actionLabelArray[5] = textLabel_Status5;
	actionLabelArray[6] = textLabel_Status6;
	actionLabelArray[7] = textLabel_Status7;
	actionLabelArray[8] = textLabel_Status8;
	actionLabelArray[9] = textLabel_Status9;

	textLabel_Status0->setMyW(this);

	//check position depending on card deck type
	checkActionLabelPosition();


	// GroupBoxArray init
	groupBoxArray[0] = groupBox0;
	groupBoxArray[1] = groupBox1;
	groupBoxArray[2] = groupBox2;
	groupBoxArray[3] = groupBox3;
	groupBoxArray[4] = groupBox4;
	groupBoxArray[5] = groupBox5;
	groupBoxArray[6] = groupBox6;
	groupBoxArray[7] = groupBox7;
	groupBoxArray[8] = groupBox8;
	groupBoxArray[9] = groupBox9;

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
	holeCardsArray[7][0] = pixmapLabel_card7a;
	holeCardsArray[7][1] = pixmapLabel_card7b;
	holeCardsArray[8][0] = pixmapLabel_card8a;
	holeCardsArray[8][1] = pixmapLabel_card8b;
	holeCardsArray[9][0] = pixmapLabel_card9a;
	holeCardsArray[9][1] = pixmapLabel_card9b;

	pushButton_showMyCards->hide();

	spectatorIcon = new QLabel(this);
	spectatorNumberLabel = new QLabel(this);

	//style Game Table
	refreshGameTableStyle();

	//raise actionLable above just inserted mypixmaplabel
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
		actionLabelArray[i]->raise();
	}

	//raise board cards
	for (i=0; i<5; i++) {
		boardCardsArray[i]->raise();
	}

	//fix for away string bug in righttabwidget on windows
#if (defined _WIN32) || (defined __APPLE__)
	tabWidget_Right->setTabText(0, " "+tabWidget_Right->tabText(0)+" ");
	tabWidget_Right->setTabText(1, " "+tabWidget_Right->tabText(1)+" ");
	tabWidget_Right->setTabText(2, " "+tabWidget_Right->tabText(2)+" ");
	tabWidget_Left->setTabText(0, " "+tabWidget_Left->tabText(0)+" ");
	tabWidget_Left->setTabText(1, " "+tabWidget_Left->tabText(1)+" ");
	tabWidget_Left->setTabText(2, " "+tabWidget_Left->tabText(2)+" ");
	tabWidget_Left->setTabText(3, " "+tabWidget_Left->tabText(3)+" ");

#endif

	//resize stop-button depending on translation
	QFontMetrics tempMetrics = this->fontMetrics();
	int width = tempMetrics.width(tr("Stop"));

	//Clear Focus
#ifdef GUI_800x480
	tabs.pushButton_break->setMinimumSize(width+10,20);
	tabs.groupBox_LeftToolBox->clearFocus();
	tabs.groupBox_RightToolBox->clearFocus();
#else
	pushButton_break->setMinimumSize(width+10,20);
	groupBox_LeftToolBox->clearFocus();
	groupBox_RightToolBox->clearFocus();
#endif

	//set Focus to gametable
	this->setFocus();

	//windowicon
	// 	QString windowIconString();
	this->setWindowIcon(QIcon(myAppDataPath+"gfx/gui/misc/windowicon.png"));

	// 	Dialogs
#ifdef GUI_800x480
	myChat = new ChatTools(tabs.lineEdit_ChatInput, myConfig, INGAME_CHAT, tabs.textBrowser_Chat);
	myChat->setMyStyle(myGameTableStyle);
	tabs.lineEdit_ChatInput->installEventFilter(this);
#else
	myChat = new ChatTools(lineEdit_ChatInput, myConfig, INGAME_CHAT, textBrowser_Chat);
	myChat->setMyStyle(myGameTableStyle);
	lineEdit_ChatInput->installEventFilter(this);
#endif

	this->installEventFilter(this);

	// create universal messageDialgo
	myUniversalMessageDialog = new myMessageDialogImpl(myConfig, this);
	myUniversalMessageDialog->setParent(this);

	//hide left and right icon and menubar from maemo gui for ANDROID
#ifdef ANDROID
	fullscreenButton->hide();
	this->setMenuBar(0);
#endif

	//Connects
#ifdef GUI_800x480
	connect(tabs.pushButton_tipSave, SIGNAL( clicked(bool) ), playerAvatarLabelArray[0], SLOT ( setPlayerTip() ) );
#else
	connect(pushButton_tipSave, SIGNAL( clicked(bool) ), playerAvatarLabelArray[0], SLOT ( setPlayerTip() ) );
#endif
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

	connect(blinkingStartButtonAnimationTimer, SIGNAL(timeout()), this, SLOT( blinkingStartButtonAnimationAction()));
	connect(voteOnKickTimeoutTimer, SIGNAL(timeout()), this, SLOT(nextVoteOnKickTimeoutAnimationFrame()));
	connect(enableCallCheckPushButtonTimer, SIGNAL(timeout()), this, SLOT(enableCallCheckPushButton()));

#ifdef ANDROID
	connect( tabs.pushButton_settings, SIGNAL( clicked() ), this, SLOT( callSettingsDialog() ) );
#else
	connect( actionConfigure_PokerTH, SIGNAL( triggered() ), this, SLOT( callSettingsDialog() ) );
#endif

	connect( actionClose, SIGNAL( triggered() ), this, SLOT( closeGameTable()) );

#ifdef GUI_800x480
	connect( fullscreenButton, SIGNAL( clicked() ), this, SLOT( switchFullscreen() ) );
#else
	connect( actionFullScreen, SIGNAL( triggered() ), this, SLOT( switchFullscreen() ) );
#endif

	connect( actionShowHideChat, SIGNAL( triggered() ), this, SLOT( switchChatWindow() ) );
	connect( actionShowHideHelp, SIGNAL( triggered() ), this, SLOT( switchHelpWindow() ) );
	connect( actionShowHideLog, SIGNAL( triggered() ), this, SLOT( switchLogWindow() ) );
	connect( actionShowHideAway, SIGNAL( triggered() ), this, SLOT( switchAwayWindow() ) );
	connect( actionShowHideChance, SIGNAL( triggered() ), this, SLOT( switchChanceWindow() ) );

	connect( pushButton_BetRaise, SIGNAL( clicked(bool) ), this, SLOT( pushButtonBetRaiseClicked(bool) ) );
	connect( pushButton_Fold, SIGNAL( clicked(bool) ), this, SLOT( pushButtonFoldClicked(bool) ) );
	connect( pushButton_CallCheck, SIGNAL( clicked(bool) ), this, SLOT( pushButtonCallCheckClicked(bool) ) );
	connect( pushButton_AllIn, SIGNAL( clicked(bool) ), this, SLOT(pushButtonAllInClicked(bool) ) );
	connect( horizontalSlider_bet, SIGNAL( valueChanged(int)), this, SLOT ( changeSpinBoxBetValue(int) ) );
	connect( spinBox_betValue, SIGNAL( valueChanged(int)), this, SLOT ( spinBoxBetValueChanged(int) ) );

#ifdef GUI_800x480
	connect( tabs.horizontalSlider_speed, SIGNAL( valueChanged(int)), this, SLOT ( setGameSpeed(int) ) );
	connect( tabs.pushButton_break, SIGNAL( clicked()), this, SLOT ( breakButtonClicked() ) ); // auch wieder starten!!!!

	connect( tabs.tabWidget_Left, SIGNAL( currentChanged(int) ), this, SLOT( tabSwitchAction() ) );
	connect( tabs.lineEdit_ChatInput, SIGNAL( returnPressed () ), this, SLOT( sendChatMessage() ) );
	connect( tabs.lineEdit_ChatInput, SIGNAL( textChanged (QString) ), this, SLOT( checkChatInputLength(QString) ) );
	connect( tabs.lineEdit_ChatInput, SIGNAL( textEdited (QString) ), myChat, SLOT( setChatTextEdited() ) );

	connect( tabs.radioButton_manualAction, SIGNAL( clicked() ) , this, SLOT( changePlayingMode() ) );
	connect( tabs.radioButton_autoCheckFold, SIGNAL( clicked() ) , this, SLOT( changePlayingMode() ) );
	connect( tabs.radioButton_autoCheckCallAny, SIGNAL( clicked() ), this, SLOT( changePlayingMode() ) );

	connect( tabs.pushButton_voteOnKickYes, SIGNAL( clicked() ), this, SLOT( voteOnKickYes() ) );
	connect( tabs.pushButton_voteOnKickNo, SIGNAL( clicked() ), this, SLOT( voteOnKickNo() ) );
#else
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
#endif

	connect( pushButton_showMyCards, SIGNAL( clicked() ), this, SLOT( sendShowMyCardsSignal() ) );
	for(i=0; i<=9; i++)connect( playerTipLabelArray[i], SIGNAL( linkActivated(QString) ), playerAvatarLabelArray[i], SLOT(startChangePlayerTip(QString) ) );
	for(i=0; i<=9; i++) {
		for(int j=1; j<=5; j++) {
			connect( playerStarsArray[j][i], SIGNAL( linkActivated(QString) ), playerAvatarLabelArray[i], SLOT(setPlayerRating(QString) ) );
		}
	}
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
	connect(this, SIGNAL(signalRefreshSpectatorsDisplay()), this, SLOT(refreshSpectatorsDisplay()));
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
	connect(this, SIGNAL(signalPostRiverShowCards(unsigned)), this, SLOT(showHoleCards(unsigned)));
	connect(this, SIGNAL(signalFlipHolecardsAllIn()), this, SLOT(flipHolecardsAllIn()));
	connect(this, SIGNAL(signalNextRoundCleanGui()), this, SLOT(nextRoundCleanGui()));
	connect(this, SIGNAL(signalStartVoteOnKick(unsigned, unsigned, int, int)), this, SLOT(startVoteOnKick(unsigned, unsigned, int, int)));
	connect(this, SIGNAL(signalChangeVoteOnKickButtonsState(bool)), this, SLOT(changeVoteOnKickButtonsState(bool)));
	connect(this, SIGNAL(signalEndVoteOnKick()), this, SLOT(endVoteOnKick()));
	connect(this, SIGNAL(signalNetClientPlayerLeft(unsigned)), this, SLOT(netClientPlayerLeft(unsigned)));
	connect(this, SIGNAL(signalNetClientSpectatorLeft(unsigned)), this, SLOT(netClientSpectatorLeft(unsigned)));
	connect(this, SIGNAL(signalNetClientSpectatorJoined(unsigned)), this, SLOT(netClientSpectatorJoined(unsigned)));
	connect(this, SIGNAL(signalNetClientPingUpdate(unsigned, unsigned, unsigned)), this, SLOT(pingUpdate(unsigned, unsigned, unsigned)));

#ifdef GUI_800x480
	connect( tabsButton, SIGNAL( clicked() ), this, SLOT( tabsButtonClicked() ) );
#endif
}

gameTableImpl::~gameTableImpl()
{


}

void gameTableImpl::callSettingsDialog()
{
	bool iamInGame = true;
	myStartWindow->callSettingsDialog(iamInGame);
}

void gameTableImpl::applySettings(settingsDialogImpl* mySettingsDialog)
{
#ifndef GUI_800x480 //currently not for mobile guis because we just use the default style here
	//apply card deck style
	myCardDeckStyle->readStyleFile(QString::fromUtf8(myConfig->readConfigString("CurrentCardDeckStyle").c_str()));
	checkActionLabelPosition();
	//apply game table style
	myGameTableStyle->readStyleFile(QString::fromUtf8(myConfig->readConfigString("CurrentGameTableStyle").c_str()));
#endif

#ifdef GUI_800x480
	//cardschancemonitor show/hide
	if (!myConfig->readConfigInt("ShowCardsChanceMonitor")) {
		tabs.tabWidget_Right->removeTab(2);
		tabs.tabWidget_Right->setCurrentIndex(0);
	} else {
		if(tabs.tabWidget_Right->widget(2) != tabs.tab_Chance)
			tabs.tabWidget_Right->insertTab(2, tabs.tab_Chance, QString(tr("Chance")));
	}
#else
	//Toolbox verstecken?
	if (myConfig->readConfigInt("ShowLeftToolBox")) {
		groupBox_LeftToolBox->show();
	} else {
		groupBox_LeftToolBox->hide();
	}

	if (myConfig->readConfigInt("ShowRightToolBox")) {
		groupBox_RightToolBox->show();
	} else {
		groupBox_RightToolBox->hide();
	}

	//cardschancemonitor show/hide
	if (!myConfig->readConfigInt("ShowCardsChanceMonitor")) {
		tabWidget_Right->removeTab(2);
		tabWidget_Right->setCurrentIndex(0);
	} else {
		if(tabWidget_Right->widget(2) != tab_Chance)
			tabWidget_Right->insertTab(2, tab_Chance, QString(tr("Chance")));
	}
#endif

	//Add avatar (if set)
	myStartWindow->getSession()->addOwnAvatar(QString::fromUtf8(myConfig->readConfigString("MyAvatar").c_str()).toLocal8Bit().constData());

	//Falls Spielernamen geändert wurden --> neu zeichnen --> erst beim nächsten Neustart neu ausgelesen
	if (mySettingsDialog->getPlayerNickIsChanged() && myStartWindow->getSession()->getCurrentGame() && !myStartWindow->getSession()->isNetworkClientRunning()) {

		boost::shared_ptr<Game> currentGame = myStartWindow->getSession()->getCurrentGame();
		PlayerListIterator it = currentGame->getSeatsList()->begin();
		(*it)->setMyName(mySettingsDialog->lineEdit_HumanPlayerName->text().toUtf8().constData());
		(*(++it))->setMyName(mySettingsDialog->lineEdit_Opponent1Name->text().toUtf8().constData());
		(*(++it))->setMyName(mySettingsDialog->lineEdit_Opponent2Name->text().toUtf8().constData());
		(*(++it))->setMyName(mySettingsDialog->lineEdit_Opponent3Name->text().toUtf8().constData());
		(*(++it))->setMyName(mySettingsDialog->lineEdit_Opponent4Name->text().toUtf8().constData());
		(*(++it))->setMyName(mySettingsDialog->lineEdit_Opponent5Name->text().toUtf8().constData());
		(*(++it))->setMyName(mySettingsDialog->lineEdit_Opponent6Name->text().toUtf8().constData());
		(*(++it))->setMyName(mySettingsDialog->lineEdit_Opponent7Name->text().toUtf8().constData());
		(*(++it))->setMyName(mySettingsDialog->lineEdit_Opponent8Name->text().toUtf8().constData());
		(*(++it))->setMyName(mySettingsDialog->lineEdit_Opponent9Name->text().toUtf8().constData());
		mySettingsDialog->setPlayerNickIsChanged(false);

		refreshPlayerName();
	}

	if(myStartWindow->getSession()->getCurrentGame() && !myStartWindow->getSession()->isNetworkClientRunning()) {

		boost::shared_ptr<Game> currentGame = myStartWindow->getSession()->getCurrentGame();
		PlayerListIterator it = currentGame->getSeatsList()->begin();
		(*it)->setMyAvatar(mySettingsDialog->pushButton_HumanPlayerAvatar->getMyLink().toUtf8().constData());
		(*(++it))->setMyAvatar(mySettingsDialog->pushButton_Opponent1Avatar->getMyLink().toUtf8().constData());
		(*(++it))->setMyAvatar(mySettingsDialog->pushButton_Opponent2Avatar->getMyLink().toUtf8().constData());
		(*(++it))->setMyAvatar(mySettingsDialog->pushButton_Opponent3Avatar->getMyLink().toUtf8().constData());
		(*(++it))->setMyAvatar(mySettingsDialog->pushButton_Opponent4Avatar->getMyLink().toUtf8().constData());
		(*(++it))->setMyAvatar(mySettingsDialog->pushButton_Opponent5Avatar->getMyLink().toUtf8().constData());
		(*(++it))->setMyAvatar(mySettingsDialog->pushButton_Opponent6Avatar->getMyLink().toUtf8().constData());
		(*(++it))->setMyAvatar(mySettingsDialog->pushButton_Opponent7Avatar->getMyLink().toUtf8().constData());
		(*(++it))->setMyAvatar(mySettingsDialog->pushButton_Opponent8Avatar->getMyLink().toUtf8().constData());
		(*(++it))->setMyAvatar(mySettingsDialog->pushButton_Opponent9Avatar->getMyLink().toUtf8().constData());

		//avatar refresh
		refreshPlayerAvatar();
	}

	//refresh board cards if game is running
	if(myStartWindow->getSession()->getCurrentGame()) {

		int tempBoardCardsArray[5];
		myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getBoard()->getMyCards(tempBoardCardsArray);
		GameState currentState = myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getCurrentBeRo()->getMyBeRoID();
		if(currentState >= GAME_STATE_FLOP && currentState <= GAME_STATE_POST_RIVER)
			for(int i=0; i<3; i++) {
				QPixmap card = QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempBoardCardsArray[i], 10)+".png"));
				boardCardsArray[i]->setPixmap(card, false);
			}
		if(currentState >= GAME_STATE_TURN && currentState <= GAME_STATE_POST_RIVER) {
			QPixmap card = QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempBoardCardsArray[3], 10)+".png"));
			boardCardsArray[3]->setPixmap(card, false);
		}
		if(currentState == GAME_STATE_RIVER || currentState == GAME_STATE_POST_RIVER) {
			QPixmap card = QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempBoardCardsArray[4], 10)+".png"));
			boardCardsArray[4]->setPixmap(card, false);
		}
	}

	//Flipside refresh
	if (myConfig->readConfigInt("FlipsideOwn") && myConfig->readConfigString("FlipsideOwnFile") != "") {
		flipside = QPixmap::fromImage(QImage(QString::fromUtf8(myConfig->readConfigString("FlipsideOwnFile").c_str())));
	} else {
		flipside = QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+"flipside.png"));
	}
	int j,k;
	for (j=1; j<MAX_NUMBER_OF_PLAYERS; j++ ) {
		for ( k=0; k<=1; k++ ) {
			if (holeCardsArray[j][k]->getIsFlipside()) {
				holeCardsArray[j][k]->setPixmap(flipside, true);
			}
		}
	}

	//Check for anti-peek mode
	if(myStartWindow->getSession()->getCurrentGame()) {
		// 		check if human player is already active
		boost::shared_ptr<PlayerInterface> humanPlayer = myStartWindow->getSession()->getCurrentGame()->getSeatsList()->front();
		if(humanPlayer->getMyActiveStatus()) {

			QPixmap tempCardsPixmapArray[2];
			int tempCardsIntArray[2];

			humanPlayer->getMyCards(tempCardsIntArray);
			if(myConfig->readConfigInt("AntiPeekMode")) {
				holeCardsArray[0][0]->setPixmap(flipside, true);
				tempCardsPixmapArray[0] = QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempCardsIntArray[0], 10)+".png"));
				holeCardsArray[0][0]->setHiddenFrontPixmap(tempCardsPixmapArray[0]);
				holeCardsArray[0][1]->setPixmap(flipside, true);
				tempCardsPixmapArray[1]= QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempCardsIntArray[1], 10)+".png"));
				holeCardsArray[0][1]->setHiddenFrontPixmap(tempCardsPixmapArray[1]);
			} else {
				tempCardsPixmapArray[0]= QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempCardsIntArray[0], 10)+".png"));
				holeCardsArray[0][0]->setPixmap(tempCardsPixmapArray[0],false);
				tempCardsPixmapArray[1]= QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempCardsIntArray[1], 10)+".png"));
				holeCardsArray[0][1]->setPixmap(tempCardsPixmapArray[1],false);
			}
		}
	}
#ifndef GUI_800x480 //currently not for mobile guis because we just use the default style here
	refreshGameTableStyle();
#endif
	if(this->isVisible() && myGameTableStyle->getState() != GT_STYLE_OK) myGameTableStyle->showErrorMessage();

	//blind buttons refresh
	if(myStartWindow->getSession()->getCurrentGame()) {
		refreshButton();
		refreshGroupbox();
		provideMyActions();
	}

	mySoundEventHandler->reInitSoundEngine();
}

void gameTableImpl::initGui(int speed)
{
	//kill running Singleshots!!!
	stopTimer();

	label_handNumber->setText(HandString+":");
	label_gameNumber->setText(GameString+":");

	//set WindowTitle dynamically
	QString titleString = "";
	assert(myStartWindow->getSession());
	if(myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_INTERNET || myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_NETWORK) {
		GameInfo info(myStartWindow->getSession()->getClientGameInfo(myStartWindow->getSession()->getClientCurrentGameId()));
		titleString = QString::fromUtf8(info.name.c_str())+" - ";
	}

	//show human player buttons
	for(int i=0; i<6; i++) {
		userWidgetsArray[i]->show();
	}

	//set speeds for local game and for first network game
	if( !myStartWindow->getSession()->isNetworkClientRunning() || (myStartWindow->getSession()->isNetworkClientRunning() && !myStartWindow->getSession()->getCurrentGame()) ) {
		guiGameSpeed = speed;
		setSpeeds();
	}

	//set session for chat
	myChat->setSession(this->getSession());

#ifdef GUI_800x480
	tabsDiag->setWindowTitle("Tabs");
	this->setWindowTitle(QString(titleString + tr("PokerTH %1").arg(POKERTH_BETA_RELEASE_STRING)));

	label_Sets->setText(BetsString);
	label_Total->setText(TotalString);
	tabs.groupBox_RightToolBox->setDisabled(false);
	tabs.groupBox_LeftToolBox->setDisabled(false);

	//set minimum gui speed to prevent gui lags on fast inet games
	if( myStartWindow->getSession()->isNetworkClientRunning() ) {
		tabs.horizontalSlider_speed->setMinimum(speed);
	} else {
		tabs.horizontalSlider_speed->setMinimum(1);
	}

	//positioning Slider
	tabs.horizontalSlider_speed->setValue(guiGameSpeed);

#else
	this->setWindowTitle(QString(titleString + tr("PokerTH %1 - The Open-Source Texas Holdem Engine").arg(POKERTH_BETA_RELEASE_STRING)));

	label_Pot->setText(PotString);
	label_Total->setText(TotalString+":");
	label_Sets->setText(BetsString+":");
	groupBox_RightToolBox->setDisabled(false);
	groupBox_LeftToolBox->setDisabled(false);

	//set minimum gui speed to prevent gui lags on fast inet games
	if( myStartWindow->getSession()->isNetworkClientRunning() ) {
		horizontalSlider_speed->setMinimum(speed);
	} else {
		horizontalSlider_speed->setMinimum(1);
	}

	horizontalSlider_speed->setValue(guiGameSpeed);

#endif

}

boost::shared_ptr<Session> gameTableImpl::getSession()
{
	assert(myStartWindow->getSession().get());
	return myStartWindow->getSession();
}

//refresh-Funktionen
void gameTableImpl::refreshSet()
{

	boost::shared_ptr<Game> currentGame = myStartWindow->getSession()->getCurrentGame();

	PlayerListConstIterator it_c;
	PlayerList seatsList = currentGame->getSeatsList();
	for (it_c=seatsList->begin(); it_c!=seatsList->end(); ++it_c) {
		if( (*it_c)->getMySet() == 0 )
			setLabelArray[(*it_c)->getMyID()]->setText("");
		else
			setLabelArray[(*it_c)->getMyID()]->setText("$"+QString("%L1").arg((*it_c)->getMySet()));
	}
}

void gameTableImpl::refreshButton()
{

	QPixmap dealerButton = QPixmap::fromImage(QImage(myGameTableStyle->getDealerPuck()));
	QPixmap smallblindButton = QPixmap::fromImage(QImage(myGameTableStyle->getSmallBlindPuck()));
	QPixmap bigblindButton = QPixmap::fromImage(QImage(myGameTableStyle->getBigBlindPuck()));
	QPixmap onePix = QPixmap::fromImage(QImage(myAppDataPath +"gfx/gui/misc/1px.png"));

	boost::shared_ptr<Game> currentGame = myStartWindow->getSession()->getCurrentGame();

	PlayerListConstIterator it_c;
	PlayerList seatsList = currentGame->getSeatsList();
	PlayerList activePlayerList = currentGame->getActivePlayerList();
	for (it_c=seatsList->begin(); it_c!=seatsList->end(); ++it_c) {
		if( (*it_c)->getMyActiveStatus() ) {
			if( activePlayerList->size() > 2 ) {
				switch ( (*it_c)->getMyButton() ) {

				case 1 :
					buttonLabelArray[(*it_c)->getMyID()]->setPixmap(dealerButton);
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
				default:
					buttonLabelArray[(*it_c)->getMyID()]->setPixmap(onePix);

				}
			} else {
				switch ((*it_c)->getMyButton()) {

				case 2 :
					buttonLabelArray[(*it_c)->getMyID()]->setPixmap(dealerButton);
					break;
				case 3 : {
					if(myConfig->readConfigInt("ShowBlindButtons"))
						buttonLabelArray[(*it_c)->getMyID()]->setPixmap(bigblindButton);
					else
						buttonLabelArray[(*it_c)->getMyID()]->setPixmap(onePix);
				}
				break;
				default:
					buttonLabelArray[(*it_c)->getMyID()]->setPixmap(onePix);

				}
			}
		} else {
			buttonLabelArray[(*it_c)->getMyID()]->setPixmap(onePix);
		}
	}
}


void gameTableImpl::refreshPlayerName()
{

	if(myStartWindow->getSession()->getCurrentGame()) {

		boost::shared_ptr<Game> currentGame = myStartWindow->getSession()->getCurrentGame();
		PlayerListConstIterator it_c;
		PlayerList seatsList = currentGame->getSeatsList();
		for (it_c=seatsList->begin(); it_c!=seatsList->end(); ++it_c) {

			//collect needed infos
			bool guest = myStartWindow->getSession()->getClientPlayerInfo((*it_c)->getMyUniqueID()).isGuest;
			bool computerPlayer = false;
			if((*it_c)->getMyType() == PLAYER_TYPE_COMPUTER) {
				computerPlayer = true;
			}
			QString nick = QString::fromUtf8((*it_c)->getMyName().c_str());

			//check SeatStates and refresh
			switch(getCurrentSeatState((*it_c))) {

			case SEAT_ACTIVE: {
				playerNameLabelArray[(*it_c)->getMyID()]->setText(nick, false, guest, computerPlayer );
			}
			break;
			case SEAT_AUTOFOLD: {
				playerNameLabelArray[(*it_c)->getMyID()]->setText(nick, true, guest, computerPlayer );
			}
			break;
			case SEAT_STAYONTABLE: {
				playerNameLabelArray[(*it_c)->getMyID()]->setText(nick, true, guest, computerPlayer );
			}
			break;
			case SEAT_CLEAR: {
				playerNameLabelArray[(*it_c)->getMyID()]->setText("");
			}
			break;
			default: {
				playerNameLabelArray[(*it_c)->getMyID()]->setText("");
			}
			}
		}
	}

	playerAvatarLabelArray[0]->refreshTooltips();
}

void gameTableImpl::refreshPlayerAvatar()
{

	if(myStartWindow->getSession()->getCurrentGame()) {

		QPixmap onePix = QPixmap::fromImage(QImage(myAppDataPath +"gfx/gui/misc/1px.png"));

		boost::shared_ptr<Game> currentGame = myStartWindow->getSession()->getCurrentGame();
		int seatPlace;
		PlayerListConstIterator it_c;
		PlayerList seatsList = currentGame->getSeatsList();
		for (it_c=seatsList->begin(), seatPlace=0; it_c!=seatsList->end(); ++it_c, seatPlace++) {

			//set uniqueID
			playerAvatarLabelArray[(*it_c)->getMyID()]->setMyUniqueId((*it_c)->getMyUniqueID());

			//get CountryString
			QString countryString(QString(myStartWindow->getSession()->getClientPlayerInfo((*it_c)->getMyUniqueID()).countryCode.c_str()).toLower());
			countryString = QString(":/cflags/cflags/%1.png").arg(countryString);

			//get AvatarPic
			QFile myAvatarFile(QString::fromUtf8((*it_c)->getMyAvatar().c_str()));
			QPixmap avatarPic;
			if((*it_c)->getMyAvatar() == "" || !myAvatarFile.exists()) {
				avatarPic = QPixmap::fromImage(QImage(myGameTableStyle->getDefaultAvatar()));
			} else {
				avatarPic = QPixmap::fromImage(QImage(QString::fromUtf8((*it_c)->getMyAvatar().c_str())));
			}

			//check SeatStates and refresh
			switch(getCurrentSeatState((*it_c))) {

			case SEAT_ACTIVE: {
				playerAvatarLabelArray[(*it_c)->getMyID()]->setPixmapAndCountry(avatarPic, countryString, seatPlace);
			}
			break;
			case SEAT_AUTOFOLD: {
//				qDebug() << seatPlace << "AVATAR AUTOFOLD";
				playerAvatarLabelArray[(*it_c)->getMyID()]->setPixmapAndCountry(avatarPic, countryString, seatPlace, true);
			}
			break;
			case SEAT_STAYONTABLE: {
//				qDebug() << seatPlace << "AVATAR STAYONTABLE";
				playerAvatarLabelArray[(*it_c)->getMyID()]->setPixmapAndCountry(avatarPic, countryString, seatPlace, true);
			}
			break;
			case SEAT_CLEAR: {
				playerAvatarLabelArray[(*it_c)->getMyID()]->setPixmap(onePix);
			}
			break;
			default: {
				playerAvatarLabelArray[(*it_c)->getMyID()]->setPixmap(onePix);
			}
			}
		}
	}
}

void gameTableImpl::setPlayerAvatar(int myID, QString myAvatar)
{

	if(myStartWindow->getSession()->getCurrentGame()) {

		boost::shared_ptr<PlayerInterface> tmpPlayer = myStartWindow->getSession()->getCurrentGame()->getPlayerByUniqueId(myID);
		if (tmpPlayer.get()) {

			QFile myAvatarFile(myAvatar);
			if(myAvatarFile.exists()) {
				playerAvatarLabelArray[tmpPlayer->getMyID()]->setPixmap(myAvatar);
				tmpPlayer->setMyAvatar(myAvatar.toUtf8().constData());
			} else {
				playerAvatarLabelArray[tmpPlayer->getMyID()]->setPixmap(QPixmap::fromImage(QImage(myGameTableStyle->getDefaultAvatar())));
				tmpPlayer->setMyAvatar("");
			}
		}
	}
}

void gameTableImpl::refreshAction(int playerID, int playerAction)
{

	QPixmap onePix = QPixmap::fromImage(QImage(myAppDataPath +"gfx/gui/misc/1px.png"));
	QPixmap action;

	QStringList actionArray;
	actionArray << "" << "fold" << "check" << "call" << "bet" << "raise" << "allin";

	boost::shared_ptr<Game> currentGame = myStartWindow->getSession()->getCurrentGame();

	if(playerID == -1 || playerAction == -1) {

		PlayerListConstIterator it_c;
		PlayerList seatsList = currentGame->getSeatsList();
		for (it_c=seatsList->begin(); it_c!=seatsList->end(); ++it_c) {

			//if no action --> clear Pixmap
			if( (*it_c)->getMyAction() == 0) {
				actionLabelArray[(*it_c)->getMyID()]->setPixmap(onePix);
			} else {
				//paint action pixmap
				actionLabelArray[(*it_c)->getMyID()]->setPixmap(QPixmap::fromImage(QImage(myGameTableStyle->getActionPic((*it_c)->getMyAction()))));
			}

			if ((*it_c)->getMyAction()==1) {

				if((*it_c)->getMyID() != 0) {
					holeCardsArray[(*it_c)->getMyID()][0]->setPixmap(onePix, false);
					holeCardsArray[(*it_c)->getMyID()][1]->setPixmap(onePix, false);
				}
			}
		}
	} else {
		//if no action --> clear Pixmap
		if(playerAction == 0) {
			actionLabelArray[playerID]->setPixmap(onePix);
		} else {

			// 		paint action pixmap and raise
			actionLabelArray[playerID]->setPixmap(QPixmap::fromImage(QImage(myGameTableStyle->getActionPic(playerAction))));

			//play sounds if exist
			if(myConfig->readConfigInt("PlayGameActions"))
				mySoundEventHandler->playSound(actionArray[playerAction].toStdString(), playerID);
		}

		if (playerAction == 1) { // FOLD

			if (playerID == 0) {
				holeCardsArray[0][0]->startFadeOut(10);
				holeCardsArray[0][1]->startFadeOut(10);
			} else {
				holeCardsArray[playerID][0]->setPixmap(onePix, false);
				holeCardsArray[playerID][1]->setPixmap(onePix, false);
			}
		}
	}
}

void gameTableImpl::refreshCash()
{

	boost::shared_ptr<Game> currentGame = myStartWindow->getSession()->getCurrentGame();

	bool transparent = true;
	PlayerListConstIterator it_c;
	PlayerList seatsList = currentGame->getSeatsList();
	for (it_c=seatsList->begin(); it_c!=seatsList->end(); ++it_c) {

		//check SeatStates and refresh
		switch(getCurrentSeatState((*it_c))) {

		case SEAT_ACTIVE: {
//			qDebug() << (*it_c)->getMyID() << "CASH ACTIVE";
			cashLabelArray[(*it_c)->getMyID()]->setText("$"+QString("%L1").arg((*it_c)->getMyCash()));
		}
		break;
		case SEAT_AUTOFOLD: {
//			qDebug() << (*it_c)->getMyID() << "CASH AUTOFOLD"; //TODO transparent
			cashLabelArray[(*it_c)->getMyID()]->setText("$"+QString("%L1").arg((*it_c)->getMyCash()), transparent);
		}
		break;
		case SEAT_STAYONTABLE: {
			cashLabelArray[(*it_c)->getMyID()]->setText("");
		}
		break;
		case SEAT_CLEAR: {
			cashLabelArray[(*it_c)->getMyID()]->setText("");
		}
		break;
		default: {
			cashLabelArray[(*it_c)->getMyID()]->setText("");
		}
		}
	}
}

void gameTableImpl::refreshGroupbox(int playerID, int status)
{

	int j;

	if(playerID == -1 || status == -1) {

		boost::shared_ptr<Game> currentGame = myStartWindow->getSession()->getCurrentGame();
		PlayerListConstIterator it_c;
		PlayerList seatsList = currentGame->getSeatsList();
		for (it_c=seatsList->begin(); it_c!=seatsList->end(); ++it_c) {

			if((*it_c)->getMyTurn()) {
				//Groupbox glow wenn der Spiele dran ist.
				myGameTableStyle->setPlayerSeatActiveStyle(groupBoxArray[(*it_c)->getMyID()]);
			} else {
				//Groupbox auf Hintergrundfarbe setzen wenn der Spiele nicht dran aber aktiv ist.
				if((*it_c)->getMyActiveStatus()) {
					if((*it_c)->getMyID()==0) {
						//show buttons
						for(j=0; j<6; j++) {
							userWidgetsArray[j]->show();
						}
					}
					myGameTableStyle->setPlayerSeatInactiveStyle(groupBoxArray[(*it_c)->getMyID()]);

				}
				//Groupbox verdunkeln wenn der Spiele inactive ist.
				else {
					if((*it_c)->getMyID()==0) {
						//hide buttons
						for(j=0; j<6; j++) {
							userWidgetsArray[j]->hide();
						}
						//disable anti-peek front after player is out
						holeCardsArray[0][0]->signalFastFlipCards(false);
						holeCardsArray[0][1]->signalFastFlipCards(false);
					}
					myGameTableStyle->setPlayerSeatInactiveStyle(groupBoxArray[(*it_c)->getMyID()]);
				}
			}
		}
	} else {
		switch(status) {

		//inactive
		case 0: {
			if (!playerID) {
				//hide buttons
				for(j=0; j<6; j++) {
					userWidgetsArray[j]->hide();
				}
				//disable anti-peek front after player is out
				holeCardsArray[0][0]->signalFastFlipCards(false);
				holeCardsArray[0][1]->signalFastFlipCards(false);
			}
			myGameTableStyle->setPlayerSeatInactiveStyle(groupBoxArray[playerID]);
		}
		break;
		//active but fold
		case 1: {
			if (!playerID) {
				//show buttons
				for(j=0; j<6; j++) {
					userWidgetsArray[j]->show();
				}
			}
			myGameTableStyle->setPlayerSeatInactiveStyle(groupBoxArray[playerID]);
		}
		break;
		//active in action
		case 2:  {
			myGameTableStyle->setPlayerSeatActiveStyle(groupBoxArray[playerID]);
		}
		break;
		//active not in action
		case 3:  {
			myGameTableStyle->setPlayerSeatInactiveStyle(groupBoxArray[playerID]);
		}
		break;
		default: {
		}
		}
	}
}

void gameTableImpl::refreshGameLabels(int gameState)
{

	switch(gameState) {
	case 0: {
		textLabel_handLabel->setText(PreflopString);
	}
	break;
	case 1: {
		textLabel_handLabel->setText(FlopString);
	}
	break;
	case 2: {
		textLabel_handLabel->setText(TurnString);
	}
	break;
	case 3: {
		textLabel_handLabel->setText(RiverString);
	}
	break;
	case 4: {
		textLabel_handLabel->setText("");
	}
	break;
	default: {
		textLabel_handLabel->setText("!!! ERROR !!!");
	}
	}

	label_handNumberValue->setText(QString::number(myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getMyID(),10));
	label_gameNumberValue->setText(QString::number(myStartWindow->getSession()->getCurrentGame()->getMyGameID(),10));
}

void gameTableImpl::refreshAll()
{

	refreshSet();
	refreshButton();

	boost::shared_ptr<Game> currentGame = myStartWindow->getSession()->getCurrentGame();
	PlayerListConstIterator it_c;
	PlayerList seatsList = currentGame->getSeatsList();
	for (it_c=seatsList->begin(); it_c!=seatsList->end(); ++it_c) {
		refreshAction( (*it_c)->getMyID(), (*it_c)->getMyAction());
	}

	refreshCash();
	refreshGroupbox();
	refreshPlayerName();
	refreshPlayerAvatar();
}

void gameTableImpl::refreshChangePlayer()
{

	refreshSet();

	boost::shared_ptr<Game> currentGame = myStartWindow->getSession()->getCurrentGame();
	PlayerListConstIterator it_c;
	PlayerList seatsList = currentGame->getSeatsList();
	for (it_c=seatsList->begin(); it_c!=seatsList->end(); ++it_c) {
		refreshAction( (*it_c)->getMyID(), (*it_c)->getMyAction());
	}

	refreshCash();
}

void gameTableImpl::refreshPot()
{
	boost::shared_ptr<HandInterface> currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();

	textLabel_Sets->setText("$"+QString("%L1").arg(currentHand->getBoard()->getSets()));
	textLabel_Pot->setText("$"+QString("%L1").arg(currentHand->getBoard()->getPot()));
}

void gameTableImpl::guiUpdateDone()
{
	guiUpdateSemaphore.release();
}

void gameTableImpl::waitForGuiUpdateDone()
{
	guiUpdateSemaphore.acquire();
}

void gameTableImpl::dealHoleCards()
{

	int i,k;
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++ ) {
		for ( k=0; k<=1; k++ ) {
			holeCardsArray[i][k]->setFadeOutAction(false);
			holeCardsArray[i][k]->stopFlipCardsAnimation();
		}
	}

	QPixmap onePix = QPixmap::fromImage(QImage(myAppDataPath +"gfx/gui/misc/1px.png"));

	//TempArrays
	QPixmap tempCardsPixmapArray[2];
	int tempCardsIntArray[2];

	// Karten der Gegner und eigene Karten austeilen
	int j;
	boost::shared_ptr<Game> currentGame = myStartWindow->getSession()->getCurrentGame();

	PlayerListConstIterator it_c;
	PlayerList seatsList = currentGame->getSeatsList();
	for (it_c=seatsList->begin(); it_c!=seatsList->end(); ++it_c) {
		(*it_c)->getMyCards(tempCardsIntArray);
		for(j=0; j<2; j++) {
			if((*it_c)->getMyActiveStatus()) {
				if (( (*it_c)->getMyID() == 0)/* || DEBUG_MODE*/) {
					tempCardsPixmapArray[j].load(myCardDeckStyle->getCurrentDir()+QString::number(tempCardsIntArray[j], 10)+".png");
					if(myConfig->readConfigInt("AntiPeekMode")) {
						holeCardsArray[(*it_c)->getMyID()][j]->setPixmap(flipside, true);
						holeCardsArray[(*it_c)->getMyID()][j]->setFront(flipside);
						holeCardsArray[(*it_c)->getMyID()][j]->setHiddenFrontPixmap(tempCardsPixmapArray[j]);
					} else {
						holeCardsArray[(*it_c)->getMyID()][j]->setPixmap(tempCardsPixmapArray[j],false);
						holeCardsArray[(*it_c)->getMyID()][j]->setFront(tempCardsPixmapArray[j]);
					}
				} else {
					holeCardsArray[(*it_c)->getMyID()][j]->setPixmap(flipside, true);
					holeCardsArray[(*it_c)->getMyID()][j]->setFlipsidePix(flipside);
				}
			} else {

				holeCardsArray[(*it_c)->getMyID()][j]->setPixmap(onePix, false);
			}
		}
	}

	//fix press mouse button during bankrupt with anti-peek-mode
	this->mouseOverFlipCards(false);

	//refresh CardsChanceMonitor Tool
	refreshCardsChance(GAME_STATE_PREFLOP);
}

void gameTableImpl::dealBeRoCards(int myBeRoID)
{

	uncheckMyButtons();
	myButtonsCheckable(false);
	resetMyButtonsCheckStateMemory();
	clearMyButtons();

	horizontalSlider_bet->setDisabled(true);
	spinBox_betValue->setDisabled(true);

	switch(myBeRoID) {

	case 1: {
		dealFlopCards0();
	}
	break;
	case 2: {
		dealTurnCards0();
	}
	break;
	case 3: {
		dealRiverCards0();
	}
	break;
	default: {
		cout << "dealBeRoCards() Error" << endl;
	}
	}
}


void gameTableImpl::dealFlopCards0()
{
	dealFlopCards0Timer->start(preDealCardsSpeed);
}

void gameTableImpl::dealFlopCards1()
{

	boardCardsArray[0]->setPixmap(flipside, true);
	dealFlopCards1Timer->start(dealCardsSpeed);
}

void gameTableImpl::dealFlopCards2()
{

	boardCardsArray[1]->setPixmap(flipside, true);
	dealFlopCards2Timer->start(dealCardsSpeed);
}

void gameTableImpl::dealFlopCards3()
{

	boardCardsArray[2]->setPixmap(flipside, true);
	dealFlopCards3Timer->start(dealCardsSpeed);
}

void gameTableImpl::dealFlopCards4()
{

	int tempBoardCardsArray[5];

	myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getBoard()->getMyCards(tempBoardCardsArray);
	QPixmap card = QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempBoardCardsArray[0], 10)+".png"));

	//Config? mit oder ohne Eye-Candy?
	if(myConfig->readConfigInt("ShowFlipCardsAnimation")) {
		//with Eye-Candy
		boardCardsArray[0]->startFlipCards(guiGameSpeed, card, flipside);
	} else {
		//without Eye-Candy
		boardCardsArray[0]->setFront(card);
		boardCardsArray[0]->setPixmap(card, false);
	}
	dealFlopCards4Timer->start(dealCardsSpeed);
}

void gameTableImpl::dealFlopCards5()
{

	int tempBoardCardsArray[5];
	myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getBoard()->getMyCards(tempBoardCardsArray);
	QPixmap card = QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempBoardCardsArray[1], 10)+".png"));

	//Config? mit oder ohne Eye-Candy?
	if(myConfig->readConfigInt("ShowFlipCardsAnimation")) {
		//with Eye-Candy
		boardCardsArray[1]->startFlipCards(guiGameSpeed, card, flipside);
	} else {
		//without Eye-Candy
		boardCardsArray[1]->setFront(card);
		boardCardsArray[1]->setPixmap(card, false);
	}
	dealFlopCards5Timer->start(dealCardsSpeed);
}

void gameTableImpl::dealFlopCards6()
{

	int tempBoardCardsArray[5];
	myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getBoard()->getMyCards(tempBoardCardsArray);
	QPixmap card = QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempBoardCardsArray[2], 10)+".png"));

	//Config? mit oder ohne Eye-Candy?
	if(myConfig->readConfigInt("ShowFlipCardsAnimation")) {
		//with Eye-Candy
		boardCardsArray[2]->startFlipCards(guiGameSpeed, card, flipside);
	} else {
		//without Eye-Candy
		boardCardsArray[2]->setFront(card);
		boardCardsArray[2]->setPixmap(card, false);
	}

	// stable
	// wenn alle All In
	if(myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getAllInCondition()) {
		dealFlopCards6Timer->start(AllInDealCardsSpeed);
	}
	// sonst normale Variante
	else {
		updateMyButtonsState(0);  //mode 0 == called from dealberocards
		dealFlopCards6Timer->start(postDealCardsSpeed);
	}

	//refresh CardsChanceMonitor Tool
	refreshCardsChance(GAME_STATE_FLOP);
}

void gameTableImpl::dealTurnCards0()
{
	dealTurnCards0Timer->start(preDealCardsSpeed);
}

void gameTableImpl::dealTurnCards1()
{

	boardCardsArray[3]->setPixmap(flipside, true);
	dealTurnCards1Timer->start(dealCardsSpeed);
}

void gameTableImpl::dealTurnCards2()
{

	int tempBoardCardsArray[5];
	myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getBoard()->getMyCards(tempBoardCardsArray);
	QPixmap card = QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempBoardCardsArray[3], 10)+".png"));

	//Config? mit oder ohne Eye-Candy?
	if(myConfig->readConfigInt("ShowFlipCardsAnimation")) {
		//with Eye-Candy
		boardCardsArray[3]->startFlipCards(guiGameSpeed, card, flipside);
	} else {
		//without Eye-Candy
		boardCardsArray[3]->setFront(card);
		boardCardsArray[3]->setPixmap(card, false);
	}

	// stable
	// wenn alle All In
	if(myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getAllInCondition()) {
		dealTurnCards2Timer->start(AllInDealCardsSpeed);
	}
	// sonst normale Variante
	else {
		updateMyButtonsState(0);  //mode 0 == called from dealberocards
		dealTurnCards2Timer->start(postDealCardsSpeed);
	}
	//refresh CardsChanceMonitor Tool
	refreshCardsChance(GAME_STATE_TURN);
}

void gameTableImpl::dealRiverCards0()
{
	dealRiverCards0Timer->start(preDealCardsSpeed);
}

void gameTableImpl::dealRiverCards1()
{

	boardCardsArray[4]->setPixmap(flipside, true);

	// 	QTimer::singleShot(dealCardsSpeed, this, SLOT( dealRiverCards2() ));
	dealRiverCards1Timer->start(dealCardsSpeed);
}

void gameTableImpl::dealRiverCards2()
{

	int tempBoardCardsArray[5];
	myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getBoard()->getMyCards(tempBoardCardsArray);
	QPixmap card = QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempBoardCardsArray[4], 10)+".png"));

	//Config? mit oder ohne Eye-Candy?
	if(myConfig->readConfigInt("ShowFlipCardsAnimation")) {
		//with Eye-Candy
		boardCardsArray[4]->startFlipCards(guiGameSpeed, card, flipside);
	} else {
		//without Eye-Candy
		boardCardsArray[4]->setFront(card);
		boardCardsArray[4]->setPixmap(card, false);
	}

	// stable
	// wenn alle All In
	if(myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getAllInCondition()) {
		dealRiverCards2Timer->start(AllInDealCardsSpeed);
	}
	// sonst normale Variante
	else {
		updateMyButtonsState(0);  //mode 0 == called from dealberocards
		dealRiverCards2Timer->start(postDealCardsSpeed);
	}
	//refresh CardsChanceMonitor Tool
	refreshCardsChance(GAME_STATE_RIVER);
}

void gameTableImpl::provideMyActions(int mode)
{

	QString pushButtonFoldString;
	QString pushButtonBetRaiseString;
	QString lastPushButtonBetRaiseString = pushButton_BetRaise->text();
	QString pushButtonCallCheckString;
	QString pushButtonAllInString;
	QString lastPushButtonCallCheckString = pushButton_CallCheck->text();

	boost::shared_ptr<HandInterface> currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();
	boost::shared_ptr<PlayerInterface> humanPlayer = currentHand->getSeatsList()->front();
	PlayerList activePlayerList = currentHand->getActivePlayerList();

	//really disabled buttons if human player is fold/all-in or server-autofold... and not called from dealberocards
	if(/*pushButton_BetRaise->isCheckable() && */(mode != 0 && (humanPlayer->getMyAction() == PLAYER_ACTION_ALLIN || humanPlayer->getMyAction() == PLAYER_ACTION_FOLD || (humanPlayer->getMySet() == currentHand->getCurrentBeRo()->getHighestSet() && (humanPlayer->getMyAction() != PLAYER_ACTION_NONE)))) || !humanPlayer->isSessionActive() /*server-autofold*/) {

		pushButton_BetRaise->setText("");
		pushButton_CallCheck->setText("");
		pushButton_Fold->setText("");
		pushButton_AllIn->setText("");

		horizontalSlider_bet->setDisabled(true);
		spinBox_betValue->setDisabled(true);

		myButtonsCheckable(false);

		refreshActionButtonFKeyIndicator(1);
	} else {
		horizontalSlider_bet->setEnabled(true);
		spinBox_betValue->setEnabled(true);

		//show available actions on buttons
		if(currentHand->getCurrentRound() == 0) { // preflop

			if (humanPlayer->getMyCash()+humanPlayer->getMySet() > currentHand->getCurrentBeRo()->getHighestSet() && !currentHand->getCurrentBeRo()->getFullBetRule()) {
				pushButtonBetRaiseString = RaiseString+"\n$"+QString("%L1").arg(getMyBetAmount());
			}

			if (humanPlayer->getMySet()== currentHand->getCurrentBeRo()->getHighestSet() &&  humanPlayer->getMyButton() == 3) {
				pushButtonCallCheckString = CheckString;
			} else {
				pushButtonCallCheckString = CallString+"\n$"+QString("%L1").arg(getMyCallAmount());
			}

			pushButtonFoldString = FoldString;
			if(!currentHand->getCurrentBeRo()->getFullBetRule()) {
				pushButtonAllInString = AllInString;
			}
		} else { // flop,turn,river

			if (currentHand->getCurrentBeRo()->getHighestSet() == 0 && pushButton_Fold->isCheckable() ) {
				pushButtonFoldString = CheckString+" /\n"+FoldString;
			} else {
				pushButtonFoldString = FoldString;
			}
			if (currentHand->getCurrentBeRo()->getHighestSet() == 0) {

				pushButtonCallCheckString = CheckString;
				pushButtonBetRaiseString = BetString+"\n$"+QString("%L1").arg(getMyBetAmount());
			}
			if (currentHand->getCurrentBeRo()->getHighestSet() > 0 && currentHand->getCurrentBeRo()->getHighestSet() > humanPlayer->getMySet()) {
				pushButtonCallCheckString = CallString+"\n$"+QString("%L1").arg(getMyCallAmount());
				if (humanPlayer->getMyCash()+humanPlayer->getMySet() > currentHand->getCurrentBeRo()->getHighestSet() && !currentHand->getCurrentBeRo()->getFullBetRule()) {
					pushButtonBetRaiseString = RaiseString+"\n$"+QString("%L1").arg(getMyBetAmount());
				}
			}
			if(!currentHand->getCurrentBeRo()->getFullBetRule()) {
				pushButtonAllInString = AllInString;
			}
		}

		if(mode == 0) {
			if( humanPlayer->getMyAction() != PLAYER_ACTION_FOLD ) {
				pushButtonBetRaiseString = BetString+"\n$"+QString("%L1").arg(getMyBetAmount());
				pushButtonCallCheckString = CheckString;
				if( (activePlayerList->size() > 2 && humanPlayer->getMyButton() == BUTTON_SMALL_BLIND ) || ( activePlayerList->size() <= 2 && humanPlayer->getMyButton() == BUTTON_BIG_BLIND)) {
					pushButtonFoldString = FoldString;
				} else {
					pushButtonFoldString = CheckString+" /\n"+FoldString;
				}

				pushButtonAllInString = AllInString;
			} else {
				pushButtonBetRaiseString = "";
				pushButtonCallCheckString = "";
				pushButtonFoldString = "";
				pushButtonAllInString = "";
				horizontalSlider_bet->setDisabled(true);
				spinBox_betValue->setDisabled(true);

				myButtonsCheckable(false);

			}
		}

		//if text changed on checked button --> do something to prevent unwanted actions
		if(pushButtonCallCheckString != lastPushButtonCallCheckString) {

			if(pushButton_CallCheck->isChecked()) {
				//uncheck a previous checked button to prevent unwanted action
				uncheckMyButtons();
				resetMyButtonsCheckStateMemory();
			}
			//disable button to prevent unwanted clicks (e.g. call allin)
			if(myConfig->readConfigInt("AccidentallyCallBlocker")) {
				pushButton_CallCheck->setEatMyEvents(true);
				enableCallCheckPushButtonTimer->start(1000);
			}

		}

		if(pushButtonBetRaiseString == "") {

			horizontalSlider_bet->setDisabled(true);
			spinBox_betValue->setDisabled(true);
		}

		pushButton_Fold->setText(pushButtonFoldString);
		pushButton_BetRaise->setText(pushButtonBetRaiseString);
		pushButton_CallCheck->setText(pushButtonCallCheckString);
		pushButton_AllIn->setText(pushButtonAllInString);

		refreshActionButtonFKeyIndicator();
		// 		myBetRaise();

		if(pushButton_BetRaise->text().startsWith(RaiseString)) {

			horizontalSlider_bet->setMinimum(currentHand->getCurrentBeRo()->getHighestSet() - humanPlayer->getMySet() + currentHand->getCurrentBeRo()->getMinimumRaise());
			horizontalSlider_bet->setMaximum(humanPlayer->getMyCash());
			horizontalSlider_bet->setSingleStep(10);
			spinBox_betValue->setMinimum(currentHand->getCurrentBeRo()->getHighestSet() - humanPlayer->getMySet() + currentHand->getCurrentBeRo()->getMinimumRaise());
			spinBox_betValue->setMaximum(humanPlayer->getMyCash());
			changeSpinBoxBetValue(horizontalSlider_bet->value());

			myActionIsRaise = 1;
		} else if(pushButton_BetRaise->text().startsWith(BetString)) {

			horizontalSlider_bet->setMinimum(currentHand->getSmallBlind()*2);
			horizontalSlider_bet->setMaximum(humanPlayer->getMyCash());
			horizontalSlider_bet->setSingleStep(10);
			spinBox_betValue->setMinimum(currentHand->getSmallBlind()*2);
			spinBox_betValue->setMaximum(humanPlayer->getMyCash());
			changeSpinBoxBetValue(horizontalSlider_bet->value());

			myActionIsBet = 1;
		} else {}


		//if value changed on bet/raise button --> uncheck to prevent unwanted actions
		int lastBetValue = lastPushButtonBetRaiseString.simplified().remove(QRegExp("[^0-9]")).toInt();

		if((lastBetValue < horizontalSlider_bet->minimum() && pushButton_BetRaise->isChecked())) {

			uncheckMyButtons();
			resetMyButtonsCheckStateMemory();
		}

#ifdef GUI_800x480
		if((myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_INTERNET || myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_NETWORK) && !tabs.lineEdit_ChatInput->hasFocus() && myConfig->readConfigInt("EnableBetInputFocusSwitch")) {
			spinBox_betValue->setFocus();
			spinBox_betValue->selectAll();
		}
#else
		if((myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_INTERNET || myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_NETWORK) && !lineEdit_ChatInput->hasFocus() && myConfig->readConfigInt("EnableBetInputFocusSwitch")) {
			spinBox_betValue->setFocus();
			spinBox_betValue->selectAll();
		}
#endif

		if(myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_LOCAL) {
			spinBox_betValue->setFocus();
			spinBox_betValue->selectAll();
		}

	}
}

void gameTableImpl::meInAction()
{

	myButtonsCheckable(false);

	horizontalSlider_bet->setEnabled(true);
	spinBox_betValue->setEnabled(true);

#ifdef GUI_800x480
	if((myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_INTERNET || myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_NETWORK) && tabs.lineEdit_ChatInput->text() == "" && myConfig->readConfigInt("EnableBetInputFocusSwitch")) {
		spinBox_betValue->setFocus();
		spinBox_betValue->selectAll();
	}
#else
	if((myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_INTERNET || myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_NETWORK) && lineEdit_ChatInput->text() == "" && myConfig->readConfigInt("EnableBetInputFocusSwitch")) {
		spinBox_betValue->setFocus();
		spinBox_betValue->selectAll();
	}
#endif

	//    if(this->isMinimized()){
	//        this->showNormal();
	//        this->activateWindow();
	//        this->raise();
	//    }
	//    else if(!this->isActiveWindow()) {
	//        this->showMinimized();
	//        this->activateWindow();
	//        this->raise();
	//        this->showNormal();
	//        this->activateWindow();
	//        this->raise();
	//
	//    }


	myActionIsRaise = 0;
	myActionIsBet = 0;

	/*	if(myConfig->readConfigInt("ShowStatusbarMessages")) {
			if ( myConfig->readConfigInt("AlternateFKeysUserActionMode") == 0 ) {
				// // 			statusBar()->showMessage(tr("F1 - Fold | F2 - Check/Call | F3 - Bet/Raise | F4 - All-In"), 15000);
			} else {
				// 			statusBar()->showMessage(tr("F1 - All-In | F2 - Bet/Raise | F3 - Check/Call | F4 - Fold"), 15000);
			}
		}*/

	QString lastPushButtonFoldString = pushButton_Fold->text();

	//paint actions on buttons
	provideMyActions();

	//do remembered action
	if( pushButtonBetRaiseIsChecked ) {
		pushButton_BetRaise->click();
		pushButtonBetRaiseIsChecked = false;
	}
	if( pushButtonCallCheckIsChecked )  {
		pushButton_CallCheck->click();
		pushButtonCallCheckIsChecked = false;
	}
	if( pushButtonFoldIsChecked ) {
		if(lastPushButtonFoldString == CheckString+" /\n"+FoldString && pushButton_CallCheck->text() == CheckString) {
			pushButton_CallCheck->click();
		} else {
			pushButton_Fold->click();
		}
		pushButtonFoldIsChecked = false;
	}
	if( pushButtonAllInIsChecked ) {
		pushButton_AllIn->click();
		pushButtonAllInIsChecked = false;
	}

	//automatic mode
	switch (playingMode) {
	case 0: // Manual mode
		break;
	case 1: // Auto check / call all
		myCallCheck();
		break;
	case 2: // Auto check / fold all
		if (pushButton_CallCheck->text() == CheckString) {
			myCheck();
		} else {
			myFold();
		}
		break;
	}
}

void gameTableImpl::startTimeoutAnimation(int playerId, int timeoutSec)
{
	assert(playerId >= 0 && playerId < myStartWindow->getSession()->getCurrentGame()->getStartQuantityPlayers());

	//beep for player 0
	if(playerId) {
		timeoutLabelArray[playerId]->startTimeOutAnimation(timeoutSec, false);
	} else {
		timeoutLabelArray[playerId]->startTimeOutAnimation(timeoutSec, true);
	}
}

void gameTableImpl::stopTimeoutAnimation(int playerId)
{
	assert(playerId >= 0 && playerId < myStartWindow->getSession()->getCurrentGame()->getStartQuantityPlayers());
	timeoutLabelArray[playerId]->stopTimeOutAnimation();
}

void gameTableImpl::disableMyButtons()
{

	boost::shared_ptr<PlayerInterface> humanPlayer = myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getSeatsList()->front();

	clearMyButtons();

	//clear userWidgets
	horizontalSlider_bet->setDisabled(true);
	spinBox_betValue->setDisabled(true);
	horizontalSlider_bet->setMinimum(0);
	horizontalSlider_bet->setMaximum(humanPlayer->getMyCash());
	spinBox_betValue->setMinimum(0);
	spinBox_betValue->setMaximum(humanPlayer->getMyCash());
	spinBox_betValue->clear();
	horizontalSlider_bet->setValue(0);

#ifdef _WIN32
	QString humanPlayerButtonFontSize = "13";
#else
	QString humanPlayerButtonFontSize = "12";
#endif
}

void gameTableImpl::myCallCheck()
{

	if(pushButton_CallCheck->text().startsWith(CallString)) {
		myCall();
	}
	if(pushButton_CallCheck->text() == CheckString) {
		myCheck();
	}
}

void gameTableImpl::myFold()
{

	if(pushButton_Fold->text() == FoldString) {

		boost::shared_ptr<HandInterface> currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();
		boost::shared_ptr<PlayerInterface> humanPlayer = currentHand->getSeatsList()->front();
		humanPlayer->setMyAction(PLAYER_ACTION_FOLD,true);
		humanPlayer->setMyTurn(0);

		//set that i was the last active player. need this for unhighlighting groupbox
		currentHand->setPreviousPlayerID(0);

		// 		statusBar()->clearMessage();

		//Spiel läuft weiter
		myActionDone();
	}
}

void gameTableImpl::myCheck()
{

	boost::shared_ptr<HandInterface> currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();
	boost::shared_ptr<PlayerInterface> humanPlayer = currentHand->getSeatsList()->front();
	humanPlayer->setMyTurn(0);
	humanPlayer->setMyAction(PLAYER_ACTION_CHECK,true);

	//set that i was the last active player. need this for unhighlighting groupbox
	currentHand->setPreviousPlayerID(0);

	// 	statusBar()->clearMessage();

	//Spiel läuft weiter
	myActionDone();
}

int gameTableImpl::getMyCallAmount()
{

	boost::shared_ptr<HandInterface> currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();
	boost::shared_ptr<PlayerInterface> humanPlayer = currentHand->getSeatsList()->front();
	int tempHighestSet = currentHand->getCurrentBeRo()->getHighestSet();

	if (humanPlayer->getMyCash() + humanPlayer->getMySet() <= tempHighestSet) {

		return humanPlayer->getMyCash();
	} else {
		return tempHighestSet - humanPlayer->getMySet();
	}
}

int gameTableImpl::getBetRaisePushButtonValue()
{

	int betValue = pushButton_BetRaise->text().simplified().remove(QRegExp("[^0-9]")).toInt();
	return betValue;
}

int gameTableImpl::getMyBetAmount()
{

	boost::shared_ptr<HandInterface> currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();
	boost::shared_ptr<PlayerInterface> humanPlayer = currentHand->getSeatsList()->front();

	int betValue = getBetRaisePushButtonValue();
	int minimum;

	minimum = currentHand->getCurrentBeRo()->getHighestSet() - humanPlayer->getMySet() + currentHand->getCurrentBeRo()->getMinimumRaise();

	if(betValue < minimum) {
		return min(minimum,humanPlayer->getMyCash());
	} else {
		return betValue;
	}
}

void gameTableImpl::myCall()
{

	boost::shared_ptr<HandInterface> currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();
	boost::shared_ptr<PlayerInterface> humanPlayer = currentHand->getSeatsList()->front();

	int tempHighestSet = currentHand->getCurrentBeRo()->getHighestSet();

	if (humanPlayer->getMyCash() + humanPlayer->getMySet() <= tempHighestSet) {

		humanPlayer->setMySet(humanPlayer->getMyCash());
		humanPlayer->setMyCash(0);
		humanPlayer->setMyAction(PLAYER_ACTION_ALLIN,true);
	} else {
		humanPlayer->setMySet(tempHighestSet - humanPlayer->getMySet());
		humanPlayer->setMyAction(PLAYER_ACTION_CALL,true);
	}
	humanPlayer->setMyTurn(0);

	currentHand->getBoard()->collectSets();
	refreshPot();

	//set that i was the last active player. need this for unhighlighting groupbox
	currentHand->setPreviousPlayerID(0);

	// 	statusBar()->clearMessage();

	//Spiel läuft weiter
	myActionDone();
}

void gameTableImpl::mySet()
{

	if(pushButton_BetRaise->text() != "") {

		boost::shared_ptr<HandInterface> currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();
		boost::shared_ptr<PlayerInterface> humanPlayer = currentHand->getSeatsList()->front();

		int tempCash = humanPlayer->getMyCash();

		// 		cout << "Set-Value " << getBetRaisePushButtonValue() << endl;
		humanPlayer->setMySet(getBetRaisePushButtonValue());

		if (getBetRaisePushButtonValue() >= tempCash ) {

			humanPlayer->setMySet(humanPlayer->getMyCash());
			humanPlayer->setMyCash(0);
			humanPlayer->setMyAction(PLAYER_ACTION_ALLIN,true);

			// full bet rule
			if(currentHand->getCurrentBeRo()->getHighestSet() + currentHand->getCurrentBeRo()->getMinimumRaise() > humanPlayer->getMySet()) {
				currentHand->getCurrentBeRo()->setFullBetRule(true);
			}
		}

		if(myActionIsRaise) {
			//do not if allIn
			if(humanPlayer->getMyAction() != 6) {
				humanPlayer->setMyAction(PLAYER_ACTION_RAISE,true);
			}
			myActionIsRaise = 0;

			currentHand->getCurrentBeRo()->setMinimumRaise(humanPlayer->getMySet() - currentHand->getCurrentBeRo()->getHighestSet());
		}

		if(myActionIsBet) {
			//do not if allIn
			if(humanPlayer->getMyAction() != 6) {
				humanPlayer->setMyAction(PLAYER_ACTION_BET,true);
			}
			myActionIsBet = 0;

			currentHand->getCurrentBeRo()->setMinimumRaise(humanPlayer->getMySet());
		}

		currentHand->getCurrentBeRo()->setHighestSet(humanPlayer->getMySet());

		humanPlayer->setMyTurn(0);

		currentHand->getBoard()->collectSets();
		refreshPot();

		// 		statusBar()->clearMessage();

		//set that i was the last active player. need this for unhighlighting groupbox
		currentHand->setPreviousPlayerID(0);

		// lastPlayerAction für Karten umblättern reihenfolge setzrn
		currentHand->setLastActionPlayerID(humanPlayer->getMyUniqueID());

		//Spiel läuft weiter
		myActionDone();
	}
}

void gameTableImpl::myAllIn()
{

	if(pushButton_AllIn->text() == AllInString) {

		boost::shared_ptr<HandInterface> currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();
		boost::shared_ptr<PlayerInterface> humanPlayer = currentHand->getSeatsList()->front();

		humanPlayer->setMySet(humanPlayer->getMyCash());
		humanPlayer->setMyCash(0);
		humanPlayer->setMyAction(PLAYER_ACTION_ALLIN,true);

		// full bet rule
		if(currentHand->getCurrentBeRo()->getHighestSet() + currentHand->getCurrentBeRo()->getMinimumRaise() > humanPlayer->getMySet()) {
			currentHand->getCurrentBeRo()->setFullBetRule(true);
		}

		if(humanPlayer->getMySet() > currentHand->getCurrentBeRo()->getHighestSet()) {
			currentHand->getCurrentBeRo()->setMinimumRaise(humanPlayer->getMySet() - currentHand->getCurrentBeRo()->getHighestSet());

			currentHand->getCurrentBeRo()->setHighestSet(humanPlayer->getMySet());

			// lastPlayerAction für Karten umblättern reihenfolge setzrn
			currentHand->setLastActionPlayerID(humanPlayer->getMyUniqueID());

		}

		humanPlayer->setMyTurn(0);

		currentHand->getBoard()->collectSets();
		refreshPot();

		//set that i was the last active player. need this for unhighlighting groupbox
		currentHand->setPreviousPlayerID(0);

		//Spiel läuft weiter
		myActionDone();
	}
}


void gameTableImpl::pushButtonBetRaiseClicked(bool checked)
{

	if (pushButton_BetRaise->isCheckable()) {
		if(checked) {
			pushButton_CallCheck->setChecked(false);
			pushButton_Fold->setChecked(false);
			pushButton_AllIn->setChecked(false);

			pushButtonCallCheckIsChecked = false;
			pushButtonFoldIsChecked = false;
			pushButtonAllInIsChecked = false;

			pushButtonBetRaiseIsChecked = true;

#ifdef GUI_800x480
			if(!tabs.radioButton_manualAction->isChecked())
				tabs.radioButton_manualAction->click();
#else
			if(!radioButton_manualAction->isChecked())
				radioButton_manualAction->click();
#endif
			// 			myLastPreActionBetValue = spinBox_betValue->value();

		} else {
			pushButtonBetRaiseIsChecked = false;
			myLastPreActionBetValue = 0;
		}
	} else {
		mySet();
	}
}

void gameTableImpl::pushButtonCallCheckClicked(bool checked)
{

	if (pushButton_CallCheck->isCheckable()) {
		if(checked) {
			pushButton_Fold->setChecked(false);
			pushButton_BetRaise->setChecked(false);
			pushButton_AllIn->setChecked(false);

			pushButtonAllInIsChecked = false;
			pushButtonFoldIsChecked = false;
			pushButtonBetRaiseIsChecked = false;

			pushButtonCallCheckIsChecked = true;

#ifdef GUI_800x480
			if(!tabs.radioButton_manualAction->isChecked())
				tabs.radioButton_manualAction->click();
#else
			if(!radioButton_manualAction->isChecked())
				radioButton_manualAction->click();
#endif
		} else {
			pushButtonCallCheckIsChecked = false;
		}
	} else {
		myCallCheck();
	}
}

void gameTableImpl::pushButtonFoldClicked(bool checked)
{

	if (pushButton_Fold->isCheckable()) {
		if(checked) {
			pushButton_CallCheck->setChecked(false);
			pushButton_BetRaise->setChecked(false);
			pushButton_AllIn->setChecked(false);

			pushButtonAllInIsChecked = false;
			pushButtonCallCheckIsChecked = false;
			pushButtonBetRaiseIsChecked = false;

			pushButtonFoldIsChecked = true;

#ifdef GUI_800x480
			if(!tabs.radioButton_manualAction->isChecked())
				tabs.radioButton_manualAction->click();
#else
			if(!radioButton_manualAction->isChecked())
				radioButton_manualAction->click();
#endif
		} else {
			pushButtonFoldIsChecked = false;
		}
	} else {
		myFold();
	}
}

void gameTableImpl::pushButtonAllInClicked(bool checked)
{

	if (pushButton_AllIn->isCheckable()) {
		if(checked) {
			pushButton_CallCheck->setChecked(false);
			pushButton_BetRaise->setChecked(false);
			pushButton_Fold->setChecked(false);

			pushButtonFoldIsChecked = false;
			pushButtonCallCheckIsChecked = false;
			pushButtonBetRaiseIsChecked = false;

			pushButtonAllInIsChecked = true;

#ifdef GUI_800x480
			if(!tabs.radioButton_manualAction->isChecked())
				tabs.radioButton_manualAction->click();
#else
			if(!radioButton_manualAction->isChecked())
				radioButton_manualAction->click();
#endif
		} else {
			pushButtonAllInIsChecked = false;
		}
	} else {
		myAllIn();
	}
}

void gameTableImpl::myActionDone()
{

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

void gameTableImpl::nextPlayerAnimation()
{

	boost::shared_ptr<HandInterface> currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();

	//refresh Change Player
	refreshSet();

	PlayerListConstIterator it_c;
	PlayerList seatsList = currentHand->getSeatsList();
	for (it_c=seatsList->begin(); it_c!=seatsList->end(); ++it_c) {
		if((*it_c)->getMyID() == currentHand->getPreviousPlayerID()) break;
	}

	if(currentHand->getPreviousPlayerID() != -1) {
		refreshAction(currentHand->getPreviousPlayerID(), (*it_c)->getMyAction());
	}
	refreshCash();

	//refresh actions for human player
	updateMyButtonsState();

	nextPlayerAnimationTimer->start(nextPlayerSpeed1);
}

void gameTableImpl::beRoAnimation2(int myBeRoID)
{

	switch(myBeRoID) {

	case 0: {
		preflopAnimation2();
	}
	break;
	case 1: {
		flopAnimation2();
	}
	break;
	case 2: {
		turnAnimation2();
	}
	break;
	case 3: {
		riverAnimation2();
	}
	break;
	default: {
		cout << "beRoAnimation2() Error" << endl;
	}
	}
}


void gameTableImpl::preflopAnimation1()
{
	preflopAnimation1Timer->start(nextPlayerSpeed2);
}
void gameTableImpl::preflopAnimation1Action()
{
	myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getCurrentBeRo()->run();
}

void gameTableImpl::preflopAnimation2()
{
	preflopAnimation2Timer->start(preflopNextPlayerSpeed);
}
void gameTableImpl::preflopAnimation2Action()
{
	myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getCurrentBeRo()->nextPlayer();
}


void gameTableImpl::flopAnimation1()
{
	flopAnimation1Timer->start(nextPlayerSpeed2);
}
void gameTableImpl::flopAnimation1Action()
{
	myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getCurrentBeRo()->run();
}

void gameTableImpl::flopAnimation2()
{
	flopAnimation2Timer->start(nextPlayerSpeed3);
}
void gameTableImpl::flopAnimation2Action()
{
	myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getCurrentBeRo()->nextPlayer();
}

void gameTableImpl::turnAnimation1()
{
	turnAnimation1Timer->start(nextPlayerSpeed2);
}
void gameTableImpl::turnAnimation1Action()
{
	myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getCurrentBeRo()->run();
}

void gameTableImpl::turnAnimation2()
{
	turnAnimation2Timer->start(nextPlayerSpeed3);
}
void gameTableImpl::turnAnimation2Action()
{
	myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getCurrentBeRo()->nextPlayer();
}

void gameTableImpl::riverAnimation1()
{
	riverAnimation1Timer->start(nextPlayerSpeed2);
}
void gameTableImpl::riverAnimation1Action()
{
	myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getCurrentBeRo()->run();
}

void gameTableImpl::riverAnimation2()
{
	riverAnimation2Timer->start(nextPlayerSpeed3);
}
void gameTableImpl::riverAnimation2Action()
{
	myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getCurrentBeRo()->nextPlayer();
}

void gameTableImpl::postRiverAnimation1()
{
	postRiverAnimation1Timer->start(nextPlayerSpeed2);
}
void gameTableImpl::postRiverAnimation1Action()
{
	myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getCurrentBeRo()->postRiverRun();
}

void gameTableImpl::postRiverRunAnimation1()
{
	postRiverRunAnimation1Timer->start(postRiverRunAnimationSpeed);
}

void gameTableImpl::postRiverRunAnimation2()
{

	uncheckMyButtons();
	myButtonsCheckable(false);
	clearMyButtons();
	resetMyButtonsCheckStateMemory();

	horizontalSlider_bet->setDisabled(true);
	spinBox_betValue->setDisabled(true);

	boost::shared_ptr<Game> currentGame = myStartWindow->getSession()->getCurrentGame();

	bool internetOrNetworkGame = (myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_INTERNET || myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_NETWORK);

	int nonfoldPlayersCounter = 0;
	PlayerListConstIterator it_c;
	PlayerList activePlayerList = currentGame->getActivePlayerList();
	for (it_c=activePlayerList->begin(); it_c!=activePlayerList->end(); ++it_c) {
		if ((*it_c)->getMyAction() != PLAYER_ACTION_FOLD)
			nonfoldPlayersCounter++;
	}

	if(nonfoldPlayersCounter!=1) {

		if(!flipHolecardsAllInAlreadyDone) {

			for (it_c=activePlayerList->begin(); it_c!=activePlayerList->end(); ++it_c) {
				if((*it_c)->getMyAction() != PLAYER_ACTION_FOLD && (*it_c)->checkIfINeedToShowCards()) {

					showHoleCards((*it_c)->getMyUniqueID());
				}

				//if human player dont need to show cards he gets the button "show cards" in internet or network game
				if( internetOrNetworkGame && (*it_c)->getMyID() == 0 && (*it_c)->getMyAction() != PLAYER_ACTION_FOLD && !(*it_c)->checkIfINeedToShowCards()) {

					showShowMyCardsButton();
				}
			}
			//Wenn einmal umgedreht dann fertig!!
			flipHolecardsAllInAlreadyDone = true;
		} else {
			for (it_c=activePlayerList->begin(); it_c!=activePlayerList->end(); ++it_c) {
				if((*it_c)->getMyAction() != PLAYER_ACTION_FOLD) {
					//set Player value (logging) for all in already shown cards
					(*it_c)->setMyCardsFlip(1,3);
				}
			}
		}
		postRiverRunAnimation2Timer->start(postRiverRunAnimationSpeed);
	} else {

		//display show! button if human player is active and the latest non foldedone
		boost::shared_ptr<PlayerInterface> humanPlayer = myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getSeatsList()->front();
		if( internetOrNetworkGame && humanPlayer->getMyActiveStatus() && humanPlayer->getMyAction() != PLAYER_ACTION_FOLD) {

			showShowMyCardsButton();
		}

		postRiverRunAnimation3();
	}

}

void gameTableImpl::postRiverRunAnimation3()
{

	boost::shared_ptr<HandInterface> currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();

	int nonfoldPlayerCounter = 0;
	PlayerListConstIterator it_c;

	PlayerList activePlayerList = currentHand->getActivePlayerList();
	for(it_c=activePlayerList->begin(); it_c!=activePlayerList->end(); ++it_c) {
		if((*it_c)->getMyAction() != PLAYER_ACTION_FOLD) {
			nonfoldPlayerCounter++;
		}
	}

	list<unsigned> winners = currentHand->getBoard()->getWinners();

	for(it_c=activePlayerList->begin(); it_c!=activePlayerList->end(); ++it_c) {
		if((*it_c)->getMyAction() != PLAYER_ACTION_FOLD && (*it_c)->getMyCardsValueInt() == currentHand->getCurrentBeRo()->getHighestCardsValue() ) {

			//Show "Winner" label
			actionLabelArray[(*it_c)->getMyID()]->setPixmap(QPixmap::fromImage(QImage(myGameTableStyle->getActionPic(7))));

			//show winnercards if more than one player is active TODO
			if ( nonfoldPlayerCounter != 1 && myConfig->readConfigInt("ShowFadeOutCardsAnimation")) {

				int j;
				int bestHandPos[5];
				(*it_c)->getMyBestHandPosition(bestHandPos);

				bool index0 = true;
				for(j=0; j<5; j++) {
					if (bestHandPos[j] == 0 ) {
						index0 = false;
					}
				}
				if (index0) {
					holeCardsArray[(*it_c)->getMyID()][0]->startFadeOut(guiGameSpeed); /*cout << "Fade Out index0" << endl;*/
				}
				//index 1 testen
				bool index1 = true;
				for(j=0; j<5; j++) {
					if (bestHandPos[j] == 1 ) {
						index1 = false;
					}
				}
				if (index1) {
					holeCardsArray[(*it_c)->getMyID()][1]->startFadeOut(guiGameSpeed); /*cout << "Fade Out index1" << endl;*/
				}
				//index 2 testen
				bool index2 = true;
				for(j=0; j<5; j++) {
					if (bestHandPos[j] == 2 ) {
						index2 = false;
					}
				}
				if (index2) {
					boardCardsArray[0]->startFadeOut(guiGameSpeed); /*cout << "Fade Out index2" << endl;*/
				}
				//index 3 testen
				bool index3 = true;
				for(j=0; j<5; j++) {
					if (bestHandPos[j] == 3 ) {
						index3 = false;
					}
				}
				if (index3) {
					boardCardsArray[1]->startFadeOut(guiGameSpeed); /*cout << "Fade Out index3" << endl;*/
				}
				//index 4 testen
				bool index4 = true;
				for(j=0; j<5; j++) {
					if (bestHandPos[j] == 4 ) {
						index4 = false;
					}
				}
				if (index4) {
					boardCardsArray[2]->startFadeOut(guiGameSpeed); /*cout << "Fade Out index4" << endl;*/
				}
				//index 5 testen
				bool index5 = true;
				for(j=0; j<5; j++) {
					if (bestHandPos[j] == 5 ) {
						index5 = false;
					}
				}
				if (index5) {
					boardCardsArray[3]->startFadeOut(guiGameSpeed); /*cout << "Fade Out index5" << endl;*/
				}
				//index 6 testen
				bool index6 = true;
				for(j=0; j<5; j++) {
					if (bestHandPos[j] == 6 ) {
						index6 = false;
					}
				}
				if (index6) {
					boardCardsArray[4]->startFadeOut(guiGameSpeed); /*cout << "Fade Out index6" << endl;*/
				}
			}
			//Pot-Verteilung Loggen
			//Wenn River dann auch das Blatt loggen!
			// 			if (textLabel_handLabel->text() == "River") {

			//set Player value (logging)
			myGuiLog->logPlayerWinsMsg(QString::fromUtf8((*it_c)->getMyName().c_str()),(*it_c)->getLastMoneyWon(),true);

			// 			}
			// 			else {
			// 				myLog->logPlayerWinsMsg(i, pot);
			// 			}
		} else {

			if( activePlayerList->size() != 1 && (*it_c)->getMyAction() != PLAYER_ACTION_FOLD && myConfig->readConfigInt("ShowFadeOutCardsAnimation")
			  ) {

				//aufgedeckte Gegner auch ausblenden
				holeCardsArray[(*it_c)->getMyID()][0]->startFadeOut(guiGameSpeed);
				holeCardsArray[(*it_c)->getMyID()][1]->startFadeOut(guiGameSpeed);
			}
		}
	}

	// log side pot winners -> TODO
	list<unsigned>::iterator it_int;
	for(it_c=activePlayerList->begin(); it_c!=activePlayerList->end(); ++it_c) {
		if((*it_c)->getMyAction() != PLAYER_ACTION_FOLD && (*it_c)->getMyCardsValueInt() != currentHand->getCurrentBeRo()->getHighestCardsValue() ) {

			for(it_int = winners.begin(); it_int != winners.end(); ++it_int) {
				if((*it_int) == (*it_c)->getMyUniqueID()) {
					myGuiLog->logPlayerWinsMsg(QString::fromUtf8((*it_c)->getMyName().c_str()), (*it_c)->getLastMoneyWon(), false);
				}
			}

		}
	}

	for(it_c=activePlayerList->begin(); it_c!=activePlayerList->end(); ++it_c) {
		if((*it_c)->getMyCash() == 0) {
			currentHand->getGuiInterface()->logPlayerSitsOut((*it_c)->getMyName());
		}
	}

//	textBrowser_Log->append("");

	postRiverRunAnimation3Timer->start(postRiverRunAnimationSpeed/2);
}

void gameTableImpl::postRiverRunAnimation4()
{

	distributePotAnimCounter=0;
	potDistributeTimer->start(winnerBlinkSpeed);
}

void gameTableImpl::postRiverRunAnimation5()
{

	boost::shared_ptr<HandInterface> currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();
	PlayerList activePlayerList = currentHand->getActivePlayerList();
	PlayerListConstIterator it_c;

	if (distributePotAnimCounter<10) {

		if (distributePotAnimCounter==0 || distributePotAnimCounter==2 || distributePotAnimCounter==4 || distributePotAnimCounter==6 || distributePotAnimCounter==8) {

#ifndef GUI_800x480
			label_Pot->setText("");
#endif

			for(it_c=activePlayerList->begin(); it_c!=activePlayerList->end(); ++it_c) {

				if((*it_c)->getMyAction() != PLAYER_ACTION_FOLD && (*it_c)->getMyCardsValueInt() == currentHand->getCurrentBeRo()->getHighestCardsValue() ) {

					playerNameLabelArray[(*it_c)->getMyID()]->hide();
				}
			}
		} else {
#ifndef GUI_800x480
			label_Pot->setText(PotString);
#endif

			for(it_c=activePlayerList->begin(); it_c!=activePlayerList->end(); ++it_c) {

				if((*it_c)->getMyAction() != PLAYER_ACTION_FOLD && (*it_c)->getMyCardsValueInt() == currentHand->getCurrentBeRo()->getHighestCardsValue() ) {

					playerNameLabelArray[(*it_c)->getMyID()]->show();
				}
			}
		}

		distributePotAnimCounter++;
	} else {
		potDistributeTimer->stop();
		postRiverRunAnimation5Timer->start(gameSpeed);
	}
}

void gameTableImpl::postRiverRunAnimation6()
{

	//GUI HACK show every nick label
	int i;
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
		playerNameLabelArray[i]->show();
	}

	boost::shared_ptr<HandInterface> currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();

	refreshCash();
	refreshPot();

	// TODO HACK
	// Check for network client, do not start new hand if client is running.
	if (myStartWindow->getSession()->isNetworkClientRunning())
		return;

	// wenn nur noch ein Spieler aktive "neues Spiel"-Dialog anzeigen
	int playersPositiveCashCounter = 0;

	PlayerListConstIterator it_c;
	PlayerList activePlayerList = currentHand->getActivePlayerList();
	for (it_c=activePlayerList->begin(); it_c!=activePlayerList->end(); ++it_c) {

		if ((*it_c)->getMyCash() > 0) playersPositiveCashCounter++;
	}

	if (playersPositiveCashCounter==1) {

		for (it_c=activePlayerList->begin(); it_c!=activePlayerList->end(); ++it_c) {

			if ((*it_c)->getMyCash() > 0) {
				currentHand->getGuiInterface()->logPlayerWinGame((*it_c)->getMyName(),  myStartWindow->getSession()->getCurrentGame()->getMyGameID());
			}
		}

		if( !DEBUG_MODE ) {

			if(myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_LOCAL) {
				currentGameOver = true;
#ifdef GUI_800x480
				tabs.pushButton_break->setDisabled(false);
#else
				pushButton_break->setDisabled(false);
#endif
				QFontMetrics tempMetrics = this->fontMetrics();
				int width = tempMetrics.width(tr("Start"));
#ifdef GUI_800x480
				tabs.pushButton_break->setMinimumSize(width+10,20);
				tabs.pushButton_break->setText(tr("Start"));
#else
				pushButton_break->setMinimumSize(width+10,20);
				pushButton_break->setText(tr("Start"));
#endif
				blinkingStartButtonAnimationTimer->start(500);
			}
		} else {
			myStartWindow->callNewGameDialog();
			//Bei Cancel nichts machen!!!
		}
		return;
	}

	postRiverRunAnimation6Timer->start(newRoundSpeed);
}

void gameTableImpl::showHoleCards(unsigned playerId, bool allIn)
{
	boost::shared_ptr<HandInterface> currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();
	//TempArrays
	QPixmap tempCardsPixmapArray[2];
	int tempCardsIntArray[2];
	int showFlipcardAnimation = myConfig->readConfigInt("ShowFlipCardsAnimation");
	int j;
	PlayerListConstIterator it_c;
	PlayerList activePlayerList = currentHand->getActivePlayerList();
	for (it_c=activePlayerList->begin(); it_c!=activePlayerList->end(); ++it_c) {

		if((*it_c)->getMyUniqueID() == playerId) {

			(*it_c)->getMyCards(tempCardsIntArray);
			for(j=0; j<2; j++) {

				if(showFlipcardAnimation) { // with Eye-Candy
					holeCardsArray[(*it_c)->getMyID()][j]->startFlipCards(guiGameSpeed, QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempCardsIntArray[j], 10)+".png")), flipside);
				} else { //without Eye-Candy
					tempCardsPixmapArray[j] = QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempCardsIntArray[j], 10)+".png"));
					holeCardsArray[(*it_c)->getMyID()][j]->setPixmap(tempCardsPixmapArray[j], false);
				}
			}
			//set Player value (logging)
			if(currentHand->getCurrentRound() < GAME_STATE_RIVER || allIn) {
				(*it_c)->setMyCardsFlip(1,2); //for bero before postriver or allin just log the hole cards
			} else {
				(*it_c)->setMyCardsFlip(1,1); //for postriver log the value
			}
		}
	}
}

void gameTableImpl::flipHolecardsAllIn()
{

	boost::shared_ptr<Game> currentGame = myStartWindow->getSession()->getCurrentGame();

	if(!flipHolecardsAllInAlreadyDone && currentGame->getCurrentHand()->getCurrentRound() < GAME_STATE_RIVER) {
		//Aktive Spieler zählen --> wenn nur noch einer nicht-folded dann keine Karten umdrehen
		int nonfoldPlayersCounter = 0;
		PlayerListConstIterator it_c;
		PlayerList activePlayerList = currentGame->getActivePlayerList();
		for (it_c=activePlayerList->begin(); it_c!=activePlayerList->end(); ++it_c) {
			if ((*it_c)->getMyAction() != PLAYER_ACTION_FOLD) nonfoldPlayersCounter++;
		}

		if(nonfoldPlayersCounter!=1) {
			for (it_c=activePlayerList->begin(); it_c!=activePlayerList->end(); ++it_c) {
				if((*it_c)->getMyAction() != PLAYER_ACTION_FOLD) {

					showHoleCards((*it_c)->getMyUniqueID());
				}
			}
		}

		//Wenn einmal umgedreht dann fertig!!
		flipHolecardsAllInAlreadyDone = true;
	}
}


void gameTableImpl::startNewHand()
{

	if( !breakAfterCurrentHand) {
		myStartWindow->getSession()->getCurrentGame()->initHand();
		myStartWindow->getSession()->getCurrentGame()->startHand();
	} else {

		if(myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_LOCAL) {
#ifdef GUI_800x480
			tabs.pushButton_break->setDisabled(false);
#else
			pushButton_break->setDisabled(false);
#endif

			QFontMetrics tempMetrics = this->fontMetrics();
			int width = tempMetrics.width(tr("Start"));
#ifdef GUI_800x480
			tabs.pushButton_break->setMinimumSize(width+10,20);
			tabs.pushButton_break->setText(tr("Start"));
#else
			pushButton_break->setMinimumSize(width+10,20);
			pushButton_break->setText(tr("Start"));
#endif

			breakAfterCurrentHand=false;

			blinkingStartButtonAnimationTimer->start(500);
		}
	}
}

void gameTableImpl::handSwitchRounds()
{
	myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->switchRounds();
}

void gameTableImpl::nextRoundCleanGui()
{

	int i,j;

	// GUI bereinigen - Bilder löschen, Animationen unterbrechen
	QPixmap onePix = QPixmap::fromImage(QImage(myAppDataPath +"gfx/gui/misc/1px.png"));
	for (i=0; i<5; i++ ) {
		boardCardsArray[i]->setPixmap(onePix, false);
		boardCardsArray[i]->setFadeOutAction(false);
		boardCardsArray[i]->stopFlipCardsAnimation();

	}
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++ ) {
		timeoutLabelArray[i]->stopTimeOutAnimation();
		for ( j=0; j<=1; j++ ) {
			holeCardsArray[i][j]->setFadeOutAction(false);
			holeCardsArray[i][j]->stopFlipCardsAnimation();
		}
	}

	// for startNewGame during human player is active
	if(myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getSeatsList()->front()->getMyActiveStatus() == 1) {
		disableMyButtons();
	}

	textLabel_handLabel->setText("");

	refreshAll();

	flipHolecardsAllInAlreadyDone = false;

	//Wenn Pause zwischen den Hands in der Konfiguration steht den Stop Button drücken!
	if (myConfig->readConfigInt("PauseBetweenHands") /*&& blinkingStartButtonAnimationTimer->isActive() == false*/ && myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_LOCAL) {
#ifdef GUI_800x480
		tabs.pushButton_break->click();
#else
		pushButton_break->click();
#endif
	} else {
		//FIX STRG+N Bug
#ifdef GUI_800x480
		tabs.pushButton_break->setEnabled(true);
#else
		pushButton_break->setEnabled(true);
#endif
		breakAfterCurrentHand=false;
	}

	//Clean breakbutton
	if(myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_LOCAL) {
		blinkingStartButtonAnimationTimer->stop();
#ifdef GUI_800x480
		myGameTableStyle->setBreakButtonStyle(tabs.pushButton_break,0);
#else
		myGameTableStyle->setBreakButtonStyle(pushButton_break,0);
#endif
		blinkingStartButtonAnimationTimer->stop();
		QFontMetrics tempMetrics = this->fontMetrics();
		int width = tempMetrics.width(tr("Stop"));
#ifdef GUI_800x480
		tabs.pushButton_break->setMinimumSize(width+10,20);
		tabs.pushButton_break->setText(tr("Stop"));
#else
		pushButton_break->setMinimumSize(width+10,20);
		pushButton_break->setText(tr("Stop"));
#endif
	}
	//Clear Statusbarmessage
	// 	statusBar()->clearMessage();

	//fix press mouse button during bankrupt with anti-peek-mode
	this->mouseOverFlipCards(false);

	horizontalSlider_bet->setDisabled(true);
	spinBox_betValue->setDisabled(true);

	uncheckMyButtons();
	myButtonsCheckable(false);
	resetMyButtonsCheckStateMemory();
	clearMyButtons();
	pushButton_showMyCards->hide();
}

void gameTableImpl::stopTimer()
{

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

void gameTableImpl::setSpeeds()
{

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

void gameTableImpl::breakButtonClicked()
{

#ifdef GUI_800x480
	if (tabs.pushButton_break->text() == tr("Stop")) {
		tabs.pushButton_break->setDisabled(true);
		breakAfterCurrentHand=true;
	} else if (tabs.pushButton_break->text() == tr("Lobby")) {
		tabsButtonClose();
		leaveCurrentNetworkGame();
	} else if (tabs.pushButton_break->text() == tr("Start")) {

		blinkingStartButtonAnimationTimer->stop();
		//Set default Color
		myGameTableStyle->setBreakButtonStyle(tabs.pushButton_break,0);
		QFontMetrics tempMetrics = this->fontMetrics();
		int width = tempMetrics.width(tr("Stop"));
		tabs.pushButton_break->setMinimumSize(width+10,20);

		tabs.pushButton_break->setText(tr("Stop"));
#else
	if (pushButton_break->text() == tr("Stop")) {
		pushButton_break->setDisabled(true);
		breakAfterCurrentHand=true;
	} else if (pushButton_break->text() == tr("Lobby")) {
		leaveCurrentNetworkGame();
	} else if (pushButton_break->text() == tr("Start")) {

		blinkingStartButtonAnimationTimer->stop();
		//Set default Color
		myGameTableStyle->setBreakButtonStyle(pushButton_break,0);
		QFontMetrics tempMetrics = this->fontMetrics();
		int width = tempMetrics.width(tr("Stop"));
		pushButton_break->setMinimumSize(width+10,20);

		pushButton_break->setText(tr("Stop"));
#endif

		if(currentGameOver) {
			//let the SoundEventHandler know that there is a new game
			mySoundEventHandler->newGameStarts();

			currentGameOver = false;
			myStartWindow->callNewGameDialog();
			//Bei Cancel nichts machen!!!
		} else {
			startNewHand();
		}
	}
}

void gameTableImpl::keyPressEvent ( QKeyEvent * event )
{

	// 	cout << event->key() << endl;

	//bool ctrlPressed = false;

	if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return ) { /*ENTER*/
		if(spinBox_betValue->hasFocus()) {
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
	if (event->key() == Qt::Key_F5) {
		pushButton_showMyCards->click();
	}

#ifndef GUI_800x480
	if (event->key() == Qt::Key_F6) {
		radioButton_manualAction->click();
	}
	if (event->key() == Qt::Key_F7) {
		radioButton_autoCheckFold->click();
	}
	if (event->key() == Qt::Key_F8) {
		radioButton_autoCheckCallAny->click();
	}
	if (event->key() == Qt::Key_Shift) {
		if(myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_LOCAL) {
			pushButton_break->click();
			//ctrlPressed = true;
		}
	}
	//    if (event->key() == Qt::Key_Escape && (myActionIsBet || myActionIsRaise)) {
	//            meInAction();
	//    }
	if (event->key() == Qt::Key_Up && lineEdit_ChatInput->hasFocus()) {
		if((keyUpDownChatCounter + 1) <= myChat->getChatLinesHistorySize()) {
			keyUpDownChatCounter++;
		}
		// 		std::cout << "Up keyUpDownChatCounter: " << keyUpDownChatCounter << "\n";
		myChat->showChatHistoryIndex(keyUpDownChatCounter);
	} else if(event->key() == Qt::Key_Down && lineEdit_ChatInput->hasFocus()) {
		if((keyUpDownChatCounter - 1) >= 0) {
			keyUpDownChatCounter--;
		}
		// 		std::cout << "Down keyUpDownChatCounter: " << keyUpDownChatCounter << "\n";
		myChat->showChatHistoryIndex(keyUpDownChatCounter);
	} else {
		keyUpDownChatCounter = 0;
	}

#endif

}

void gameTableImpl::changePlayingMode()
{

	int mode = -1;

#ifdef GUI_800x480
	if(tabs.radioButton_manualAction->isChecked()) {
		mode=0;
	}
	if(tabs.radioButton_autoCheckFold->isChecked()) {
		mode=2;
	}
	if(tabs.radioButton_autoCheckCallAny->isChecked()) {
		mode=1;
	}
#else
	if(radioButton_manualAction->isChecked()) {
		mode=0;
	}
	if(radioButton_autoCheckFold->isChecked()) {
		mode=2;
	}
	if(radioButton_autoCheckCallAny->isChecked()) {
		mode=1;
	}
#endif


	/*	switch (mode) {

			// 		case 0: { statusBar()->showMessage(tr("Manual mode set. You've got to choose yourself now."), 5000); }
			break;
			// 		case 1: { statusBar()->showMessage(tr("Auto mode set: Check or call any."), 5000); }
			break;
			// 		case 2: { statusBar()->showMessage(tr("Auto mode set: Check or fold."), 5000); }
			break;
		default: {
			//cout << "changePlayingMode ERROR!!!!" << endl;
		}

		}*/

	playingMode = mode;
}

bool gameTableImpl::eventFilter(QObject *obj, QEvent *event)
{
	QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

	if (/*obj == lineEdit_ChatInput && lineEdit_ChatInput->text() != "" && */event->type() == QEvent::KeyPress && keyEvent->key() == Qt::Key_Tab) {
		myChat->nickAutoCompletition();
		return true;
	} else if (event->type() == QEvent::KeyPress && keyEvent->key() == Qt::Key_Back) {
		event->ignore();
		closeGameTable();
		return true;
	} else if (event->type() == QEvent::Close) {
		event->ignore();
		closeGameTable();
		return true;
	} else if (event->type() == QEvent::Resize) {
		refreshSpectatorsDisplay();
		return true;
	} else {
		// pass the event on to the parent class
		return QMainWindow::eventFilter(obj, event);
	}
}

void gameTableImpl::switchChatWindow()
{

	int tab = 1;
#ifdef GUI_800x480
	if (tabs.groupBox_LeftToolBox->isHidden()) {
		tabs.tabWidget_Left->setCurrentIndex(tab);
		tabs.groupBox_LeftToolBox->show();
	}	else {
		if (tabs.tabWidget_Left->currentIndex() == tab) {
			tabs.groupBox_LeftToolBox->hide();
		} else {
			tabs.tabWidget_Left->setCurrentIndex(tab);
		}
	}
#else
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
#endif
}

void gameTableImpl::switchHelpWindow()
{

	int tab = 0;
#ifdef GUI_800x480
	if (tabs.groupBox_LeftToolBox->isHidden()) {
		tabs.tabWidget_Left->setCurrentIndex(tab);
		tabs.groupBox_LeftToolBox->show();
	}	else {
		if (tabs.tabWidget_Left->currentIndex() == tab) {
			tabs.groupBox_LeftToolBox->hide();
		} else {
			tabs.tabWidget_Left->setCurrentIndex(tab);
		}
	}
#else
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
#endif
}

void gameTableImpl::switchLogWindow()
{

	int tab = 0;
#ifdef GUI_800x480
	if (tabs.groupBox_RightToolBox->isHidden()) {
		tabs.tabWidget_Right->setCurrentIndex(tab);
		tabs.groupBox_RightToolBox->show();
	}	else {
		if (tabs.tabWidget_Right->currentIndex() == tab) {
			tabs.groupBox_RightToolBox->hide();
		} else {
			tabs.tabWidget_Right->setCurrentIndex(tab);
		}
	}
#else
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
#endif
}

void gameTableImpl::switchAwayWindow()
{

	int tab = 1;
#ifdef GUI_800x480
	if (tabs.groupBox_RightToolBox->isHidden()) {
		tabs.tabWidget_Right->setCurrentIndex(tab);
		tabs.groupBox_RightToolBox->show();
	}	else {
		if (tabs.tabWidget_Right->currentIndex() == tab) {
			tabs.groupBox_RightToolBox->hide();
		} else {
			tabs.tabWidget_Right->setCurrentIndex(tab);
		}
	}
#else
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
#endif
}

void gameTableImpl::switchChanceWindow()
{

	int tab = 2;
#ifdef GUI_800x480
	if (tabs.groupBox_RightToolBox->isHidden()) {
		tabs.tabWidget_Right->setCurrentIndex(tab);
		tabs.groupBox_RightToolBox->show();
	}	else {
		if (tabs.tabWidget_Right->currentIndex() == tab) {
			tabs.groupBox_RightToolBox->hide();
		} else {
			tabs.tabWidget_Right->setCurrentIndex(tab);
		}
	}
#else
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
#endif
}

void gameTableImpl::switchFullscreen()
{

	if (this->isFullScreen()) {
		this->showNormal();
	} else {
		this->showFullScreen();
	}
}

void gameTableImpl::blinkingStartButtonAnimationAction()
{

#ifdef GUI_800x480
	QString style = tabs.pushButton_break->styleSheet();
#else
	QString style = pushButton_break->styleSheet();
#endif

	if(style.contains("QPushButton:enabled { background-color: #"+myGameTableStyle->getBreakLobbyButtonBgColor())) {
#ifdef GUI_800x480
		myGameTableStyle->setBreakButtonStyle(tabs.pushButton_break,1);
#else
		myGameTableStyle->setBreakButtonStyle(pushButton_break,1);
#endif
	} else {
#ifdef GUI_800x480
		myGameTableStyle->setBreakButtonStyle(tabs.pushButton_break,0);
#else
		myGameTableStyle->setBreakButtonStyle(pushButton_break,0);
#endif
	}
}

void gameTableImpl::sendChatMessage()
{
	myChat->sendMessage();
}
void gameTableImpl::checkChatInputLength(QString string)
{
	myChat->checkInputLength(string);
}


void gameTableImpl::tabSwitchAction()
{

#ifdef GUI_800x480
	switch(tabs.tabWidget_Left->currentIndex()) {

	case 1: {
		tabs.lineEdit_ChatInput->setFocus();
	}
	break;
	default: {
		tabs.lineEdit_ChatInput->clearFocus();
	}

	}
#else
	switch(tabWidget_Left->currentIndex()) {

	case 1: {
		lineEdit_ChatInput->setFocus();
	}
	break;
	default: {
		lineEdit_ChatInput->clearFocus();
	}

	}
#endif

}


void gameTableImpl::localGameModification()
{

#ifdef GUI_800x480
	tabs.tabWidget_Left->setCurrentIndex(0);
	tabs.tabWidget_Left->removeTab(1);
	tabs.tabWidget_Left->removeTab(1);
	tabs.tabWidget_Left->removeTab(1);
#else
	tabWidget_Left->setCurrentIndex(0);
	tabWidget_Left->removeTab(1);
	tabWidget_Left->removeTab(1);
	tabWidget_Left->removeTab(1);
#endif

	int i;
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++ ) {
		timeoutLabelArray[i]->stopTimeOutAnimation();
		playerAvatarLabelArray[i]->setEnabledContextMenu(false);
	}

#ifdef GUI_800x480
	tabs.pushButton_break->show();
#else
	pushButton_break->show();
#endif
	QFontMetrics tempMetrics = this->fontMetrics();
	int width = tempMetrics.width(tr("Stop"));
#ifdef GUI_800x480
	tabs.pushButton_break->setText(tr("Stop"));
	tabs.pushButton_break->setMinimumSize(width+10,20);
#else
	pushButton_break->setText(tr("Stop"));
	pushButton_break->setMinimumSize(width+10,20);
#endif

	//Set the playing mode to "manual"
#ifdef GUI_800x480
	tabs.radioButton_manualAction->click();
#else
	radioButton_manualAction->click();
#endif

	//restore saved windows geometry
	restoreGameTableGeometry();

	if(myGameTableStyle->getState() != GT_STYLE_OK) myGameTableStyle->showErrorMessage();

	//let the SoundEventHandler know that there is a new game
	mySoundEventHandler->newGameStarts();
	spectatorIcon->hide();
	spectatorNumberLabel->hide();
}

void gameTableImpl::networkGameModification()
{

#ifdef GUI_800x480
	if(tabs.tabWidget_Left->widget(1) != tabs.tab_Chat)
		tabs.tabWidget_Left->insertTab(1, tabs.tab_Chat, QString(tr("Chat"))); /*TODO text abgeschnitten --> stylesheets*/

	tabs.tabWidget_Left->removeTab(2);
	tabs.tabWidget_Left->removeTab(2);

	tabs.tabWidget_Left->setCurrentIndex(1);
#else
	if(tabWidget_Left->widget(1) != tab_Chat)
		tabWidget_Left->insertTab(1, tab_Chat, QString(tr("Chat"))); /*TODO text abgeschnitten --> stylesheets*/

	tabWidget_Left->removeTab(2);
	tabWidget_Left->removeTab(2);

	tabWidget_Left->setCurrentIndex(1);
#endif
	myChat->clearChat();

	int i;
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++ ) {
		playerAvatarLabelArray[i]->setEnabledContextMenu(true);
		playerAvatarLabelArray[i]->setVoteOnKickContextMenuEnabled(true);
		playerAvatarLabelArray[i]->setVoteRunning(false);
	}

	if(myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_INTERNET) {
#ifdef GUI_800x480
		tabs.pushButton_break->show();
#else
		pushButton_break->show();
#endif
		QFontMetrics tempMetrics = this->fontMetrics();
		int width = tempMetrics.width(tr("Lobby"));
#ifdef GUI_800x480
		tabs.pushButton_break->setText(tr("Lobby"));
		tabs.pushButton_break->setMinimumSize(width+10,20);
		myGameTableStyle->setBreakButtonStyle(tabs.pushButton_break,0);
#else
		pushButton_break->setText(tr("Lobby"));
		pushButton_break->setMinimumSize(width+10,20);
		myGameTableStyle->setBreakButtonStyle(pushButton_break,0);
#endif
		blinkingStartButtonAnimationTimer->stop();
		spectatorIcon->show();
		spectatorNumberLabel->show();
		refreshSpectatorsDisplay();
	}
	if(myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_NETWORK) {
#ifdef GUI_800x480
		tabs.pushButton_break->hide();
#else
		pushButton_break->hide();
#endif
		spectatorIcon->hide();
		spectatorNumberLabel->hide();
	}
	//Set the playing mode to "manual"
#ifdef GUI_800x480
	tabs.radioButton_manualAction->click();
#else
	radioButton_manualAction->click();
#endif

	//restore saved windows geometry
	restoreGameTableGeometry();

	if(myStartWindow->getSession()->getClientPlayerInfo(myStartWindow->getSession()->getClientUniquePlayerId()).isGuest) {
		guestUserMode();
	} else {
		registeredUserMode();
	}

	blinkingStartButtonAnimationTimer->stop();
#ifdef GUI_800x480
	myGameTableStyle->setBreakButtonStyle(tabs.pushButton_break,0);
#else
	myGameTableStyle->setBreakButtonStyle(pushButton_break,0);
#endif

	//let the SoundEventHandler know that there is a new game
	mySoundEventHandler->newGameStarts();
}

void gameTableImpl::mouseOverFlipCards(bool front)
{

	if(myStartWindow->getSession()->getCurrentGame()) {
		if(myConfig->readConfigInt("AntiPeekMode") && myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getSeatsList()->front()->getMyActiveStatus()/* && myStartWindow->getSession()->getCurrentGame()->getSeatsList()->front()->getMyAction() != PLAYER_ACTION_FOLD*/) {
			holeCardsArray[0][0]->signalFastFlipCards(front);
			holeCardsArray[0][1]->signalFastFlipCards(front);
		}
	}
}

void gameTableImpl::updateMyButtonsState(int mode)
{

	boost::shared_ptr<HandInterface> currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();

	if(currentHand->getPreviousPlayerID() == 0) {
		myButtonsCheckable(false);
		clearMyButtons();
	} else {
		if(currentHand->getSeatsList()->front()->getMyAction() != PLAYER_ACTION_ALLIN) { // dont show pre-actions after flip cards when allin
			myButtonsCheckable(true);
			provideMyActions(mode);
		}
	}
}

void gameTableImpl::uncheckMyButtons()
{

	pushButton_BetRaise->setChecked(false);
	pushButton_CallCheck->setChecked(false);
	pushButton_Fold->setChecked(false);
	pushButton_AllIn->setChecked(false);

}

void gameTableImpl::resetMyButtonsCheckStateMemory()
{

	pushButtonCallCheckIsChecked = false;
	pushButtonFoldIsChecked = false;
	pushButtonAllInIsChecked = false;
	pushButtonBetRaiseIsChecked = false;
}

void gameTableImpl::clearMyButtons()
{

	refreshActionButtonFKeyIndicator(1);

	pushButton_BetRaise->setText("");
	pushButton_CallCheck->setText("");
	pushButton_Fold->setText("");
	pushButton_AllIn->setText("");
}

void gameTableImpl::myButtonsCheckable(bool state)
{

	boost::shared_ptr<HandInterface> currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();

	if(state) {
		//checkable

		// exception: full bet rule
		if(!currentHand->getCurrentBeRo()->getFullBetRule()) {
			pushButton_BetRaise->setCheckable(true);
		}
		pushButton_CallCheck->setCheckable(true);
		pushButton_Fold->setCheckable(true);
		pushButton_AllIn->setCheckable(true);

		//design
		myGameTableStyle->setButtonsStyle(pushButton_BetRaise, pushButton_CallCheck, pushButton_Fold, pushButton_AllIn, 2);

		myButtonsAreCheckable = true;
	} else {
		//not checkable

		pushButton_BetRaise->setCheckable(false);
		pushButton_CallCheck->setCheckable(false);
		pushButton_Fold->setCheckable(false);
		pushButton_AllIn->setCheckable(false);

		QString hover;
		if(pushButton_AllIn->text()==AllInString) {
			myGameTableStyle->setButtonsStyle(pushButton_BetRaise, pushButton_CallCheck, pushButton_Fold, pushButton_AllIn, 0);
		} else {
			myGameTableStyle->setButtonsStyle(pushButton_BetRaise, pushButton_CallCheck, pushButton_Fold, pushButton_AllIn, 1);
		}

		myButtonsAreCheckable = false;
	}
}

void gameTableImpl::showMaximized ()
{
	this->showFullScreen ();
}

void gameTableImpl::closeGameTable()
{

	if (myStartWindow->getMyServerGuiInterface() && myStartWindow->getMyServerGuiInterface()->getSession()->isNetworkServerRunning()) {

		MyMessageBox msgBox(QMessageBox::Warning, tr("Closing PokerTH during network game"),
							tr("You are the hosting server. Do you want to close PokerTH anyway?"), QMessageBox::Yes | QMessageBox::No, this);

		if (msgBox.exec() == QMessageBox::Yes ) {
			myStartWindow->getSession()->terminateNetworkClient();
			stopTimer();
			if (myStartWindow->getMyServerGuiInterface()) myStartWindow->getMyServerGuiInterface()->getSession()->terminateNetworkServer();
			saveGameTableGeometry();
			myStartWindow->show();
			this->hide();
		}
	} else {

		bool close = true;

		if(myUniversalMessageDialog->checkIfMesssageWillBeDisplayed(CLOSE_GAMETABLE_QUESTION ) && (myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_INTERNET || myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_NETWORK ) && this->isVisible()) {
			if (myUniversalMessageDialog->exec(CLOSE_GAMETABLE_QUESTION , tr("Really want to exit?"), tr("PokerTH - Close Table?"), QPixmap(":/gfx/logoChip3D.png"), QDialogButtonBox::Yes|QDialogButtonBox::No, true) == QDialog::Rejected) {
				close = false;
			}
		}

		if(close) {
			//now really close the table
			myStartWindow->getSession()->terminateNetworkClient();
			stopTimer();
			saveGameTableGeometry();
			myStartWindow->show();
			this->hide();
		}
	}
}

void gameTableImpl::changeSpinBoxBetValue(int value)
{
	if(betSliderChangedByInput) {
		//prevent interval cutting of spinBox_betValue input from code below
		betSliderChangedByInput = false;
	} else {

		if(horizontalSlider_bet->value() == horizontalSlider_bet->maximum()) {

			spinBox_betValue->setValue(horizontalSlider_bet->value());
		} else {

			int temp;
			if(horizontalSlider_bet->maximum() <= 1000 ) {
				temp = (int)((value/10)*10);
			} else if(horizontalSlider_bet->maximum() > 1000 && horizontalSlider_bet->maximum() <= 10000) {
				temp = (int)((value/50)*50);
			} else if(horizontalSlider_bet->maximum() > 10000 && horizontalSlider_bet->maximum() <= 100000) {
				temp = (int)((value/500)*500);
			} else {
				temp = (int)((value/5000)*5000);
			}

			if(temp < horizontalSlider_bet->minimum())
				spinBox_betValue->setValue(horizontalSlider_bet->minimum());
			else
				spinBox_betValue->setValue(temp);
		}
	}
}

void gameTableImpl::spinBoxBetValueChanged(int value)
{

	if(horizontalSlider_bet->isEnabled()) {

		QString betRaise = pushButton_BetRaise->text().section("\n",0 ,0);

		if(value >= horizontalSlider_bet->minimum()) {

			if(value > horizontalSlider_bet->maximum()) { // print the maximum
				pushButton_BetRaise->setText(betRaise + "\n$" + QString("%L1").arg(horizontalSlider_bet->maximum()));
				betSliderChangedByInput = true;
				horizontalSlider_bet->setValue(horizontalSlider_bet->maximum());
			} else { // really print the value
				pushButton_BetRaise->setText(betRaise + "\n$" + QString("%L1").arg(value));
				betSliderChangedByInput = true;
				horizontalSlider_bet->setValue(value);
			}
		} else { // print the minimum
			pushButton_BetRaise->setText(betRaise + "\n$" + QString("%L1").arg(horizontalSlider_bet->minimum()));
			betSliderChangedByInput = true;
			horizontalSlider_bet->setValue(horizontalSlider_bet->minimum());
		}
	}
}

void gameTableImpl::leaveCurrentNetworkGame()
{

	if (myStartWindow->getSession()->isNetworkClientRunning()) {

		if(!myUniversalMessageDialog->checkIfMesssageWillBeDisplayed(BACKTO_LOBBY_QUESTION)) {

			assert(myStartWindow->getSession());
			myStartWindow->getSession()->sendLeaveCurrentGame();
		} else {
			if (myUniversalMessageDialog->exec(BACKTO_LOBBY_QUESTION, tr("Attention! Do you really want to leave the current game\nand go back to the lobby?"), tr("PokerTH - Internet Game Message"), QPixmap(":/gfx/logoChip3D.png"), QDialogButtonBox::Yes|QDialogButtonBox::No, true) == QDialog::Accepted) {
				assert(myStartWindow->getSession());
				myStartWindow->getSession()->sendLeaveCurrentGame();
			}
		}
	}
}

void gameTableImpl::triggerVoteOnKick(int id)
{

	assert(myStartWindow->getSession()->getCurrentGame());
	PlayerList seatList = myStartWindow->getSession()->getCurrentGame()->getSeatsList();
	int playerCount = static_cast<int>(seatList->size());
	if (id < playerCount) {
		PlayerListIterator pos = seatList->begin();
		advance(pos, id);
		myStartWindow->getSession()->startVoteKickPlayer((*pos)->getMyUniqueID());
	}
}

void gameTableImpl::startVoteOnKick(unsigned playerId, unsigned voteStarterPlayerId, int timeoutSec, int numVotesNeededToKick)
{
#ifdef GUI_800x480
	if(tabs.tabWidget_Left->widget(2) != tabs.tab_Kick)
		tabs.tabWidget_Left->insertTab(2, tabs.tab_Kick, QString(tr("Kick")));

	tabs.tabWidget_Left->setCurrentIndex(2);
	tabs.pushButton_voteOnKickNo->hide();
	tabs.pushButton_voteOnKickYes->hide();
	tabs.label_kickUser->clear();
	tabs.label_kickVoteTimeout->clear();
#else
	if(tabWidget_Left->widget(2) != tab_Kick)
		tabWidget_Left->insertTab(2, tab_Kick, QString(tr("Kick")));

	tabWidget_Left->setCurrentIndex(2);
	pushButton_voteOnKickNo->hide();
	pushButton_voteOnKickYes->hide();
	label_kickUser->clear();
	label_kickVoteTimeout->clear();
#endif

	voteOnKickTimeoutSecs = timeoutSec;

	playerAboutToBeKickedId = playerId;
	refreshVotesMonitor(1, numVotesNeededToKick);

	PlayerInfo info(myStartWindow->getSession()->getClientPlayerInfo(voteStarterPlayerId));
#ifdef GUI_800x480
	tabs.label_voteStarterNick->setText("<b>"+QString::fromUtf8(info.playerName.c_str())+"</b>");
#else
	label_voteStarterNick->setText("<b>"+QString::fromUtf8(info.playerName.c_str())+"</b>");
#endif

	startVoteOnKickTimeout();

	int i;
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++ ) {
		playerAvatarLabelArray[i]->setVoteRunning(true);
	}
}

void gameTableImpl::changeVoteOnKickButtonsState(bool showHide)
{
	if(showHide) {

		PlayerInfo info(myStartWindow->getSession()->getClientPlayerInfo(playerAboutToBeKickedId));
#ifdef GUI_800x480
		tabs.label_kickUser->setText(tr("Do you want to kick <b>%1</b><br>from this game?").arg(QString::fromUtf8(info.playerName.c_str())));
		tabs.pushButton_voteOnKickNo->show();
		tabs.pushButton_voteOnKickYes->show();
#else
		label_kickUser->setText(tr("Do you want to kick <b>%1</b><br>from this game?").arg(QString::fromUtf8(info.playerName.c_str())));
		pushButton_voteOnKickNo->show();
		pushButton_voteOnKickYes->show();
#endif
	} else {
#ifdef GUI_800x480
		tabs.label_kickUser->clear();
		tabs.pushButton_voteOnKickNo->hide();
		tabs.pushButton_voteOnKickYes->hide();
#else
		label_kickUser->clear();
		pushButton_voteOnKickNo->hide();
		pushButton_voteOnKickYes->hide();
#endif
	}
}

void gameTableImpl::endVoteOnKick()
{
	stopVoteOnKickTimeout();
#ifdef GUI_800x480
	tabs.tabWidget_Left->removeTab(2);
#else
	tabWidget_Left->removeTab(2);
#endif

	int i;
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++ ) {
		playerAvatarLabelArray[i]->setVoteRunning(false);
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
#ifdef GUI_800x480
		tabs.label_kickVoteTimeout->setText(tr("<b>%1</b> secs left").arg(voteOnKickTimeoutSecs-voteOnKickRealTimer.elapsed().total_seconds()));
#else
		label_kickVoteTimeout->setText(tr("<b>%1</b> secs left").arg(voteOnKickTimeoutSecs-voteOnKickRealTimer.elapsed().total_seconds()));
#endif
	else
#ifdef GUI_800x480
		tabs.label_kickVoteTimeout->setText(tr("<b>%1</b> secs left").arg(0));
#else
		label_kickVoteTimeout->setText(tr("<b>%1</b> secs left").arg(0));
#endif
}

void gameTableImpl::refreshVotesMonitor(int currentVotes, int numVotesNeededToKick)
{
	QString currentVotesString;
	if(currentVotes == 1) currentVotesString = tr("vote");
	else currentVotesString = tr("votes");

	if((*myStartWindow->getSession()->getCurrentGame()->getSeatsList()->begin())->getMyUniqueID() != playerAboutToBeKickedId) {
		PlayerInfo info(myStartWindow->getSession()->getClientPlayerInfo(playerAboutToBeKickedId));
#ifdef GUI_800x480
		tabs.label_votesMonitor->setText(tr("Player <b>%1</b> has <b>%2</b> %3<br>against him. <b>%4</b> vote(s) needed to kick.").arg(QString::fromUtf8(info.playerName.c_str())).arg(currentVotes).arg(currentVotesString).arg(numVotesNeededToKick-currentVotes));
#else
		label_votesMonitor->setText(tr("Player <b>%1</b> has <b>%2</b> %3<br>against him. <b>%4</b> vote(s) needed to kick.").arg(QString::fromUtf8(info.playerName.c_str())).arg(currentVotes).arg(currentVotesString).arg(numVotesNeededToKick-currentVotes));
#endif
	} else {
#ifdef GUI_800x480
		tabs.label_votesMonitor->setText(tr("You have <b>%1</b> %2 against you.<br><b>%3</b> vote(s) needed to kick.").arg(currentVotes).arg(currentVotesString).arg(numVotesNeededToKick-currentVotes));
#else
		label_votesMonitor->setText(tr("You have <b>%1</b> %2 against you.<br><b>%3</b> vote(s) needed to kick.").arg(currentVotes).arg(currentVotesString).arg(numVotesNeededToKick-currentVotes));
#endif
	}
}

void gameTableImpl::refreshCardsChance(GameState bero)
{
	if(myConfig->readConfigInt("ShowCardsChanceMonitor")) {

		boost::shared_ptr<PlayerInterface> humanPlayer = myStartWindow->getSession()->getCurrentGame()->getSeatsList()->front();
		if(humanPlayer->getMyActiveStatus()) {
			int boardCards[5];
			int holeCards[2];

			humanPlayer->getMyCards(holeCards);
			myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getBoard()->getMyCards(boardCards);

			if(humanPlayer->getMyAction() == PLAYER_ACTION_FOLD) {
#ifdef GUI_800x480
				tabs.label_chance->refreshChance(CardsValue::calcCardsChance(bero, holeCards, boardCards), true);
#else
				label_chance->refreshChance(CardsValue::calcCardsChance(bero, holeCards, boardCards), true);
#endif
			} else {
#ifdef GUI_800x480
				tabs.label_chance->refreshChance(CardsValue::calcCardsChance(bero, holeCards, boardCards), false);
#else
				label_chance->refreshChance(CardsValue::calcCardsChance(bero, holeCards, boardCards), false);
#endif
			}
		} else {
#ifdef GUI_800x480
			tabs.label_chance->resetChance();
#else
			label_chance->resetChance();
#endif
		}
	}
}

void gameTableImpl::refreshActionButtonFKeyIndicator(bool clear)
{
	if(clear) {
		pushButton_AllIn->setFKeyText("");
		pushButton_BetRaise->setFKeyText("");
		pushButton_CallCheck->setFKeyText("");
		pushButton_Fold->setFKeyText("");
	} else {
#ifndef GUI_800x480
		if(myConfig->readConfigInt("AlternateFKeysUserActionMode") == 0 ) {
			if(!pushButton_AllIn->text().isEmpty()) pushButton_AllIn->setFKeyText("F4");
			if(!pushButton_BetRaise->text().isEmpty()) pushButton_BetRaise->setFKeyText("F3");
			if(!pushButton_CallCheck->text().isEmpty()) pushButton_CallCheck->setFKeyText("F2");
			if(!pushButton_Fold->text().isEmpty()) pushButton_Fold->setFKeyText("F1");
		} else {
			if(!pushButton_AllIn->text().isEmpty()) pushButton_AllIn->setFKeyText("F1");
			if(!pushButton_BetRaise->text().isEmpty()) pushButton_BetRaise->setFKeyText("F2");
			if(!pushButton_CallCheck->text().isEmpty()) pushButton_CallCheck->setFKeyText("F3");
			if(!pushButton_Fold->text().isEmpty()) pushButton_Fold->setFKeyText("F4");
		}
#endif
	}
}

void gameTableImpl::refreshGameTableStyle()
{
	myGameTableStyle->setWindowsGeometry(this);
#ifdef GUI_800x480
#ifdef ANDROID
	myGameTableStyle->setChatLogStyle(tabs.textBrowser_Log);
#endif
	myGameTableStyle->setChatLogStyle(tabs.textBrowser_Chat);
	myGameTableStyle->setChatLogStyle(tabs.textEdit_tipInput);
	myGameTableStyle->setChatInputStyle(tabs.lineEdit_ChatInput);
#else
	myGameTableStyle->setChatLogStyle(textBrowser_Log);
	myGameTableStyle->setChatLogStyle(textBrowser_Chat);
	myGameTableStyle->setChatLogStyle(textEdit_tipInput);
	myGameTableStyle->setChatInputStyle(lineEdit_ChatInput);
#endif

	int i;
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {

		myGameTableStyle->setCashLabelStyle(cashLabelArray[i]);
		myGameTableStyle->setSetLabelStyle(setLabelArray[i]);
		myGameTableStyle->setPlayerNameLabelStyle(playerNameLabelArray[i]);
	}

#ifdef GUI_800x480
	myGameTableStyle->setBigFontBoardStyle(label_Sets);
	myGameTableStyle->setBigFontBoardStyle(label_Total);
	myGameTableStyle->setBigFontBoardStyle(textLabel_Sets);
	myGameTableStyle->setBigFontBoardStyle(textLabel_Pot);
	myGameTableStyle->setBigFontBoardStyle(label_handNumber);
	myGameTableStyle->setBigFontBoardStyle(label_gameNumber);
	myGameTableStyle->setBigFontBoardStyle(label_handNumberValue);
	myGameTableStyle->setBigFontBoardStyle(label_gameNumberValue);
	myGameTableStyle->setBigFontBoardStyle(textLabel_handLabel);
#else
	myGameTableStyle->setSmallFontBoardStyle(label_Sets);
	myGameTableStyle->setSmallFontBoardStyle(label_Total);
	myGameTableStyle->setSmallFontBoardStyle(textLabel_Sets);
	myGameTableStyle->setSmallFontBoardStyle(textLabel_Pot);
	myGameTableStyle->setSmallFontBoardStyle(label_handNumber);
	myGameTableStyle->setSmallFontBoardStyle(label_gameNumber);
	myGameTableStyle->setSmallFontBoardStyle(label_handNumberValue);
	myGameTableStyle->setSmallFontBoardStyle(label_gameNumberValue);
	myGameTableStyle->setBigFontBoardStyle(textLabel_handLabel);
	myGameTableStyle->setBigFontBoardStyle(label_Pot);
#endif
	myGameTableStyle->setCardHolderStyle(label_CardHolder0,0);
	myGameTableStyle->setCardHolderStyle(label_CardHolder1,0);
	myGameTableStyle->setCardHolderStyle(label_CardHolder2,0);
	myGameTableStyle->setCardHolderStyle(label_CardHolder3,1);
	myGameTableStyle->setCardHolderStyle(label_CardHolder4,2);
	myGameTableStyle->setTableBackground(this);
#ifdef GUI_800x480
	myGameTableStyle->setBreakButtonStyle(tabs.pushButton_break,0);
	myGameTableStyle->setBreakButtonStyle(tabs.pushButton_tipSave,0);
	myGameTableStyle->setSpeedStringStyle(tabs.label_speedString);
	myGameTableStyle->setSpeedStringStyle(tabs.label_speedValue);
	myGameTableStyle->setVoteButtonStyle(tabs.pushButton_voteOnKickYes);
	myGameTableStyle->setVoteButtonStyle(tabs.pushButton_voteOnKickNo);
	myGameTableStyle->setVoteStringsStyle(tabs.label_timeout);
	myGameTableStyle->setVoteStringsStyle(tabs.label_kickVoteTimeout);
	myGameTableStyle->setVoteStringsStyle(tabs.label_kickUser);
	myGameTableStyle->setVoteStringsStyle(tabs.label_votesMonitor);
	myGameTableStyle->setVoteStringsStyle(tabs.label_voteStarterNick);
	myGameTableStyle->setVoteStringsStyle(tabs.label_votestartedby);
#else
	myGameTableStyle->setMenuBarStyle(menubar);
	myGameTableStyle->setBreakButtonStyle(pushButton_break,0);
	myGameTableStyle->setBreakButtonStyle(pushButton_tipSave,0);
	myGameTableStyle->setSpeedStringStyle(label_speedString);
	myGameTableStyle->setSpeedStringStyle(label_speedValue);
	myGameTableStyle->setVoteButtonStyle(pushButton_voteOnKickYes);
	myGameTableStyle->setVoteButtonStyle(pushButton_voteOnKickNo);
	myGameTableStyle->setVoteStringsStyle(label_timeout);
	myGameTableStyle->setVoteStringsStyle(label_kickVoteTimeout);
	myGameTableStyle->setVoteStringsStyle(label_kickUser);
	myGameTableStyle->setVoteStringsStyle(label_votesMonitor);
	myGameTableStyle->setVoteStringsStyle(label_voteStarterNick);
	myGameTableStyle->setVoteStringsStyle(label_votestartedby);
#endif

	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {

		myGameTableStyle->setPlayerSeatInactiveStyle(groupBoxArray[i]);
	}
	//Human player button
	myGameTableStyle->setButtonsStyle(pushButton_BetRaise, pushButton_CallCheck, pushButton_Fold, pushButton_AllIn, 0);
	myGameTableStyle->setShowMyCardsButtonStyle(pushButton_showMyCards);

	myGameTableStyle->setBetValueInputStyle(spinBox_betValue);
	myGameTableStyle->setSliderStyle(horizontalSlider_bet);
#ifdef GUI_800x480
	myGameTableStyle->setSliderStyle(tabs.horizontalSlider_speed);
#else
	myGameTableStyle->setSliderStyle(horizontalSlider_speed);
#endif

	// 	away radiobuttons
#ifdef GUI_800x480
	myGameTableStyle->setAwayRadioButtonsStyle(tabs.radioButton_manualAction);
	myGameTableStyle->setAwayRadioButtonsStyle(tabs.radioButton_autoCheckFold);
	myGameTableStyle->setAwayRadioButtonsStyle(tabs.radioButton_autoCheckCallAny);

	myGameTableStyle->setToolBoxBackground(tabs.groupBox_RightToolBox);
	myGameTableStyle->setToolBoxBackground(tabs.groupBox_LeftToolBox);

	myGameTableStyle->setTabWidgetStyle(tabs.tabWidget_Right, tabs.tabWidget_Right->getMyTabBar());
	myGameTableStyle->setTabWidgetStyle(tabs.tabWidget_Left, tabs.tabWidget_Left->getMyTabBar());

	tabs.label_Handranking->setPixmap(myGameTableStyle->getHandRanking());
#else
	myGameTableStyle->setAwayRadioButtonsStyle(radioButton_manualAction);
	myGameTableStyle->setAwayRadioButtonsStyle(radioButton_autoCheckFold);
	myGameTableStyle->setAwayRadioButtonsStyle(radioButton_autoCheckCallAny);

	myGameTableStyle->setToolBoxBackground(groupBox_RightToolBox);
	myGameTableStyle->setToolBoxBackground(groupBox_LeftToolBox);

	myGameTableStyle->setTabWidgetStyle(tabWidget_Right, tabWidget_Right->getMyTabBar());
	myGameTableStyle->setTabWidgetStyle(tabWidget_Left, tabWidget_Left->getMyTabBar());

	label_Handranking->setPixmap(myGameTableStyle->getHandRanking());
#endif

	if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || myGameTableStyle->getActionCallI18NString() == "NULL") {
		CallString = "Call";
	} else {
		CallString = myGameTableStyle->getActionCallI18NString();
	}
	if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || myGameTableStyle->getActionCheckI18NString() == "NULL") {
		CheckString = "Check";
	} else {
		CheckString = myGameTableStyle->getActionCheckI18NString();
	}
	if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || myGameTableStyle->getActionBetI18NString() == "NULL") {
		BetString = "Bet";
	} else {
		BetString = myGameTableStyle->getActionBetI18NString();
	}
	if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || myGameTableStyle->getActionRaiseI18NString() == "NULL") {
		RaiseString = "Raise";
	} else {
		RaiseString = myGameTableStyle->getActionRaiseI18NString();
	}
	if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || myGameTableStyle->getActionFoldI18NString() == "NULL") {
		FoldString = "Fold";
	} else {
		FoldString = myGameTableStyle->getActionFoldI18NString();
	}
	if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || myGameTableStyle->getActionAllInI18NString() == "NULL") {
		AllInString = "All-In";
	} else {
		AllInString = myGameTableStyle->getActionAllInI18NString();
	}
	if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || myGameTableStyle->getPotI18NString() == "NULL") {
		PotString = "Pot";
	} else {
		PotString = myGameTableStyle->getPotI18NString();
	}
	if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || myGameTableStyle->getTotalI18NString() == "NULL") {
		TotalString = "Total";
	} else {
		TotalString = myGameTableStyle->getTotalI18NString();
	}
	if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || myGameTableStyle->getBetsI18NString() == "NULL") {
		BetsString = "Bets";
	} else {
		BetsString = myGameTableStyle->getBetsI18NString();
	}
	if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || myGameTableStyle->getGameI18NString() == "NULL") {
		GameString = "Game";
	} else {
		GameString = myGameTableStyle->getGameI18NString();
	}
	if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || myGameTableStyle->getHandI18NString() == "NULL") {
		HandString = "Hand";
	} else {
		HandString = myGameTableStyle->getHandI18NString();
	}
	if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || myGameTableStyle->getPreflopI18NString() == "NULL") {
		PreflopString = "Preflop";
	} else {
		PreflopString = myGameTableStyle->getPreflopI18NString();
	}
	if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || myGameTableStyle->getFlopI18NString() == "NULL") {
		FlopString = "Flop";
	} else {
		FlopString = myGameTableStyle->getFlopI18NString();
	}
	if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || myGameTableStyle->getTurnI18NString() == "NULL") {
		TurnString = "Turn";
	} else {
		TurnString = myGameTableStyle->getTurnI18NString();
	}
	if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || myGameTableStyle->getRiverI18NString() == "NULL") {
		RiverString = "River";
	} else {
		RiverString = myGameTableStyle->getRiverI18NString();
	}

#ifndef GUI_800x480
	label_Pot->setText(PotString);
#endif
	label_Total->setText(TotalString+":");
	label_Sets->setText(BetsString+":");
	label_handNumber->setText(HandString+":");
	label_gameNumber->setText(GameString+":");

	myGameTableStyle->setSpectatorNumberLabelStyle(spectatorNumberLabel);
}

void gameTableImpl::saveGameTableGeometry()
{
	if(this->isFullScreen()) {
		myConfig->writeConfigInt("GameTableFullScreenSave", 1);
	} else {
		myConfig->writeConfigInt("GameTableFullScreenSave", 0);
		myConfig->writeConfigInt("GameTableHeightSave", this->height());
		myConfig->writeConfigInt("GameTableWidthSave", this->width());
	}
	myConfig->writeBuffer();
}

void gameTableImpl::restoreGameTableGeometry()
{
	if(myConfig->readConfigInt("GameTableFullScreenSave")) {
#ifndef GUI_800x480
		if(actionFullScreen->isEnabled()) {
			this->showFullScreen();
		}
#endif
	} else {
		//resize only if style size allow this and if NOT fixed windows size
		if(!myGameTableStyle->getIfFixedWindowSize().toInt() && myConfig->readConfigInt("GameTableHeightSave") <= myGameTableStyle->getMaximumWindowHeight().toInt() && myConfig->readConfigInt("GameTableHeightSave") >= myGameTableStyle->getMinimumWindowHeight().toInt() && myConfig->readConfigInt("GameTableWidthSave") <= myGameTableStyle->getMaximumWindowWidth().toInt() && myConfig->readConfigInt("GameTableWidthSave") >= myGameTableStyle->getMinimumWindowWidth().toInt()) {

			this->resize(myConfig->readConfigInt("GameTableWidthSave"), myConfig->readConfigInt("GameTableHeightSave"));
		}
	}
#ifdef ANDROID
	if(getAndroidApiVersion() == 10) {
		QDesktopWidget dw;
		int availableWidth = dw.screenGeometry().width();
		int availableHeight = dw.screenGeometry().height();
		this->showNormal();
		this->setGeometry(0,0,availableWidth, availableHeight);
	}
#endif
}

void gameTableImpl::netClientPlayerLeft(unsigned /*playerId*/)
{
	if (myStartWindow->getSession()->getCurrentGame() && myStartWindow->getSession()->isNetworkClientRunning()) {
		refreshPlayerAvatar();
		refreshPlayerName();
	}
}

void gameTableImpl::netClientSpectatorJoined(unsigned /*playerId*/)
{
	refreshSpectatorsDisplay();
}

void gameTableImpl::netClientSpectatorLeft(unsigned /*playerId*/)
{
	refreshSpectatorsDisplay();
}

void gameTableImpl::registeredUserMode()
{
#ifdef GUI_800x480
	tabs.lineEdit_ChatInput->clear();
	tabs.lineEdit_ChatInput->setEnabled(true);
#else
	lineEdit_ChatInput->clear();
	lineEdit_ChatInput->setEnabled(true);
#endif
	guestMode = false;
}


void gameTableImpl::guestUserMode()
{
#ifdef GUI_800x480
	tabs.lineEdit_ChatInput->setText(tr("Chat is only available to registered players."));
	tabs.lineEdit_ChatInput->setDisabled(true);
#else
	lineEdit_ChatInput->setText(tr("Chat is only available to registered players."));
	lineEdit_ChatInput->setDisabled(true);
#endif
	guestMode = true;
}

void gameTableImpl::showShowMyCardsButton()
{
	pushButton_showMyCards->show();
	pushButton_showMyCards->raise();
}

void gameTableImpl::sendShowMyCardsSignal()
{
	if(pushButton_showMyCards->isVisible()) {

		myStartWindow->getSession()->showMyCards();

		pushButton_showMyCards->hide();
	}
}

void gameTableImpl::closeMessageBoxes()
{

	myUniversalMessageDialog->close();
}

void gameTableImpl::hide()
{
	//clear log
#ifdef GUI_800x480
	tabs.textBrowser_Log->clear();
	tabsDiag->hide();
#else
	textBrowser_Log->clear();
#endif
	QWidget::hide();
}

SeatState gameTableImpl::getCurrentSeatState(boost::shared_ptr<PlayerInterface> player)
{

	if(player->getMyActiveStatus()) {
		if(player->isSessionActive()) {
			return SEAT_ACTIVE;
		} else {
			return SEAT_AUTOFOLD;
		}
	} else {
		if(player->getMyStayOnTableStatus() && (myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_INTERNET || myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_NETWORK)) {
			return SEAT_STAYONTABLE;
		} else {
			return SEAT_CLEAR;
		}
	}
	return SEAT_UNDEFINED;
}

void gameTableImpl::soundEvent_blindsWereSet(int sbSet)
{
	mySoundEventHandler->blindsWereSet(sbSet);
}

void gameTableImpl::enableCallCheckPushButton()
{
	pushButton_CallCheck->setEatMyEvents(false);
}

#ifdef GUI_800x480
void gameTableImpl::tabsButtonClicked()
{
	tabsDiag->setParent(this, Qt::Dialog);
	tabsDiag->showFullScreen();
	tabsDiag->show();
	tabsDiag->raise();
	tabsDiag->activateWindow();
}

void gameTableImpl::tabsButtonClose()
{
	tabsDiag->close();
}
#endif


void gameTableImpl::checkActionLabelPosition()
{
#ifndef GUI_800x480
	int i;
	if(myCardDeckStyle->getBigIndexesActionBottom() == "1") {
		for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
			if(i>=3 && i<=7) {
				if(actionLabelArray[i]->y() == 56) {
					actionLabelArray[i]->move(actionLabelArray[i]->x(), 80);
				}
			} else {
				if(actionLabelArray[i]->y() == 43) {
					actionLabelArray[i]->move(actionLabelArray[i]->x(), 67);
				}
			}
		}
	} else {
		for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
			if(i>=3 && i<=7) {
				if(actionLabelArray[i]->y() == 80) {
					actionLabelArray[i]->move(actionLabelArray[i]->x(), 56);
				}
			} else {
				if(actionLabelArray[i]->y() == 67) {
					actionLabelArray[i]->move(actionLabelArray[i]->x(), 43);
				}
			}
		}
	}
#endif
}

void gameTableImpl::refreshSpectatorsDisplay()
{
	assert(myStartWindow->getSession());
	GameInfo info(myStartWindow->getSession()->getClientGameInfo(myStartWindow->getSession()->getClientCurrentGameId()));
	if(!info.spectatorsDuringGame.empty()) {
		spectatorIcon->show();
		spectatorNumberLabel->show();
		QPixmap spectatorPix(":/gfx/spectator.png");
		int iconX = this->centralWidget()->geometry().width() - spectatorPix.width() - 1;
		int iconY = 2;
		spectatorIcon->move(iconX,iconY);
		spectatorIcon->setPixmap(spectatorPix);
		int labelX = this->centralWidget()->geometry().width() - spectatorPix.width() - 1;
		int labelY = spectatorPix.height()+2;
		spectatorNumberLabel->setGeometry(labelX, labelY, 32, 14);
		spectatorNumberLabel->setText(QString("%1").arg(info.spectatorsDuringGame.size()));

		QString spectatorList = QString("<b>"+tr("Spectators")+":</b><br>");
		PlayerIdList::const_iterator i = info.spectatorsDuringGame.begin();
		PlayerIdList::const_iterator end = info.spectatorsDuringGame.end();
		while (i != end) {
			PlayerInfo playerInfo(myStartWindow->getSession()->getClientPlayerInfo(*i));
			spectatorList.append(QString::fromUtf8(playerInfo.playerName.c_str())+"<br>");
			++i;
		}
		spectatorList.remove(spectatorList.size()-4,4);
		spectatorIcon->setToolTip(spectatorList);
		spectatorNumberLabel->setToolTip(spectatorList);
	} else {
		spectatorIcon->setToolTip("");
		spectatorIcon->clear();
		spectatorIcon->hide();
		spectatorNumberLabel->setToolTip("");
		spectatorNumberLabel->clear();
		spectatorNumberLabel->hide();
	}
}

void gameTableImpl::pingUpdate(unsigned minPing, unsigned avgPing, unsigned maxPing)
{
	label_Avatar0->refreshPing(minPing, avgPing, maxPing);
}

int gameTableImpl::getAndroidApiVersion()
{
	int api = -1;
#ifdef ANDROID
#ifndef ANDROID_TEST
	JavaVM *currVM = (JavaVM *)QApplication::platformNativeInterface()->nativeResourceForIntegration("JavaVM");
	JNIEnv* env;
	if (currVM->AttachCurrentThread(&env, NULL)<0) {
		qCritical()<<"AttachCurrentThread failed";
	} else {
		jclass jclassApplicationClass = env->FindClass("android/os/Build$VERSION");
		if (jclassApplicationClass) {
			api = env->GetStaticIntField(jclassApplicationClass, env->GetStaticFieldID(jclassApplicationClass,"SDK_INT", "I"));
		}
		currVM->DetachCurrentThread();
	}
#endif
#endif
	return api;
}
