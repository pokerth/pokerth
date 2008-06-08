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
#ifndef GAMETABLEIMPL_H
#define GAMETABLEIMPL_H

#include "ui_gametable.h"
#include "game_defs.h"

#include <string>
#include <boost/shared_ptr.hpp>

#include <QtGui>
#include <QtCore>

class Log;
class Chat;
class ConfigFile;
class Session;
class Game;

class GuiInterface;
class BoardInterface;
// class HandInterface;
class PlayerInterface;
class MyCardsPixmapLabel;
class MyAvatarLabel;
class newGameDialogImpl;
class settingsDialogImpl;
class selectAvatarDialogImpl;
class aboutPokerthImpl;
class joinNetworkGameDialogImpl;
class connectToServerDialogImpl;
class createNetworkGameDialogImpl;
class startNetworkGameDialogImpl;
class changeHumanPlayerNameDialogImpl;
class gameLobbyDialogImpl;
class timeoutMsgBoxImpl;
class startWindowImpl;

class QColor;
class SDLPlayer;
// class StyleSheetReader;


class gameTableImpl: public QMainWindow, public Ui::gameTable {
Q_OBJECT

public:
	gameTableImpl(ConfigFile *c = 0, QMainWindow *parent = 0);

	~gameTableImpl();

	Session &getSession();
	void setSession(boost::shared_ptr<Session> session);
	void setStartWindow(boost::shared_ptr<startWindowImpl> s) { myStartWindow = s; }

	void setLog(Log* l) { myLog = l; }
	
	SDLPlayer* getMySDLPlayer() const { return mySDLPlayer; }
	
	void setSpeeds();

signals:
	void signalInitGui(int speed);

	void signalShowClientDialog();

	void signalRefreshSet();
	void signalRefreshCash();
	void signalRefreshAction(int =-1, int=-1);
	void signalRefreshChangePlayer();
	void signalRefreshPot();
	void signalRefreshGroupbox(int =-1, int =-1);
	void signalRefreshAll();
	void signalRefreshPlayerName();
	void signalRefreshButton();
	void signalRefreshGameLabels(int);

	void signalSetPlayerAvatar(int, QString);

	void signalGuiUpdateDone();

	void signalMeInAction();
	void signalUpdateMyButtonsState();
	void signalDisableMyButtons();
	void signalStartTimeoutAnimation(int playerId, int timeoutSec);
	void signalStopTimeoutAnimation(int playerId);

	void signalDealBeRoCards(int myBeRoID);

	void signalDealHoleCards();
	void signalDealFlopCards0();
	void signalDealTurnCards0();
	void signalDealRiverCards0();

	void signalNextPlayerAnimation();

	void signalBeRoAnimation2(int);

	void signalPreflopAnimation1();
	void signalPreflopAnimation2();
	void signalFlopAnimation1();
	void signalFlopAnimation2();
	void signalTurnAnimation1();
	void signalTurnAnimation2();
	void signalRiverAnimation1();
	void signalRiverAnimation2();
	void signalPostRiverAnimation1();
	void signalPostRiverRunAnimation1();

	void signalFlipHolecardsAllIn();

	void signalNextRoundCleanGui();

	void signalNetClientConnect(int actionID);
	void signalNetClientGameInfo(int actionID);
	void signalNetClientError(int errorID, int osErrorID);
	void signalNetClientNotification(int notificationId);
	void signalNetClientStatsUpdate(ServerStats stats);
	void signalNetClientShowTimeoutDialog(int, unsigned);
	void signalNetClientRemovedFromGame(int notificationId);
	void signalNetServerError(int errorID, int osErrorID);
	void signalNetClientSelfJoined(unsigned playerId, QString playerName, int rights);
	void signalNetClientPlayerJoined(unsigned playerId, QString playerName, int rights);
	void signalNetClientPlayerChanged(unsigned playerId, QString newPlayerName);
	void signalNetClientPlayerLeft(unsigned playerId, QString playerName);
	void signalNetClientNewGameAdmin(unsigned playerId, QString playerName);
	void signalNetClientGameListNew(unsigned gameId);
	void signalNetClientGameListRemove(unsigned gameId);
	void signalNetClientGameListUpdateMode(unsigned gameId, int mode);
	void signalNetClientGameListUpdateAdmin(unsigned gameId, unsigned adminPlayerId);
	void signalNetClientGameListPlayerJoined(unsigned gameId, unsigned playerId);
	void signalNetClientGameListPlayerLeft(unsigned gameId, unsigned playerId);
	void signalNetClientGameStart(boost::shared_ptr<Game> game);
	void signalNetClientChatMsg(QString nickName, QString msg);

	void signalIrcConnect(QString server);
	void signalIrcSelfJoined(QString nickName, QString channel);
	void signalIrcPlayerJoined(QString nickName);
	void signalIrcPlayerChanged(QString oldNick, QString newNick);
	void signalIrcPlayerKicked(QString nickName, QString byWhom, QString reason);
	void signalIrcPlayerLeft(QString nickName);
	void signalIrcChatMessage(QString nickName, QString msg);
	void signalIrcError(int errorCode);
	void signalIrcServerError(int errorCode);

public slots:

	void initGui(int speed);

	void showClientDialog();
	void showNetworkStartDialog();
	void showLobbyDialog();

	//refresh-Funktionen
	void refreshSet();
	void refreshCash();
	void refreshAction(int =-1, int=-1);
	void refreshChangePlayer();
	void refreshPot();
	void refreshGroupbox(int =-1, int =-1);
	void refreshAll();
	void refreshPlayerName();
	QStringList getPlayerNicksList();
	void refreshGameLabels(int);
	void refreshButton();
	void refreshPlayerAvatar();
	void setPlayerAvatar(int myID, QString myAvatar);

	void guiUpdateDone();
	void waitForGuiUpdateDone();

	// Karten-Funktionen
	void dealHoleCards();

	//Spieler-Funktionen
	void provideMyActions(int mode = -1);  //mode 0 == called from dealberocards
	void meInAction();
	void disableMyButtons();
	void startTimeoutAnimation(int playerId, int timoutSec);
	void stopTimeoutAnimation(int playerId);

	void setGameSpeed(const int theValue) { guiGameSpeed = theValue; setSpeeds(); } // Achtung Faktor 10!!!

	void callNewGameDialog() ;
	void callAboutPokerthDialog();
	void callSettingsDialog();
	void callCreateNetworkGameDialog();
	void callJoinNetworkGameDialog();
	void callGameLobbyDialog();
	void joinGameLobby();

	void pushButtonBetRaiseClicked(bool checked);
	void pushButtonCallCheckClicked(bool checked);
	void pushButtonFoldClicked(bool checked);
	void pushButtonAllInClicked(bool checked);

	void myCallCheck();

	void myFold();
	void myCheck();
	int getMyCallAmount();
	int getBetRaisePushButtonValue();
	int getMyBetAmount();
	void myCall();
	void mySet();
	void myAllIn();

	void myActionDone();

	void dealBeRoCards(int);

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

	void beRoAnimation2(int);

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
	void postRiverRunAnimation2_flipHoleCards1();
	void postRiverRunAnimation2_flipHoleCards2();
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
	bool eventFilter(QObject *obj, QEvent *event);

	void switchChatWindow();
	void switchHelpWindow();
	void switchLogWindow();
	void switchAwayWindow();
	void switchFullscreen();

	void paintStartSplash();

	void sendChatMessage();
	void checkChatInputLength(QString);
	void tabSwitchAction();

	void leaveCurrentNetworkGame();

	void networkError(int, int);
	void networkNotification(int);
	void networkStart(boost::shared_ptr<Game> game);

// 	void closeEvent(QCloseEvent*);
	
	void localGameModification();
	void networkGameModification();

	void mouseOverFlipCards(bool front);

	void updateMyButtonsState(int mode = -1); //mode 0 == called from dealberocards
	void uncheckMyButtons();
	void resetMyButtonsCheckStateMemory();
	void clearMyButtons();
	void myButtonsCheckable(bool state);

	void changePlayingMode();
	void changeLineEditBetValue(int);
	void lineEditBetValueChanged(QString);
	
	void showMaximized ();
	void quitPokerTH();

	void showTimeoutDialog(int msgID, unsigned duration);
	void hideTimeoutDialog();
	
// 	void paintEvent(QPaintEvent *);

private: 

	boost::shared_ptr<GuiInterface> myServerGuiInterface;
	boost::shared_ptr<Session> mySession;
	Log *myLog;
	Chat *myChat;
	ConfigFile *myConfig;
	
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
	QTimer *postRiverRunAnimation2_flipHoleCards1Timer;
	QTimer *postRiverRunAnimation2_flipHoleCards2Timer;
	QTimer *postRiverRunAnimation3Timer;
	QTimer *postRiverRunAnimation5Timer;
	QTimer *postRiverRunAnimation6Timer;

	QTimer *blinkingStartButtonAnimationTimer;
	

	QWidget *userWidgetsArray[6];
	QLabel *buttonLabelArray[MAX_NUMBER_OF_PLAYERS];
	QLabel *cashLabelArray[MAX_NUMBER_OF_PLAYERS];
	QLabel *cashTopLabelArray[MAX_NUMBER_OF_PLAYERS];
	MySetLabel *setLabelArray[MAX_NUMBER_OF_PLAYERS];
	QLabel *actionLabelArray[MAX_NUMBER_OF_PLAYERS];
	QLabel *playerNameLabelArray[MAX_NUMBER_OF_PLAYERS];
	MyAvatarLabel *playerAvatarLabelArray[MAX_NUMBER_OF_PLAYERS];

	QGroupBox *groupBoxArray[MAX_NUMBER_OF_PLAYERS];
	MyCardsPixmapLabel *boardCardsArray[5];
	MyCardsPixmapLabel *holeCardsArray[MAX_NUMBER_OF_PLAYERS][2];

	QPixmap *flipside;

// 	Dialogs
	boost::shared_ptr<aboutPokerthImpl> myAboutPokerthDialog;
	boost::shared_ptr<newGameDialogImpl> myNewGameDialog;
	boost::shared_ptr<settingsDialogImpl> mySettingsDialog;
	boost::shared_ptr<selectAvatarDialogImpl> mySelectAvatarDialog;
	boost::shared_ptr<changeHumanPlayerNameDialogImpl> myChangeHumanPlayerNameDialog;
	boost::shared_ptr<joinNetworkGameDialogImpl> myJoinNetworkGameDialog;
	boost::shared_ptr<connectToServerDialogImpl> myConnectToServerDialog;
	boost::shared_ptr<startNetworkGameDialogImpl> myStartNetworkGameDialog;
	boost::shared_ptr<createNetworkGameDialogImpl> myCreateNetworkGameDialog;
	boost::shared_ptr<gameLobbyDialogImpl> myGameLobbyDialog;
	boost::shared_ptr<timeoutMsgBoxImpl> myTimeoutDialog;
	boost::shared_ptr<startWindowImpl> myStartWindow;

	//Sound
	SDLPlayer *mySDLPlayer;
	QString myAppDataPath;


	int distributePotAnimCounter;
	int playingMode;
	
	QString font2String;
	QString font1String;

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

	bool myActionIsBet;
	bool myActionIsRaise;
	bool pushButtonBetRaiseIsChecked;
	bool pushButtonCallCheckIsChecked;
	bool pushButtonFoldIsChecked;
	bool pushButtonAllInIsChecked;
	bool myButtonsAreCheckable;
	bool breakAfterCurrentHand;
	bool currentGameOver;	
	bool flipHolecardsAllInAlreadyDone;
	bool betSliderChangedByInput;

	QColor active;
	QColor inactive;
	QColor highlight;

	// statistic testing
	int statisticArray[15];

	QSemaphore guiUpdateSemaphore;

	int keyUpDownChatCounter;
	int myLastPreActionBetValue;

// 	StyleSheetReader *myStyleSheetReader;

friend class GuiWrapper;
};

#endif
