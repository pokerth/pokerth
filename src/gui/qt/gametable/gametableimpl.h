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
#ifndef GAMETABLEIMPL_H
#define GAMETABLEIMPL_H

#ifdef GUI_800x480
#include "ui_gametable_800x480.h"
#include "ui_tabs_800x480.h"
#else
#include "ui_gametable.h"
#endif
#include "game_defs.h"

#include <string>

#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif

#include <QtGui>
#include <QtCore>
#if QT_VERSION >= 0x050000
#include <QtWidgets>
#endif

class guiLog;
class ChatTools;
class ConfigFile;
class Session;
class Game;

class GuiInterface;
class BoardInterface;
class PlayerInterface;
class MyCardsPixmapLabel;
class MyAvatarLabel;
class MyTimeoutLabel;
class MyCashLabel;
class MyNameLabel;

class settingsDialogImpl;
class startWindowImpl;
class myMessageDialogImpl;

class GameTableStyleReader;
class CardDeckStyleReader;
class SoundEvents;

enum SeatState { SEAT_UNDEFINED, SEAT_ACTIVE, SEAT_AUTOFOLD, SEAT_STAYONTABLE, SEAT_CLEAR };

class gameTableImpl: public QMainWindow, public Ui::gameTable
{
	Q_OBJECT

public:
	gameTableImpl(ConfigFile *c = 0, QMainWindow *parent = 0);

	~gameTableImpl();

	boost::shared_ptr<Session> getSession();

	SoundEvents* getMySoundEventHandler() const {
		return mySoundEventHandler;
	}
	ChatTools* getMyChat() const {
		return myChat;
	}
	ConfigFile* getMyConfig() const {
		return myConfig;
	}
	GameTableStyleReader* getMyGameTableStyle() const {
		return myGameTableStyle;
	}
	bool getGuestMode() const {
		return guestMode;
	}

	void setStartWindow(startWindowImpl* s) {
		myStartWindow = s;
	}
	void setGuiLog(guiLog* l) {
		myGuiLog = l;
	}

	void setSpeeds();

#ifdef GUI_800x480
	Ui::tabs tabs;
	QDialog *tabsDiag;
#endif


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
	void signalRefreshSpectatorsDisplay();

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
	void signalPostRiverShowCards(unsigned playerId);

	void signalFlipHolecardsAllIn();
	void signalNextRoundCleanGui();
	void signalStartVoteOnKick(unsigned playerId, unsigned voteStarterPlayerId, int timeoutSec, int numVotesNeededToKick);
	void signalChangeVoteOnKickButtonsState(bool showHide);
	void signalEndVoteOnKick();
	void signalNetClientPlayerLeft(unsigned playerId);
	void signalNetClientSpectatorLeft(unsigned playerId);
	void signalNetClientSpectatorJoined(unsigned playerId);
	void signalNetClientPingUpdate(unsigned minPing, unsigned avgPing, unsigned maxPing);

public slots:

	void initGui(int speed);

	//refresh-Funktionen
	void refreshSet();
	void refreshCash();
	void refreshAction(int =-1, int=-1);
	void refreshChangePlayer();
	void refreshPot();
	void refreshGroupbox(int =-1, int =-1);
	void refreshAll();
	void refreshPlayerName();
	void refreshGameLabels(int);
	void refreshButton();
	void refreshPlayerAvatar();
	void refreshActionButtonFKeyIndicator(bool =0);
	void setPlayerAvatar(int myID, QString myAvatar);

	SeatState getCurrentSeatState(boost::shared_ptr<PlayerInterface> );

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

	void setGameSpeed(const int theValue) {
		guiGameSpeed = theValue;    // Achtung Faktor 10!!!
		setSpeeds();
	}

	void callSettingsDialog();
	void applySettings(settingsDialogImpl*);

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
	void postRiverRunAnimation3();
	void postRiverRunAnimation4();
	void postRiverRunAnimation5();
	void postRiverRunAnimation6();

	void showHoleCards(unsigned playerId, bool allIn = false);

	void refreshCardsChance(GameState);

	void blinkingStartButtonAnimationAction();

	void flipHolecardsAllIn();
	void triggerVoteOnKick(int id);

	void handSwitchRounds();

	void startNewHand();

	void stopTimer();

	void nextRoundCleanGui();

	void breakButtonClicked();

	void keyPressEvent ( QKeyEvent*);
	bool eventFilter(QObject *obj, QEvent *event);

	void switchChatWindow();
	void switchHelpWindow();
	void switchLogWindow();
	void switchAwayWindow();
	void switchChanceWindow();
	void switchFullscreen();

	void sendChatMessage();
	void checkChatInputLength(QString);
	void tabSwitchAction();

	void leaveCurrentNetworkGame();

	void localGameModification();
	void networkGameModification();

	void mouseOverFlipCards(bool front);

	void updateMyButtonsState(int mode = -1); //mode 0 == called from dealberocards
	void uncheckMyButtons();
	void resetMyButtonsCheckStateMemory();
	void clearMyButtons();
	void myButtonsCheckable(bool state);

	void changePlayingMode();
	void changeSpinBoxBetValue(int);
	void spinBoxBetValueChanged(int);

	void showMaximized ();
	void closeGameTable();

	void startVoteOnKick(unsigned playerId, unsigned voteStarterPlayerId, int timeoutSec, int numVotesNeededToKick);
	void changeVoteOnKickButtonsState(bool showHide);
	void endVoteOnKick();
	void voteOnKickYes();
	void voteOnKickNo();
	void startVoteOnKickTimeout();
	void stopVoteOnKickTimeout();
	void nextVoteOnKickTimeoutAnimationFrame();
	void refreshVotesMonitor(int currentVotes, int numVotesNeededToKick);

	void refreshGameTableStyle();
	void saveGameTableGeometry();
	void restoreGameTableGeometry();

	void netClientPlayerLeft(unsigned playerId);
	void netClientSpectatorLeft(unsigned playerId);
	void netClientSpectatorJoined(unsigned playerId);
	void registeredUserMode();
	void guestUserMode();

	void showShowMyCardsButton();
	void sendShowMyCardsSignal();

	void closeMessageBoxes();
	void hide();
	void soundEvent_blindsWereSet(int);
	void enableCallCheckPushButton();
	void checkActionLabelPosition();

#ifdef GUI_800x480
	void tabsButtonClicked();
	void tabsButtonClose();
#endif
	void refreshSpectatorsDisplay();
	void pingUpdate(unsigned, unsigned, unsigned);
	int getAndroidApiVersion();

private:

	boost::shared_ptr<GuiInterface> myServerGuiInterface;
	guiLog *myGuiLog;
	ChatTools *myChat;
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
	QTimer *voteOnKickTimeoutTimer;
	boost::timers::portable::microsec_timer voteOnKickRealTimer;
	QTimer *enableCallCheckPushButtonTimer;

	QWidget *userWidgetsArray[6];
	QLabel *buttonLabelArray[MAX_NUMBER_OF_PLAYERS];
	MyCashLabel *cashLabelArray[MAX_NUMBER_OF_PLAYERS];
	QLabel *cashTopLabelArray[MAX_NUMBER_OF_PLAYERS];
	MySetLabel *setLabelArray[MAX_NUMBER_OF_PLAYERS];
	QLabel *actionLabelArray[MAX_NUMBER_OF_PLAYERS];
	MyNameLabel *playerNameLabelArray[MAX_NUMBER_OF_PLAYERS];
	MyAvatarLabel *playerAvatarLabelArray[MAX_NUMBER_OF_PLAYERS];
	MyTimeoutLabel *timeoutLabelArray[MAX_NUMBER_OF_PLAYERS];
	QLabel *playerStarsArray[6][MAX_NUMBER_OF_PLAYERS];

	QGroupBox *groupBoxArray[MAX_NUMBER_OF_PLAYERS];
	MyCardsPixmapLabel *boardCardsArray[5];
	MyCardsPixmapLabel *holeCardsArray[MAX_NUMBER_OF_PLAYERS][2];

	QLabel *playerTipLabelArray[MAX_NUMBER_OF_PLAYERS];
	QPixmap flipside;
	QLabel *spectatorIcon;
	QLabel *spectatorNumberLabel;

	// 	Dialogs
	startWindowImpl *myStartWindow;
	myMessageDialogImpl *myUniversalMessageDialog;

	//Sound
	SoundEvents *mySoundEventHandler;
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
	bool guestMode;

	// statistic testing
	int statisticArray[15];

	QSemaphore guiUpdateSemaphore;

	int keyUpDownChatCounter;
	int myLastPreActionBetValue;

	int voteOnKickTimeoutSecs;
	unsigned playerAboutToBeKickedId;

	GameTableStyleReader *myGameTableStyle;
	CardDeckStyleReader *myCardDeckStyle;

	QString AllInString;
	QString RaiseString;
	QString BetString;
	QString CallString;
	QString CheckString;
	QString FoldString;
	QString PotString;
	QString TotalString;
	QString BetsString;
	QString GameString;
	QString HandString;
	QString PreflopString;
	QString FlopString;
	QString TurnString;
	QString RiverString;

	friend class MyAvatarLabel;
	friend class GuiWrapper;
};

#endif
