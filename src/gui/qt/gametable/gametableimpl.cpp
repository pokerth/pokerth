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
#include "myactionbutton.h"
#include "mychancelabel.h"
#include "mytimeoutlabel.h"
#include "mymenubar.h"
#include "log.h"
#include "chattools.h"

#include "playerinterface.h"
#include "boardinterface.h"
#include "handinterface.h"
#include "game.h"
#include "session.h"
#include "cardsvalue.h"

#include "configfile.h"
#include "sdlplayer.h"
#include "gametablestylereader.h"
#include "carddeckstylereader.h"
#include <gamedata.h>
#include <generic/serverguiwrapper.h>

#include <net/socket_msg.h>

#include "math.h"

#define FORMATLEFT(X) "<p align='center'>(X)"
#define FORMATRIGHT(X) "(X)</p>"

using namespace std;

gameTableImpl::gameTableImpl(ConfigFile *c, QMainWindow *parent)
    : QMainWindow(parent), myChat(NULL), myConfig(c), gameSpeed(0), myActionIsBet(0), myActionIsRaise(0), pushButtonBetRaiseIsChecked(FALSE), pushButtonCallCheckIsChecked(FALSE), pushButtonFoldIsChecked(FALSE), pushButtonAllInIsChecked(FALSE), myButtonsAreCheckable(FALSE), breakAfterCurrentHand(FALSE), currentGameOver(FALSE), betSliderChangedByInput(FALSE), guestMode(FALSE), myLastPreActionBetValue(0)
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
    label_chance->setMyStyle(myGameTableStyle);

    //Flipside festlegen;
    if (myConfig->readConfigInt("FlipsideOwn") && myConfig->readConfigString("FlipsideOwnFile") != "") {
        flipside = QPixmap::fromImage(QImage(QString::fromUtf8(myConfig->readConfigString("FlipsideOwnFile").c_str())));
    }
    else {
        flipside = QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+"flipside.png"));
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
    for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) { timeoutLabelArray[i]->setMyW(this); }

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
    for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) { setLabelArray[i]->setMyW(this); }
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

    //style Game Table
    refreshGameTableStyle();

    //raise actionLable above just inserted mypixmaplabel
    for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) { actionLabelArray[i]->raise(); }

    //raise board cards
    for (i=0; i<5; i++) { boardCardsArray[i]->raise(); }

    //fix for away string bug in righttabwidget on windows
#if (defined _WIN32) || (defined __APPLE__)
    tabWidget_Right->setTabText(0, " "+tabWidget_Right->tabText(0)+" ");
    tabWidget_Right->setTabText(1, " "+tabWidget_Right->tabText(1)+" ");
    tabWidget_Right->setTabText(2, " "+tabWidget_Right->tabText(2)+" ");
    tabWidget_Left->setTabText(0, " "+tabWidget_Left->tabText(0)+" ");
    tabWidget_Left->setTabText(1, " "+tabWidget_Left->tabText(1)+" ");
    tabWidget_Left->setTabText(2, " "+tabWidget_Left->tabText(2)+" ");
#endif

    //resize stop-button depending on translation
    QFontMetrics tempMetrics = this->fontMetrics();
    int width = tempMetrics.width(tr("Stop"));
    pushButton_break->setMinimumSize(width+10,20);

    //set inputvalidator for lineeditbetvalue
    QRegExp rx("[1-9]\\d{0,7}");
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
        //                 statusBar()->showMessage(tr("Cmd+N to start a new game"));
#else
        //                 statusBar()->showMessage(tr("Ctrl+N to start a new game"));
#endif
    }

    // 	Dialogs
    myChat = new ChatTools(lineEdit_ChatInput, myConfig, INGAME_CHAT, textBrowser_Chat);
    myChat->setMyStyle(myGameTableStyle);

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

    connect( pushButton_showMyCards, SIGNAL( clicked() ), this, SLOT( sendShowMyCardsSignal() ) );

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
    connect(this, SIGNAL(signalPostRiverShowCards(unsigned)), this, SLOT(postRiverShowCards(unsigned)));
    connect(this, SIGNAL(signalFlipHolecardsAllIn()), this, SLOT(flipHolecardsAllIn()));
    connect(this, SIGNAL(signalNextRoundCleanGui()), this, SLOT(nextRoundCleanGui()));
    connect(this, SIGNAL(signalStartVoteOnKick(unsigned, unsigned, int, int)), this, SLOT(startVoteOnKick(unsigned, unsigned, int, int)));
    connect(this, SIGNAL(signalChangeVoteOnKickButtonsState(bool)), this, SLOT(changeVoteOnKickButtonsState(bool)));
    connect(this, SIGNAL(signalEndVoteOnKick()), this, SLOT(endVoteOnKick()));

    connect(this, SIGNAL(signalNetClientPlayerLeft(unsigned)), this, SLOT(netClientPlayerLeft(unsigned)));

}

gameTableImpl::~gameTableImpl() {


}

void gameTableImpl::callSettingsDialog() { myStartWindow->callSettingsDialog(); }

void gameTableImpl::applySettings(settingsDialogImpl* mySettingsDialog) {

    //apply card deck style
    myCardDeckStyle->readStyleFile(QString::fromUtf8(myConfig->readConfigString("CurrentCardDeckStyle").c_str()));
    //apply game table style
    myGameTableStyle->readStyleFile(QString::fromUtf8(myConfig->readConfigString("CurrentGameTableStyle").c_str()));

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
        (*(++it))->setMyName(mySettingsDialog->lineEdit_Opponent7Name->text().toUtf8().constData());
        (*(++it))->setMyName(mySettingsDialog->lineEdit_Opponent8Name->text().toUtf8().constData());
        (*(++it))->setMyName(mySettingsDialog->lineEdit_Opponent9Name->text().toUtf8().constData());
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
        int i;
        GameState currentState = myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getCurrentBeRo()->getMyBeRoID();
        if(currentState >= GAME_STATE_FLOP && currentState <= GAME_STATE_POST_RIVER)
            for(i=0; i<3; i++) {
            QPixmap card = QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempBoardCardsArray[i], 10)+".png"));
            boardCardsArray[i]->setPixmap(card, FALSE);
        }
        if(currentState >= GAME_STATE_TURN && currentState <= GAME_STATE_POST_RIVER) {
            QPixmap card = QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempBoardCardsArray[3], 10)+".png"));
            boardCardsArray[3]->setPixmap(card, FALSE);
        }
        if(currentState == GAME_STATE_RIVER || currentState == GAME_STATE_POST_RIVER) {
            QPixmap card = QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempBoardCardsArray[4], 10)+".png"));
            boardCardsArray[4]->setPixmap(card, FALSE);
        }
    }

    //Flipside refresh
    if (myConfig->readConfigInt("FlipsideOwn") && myConfig->readConfigString("FlipsideOwnFile") != "") {
        flipside = QPixmap::fromImage(QImage(QString::fromUtf8(myConfig->readConfigString("FlipsideOwnFile").c_str())));
    }
    else {
        flipside = QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+"flipside.png"));
    }
    int j,k;
    for (j=1; j<MAX_NUMBER_OF_PLAYERS; j++ ) {
        for ( k=0; k<=1; k++ ) {
            if (holeCardsArray[j][k]->getIsFlipside()) {
                holeCardsArray[j][k]->setPixmap(flipside, TRUE);
            }
        }
    }

    //Check for anti-peek mode
    if(myStartWindow->getSession()->getCurrentGame()) {
        // 		check if human player is already active
        if(myStartWindow->getSession()->getCurrentGame()->getSeatsList()->front()->getMyActiveStatus()) {

            QPixmap tempCardsPixmapArray[2];
            int tempCardsIntArray[2];

            myStartWindow->getSession()->getCurrentGame()->getSeatsList()->front()->getMyCards(tempCardsIntArray);
            if(myConfig->readConfigInt("AntiPeekMode")) {
                holeCardsArray[0][0]->setPixmap(flipside, TRUE);
                tempCardsPixmapArray[0] = QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempCardsIntArray[0], 10)+".png"));
                holeCardsArray[0][0]->setHiddenFrontPixmap(tempCardsPixmapArray[0]);
                holeCardsArray[0][1]->setPixmap(flipside, TRUE);
                tempCardsPixmapArray[1]= QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempCardsIntArray[1], 10)+".png"));
                holeCardsArray[0][1]->setHiddenFrontPixmap(tempCardsPixmapArray[1]);
            }
            else {
                tempCardsPixmapArray[0]= QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempCardsIntArray[0], 10)+".png"));
                holeCardsArray[0][0]->setPixmap(tempCardsPixmapArray[0],FALSE);
                tempCardsPixmapArray[1]= QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempCardsIntArray[1], 10)+".png"));
                holeCardsArray[0][1]->setPixmap(tempCardsPixmapArray[1],FALSE);
            }
        }
    }

    refreshGameTableStyle();
    //    qDebug() << "table: " << myGameTableStyle->getStyleDescription() << myGameTableStyle->getState();
    if(this->isVisible() && myGameTableStyle->getState() != GT_STYLE_OK) myGameTableStyle->showErrorMessage();

    //blind buttons refresh
    if(myStartWindow->getSession()->getCurrentGame()) {
        refreshButton();
        refreshGroupbox();
        provideMyActions();
    }

    // Re-init audio.
    mySDLPlayer->audioDone();
    mySDLPlayer->initAudio();
}

void gameTableImpl::initGui(int speed)
{
    //kill running Singleshots!!!
    stopTimer();

    label_Pot->setText(PotString);
    label_Total->setText(TotalString+":");
    label_Sets->setText(BetsString+":");
    label_handNumber->setText(HandString+":");
    label_gameNumber->setText(GameString+":");

    groupBox_RightToolBox->setDisabled(FALSE);
    groupBox_LeftToolBox->setDisabled(FALSE);

    //show human player buttons
    for(int i=0; i<6; i++) {
        userWidgetsArray[i]->show();
    }

    //set minimum gui speed to prevent gui lags on fast inet games
    if( myStartWindow->getSession()->isNetworkClientRunning() ) { horizontalSlider_speed->setMinimum(speed); }
    else { horizontalSlider_speed->setMinimum(1); }

    //set speeds for local game and for first network game
    if( !myStartWindow->getSession()->isNetworkClientRunning() || (myStartWindow->getSession()->isNetworkClientRunning() && !myStartWindow->getSession()->getCurrentGame()) ) {
	
        guiGameSpeed = speed;
        //positioning Slider
        horizontalSlider_speed->setValue(guiGameSpeed);
        setSpeeds();
    }

    //set session for chat
    myChat->setSession(this->getSession());
}

boost::shared_ptr<Session> gameTableImpl::getSession() { assert(myStartWindow->getSession().get()); return myStartWindow->getSession(); }

//refresh-Funktionen
void gameTableImpl::refreshSet() {

    Game *currentGame = myStartWindow->getSession()->getCurrentGame();

    PlayerListConstIterator it_c;
    for (it_c=currentGame->getSeatsList()->begin(); it_c!=currentGame->getSeatsList()->end(); it_c++) {
        if( (*it_c)->getMySet() == 0 )
            setLabelArray[(*it_c)->getMyID()]->setText("");
        else
            setLabelArray[(*it_c)->getMyID()]->setText("$"+QString("%L1").arg((*it_c)->getMySet()));
    }
}

void gameTableImpl::refreshButton() {

    QPixmap dealerButton = QPixmap::fromImage(QImage(myGameTableStyle->getDealerPuck()));
    QPixmap smallblindButton = QPixmap::fromImage(QImage(myGameTableStyle->getSmallBlindPuck()));
    QPixmap bigblindButton = QPixmap::fromImage(QImage(myGameTableStyle->getBigBlindPuck()));
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

    if(myStartWindow->getSession()->getCurrentGame()) {
        HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();

        PlayerListConstIterator it_c;
        for (it_c=currentHand->getSeatsList()->begin(); it_c!=currentHand->getSeatsList()->end(); it_c++) {
            if((*it_c)->getMyActiveStatus()) {
                playerNameLabelArray[(*it_c)->getMyID()]->setText(QString::fromUtf8((*it_c)->getMyName().c_str()));

            } else {
                if((myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_INTERNET || myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_NETWORK)) {
                    if((*it_c)->getMyStayOnTableStatus() == TRUE && (*it_c)->getMyType() != PLAYER_TYPE_COMPUTER) {
                        playerNameLabelArray[(*it_c)->getMyID()]->setText(QString::fromUtf8((*it_c)->getMyName().c_str()), TRUE);
                    }
                    else {
                        playerNameLabelArray[(*it_c)->getMyID()]->setText("");
                    }
                }
                else {
                    playerNameLabelArray[(*it_c)->getMyID()]->setText("");
                }
            }
        }
    }
}

void gameTableImpl::refreshPlayerAvatar() {

    if(myStartWindow->getSession()->getCurrentGame()) {

        QPixmap onePix = QPixmap::fromImage(QImage(myAppDataPath +"gfx/gui/misc/1px.png"));

        HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();

        PlayerListConstIterator it_c;
        for (it_c=currentHand->getSeatsList()->begin(); it_c!=currentHand->getSeatsList()->end(); it_c++) {
            if((*it_c)->getMyActiveStatus()) {

		QFile myAvatarFile(QString::fromUtf8((*it_c)->getMyAvatar().c_str()));
                if((*it_c)->getMyAvatar() == "" || !myAvatarFile.exists()) {
                    playerAvatarLabelArray[(*it_c)->getMyID()]->setPixmap(QPixmap::fromImage(QImage(myGameTableStyle->getDefaultAvatar())));
                }
                else {
                    playerAvatarLabelArray[(*it_c)->getMyID()]->setPixmap(QPixmap::fromImage(QImage(QString::fromUtf8((*it_c)->getMyAvatar().c_str()))));
                }
            }
            else {
                if((myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_INTERNET || myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_NETWORK)) {
                    if((*it_c)->getMyStayOnTableStatus() == TRUE && (*it_c)->getMyType() != PLAYER_TYPE_COMPUTER) {
                	QFile myAvatarFile(QString::fromUtf8((*it_c)->getMyAvatar().c_str()));
                        if((*it_c)->getMyAvatar() == "" || !myAvatarFile.exists()) {
                            playerAvatarLabelArray[(*it_c)->getMyID()]->setPixmap(QPixmap::fromImage(QImage(myGameTableStyle->getDefaultAvatar())), TRUE);
                        }
                        else {
                            playerAvatarLabelArray[(*it_c)->getMyID()]->setPixmap(QPixmap::fromImage(QImage(QString::fromUtf8((*it_c)->getMyAvatar().c_str()))), TRUE);
                        }
                    }
                    else {
                        playerAvatarLabelArray[(*it_c)->getMyID()]->setPixmap(onePix);
                    }
                }
                else {
                    playerAvatarLabelArray[(*it_c)->getMyID()]->setPixmap(onePix);
                }
            }
        }
    }
}

void gameTableImpl::setPlayerAvatar(int myID, QString myAvatar) {

    if(myStartWindow->getSession()->getCurrentGame()) {

        boost::shared_ptr<PlayerInterface> tmpPlayer = myStartWindow->getSession()->getCurrentGame()->getPlayerByUniqueId(myID);
        if (tmpPlayer.get()) {

	    QFile myAvatarFile(myAvatar);
            if(myAvatarFile.exists()) {
                playerAvatarLabelArray[tmpPlayer->getMyID()]->setPixmap(myAvatar);
                tmpPlayer->setMyAvatar(myAvatar.toUtf8().constData());
            }
            else {
                playerAvatarLabelArray[tmpPlayer->getMyID()]->setPixmap(QPixmap::fromImage(QImage(myGameTableStyle->getDefaultAvatar())));
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
                actionLabelArray[(*it_c)->getMyID()]->setPixmap(QPixmap::fromImage(QImage(myGameTableStyle->getActionPic((*it_c)->getMyAction()))));
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
            actionLabelArray[playerID]->setPixmap(QPixmap::fromImage(QImage(myGameTableStyle->getActionPic(playerAction))));

            //play sounds if exist
            if(myConfig->readConfigInt("PlayGameActions"))
                mySDLPlayer->playSound(actionArray[playerAction].toStdString(), playerID);
        }

        if (playerAction == 1) { // FOLD

            if (playerID == 0) {
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

            cashLabelArray[(*it_c)->getMyID()]->setText("$"+QString("%L1").arg((*it_c)->getMyCash()));

        } else {
            cashLabelArray[(*it_c)->getMyID()]->setText("");
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
                        holeCardsArray[0][0]->signalFastFlipCards(FALSE);
                        holeCardsArray[0][1]->signalFastFlipCards(FALSE);
                    }
                    myGameTableStyle->setPlayerSeatInactiveStyle(groupBoxArray[(*it_c)->getMyID()]);
                }
            }
        }
    }
    else {
        switch(status) {

            //inactive
        case 0: {
                if (!playerID) {
                    //hide buttons
                    for(j=0; j<6; j++) {
                        userWidgetsArray[j]->hide();
                    }
                    //disable anti-peek front after player is out
                    holeCardsArray[0][0]->signalFastFlipCards(FALSE);
                    holeCardsArray[0][1]->signalFastFlipCards(FALSE);
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
        default: {}
        }
    }
}

void gameTableImpl::refreshGameLabels(int gameState) { 

    switch(gameState) {
    case 0: {
            textLabel_handLabel->setText(PreflopString);
        } break;
    case 1: {
            textLabel_handLabel->setText(FlopString);
        } break;
    case 2: {
            textLabel_handLabel->setText(TurnString);
        } break;
    case 3: {
            textLabel_handLabel->setText(RiverString);
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

    textLabel_Sets->setText("$"+QString("%L1").arg(currentHand->getBoard()->getSets()));
    textLabel_Pot->setText("$"+QString("%L1").arg(currentHand->getBoard()->getPot()));
}

void gameTableImpl::guiUpdateDone() {
    guiUpdateSemaphore.release();
}

void gameTableImpl::waitForGuiUpdateDone() {
    guiUpdateSemaphore.acquire();
}

void gameTableImpl::dealHoleCards() {

    for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++ ) {
        for ( j=0; j<=1; j++ ) {
            holeCardsArray[i][j]->setFadeOutAction(FALSE);
            holeCardsArray[i][j]->stopFlipCardsAnimation();
        }
    }

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
                    tempCardsPixmapArray[j].load(myCardDeckStyle->getCurrentDir()+QString::number(tempCardsIntArray[j], 10)+".png");
                    if(myConfig->readConfigInt("AntiPeekMode")) {
                        holeCardsArray[(*it_c)->getMyID()][j]->setPixmap(flipside, TRUE);
                        holeCardsArray[(*it_c)->getMyID()][j]->setFront(flipside);
                        holeCardsArray[(*it_c)->getMyID()][j]->setHiddenFrontPixmap(tempCardsPixmapArray[j]);
                    }
                    else {
                        holeCardsArray[(*it_c)->getMyID()][j]->setPixmap(tempCardsPixmapArray[j],FALSE);
                        holeCardsArray[(*it_c)->getMyID()][j]->setFront(tempCardsPixmapArray[j]);
                    }
                }
                else {
                    holeCardsArray[(*it_c)->getMyID()][j]->setPixmap(flipside, TRUE);
                    holeCardsArray[(*it_c)->getMyID()][j]->setFlipsidePix(flipside);
                }
            }
            else {

                holeCardsArray[(*it_c)->getMyID()][j]->setPixmap(onePix, FALSE);
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

    boardCardsArray[0]->setPixmap(flipside, TRUE);
    dealFlopCards1Timer->start(dealCardsSpeed);
}

void gameTableImpl::dealFlopCards2() {

    boardCardsArray[1]->setPixmap(flipside, TRUE);
    dealFlopCards2Timer->start(dealCardsSpeed);
}

void gameTableImpl::dealFlopCards3() {

    boardCardsArray[2]->setPixmap(flipside, TRUE);
    dealFlopCards3Timer->start(dealCardsSpeed);
}

void gameTableImpl::dealFlopCards4() {

    int tempBoardCardsArray[5];

    myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getBoard()->getMyCards(tempBoardCardsArray);
    QPixmap card = QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempBoardCardsArray[0], 10)+".png"));

    //Config? mit oder ohne Eye-Candy?
    if(myConfig->readConfigInt("ShowFlipCardsAnimation")) {
        //with Eye-Candy
        boardCardsArray[0]->startFlipCards(guiGameSpeed, card, flipside);
    }
    else {
        //without Eye-Candy
        boardCardsArray[0]->setFront(card);
        boardCardsArray[0]->setPixmap(card, FALSE);
    }
    dealFlopCards4Timer->start(dealCardsSpeed);
}

void gameTableImpl::dealFlopCards5() {

    int tempBoardCardsArray[5];
    myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getBoard()->getMyCards(tempBoardCardsArray);
    QPixmap card = QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempBoardCardsArray[1], 10)+".png"));

    //Config? mit oder ohne Eye-Candy?
    if(myConfig->readConfigInt("ShowFlipCardsAnimation")) {
        //with Eye-Candy
        boardCardsArray[1]->startFlipCards(guiGameSpeed, card, flipside);
    }
    else {
        //without Eye-Candy
        boardCardsArray[1]->setFront(card);
        boardCardsArray[1]->setPixmap(card, FALSE);
    }
    dealFlopCards5Timer->start(dealCardsSpeed);
}

void gameTableImpl::dealFlopCards6() {

    int tempBoardCardsArray[5];
    myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getBoard()->getMyCards(tempBoardCardsArray);
    QPixmap card = QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempBoardCardsArray[2], 10)+".png"));

    //Config? mit oder ohne Eye-Candy?
    if(myConfig->readConfigInt("ShowFlipCardsAnimation")) {
        //with Eye-Candy
        boardCardsArray[2]->startFlipCards(guiGameSpeed, card, flipside);
    }
    else {
        //without Eye-Candy
        boardCardsArray[2]->setFront(card);
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

    boardCardsArray[3]->setPixmap(flipside, TRUE);
    dealTurnCards1Timer->start(dealCardsSpeed);
}

void gameTableImpl::dealTurnCards2() {

    int tempBoardCardsArray[5];
    myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getBoard()->getMyCards(tempBoardCardsArray);
    QPixmap card = QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempBoardCardsArray[3], 10)+".png"));

    //Config? mit oder ohne Eye-Candy?
    if(myConfig->readConfigInt("ShowFlipCardsAnimation")) {
        //with Eye-Candy
        boardCardsArray[3]->startFlipCards(guiGameSpeed, card, flipside);
    }
    else {
        //without Eye-Candy
        boardCardsArray[3]->setFront(card);
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

    boardCardsArray[4]->setPixmap(flipside, TRUE);

    // 	QTimer::singleShot(dealCardsSpeed, this, SLOT( dealRiverCards2() ));
    dealRiverCards1Timer->start(dealCardsSpeed);
}

void gameTableImpl::dealRiverCards2() {

    int tempBoardCardsArray[5];
    myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getBoard()->getMyCards(tempBoardCardsArray);
    QPixmap card = QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempBoardCardsArray[4], 10)+".png"));

    //Config? mit oder ohne Eye-Candy?
    if(myConfig->readConfigInt("ShowFlipCardsAnimation")) {
        //with Eye-Candy
        boardCardsArray[4]->startFlipCards(guiGameSpeed, card, flipside);
    }
    else {
        //without Eye-Candy
        boardCardsArray[4]->setFront(card);
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
    QString pushButtonAllInString;
    QString lastPushButtonCallCheckString = pushButton_CallCheck->text();


    Game *currentGame = myStartWindow->getSession()->getCurrentGame();
    HandInterface *currentHand = currentGame->getCurrentHand();

    //really disabled buttons if human player is fold/all-in or ... and not called from dealberocards
    if(/*pushButton_BetRaise->isCheckable() && */mode != 0 && (currentHand->getSeatsList()->front()->getMyAction() == PLAYER_ACTION_ALLIN || currentHand->getSeatsList()->front()->getMyAction() == PLAYER_ACTION_FOLD || (currentGame->getSeatsList()->front()->getMySet() == currentHand->getCurrentBeRo()->getHighestSet() && (currentGame->getSeatsList()->front()->getMyAction() != PLAYER_ACTION_NONE)))) {
	
        pushButton_BetRaise->setText("");
        pushButton_CallCheck->setText("");
        pushButton_Fold->setText("");
        pushButton_AllIn->setText("");

        horizontalSlider_bet->setDisabled(TRUE);
        lineEdit_betValue->setDisabled(TRUE);

        myButtonsCheckable(FALSE);

        refreshActionButtonFKeyIndicator(1);
    }
    else {
        horizontalSlider_bet->setEnabled(TRUE);
        lineEdit_betValue->setEnabled(TRUE);

        //show available actions on buttons
        if(currentHand->getCurrentRound() == 0) { // preflop

            if (currentGame->getSeatsList()->front()->getMyCash()+currentGame->getSeatsList()->front()->getMySet() > currentHand->getCurrentBeRo()->getHighestSet() && !currentHand->getCurrentBeRo()->getFullBetRule()) {
                pushButtonBetRaiseString = RaiseString+"\n$"+QString("%L1").arg(getMyBetAmount());
            }

            if (currentGame->getSeatsList()->front()->getMySet()== currentHand->getCurrentBeRo()->getHighestSet() &&  currentGame->getSeatsList()->front()->getMyButton() == 3) { pushButtonCallCheckString = CheckString; }
            else { pushButtonCallCheckString = CallString+"\n$"+QString("%L1").arg(getMyCallAmount()); }

            pushButtonFoldString = FoldString;
            if(!currentHand->getCurrentBeRo()->getFullBetRule()) {
                pushButtonAllInString = AllInString;
            }
        }
        else { // flop,turn,river

            if (currentHand->getCurrentBeRo()->getHighestSet() == 0 && pushButton_Fold->isCheckable() ) {
                pushButtonFoldString = CheckString+" /\n"+FoldString;
            }
            else { pushButtonFoldString = FoldString; }
            if (currentHand->getCurrentBeRo()->getHighestSet() == 0) {

                pushButtonCallCheckString = CheckString;
                pushButtonBetRaiseString = BetString+"\n$"+QString("%L1").arg(getMyBetAmount());
            }
            if (currentHand->getCurrentBeRo()->getHighestSet() > 0 && currentHand->getCurrentBeRo()->getHighestSet() > currentGame->getSeatsList()->front()->getMySet()) {
                pushButtonCallCheckString = CallString+"\n$"+QString("%L1").arg(getMyCallAmount());
                if (currentGame->getSeatsList()->front()->getMyCash()+currentGame->getSeatsList()->front()->getMySet() > currentHand->getCurrentBeRo()->getHighestSet() && !currentHand->getCurrentBeRo()->getFullBetRule()) {
                    pushButtonBetRaiseString = RaiseString+"\n$"+QString("%L1").arg(getMyBetAmount());
                }
            }
            if(!currentHand->getCurrentBeRo()->getFullBetRule()) {
                pushButtonAllInString = AllInString;
            }
        }

        if(mode == 0) {
            if( currentHand->getSeatsList()->front()->getMyAction() != PLAYER_ACTION_FOLD ) {
                pushButtonBetRaiseString = BetString+"\n$"+QString("%L1").arg(getMyBetAmount());
                pushButtonCallCheckString = CheckString;
                if( (currentGame->getActivePlayerList()->size() > 2 && currentGame->getSeatsList()->front()->getMyButton() == BUTTON_SMALL_BLIND ) || ( currentGame->getActivePlayerList()->size() <= 2 && currentGame->getSeatsList()->front()->getMyButton() == BUTTON_BIG_BLIND)) { pushButtonFoldString = FoldString; }
                else { pushButtonFoldString = CheckString+" /\n"+FoldString; }

                pushButtonAllInString = AllInString;
            }
            else {
                pushButtonBetRaiseString = "";
                pushButtonCallCheckString = "";
                pushButtonFoldString = "";
                pushButtonAllInString = "";
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

            horizontalSlider_bet->setDisabled(TRUE);
            lineEdit_betValue->setDisabled(TRUE);
        }

        pushButton_Fold->setText(pushButtonFoldString);
        pushButton_BetRaise->setText(pushButtonBetRaiseString);
        pushButton_CallCheck->setText(pushButtonCallCheckString);
        pushButton_AllIn->setText(pushButtonAllInString);

        refreshActionButtonFKeyIndicator();
        // 		myBetRaise();

        if(pushButton_BetRaise->text().startsWith(RaiseString)) {

            horizontalSlider_bet->setMinimum(currentHand->getCurrentBeRo()->getHighestSet() - currentHand->getSeatsList()->front()->getMySet() + currentHand->getCurrentBeRo()->getMinimumRaise());
            horizontalSlider_bet->setMaximum(currentHand->getSeatsList()->front()->getMyCash());
            horizontalSlider_bet->setSingleStep(10);
            changeLineEditBetValue(horizontalSlider_bet->value());

            myActionIsRaise = 1;
        }
        else if(pushButton_BetRaise->text().startsWith(BetString)) {

            horizontalSlider_bet->setMinimum(currentHand->getSmallBlind()*2);
            horizontalSlider_bet->setMaximum(currentHand->getSeatsList()->front()->getMyCash());
            horizontalSlider_bet->setSingleStep(10);
            changeLineEditBetValue(horizontalSlider_bet->value());

            myActionIsBet = 1;
        }
        else {}


        //if value changed on bet/raise button --> uncheck to prevent unwanted actions
        int lastBetValue = lastPushButtonBetRaiseString.simplified().remove(QRegExp("[^0-9]")).toInt();

        if((lastBetValue < horizontalSlider_bet->minimum() && pushButton_BetRaise->isChecked())) {

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

    if(myConfig->readConfigInt("ShowStatusbarMessages")) {
        if ( myConfig->readConfigInt("AlternateFKeysUserActionMode") == 0 ) {
            // // 			statusBar()->showMessage(tr("F1 - Fold | F2 - Check/Call | F3 - Bet/Raise | F4 - All-In"), 15000);
        } else {
            // 			statusBar()->showMessage(tr("F1 - All-In | F2 - Bet/Raise | F3 - Check/Call | F4 - Fold"), 15000);
        }
    }

    QString lastPushButtonFoldString = pushButton_Fold->text();

    //paint actions on buttons
    provideMyActions();

    //do remembered action
    if( pushButtonBetRaiseIsChecked ) { pushButton_BetRaise->click(); pushButtonBetRaiseIsChecked = FALSE;}
    if( pushButtonCallCheckIsChecked )  { pushButton_CallCheck->click(); pushButtonCallCheckIsChecked = FALSE;}
    if( pushButtonFoldIsChecked ) {
        if(lastPushButtonFoldString == CheckString+" /\n"+FoldString && pushButton_CallCheck->text() == CheckString) {
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
        if (pushButton_CallCheck->text() == CheckString) {
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
    if(playerId) { timeoutLabelArray[playerId]->startTimeOutAnimation(timeoutSec, FALSE); }
    else { timeoutLabelArray[playerId]->startTimeOutAnimation(timeoutSec, TRUE); }
}

void gameTableImpl::stopTimeoutAnimation(int playerId) {
    assert(playerId >= 0 && playerId < myStartWindow->getSession()->getCurrentGame()->getStartQuantityPlayers());
    timeoutLabelArray[playerId]->stopTimeOutAnimation();
}

void gameTableImpl::disableMyButtons() {

    HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();

    clearMyButtons();

    //clear userWidgets
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

    if(pushButton_CallCheck->text().startsWith(CallString)) { myCall(); }
    if(pushButton_CallCheck->text() == CheckString) { myCheck(); }
}

void gameTableImpl::myFold(){

    if(pushButton_Fold->text() == FoldString) {

        HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();
        currentHand->getSeatsList()->front()->setMyAction(1);
        currentHand->getSeatsList()->front()->setMyTurn(0);
	
        //set that i was the last active player. need this for unhighlighting groupbox
        currentHand->setLastPlayersTurn(0);

        // 		statusBar()->clearMessage();
	
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

    // 	statusBar()->clearMessage();

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

    int betValue = pushButton_BetRaise->text().simplified().remove(QRegExp("[^0-9]")).toInt();
    return betValue;
}

int gameTableImpl::getMyBetAmount() {

    HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();

    int betValue = getBetRaisePushButtonValue();
    int minimum;

    minimum = currentHand->getCurrentBeRo()->getHighestSet() - currentHand->getSeatsList()->front()->getMySet() + currentHand->getCurrentBeRo()->getMinimumRaise();

    if(betValue < minimum) {
        return min(minimum,currentHand->getSeatsList()->front()->getMyCash());
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

    // 	statusBar()->clearMessage();

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

            // full bet rule
            if(currentHand->getCurrentBeRo()->getHighestSet() + currentHand->getCurrentBeRo()->getMinimumRaise() > currentHand->getSeatsList()->front()->getMySet()) {
                currentHand->getCurrentBeRo()->setFullBetRule(true);
            }
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
	
        // 		statusBar()->clearMessage();
	
        //set that i was the last active player. need this for unhighlighting groupbox
        currentHand->setLastPlayersTurn(0);

        // lastPlayerAction für Karten umblättern reihenfolge setzrn
        currentHand->setLastActionPlayer(currentHand->getSeatsList()->front()->getMyUniqueID());
	
        //Spiel läuft weiter
        myActionDone();
    }
}

void gameTableImpl::myAllIn(){

    if(pushButton_AllIn->text() == AllInString) {

        HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();

        currentHand->getSeatsList()->front()->setMySet(currentHand->getSeatsList()->front()->getMyCash());
        currentHand->getSeatsList()->front()->setMyCash(0);
        currentHand->getSeatsList()->front()->setMyAction(6);

        // full bet rule
        if(currentHand->getCurrentBeRo()->getHighestSet() + currentHand->getCurrentBeRo()->getMinimumRaise() > currentHand->getSeatsList()->front()->getMySet()) {
            currentHand->getCurrentBeRo()->setFullBetRule(true);
        }

        if(currentHand->getSeatsList()->front()->getMySet() > currentHand->getCurrentBeRo()->getHighestSet()) {
            currentHand->getCurrentBeRo()->setMinimumRaise(currentHand->getSeatsList()->front()->getMySet() - currentHand->getCurrentBeRo()->getHighestSet());

            currentHand->getCurrentBeRo()->setHighestSet(currentHand->getSeatsList()->front()->getMySet());

            // lastPlayerAction für Karten umblättern reihenfolge setzrn
            currentHand->setLastActionPlayer(currentHand->getSeatsList()->front()->getMyUniqueID());

        }

        currentHand->getSeatsList()->front()->setMyTurn(0);

        currentHand->getBoard()->collectSets();
        refreshPot();

        //set that i was the last active player. need this for unhighlighting groupbox
        currentHand->setLastPlayersTurn(0);

        //Spiel läuft weiter
        myActionDone();
    }
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

            QPixmap onePix = QPixmap::fromImage(QImage(myAppDataPath +"gfx/gui/misc/1px.png"));
            //TempArrays
            QPixmap tempCardsPixmapArray[2];
            int tempCardsIntArray[2];
            int showFlipcardAnimation = myConfig->readConfigInt("ShowFlipCardsAnimation");
            int antiPeekMode = myConfig->readConfigInt("AntiPeekMode");

            int j;
            for (it_c=currentHand->getActivePlayerList()->begin(); it_c!=currentHand->getActivePlayerList()->end(); it_c++) {
                (*it_c)->getMyCards(tempCardsIntArray);
                if((*it_c)->getMyAction() != PLAYER_ACTION_FOLD && (*it_c)->checkIfINeedToShowCards()) {

                    //                    if((*it_c)->getMyID() || ((*it_c)->getMyID()==0 && antiPeekMode) ) {
                    for(j=0; j<2; j++) {
                        if(showFlipcardAnimation) { // with Eye-Candy
                            holeCardsArray[(*it_c)->getMyID()][j]->startFlipCards(guiGameSpeed, QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempCardsIntArray[j], 10)+".png")), flipside);
                        }
                        else { //without Eye-Candy
                            tempCardsPixmapArray[j] = QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempCardsIntArray[j], 10)+".png"));
                            holeCardsArray[(*it_c)->getMyID()][j]->setFront(tempCardsPixmapArray[j]);
                            holeCardsArray[(*it_c)->getMyID()][j]->setPixmap(tempCardsPixmapArray[j], FALSE);
                        }
                    }

                    //                    }
                    //set Player value (logging)
                    (*it_c)->setMyCardsFlip(1,1);
                }

                //if human player dont need to show cards he gets the button "show cards" in internet or network game
                if( (myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_INTERNET || myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_NETWORK) && (*it_c)->getMyID() == 0 && (*it_c)->getMyAction() != PLAYER_ACTION_FOLD && !(*it_c)->checkIfINeedToShowCards()) {

                    showShowMyCardsButton();
                }
                //Wenn einmal umgedreht dann fertig!!
                flipHolecardsAllInAlreadyDone = TRUE;
            }
        }
        else {
            int tempCardsIntArray[2];
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

void gameTableImpl::postRiverRunAnimation3() {

    HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();

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

            //Show "Winner" label
            actionLabelArray[(*it_c)->getMyID()]->setPixmap(QPixmap::fromImage(QImage(myGameTableStyle->getActionPic(7))));

            //show winnercards if more than one player is active TODO
            if ( nonfoldPlayerCounter != 1 && myConfig->readConfigInt("ShowFadeOutCardsAnimation")) {

                int j;
                int bestHandPos[5];
                (*it_c)->getMyBestHandPosition(bestHandPos);

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

            if( currentHand->getActivePlayerList()->size() != 1 && (*it_c)->getMyAction() != PLAYER_ACTION_FOLD && myConfig->readConfigInt("ShowFadeOutCardsAnimation")
                ) {

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

                    playerNameLabelArray[(*it_c)->getMyID()]->setText("");
                }
            }
        }
        else {
            label_Pot->setText(PotString);

            for(it_c=currentHand->getActivePlayerList()->begin(); it_c!=currentHand->getActivePlayerList()->end(); it_c++) {
                if((*it_c)->getMyAction() != PLAYER_ACTION_FOLD && (*it_c)->getMyCardsValueInt() == currentHand->getCurrentBeRo()->getHighestCardsValue() ) {

                    playerNameLabelArray[(*it_c)->getMyID()]->setText(QString::fromUtf8((*it_c)->getMyName().c_str()));
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

void gameTableImpl::postRiverShowCards(unsigned playerId)
{
    HandInterface *currentHand = myStartWindow->getSession()->getCurrentGame()->getCurrentHand();
    QPixmap onePix = QPixmap::fromImage(QImage(myAppDataPath +"gfx/gui/misc/1px.png"));
    //TempArrays
    QPixmap tempCardsPixmapArray[2];
    int tempCardsIntArray[2];
    int showFlipcardAnimation = myConfig->readConfigInt("ShowFlipCardsAnimation");
    int j;
    PlayerListConstIterator it_c;
    for (it_c=currentHand->getActivePlayerList()->begin(); it_c!=currentHand->getActivePlayerList()->end(); it_c++) {
        if((*it_c)->getMyUniqueID() == playerId) {

            (*it_c)->getMyCards(tempCardsIntArray);
            for(j=0; j<2; j++) {

                if(showFlipcardAnimation) { // with Eye-Candy
                    holeCardsArray[(*it_c)->getMyID()][j]->startFlipCards(guiGameSpeed, QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempCardsIntArray[j], 10)+".png")), flipside);
                }
                else { //without Eye-Candy
                    tempCardsPixmapArray[j] = QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempCardsIntArray[j], 10)+".png"));
                    holeCardsArray[(*it_c)->getMyID()][j]->setPixmap(tempCardsPixmapArray[j], FALSE);
                }
            }
        }
        //set Player value (logging)
        (*it_c)->setMyCardsFlip(1,1);
    }
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
                        //                        if((*it_c)->getMyID() || ((*it_c)->getMyID()==0 && myConfig->readConfigInt("AntiPeekMode")) ) {
                        for(j=0; j<2; j++) {
                            holeCardsArray[(*it_c)->getMyID()][j]->startFlipCards(guiGameSpeed, QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempCardsIntArray[j], 10)+".png")), flipside);
                        }
                        //                        }
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
                        //                        if((*it_c)->getMyID() || ((*it_c)->getMyID()==0 && myConfig->readConfigInt("AntiPeekMode")) ) {
                        for(j=0; j<2; j++) {

                            tempCardsPixmapArray[j] = QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(temp2CardsIntArray[j], 10)+".png"));
                            holeCardsArray[(*it_c)->getMyID()][j]->setFront(tempCardsPixmapArray[j]);
                            holeCardsArray[(*it_c)->getMyID()][j]->setPixmap(tempCardsPixmapArray[j], FALSE);
                        }
                        //                        }
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

    if( currentHand->getSeatsList()->front()->getMyCardsFlip() == 0 &&  currentHand->getCurrentRound() == 3 && currentHand->getSeatsList()->front()->getMyActiveStatus() && currentHand->getSeatsList()->front()->getMyAction() != PLAYER_ACTION_FOLD) {

        myStartWindow->getSession()->showMyCards();

        if(myConfig->readConfigInt("ShowFlipCardsAnimation")) { // with Eye-Candy
            holeCardsArray[0][0]->startFlipCards(guiGameSpeed, QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempCardsIntArray[0], 10)+".png")), flipside);
            holeCardsArray[0][1]->startFlipCards(guiGameSpeed, QPixmap::fromImage(QImage(myCardDeckStyle->getCurrentDir()+QString::number(tempCardsIntArray[1], 10)+".png")), flipside);
        }
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

    }
    for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++ ) {
        timeoutLabelArray[i]->stopTimeOutAnimation();
        for ( j=0; j<=1; j++ ) {
            holeCardsArray[i][j]->setFadeOutAction(FALSE);
            holeCardsArray[i][j]->stopFlipCardsAnimation();
        }
    }

    // for startNewGame during human player is active
    if(myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getSeatsList()->front()->getMyActiveStatus() == 1) {
        disableMyButtons();
    }

    textLabel_handLabel->setText("");

    refreshAll();

    flipHolecardsAllInAlreadyDone = FALSE;

    //Wenn Pause zwischen den Hands in der Konfiguration steht den Stop Button drücken!
    if (myConfig->readConfigInt("PauseBetweenHands") /*&& blinkingStartButtonAnimationTimer->isActive() == FALSE*/ && myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_LOCAL) {
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
        myGameTableStyle->setBreakButtonStyle(pushButton_break,0);
        blinkingStartButtonAnimationTimer->stop();
        QFontMetrics tempMetrics = this->fontMetrics();
        int width = tempMetrics.width(tr("Stop"));
        pushButton_break->setMinimumSize(width+10,20);
        pushButton_break->setText(tr("Stop"));
    }
    //Clear Statusbarmessage
    // 	statusBar()->clearMessage();

    //fix press mouse button during bankrupt with anti-peek-mode
    this->mouseOverFlipCards(FALSE);

    horizontalSlider_bet->setDisabled(TRUE);
    lineEdit_betValue->setDisabled(TRUE);

    uncheckMyButtons();
    myButtonsCheckable(FALSE);
    resetMyButtonsCheckStateMemory();
    clearMyButtons();
    pushButton_showMyCards->hide();
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
        myGameTableStyle->setBreakButtonStyle(pushButton_break,0);
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
    if (event->key() == Qt::Key_F5) { pushButton_showMyCards->click(); }

    if (event->key() == Qt::Key_F6) { radioButton_manualAction->click(); }
    if (event->key() == Qt::Key_F7) { radioButton_autoCheckFold->click(); }
    if (event->key() == Qt::Key_F8) { radioButton_autoCheckCallAny->click(); }
    if (event->key() == Qt::Key_Shift) {
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
    if (event->key() == Qt::Key_M) { showShowMyCardsButton(); }
}

void gameTableImpl::changePlayingMode() {

    int mode = -1;

    if(radioButton_manualAction->isChecked()) { mode=0; }
    if(radioButton_autoCheckFold->isChecked()) { mode=2; }
    if(radioButton_autoCheckCallAny->isChecked()) { mode=1; }


    switch (mode) {

        // 		case 0: { statusBar()->showMessage(tr("Manual mode set. You've got to choose yourself now."), 5000); }
        break;
        // 		case 1: { statusBar()->showMessage(tr("Auto mode set: Check or call any."), 5000); }
        break;
        // 		case 2: { statusBar()->showMessage(tr("Auto mode set: Check or fold."), 5000); }
        break;
    default: { /*cout << "changePlayingMode ERROR!!!!" << endl;*/ }

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

    if(style.contains("QPushButton:enabled { background-color: #"+myGameTableStyle->getBreakLobbyButtonBgColor())) {
        myGameTableStyle->setBreakButtonStyle(pushButton_break,1);
    }
    else {
        myGameTableStyle->setBreakButtonStyle(pushButton_break,0);
    }
}

void gameTableImpl::sendChatMessage() { myChat->sendMessage(); }
void gameTableImpl::checkChatInputLength(QString string) { myChat->checkInputLength(string); }


void gameTableImpl::tabSwitchAction() { 

    switch(tabWidget_Left->currentIndex()) {

    case 1: { lineEdit_ChatInput->setFocus(); }
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
        timeoutLabelArray[i]->stopTimeOutAnimation();
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

    //restore saved windows geometry
    restoreGameTableGeometry();

    if(myGameTableStyle->getState() != GT_STYLE_OK) myGameTableStyle->showErrorMessage();
}

void gameTableImpl::networkGameModification() {

    if(tabWidget_Left->widget(1) != tab_Chat)
        tabWidget_Left->insertTab(1, tab_Chat, QString(tr("Chat"))); /*TODO text abgeschnitten --> stylesheets*/

    tabWidget_Left->removeTab(2);

    tabWidget_Left->setCurrentIndex(1);
    myChat->clearChat();

    int i;
    for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++ ) {
        playerAvatarLabelArray[i]->setEnabledContextMenu(TRUE);
        playerAvatarLabelArray[i]->setVoteOnKickContextMenuEnabled(TRUE);
        playerAvatarLabelArray[i]->setVoteRunning(FALSE);
    }

    if(myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_INTERNET) {
        pushButton_break->show();
        QFontMetrics tempMetrics = this->fontMetrics();
        int width = tempMetrics.width(tr("Lobby"));
        pushButton_break->setText(tr("Lobby"));
        pushButton_break->setMinimumSize(width+10,20);
        myGameTableStyle->setBreakButtonStyle(pushButton_break,0);
        blinkingStartButtonAnimationTimer->stop();
    }
    if(myStartWindow->getSession()->getGameType() == Session::GAME_TYPE_NETWORK) {

        pushButton_break->hide();
    }
    //Set the playing mode to "manual"
    radioButton_manualAction->click();

    //clear log
    textBrowser_Log->clear();

    //restore saved windows geometry
    restoreGameTableGeometry();

    if(myStartWindow->getSession()->getClientPlayerInfo(myStartWindow->getSession()->getClientUniquePlayerId()).isGuest) {
        guestUserMode();
    }
    else { registeredUserMode(); }

    blinkingStartButtonAnimationTimer->stop();
    myGameTableStyle->setBreakButtonStyle(pushButton_break,0);

}

void gameTableImpl::mouseOverFlipCards(bool front) {

    if(myStartWindow->getSession()->getCurrentGame()) {
        if(myConfig->readConfigInt("AntiPeekMode") && myStartWindow->getSession()->getCurrentGame()->getCurrentHand()->getSeatsList()->front()->getMyActiveStatus()/* && myStartWindow->getSession()->getCurrentGame()->getSeatsList()->front()->getMyAction() != PLAYER_ACTION_FOLD*/){
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

    refreshActionButtonFKeyIndicator(1);

    pushButton_BetRaise->setText("");
    pushButton_CallCheck->setText("");
    pushButton_Fold->setText("");
    pushButton_AllIn->setText("");
}

void gameTableImpl::myButtonsCheckable(bool state) {

    Game *currentGame = myStartWindow->getSession()->getCurrentGame();
    HandInterface *currentHand = currentGame->getCurrentHand();

    if(state) {
        //checkable

        // exception: full bet rule
        if(!currentHand->getCurrentBeRo()->getFullBetRule()) {
            pushButton_BetRaise->setCheckable(TRUE);
        }
        pushButton_CallCheck->setCheckable(TRUE);
        pushButton_Fold->setCheckable(TRUE);
        pushButton_AllIn->setCheckable(TRUE);

        //design
        myGameTableStyle->setButtonsStyle(pushButton_BetRaise, pushButton_CallCheck, pushButton_Fold, pushButton_AllIn, 2);

        myButtonsAreCheckable = TRUE;
    }
    else {
        //not checkable

        pushButton_BetRaise->setCheckable(FALSE);
        pushButton_CallCheck->setCheckable(FALSE);
        pushButton_Fold->setCheckable(FALSE);
        pushButton_AllIn->setCheckable(FALSE);
	
        QString hover;
        if(pushButton_AllIn->text()==AllInString) {
            myGameTableStyle->setButtonsStyle(pushButton_BetRaise, pushButton_CallCheck, pushButton_Fold, pushButton_AllIn, 0);
        }
        else {
            myGameTableStyle->setButtonsStyle(pushButton_BetRaise, pushButton_CallCheck, pushButton_Fold, pushButton_AllIn, 1);
        }

        myButtonsAreCheckable = FALSE;
    }
}

void gameTableImpl::showMaximized () {
    this->showFullScreen ();
}

void gameTableImpl::closeGameTable() {

    if (myStartWindow->getMyServerGuiInterface() && myStartWindow->getMyServerGuiInterface()->getSession()->isNetworkServerRunning()) {

        QMessageBox msgBox(QMessageBox::Warning, tr("Closing PokerTH during network game"),
                           tr("You are the hosting server. Do you want to close PokerTH anyway?"), QMessageBox::Yes | QMessageBox::No, this);

        if (msgBox.exec() == QMessageBox::Yes ) {
            myStartWindow->getSession()->terminateNetworkClient();
            stopTimer();
            if (myStartWindow->getMyServerGuiInterface()) myStartWindow->getMyServerGuiInterface()->getSession()->terminateNetworkServer();
            saveGameTableGeometry();
            myStartWindow->show();
            this->hide();
        }
    }
    else {
        myStartWindow->getSession()->terminateNetworkClient();
        stopTimer();
        saveGameTableGeometry();
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
                lineEdit_betValue->setText(QString("%L1").arg(horizontalSlider_bet->minimum()));
            else
                lineEdit_betValue->setText(QString("%L1").arg(temp));
        }
        else if(horizontalSlider_bet->maximum() > 1000 && horizontalSlider_bet->maximum() <= 10000) {
            temp = (int)((value/50)*50);
            if(temp < horizontalSlider_bet->minimum())
                lineEdit_betValue->setText(QString("%L1").arg(horizontalSlider_bet->minimum()));
            else
                lineEdit_betValue->setText(QString("%L1").arg(temp));
        }
        else if(horizontalSlider_bet->maximum() > 10000 && horizontalSlider_bet->maximum() <= 100000) {
            temp = (int)((value/500)*500);
            if(temp < horizontalSlider_bet->minimum())
                lineEdit_betValue->setText(QString("%L1").arg(horizontalSlider_bet->minimum()));
            else
                lineEdit_betValue->setText(QString("%L1").arg(temp));
        }
        else {
            temp = (int)((value/5000)*5000);
            if(temp < horizontalSlider_bet->minimum())
                lineEdit_betValue->setText(QString("%L1").arg(horizontalSlider_bet->minimum()));
            else
                lineEdit_betValue->setText(QString("%L1").arg(temp));
        }
    }
}

void gameTableImpl::lineEditBetValueChanged(QString valueString) {

    if(horizontalSlider_bet->isEnabled()) {

        bool ok;
        int betValue = QString(valueString.remove(QRegExp("[^0-9]"))).toInt(&ok, 10);
        QString betRaise = pushButton_BetRaise->text().section("\n",0 ,0);
	
        if(betValue >= horizontalSlider_bet->minimum()) {

            if(betValue > horizontalSlider_bet->maximum()) { // print the maximum
                pushButton_BetRaise->setText(betRaise + "\n$" + QString("%L1").arg(horizontalSlider_bet->maximum()));
                betSliderChangedByInput = TRUE;
                horizontalSlider_bet->setValue(horizontalSlider_bet->maximum());
            }
            else { // really print the value
                pushButton_BetRaise->setText(betRaise + "\n$" + QString("%L1").arg(valueString.toInt()));
                betSliderChangedByInput = TRUE;
                horizontalSlider_bet->setValue(betValue);
            }
        }
        else { // print the minimum
            pushButton_BetRaise->setText(betRaise + "\n$" + QString("%L1").arg(horizontalSlider_bet->minimum()));
            betSliderChangedByInput = TRUE;
            horizontalSlider_bet->setValue(horizontalSlider_bet->minimum());
        }
    }
}

void gameTableImpl::leaveCurrentNetworkGame() {

    if (myStartWindow->getSession()->isNetworkClientRunning()) {

        myMessageDialogImpl dialog(myConfig, this);

        if(!dialog.checkIfMesssageWillBeDisplayed(1)) {

            assert(myStartWindow->getSession());
            myStartWindow->getSession()->sendLeaveCurrentGame();
        }
        else {
            if (dialog.exec(1, tr("Attention! Do you really want to leave the current game\nand go back to the lobby?"), tr("PokerTH - Internet Game Message"), QPixmap(":/gfx/logoChip3D.png"), QDialogButtonBox::Yes|QDialogButtonBox::No, true) == QDialog::Accepted) {
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
        label_votesMonitor->setText(tr("Player <b>%1</b> has <b>%2</b> %3<br>against him. <b>%4</b> vote(s) needed to kick.").arg(QString::fromUtf8(info.playerName.c_str())).arg(currentVotes).arg(currentVotesString).arg(numVotesNeededToKick-currentVotes));
    }
    else {
        label_votesMonitor->setText(tr("You have <b>%1</b> %2 against you.<br><b>%3</b> vote(s) needed to kick.").arg(currentVotes).arg(currentVotesString).arg(numVotesNeededToKick-currentVotes));
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

void gameTableImpl::refreshActionButtonFKeyIndicator(bool clear)
{
    if(clear) {
        pushButton_AllIn->setFKeyText("");
        pushButton_BetRaise->setFKeyText("");
        pushButton_CallCheck->setFKeyText("");
        pushButton_Fold->setFKeyText("");
    }
    else {
        if(myConfig->readConfigInt("AlternateFKeysUserActionMode") == 0 ){
            if(!pushButton_AllIn->text().isEmpty()) pushButton_AllIn->setFKeyText("F4");
            if(!pushButton_BetRaise->text().isEmpty()) pushButton_BetRaise->setFKeyText("F3");
            if(!pushButton_CallCheck->text().isEmpty()) pushButton_CallCheck->setFKeyText("F2");
            if(!pushButton_Fold->text().isEmpty()) pushButton_Fold->setFKeyText("F1");
        }
        else {
            if(!pushButton_AllIn->text().isEmpty()) pushButton_AllIn->setFKeyText("F1");
            if(!pushButton_BetRaise->text().isEmpty()) pushButton_BetRaise->setFKeyText("F2");
            if(!pushButton_CallCheck->text().isEmpty()) pushButton_CallCheck->setFKeyText("F3");
            if(!pushButton_Fold->text().isEmpty()) pushButton_Fold->setFKeyText("F4");
        }
    }
}

void gameTableImpl::refreshGameTableStyle()
{
    myGameTableStyle->setWindowsGeometry(this);
    myGameTableStyle->setChatLogStyle(textBrowser_Log);
    myGameTableStyle->setChatLogStyle(textBrowser_Chat);
    myGameTableStyle->setChatInputStyle(lineEdit_ChatInput);

    int i;
    for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {

        myGameTableStyle->setCashLabelStyle(cashLabelArray[i]);
        myGameTableStyle->setSetLabelStyle(setLabelArray[i]);
        myGameTableStyle->setPlayerNameLabelStyle(playerNameLabelArray[i]);
    }

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
    myGameTableStyle->setCardHolderStyle(label_CardHolder0,0);
    myGameTableStyle->setCardHolderStyle(label_CardHolder1,0);
    myGameTableStyle->setCardHolderStyle(label_CardHolder2,0);
    myGameTableStyle->setCardHolderStyle(label_CardHolder3,1);
    myGameTableStyle->setCardHolderStyle(label_CardHolder4,2);
    myGameTableStyle->setTableBackground(this);
    myGameTableStyle->setMenuBarStyle(menubar);
    myGameTableStyle->setBreakButtonStyle(pushButton_break,0);
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

    for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {

        myGameTableStyle->setPlayerSeatInactiveStyle(groupBoxArray[i]);
    }
    //Human player button
    myGameTableStyle->setButtonsStyle(pushButton_BetRaise, pushButton_CallCheck, pushButton_Fold, pushButton_AllIn, 0);
    myGameTableStyle->setShowMyCardsButtonStyle(pushButton_showMyCards);

    myGameTableStyle->setBetValueInputStyle(lineEdit_betValue);
    myGameTableStyle->setSliderStyle(horizontalSlider_bet);
    myGameTableStyle->setSliderStyle(horizontalSlider_speed);

    // 	away radiobuttons
    myGameTableStyle->setAwayRadioButtonsStyle(radioButton_manualAction);
    myGameTableStyle->setAwayRadioButtonsStyle(radioButton_autoCheckFold);
    myGameTableStyle->setAwayRadioButtonsStyle(radioButton_autoCheckCallAny);

    myGameTableStyle->setToolBoxBackground(groupBox_RightToolBox);
    myGameTableStyle->setToolBoxBackground(groupBox_LeftToolBox);

    myGameTableStyle->setTabWidgetStyle(tabWidget_Right, tabWidget_Right->getMyTabBar());
    myGameTableStyle->setTabWidgetStyle(tabWidget_Left, tabWidget_Left->getMyTabBar());

    label_Handranking->setPixmap(myGameTableStyle->getHandRanking());

    if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || myGameTableStyle->getActionCallI18NString() == "NULL") { CallString = "Call";	}
    else { CallString = myGameTableStyle->getActionCallI18NString(); }
    if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || myGameTableStyle->getActionCheckI18NString() == "NULL") { CheckString = "Check"; }
    else { CheckString = myGameTableStyle->getActionCheckI18NString(); }
    if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || myGameTableStyle->getActionBetI18NString() == "NULL") { BetString = "Bet"; }
    else { BetString = myGameTableStyle->getActionBetI18NString(); }
    if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || myGameTableStyle->getActionRaiseI18NString() == "NULL") { RaiseString = "Raise"; }
    else { RaiseString = myGameTableStyle->getActionRaiseI18NString(); }
    if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || myGameTableStyle->getActionFoldI18NString() == "NULL") { FoldString = "Fold"; }
    else { FoldString = myGameTableStyle->getActionFoldI18NString(); }
    if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || myGameTableStyle->getActionAllInI18NString() == "NULL") { AllInString = "All-In"; }
    else { AllInString = myGameTableStyle->getActionAllInI18NString(); }
    if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || myGameTableStyle->getPotI18NString() == "NULL") { PotString = "Pot";	}
    else { PotString = myGameTableStyle->getPotI18NString(); }
    if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || myGameTableStyle->getTotalI18NString() == "NULL") { TotalString = "Total"; }
    else { TotalString = myGameTableStyle->getTotalI18NString(); }
    if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || myGameTableStyle->getBetsI18NString() == "NULL") { BetsString = "Bets"; }
    else { BetsString = myGameTableStyle->getBetsI18NString(); }
    if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || myGameTableStyle->getGameI18NString() == "NULL") { GameString = "Game"; }
    else { GameString = myGameTableStyle->getGameI18NString(); }
    if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || myGameTableStyle->getHandI18NString() == "NULL") { HandString = "Hand"; }
    else { HandString = myGameTableStyle->getHandI18NString(); }
    if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || myGameTableStyle->getPreflopI18NString() == "NULL") { PreflopString = "Preflop"; }
    else { PreflopString = myGameTableStyle->getPreflopI18NString(); }
    if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || myGameTableStyle->getFlopI18NString() == "NULL") { FlopString = "Flop"; }
    else { FlopString = myGameTableStyle->getFlopI18NString(); }
    if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || myGameTableStyle->getTurnI18NString() == "NULL") { TurnString = "Turn"; }
    else { TurnString = myGameTableStyle->getTurnI18NString(); }
    if(myConfig->readConfigInt("DontTranslateInternationalPokerStringsFromStyle") || myGameTableStyle->getRiverI18NString() == "NULL") { RiverString = "River"; }
    else { RiverString = myGameTableStyle->getRiverI18NString(); }

    label_Pot->setText(PotString);
    label_Total->setText(TotalString+":");
    label_Sets->setText(BetsString+":");
    label_handNumber->setText(HandString+":");
    label_gameNumber->setText(GameString+":");

}

void gameTableImpl::saveGameTableGeometry()
{
    if(this->isFullScreen()) {
        myConfig->writeConfigInt("GameTableFullScreenSave", 1);
    }
    else {
        myConfig->writeConfigInt("GameTableFullScreenSave", 0);
        myConfig->writeConfigInt("GameTableHeightSave", this->height());
        myConfig->writeConfigInt("GameTableWidthSave", this->width());
    }
    myConfig->writeBuffer();
}

void gameTableImpl::restoreGameTableGeometry()
{
    if(myConfig->readConfigInt("GameTableFullScreenSave")) {
        if(actionFullScreen->isEnabled()) this->showFullScreen();
    }
    else {
        //resize only if style size allow this and if NOT fixed windows size
        if(!myGameTableStyle->getIfFixedWindowSize().toInt() && myConfig->readConfigInt("GameTableHeightSave") <= myGameTableStyle->getMaximumWindowHeight().toInt() && myConfig->readConfigInt("GameTableHeightSave") >= myGameTableStyle->getMinimumWindowHeight().toInt() && myConfig->readConfigInt("GameTableWidthSave") <= myGameTableStyle->getMaximumWindowWidth().toInt() && myConfig->readConfigInt("GameTableWidthSave") >= myGameTableStyle->getMinimumWindowWidth().toInt()){

            this->resize(myConfig->readConfigInt("GameTableWidthSave"), myConfig->readConfigInt("GameTableHeightSave"));
        }
    }
}

void gameTableImpl::netClientPlayerLeft(unsigned playerId) {

    if (myStartWindow->getSession()->getCurrentGame() && myStartWindow->getSession()->isNetworkClientRunning()) {

        boost::shared_ptr<PlayerInterface> tmpPlayer = myStartWindow->getSession()->getCurrentGame()->getPlayerByUniqueId(playerId);
        if (tmpPlayer.get()) {
            tmpPlayer->setMyStayOnTableStatus(0);
            refreshPlayerAvatar();
            refreshPlayerName();
        }
    }
}

void gameTableImpl::registeredUserMode()
{
    lineEdit_ChatInput->clear();
    lineEdit_ChatInput->setEnabled(true);
    guestMode = false;
}


void gameTableImpl::guestUserMode()
{
    lineEdit_ChatInput->setText(tr("Chat is only available to registered players."));
    lineEdit_ChatInput->setDisabled(true);
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
        //        showMyCards();
        pushButton_showMyCards->hide();
    }
}
