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
#include "startwindowimpl.h"
#include "playerinterface.h"
#include <gamedata.h>
#include <generic/serverguiwrapper.h>
#include <net/socket_msg.h>
#include <iostream>
#include "tools.h"
#include "session.h"
#include "game.h"
#include "guiwrapper.h"
#include "configfile.h"
#include "gametableimpl.h"
#include "soundevents.h"
#include "newgamedialogimpl.h"
#include "aboutpokerthimpl.h"
#include "mymessagedialogimpl.h"
#include "mymessagebox.h"
#include "settingsdialogimpl.h"
#include "selectavatardialogimpl.h"
#include "joinnetworkgamedialogimpl.h"
#include "connecttoserverdialogimpl.h"
#include "createnetworkgamedialogimpl.h"
#include "startnetworkgamedialogimpl.h"
#include "changecontentdialogimpl.h"
#include "changecompleteblindsdialogimpl.h"
#include "gamelobbydialogimpl.h"
#include "timeoutmsgboximpl.h"
#include "chattools.h"
#include "serverlistdialogimpl.h"
#include "internetgamelogindialogimpl.h"
#include "logfiledialog.h"
#include "guilog.h"
#include "darkmodehelper.h"
#include <QDebug>
#include <QScreen>
#include <QWindow>
#include <QGuiApplication>
#include <QStyle>
#include <QStyleFactory>

#ifdef ANDROID
#ifndef ANDROID_TEST
#include <QJniEnvironment>
#include <QJniObject>
#endif
#endif

using namespace std;

startWindowImpl::startWindowImpl(ConfigFile *c, Log *l, const std::string &password)
	: myConfig(c), myLog(l), msgBoxOutdatedVersionActive(false)
{

	myGuiInterface.reset(new GuiWrapper(myConfig, this));
	{
		mySession.reset(new Session(myGuiInterface.get(), myConfig, myLog));
		mySession->bbcbotpassword = password; // bbcbot code
		mySession->init(); // TODO handle error
		myLog->init();
		// 		myGuiInterface->setSession(session);
	}
	myGuiInterface->getMyGuiLog()->setMySqliteLogFileName(myLog->getMySqliteLogFileName());

	// #ifdef __APPLE__
	// 	setWindowModality(Qt::ApplicationModal);
	// 	setWindowFlags(Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint | Qt::Dialog);
	// #endif
	setupUi(this);
	this->setWindowTitle(QString(tr("PokerTH %1").arg(POKERTH_BETA_RELEASE_STRING)));
	this->installEventFilter(this);

	// React to screen changes (hibernate/resume, DPI changes, monitor switch)
	if (windowHandle()) {
		connect(windowHandle(), &QWindow::screenChanged,
			this, &startWindowImpl::onScreenChanged);
	}
	QScreen *primaryScreen = QGuiApplication::primaryScreen();
	if (primaryScreen) {
		connect(primaryScreen, &QScreen::geometryChanged,
			this, &startWindowImpl::onScreenGeometryChanged, Qt::UniqueConnection);
		connect(primaryScreen, &QScreen::logicalDotsPerInchChanged,
			this, &startWindowImpl::onScreenDpiChanged, Qt::UniqueConnection);
	}

	//Widgets Grafiken per Stylesheets setzen
	QString myAppDataPath = QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str());
	this->setWindowIcon(QIcon(myAppDataPath+"gfx/gui/misc/windowicon.png"));

	this->setStatusBar(0);

#ifdef GUI_800x480
#ifdef ANDROID
	this->setMenuBar(0);
//	this->menubar->clear();
//	this->menubar->hide();

	//check if custom background pictures for the resolution are there. Otherwise create them!
	QString UserDataDir = QString::fromUtf8(myConfig->readConfigString("UserDataDir").c_str());
	QScreen *screen = QGuiApplication::primaryScreen();
	QRect screenGeometry = screen->geometry();
	int screenWidth = screenGeometry.width();
	int screenHeight = screenGeometry.height();
	QString customStartWindowBgFileString(UserDataDir+"/startwindowbg10_"+QString::number(screenWidth)+"x"+QString::number(screenHeight)+".png");
	QString customWelcomePokerTHFileString(UserDataDir+"/welcomepokerth10_"+QString::number(screenWidth)+"x"+QString::number(screenHeight)+".png");
	QFile customStartWindowBgFile(customStartWindowBgFileString);
	QFile customWelcomePokerTHFile(customWelcomePokerTHFileString);
	if(customStartWindowBgFile.exists()) {
		centralwidget->setStyleSheet(".QWidget { border-image: url("+QFileInfo(customStartWindowBgFile).absoluteFilePath()+") 0 0 0 0 stretch stretch;}");
	} else {
		//if custom bg file could not be found load the big origin file
		centralwidget->setStyleSheet(".QWidget { border-image: url(:/android/android-data/gfx/gui/misc/startwindowbg10_mobile.png) 0 0 0 0 stretch stretch;}");
	}
	this->showFullScreen();

	// showFullScreen() alone does not reliably resize the QMainWindow
	// when QT_SCALE_FACTOR < 1.0 is set.  Force the geometry explicitly.
	setMinimumSize(0, 0);
	{
		QScreen *scr = QGuiApplication::primaryScreen();
		if (scr) {
			setGeometry(scr->availableGeometry());
		}
	}

	// The .ui file constrains buttons to fixed 350×80 (designed for 800px).
	// Remove ALL min/max width caps so the grid layout can expand them.
	QList<QPushButton*> allBtns = { pushButtonStart_Local_Game,
		pushButton_Create_Network_Game, pushButtonInternet_Game,
		pushButton_Join_Network_Game, pushButton_configure,
		pushButton_about, pushButton_Logs };
	for (QPushButton *btn : allBtns) {
		btn->setMinimumSize(0, 0);
		btn->setMaximumWidth(QWIDGETSIZE_MAX);
	}

	// The .ui gridLayout_2 has horizontal spacers in columns 0 and 2 that
	// center the button grid at 800px.  With QT_SCALE_FACTOR the screen is
	// wider, so give column 1 (content) all the stretch.
	QGridLayout *mainGrid = qobject_cast<QGridLayout*>(centralwidget->layout());
	if (mainGrid) {
		mainGrid->setColumnStretch(0, 0);  // left spacer: no stretch
		mainGrid->setColumnStretch(1, 1);  // button grid: takes all space
		mainGrid->setColumnStretch(2, 0);  // right spacer: no stretch
		// Also set row stretches: button grid rows expand, spacers minimal
		mainGrid->setRowStretch(0, 0);  // top vertical spacer
		mainGrid->setRowStretch(1, 1);  // button grid row
		mainGrid->setRowStretch(2, 0);  // Logs row
		mainGrid->setRowStretch(3, 0);  // bottom vertical spacer
	}

	// The inner 2-column button grid needs equal column stretches too.
	if (gridLayout) {
		gridLayout->setColumnStretch(0, 1);
		gridLayout->setColumnStretch(1, 1);
	}

	// Logs row (horizontalLayout): give the button stretch so it fills.
	if (horizontalLayout) {
		horizontalLayout->setStretch(0, 0);  // left spacer
		horizontalLayout->setStretch(1, 1);  // Logs button
		horizontalLayout->setStretch(2, 0);  // right spacer
	}

	//TODO HACK Missing QSystemScreenSaver::setScreenSaverInhibited(true)
//		#ifndef ANDROID_TEST
//			JavaVM *currVM = (JavaVM *)QApplication::platformNativeInterface()->nativeResourceForWidget("JavaVM", 0);
//			JNIEnv* env;
//			if (currVM->AttachCurrentThread(&env, NULL)<0) {
//				qCritical()<<"AttachCurrentThread failed";
//			} else {
//	//			jclass jclassApplicationClass = env->FindClass("android/view/View");
//	//			if (jclassApplicationClass) {
//	//				env->SetStaticIntField(jclassApplicationClass, env->GetStaticFieldID(jclassApplicationClass,"KEEP_SCREEN_ON", "I"), 1);
//	//			}
//				currVM->DetachCurrentThread();
//			}
//		#endif
#else
//		Maemo
	this->menubar->hide();
	centralwidget->setStyleSheet(".QWidget { background-image: url(\""+myAppDataPath+"gfx/gui/misc/startwindowbg10_desktop.png\"); background-position: bottom center; background-origin: content; background-repeat: no-repeat;}");
#endif
	// All mobile GUI's
	// Scale button font-size to screen, same algorithm as in pokerth.cpp.
	int mobileBtnFontPx = 22;
#ifdef ANDROID
	{
		int userScale = myConfig->readConfigInt("AndroidUiScalePercent");
		if (userScale > 0) {
			mobileBtnFontPx = qMax(10, 22 * userScale / 100);
		} else {
			QScreen *scr = QGuiApplication::primaryScreen();
			if (scr) {
				QRect geo = scr->availableGeometry();
				int sw = qMax(geo.width(), geo.height());
				int sh = qMin(geo.width(), geo.height());
				qreal scale = qMin(static_cast<qreal>(sw) / 800.0,
				                   static_cast<qreal>(sh) / 480.0);
				scale = qBound(0.5, scale, 1.0);
				mobileBtnFontPx = qMax(10, static_cast<int>(22.0 * scale + 0.5));
			}
		}
	}
#endif
	QString mobileBtnStyle = QString("QPushButton { text-align:left; font-weight:bold; padding-left: 3px; padding-bottom: 3px; padding-top: 3px; padding-right: 3px; background-color: #505050; color: #FDC942; font-size:%1px; border-width: 0px;}").arg(mobileBtnFontPx);
	pushButtonStart_Local_Game->setStyleSheet(mobileBtnStyle);
	pushButtonInternet_Game->setStyleSheet(mobileBtnStyle);
	pushButton_Create_Network_Game->setStyleSheet(mobileBtnStyle);
	pushButton_Join_Network_Game->setStyleSheet(mobileBtnStyle);
	pushButton_about->setStyleSheet(mobileBtnStyle);
	pushButton_configure->setStyleSheet(mobileBtnStyle);
	pushButton_Logs->setStyleSheet(mobileBtnStyle);

	connect( pushButton_about, SIGNAL( clicked() ), this, SLOT( callAboutPokerthDialog() ) );
	connect( pushButton_configure, SIGNAL( clicked() ), this, SLOT( callSettingsDialogFromStartwindow() ) );
#else
	//Desktop
	this->menubar->setStyleSheet("QMenuBar { background-color: #505050; font-size:12px; border-width: 0px;} QMenuBar::item { background: transparent; color: #FDC942; } QMenuBar::item:selected { background: #787878; color: #FDC942; } QMenuBar::item:pressed { background: #FDC942; color: #505050; }");
	centralwidget->setStyleSheet(".QWidget { background-image: url(\""+myAppDataPath+"gfx/gui/misc/startwindowbg10_desktop.png\"); background-position: bottom center; background-origin: content; background-repeat: no-repeat;}");

	pushButtonStart_Local_Game->setStyleSheet("QPushButton { text-align:left; font-weight:bold; padding-left: 1px; padding-bottom: 3px; padding-top: 3px; padding-right: 3px; background-color: #505050; color: #FDC942; font-size:12px; border: 1px solid #505050;}");
	pushButtonInternet_Game->setStyleSheet("QPushButton { text-align:left; font-weight:bold; padding-left: 1px; padding-bottom: 3px; padding-top: 3px; padding-right: 3px; background-color: #505050; color: #FDC942; font-size:12px; border: 1px solid #505050;}");
	pushButton_Create_Network_Game->setStyleSheet("QPushButton { text-align:left; font-weight:bold; padding-left: 1px; padding-bottom: 3px; padding-top: 3px; padding-right: 3px; background-color: #505050; color: #FDC942; font-size:12px; border: 1px solid #505050;}");
	pushButton_Join_Network_Game->setStyleSheet("QPushButton { text-align:left; font-weight:bold; padding-left: 1px; padding-bottom: 3px; padding-top: 3px; padding-right: 3px; background-color: #505050; color: #FDC942; font-size:12px; border: 1px solid #505050;}");
	pushButton_Logs->setStyleSheet("QPushButton { text-align:left; font-weight:bold; padding-left: 1px; padding-bottom: 3px; padding-top: 3px; padding-right: 3px; background-color: #505050; color: #FDC942; font-size:12px; border: 1px solid #505050;}");

	// On Windows 10 the native "windowsvista" style keeps drawing the push
	// buttons with the native theme and ignores the stylesheet
	// background-color/color set above, so the buttons appear white/native
	// (Windows 11 uses the newer "windows11" style, where it works).  Force
	// the Fusion style on just these buttons so the stylesheet colors are
	// honored identically on Win10 and Win11.
	if (QStyle *fusionStyle = QStyleFactory::create("Fusion")) {
		fusionStyle->setParent(this);
		const QList<QPushButton*> startWindowButtons = {
			pushButtonStart_Local_Game, pushButtonInternet_Game,
			pushButton_Create_Network_Game, pushButton_Join_Network_Game,
			pushButton_Logs };
		for (QPushButton *btn : startWindowButtons) {
			btn->setStyle(fusionStyle);
		}
	}

	connect( actionAbout_PokerTH, SIGNAL( triggered() ), this, SLOT( callAboutPokerthDialog() ) );
	connect( actionConfigure_PokerTH, SIGNAL( triggered() ), this, SLOT( callSettingsDialogFromStartwindow() ) );

#endif

	// 	Dialogs
	myNewGameDialog = new newGameDialogImpl(this, myConfig);
	mySelectAvatarDialog = new selectAvatarDialogImpl(this, myConfig);
	mySettingsDialog = new settingsDialogImpl(this, myConfig, mySelectAvatarDialog);
	myJoinNetworkGameDialog = new joinNetworkGameDialogImpl(this, myConfig);
	myConnectToServerDialog = new connectToServerDialogImpl(this);
	myStartNetworkGameDialog = new startNetworkGameDialogImpl(this, myConfig);
	myCreateNetworkGameDialog = new createNetworkGameDialogImpl(this, myConfig);
	myAboutPokerthDialog = new aboutPokerthImpl(this, myConfig);
	myGameLobbyDialog = new gameLobbyDialogImpl(this, myConfig);
	myLogFileDialog = new LogFileDialog(this, myConfig);

	myStartNetworkGameDialog->setMyW(myGuiInterface->getMyW());
	myGameLobbyDialog->setMyW(myGuiInterface->getMyW());
	mySettingsDialog->setGuiLog(myGuiLog);
	myLogFileDialog->setGuiLog(myGuiLog);

	myTimeoutDialog = new timeoutMsgBoxImpl(this);
	myServerListDialog = new serverListDialogImpl(this, this, myConfig);
	myInternetGameLoginDialog = new internetGameLoginDialogImpl(this, myConfig);

	connect( actionStart_Local_Game, SIGNAL( triggered() ), this, SLOT( callNewGameDialog() ) );
	connect( pushButtonStart_Local_Game, SIGNAL( clicked() ), this, SLOT( callNewGameDialog() ) );
	connect( actionInternet_Game, SIGNAL( triggered() ), this, SLOT( joinGameLobby() ) );
	connect( pushButtonInternet_Game, SIGNAL( clicked() ), this, SLOT( joinGameLobby() ) );
	connect( actionCreate_Network_Game, SIGNAL( triggered() ), this, SLOT( callCreateNetworkGameDialog() ) );
	connect( pushButton_Create_Network_Game, SIGNAL( clicked() ), this, SLOT( callCreateNetworkGameDialog() ) );
	connect( actionJoin_Network_Game, SIGNAL( triggered() ), this, SLOT( callJoinNetworkGameDialog() ) );
	connect( pushButton_Join_Network_Game, SIGNAL( clicked() ), this, SLOT( callJoinNetworkGameDialog() ) );
	connect( pushButton_Logs, SIGNAL( clicked() ), this, SLOT( callLogFileDialog() ) );

	connect(this, SIGNAL(signalShowClientDialog()), this, SLOT(showClientDialog()));

	connect(this, SIGNAL(signalNetClientConnect(int)), myConnectToServerDialog, SLOT(refresh(int)));
	connect(this, SIGNAL(signalNetClientGameInfo(int)), myStartNetworkGameDialog, SLOT(refresh(int)));
	connect(this, SIGNAL(signalNetClientGameInfo(int)), myGameLobbyDialog, SLOT(refresh(int)));

	connect(this, SIGNAL(signalNetClientServerListShow()), myServerListDialog, SLOT(exec()));
	connect(this, SIGNAL(signalNetClientServerListClear()), myServerListDialog, SLOT(clearList()));
	connect(this, SIGNAL(signalNetClientServerListAdd(unsigned)), myServerListDialog, SLOT(addServerItem(unsigned)));

	connect(this, SIGNAL(signalNetClientLoginShow()), this, SLOT(callInternetGameLoginDialog()));
	connect(this, SIGNAL(signalNetClientRejoinPossible(unsigned)), this, SLOT(callRejoinPossibleDialog(unsigned)));

	connect(this, SIGNAL(signalNetClientSelfJoined(unsigned, QString, bool)), myStartNetworkGameDialog, SLOT(joinedNetworkGame(unsigned, QString, bool)));
	connect(this, SIGNAL(signalNetClientPlayerJoined(unsigned, QString, bool)), myStartNetworkGameDialog, SLOT(addConnectedPlayer(unsigned, QString, bool)));
	connect(this, SIGNAL(signalNetClientPlayerChanged(unsigned, QString)), myStartNetworkGameDialog, SLOT(updatePlayer(unsigned, QString)));
	connect(this, SIGNAL(signalNetClientPlayerLeft(unsigned, QString)), myStartNetworkGameDialog, SLOT(removePlayer(unsigned, QString)));
	connect(this, SIGNAL(signalNetClientNewGameAdmin(unsigned, QString)), myStartNetworkGameDialog, SLOT(newGameAdmin(unsigned, QString)));
	connect(this, SIGNAL(signalNetClientGameListNew(unsigned)), myStartNetworkGameDialog, SLOT(gameCreated(unsigned)));

	connect(this, SIGNAL(signalNetClientSelfJoined(unsigned, QString, bool)), myGameLobbyDialog, SLOT(joinedNetworkGame(unsigned, QString, bool)));
	
	// BBCBot: Reset reconnect attempts on successful connection
	if (bbcbotReconnectEnabled) {
		connect(this, SIGNAL(signalNetClientSelfJoined(unsigned, QString, bool)), this, SLOT(bbcbotResetReconnectAttempts()));
	}
	connect(this, SIGNAL(signalNetClientPlayerJoined(unsigned, QString, bool)), myGameLobbyDialog, SLOT(addConnectedPlayer(unsigned, QString, bool)));
	connect(this, SIGNAL(signalNetClientPlayerChanged(unsigned, QString)), myGameLobbyDialog, SLOT(updatePlayer(unsigned, QString)));
	connect(this, SIGNAL(signalNetClientPlayerLeft(unsigned, QString)), myGameLobbyDialog, SLOT(removePlayer(unsigned, QString)));
	connect(this, SIGNAL(signalNetClientSpectatorJoined(unsigned, QString)), myGameLobbyDialog, SLOT(addConnectedSpectator(unsigned, QString)));
	connect(this, SIGNAL(signalNetClientSpectatorLeft(unsigned, QString)), myGameLobbyDialog, SLOT(removeSpectator(unsigned, QString)));
	connect(this, SIGNAL(signalNetClientNewGameAdmin(unsigned, QString)), myGameLobbyDialog, SLOT(newGameAdmin(unsigned, QString)));

	connect(this, SIGNAL(signalNetClientGameListNew(unsigned)), myGameLobbyDialog, SLOT(addGame(unsigned)));
	connect(this, SIGNAL(signalNetClientGameListRemove(unsigned)), myGameLobbyDialog, SLOT(removeGame(unsigned)));
	connect(this, SIGNAL(signalNetClientGameListUpdateMode(unsigned, int)), myGameLobbyDialog, SLOT(updateGameMode(unsigned, int)));
	connect(this, SIGNAL(signalNetClientGameListUpdateAdmin(unsigned, unsigned)), myGameLobbyDialog, SLOT(updateGameAdmin(unsigned, unsigned)));
	connect(this, SIGNAL(signalNetClientGameListPlayerJoined(unsigned, unsigned)), myGameLobbyDialog, SLOT(gameAddPlayer(unsigned, unsigned)));
	connect(this, SIGNAL(signalNetClientGameListPlayerLeft(unsigned, unsigned)), myGameLobbyDialog, SLOT(gameRemovePlayer(unsigned, unsigned)));
	connect(this, SIGNAL(signalNetClientGameListSpectatorJoined(unsigned, unsigned)), myGameLobbyDialog, SLOT(gameAddSpectator(unsigned, unsigned)));
	connect(this, SIGNAL(signalNetClientGameListSpectatorLeft(unsigned, unsigned)), myGameLobbyDialog, SLOT(gameRemoveSpectator(unsigned, unsigned)));
	connect(this, SIGNAL(signalNetClientRemovedFromGame(int)), myGameLobbyDialog, SLOT(removedFromGame(int)));
	connect(this, SIGNAL(signalNetClientStatsUpdate(ServerStats)), this, SLOT(handleStatsUpdate(ServerStats)));

	// Connection monitoring: update activity on frequent server events
	// Lobby signals (active when not in a game)
	connect(this, SIGNAL(signalNetClientGameListNew(unsigned)), this, SLOT(updateServerActivity()));
	connect(this, SIGNAL(signalNetClientGameListRemove(unsigned)), this, SLOT(updateServerActivity()));
	connect(this, SIGNAL(signalLobbyPlayerJoined(unsigned, QString)), this, SLOT(updateServerActivity()));
	connect(this, SIGNAL(signalLobbyPlayerLeft(unsigned)), this, SLOT(updateServerActivity()));
	// In-game signals (critical: these are the primary activity source during active games)
	connect(this, SIGNAL(signalNetClientGameInfo(int)), this, SLOT(updateServerActivity()));
	connect(this, SIGNAL(signalNetClientGameChatMsg(QString, QString)), this, SLOT(updateServerActivity()));
	connect(this, SIGNAL(signalNetClientGameStart(boost::shared_ptr<Game>)), this, SLOT(updateServerActivity()));
	connect(this, SIGNAL(signalNetClientPlayerJoined(unsigned, QString, bool)), this, SLOT(updateServerActivity()));
	connect(this, SIGNAL(signalNetClientPlayerLeft(unsigned, QString)), this, SLOT(updateServerActivity()));
	connect(this, SIGNAL(signalNetClientShowTimeoutDialog(int, unsigned)), this, SLOT(updateServerActivity()));

	connect(this, SIGNAL(signalNetClientGameChatMsg(QString, QString)), myGuiInterface->getMyW()->getMyChat(), SLOT(receiveMessage(QString, QString)));
	connect(this, SIGNAL(signalNetClientLobbyChatMsg(QString, QString)), myStartNetworkGameDialog->getMyChat(), SLOT(receiveMessage(QString, QString)));
	connect(this, SIGNAL(signalNetClientLobbyChatMsg(QString, QString)), myGameLobbyDialog->getMyChat(), SLOT(receiveMessage(QString, QString)));
	connect(this, SIGNAL(signalNetClientPrivateChatMsg(QString, QString)), myStartNetworkGameDialog->getMyChat(), SLOT(privateMessage(QString, QString)));
	connect(this, SIGNAL(signalNetClientPrivateChatMsg(QString, QString)), myGameLobbyDialog->getMyChat(), SLOT(privateMessage(QString, QString)));
	connect(this, SIGNAL(signalNetClientMsgBox(QString)), this, SLOT(networkMessage(QString)));
	connect(this, SIGNAL(signalNetClientMsgBox(unsigned)), this, SLOT(networkMessage(unsigned)));
	connect(this, SIGNAL(signalNetClientShowTimeoutDialog(int, unsigned)), this, SLOT(showTimeoutDialog(int, unsigned)));

	connect(this, SIGNAL(signalLobbyPlayerJoined(unsigned, QString)), myGameLobbyDialog, SLOT(playerJoinedLobby(unsigned, QString)));
	connect(this, SIGNAL(signalLobbyPlayerLeft(unsigned)), myGameLobbyDialog, SLOT(playerLeftLobby(unsigned)));


	// Errors are handled globally, not within one dialog.
	connect(this, SIGNAL(signalNetClientError(int, int)), this, SLOT(networkError(int, int)));
	connect(this, SIGNAL(signalNetClientNotification(int)), this, SLOT(networkNotification(int)));
	connect(this, SIGNAL(signalNetServerError(int, int)), this, SLOT(networkError(int, int)));
	connect(this, SIGNAL(signalNetClientRemovedFromGame(int)), this, SLOT(networkNotification(int)));
	connect(this, SIGNAL(signalNetClientGameStart(boost::shared_ptr<Game>)), this, SLOT(networkStart(boost::shared_ptr<Game>)));

	connect(this, SIGNAL(signalSelfGameInvitation(unsigned, unsigned)), myGameLobbyDialog, SLOT(showInvitationDialog(unsigned, unsigned)));
	connect(this, SIGNAL(signalPlayerGameInvitation(unsigned, unsigned, unsigned)), myGameLobbyDialog, SLOT(chatInfoPlayerInvitation(unsigned, unsigned, unsigned)));
	connect(this, SIGNAL(signalRejectedGameInvitation(unsigned, unsigned, DenyGameInvitationReason)), myGameLobbyDialog, SLOT(chatInfoPlayerRejectedInvitation(unsigned, unsigned, DenyGameInvitationReason)));

	// Initialize connection heartbeat monitoring
	connectionHeartbeatTimer = new QTimer(this);
	connect(connectionHeartbeatTimer, SIGNAL(timeout()), this, SLOT(connectionHeartbeatCheck()));
	connectionMonitoringActive = false;
	connectionLostHandlingActive = false;
	missedHeartbeats = 0;
	lastServerActivityTimer.start(); // monotonic clock

	this->show();

	//update HACKS
	if(!checkForFirstStartAfterUpdated().isEmpty()) {
	}

	// bbcbot code - auto-start internet game if password is provided
	if (!mySession->bbcbotpassword.empty()) {
		// Use QTimer to start after the event loop is running
		QTimer::singleShot(100, this, SLOT(callGameLobbyDialog()));
		
		// Initialize reconnect timer for BBCBot
		bbcbotReconnectTimer = new QTimer(this);
		bbcbotReconnectTimer->setSingleShot(true);
		connect(bbcbotReconnectTimer, SIGNAL(timeout()), this, SLOT(bbcbotAttemptReconnect()));
		bbcbotReconnectEnabled = true;
		bbcbotReconnectAttempts = 0;
	} else {
		bbcbotReconnectTimer = nullptr;
		bbcbotReconnectEnabled = false;
		bbcbotReconnectAttempts = 0;
	}
	// end bbcbot code

}

startWindowImpl::~startWindowImpl()
{
#if !defined(__APPLE__) && !defined(_WIN32)
	// On Linux the lobby dialog has no Qt parent (setParent(nullptr) to
	// break WM_TRANSIENT_FOR), so it must be deleted explicitly.
	delete myGameLobbyDialog;
#endif
}

void startWindowImpl::callNewGameDialog()
{

	//wenn Dialogfenster gezeigt werden soll
	if(myConfig->readConfigInt("ShowGameSettingsDialogOnNewGame")) {

#ifdef ANDROID
		myGuiInterface->getMyW()->hide();
#endif
		myNewGameDialog->exec();
		if (myNewGameDialog->result() == QDialog::Accepted ) {
			startNewLocalGame(myNewGameDialog);
		}
	}
	// sonst mit gespeicherten Werten starten
	else {
		startNewLocalGame();
	}
}

void startWindowImpl::startNewLocalGame(newGameDialogImpl *v)
{

	this->hide();
	myGuiInterface->getMyW()->show();

	// Start new local game - terminate existing network game.
	stopConnectionMonitoring();
	mySession->terminateNetworkClient();
	if (myServerGuiInterface.get())
		myServerGuiInterface->getSession()->terminateNetworkServer();

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
		} else {
			gameData.raiseIntervalMode = RAISE_ON_MINUTES;
			gameData.raiseSmallBlindEveryMinutesValue = v->getChangeCompleteBlindsDialog()->spinBox_raiseSmallBlindEveryMinutes->value();
		}

		if(v->getChangeCompleteBlindsDialog()->radioButton_alwaysDoubleBlinds->isChecked()) {
			gameData.raiseMode = DOUBLE_BLINDS;
		} else {
			gameData.raiseMode = MANUAL_BLINDS_ORDER;
			list<int> tempBlindList;
			int i;
			bool ok = true;
			for(i=0; i<v->getChangeCompleteBlindsDialog()->listWidget_blinds->count(); i++) {
				tempBlindList.push_back(v->getChangeCompleteBlindsDialog()->listWidget_blinds->item(i)->text().toInt(&ok,10));
			}
			gameData.manualBlindsList = tempBlindList;

			if(v->getChangeCompleteBlindsDialog()->radioButton_afterThisAlwaysDoubleBlinds->isChecked()) {
				gameData.afterManualBlindsMode = AFTERMB_DOUBLE_BLINDS;
			} else {
				if(v->getChangeCompleteBlindsDialog()->radioButton_afterThisAlwaysRaiseAbout->isChecked()) {
					gameData.afterManualBlindsMode = AFTERMB_RAISE_ABOUT;
					gameData.afterMBAlwaysRaiseValue = v->getChangeCompleteBlindsDialog()->spinBox_afterThisAlwaysRaiseValue->value();
				} else {
					gameData.afterManualBlindsMode = AFTERMB_STAY_AT_LAST_BLIND;
				}
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
		} else {
			gameData.raiseIntervalMode = RAISE_ON_MINUTES;
			gameData.raiseSmallBlindEveryMinutesValue = myConfig->readConfigInt("RaiseSmallBlindEveryMinutes");
		}

		if(myConfig->readConfigInt("AlwaysDoubleBlinds")) {
			gameData.raiseMode = DOUBLE_BLINDS;
		} else {
			gameData.raiseMode = MANUAL_BLINDS_ORDER;
			gameData.manualBlindsList = myConfig->readConfigIntList("ManualBlindsList");

			if(myConfig->readConfigInt("AfterMBAlwaysDoubleBlinds")) {
				gameData.afterManualBlindsMode = AFTERMB_DOUBLE_BLINDS;
			} else {
				if(myConfig->readConfigInt("AfterMBAlwaysRaiseAbout")) {
					gameData.afterManualBlindsMode = AFTERMB_RAISE_ABOUT;
					gameData.afterMBAlwaysRaiseValue = myConfig->readConfigInt("AfterMBAlwaysRaiseValue");
				} else {
					gameData.afterManualBlindsMode = AFTERMB_STAY_AT_LAST_BLIND;
				}
			}
		}
		//Speeds
		gameData.guiSpeed = myConfig->readConfigInt("GameSpeed");
	}
	// Set dealer pos.
	StartData startData;
	int tmpDealerPos = 0;
	startData.numberOfPlayers = gameData.maxNumberOfPlayers;
	Tools::GetRand(0, startData.numberOfPlayers-1, 1, &tmpDealerPos);
	//if(DEBUG_MODE) {
	//    tmpDealerPos = 4;
	//}
	startData.startDealerPlayerId = static_cast<unsigned>(tmpDealerPos);

	//some gui modifications
	myGuiInterface->getMyW()->localGameModification();

	//Start Game!!!
	mySession->startLocalGame(gameData, startData);
}

void startWindowImpl::callGameLobbyDialog()
{

	//Avoid join Lobby with "Human Player" nick
	if(QString::fromUtf8(myConfig->readConfigString("MyName").c_str()) == QString("Human Player")) {
		changeContentDialogImpl dialog(this, myConfig, CHANGE_HUMAN_PLAYER_NAME);
		dialog.exec();
		if(dialog.result() == QDialog::Accepted) {
			joinGameLobby();
		}
	} else {
		joinGameLobby();
	}
}

void startWindowImpl::joinGameLobby()
{

	// Stop local game.
	myGuiInterface->getMyW()->stopTimer();

	stopConnectionMonitoring();
	mySession->terminateNetworkClient();
	if (myServerGuiInterface)
		myServerGuiInterface->getSession()->terminateNetworkServer();

	myGameLobbyDialog->setSession(getSession());
	myStartNetworkGameDialog->setSession(getSession());

	// Clear Lobby dialog.
	myGameLobbyDialog->clearDialog();

	//start internet client with config values for user and pw TODO
	mySession->startInternetClient();

	//Dialog mit Statusbalken
	myConnectToServerDialog->exec();
	if (myConnectToServerDialog->result() == QDialog::Accepted ) {
		showLobbyDialog();
	} else {
		mySession->terminateNetworkClient();
	}
}


void startWindowImpl::callInternetGameLoginDialog()
{

	//login

	//HACK if Outdated Version info is available show it here!
	if(msgBoxOutdatedVersionActive) {
		msgBoxOutdatedVersionActive = false;
		msgBoxOutdatedVersion.exec();
		msgBoxOutdatedVersion.raise();
		msgBoxOutdatedVersion.activateWindow();
	}

	// bbcbot code - auto-login with bbcbotpassword if available
	if (!mySession->bbcbotpassword.empty()) {
		// Auto-login mode with password from argv
		// Read account name from file or use default
		std::string accountname;
		std::ifstream accountnamefile("accountname.txt");
		if (!std::getline(accountnamefile, accountname)) {
			accountname = "bbcbot";
		}
		mySession->setLogin(
			accountname,
			mySession->bbcbotpassword,
			false); // not a guest
	} else {
		// Normal interactive login
		myInternetGameLoginDialog->exec();

		if(myInternetGameLoginDialog->result() == QDialog::Accepted) {
			//send login infos
			mySession->setLogin(
				myConfig->readConfigString("MyName"),
				myInternetGameLoginDialog->lineEdit_password->text().toUtf8().constData(),
				myInternetGameLoginDialog->checkBox_guest->isChecked());
		} else {
			myConnectToServerDialog->reject();
			mySession->terminateNetworkClient();
		}
	}
}


void startWindowImpl::callRejoinPossibleDialog(unsigned gameId)
{
	/*
	assert(mySession);
	GameInfo info(mySession->getClientGameInfo(gameId));*/

	MyMessageBox msgBox;
	msgBox.setIcon(QMessageBox::Question);
	msgBox.setWindowTitle(tr("Rejoin possible!"));
	msgBox.setText(tr("There is an existing session with a previous game."));
	msgBox.setInformativeText(tr("Do you want to rejoin this game?"));
	msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	msgBox.setDefaultButton(QMessageBox::Yes);
	int ret = msgBox.exec();

	switch (ret) {
	case QMessageBox::Yes:
		mySession->clientRejoinGame(gameId);
		break;
	case QMessageBox::No:
		;
		break;
	}
}


void startWindowImpl::callCreateNetworkGameDialog()
{

	myCreateNetworkGameDialog->exec();
	//
	if (myCreateNetworkGameDialog->result() == QDialog::Accepted ) {

		// Stop local game.
		myGuiInterface->getMyW()->stopTimer();

		if (!myServerGuiInterface) {
			// Create pseudo Gui Wrapper for the server.
			myServerGuiInterface.reset(new ServerGuiWrapper(myConfig, mySession->getGui(), mySession->getGui()));
			{
				boost::shared_ptr<Session> session(new Session(myServerGuiInterface.get(), myConfig, 0));
				session->init(mySession->getAvatarManager());
				myServerGuiInterface->setSession(session);
			}
		}

		// Terminate existing network games.
		mySession->terminateNetworkClient();
		myServerGuiInterface->getSession()->terminateNetworkServer();

		GameData gameData;
		gameData.maxNumberOfPlayers = myCreateNetworkGameDialog->spinBox_quantityPlayers->value();
		gameData.startMoney = myCreateNetworkGameDialog->spinBox_startCash->value();
		gameData.firstSmallBlind = myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->spinBox_firstSmallBlind->value();

		if(myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->radioButton_raiseBlindsAtHands->isChecked()) {
			gameData.raiseIntervalMode = RAISE_ON_HANDNUMBER;
			gameData.raiseSmallBlindEveryHandsValue = myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->spinBox_raiseSmallBlindEveryHands->value();
		} else {
			gameData.raiseIntervalMode = RAISE_ON_MINUTES;
			gameData.raiseSmallBlindEveryMinutesValue = myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->spinBox_raiseSmallBlindEveryMinutes->value();
		}

		if(myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->radioButton_alwaysDoubleBlinds->isChecked()) {
			gameData.raiseMode = DOUBLE_BLINDS;
		} else {
			gameData.raiseMode = MANUAL_BLINDS_ORDER;
			std::list<int> tempBlindList;
			int i;
			bool ok = true;
			for(i=0; i<myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->listWidget_blinds->count(); i++) {
				tempBlindList.push_back(myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->listWidget_blinds->item(i)->text().toInt(&ok,10));
			}
			gameData.manualBlindsList = tempBlindList;

			if(myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->radioButton_afterThisAlwaysDoubleBlinds->isChecked()) {
				gameData.afterManualBlindsMode = AFTERMB_DOUBLE_BLINDS;
			} else {
				if(myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->radioButton_afterThisAlwaysRaiseAbout->isChecked()) {
					gameData.afterManualBlindsMode = AFTERMB_RAISE_ABOUT;
					gameData.afterMBAlwaysRaiseValue = myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->spinBox_afterThisAlwaysRaiseValue->value();
				} else {
					gameData.afterManualBlindsMode = AFTERMB_STAY_AT_LAST_BLIND;
				}
			}
		}

		gameData.guiSpeed = myConfig->readConfigInt("GameSpeed");
		gameData.delayBetweenHandsSec = myCreateNetworkGameDialog->spinBox_netDelayBetweenHands->value();
		gameData.playerActionTimeoutSec = myCreateNetworkGameDialog->spinBox_netTimeOutPlayerAction->value();

		myGameLobbyDialog->setSession(getSession());
		myStartNetworkGameDialog->setSession(getSession());

		// Clear network game dialog.
		myStartNetworkGameDialog->clearDialog();

		myServerGuiInterface->getSession()->startNetworkServer(false);
		mySession->startNetworkClientForLocalServer(gameData);

		myStartNetworkGameDialog->setMaxPlayerNumber(gameData.maxNumberOfPlayers);

		myStartNetworkGameDialog->setWindowTitle(tr("Start Network Game"));

		showNetworkStartDialog();
	}

}

void startWindowImpl::callJoinNetworkGameDialog()
{

	myJoinNetworkGameDialog->exec();

	if (myJoinNetworkGameDialog->result() == QDialog::Accepted ) {

		// Stop local game.
		myGuiInterface->getMyW()->stopTimer();

		mySession->terminateNetworkClient();
		if (myServerGuiInterface)
			myServerGuiInterface->getSession()->terminateNetworkServer();

		myGameLobbyDialog->setSession(getSession());
		myStartNetworkGameDialog->setSession(getSession());
		// Clear network game dialog
		myStartNetworkGameDialog->clearDialog();
		// Maybe use QUrl::toPunycode.
		mySession->startNetworkClient(
			myJoinNetworkGameDialog->lineEdit_ipAddress->text().toUtf8().constData(),
			myJoinNetworkGameDialog->spinBox_port->value(),
			myJoinNetworkGameDialog->checkBox_ipv6->isChecked(),
			myJoinNetworkGameDialog->checkBox_sctp->isChecked());

		//Dialog mit Statusbalken
		myConnectToServerDialog->exec();

		if (myConnectToServerDialog->result() == QDialog::Rejected ) {
			mySession->terminateNetworkClient();
			actionJoin_Network_Game->trigger(); // re-trigger
		} else {
			//needed for join and ready sounds - TODO
			//myStartNetworkGameDialog->setMaxPlayerNumber(gameData.maxNumberOfPlayers);
			myStartNetworkGameDialog->setWindowTitle(tr("Start Network Game"));

			showNetworkStartDialog();
		}
	}
}


void startWindowImpl::showClientDialog()
{
	if (mySession->getGameType() == Session::GAME_TYPE_NETWORK) {
		if (myGuiInterface->getMyW()->isVisible())
			myGuiInterface->getMyW()->hide();
		if (!this->isVisible())
			this->show();
		if (!myStartNetworkGameDialog->isVisible())
			showNetworkStartDialog();
	} else if (mySession->getGameType() == Session::GAME_TYPE_INTERNET) {
		if (myGuiInterface->getMyW()->isVisible())
			myGuiInterface->getMyW()->closeMessageBoxes();
		myGuiInterface->getMyW()->hide();
		if (!this->isVisible())
			this->show();
		if (!myGameLobbyDialog->isVisible())
			showLobbyDialog();
	}
}

void startWindowImpl::showLobbyDialog()
{
#ifdef __APPLE__
	// On macOS, show the dialog non-modally to keep menu accessible
	// Connect to finished signal to handle result
	connect(myGameLobbyDialog, &QDialog::finished, this, [this](int result) {
		disconnect(myGameLobbyDialog, &QDialog::finished, this, nullptr);
		if (result == QDialog::Accepted) {
			this->hide();
			myGuiInterface->getMyW()->networkGameModification();
		} else {
			myGameLobbyDialog->clearDialog();
			stopConnectionMonitoring();
			mySession->terminateNetworkClient();
			
			// BBCBot: Trigger auto-reconnect when lobby is closed
			if (bbcbotReconnectEnabled && !mySession->bbcbotpassword.empty()) {
				bbcbotReconnectAttempts++;
				int delaySeconds = std::min(5 + (bbcbotReconnectAttempts - 1) * 5, 30);
				std::cout << "[BBCBot] Lobby closed. Attempting reconnect #" << bbcbotReconnectAttempts 
				          << " in " << delaySeconds << " seconds..." << std::endl;
				bbcbotReconnectTimer->start(delaySeconds * 1000);
			}
		}
	}, Qt::UniqueConnection);
	myGameLobbyDialog->show();
	myGameLobbyDialog->raise();
	myGameLobbyDialog->activateWindow();
#else
	myGameLobbyDialog->exec();

	if (myGameLobbyDialog->result() == QDialog::Accepted ) {
		this->hide();
		//some gui modifications
		myGuiInterface->getMyW()->networkGameModification();
	} else {
		myGameLobbyDialog->clearDialog();
		stopConnectionMonitoring();
		mySession->terminateNetworkClient();
		
		// BBCBot: Trigger auto-reconnect when lobby is closed
		if (bbcbotReconnectEnabled && !mySession->bbcbotpassword.empty()) {
			bbcbotReconnectAttempts++;
			int delaySeconds = std::min(5 + (bbcbotReconnectAttempts - 1) * 5, 30);
			std::cout << "[BBCBot] Lobby closed. Attempting reconnect #" << bbcbotReconnectAttempts 
			          << " in " << delaySeconds << " seconds..." << std::endl;
			bbcbotReconnectTimer->start(delaySeconds * 1000);
		}
	}
#endif
}

void startWindowImpl::showNetworkStartDialog()
{
	myStartNetworkGameDialog->exec();

	if (myStartNetworkGameDialog->result() == QDialog::Accepted ) {
		this->hide();
		//some gui modifications
		myGuiInterface->getMyW()->networkGameModification();
	} else {
		mySession->terminateNetworkClient();
		if (myServerGuiInterface)
			myServerGuiInterface->getSession()->terminateNetworkServer();
	}
}

void startWindowImpl::callAboutPokerthDialog()
{
	myAboutPokerthDialog->exec();
}

void startWindowImpl::callSettingsDialogFromStartwindow()
{
	callSettingsDialog(false);
}

void startWindowImpl::callSettingsDialog(bool ingame)
{
	mySettingsDialog->exec(ingame);

	if (mySettingsDialog->result() == QDialog::Accepted && mySettingsDialog->getSettingsCorrect()) {
		myGuiInterface->getMyW()->applySettings(mySettingsDialog);
		// Apply dark mode palette based on updated config
		DarkModeHelper::applyPalette(myConfig);
		// Re-apply game table style AFTER DarkModeHelper, because
		// applyPalette() iterates all widgets with unpolish/polish/
		// setPalette which can override stylesheet-based styling.
		myGuiInterface->getMyW()->refreshGameTableStyle();
		// Update lobby dialog dark mode styling if visible
		myGameLobbyDialog->updateGameListStyleSheet();

		// Echtzeit: Chat-Übersetzung auf allen Chats anwenden (Globus/
		// Übersetzungen ein-/ausblenden), sobald "AllowChatTranslation"
		// geändert wurde – ohne Neustart.
		if(myGameLobbyDialog->getMyChat())
			myGameLobbyDialog->getMyChat()->refreshTranslationEnabled();
		if(myStartNetworkGameDialog->getMyChat())
			myStartNetworkGameDialog->getMyChat()->refreshTranslationEnabled();
		if(myGuiInterface->getMyW() && myGuiInterface->getMyW()->getMyChat())
			myGuiInterface->getMyW()->getMyChat()->refreshTranslationEnabled();
	}
}

void startWindowImpl::callLogFileDialog()
{
	myLogFileDialog->exec();
}

void startWindowImpl::showTimeoutDialog(int msgID, unsigned duration)
{
	// BBCBot: Automatically reset timeout without showing dialog
	if (!mySession->bbcbotpassword.empty()) {
		mySession->resetNetworkTimeout();
		return;
	}
	
	if(myTimeoutDialog->isHidden()) {
		myTimeoutDialog->setMySession(mySession);
		myTimeoutDialog->setMsgID((NetTimeoutReason)msgID);
		myTimeoutDialog->setTimeoutDuration(duration);
		myTimeoutDialog->show();
		myTimeoutDialog->raise();
		myTimeoutDialog->activateWindow();
		myTimeoutDialog->startTimeout();

		// Audio-Hinweis zum AFK-Countdown (Lobby wie ingame) – das Popup
		// kann hinter anderen Fenstern liegen oder übersehen werden.
		if(myGuiInterface && myGuiInterface->getMyW()
		   && myGuiInterface->getMyW()->getMySoundEventHandler()) {
			myGuiInterface->getMyW()->getMySoundEventHandler()->playSound("yourturn", 0);
		}
	}
}

void startWindowImpl::hideTimeoutDialog()
{
	myTimeoutDialog->hide();
}

void startWindowImpl::handleStatsUpdate(ServerStats stats)
{
	// Forward to lobby dialog
	myGameLobbyDialog->updateStats(stats);
	
	// Update activity timestamp
	updateServerActivity();
}

void startWindowImpl::updateServerActivity()
{
	// Update last activity timestamp for connection monitoring (monotonic clock)
	lastServerActivityTimer.restart();
	
	// Start monitoring if not already active
	if (!connectionMonitoringActive) {
		connectionMonitoringActive = true;
		connectionHeartbeatTimer->start(30000); // Check every 30 seconds
	}
}

void startWindowImpl::stopConnectionMonitoring()
{
	connectionMonitoringActive = false;
	missedHeartbeats = 0;
	connectionHeartbeatTimer->stop();
}

void startWindowImpl::connectionHeartbeatCheck()
{
	if (!connectionMonitoringActive) {
		return;
	}
	
	// Server sends stats heartbeat every 45 seconds. In-game signals
	// (hand start/end, player actions) also update the activity timestamp.
	// Use a 180s window (~4x the heartbeat interval) to tolerate
	// network jitter, server load spikes, and brief WiFi suspensions
	// that are common on Windows laptops, while detecting truly dead
	// connections in a reasonable time (~4 min worst case with 3 misses).
	// QElapsedTimer uses a monotonic clock, immune to NTP/DST/sleep clock jumps
	// that caused false disconnects on Windows.
	qint64 elapsedMs = lastServerActivityTimer.elapsed(); // milliseconds
	if (elapsedMs > 180000) {
		// Require three consecutive missed checks before declaring connection lost.
		// This avoids false positives from transient network issues.
		missedHeartbeats++;
		if (missedHeartbeats >= 3) {
			showConnectionLostDialog();
		}
	} else {
		missedHeartbeats = 0;
	}
}

void startWindowImpl::showConnectionLostDialog()
{
	// Guard against re-entrant calls from nested event loops
	if (connectionLostHandlingActive) {
		return;
	}
	connectionLostHandlingActive = true;

	// Stop monitoring
	stopConnectionMonitoring();

	// BBCBot: auto-reconnect instead of showing dialog
	if (bbcbotReconnectEnabled && !mySession->bbcbotpassword.empty()) {
		std::cout << "[BBCBot] Connection lost (silent disconnect). Triggering reconnect..." << std::endl;
		bbcbotReconnectAttempts++;
		int delaySeconds = std::min(5 + (bbcbotReconnectAttempts - 1) * 5, 30);
		std::cout << "[BBCBot] Reconnect #" << bbcbotReconnectAttempts 
		          << " in " << delaySeconds << " seconds..." << std::endl;
		
		// Close lobby and terminate broken connection
		if (myGameLobbyDialog && myGameLobbyDialog->isVisible()) {
			myGameLobbyDialog->close();
		}
		mySession->terminateNetworkClient();
		
		// Schedule reconnect
		bbcbotReconnectTimer->start(delaySeconds * 1000);
		return;
	}

	// Stop all game table animation timers BEFORE terminating the network
	// client. The modal dialog below runs a nested Qt event loop, during
	// which pending timer events would fire and access the now-invalid
	// game/network state, causing a crash.
	myGuiInterface->getMyW()->stopTimer();

	// Terminate the connection BEFORE showing the modal dialog.
	// The modal dialog runs a nested event loop, during which
	// incoming network signals would otherwise be processed,
	// causing duplicate kick/error dialogs.
	mySession->terminateNetworkClient();

	// Show warning to user
	MyMessageBox::warning(this, tr("Connection Lost"),
						  tr("The connection to the server was lost.\nPlease reconnect to continue."),
						  QMessageBox::Ok);

	connectionLostHandlingActive = false;
}

void startWindowImpl::networkError(int errorID, int /*osErrorID*/)
{
	// Suppress errors arriving while heartbeat connection-lost dialog is active
	if (connectionLostHandlingActive) {
		return;
	}

	// Stop connection monitoring
	stopConnectionMonitoring();

	hideTimeoutDialog();
	switch (errorID) {
	case ERR_SOCK_SERVERADDR_NOT_SET: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("Server address was not set."),
							  QMessageBox::Close);
	}
	break;
	case ERR_SOCK_INVALID_PORT: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("An invalid port was set (ports 0-1023 are not allowed)."),
							  QMessageBox::Close);
	}
	break;
	case ERR_SOCK_CREATION_FAILED: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("Could not create a socket for TCP communication."),
							  QMessageBox::Close);
	}
	break;
	case ERR_SOCK_SET_ADDR_FAILED: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("Could not set the IP address."),
							  QMessageBox::Close);
	}
	break;
	case ERR_SOCK_SET_PORT_FAILED: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("Could not set the port for this type of address."),
							  QMessageBox::Close);
	}
	break;
	case ERR_SOCK_RESOLVE_FAILED: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("The server name could not be resolved."),
							  QMessageBox::Close);
	}
	break;
	case ERR_SOCK_BIND_FAILED: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("Bind failed - please choose a different port."),
							  QMessageBox::Close);
	}
	break;
	case ERR_SOCK_LISTEN_FAILED: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("Internal network error: \"listen\" failed."),
							  QMessageBox::Close);
	}
	break;
	case ERR_SOCK_ACCEPT_FAILED: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("Server execution was terminated."),
							  QMessageBox::Close);
	}
	break;
	case ERR_SOCK_CONNECT_FAILED: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("Could not connect to the server.\n\nThe server might still be processing a previous connection attempt.\nPlease wait a moment and try again in about 1 minute.\n\nIf the problem persists, the server might be temporarily unavailable."),
							  QMessageBox::Close);
	}
	break;
	case ERR_SOCK_CONNECT_TIMEOUT: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("Connection timed out.\n\nThe server might be busy or still processing a previous connection.\nPlease wait a moment and try again in about 1 minute.\n\nIf the problem persists, please check the server address or try again later."),
							  QMessageBox::Close);
	}
	break;
	case ERR_SOCK_SELECT_FAILED: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("Internal network error: \"select\" failed."),
							  QMessageBox::Close);
	}
	break;
	case ERR_SOCK_SEND_FAILED: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("Internal network error: \"send\" failed."),
							  QMessageBox::Close);
	}
	break;
	case ERR_SOCK_RECV_FAILED: // Sometimes windows reports recv failed on close.
	case ERR_SOCK_CONN_RESET: {
		// BBCBot: Automatic reconnect on connection loss
		if (bbcbotReconnectEnabled && !mySession->bbcbotpassword.empty()) {
			bbcbotReconnectAttempts++;
			int delaySeconds = std::min(5 + (bbcbotReconnectAttempts - 1) * 5, 30); // 5s, 10s, 15s, ... max 30s
			std::cout << "[BBCBot] Connection lost. Attempting reconnect #" << bbcbotReconnectAttempts 
			          << " in " << delaySeconds << " seconds..." << std::endl;
			bbcbotReconnectTimer->start(delaySeconds * 1000);
		} else {
			MyMessageBox::warning(this, tr("Network Error"),
								  tr("The connection to the server was lost."),
								  QMessageBox::Close);
		}
	}
	break;
	case ERR_SOCK_CONN_EXISTS: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("Internal network error: Duplicate TCP connection."),
							  QMessageBox::Close);
	}
	break;
	case ERR_SOCK_INVALID_PACKET: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("An invalid network packet was received.\nPlease make sure that all players use the same version of PokerTH."),
							  QMessageBox::Close);
	}
	break;
	case ERR_SOCK_INVALID_STATE: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("Internal state error.\nPlease make sure that all players use the same version of PokerTH."),
							  QMessageBox::Close);
	}
	break;
	case ERR_SOCK_INVALID_SERVERLIST_URL:
	case ERR_SOCK_TRANSFER_INVALID_URL: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("Invalid server list URL.\nPlease correct the address in the settings."),
							  QMessageBox::Close);
	}
	break;
	case ERR_SOCK_INVALID_SERVERLIST_XML: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("The PokerTH internet server list contains invalid data.\nIf you use a custom server list, please make sure its format is correct."),
							  QMessageBox::Close);
	}
	break;
	case ERR_SOCK_UNZIP_FAILED: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("Could not unzip the PokerTH internet server list."),
							  QMessageBox::Close);
	}
	break;
	case ERR_SOCK_TRANSFER_INIT_FAILED:
	case ERR_SOCK_TRANSFER_SELECT_FAILED:
	case ERR_SOCK_TRANSFER_FAILED: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("Could not download the PokerTH internet server list.\nPlease make sure you are directly connected to the internet."),
							  QMessageBox::Close);
	}
	break;
	case ERR_SOCK_TRANSFER_OPEN_FAILED: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("Could not open the target file when downloading the server list."),
							  QMessageBox::Close);
	}
	break;
	case ERR_NET_VERSION_NOT_SUPPORTED: {
		MyMessageBox msgBox(QMessageBox::Warning, tr("Network Error"),
							tr("The PokerTH server does not support this version of the game.<br>Please go to <a href=\"https://www.pokerth.net/\" target=\"_blank\">https://www.pokerth.net</a> and download the latest version."),
							QMessageBox::Close, this);
		msgBox.setTextFormat(Qt::RichText);
		msgBox.exec();
	}
	break;
	case ERR_NET_SERVER_FULL: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("Sorry, this server is already full."),
							  QMessageBox::Close);
	}
	break;
	case ERR_NET_INVALID_PASSWORD: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("Invalid login.\nPlease check your username and password."),
							  QMessageBox::Close);
	}
	break;
	case ERR_NET_INVALID_PASSWORD_STR: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("The password is too long. Please choose another one."),
							  QMessageBox::Close);
	}
	break;
	case ERR_NET_PLAYER_NAME_IN_USE: {
		changeContentDialogImpl dialog(this, myConfig, CHANGE_NICK_ALREADY_IN_USE);
		dialog.exec();
	}
	break;
	case ERR_NET_INVALID_PLAYER_NAME: {
		changeContentDialogImpl dialog(this, myConfig, CHANGE_NICK_INVALID);
		dialog.exec();
	}
	break;
	case ERR_NET_INVALID_GAME_NAME: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("The game name is either too short or too long. Please choose another one."),
							  QMessageBox::Close);
	}
	break;
	case ERR_NET_UNKNOWN_GAME: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("The game could not be found."),
							  QMessageBox::Close);
	}
	break;
	case ERR_NET_INVALID_CHAT_TEXT: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("The chat text is invalid."),
							  QMessageBox::Close);
	}
	break;
	case ERR_NET_UNKNOWN_PLAYER_ID: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("The server referred to an unknown player. Aborting."),
							  QMessageBox::Close);
	}
	break;
	case ERR_NET_NO_CURRENT_PLAYER: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("Internal error: The current player could not be found."),
							  QMessageBox::Close);
	}
	break;
	case ERR_NET_PLAYER_NOT_ACTIVE: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("Internal error: The current player is not active."),
							  QMessageBox::Close);
	}
	break;
	case ERR_NET_PLAYER_KICKED: {
		mySession->terminateNetworkClient();
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("You were kicked from the server."),
							  QMessageBox::Close);
	}
	break;
	case ERR_NET_PLAYER_BANNED: {
		mySession->terminateNetworkClient();
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("You were temporarily banned from the server."),
							  QMessageBox::Close);
	}
	break;
	case ERR_NET_PLAYER_BLOCKED: {
		mySession->terminateNetworkClient();
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("Your account is blocked indefinitely."),
							  QMessageBox::Close);
	}
	break;
	case ERR_NET_SESSION_TIMED_OUT: {
		mySession->terminateNetworkClient();
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("Your server connection timed out due to inactivity. You are very welcome to reconnect!"),
							  QMessageBox::Close);
	}
	break;
	case ERR_NET_INVALID_PLAYER_COUNT: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("The client player count is invalid."),
							  QMessageBox::Close);
	}
	break;
	case ERR_NET_TOO_MANY_MANUAL_BLINDS: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("Too many manual blinds were set. Please reconfigure the manual blinds."),
							  QMessageBox::Close);
	}
	break;
	case ERR_NET_INVALID_AVATAR_FILE:
	case ERR_NET_WRONG_AVATAR_SIZE: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("An invalid avatar file was configured. Please choose a different avatar."),
							  QMessageBox::Close);
	}
	break;
	case ERR_NET_AVATAR_TOO_LARGE: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("The selected avatar file is too large. Please choose a different avatar."),
							  QMessageBox::Close);
	}
	break;
	case ERR_NET_INIT_BLOCKED: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("You cannot login at this time. Please try again in a few seconds."),
							  QMessageBox::Close);
	}
	break;
	case ERR_NET_INVALID_REQUEST_ID: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("An internal avatar error occured. Please report this to an admin in the lobby chat."),
							  QMessageBox::Close);
	}
	break;
	case ERR_NET_START_TIMEOUT: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("Could not start game: Synchronization failed."),
							  QMessageBox::Close);
	}
	break;
	case ERR_NET_SERVER_MAINTENANCE: {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("The server is down for maintenance. Please try again later."),
							  QMessageBox::Close);
	}
	break;
	default:  {
		MyMessageBox::warning(this, tr("Network Error"),
							  tr("An internal error occured."),
							  QMessageBox::Close);
	}
	}

	// Always make sure the network client is terminated after any error.
	// Without this, the ClientThread survives as a zombie after socket-level
	// errors (ERR_SOCK_CONN_RESET, ERR_SOCK_SEND_FAILED etc.), blocking
	// any subsequent reconnect/rejoin attempt.
	mySession->terminateNetworkClient();

	// Stop all game table animation timers BEFORE closing dialogs.
	// Rejecting the lobby dialog while exec() is running causes
	// terminateNetworkClient() -> currentGame.reset(). Any animation
	// timer callback that fires afterwards would dereference null.
	myGuiInterface->getMyW()->stopTimer();
	// close dialogs
	myGameLobbyDialog->reject();
	myConnectToServerDialog->reject();
	myStartNetworkGameDialog->reject();
	myGuiInterface->getMyW()->close();
	myInternetGameLoginDialog->reject();

}

void startWindowImpl::networkNotification(int notificationId)
{
	// Suppress notifications arriving while heartbeat connection-lost dialog is active
	if (connectionLostHandlingActive) {
		return;
	}

	hideTimeoutDialog();
	switch (notificationId) {
	case NTF_NET_JOIN_IP_BLOCKED: {
		MyMessageBox::warning(this, tr("Network Notification"),
							  tr("You cannot join this game, because another player in that game has your network address."),
							  QMessageBox::Close);
	}
	break;
	case NTF_NET_REMOVED_START_FAILED: {
		myGameLobbyDialog->stopWaitStartGameMsgBoxTimer();
		if(MyMessageBox::warning(this, tr("Network Notification"),
								 tr("Your connection to the server is very slow, the game had to start without you."),
								 QMessageBox::Close) == QMessageBox::Close) {
			myGameLobbyDialog->hideWaitStartGameMsgBox();
		}
	}
	break;
	case NTF_NET_REMOVED_KICKED: {
		MyMessageBox::warning(this, tr("Network Notification"),
							  tr("You were kicked from the game."),
							  QMessageBox::Close);
	}
	break;
	case NTF_NET_REMOVED_GAME_FULL:
	case NTF_NET_JOIN_GAME_FULL: {
		MyMessageBox::warning(this, tr("Network Notification"),
							  tr("Sorry, this game is already full."),
							  QMessageBox::Close);
	}
	break;
	case NTF_NET_REMOVED_ALREADY_RUNNING:
	case NTF_NET_JOIN_ALREADY_RUNNING: {
		MyMessageBox::warning(this, tr("Network Notification"),
							  tr("Unable to join - the server has already started the game."),
							  QMessageBox::Close);
	}
	break;
	case NTF_NET_JOIN_NOT_INVITED: {
		MyMessageBox::warning(this, tr("Network Notification"),
							  tr("This game is of type invite-only. You cannot join this game without being invited."),
							  QMessageBox::Close);
	}
	break;
	case NTF_NET_JOIN_GAME_NAME_IN_USE: {
		changeContentDialogImpl dialog(this, myConfig, CHANGE_INET_GAME_NAME_IN_USE);
		dialog.exec();
		if(dialog.result() == QDialog::Accepted) {
			myGameLobbyDialog->pushButton_CreateGame->click();
		}
	}
	break;
	case NTF_NET_JOIN_GAME_BAD_NAME: {
		changeContentDialogImpl dialog(this, myConfig, CHANGE_INET_BAD_GAME_NAME);
		dialog.exec();
		if(dialog.result() == QDialog::Accepted) {
			myGameLobbyDialog->pushButton_CreateGame->click();
		}
	}
	break;
	case NTF_NET_REMOVED_TIMEOUT: {
		MyMessageBox::warning(this, tr("Network Notification"),
							  tr("You were removed due to inactivity."),
							  QMessageBox::Close);
	}
	break;
	case NTF_NET_JOIN_INVALID_PASSWORD: {
		MyMessageBox::warning(this, tr("Network Notification"),
							  tr("Invalid password when joining the game.\nPlease reenter the password and try again."),
							  QMessageBox::Close);
	}
	break;
	case NTF_NET_JOIN_GUEST_FORBIDDEN: {
		MyMessageBox::warning(this, tr("Network Notification"),
							  tr("You cannot join this type of game as guest."),
							  QMessageBox::Close);
	}
	break;
	case NTF_NET_JOIN_INVALID_SETTINGS: {
		MyMessageBox::warning(this, tr("Network Notification"),
							  tr("The settings are invalid for this type of game."),
							  QMessageBox::Close);
	}
	break;
	case NTF_NET_NEW_RELEASE_AVAILABLE: {
		msgBoxOutdatedVersion.setIcon(QMessageBox::Information);
		msgBoxOutdatedVersion.setWindowTitle(tr("Network Notification"));
		msgBoxOutdatedVersion.setText(tr("A new release of PokerTH is available.<br>Please go to <a href=\"https://www.pokerth.net/\" target=\"_blank\">https://www.pokerth.net</a> and download the latest version."));
		msgBoxOutdatedVersion.setTextFormat(Qt::RichText);
		msgBoxOutdatedVersion.setStandardButtons(QMessageBox::Ok);
		msgBoxOutdatedVersion.setDefaultButton(QMessageBox::Ok);

		msgBoxOutdatedVersionActive = true;
	}
	break;
	case NTF_NET_OUTDATED_BETA: {
		msgBoxOutdatedVersion.setIcon(QMessageBox::Information);
		msgBoxOutdatedVersion.setWindowTitle(tr("Network Notification"));
		msgBoxOutdatedVersion.setText(tr("This beta release of PokerTH is outdated.<br>Please go to <a href=\"https://www.pokerth.net/\" target=\"_blank\">https://www.pokerth.net</a> and download the latest version."));
		msgBoxOutdatedVersion.setTextFormat(Qt::RichText);
		msgBoxOutdatedVersion.setStandardButtons(QMessageBox::Ok);
		msgBoxOutdatedVersion.setDefaultButton(QMessageBox::Ok);

		msgBoxOutdatedVersionActive = true;
	}
	break;
	}
}

void startWindowImpl::networkMessage(QString msg)
{
	MyMessageBox msgBox(QMessageBox::Information, tr("Server Message"),
						msg, QMessageBox::Close, this);
	msgBox.setTextFormat(Qt::RichText);
	msgBox.exec();
}

void startWindowImpl::networkMessage(unsigned msgId)
{
	QString msgText;
	bool showMsgBox = true;

	switch(msgId) {

	case MSG_NET_AVATAR_REPORT_ACCEPTED: {
		msgText = tr("The avatar report was accepted by the server. Thank you.");
	}
	break;
	case MSG_NET_AVATAR_REPORT_DUP: {
		msgText = tr("This avatar was already reported by another player.");
	}
	break;
	case MSG_NET_AVATAR_REPORT_REJECTED: {
		msgText = tr("An error occurred while reporting the avatar.");
	}
	break;
	case MSG_NET_GAMENAME_REPORT_ACCEPTED: {
		msgText = tr("The game name report was accepted by the server. Thank you.");
	}
	break;
	case MSG_NET_GAMENAME_REPORT_DUP: {
		msgText = tr("This game name was already reported by another player.");
	}
	break;
	case MSG_NET_GAMENAME_REPORT_REJECTED: {
		msgText = tr("An error occurred while reporting the game name.");
	}
	break;
	case MSG_NET_ADMIN_REMOVE_GAME_ACCEPTED: {
		msgText = tr("The game was closed.");
	}
	break;
	case MSG_NET_ADMIN_REMOVE_GAME_REJECTED: {
		msgText = tr("The game could not be closed.");
	}
	break;
	case MSG_NET_ADMIN_BAN_PLAYER_ACCEPTED: {
		msgText = tr("The player was kicked and banned permanently.");
	}
	break;
	case MSG_NET_ADMIN_BAN_PLAYER_NODB: {
		msgText = tr("The player was kicked, but could not be banned because it was a guest player.");
	}
	break;
	case MSG_NET_ADMIN_BAN_PLAYER_DBERROR: {
		msgText = tr("The player was kicked, but could not be banned, \nbecause the nick could not be found in the database");
	}
	break;
	case MSG_NET_ADMIN_BAN_PLAYER_REJECTED: {
		msgText = tr("The player could not be found.");
	}
	break;
	default:
		showMsgBox = false;
		break;
	}

	if(showMsgBox) {
		MyMessageBox msgBox(QMessageBox::Information, tr("Server Message"),
							msgText, QMessageBox::Close, this);
		//    msgBox.setTextFormat(Qt::RichText);
		msgBox.exec();
	}

}


void startWindowImpl::networkStart(boost::shared_ptr<Game> game)
{
	mySession->startClientGame(game);

	//send playerNicksList to chat for nick-autocompletition
	myGuiInterface->getMyW()->getMyChat()->setPlayerNicksList(getPlayerNicksList());
}


QStringList startWindowImpl::getPlayerNicksList()
{

	QStringList list;
	PlayerListConstIterator it_c;
	PlayerList seatList = mySession->getCurrentGame()->getSeatsList();
	for (it_c=seatList->begin(); it_c!=seatList->end(); ++it_c) {

		list << QString::fromUtf8((*it_c)->getMyName().c_str());
	}

	return list;
}

QString startWindowImpl::checkForFirstStartAfterUpdated()
{
	if(myConfig->getConfigState() == OLD) {

		if(POKERTH_VERSION_MAJOR == 0 && POKERTH_VERSION_MINOR == 80) {
			//version 0.8 HACK
			//to avoid old PokerTH distributed styles pathes in settings which leads to error message like "outdated" prepare settings dialog (fallback will correct this issue) and save settings
			mySettingsDialog->prepareDialog();
			mySettingsDialog->isAccepted();
			myGuiInterface->getMyW()->applySettings(mySettingsDialog);
		}
		return QString("Update old config to version %1").arg(QString::number(POKERTH_VERSION_MAJOR)+"."+QString::number(POKERTH_VERSION_MINOR));
	}

	return QString();

}

void startWindowImpl::bbcbotAttemptReconnect()
{
	std::cout << "[BBCBot] Attempting to reconnect to server..." << std::endl;
	
	// Close any existing dialogs
	if (myGameLobbyDialog && myGameLobbyDialog->isVisible()) {
		myGameLobbyDialog->close();
	}
	if (myStartNetworkGameDialog && myStartNetworkGameDialog->isVisible()) {
		myStartNetworkGameDialog->close();
	}
	
	// Terminate the existing client connection
	if (mySession) {
		mySession->terminateNetworkClient();
	}
	
	// Trigger reconnect after a short delay to allow cleanup
	// Use callGameLobbyDialog which will handle the connection setup
	QTimer::singleShot(500, this, SLOT(callGameLobbyDialog()));
}

void startWindowImpl::bbcbotResetReconnectAttempts()
{
	if (bbcbotReconnectEnabled) {
		std::cout << "[BBCBot] Connection successful. Resetting reconnect attempts counter." << std::endl;
		bbcbotReconnectAttempts = 0;
		// Reset activity timestamp on successful connection
		lastServerActivityTimer.restart();
	}
}

bool startWindowImpl::eventFilter(QObject *obj, QEvent *event)
{
	if (event->type() == QEvent::Close) {
		event->ignore();
		//        mySession->getMyLog()->closeLogDbAtExit();
		return QMainWindow::eventFilter(obj, event);
	} else {
		// pass the event on to the parent class
		return QMainWindow::eventFilter(obj, event);
	}
}

void startWindowImpl::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::WindowStateChange
		|| event->type() == QEvent::ScreenChangeInternal) {
		// After hibernate/resume the window manager may report a different
		// geometry or DPI.  Force a re-layout so the UI matches the window.
		if (layout()) {
			layout()->invalidate();
			layout()->activate();
		}
		update();
	}
	QMainWindow::changeEvent(event);
}

void startWindowImpl::onScreenChanged(QScreen *screen)
{
	if (screen) {
		connect(screen, &QScreen::geometryChanged,
			this, &startWindowImpl::onScreenGeometryChanged, Qt::UniqueConnection);
		connect(screen, &QScreen::logicalDotsPerInchChanged,
			this, &startWindowImpl::onScreenDpiChanged, Qt::UniqueConnection);
	}
	// Force re-layout after screen change (e.g. hibernate/resume)
	if (layout()) {
		layout()->invalidate();
		layout()->activate();
	}
	update();
}

void startWindowImpl::onScreenGeometryChanged(const QRect & /*geometry*/)
{
	if (layout()) {
		layout()->invalidate();
		layout()->activate();
	}
	update();
}

void startWindowImpl::onScreenDpiChanged(qreal /*dpi*/)
{
	if (layout()) {
		layout()->invalidate();
		layout()->activate();
	}
	update();
}
