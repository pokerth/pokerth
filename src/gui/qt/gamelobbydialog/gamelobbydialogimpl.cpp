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
#include "gamelobbydialogimpl.h"
#include "mygamelistsortfilterproxymodel.h"
#include "mynicklistsortfilterproxymodel.h"
#include "startwindowimpl.h"
#include "chattools.h"
#include "changecompleteblindsdialogimpl.h"
#include "session.h"
#include "configfile.h"
#include "gamedata.h"
#include "game_defs.h"
#include <net/socket_msg.h>
#include "mymessagedialogimpl.h"
#include "soundevents.h"

using namespace std;


gameLobbyDialogImpl::gameLobbyDialogImpl(startWindowImpl *parent, ConfigFile *c)
	: QDialog(parent), myW(NULL), myStartWindow(parent), myConfig(c), currentGameName(""), myPlayerId(0), isGameAdministrator(false), inGame(false), guestMode(false), blinkingButtonAnimationState(true), myChat(NULL), keyUpCounter(0), infoMsgToShowId(0), currentInvitationGameId(0), inviteDialogIsCurrentlyShown(false), autoStartTimerCounter(0), lastNickListFilterState(0)
{

#ifdef __APPLE__
	setWindowModality(Qt::ApplicationModal);
	setWindowFlags(Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint | Qt::Dialog);
#elif _WIN32
//	setWindowFlags(Qt::Dialog | Qt::WindowMinimizeButtonHint);
#endif
	setupUi(this);
	myAppDataPath = QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str());
#ifdef ANDROID
	this->setWindowState(Qt::WindowFullScreen);
#endif
	//wait start game message
	waitStartGameMsgBox = new MyMessageBox(this);
	waitStartGameMsgBox->setText(tr("Starting game. Please wait ..."));
#ifndef ANDROID
	waitStartGameMsgBox->setWindowModality(Qt::NonModal);
#endif
#ifdef __APPLE__
	waitStartGameMsgBox->setWindowFlags(Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint | Qt::Dialog);
#endif
	waitStartGameMsgBox->setStandardButtons(QMessageBox::NoButton);

	//wait start game message for rejoin
	waitRejoinStartGameMsgBox = new MyMessageBox(this);
	waitRejoinStartGameMsgBox->setText(tr("Waiting for the start of the next hand to rejoin the game ..."));
#ifndef ANDROID
	waitRejoinStartGameMsgBox->setWindowModality(Qt::NonModal);
#endif
#ifdef __APPLE__
	waitRejoinStartGameMsgBox->setWindowFlags(Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint | Qt::Dialog);
#endif
	waitRejoinStartGameMsgBox->setStandardButtons(QMessageBox::NoButton);

	inviteOnlyInfoMsgBox = new myMessageDialogImpl(myConfig, this);

	//HTML stuff
	QString pokerthDotNet("<a href='http://www.pokerth.net'>http://www.pokerth.net</a>");
	QString clickToRanking(QString("<a href='http://online-ranking.pokerth.net'>%1</a>").arg(tr("Click here to view the online rankings")));
	QString clickToSpectate(QString("<a href='http://pokerth.net/live'><b>%1</b></a>").arg(tr("Spectate")));
	label_pokerthDotNet->setText(pokerthDotNet);
	label_rankings->setText(clickToRanking);
	label_spectate->setText(clickToSpectate);

	waitStartGameMsgBoxTimer = new QTimer(this);
	waitStartGameMsgBoxTimer->setSingleShot(true);
	blinkingButtonAnimationTimer = new QTimer(this);
	blinkingButtonAnimationTimer->setInterval(1000);
	autoStartTimer = new QTimer(this);
	autoStartTimer->setInterval(1000);
	showInfoMsgBoxTimer = new QTimer(this);
	showInfoMsgBoxTimer->setSingleShot(true);

	//fetch button colors for blinking
	groupBox_GameInfo->setEnabled(true);
	pushButton_StartGame->setEnabled(true);
	defaultStartButtonColor = pushButton_StartGame->palette().button().color();
	defaultStartButtonTextColor = pushButton_StartGame->palette().buttonText().color();
	pushButton_StartGame->setEnabled(false);
	groupBox_GameInfo->setEnabled(false);
	disabledStartButtonColor = pushButton_StartGame->palette().button().color();
	disabledStartButtonTextColor = pushButton_StartGame->palette().buttonText().color();

	//prepare overlay
	autoStartTimerOverlay = new QLabel(scrollArea_gameInfos);
	autoStartTimerOverlay->hide();
	autoStartTimerOverlay->setWordWrap(true);
	autoStartTimerOverlay->setMaximumWidth(190);
	autoStartTimerOverlay->setMinimumWidth(190);
	autoStartTimerOverlay->setTextFormat(Qt::RichText);
	autoStartTimerOverlay->setAlignment(Qt::AlignCenter);
	autoStartTimerOverlay->setAutoFillBackground(true);
	autoStartTimerOverlay->setFrameStyle(QFrame::StyledPanel);
	QPalette p;
	p.setColor(QPalette::Background, QColor(255, 255, 255, 210));
	autoStartTimerOverlay->setPalette(p);


	myGameListModel = new QStandardItemModel(this);
	myGameListSortFilterProxyModel = new MyGameListSortFilterProxyModel(this);
	myGameListSortFilterProxyModel->setSourceModel(myGameListModel);
	myGameListSortFilterProxyModel->setDynamicSortFilter(true);
	treeView_GameList->setModel(myGameListSortFilterProxyModel);
	myGameListSelectionModel = treeView_GameList->selectionModel();

	QStringList headerList;
	headerList << tr("Game") << tr("Players") << tr("State") << tr("T") << tr("P") << tr("Time");
	myGameListModel->setHorizontalHeaderLabels(headerList);

#ifdef GUI_800x480
	treeView_GameList->setColumnWidth(0,200); //484px alltogether
	treeView_GameList->setColumnWidth(1,65);
	treeView_GameList->setColumnWidth(2,100);
	treeView_GameList->setColumnWidth(3,20);
	treeView_GameList->setColumnWidth(4,20);
	treeView_GameList->setColumnWidth(5,75);

	treeView_GameList->setStyleSheet("QTreeView {background-color: white; background-image: url(\""+myAppDataPath +"gfx/gui/misc/background_gamelist.png\"); background-attachment: fixed; background-position: top center ; background-repeat: no-repeat; color:rgb(0, 0, 0); font: 22px}");
	treeView_GameList->header()->setStyleSheet("QObject {font: bold 18px}");

	//make the scrollbars touchable for mobile guis
	this->setStyleSheet("QScrollBar:vertical { border: 1px solid grey; background: #f0f0f0; width: 55px; margin: 1px -1px 0px 0px; } QScrollBar::handle:vertical { border-radius: 4px; border: 3px solid grey; background: LightGrey ; min-height: 60px; } QScrollBar::add-line:vertical { background: none; } QScrollBar::sub-line:vertical { background: none; } QScrollBar:up-arrow:vertical, QScrollBar::down-arrow:vertical { background: none; } QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }");

	//remove game type in game description field to save space
	label_gameType->hide();
	label_typeIcon->hide();
	label_typeText->hide();

	//set colors
	QPalette p1;
	p1.setColor(QPalette::WindowText, this->palette().color(QPalette::Link));
	label_StartCash->setPalette(p1);
	label_blindsList->setPalette(p1);
	label_blindsRaiseIntervall->setPalette(p1);
	label_blindsRaiseMode->setPalette(p1);
	label_GameTiming->setPalette(p1);
	label_SmallBlind->setPalette(p1);

#else
	treeView_GameList->setColumnWidth(0,190);
	treeView_GameList->setColumnWidth(1,55);
	treeView_GameList->setColumnWidth(2,65);
	treeView_GameList->setColumnWidth(3,25);
	treeView_GameList->setColumnWidth(4,25);
	treeView_GameList->setColumnWidth(5,30);

	treeView_GameList->setStyleSheet("QTreeView {background-color: white; background-image: url(\""+myAppDataPath +"gfx/gui/misc/background_gamelist.png\"); background-attachment: fixed; background-position: top center ; background-repeat: no-repeat;}");
#endif
	treeView_GameList->setAutoFillBackground(true);

	myNickListModel = new QStandardItemModel(this);
	myNickListSortFilterProxyModel = new MyNickListSortFilterProxyModel(this);
	myNickListSortFilterProxyModel->setSourceModel(myNickListModel);
	myNickListSortFilterProxyModel->setDynamicSortFilter(true);
	myNickListSortFilterProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);

	treeView_NickList->setModel(myNickListSortFilterProxyModel);
	myNickListSelectionModel = treeView_NickList->selectionModel();

	QStringList headerList2;
	headerList2 << tr("Available Players");
	myNickListModel->setHorizontalHeaderLabels(headerList2);
	treeView_NickList->sortByColumn(0, Qt::AscendingOrder);

	myChat = new ChatTools(lineEdit_ChatInput, myConfig, INET_LOBBY_CHAT, textBrowser_ChatDisplay, myNickListModel, this);

	nickListContextMenu = new QMenu();
	nickListInviteAction = new QAction(QIcon(":/gfx/list_add_user.png"), tr("Invite player"), nickListContextMenu);
	nickListContextMenu->addAction(nickListInviteAction);
	nickListIgnorePlayerAction = new QAction(QIcon(":/gfx/im-ban-user.png"), tr("Ignore player"), nickListContextMenu);
	nickListContextMenu->addAction(nickListIgnorePlayerAction);
	nickListUnignorePlayerAction = new QAction(QIcon(":/gfx/dialog_ok_apply.png"), tr("Unignore player"), nickListContextMenu);
	nickListContextMenu->addAction(nickListUnignorePlayerAction);
	nickListPlayerInfoSubMenu = nickListContextMenu->addMenu(QIcon(":/gfx/dialog-information.png"), tr("Player infos ..."));
	nickListPlayerInGameInfo = new QAction(nickListContextMenu);
	nickListPlayerInfoSubMenu->addAction(nickListPlayerInGameInfo);
	nickListOpenPlayerStats1 = new QAction(QIcon(":/gfx/view-statistics.png"), tr("Show player stats"), nickListContextMenu);
	nickListContextMenu->addAction(nickListOpenPlayerStats1);
	nickListAdminSubMenu = nickListContextMenu->addMenu(tr("Admin action ..."));
	nickListAdminTotalKickBan = new QAction(tr("Total kickban"), nickListAdminSubMenu);
	nickListAdminSubMenu->addAction(nickListAdminTotalKickBan);

	connectedPlayersListPlayerInfoSubMenu = new QMenu();
	connectedPlayersListOpenPlayerStats = new QAction(QIcon(":/gfx/view-statistics.png"), tr("Show player stats"), connectedPlayersListPlayerInfoSubMenu);
	connectedPlayersListPlayerInfoSubMenu->addAction(connectedPlayersListOpenPlayerStats);

	gameListContextMenu = new QMenu();
	gameListReportBadGameNameAction = new QAction(QIcon(":/gfx/emblem-important.png"), tr("Report inappropriate game name"), gameListContextMenu);
	gameListContextMenu->addAction(gameListReportBadGameNameAction);
	gameListAdminSubMenu = gameListContextMenu->addMenu(tr("Admin action ..."));
	gameListAdminCloseGame = new QAction(tr("Close game"), gameListAdminSubMenu);
	gameListAdminSubMenu->addAction(gameListAdminCloseGame);

	connect( pushButton_CreateGame, SIGNAL( clicked() ), this, SLOT( createGame() ) );
	connect( pushButton_JoinGame, SIGNAL( clicked() ), this, SLOT( joinGame() ) );
	connect( pushButton_StartGame, SIGNAL( clicked() ), this, SLOT( startGame() ) );
	connect( pushButton_Kick, SIGNAL( clicked() ), this, SLOT( kickPlayer() ) );
	connect( pushButton_Leave, SIGNAL( clicked() ), this, SLOT( leaveGame() ) );
	connect( myGameListSelectionModel, SIGNAL( currentChanged (const QModelIndex &, const QModelIndex &) ), this, SLOT( gameSelected(const QModelIndex &) ) );
	connect( treeView_GameList, SIGNAL( doubleClicked (const QModelIndex &) ), this, SLOT( joinGame() ) );
	connect( treeView_GameList->header(), SIGNAL( sortIndicatorChanged ( int , Qt::SortOrder )), this, SLOT( changeGameListSorting() ) );
	connect( treeWidget_connectedPlayers, SIGNAL( currentItemChanged ( QTreeWidgetItem*, QTreeWidgetItem*) ), this, SLOT( playerSelected(QTreeWidgetItem*, QTreeWidgetItem*) ) );
	connect( lineEdit_ChatInput, SIGNAL( returnPressed () ), myChat, SLOT( sendMessage() ) );
	connect( lineEdit_ChatInput, SIGNAL( textChanged (QString) ), myChat, SLOT( checkInputLength(QString) ) );
	connect( lineEdit_ChatInput, SIGNAL( textEdited (QString) ), myChat, SLOT( setChatTextEdited() ) );
	connect( waitStartGameMsgBoxTimer, SIGNAL(timeout()), this, SLOT( showWaitStartGameMsgBox() ));
	connect( blinkingButtonAnimationTimer, SIGNAL(timeout()), this, SLOT( blinkingStartButtonAnimation() ));
	connect( autoStartTimer, SIGNAL(timeout()), this, SLOT( updateAutoStartTimer() ));
	connect( showInfoMsgBoxTimer, SIGNAL(timeout()), this, SLOT( showInfoMsgBox() ));
	connect( comboBox_gameListFilter, SIGNAL(currentIndexChanged(int)), this, SLOT(changeGameListFilter(int)));
	connect( comboBox_nickListFilter, SIGNAL(currentIndexChanged(int)), this, SLOT(changeNickListFilter(int)));
	connect( treeView_NickList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT( showNickListContextMenu(QPoint) ) );
	connect( treeWidget_connectedPlayers, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT( showConnectedPlayersContextMenu(QPoint) ) );
	connect( treeView_GameList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT( showGameListContextMenu(QPoint) ) );
	connect( nickListInviteAction, SIGNAL(triggered()), this, SLOT( invitePlayerToCurrentGame() ));
	connect( nickListIgnorePlayerAction, SIGNAL(triggered()), this, SLOT( putPlayerOnIgnoreList() ));
	connect( nickListUnignorePlayerAction, SIGNAL(triggered()), this, SLOT( removePlayerFromIgnoreList() ));
	connect( nickListOpenPlayerStats1, SIGNAL(triggered()), this, SLOT( openPlayerStats1() ));
	connect( connectedPlayersListOpenPlayerStats, SIGNAL(triggered()), this, SLOT( openPlayerStats2() ));
	connect( lineEdit_searchForPlayers, SIGNAL(textChanged(QString)),this, SLOT(searchForPlayerRegExpChanged()));
	connect( gameListReportBadGameNameAction, SIGNAL(triggered()), this, SLOT( reportBadGameName()));
	connect( gameListAdminCloseGame, SIGNAL(triggered()), this, SLOT( adminActionCloseGame() ));
	connect( nickListAdminTotalKickBan, SIGNAL(triggered()), this, SLOT( adminActionTotalKickBan() ));

	lineEdit_searchForPlayers->installEventFilter(this);
	lineEdit_ChatInput->installEventFilter(this);
	this->installEventFilter(this);
	clearDialog();
}

int gameLobbyDialogImpl::exec()
{

	if(myConfig->readConfigInt("UseLobbyChat"))  {
		groupBox_lobbyChat->show();
	} else {
		groupBox_lobbyChat->hide();
	}
	readDialogSettings();

	PlayerInfo playerInfo(mySession->getClientPlayerInfo(mySession->getClientUniquePlayerId()));
	if(playerInfo.isGuest) {
		guestUserMode();
	} else {
		registeredUserMode();
	}

	// Bug #320: https://github.com/pokerth/pokerth/issues/320
	myChat->refreshIgnoreList();

#ifdef ANDROID
	this->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
#endif
	int ret = QDialog::exec();
	waitStartGameMsgBoxTimer->stop();
	closeAllChildDialogs();

	return ret;
}


gameLobbyDialogImpl::~gameLobbyDialogImpl()
{
	delete myChat;
	myChat = NULL;

	delete inviteOnlyInfoMsgBox;
	inviteOnlyInfoMsgBox = NULL;
}

void gameLobbyDialogImpl::setSession(boost::shared_ptr<Session> session)
{
	mySession = session;
	myChat->setSession(mySession);
}

void gameLobbyDialogImpl::createGame()
{
	assert(mySession);

	myCreateInternetGameDialog = new createInternetGameDialogImpl(this, myConfig);
	PlayerInfo playerInfo(mySession->getClientPlayerInfo(mySession->getClientUniquePlayerId()));
	myCreateInternetGameDialog->exec(guestMode, QString::fromUtf8(playerInfo.playerName.c_str()));

	if (myCreateInternetGameDialog->result() == QDialog::Accepted ) {

		GameData gameData;
		// Set Game Data
		gameData.maxNumberOfPlayers = myCreateInternetGameDialog->spinBox_quantityPlayers->value();
		gameData.startMoney = myCreateInternetGameDialog->spinBox_startCash->value();
		gameData.firstSmallBlind = myCreateInternetGameDialog->getChangeCompleteBlindsDialog()->spinBox_firstSmallBlind->value();

		if(myCreateInternetGameDialog->getChangeCompleteBlindsDialog()->radioButton_raiseBlindsAtHands->isChecked()) {
			gameData.raiseIntervalMode = RAISE_ON_HANDNUMBER;
			gameData.raiseSmallBlindEveryHandsValue = myCreateInternetGameDialog->getChangeCompleteBlindsDialog()->spinBox_raiseSmallBlindEveryHands->value();
		} else {
			gameData.raiseIntervalMode = RAISE_ON_MINUTES;
			gameData.raiseSmallBlindEveryMinutesValue = myCreateInternetGameDialog->getChangeCompleteBlindsDialog()->spinBox_raiseSmallBlindEveryMinutes->value();
		}

		if(myCreateInternetGameDialog->getChangeCompleteBlindsDialog()->radioButton_alwaysDoubleBlinds->isChecked()) {
			gameData.raiseMode = DOUBLE_BLINDS;
		} else {
			gameData.raiseMode = MANUAL_BLINDS_ORDER;
			std::list<int> tempBlindList;
			int i;
			bool ok = true;
			for(i=0; i<myCreateInternetGameDialog->getChangeCompleteBlindsDialog()->listWidget_blinds->count(); i++) {
				tempBlindList.push_back(myCreateInternetGameDialog->getChangeCompleteBlindsDialog()->listWidget_blinds->item(i)->text().toInt(&ok,10));
			}
			gameData.manualBlindsList = tempBlindList;

			if(myCreateInternetGameDialog->getChangeCompleteBlindsDialog()->radioButton_afterThisAlwaysDoubleBlinds->isChecked()) {
				gameData.afterManualBlindsMode = AFTERMB_DOUBLE_BLINDS;
			} else {
				if(myCreateInternetGameDialog->getChangeCompleteBlindsDialog()->radioButton_afterThisAlwaysRaiseAbout->isChecked()) {
					gameData.afterManualBlindsMode = AFTERMB_RAISE_ABOUT;
					gameData.afterMBAlwaysRaiseValue = myCreateInternetGameDialog->getChangeCompleteBlindsDialog()->spinBox_afterThisAlwaysRaiseValue->value();
				} else {
					gameData.afterManualBlindsMode = AFTERMB_STAY_AT_LAST_BLIND;
				}
			}
		}

		gameData.guiSpeed = myConfig->readConfigInt("GameSpeed");
		gameData.delayBetweenHandsSec = myCreateInternetGameDialog->spinBox_netDelayBetweenHands->value();
		gameData.playerActionTimeoutSec = myCreateInternetGameDialog->spinBox_netTimeOutPlayerAction->value();
		gameData.gameType = GameType(myCreateInternetGameDialog->comboBox_gameType->itemData(myCreateInternetGameDialog->comboBox_gameType->currentIndex(), Qt::UserRole).toInt());
		gameData.allowSpectators = myCreateInternetGameDialog->checkBox_allowSpectators->isChecked();

		currentGameName = myCreateInternetGameDialog->lineEdit_gameName->text().simplified();

		switch (gameData.gameType) {
		case GAME_TYPE_NORMAL: {
			label_typeIcon->setPixmap(QPixmap(":/gfx/player_play.png"));
			label_typeText->setText(tr("Standard"));
		}
		break;
		case GAME_TYPE_REGISTERED_ONLY: {
			label_typeIcon->setPixmap(QPixmap(":/gfx/registered.png"));
			label_typeText->setText(tr("Registered players only"));
		}
		break;
		case GAME_TYPE_INVITE_ONLY: {
			label_typeIcon->setPixmap(QPixmap(":/gfx/list_add_user.png"));
			label_typeText->setText(tr("Invited players only"));
		}
		break;
		case GAME_TYPE_RANKING: {
			label_typeIcon->setPixmap(QPixmap(":/gfx/cup.png"));
			label_typeText->setText(tr("Ranking game"));
		}
		break;
		}

		showGameDescription(true);

		label_SmallBlind->setText(QString("%L1").arg(gameData.firstSmallBlind));
		label_StartCash->setText(QString("%L1").arg(gameData.startMoney));
		updateDialogBlinds(gameData);
		label_GameTiming->setText(QString::number(gameData.playerActionTimeoutSec)+" "+tr("sec (action)")+"\n"+QString::number(gameData.delayBetweenHandsSec)+" "+tr("sec (hand delay)"));

		mySession->clientCreateGame(gameData, currentGameName.toUtf8().constData(), myCreateInternetGameDialog->lineEdit_Password->text().toUtf8().constData());

	}
#ifdef ANDROID
	this->setFocus();
#endif
}

void gameLobbyDialogImpl::joinGame()
{
	assert(mySession);
	QItemSelectionModel *selection = treeView_GameList->selectionModel();
	if (!inGame && selection->hasSelection()) {
		unsigned gameId = selection->selectedRows().first().data(Qt::UserRole).toUInt();
		GameInfo info(mySession->getClientGameInfo(gameId));
		bool ok = true;
		QString password;
		//if private ask for password
		if (info.isPasswordProtected) {
			password = QInputDialog::getText(this, tr("Joining a private Game"),
											 tr("You are about to join a private game. Please enter the password!"), QLineEdit::Password, (""), &ok);
		}

		if (ok)
			mySession->clientJoinGame(gameId, password.toUtf8().constData());
	}
}

void gameLobbyDialogImpl::refresh(int actionID)
{

	if (actionID == MSG_NET_GAME_CLIENT_START) {
		myGameListModel->clear();
		myGameListSelectionModel->clear();
		myGameListSelectionModel->clearSelection();
		myGameListSortFilterProxyModel->clear();
		myNickListModel->clear();
		myNickListSelectionModel->clear();
		myNickListSelectionModel->clearSelection();;

		QStringList headerList;
		headerList << tr("Game") << tr("Players") << tr("State") << tr("T") << tr("P") << tr("Time");;
		myGameListModel->setHorizontalHeaderLabels(headerList);

#ifdef GUI_800x480
		treeView_GameList->setColumnWidth(0,200); //484px alltogether
		treeView_GameList->setColumnWidth(1,65);
		treeView_GameList->setColumnWidth(2,100);
		treeView_GameList->setColumnWidth(3,20);
		treeView_GameList->setColumnWidth(4,20);
		treeView_GameList->setColumnWidth(5,75);
#else
		treeView_GameList->setColumnWidth(0,190);
		treeView_GameList->setColumnWidth(1,55);
		treeView_GameList->setColumnWidth(2,65);
		treeView_GameList->setColumnWidth(3,25);
		treeView_GameList->setColumnWidth(4,25);
		treeView_GameList->setColumnWidth(5,30);
#endif

		QStringList headerList2;
		headerList2 << tr("Available Players");
		myNickListModel->setHorizontalHeaderLabels(headerList2);

		//stop waitStartGameMsgBoxes
		waitStartGameMsgBoxTimer->stop();
		closeAllChildDialogs();
		this->accept();
		myW->show();
	} else if(actionID == MSG_NET_GAME_CLIENT_SYNCSTART) {
		waitStartGameMsgBoxTimer->start(2000);
	} else if(actionID == MSG_NET_GAME_CLIENT_SYNCREJOIN) {
		//		if(this->isVisible()) { //TODO <-- does it work without isVisible???
		//break the autoStartTimer animation
		autoStartTimer->stop();
		autoStartTimerOverlay->hide();
		//show msg dialog
		waitRejoinStartGameMsgBox->show();
		waitRejoinStartGameMsgBox->raise();
		waitRejoinStartGameMsgBox->activateWindow();
		pushButton_Leave->setDisabled(true);
		//		}
	}
}

void gameLobbyDialogImpl::removedFromGame(int /*reason*/)
{
	inGame = false;
	isGameAdministrator = false;
	leftGameDialogUpdate();
}

void gameLobbyDialogImpl::gameSelected(const QModelIndex &index)
{

	if (!inGame && index.isValid() ) {
		pushButton_JoinGame->setEnabled(true);

		currentGameName = myGameListModel->item(myGameListSortFilterProxyModel->mapToSource(index).row(), 0)->text();

		groupBox_GameInfo->setEnabled(true);
		groupBox_GameInfo->setTitle(tr("Game Info") + " - " + currentGameName);

		assert(mySession);
		GameInfo info(mySession->getClientGameInfo(myGameListModel->item(myGameListSortFilterProxyModel->mapToSource(index).row(), 0)->data(Qt::UserRole).toUInt()));

		switch (info.data.gameType) {
		case GAME_TYPE_NORMAL: {
			label_typeIcon->setPixmap(QPixmap(":/gfx/player_play.png"));
			label_typeText->setText(tr("Standard"));
		}
		break;
		case GAME_TYPE_REGISTERED_ONLY: {
			label_typeIcon->setPixmap(QPixmap(":/gfx/registered.png"));
			label_typeText->setText(tr("Registered players only"));
		}
		break;
		case GAME_TYPE_INVITE_ONLY: {
			label_typeIcon->setPixmap(QPixmap(":/gfx/list_add_user.png"));
			label_typeText->setText(tr("Invited players only"));
		}
		break;
		case GAME_TYPE_RANKING: {
			label_typeIcon->setPixmap(QPixmap(":/gfx/cup.png"));
			label_typeText->setText(tr("Ranking game"));
		}
		break;
		}

		showGameDescription(true);
		label_SmallBlind->setText(QString("%L1").arg(info.data.firstSmallBlind));
		label_StartCash->setText(QString("%L1").arg(info.data.startMoney));
		//		label_MaximumNumberOfPlayers->setText(QString::number(info.data.maxNumberOfPlayers));s
		updateDialogBlinds(info.data);

		label_GameTiming->setText(QString::number(info.data.playerActionTimeoutSec)+" "+tr("sec (action)")+"\n"+QString::number(info.data.delayBetweenHandsSec)+" "+tr("sec (hand delay)"));

		treeWidget_connectedPlayers->clear();
		PlayerIdList::const_iterator i = info.players.begin();
		PlayerIdList::const_iterator end = info.players.end();
		while (i != end) {
			bool admin = info.adminPlayerId == *i;
			PlayerInfo playerInfo(mySession->getClientPlayerInfo(*i));
			addConnectedPlayer(*i, QString::fromUtf8(playerInfo.playerName.c_str()), admin);
			++i;
		}

		treeWidget_connectedSpectators->clear();
		PlayerIdList::const_iterator s = info.spectators.begin();
		PlayerIdList::const_iterator s_end = info.spectators.end();
		while (s != s_end) {
			PlayerInfo playerInfo(mySession->getClientPlayerInfo(*s));
			addConnectedSpectator(*s, QString::fromUtf8(playerInfo.playerName.c_str()));
			++s;
		}

#ifdef __APPLE__
		// Dirty workaround for a Qt redraw bug on Mac OS.
		treeWidget_connectedPlayers->setFocus();
		treeView_GameList->setFocus();
#endif
	}
}

void gameLobbyDialogImpl::updateGameItem(QList <QStandardItem*> itemList, unsigned gameId)
{
	assert(mySession);
	GameInfo info(mySession->getClientGameInfo(gameId));

	itemList.at(0)->setData(gameId, Qt::UserRole);
	itemList.at(0)->setData(QString::fromUtf8(info.name.c_str()), Qt::DisplayRole);

	PlayerIdList::const_iterator i = info.players.begin();
	PlayerIdList::const_iterator end = info.players.end();

	while (i != end) {
		if(myPlayerId == *i) {
			itemList.at(0)->setData( "MeInThisGame", 16);
			itemList.at(0)->setBackground(QBrush(QColor(0, 255, 0, 127)));
			itemList.at(1)->setBackground(QBrush(QColor(0, 255, 0, 127)));
			itemList.at(2)->setBackground(QBrush(QColor(0, 255, 0, 127)));
			itemList.at(3)->setBackground(QBrush(QColor(0, 255, 0, 127)));
			itemList.at(4)->setBackground(QBrush(QColor(0, 255, 0, 127)));
			itemList.at(5)->setBackground(QBrush(QColor(0, 255, 0, 127)));
			break;
		} else {
			itemList.at(0)->setData( "", 16);
			itemList.at(0)->setBackground(QBrush());
			itemList.at(1)->setBackground(QBrush());
			itemList.at(2)->setBackground(QBrush());
			itemList.at(3)->setBackground(QBrush());
			itemList.at(4)->setBackground(QBrush());
			itemList.at(5)->setBackground(QBrush());
		}
		++i;
	}

	//reset players iterator
	i = info.players.begin();
	end = info.players.end();
	while (i != end) {
		//mark players as active
		int it1 = 0;
		while (myNickListModel->item(it1)) {
			if (myNickListModel->item(it1, 0)->data(Qt::UserRole) == *i) {
				myNickListModel->item(it1, 0)->setData("active", 34);
				break;
			}
			++it1;
		}
		++i;
	}

	QString playerStr;
	playerStr.sprintf("%u/%u", (unsigned)info.players.size(), (unsigned)info.data.maxNumberOfPlayers);
	itemList.at(1)->setData(playerStr, Qt::DisplayRole);
	if((unsigned)info.players.size() == (unsigned)info.data.maxNumberOfPlayers) {
		itemList.at(1)->setData("totalfull", 16);
	} else {
		itemList.at(1)->setData("nonfull", 16);
	}\

	if (info.mode == GAME_MODE_STARTED) {
		itemList.at(2)->setData(tr("running"), Qt::DisplayRole);
		itemList.at(2)->setData("running", 16);
	} else {
		itemList.at(2)->setData(tr("open"), Qt::DisplayRole);
		itemList.at(2)->setData("open", 16);
	}

	switch (info.data.gameType) {
	case GAME_TYPE_NORMAL: {
		itemList.at(3)->setIcon(QIcon(":/gfx/player_play.png"));
		itemList.at(3)->setData("", Qt::DisplayRole);
		itemList.at(3)->setData("standard", 16);
	}
	break;
	case GAME_TYPE_REGISTERED_ONLY: {
		itemList.at(3)->setIcon(QIcon(":/gfx/registered.png"));
		itemList.at(3)->setData(" ", Qt::DisplayRole);
		itemList.at(3)->setData("registered", 16);
	}
	break;
	case GAME_TYPE_INVITE_ONLY: {
		itemList.at(3)->setIcon(QIcon(":/gfx/list_add_user.png"));
		itemList.at(3)->setData("  ", Qt::DisplayRole);
		itemList.at(3)->setData("invited", 16);
	}
	break;
	case GAME_TYPE_RANKING: {
		itemList.at(3)->setIcon(QIcon(":/gfx/cup.png"));
		itemList.at(3)->setData("   ", Qt::DisplayRole);
		itemList.at(3)->setData("ranking", 16);
	}
	break;
	}

	if (info.isPasswordProtected) {
		itemList.at(4)->setIcon(QIcon(":/gfx/lock.png"));
		itemList.at(4)->setData(" ", Qt::DisplayRole);
		itemList.at(4)->setData("private", 16);
	} else {
		itemList.at(4)->setData("", Qt::DisplayRole);
		itemList.at(4)->setData("nonpriv", 16);
	}

	itemList.at(5)->setData(QString::number(info.data.playerActionTimeoutSec)+"s/"+QString::number(info.data.delayBetweenHandsSec)+"s", Qt::DisplayRole);
	QString actionTimeOutString = QString::number(info.data.playerActionTimeoutSec);
	if(info.data.playerActionTimeoutSec < 10) {
		actionTimeOutString = "0"+actionTimeOutString;
	}
	itemList.at(5)->setData(actionTimeOutString, 16);

	treeView_GameList->sortByColumn(myConfig->readConfigInt("DlgGameLobbyGameListSortingSection"), (Qt::SortOrder)myConfig->readConfigInt("DlgGameLobbyGameListSortingOrder") );
	refreshGameStats();

	//mark spactators as active
	PlayerIdList::const_iterator s = info.spectators.begin();
	PlayerIdList::const_iterator s_end = info.spectators.end();

	while (s != s_end) {
		int it2 = 0;
		while (myNickListModel->item(it2)) {
			if (myNickListModel->item(it2, 0)->data(Qt::UserRole) == *s) {
				myNickListModel->item(it2, 0)->setData("active", 34);
				break;
			}
			++it2;
		}
		++s;
	}
}

void gameLobbyDialogImpl::addGame(unsigned gameId)
{
	QList <QStandardItem*> itemList;
	QStandardItem *item1 = new QStandardItem();
	QStandardItem *item2 = new QStandardItem();
	QStandardItem *item3 = new QStandardItem();
	QStandardItem *item4 = new QStandardItem();
	QStandardItem *item5 = new QStandardItem();
	QStandardItem *item6 = new QStandardItem();
	itemList << item1 << item2 << item3 << item4 << item5 << item6;

	myGameListModel->appendRow(itemList);

	updateGameItem(itemList, gameId);
}

void gameLobbyDialogImpl::updateGameMode(unsigned gameId, int /*newMode*/)
{
	int it = 0;
	while (myGameListModel->item(it)) {
		if (myGameListModel->item(it, 0)->data(Qt::UserRole) == gameId) {
			QList <QStandardItem*> itemList;
			itemList << myGameListModel->item(it, 0) << myGameListModel->item(it, 1) << myGameListModel->item(it, 2) << myGameListModel->item(it, 3) << myGameListModel->item(it, 4) << myGameListModel->item(it, 5);
			updateGameItem(itemList, gameId);
			break;
		}
		it++;
	}
}

void gameLobbyDialogImpl::updateGameAdmin(unsigned /*gameId*/, unsigned adminPlayerId)
{
	newGameAdmin(adminPlayerId, "");
}

void gameLobbyDialogImpl::removeGame(unsigned gameId)
{
	int it = 0;
	while (myGameListModel->item(it)) {
		if (myGameListModel->item(it, 0)->data(Qt::UserRole) == gameId) {
			myGameListModel->removeRow(it);
			break;
		}
		it++;
	}

	refreshGameStats();
}

void gameLobbyDialogImpl::refreshGameStats()
{

	int runningGamesCounter = 0;
	int openGamesCounter = 0;

	int it = 0;
	while (myGameListModel->item(it)) {
		if (myGameListModel->item(it, 2)->data(16) == "running") {
			runningGamesCounter++;
		}
		if (myGameListModel->item(it, 2)->data(16) == "open") {
			openGamesCounter++;
		}
		++it;
	}

	label_openGamesCounter->setText(" | "+tr("running games: %1").arg(runningGamesCounter));
	label_runningGamesCounter->setText(" | "+tr("open games: %1").arg(openGamesCounter));

}

void gameLobbyDialogImpl::refreshPlayerStats()
{
	//ServerStats stats = mySession->getClientStats();
	label_connectedPlayersCounter->setText(tr("connected players: %1").arg(myNickListModel->rowCount()));
}

void gameLobbyDialogImpl::gameAddPlayer(unsigned gameId, unsigned playerId)
{
	if (!inGame) {
		QItemSelectionModel *selection = treeView_GameList->selectionModel();
		if (selection->hasSelection()) {
			if(selection->selectedRows().at(0).data(Qt::UserRole).toUInt() == gameId) {
				assert(mySession);
				GameInfo info(mySession->getClientGameInfo(gameId));
				bool admin = info.adminPlayerId == playerId;
				PlayerInfo playerInfo(mySession->getClientPlayerInfo(playerId));

				addConnectedPlayer(playerId, QString::fromUtf8(playerInfo.playerName.c_str()), admin);
			}
		}
	}

	int it = 0;
	while (myGameListModel->item(it)) {
		if (myGameListModel->item(it, 0)->data(Qt::UserRole) == gameId) {
			QList <QStandardItem*> itemList;
			itemList << myGameListModel->item(it, 0) << myGameListModel->item(it, 1) << myGameListModel->item(it, 2) << myGameListModel->item(it, 3) << myGameListModel->item(it, 4)  << myGameListModel->item(it, 5);
			updateGameItem(itemList, gameId);
			break;
		}
		it++;
	}
}

void gameLobbyDialogImpl::gameAddSpectator(unsigned /*gameId*/, unsigned playerId)
{
	int it1 = 0;
	while (myNickListModel->item(it1)) {
		if (myNickListModel->item(it1, 0)->data(Qt::UserRole) == playerId) {
			myNickListModel->item(it1, 0)->setData("active", 34);
			break;
		}
		++it1;
	}
}

void gameLobbyDialogImpl::gameRemovePlayer(unsigned gameId, unsigned playerId)
{
	if (!inGame) {
		QItemSelectionModel *selection = treeView_GameList->selectionModel();
		if (selection->hasSelection()) {
			if(selection->selectedRows().at(0).data(Qt::UserRole).toUInt() == gameId) {
				assert(mySession);
				PlayerInfo info(mySession->getClientPlayerInfo(playerId));
				removePlayer(playerId, QString::fromUtf8(info.playerName.c_str()));
			}
		}
	}

	int it = 0;
	while (myGameListModel->item(it)) {
		if (myGameListModel->item(it, 0)->data(Qt::UserRole) == gameId) {
			QList <QStandardItem*> itemList;
			itemList << myGameListModel->item(it, 0) << myGameListModel->item(it, 1) << myGameListModel->item(it, 2) << myGameListModel->item(it, 3) << myGameListModel->item(it, 4) << myGameListModel->item(it, 5);

			updateGameItem(itemList, gameId);
			break;
		}
		++it;
	}

	//mark player as idle again
	int it1 = 0;
	while (myNickListModel->item(it1)) {
		if (myNickListModel->item(it1, 0)->data(Qt::UserRole) == playerId) {
			myNickListModel->item(it1, 0)->setData("idle", 34);
			break;
		}
		++it1;
	}
}

void gameLobbyDialogImpl::gameRemoveSpectator(unsigned, unsigned playerId)
{
	//mark spectator as idle again
	int it1 = 0;
	while (myNickListModel->item(it1)) {
		if (myNickListModel->item(it1, 0)->data(Qt::UserRole) == playerId) {
			myNickListModel->item(it1, 0)->setData("idle", 34);
			break;
		}
		++it1;
	}
}

void gameLobbyDialogImpl::updateStats(ServerStats /*stats*/)
{
	refreshPlayerStats();
}

void gameLobbyDialogImpl::clearDialog()
{
	groupBox_GameInfo->setTitle(tr("Game Info"));
	groupBox_GameInfo->setEnabled(false);
	currentGameName = "";

	showGameDescription(false);
	label_typeIcon->setText(" ");
	label_typeText->setText(" ");
	label_SmallBlind->setText("");
	label_StartCash->setText("");
	label_blindsRaiseIntervall->setText("");
	label_blindsRaiseMode->setText("");
	label_blindsList->setText("");
	label_GameTiming->setText("");

	myGameListModel->clear();
	myGameListSelectionModel->clear();
	myGameListSelectionModel->clearSelection();
	myGameListSortFilterProxyModel->clear();
	treeView_GameList->show();
	treeWidget_connectedPlayers->clear();
	treeWidget_connectedSpectators->clear();

	pushButton_Leave->hide();
	pushButton_Kick->hide();
	pushButton_Kick->setEnabled(false);
	pushButton_StartGame->hide();
	checkBox_fillUpWithComputerOpponents->hide();
	pushButton_CreateGame->show();
	pushButton_JoinGame->show();
	pushButton_JoinGame->setEnabled(false);

	QStringList headerList;
	headerList << tr("Game") << tr("Players") << tr("State") << tr("T") << tr("P") << tr("Time");
	myGameListModel->setHorizontalHeaderLabels(headerList);

#ifdef GUI_800x480
	treeView_GameList->setColumnWidth(0,200); //484px alltogether
	treeView_GameList->setColumnWidth(1,65);
	treeView_GameList->setColumnWidth(2,100);
	treeView_GameList->setColumnWidth(3,20);
	treeView_GameList->setColumnWidth(4,20);
	treeView_GameList->setColumnWidth(5,75);
#else
	treeView_GameList->setColumnWidth(0,190);
	treeView_GameList->setColumnWidth(1,55);
	treeView_GameList->setColumnWidth(2,65);
	treeView_GameList->setColumnWidth(3,25);
	treeView_GameList->setColumnWidth(4,25);
	treeView_GameList->setColumnWidth(5,30);
#endif

	pushButton_CreateGame->clearFocus();
	lineEdit_ChatInput->setFocus();
	myChat->clearChat();

	myNickListModel->clear();
	myNickListSelectionModel->clear();
	myNickListSelectionModel->clearSelection();;
	QStringList headerList2;
	headerList2 << tr("Available Players");
	myNickListModel->setHorizontalHeaderLabels(headerList2);
	myNickListModel->sort(0, treeView_NickList->header()->sortIndicatorOrder());

	inGame = false;
	isGameAdministrator = false;
	myPlayerId = 0;

	showGameDescription(false);

	label_connectedPlayersCounter->setText(tr("connected players: %1").arg(0));
	label_openGamesCounter->setText(" | "+tr("running games: %1").arg(0));
	label_runningGamesCounter->setText(" | "+tr("open games: %1").arg(0));

	readDialogSettings();
}

void gameLobbyDialogImpl::checkPlayerQuantity()
{
	tabWidget_playerSpectators->setTabText(0, tr("Players (%1)").arg(treeWidget_connectedPlayers->topLevelItemCount()));

	assert(mySession);
	GameInfo info(mySession->getClientGameInfo(mySession->getClientCurrentGameId()));

	if(isGameAdministrator && info.data.gameType != GAME_TYPE_RANKING) {

		pushButton_Kick->show();
		pushButton_StartGame->show();
		checkBox_fillUpWithComputerOpponents->show();

		if (treeWidget_connectedPlayers->topLevelItemCount() >= 2) {
			pushButton_StartGame->setEnabled(true);

			if(treeWidget_connectedPlayers->topLevelItemCount() == info.data.maxNumberOfPlayers) {
				blinkingButtonAnimationTimer->start();
			} else {
				blinkingButtonAnimationTimer->stop();
				blinkingButtonAnimationState = false;
				blinkingStartButtonAnimation();
			}
		} else {
			pushButton_StartGame->setEnabled(false);
			blinkingButtonAnimationTimer->stop();
			blinkingButtonAnimationState = false;
			blinkingStartButtonAnimation();
		}
	}

	//general actions
	if(treeWidget_connectedPlayers->topLevelItemCount() < info.data.maxNumberOfPlayers) {
		autoStartTimerOverlay->hide();
		autoStartTimer->stop();
	}

}

void gameLobbyDialogImpl::blinkingStartButtonAnimation()
{

	if(blinkingButtonAnimationState) {
		QPalette p = pushButton_StartGame->palette();
		p.setColor(QPalette::Button, QColor(Qt::red));
		p.setColor(QPalette::ButtonText, QColor(Qt::white));
		pushButton_StartGame->setPalette(p);
		blinkingButtonAnimationState = false;
	} else {
		if(pushButton_StartGame->isEnabled()) {
			QPalette p = pushButton_StartGame->palette();
			p.setColor(QPalette::Button, defaultStartButtonColor);
			p.setColor(QPalette::ButtonText, defaultStartButtonTextColor);
			pushButton_StartGame->setPalette(p);
			blinkingButtonAnimationState = true;
		} else {
			QPalette p = pushButton_StartGame->palette();
			p.setColor(QPalette::Button, disabledStartButtonColor);
			p.setColor(QPalette::ButtonText, disabledStartButtonTextColor);
			pushButton_StartGame->setPalette(p);
			blinkingButtonAnimationState = true;
		}
	}
}

void gameLobbyDialogImpl::joinedNetworkGame(unsigned playerId, QString playerName, bool isGameAdmin)
{
	// Update dialog
	inGame = true;
	joinedGameDialogUpdate();
	myPlayerId = playerId;
	isGameAdministrator = isGameAdmin;
	addConnectedPlayer(playerId, playerName, isGameAdmin);

	//show msgBox about invite only game
	assert(mySession);
	if(mySession->getClientGameInfo(mySession->getClientCurrentGameId()).data.gameType == GAME_TYPE_INVITE_ONLY) {
		infoMsgToShowId = 2;
		showInfoMsgBoxTimer->start(1000);
	}

	//    if(!myGameListSelectionModel->hasSelection() && mySession->getClientGameInfo(mySession->getClientCurrentGameId()).data.gameType == GAME_TYPE_INVITE_ONLY) {
	//        int it = 0;
	//        while (myGameListModel->item(it)) {
	//            if (myGameListModel->item(it, 0)->data(Qt::UserRole) == mySession->getClientCurrentGameId()) {
	//                gameSelected(treeView_GameList->model()->index(it,0), true);
	//                break;
	//            }
	//            it++;
	//        }
	//    }
}


void gameLobbyDialogImpl::addConnectedPlayer(unsigned playerId, QString playerName, bool isGameAdmin)
{
	GameInfo info(mySession->getClientGameInfo(mySession->getClientCurrentGameId()));
	QTreeWidgetItem *item = new QTreeWidgetItem(treeWidget_connectedPlayers, 0);
	item->setData(0, Qt::UserRole, playerId);
	item->setData(0, Qt::DisplayRole, playerName);

	if(isGameAdmin) item->setBackground(0, QBrush(QColor(0, 255, 0, 127)));

	if(this->isVisible() && inGame && myConfig->readConfigInt("PlayNetworkGameNotification")) {
		if(treeWidget_connectedPlayers->topLevelItemCount() < info.data.maxNumberOfPlayers) {
			myW->getMySoundEventHandler()->playSound("playerconnected", 0);
		} else {
			myW->getMySoundEventHandler()->playSound("onlinegameready", 0);
			showAutoStartTimer();
		}
	}

	checkPlayerQuantity();

	if (inGame)
		refreshConnectedPlayerAvatars();
}

void gameLobbyDialogImpl::addConnectedSpectator(unsigned spectatorId, QString spectatorName)
{

	QTreeWidgetItem *item = new QTreeWidgetItem(treeWidget_connectedSpectators, 0);
	item->setData(0, Qt::UserRole, spectatorId);
	item->setData(0, Qt::DisplayRole, spectatorName);
	tabWidget_playerSpectators->setTabText(1, tr("Spectators (%1)").arg(treeWidget_connectedSpectators->topLevelItemCount()));
//	if (inGame)
//		refreshConnectedSpecatatorAvatars();
}

void gameLobbyDialogImpl::updatePlayer(unsigned playerId, QString newPlayerName)
{

	//rename player in connected players list
	QTreeWidgetItemIterator it(treeWidget_connectedPlayers);
	while (*it) {
		if ((*it)->data(0, Qt::UserRole) == playerId) {
			(*it)->setData(0, Qt::DisplayRole, newPlayerName);
			break;
		}
		++it;
	}

	if (inGame)
		refreshConnectedPlayerAvatars();

	//also rename player in nick-list
	QString oldNick;
	int it1 = 0;
	while (myNickListModel->item(it1)) {
		if (myNickListModel->item(it1, 0)->data(Qt::UserRole) == playerId) {
			oldNick = myNickListModel->item(it1, 0)->text();
			myNickListModel->item(it1, 0)->setText(newPlayerName);

			PlayerInfo playerInfo(mySession->getClientPlayerInfo(playerId));
			QString countryString = QString::fromUtf8(playerInfo.countryCode.c_str()).toLower();
			myNickListModel->item(it1, 0)->setData(countryString, 33);
			if(playerInfo.isGuest || countryString.isEmpty()) {
				myNickListModel->item(it1, 0)->setIcon(QIcon(":/cflags/cflags/undefined.png"));
				myNickListModel->item(it1, 0)->setToolTip("");
			} else {
				myNickListModel->item(it1, 0)->setIcon(QIcon(QString(":/cflags/cflags/%1.png").arg(countryString)));
				myNickListModel->item(it1, 0)->setToolTip(getFullCountryString(countryString.toUpper()));
			}
			break;
		}

		++it1;
	}

	if (myChat->getMyNick() == oldNick) {
		myChat->setMyNick(newPlayerName);
	}
}

void gameLobbyDialogImpl::removePlayer(unsigned playerId, QString)
{
	QTreeWidgetItemIterator it(treeWidget_connectedPlayers);
	while (*it) {
		if ((*it)->data(0, Qt::UserRole) == playerId) {
			treeWidget_connectedPlayers->takeTopLevelItem(treeWidget_connectedPlayers->indexOfTopLevelItem(*it));
			break;
		}
		++it;
	}
	checkPlayerQuantity();
}


void gameLobbyDialogImpl::removeSpectator(unsigned spectatorId, QString)
{
	QTreeWidgetItemIterator it(treeWidget_connectedSpectators);
	while (*it) {
		if ((*it)->data(0, Qt::UserRole) == spectatorId) {
			treeWidget_connectedSpectators->takeTopLevelItem(treeWidget_connectedSpectators->indexOfTopLevelItem(*it));
			break;
		}
		++it;
	}
	tabWidget_playerSpectators->setTabText(1, tr("Spectators (%1)").arg(treeWidget_connectedSpectators->topLevelItemCount()));
}

void gameLobbyDialogImpl::playerLeftLobby(unsigned playerId)
{
	int it1 = 0;
	while (myNickListModel->item(it1)) {
		if (myNickListModel->item(it1, 0)->data(Qt::UserRole) == playerId) {
			myNickListModel->removeRow(it1);;
			break;
		}
		++it1;
	}

	refreshPlayerStats();
}

void gameLobbyDialogImpl::playerJoinedLobby(unsigned playerId, QString /*playerName TODO remove*/)
{
	PlayerInfo playerInfo(mySession->getClientPlayerInfo(playerId));
	QString countryString = QString::fromUtf8(playerInfo.countryCode.c_str()).toLower();

	QStandardItem *item = new QStandardItem;
	item->setText(QString::fromUtf8(playerInfo.playerName.c_str()));
	item->setData(playerId, Qt::UserRole);
	item->setData(countryString, 33);
	if(playerInfo.isGuest || countryString.isEmpty()) {
		item->setIcon(QIcon(":/cflags/cflags/undefined.png"));
	} else {
		item->setIcon(QIcon(QString(":/cflags/cflags/%1.png").arg(countryString)));
		item->setToolTip(getFullCountryString(countryString.toUpper()));
	}
	item->setData("idle", 34);
	myNickListModel->appendRow(item);

	refreshPlayerStats();
}

void gameLobbyDialogImpl::newGameAdmin(unsigned playerId, QString)
{
	QTreeWidgetItemIterator it(treeWidget_connectedPlayers);
	while (*it) {
		if ((*it)->data(0, Qt::UserRole) == playerId) {
			(*it)->setBackground(0, QBrush(QColor(0, 255, 0, 127)));
		}
		++it;
	}

	if (inGame && myPlayerId == playerId) {
		isGameAdministrator = true;
		checkPlayerQuantity();
	}
}

void gameLobbyDialogImpl::refreshConnectedPlayerAvatars()
{

	QTreeWidgetItemIterator it(treeWidget_connectedPlayers);
	while (*it) {

		std::string myAvatarFileName;
		PlayerInfo playerInfo(mySession->getClientPlayerInfo((*it)->data(0, Qt::UserRole).toUInt()));

		if(mySession->getAvatarFile(playerInfo.avatar, myAvatarFileName)) {

			QString myAvatarString(QString::fromUtf8(myAvatarFileName.c_str()));
			QFile myAvatarFile(myAvatarString);
			if(myAvatarFile.exists()) {

				QPixmap tempAvatar(myAvatarString);
				QPixmap myAvatarPixmap(25,26);
				myAvatarPixmap.fill(Qt::transparent);
				QPainter p(&myAvatarPixmap);
				p.drawPixmap (0,0,25,25, tempAvatar.scaled(25,25,Qt::KeepAspectRatio,Qt::SmoothTransformation));

				(*it)->setIcon(0, QIcon(myAvatarPixmap));
			}
		}
		++it;
	}
}

void gameLobbyDialogImpl::joinedGameDialogUpdate()
{

	groupBox_GameInfo->setEnabled(true);
	groupBox_GameInfo->setTitle(currentGameName);
	treeWidget_connectedPlayers->clear();
	pushButton_CreateGame->hide();
	pushButton_JoinGame->hide();
	pushButton_Leave->show();
	pushButton_Leave->setEnabled(true);

	//this was added to show game infos even if no game was selected (e.g. invite only game)
	assert(mySession);
	GameInfo info(mySession->getClientGameInfo(mySession->getClientCurrentGameId()));

	groupBox_GameInfo->setTitle(tr("Game Info") + " - " + QString::fromUtf8(info.name.c_str()));

	switch (info.data.gameType) {
	case GAME_TYPE_NORMAL: {
		label_typeIcon->setPixmap(QPixmap(":/gfx/player_play.png"));
		label_typeText->setText(tr("Standard"));
	}
	break;
	case GAME_TYPE_REGISTERED_ONLY: {
		label_typeIcon->setPixmap(QPixmap(":/gfx/registered.png"));
		label_typeText->setText(tr("Registered players only"));
	}
	break;
	case GAME_TYPE_INVITE_ONLY: {
		label_typeIcon->setPixmap(QPixmap(":/gfx/list_add_user.png"));
		label_typeText->setText(tr("Invited players only"));
	}
	break;
	case GAME_TYPE_RANKING: {
		label_typeIcon->setPixmap(QPixmap(":/gfx/cup.png"));
		label_typeText->setText(tr("Ranking game"));
	}
	break;
	}

	showGameDescription(true);
	label_SmallBlind->setText(QString("%L1").arg(info.data.firstSmallBlind));
	label_StartCash->setText(QString("%L1").arg(info.data.startMoney));
	updateDialogBlinds(info.data);
	label_GameTiming->setText(QString::number(info.data.playerActionTimeoutSec)+" "+tr("sec (action)")+"\n"+QString::number(info.data.delayBetweenHandsSec)+" "+tr("sec (hand delay)"));
}

void gameLobbyDialogImpl::leftGameDialogUpdate()
{

	// un-select current game.
	treeView_GameList->clearSelection();

	groupBox_GameInfo->setTitle(tr("Game Info"));
	groupBox_GameInfo->setEnabled(false);
	currentGameName = "";

	showGameDescription(false);
	label_typeIcon->setText(" ");
	label_typeText->setText(" ");
	label_SmallBlind->setText("");
	label_StartCash->setText("");
	label_blindsRaiseIntervall->setText("");
	label_blindsRaiseMode->setText("");
	label_blindsList->setText("");
	label_GameTiming->setText("");

	treeWidget_connectedPlayers->clear();
	treeWidget_connectedSpectators->clear();
	pushButton_StartGame->hide();
	pushButton_Leave->hide();
	pushButton_Kick->hide();
	pushButton_Kick->setEnabled(false);
	checkBox_fillUpWithComputerOpponents->hide();
	pushButton_CreateGame->show();
	pushButton_JoinGame->show();
	pushButton_JoinGame->setEnabled(false);

	lineEdit_ChatInput->setFocus();
}

void gameLobbyDialogImpl::updateDialogBlinds(const GameData &gameData)
{

	if(gameData.raiseIntervalMode == RAISE_ON_HANDNUMBER) {
		label_blindsRaiseIntervall->setText(QString::number(gameData.raiseSmallBlindEveryHandsValue)+" "+tr("hands"));
	} else {
		label_blindsRaiseIntervall->setText(QString::number(gameData.raiseSmallBlindEveryMinutesValue)+" "+tr("minutes"));
	}
	if(gameData.raiseMode == DOUBLE_BLINDS) {
		label_blindsRaiseMode->setText(tr("double blinds"));
		label_blindsList->hide();
		label_gameDesc6->hide();
	} else {
		label_blindsRaiseMode->setText(tr("manual blinds order"));
		label_blindsList->show();
		label_gameDesc6->show();

		QString blindsListString;
		std::list<int>::const_iterator it1;
		for(it1= gameData.manualBlindsList.begin(); it1 != gameData.manualBlindsList.end(); ++it1) {
			blindsListString.append(QString("%L1").arg(*it1)).append(", ");
		}
		blindsListString.remove(blindsListString.length()-2,2);
		label_blindsList->setText(blindsListString);
	}
}

void gameLobbyDialogImpl::playerSelected(QTreeWidgetItem* item, QTreeWidgetItem*)
{

	if (item)
		pushButton_Kick->setEnabled(isGameAdministrator);
}

void gameLobbyDialogImpl::startGame()
{

	assert(mySession);
	mySession->sendStartEvent(checkBox_fillUpWithComputerOpponents->isChecked());
}

void gameLobbyDialogImpl::leaveGame()
{

	assert(mySession);
	mySession->sendLeaveCurrentGame();

	//stop autoStartTimerOverlay
	autoStartTimerOverlay->hide();
	autoStartTimer->stop();

	//stop waitStartGameMsgBoxes
	waitStartGameMsgBoxTimer->stop();
	waitStartGameMsgBox->hide();
	waitRejoinStartGameMsgBox->hide();
}

void gameLobbyDialogImpl::kickPlayer()
{

	QTreeWidgetItem *item = treeWidget_connectedPlayers->currentItem();
	if (item) {
		QString playerName = item->text(0);
		if(playerName == QString::fromUtf8(myConfig->readConfigString("MyName").c_str())) {
			{
				MyMessageBox::warning(this, tr("Server Error"),
									  tr("You should not kick yourself from this game!"),
									  QMessageBox::Close);
			}
		} else {
			assert(mySession);
			mySession->kickPlayer(item->data(0, Qt::UserRole).toUInt());
		}
	}
	pushButton_Kick->setEnabled(false);
}

void gameLobbyDialogImpl::keyPressEvent ( QKeyEvent * event )
{

	//        qDebug() << event->key() << "\n";

	if (event->key() == Qt::Key_Enter && lineEdit_ChatInput->hasFocus()) {
		myChat->sendMessage();
	}

	if (event->key() == Qt::Key_Up && lineEdit_ChatInput->hasFocus()) {
		if((keyUpCounter + 1) <= myChat->getChatLinesHistorySize()) {
			keyUpCounter++;
		}
		// 		std::cout << "Up keyUpCounter: " << keyUpCounter << "\n";
		myChat->showChatHistoryIndex(keyUpCounter);
	} else if(event->key() == Qt::Key_Down && lineEdit_ChatInput->hasFocus()) {
		if((keyUpCounter - 1) >= 0) {
			keyUpCounter--;
		}
		// 		std::cout << "Down keyUpCounter: " << keyUpCounter << "\n";
		myChat->showChatHistoryIndex(keyUpCounter);
	} else {
		keyUpCounter = 0;
	}

	// 	if (event->key() == Qt::Key_Tab) event->ignore(); else { /*blah*/ }

	//        if (event->key() == Qt::Key_N) showInvitationDialog();
	//        if (event->key() == Qt::Key_M) guestUserMode();

}

bool gameLobbyDialogImpl::eventFilter(QObject *obj, QEvent *event)
{
	QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
	QFocusEvent *focusEvent = static_cast<QFocusEvent*>(event);

	if (obj == lineEdit_ChatInput && lineEdit_ChatInput->text() != "" && event->type() == QEvent::KeyPress && keyEvent->key() == Qt::Key_Tab) {
		myChat->nickAutoCompletition();
		return true;
	} else if (obj == lineEdit_searchForPlayers && focusEvent->gotFocus() && lineEdit_searchForPlayers->text() == tr("search for player ...")) {
		lineEdit_searchForPlayers->clear();
		return QDialog::eventFilter(obj, event);
	} else if (event->type() == QEvent::KeyPress && keyEvent->key() == Qt::Key_Back) {
		event->ignore();
		this->reject();
		return false;
	} else {
		// pass the event on to the parent class
		return QDialog::eventFilter(obj, event);
	}
}

bool gameLobbyDialogImpl::event ( QEvent * event )
{

	return QDialog::event(event);
}

void gameLobbyDialogImpl::showGameDescription(bool show)
{

	if(show) {
#ifndef GUI_800x480
		label_gameType->show();
#endif
		label_gameDesc2->show();
		label_gameDesc3->show();
		label_gameDesc4->show();
		label_gameDesc5->show();
		label_gameDesc6->show();
		label_gameDesc7->show();
		checkPlayerQuantity();
	} else {
		label_gameType->hide();
		label_gameDesc2->hide();
		label_gameDesc3->hide();
		label_gameDesc4->hide();
		label_gameDesc5->hide();
		label_gameDesc6->hide();
		label_gameDesc7->hide();
		tabWidget_playerSpectators->setTabText(0, tr("Players (%1)").arg(0));
		tabWidget_playerSpectators->setTabText(1, tr("Spectators (%1)").arg(0));
	}
}

void gameLobbyDialogImpl::showWaitStartGameMsgBox()
{

	if(this->isVisible()) {
		waitStartGameMsgBox->show();
		waitStartGameMsgBox->raise();
		waitStartGameMsgBox->activateWindow();
		pushButton_Leave->setDisabled(true);
	}
}

void gameLobbyDialogImpl::hideWaitStartGameMsgBox()
{
	waitStartGameMsgBox->hide();
	waitRejoinStartGameMsgBox->hide();
}

void gameLobbyDialogImpl::reject()
{
	myStartWindow->show();
	QDialog::reject();
}

void gameLobbyDialogImpl::closeEvent(QCloseEvent *event)
{
	//stop waitStartGameMsgBoxes
	waitStartGameMsgBoxTimer->stop();
	waitStartGameMsgBox->hide();
	waitRejoinStartGameMsgBox->hide();
	//enable leave button again - it was disabled during waitxyzMsgBoxes
	pushButton_Leave->setEnabled(true);
	event->accept();
}

void gameLobbyDialogImpl::accept()
{
	//stop waitStartGameMsgBoxes
	waitStartGameMsgBoxTimer->stop();
	closeAllChildDialogs();
	//enable leave button again - it was disabled during waitxyzMsgBoxes
	pushButton_Leave->setEnabled(true);
	QDialog::accept();
}

void gameLobbyDialogImpl::writeDialogSettings(int saveMode)
{
	switch(saveMode) {
	case 0: {
		QHeaderView *header = treeView_GameList->header();
		myConfig->writeConfigInt("DlgGameLobbyGameListSortingSection", header->sortIndicatorSection());
		myConfig->writeConfigInt("DlgGameLobbyGameListSortingOrder", header->sortIndicatorOrder());
	}
	break;
	case 1: {
		myConfig->writeConfigInt("DlgGameLobbyGameListFilterIndex", comboBox_gameListFilter->currentIndex());
	}
	break;
	case 2: {
		myConfig->writeConfigInt("DlgGameLobbyNickListSortFilterIndex", comboBox_nickListFilter->currentIndex());
	}
	break;
	default:
		;
	}

	myConfig->writeBuffer();
}

void gameLobbyDialogImpl::readDialogSettings()
{
	comboBox_gameListFilter->setCurrentIndex(myConfig->readConfigInt("DlgGameLobbyGameListFilterIndex"));
	treeView_GameList->sortByColumn(myConfig->readConfigInt("DlgGameLobbyGameListSortingSection"), (Qt::SortOrder)myConfig->readConfigInt("DlgGameLobbyGameListSortingOrder") );
	comboBox_nickListFilter->setCurrentIndex(myConfig->readConfigInt("DlgGameLobbyNickListSortFilterIndex"));
}

void gameLobbyDialogImpl::changeGameListFilter(int index)
{

	switch(index) {
	case 0: {
		myGameListSortFilterProxyModel->setColumn1RegExp(QRegExp());
		myGameListSortFilterProxyModel->setColumn2RegExp(QRegExp());
		myGameListSortFilterProxyModel->setColumn3RegExp(QRegExp());
		myGameListSortFilterProxyModel->setColumn4RegExp(QRegExp());
		myGameListSortFilterProxyModel->setColumn5RegExp(QRegExp());
	}
	break;
	case 1: {
		myGameListSortFilterProxyModel->setColumn1RegExp(QRegExp());
		myGameListSortFilterProxyModel->setColumn2RegExp(QRegExp("open", Qt::CaseInsensitive, QRegExp::FixedString));
		myGameListSortFilterProxyModel->setColumn3RegExp(QRegExp());
		myGameListSortFilterProxyModel->setColumn4RegExp(QRegExp());
		myGameListSortFilterProxyModel->setColumn5RegExp(QRegExp());
	}
	break;
	case 2: {
		myGameListSortFilterProxyModel->setColumn1RegExp(QRegExp("nonfull", Qt::CaseInsensitive, QRegExp::FixedString));
		myGameListSortFilterProxyModel->setColumn2RegExp(QRegExp("open", Qt::CaseInsensitive, QRegExp::FixedString));
		myGameListSortFilterProxyModel->setColumn3RegExp(QRegExp());
		myGameListSortFilterProxyModel->setColumn4RegExp(QRegExp());
		myGameListSortFilterProxyModel->setColumn5RegExp(QRegExp());
	}
	break;
	case 3: {
		myGameListSortFilterProxyModel->setColumn1RegExp(QRegExp("nonfull", Qt::CaseInsensitive, QRegExp::FixedString));
		myGameListSortFilterProxyModel->setColumn2RegExp(QRegExp("open", Qt::CaseInsensitive, QRegExp::FixedString));
		myGameListSortFilterProxyModel->setColumn3RegExp(QRegExp());
		myGameListSortFilterProxyModel->setColumn4RegExp(QRegExp("nonpriv", Qt::CaseInsensitive, QRegExp::FixedString));
		myGameListSortFilterProxyModel->setColumn5RegExp(QRegExp());
	}
	break;
	case 4: {
		myGameListSortFilterProxyModel->setColumn1RegExp(QRegExp("nonfull", Qt::CaseInsensitive, QRegExp::FixedString));
		myGameListSortFilterProxyModel->setColumn2RegExp(QRegExp("open", Qt::CaseInsensitive, QRegExp::FixedString));
		myGameListSortFilterProxyModel->setColumn3RegExp(QRegExp());
		myGameListSortFilterProxyModel->setColumn4RegExp(QRegExp("private", Qt::CaseInsensitive, QRegExp::FixedString));
		myGameListSortFilterProxyModel->setColumn5RegExp(QRegExp());
	}
	break;
	case 5: {
		myGameListSortFilterProxyModel->setColumn1RegExp(QRegExp("nonfull", Qt::CaseInsensitive, QRegExp::FixedString));
		myGameListSortFilterProxyModel->setColumn2RegExp(QRegExp("open", Qt::CaseInsensitive, QRegExp::FixedString));
		myGameListSortFilterProxyModel->setColumn3RegExp(QRegExp("ranking", Qt::CaseInsensitive, QRegExp::FixedString));
		myGameListSortFilterProxyModel->setColumn4RegExp(QRegExp());
		myGameListSortFilterProxyModel->setColumn5RegExp(QRegExp());
	}
	break;
	default:
		;
	}
	myGameListSortFilterProxyModel->setFilterRegExp(QString());
	myGameListSortFilterProxyModel->setFilterKeyColumn(0);

	writeDialogSettings(1);

#ifdef GUI_800x480
	if(index) treeView_GameList->setStyleSheet("QTreeView { border-radius: 4px; border: 2px solid blue; background-color: white; background-image: url(\""+myAppDataPath +"gfx/gui/misc/background_gamelist.png\"); background-attachment: fixed; background-position: top center ; background-repeat: no-repeat; color:rgb(0, 0, 0); font: 20px}");
	else treeView_GameList->setStyleSheet("QTreeView { background-color: white; background-image: url(\""+myAppDataPath +"gfx/gui/misc/background_gamelist.png\"); background-attachment: fixed; background-position: top center ; background-repeat: no-repeat; color:rgb(0, 0, 0); font: 20px}");

	treeView_GameList->header()->setStyleSheet("QObject {font: bold 18px}");
#else
	if(index) treeView_GameList->setStyleSheet("QTreeView { border-radius: 4px; border: 2px solid blue; background-color: white; background-image: url(\""+myAppDataPath +"gfx/gui/misc/background_gamelist.png\"); background-attachment: fixed; background-position: top center ; background-repeat: no-repeat;}");
	else treeView_GameList->setStyleSheet("QTreeView { background-color: white; background-image: url(\""+myAppDataPath +"gfx/gui/misc/background_gamelist.png\"); background-attachment: fixed; background-position: top center ; background-repeat: no-repeat;}");
#endif
}

void gameLobbyDialogImpl::changeNickListFilter(int state)
{
	if(lastNickListFilterState == 0) {
		myNickListSortFilterProxyModel->setLastFilterStateCountry(false);
		myNickListSortFilterProxyModel->setLastFilterStateAlpha(true);
	} else if(lastNickListFilterState == 1) {
		myNickListSortFilterProxyModel->setLastFilterStateCountry(true);
		myNickListSortFilterProxyModel->setLastFilterStateAlpha(false);
	}

	myNickListSortFilterProxyModel->setFilterState(state);
	myNickListModel->sort(0, Qt::DescendingOrder);

	myNickListSortFilterProxyModel->setFilterRegExp(QString());
	myNickListSortFilterProxyModel->setFilterKeyColumn(0);

	lastNickListFilterState = state;
	writeDialogSettings(2);
}

void gameLobbyDialogImpl::changeGameListSorting()
{

	writeDialogSettings(0);
}


void gameLobbyDialogImpl::registeredUserMode()
{
	lineEdit_ChatInput->clear();
	lineEdit_ChatInput->setEnabled(true);
	guestMode = false;
}


void gameLobbyDialogImpl::guestUserMode()
{
	lineEdit_ChatInput->setText(tr("Chat is only available to registered players."));
	lineEdit_ChatInput->setDisabled(true);
	guestMode = true;
}

void gameLobbyDialogImpl::showNickListContextMenu(QPoint p)
{
	if(myNickListModel->rowCount() && myNickListSelectionModel->currentIndex().isValid()) {

		assert(mySession);
		unsigned playerUid = myNickListSelectionModel->currentIndex().data(Qt::UserRole).toUInt();

		if(inGame && mySession->getClientGameInfo(mySession->getClientCurrentGameId()).data.gameType == GAME_TYPE_INVITE_ONLY && playerUid != mySession->getClientUniquePlayerId() && !mySession->getClientPlayerInfo(playerUid).isGuest) {
			nickListInviteAction->setEnabled(true);
			nickListInviteAction->setText(tr("Invite %1").arg(QString::fromUtf8(mySession->getClientPlayerInfo(playerUid).playerName.c_str())));

		} else {
			nickListInviteAction->setText(tr("Invite player ..."));
			nickListInviteAction->setEnabled(false);
		}

		if(playerUid != mySession->getClientUniquePlayerId() && !mySession->getClientPlayerInfo(playerUid).isGuest && !playerIsOnIgnoreList(playerUid)) {
			nickListIgnorePlayerAction->setEnabled(true);
			nickListIgnorePlayerAction->setText(tr("Ignore %1").arg(QString::fromUtf8(mySession->getClientPlayerInfo(playerUid).playerName.c_str())));
		} else {
			nickListIgnorePlayerAction->setEnabled(false);
			nickListIgnorePlayerAction->setText(tr("Ignore player ..."));
		}

		if(playerUid != mySession->getClientUniquePlayerId() && !mySession->getClientPlayerInfo(playerUid).isGuest && playerIsOnIgnoreList(playerUid)) {
			nickListUnignorePlayerAction->setEnabled(true);
			nickListUnignorePlayerAction->setText(tr("Unignore %1").arg(QString::fromUtf8(mySession->getClientPlayerInfo(playerUid).playerName.c_str())));
		} else {
			nickListUnignorePlayerAction->setEnabled(false);
			nickListUnignorePlayerAction->setText(tr("Unignore player ..."));
		}

		unsigned gameIdOfPlayer = mySession->getGameIdOfPlayer(playerUid);
		QString playerInGameInfoString;
		if(gameIdOfPlayer) {
			playerInGameInfoString = tr("%1 is playing in \"%2\".").arg(QString::fromUtf8(mySession->getClientPlayerInfo(playerUid).playerName.c_str())).arg(QString::fromUtf8(mySession->getClientGameInfo(gameIdOfPlayer).name.c_str()));
		} else {
			playerInGameInfoString = tr("%1 is not playing at the moment.").arg(QString::fromUtf8(mySession->getClientPlayerInfo(playerUid).playerName.c_str()));
		}
		nickListPlayerInGameInfo->setText(playerInGameInfoString);

		//prevent admin to total kickban himself
		if(playerUid == mySession->getClientUniquePlayerId()) {
			nickListAdminTotalKickBan->setDisabled(true);
		} else {
			nickListAdminTotalKickBan->setEnabled(true);
		}

//		check for admin	and remove admin actions for non-admins
		if(!mySession->getClientPlayerInfo(mySession->getClientUniquePlayerId()).isAdmin) {
			nickListContextMenu->removeAction(nickListAdminSubMenu->menuAction());
		}

		//popup a little more to the right to avaoid double click action
		QPoint tempPoint = p;
		tempPoint.setX(p.x()+5);
		nickListContextMenu->popup(treeView_NickList->mapToGlobal(tempPoint));
	}
}

void gameLobbyDialogImpl::showGameListContextMenu(QPoint p)
{
	if(myGameListModel->rowCount() && myGameListSelectionModel->currentIndex().isValid()) {

		assert(mySession);
		unsigned selectedGameId = myGameListSelectionModel->selectedRows().first().data(Qt::UserRole).toUInt();
		if(selectedGameId == mySession->getClientCurrentGameId()) {
			gameListAdminCloseGame->setDisabled(true);
		} else {
			gameListAdminCloseGame->setEnabled(true);
		}

//		check for admin	and remove admin actions for non-admins
		if(!mySession->getClientPlayerInfo(mySession->getClientUniquePlayerId()).isAdmin) {
			gameListContextMenu->removeAction(gameListAdminSubMenu->menuAction());
		}

		//popup a little more to the right to avaoid double click action
		QPoint tempPoint = p;
		tempPoint.setX(p.x()+5);
		gameListContextMenu->popup(treeView_GameList->mapToGlobal(tempPoint));
	}
}

void gameLobbyDialogImpl::invitePlayerToCurrentGame()
{
	if(myNickListSelectionModel->currentIndex().isValid()) {
		mySession->invitePlayerToCurrentGame(myNickListSelectionModel->currentIndex().data(Qt::UserRole).toUInt());
	}
}

void gameLobbyDialogImpl::showInfoMsgBox()
{
	switch(infoMsgToShowId) {
	case 2: {
		inviteOnlyInfoMsgBox->setModal(false);
		inviteOnlyInfoMsgBox->show(INFO_AFTER_JOIN_INVITE_GAME, tr("You have entered a game with type \"invite-only\".\nFeel free to invite other players by right-clicking on their nick in the available players list."), tr("PokerTH - Info Message"), QPixmap(":/gfx/ktip.png"), QDialogButtonBox::Ok, true);
	}
	break;
	default:
		;
		break;
	}
}

void gameLobbyDialogImpl::showInvitationDialog(unsigned gameId, unsigned playerIdFrom)
{
	if(inviteDialogIsCurrentlyShown || playerIsOnIgnoreList(playerIdFrom)) {

		mySession->rejectGameInvitation(gameId, DENY_GAME_INVITATION_BUSY);
	} else {

		inviteDialogIsCurrentlyShown = true;

		myMessageDialogImpl dialog(myConfig, this);
		if(dialog.exec(JOIN_INVITE_GAME_QUESTION, tr("You have been invited to the game <b>%1</b> by <b>%2</b>.<br>Would you like to join this game?").arg(QString::fromUtf8(mySession->getClientGameInfo(gameId).name.c_str())).arg(QString::fromUtf8(mySession->getClientPlayerInfo(playerIdFrom).playerName.c_str())), tr("PokerTH - Info Message"), QPixmap(":/gfx/list_add_user_64.png"), QDialogButtonBox::Yes|QDialogButtonBox::No, false)) {

			mySession->acceptGameInvitation(gameId);
			inviteDialogIsCurrentlyShown = false;
		} else {
			mySession->rejectGameInvitation(gameId, DENY_GAME_INVITATION_NO);
			inviteDialogIsCurrentlyShown = false;
		}
	}
}


void gameLobbyDialogImpl::chatInfoPlayerInvitation(unsigned gameId, unsigned playerIdWho, unsigned playerIdFrom)
{
	textBrowser_ChatDisplay->append(tr("<span style='color:blue;'>%1 has been invited to %2 by %3.</span>").arg(QString::fromUtf8(mySession->getClientPlayerInfo(playerIdWho).playerName.c_str())).arg(QString::fromUtf8(mySession->getClientGameInfo(gameId).name.c_str())).arg(QString::fromUtf8(mySession->getClientPlayerInfo(playerIdFrom).playerName.c_str())));
}

void gameLobbyDialogImpl::chatInfoPlayerRejectedInvitation(unsigned gameId, unsigned playerIdWho, DenyGameInvitationReason reason)
{

	QString string;
	if(reason == DENY_GAME_INVITATION_NO) string = tr("<span style='color:red;'>%1 has rejected the invitation to %2.</span>");

	if(reason == DENY_GAME_INVITATION_BUSY) string = tr("<span style='color:red;'>%1 cannot join %2 because he is busy.</span>");

	textBrowser_ChatDisplay->append(string.arg(QString::fromUtf8(mySession->getClientPlayerInfo(playerIdWho).playerName.c_str())).arg(QString::fromUtf8(mySession->getClientGameInfo(gameId).name.c_str())));

}

bool gameLobbyDialogImpl::playerIsOnIgnoreList(unsigned playerId)
{

	list<std::string> playerIgnoreList = myConfig->readConfigStringList("PlayerIgnoreList");
	list<std::string>::iterator it1;
	for(it1= playerIgnoreList.begin(); it1 != playerIgnoreList.end(); ++it1) {

		if(QString::fromUtf8(mySession->getClientPlayerInfo(playerId).playerName.c_str()) == QString::fromUtf8(it1->c_str())) {
			return true;
		}
	}
	return false;
}


void gameLobbyDialogImpl::putPlayerOnIgnoreList()
{
	if(myNickListSelectionModel->currentIndex().isValid()) {
		unsigned playerId = myNickListSelectionModel->currentIndex().data(Qt::UserRole).toUInt();
		if(!playerIsOnIgnoreList(playerId)) {
			myMessageDialogImpl dialog(myConfig, this);
			if(dialog.exec(IGNORE_PLAYER_QUESTION, tr("You will no longer receive chat messages or game invitations from this user.<br>Do you really want to put player <b>%1</b> on your ignore list?").arg(QString::fromUtf8(mySession->getClientPlayerInfo(playerId).playerName.c_str())), tr("PokerTH - Question"), QPixmap(":/gfx/im-ban-user_64.png"), QDialogButtonBox::Yes|QDialogButtonBox::No, false ) == QDialog::Accepted) {
				list<std::string> playerIgnoreList = myConfig->readConfigStringList("PlayerIgnoreList");
				playerIgnoreList.push_back(QString("%1").arg(QString::fromUtf8(mySession->getClientPlayerInfo(playerId).playerName.c_str())).toUtf8().constData());
				myConfig->writeConfigStringList("PlayerIgnoreList", playerIgnoreList);
				myConfig->writeBuffer();
				myChat->refreshIgnoreList();
			}
		}
	}
}

void gameLobbyDialogImpl::removePlayerFromIgnoreList()
{
	if(myNickListSelectionModel->currentIndex().isValid()) {
		unsigned playerId = myNickListSelectionModel->currentIndex().data(Qt::UserRole).toUInt();
		if(playerIsOnIgnoreList(playerId)) {
			myMessageDialogImpl dialog(myConfig, this);
			if(dialog.exec(UNIGNORE_PLAYER_QUESTION, tr("You will receive chat messages and game invitations from this user again!<br>Do you really want to remove player <b>%1</b> from your ignore list?").arg(QString::fromUtf8(mySession->getClientPlayerInfo(playerId).playerName.c_str())), tr("PokerTH - Question"), QPixmap(":/gfx/dialog_ok_apply.png"), QDialogButtonBox::Yes|QDialogButtonBox::No, false ) == QDialog::Accepted) {
				list<std::string> playerIgnoreList = myConfig->readConfigStringList("PlayerIgnoreList");
				playerIgnoreList.remove(QString("%1").arg(QString::fromUtf8(mySession->getClientPlayerInfo(playerId).playerName.c_str())).toUtf8().constData());
				myConfig->writeConfigStringList("PlayerIgnoreList", playerIgnoreList);
				myConfig->writeBuffer();
				myChat->refreshIgnoreList();
			}
		}
	}
}

void gameLobbyDialogImpl::searchForPlayerRegExpChanged()
{
	QRegExp regExp(lineEdit_searchForPlayers->text(), Qt::CaseInsensitive);
	myNickListSortFilterProxyModel->setFilterRegExp(regExp);
}

void gameLobbyDialogImpl::showAutoStartTimer()
{

	autoStartTimerOverlay->show();
	autoStartTimerOverlay->setGeometry(((scrollArea_gameInfos->geometry().width()-190)/2), ((scrollArea_gameInfos->geometry().height()-50)/2), 190, 50);

	QString string(tr("The game will start in<br><b>%1</b> seconds.").arg(6));
	QString fontsize;
#ifdef ANDROID
	fontsize="16px";
#else
	fontsize="10px";
#endif
	autoStartTimerOverlay->setText("<span style='color:#008B00; font-size:"+fontsize+";'>"+string+"</span>");
	autoStartTimerCounter = 6;
	autoStartTimer->start(1000);

}

void gameLobbyDialogImpl::updateAutoStartTimer()
{
	--autoStartTimerCounter;
	if(autoStartTimerCounter) {
		QString string(tr("The game will start in<br><b>%1</b> seconds.").arg(autoStartTimerCounter));
		QString fontsize;
#ifdef ANDROID
		fontsize="16px";
#else
		fontsize="10px";
#endif
		autoStartTimerOverlay->setText("<span style='color:#008B00; font-size:"+fontsize+";'>"+string+"</span>");
	} else {
		autoStartTimer->stop();
		autoStartTimerOverlay->hide();
	}
}

void gameLobbyDialogImpl::openPlayerStats1()
{
	if(myNickListSelectionModel->currentIndex().isValid()) {

		unsigned playerId = myNickListSelectionModel->currentIndex().data(Qt::UserRole).toUInt();
		if(!mySession->getClientPlayerInfo(playerId).isGuest) {
			QUrl url("http://pokerth.net/redirect_user_profile.php?nick="+QUrl::toPercentEncoding(myNickListSelectionModel->currentIndex().data(Qt::DisplayRole).toString()));
			QDesktopServices::openUrl(url);
		}
	}
}

void gameLobbyDialogImpl::openPlayerStats2()
{
	if(treeWidget_connectedPlayers->currentItem()) {

		unsigned playerId = treeWidget_connectedPlayers->currentItem()->data(0, Qt::UserRole).toUInt();
		if(!mySession->getClientPlayerInfo(playerId).isGuest) {
			QUrl url("http://pokerth.net/redirect_user_profile.php?nick="+QUrl::toPercentEncoding(treeWidget_connectedPlayers->currentItem()->data(0, Qt::DisplayRole).toString()));
			QDesktopServices::openUrl(url);
		}
	}
}

void gameLobbyDialogImpl::showConnectedPlayersContextMenu(QPoint p)
{
	if(treeWidget_connectedPlayers->currentItem()) {

		assert(mySession);
		unsigned playerUid = treeWidget_connectedPlayers->currentItem()->data(0, Qt::UserRole).toUInt();

		if(!mySession->getClientPlayerInfo(playerUid).isGuest) {

			//popup a little more to the right to avaoid double click action
			QPoint tempPoint = p;
			tempPoint.setX(p.x()+5);
			connectedPlayersListPlayerInfoSubMenu->popup(treeWidget_connectedPlayers->mapToGlobal(tempPoint));
		}
	}
}

QString gameLobbyDialogImpl::getFullCountryString(QString cs)
{
	// Initialise map of country strings, if needed.
	if (countryStringMap.empty()) {
		countryStringMap.insert(CountryStringMap::value_type("AF", tr("Afghanistan")));
		countryStringMap.insert(CountryStringMap::value_type("AX", tr("ALand Islands")));
		countryStringMap.insert(CountryStringMap::value_type("AL", tr("Albania")));
		countryStringMap.insert(CountryStringMap::value_type("DZ", tr("Algeria")));
		countryStringMap.insert(CountryStringMap::value_type("AS", tr("American Samoa")));
		countryStringMap.insert(CountryStringMap::value_type("AD", tr("Andorra")));
		countryStringMap.insert(CountryStringMap::value_type("AO", tr("Angola")));
		countryStringMap.insert(CountryStringMap::value_type("AI", tr("Anguilla")));
		countryStringMap.insert(CountryStringMap::value_type("AQ", tr("Antarctica")));
		countryStringMap.insert(CountryStringMap::value_type("AG", tr("Antigua And Barbuda")));
		countryStringMap.insert(CountryStringMap::value_type("AR", tr("Argentina")));
		countryStringMap.insert(CountryStringMap::value_type("AM", tr("Armenia")));
		countryStringMap.insert(CountryStringMap::value_type("AW", tr("Aruba")));
		countryStringMap.insert(CountryStringMap::value_type("AU", tr("Australia")));
		countryStringMap.insert(CountryStringMap::value_type("AT", tr("Austria")));
		countryStringMap.insert(CountryStringMap::value_type("AZ", tr("Azerbaijan")));
		countryStringMap.insert(CountryStringMap::value_type("BS", tr("Bahamas")));
		countryStringMap.insert(CountryStringMap::value_type("BH", tr("Bahrain")));
		countryStringMap.insert(CountryStringMap::value_type("BD", tr("Bangladesh")));
		countryStringMap.insert(CountryStringMap::value_type("BB", tr("Barbados")));
		countryStringMap.insert(CountryStringMap::value_type("BY", tr("Belarus")));
		countryStringMap.insert(CountryStringMap::value_type("BE", tr("Belgium")));
		countryStringMap.insert(CountryStringMap::value_type("BZ", tr("Belize")));
		countryStringMap.insert(CountryStringMap::value_type("BJ", tr("Benin")));
		countryStringMap.insert(CountryStringMap::value_type("BM", tr("Bermuda")));
		countryStringMap.insert(CountryStringMap::value_type("BT", tr("Bhutan")));
		countryStringMap.insert(CountryStringMap::value_type("BO", tr("Bolivia")));
		countryStringMap.insert(CountryStringMap::value_type("BA", tr("Bosnia And Herzegovina")));
		countryStringMap.insert(CountryStringMap::value_type("BW", tr("Botswana")));
		countryStringMap.insert(CountryStringMap::value_type("BV", tr("Bouvet Island")));
		countryStringMap.insert(CountryStringMap::value_type("BR", tr("Brazil")));
		countryStringMap.insert(CountryStringMap::value_type("IO", tr("British Indian Ocean Territory")));
		countryStringMap.insert(CountryStringMap::value_type("BN", tr("Brunei Darussalam")));
		countryStringMap.insert(CountryStringMap::value_type("BG", tr("Bulgaria")));
		countryStringMap.insert(CountryStringMap::value_type("BF", tr("Burkina Faso")));
		countryStringMap.insert(CountryStringMap::value_type("BI", tr("Burundi")));
		countryStringMap.insert(CountryStringMap::value_type("KH", tr("Cambodia")));
		countryStringMap.insert(CountryStringMap::value_type("CM", tr("Cameroon")));
		countryStringMap.insert(CountryStringMap::value_type("CA", tr("Canada")));
		countryStringMap.insert(CountryStringMap::value_type("CV", tr("Cape Verde")));
		countryStringMap.insert(CountryStringMap::value_type("KY", tr("Cayman Islands")));
		countryStringMap.insert(CountryStringMap::value_type("CF", tr("Central African Republic")));
		countryStringMap.insert(CountryStringMap::value_type("TD", tr("Chad")));
		countryStringMap.insert(CountryStringMap::value_type("CL", tr("Chile")));
		countryStringMap.insert(CountryStringMap::value_type("CN", tr("China")));
		countryStringMap.insert(CountryStringMap::value_type("CX", tr("Christmas Island")));
		countryStringMap.insert(CountryStringMap::value_type("CC", tr("Cocos (Keeling) Islands")));
		countryStringMap.insert(CountryStringMap::value_type("CO", tr("Colombia")));
		countryStringMap.insert(CountryStringMap::value_type("KM", tr("Comoros")));
		countryStringMap.insert(CountryStringMap::value_type("CG", tr("Congo")));
		countryStringMap.insert(CountryStringMap::value_type("CD", tr("Congo, The Democratic Republic Of The")));
		countryStringMap.insert(CountryStringMap::value_type("CK", tr("Cook Islands")));
		countryStringMap.insert(CountryStringMap::value_type("CR", tr("Costa Rica")));
		countryStringMap.insert(CountryStringMap::value_type("CI", tr("Cote D'Ivoire")));
		countryStringMap.insert(CountryStringMap::value_type("HR", tr("Croatia")));
		countryStringMap.insert(CountryStringMap::value_type("CU", tr("Cuba")));
		countryStringMap.insert(CountryStringMap::value_type("CY", tr("Cyprus")));
		countryStringMap.insert(CountryStringMap::value_type("CZ", tr("Czech Republic")));
		countryStringMap.insert(CountryStringMap::value_type("DK", tr("Denmark")));
		countryStringMap.insert(CountryStringMap::value_type("DJ", tr("Djibouti")));
		countryStringMap.insert(CountryStringMap::value_type("DM", tr("Dominica")));
		countryStringMap.insert(CountryStringMap::value_type("DO", tr("Dominican Republic")));
		countryStringMap.insert(CountryStringMap::value_type("EC", tr("Ecuador")));
		countryStringMap.insert(CountryStringMap::value_type("EG", tr("Egypt")));
		countryStringMap.insert(CountryStringMap::value_type("SV", tr("El Salvador")));
		countryStringMap.insert(CountryStringMap::value_type("GQ", tr("Equatorial Guinea")));
		countryStringMap.insert(CountryStringMap::value_type("ER", tr("Eritrea")));
		countryStringMap.insert(CountryStringMap::value_type("EE", tr("Estonia")));
		countryStringMap.insert(CountryStringMap::value_type("ET", tr("Ethiopia")));
		countryStringMap.insert(CountryStringMap::value_type("FK", tr("Falkland Islands (Malvinas)")));
		countryStringMap.insert(CountryStringMap::value_type("FO", tr("Faroe Islands")));
		countryStringMap.insert(CountryStringMap::value_type("FJ", tr("Fiji")));
		countryStringMap.insert(CountryStringMap::value_type("FI", tr("Finland")));
		countryStringMap.insert(CountryStringMap::value_type("FR", tr("France")));
		countryStringMap.insert(CountryStringMap::value_type("GF", tr("French Guiana")));
		countryStringMap.insert(CountryStringMap::value_type("PF", tr("French Polynesia")));
		countryStringMap.insert(CountryStringMap::value_type("TF", tr("French Southern Territories")));
		countryStringMap.insert(CountryStringMap::value_type("GA", tr("Gabon")));
		countryStringMap.insert(CountryStringMap::value_type("GM", tr("Gambia")));
		countryStringMap.insert(CountryStringMap::value_type("GE", tr("Georgia")));
		countryStringMap.insert(CountryStringMap::value_type("DE", tr("Germany")));
		countryStringMap.insert(CountryStringMap::value_type("GH", tr("Ghana")));
		countryStringMap.insert(CountryStringMap::value_type("GI", tr("Gibraltar")));
		countryStringMap.insert(CountryStringMap::value_type("GR", tr("Greece")));
		countryStringMap.insert(CountryStringMap::value_type("GL", tr("Greenland")));
		countryStringMap.insert(CountryStringMap::value_type("GD", tr("Grenada")));
		countryStringMap.insert(CountryStringMap::value_type("GP", tr("Guadeloupe")));
		countryStringMap.insert(CountryStringMap::value_type("GU", tr("Guam")));
		countryStringMap.insert(CountryStringMap::value_type("GT", tr("Guatemala")));
		countryStringMap.insert(CountryStringMap::value_type("GG", tr("Guernsey")));
		countryStringMap.insert(CountryStringMap::value_type("GN", tr("Guinea")));
		countryStringMap.insert(CountryStringMap::value_type("GW", tr("Guinea-Bissau")));
		countryStringMap.insert(CountryStringMap::value_type("GY", tr("Guyana")));
		countryStringMap.insert(CountryStringMap::value_type("HT", tr("Haiti")));
		countryStringMap.insert(CountryStringMap::value_type("HM", tr("Heard Island And Mcdonald Islands")));
		countryStringMap.insert(CountryStringMap::value_type("VA", tr("Holy See (Vatican City State)")));
		countryStringMap.insert(CountryStringMap::value_type("HN", tr("Honduras")));
		countryStringMap.insert(CountryStringMap::value_type("HK", tr("Hong Kong")));
		countryStringMap.insert(CountryStringMap::value_type("HU", tr("Hungary")));
		countryStringMap.insert(CountryStringMap::value_type("IS", tr("Iceland")));
		countryStringMap.insert(CountryStringMap::value_type("IN", tr("India")));
		countryStringMap.insert(CountryStringMap::value_type("ID", tr("Indonesia")));
		countryStringMap.insert(CountryStringMap::value_type("IR", tr("Iran, Islamic Republic Of")));
		countryStringMap.insert(CountryStringMap::value_type("IQ", tr("Iraq")));
		countryStringMap.insert(CountryStringMap::value_type("IE", tr("Ireland")));
		countryStringMap.insert(CountryStringMap::value_type("IM", tr("Isle Of Man")));
		countryStringMap.insert(CountryStringMap::value_type("IL", tr("Israel")));
		countryStringMap.insert(CountryStringMap::value_type("IT", tr("Italy")));
		countryStringMap.insert(CountryStringMap::value_type("JM", tr("Jamaica")));
		countryStringMap.insert(CountryStringMap::value_type("JP", tr("Japan")));
		countryStringMap.insert(CountryStringMap::value_type("JE", tr("Jersey")));
		countryStringMap.insert(CountryStringMap::value_type("JO", tr("Jordan")));
		countryStringMap.insert(CountryStringMap::value_type("KZ", tr("Kazakhstan")));
		countryStringMap.insert(CountryStringMap::value_type("KE", tr("Kenya")));
		countryStringMap.insert(CountryStringMap::value_type("KI", tr("Kiribati")));
		countryStringMap.insert(CountryStringMap::value_type("KP", tr("Korea, Democratic People'S Republic Of")));
		countryStringMap.insert(CountryStringMap::value_type("KR", tr("Korea, Republic Of")));
		countryStringMap.insert(CountryStringMap::value_type("KW", tr("Kuwait")));
		countryStringMap.insert(CountryStringMap::value_type("KG", tr("Kyrgyzstan")));
		countryStringMap.insert(CountryStringMap::value_type("LA", tr("Lao People'S Democratic Republic")));
		countryStringMap.insert(CountryStringMap::value_type("LV", tr("Latvia")));
		countryStringMap.insert(CountryStringMap::value_type("LB", tr("Lebanon")));
		countryStringMap.insert(CountryStringMap::value_type("LS", tr("Lesotho")));
		countryStringMap.insert(CountryStringMap::value_type("LR", tr("Liberia")));
		countryStringMap.insert(CountryStringMap::value_type("LY", tr("Libyan Arab Jamahiriya")));
		countryStringMap.insert(CountryStringMap::value_type("LI", tr("Liechtenstein")));
		countryStringMap.insert(CountryStringMap::value_type("LT", tr("Lithuania")));
		countryStringMap.insert(CountryStringMap::value_type("LU", tr("Luxembourg")));
		countryStringMap.insert(CountryStringMap::value_type("MO", tr("Macao")));
		countryStringMap.insert(CountryStringMap::value_type("MK", tr("Macedonia, The Former Yugoslav Republic Of")));
		countryStringMap.insert(CountryStringMap::value_type("MG", tr("Madagascar")));
		countryStringMap.insert(CountryStringMap::value_type("MW", tr("Malawi")));
		countryStringMap.insert(CountryStringMap::value_type("MY", tr("Malaysia")));
		countryStringMap.insert(CountryStringMap::value_type("MV", tr("Maldives")));
		countryStringMap.insert(CountryStringMap::value_type("ML", tr("Mali")));
		countryStringMap.insert(CountryStringMap::value_type("MT", tr("Malta")));
		countryStringMap.insert(CountryStringMap::value_type("MH", tr("Marshall Islands")));
		countryStringMap.insert(CountryStringMap::value_type("MQ", tr("Martinique")));
		countryStringMap.insert(CountryStringMap::value_type("MR", tr("Mauritania")));
		countryStringMap.insert(CountryStringMap::value_type("MU", tr("Mauritius")));
		countryStringMap.insert(CountryStringMap::value_type("YT", tr("Mayotte")));
		countryStringMap.insert(CountryStringMap::value_type("MX", tr("Mexico")));
		countryStringMap.insert(CountryStringMap::value_type("FM", tr("Micronesia, Federated States Of")));
		countryStringMap.insert(CountryStringMap::value_type("MD", tr("Moldova, Republic Of")));
		countryStringMap.insert(CountryStringMap::value_type("MC", tr("Monaco")));
		countryStringMap.insert(CountryStringMap::value_type("MN", tr("Mongolia")));
		countryStringMap.insert(CountryStringMap::value_type("MS", tr("Montserrat")));
		countryStringMap.insert(CountryStringMap::value_type("MA", tr("Morocco")));
		countryStringMap.insert(CountryStringMap::value_type("MZ", tr("Mozambique")));
		countryStringMap.insert(CountryStringMap::value_type("MM", tr("Myanmar")));
		countryStringMap.insert(CountryStringMap::value_type("NA", tr("Namibia")));
		countryStringMap.insert(CountryStringMap::value_type("NR", tr("Nauru")));
		countryStringMap.insert(CountryStringMap::value_type("NP", tr("Nepal")));
		countryStringMap.insert(CountryStringMap::value_type("NL", tr("Netherlands")));
		countryStringMap.insert(CountryStringMap::value_type("AN", tr("Netherlands Antilles")));
		countryStringMap.insert(CountryStringMap::value_type("NC", tr("New Caledonia")));
		countryStringMap.insert(CountryStringMap::value_type("NZ", tr("New Zealand")));
		countryStringMap.insert(CountryStringMap::value_type("NI", tr("Nicaragua")));
		countryStringMap.insert(CountryStringMap::value_type("NE", tr("Niger")));
		countryStringMap.insert(CountryStringMap::value_type("NG", tr("Nigeria")));
		countryStringMap.insert(CountryStringMap::value_type("NU", tr("Niue")));
		countryStringMap.insert(CountryStringMap::value_type("NF", tr("Norfolk Island")));
		countryStringMap.insert(CountryStringMap::value_type("MP", tr("Northern Mariana Islands")));
		countryStringMap.insert(CountryStringMap::value_type("NO", tr("Norway")));
		countryStringMap.insert(CountryStringMap::value_type("OM", tr("Oman")));
		countryStringMap.insert(CountryStringMap::value_type("PK", tr("Pakistan")));
		countryStringMap.insert(CountryStringMap::value_type("PW", tr("Palau")));
		countryStringMap.insert(CountryStringMap::value_type("PS", tr("Palestinian Territory, Occupied")));
		countryStringMap.insert(CountryStringMap::value_type("PA", tr("Panama")));
		countryStringMap.insert(CountryStringMap::value_type("PG", tr("Papua New Guinea")));
		countryStringMap.insert(CountryStringMap::value_type("PY", tr("Paraguay")));
		countryStringMap.insert(CountryStringMap::value_type("PE", tr("Peru")));
		countryStringMap.insert(CountryStringMap::value_type("PH", tr("Philippines")));
		countryStringMap.insert(CountryStringMap::value_type("PN", tr("Pitcairn")));
		countryStringMap.insert(CountryStringMap::value_type("PL", tr("Poland")));
		countryStringMap.insert(CountryStringMap::value_type("PT", tr("Portugal")));
		countryStringMap.insert(CountryStringMap::value_type("PR", tr("Puerto Rico")));
		countryStringMap.insert(CountryStringMap::value_type("QA", tr("Qatar")));
		countryStringMap.insert(CountryStringMap::value_type("RE", tr("Reunion")));
		countryStringMap.insert(CountryStringMap::value_type("RO", tr("Romania")));
		countryStringMap.insert(CountryStringMap::value_type("RU", tr("Russian Federation")));
		countryStringMap.insert(CountryStringMap::value_type("RW", tr("Rwanda")));
		countryStringMap.insert(CountryStringMap::value_type("SH", tr("Saint Helena")));
		countryStringMap.insert(CountryStringMap::value_type("KN", tr("Saint Kitts And Nevis")));
		countryStringMap.insert(CountryStringMap::value_type("LC", tr("Saint Lucia")));
		countryStringMap.insert(CountryStringMap::value_type("PM", tr("Saint Pierre And Miquelon")));
		countryStringMap.insert(CountryStringMap::value_type("VC", tr("Saint Vincent And The Grenadines")));
		countryStringMap.insert(CountryStringMap::value_type("WS", tr("Samoa")));
		countryStringMap.insert(CountryStringMap::value_type("SM", tr("San Marino")));
		countryStringMap.insert(CountryStringMap::value_type("ST", tr("Sao Tome And Principe")));
		countryStringMap.insert(CountryStringMap::value_type("SA", tr("Saudi Arabia")));
		countryStringMap.insert(CountryStringMap::value_type("SN", tr("Senegal")));
		countryStringMap.insert(CountryStringMap::value_type("CS", tr("Serbia And Montenegro")));
		countryStringMap.insert(CountryStringMap::value_type("SC", tr("Seychelles")));
		countryStringMap.insert(CountryStringMap::value_type("SL", tr("Sierra Leone")));
		countryStringMap.insert(CountryStringMap::value_type("SG", tr("Singapore")));
		countryStringMap.insert(CountryStringMap::value_type("SK", tr("Slovakia")));
		countryStringMap.insert(CountryStringMap::value_type("SI", tr("Slovenia")));
		countryStringMap.insert(CountryStringMap::value_type("SB", tr("Solomon Islands")));
		countryStringMap.insert(CountryStringMap::value_type("SO", tr("Somalia")));
		countryStringMap.insert(CountryStringMap::value_type("ZA", tr("South Africa")));
		countryStringMap.insert(CountryStringMap::value_type("GS", tr("South Georgia And The South Sandwich Islands")));
		countryStringMap.insert(CountryStringMap::value_type("ES", tr("Spain")));
		countryStringMap.insert(CountryStringMap::value_type("LK", tr("Sri Lanka")));
		countryStringMap.insert(CountryStringMap::value_type("SD", tr("Sudan")));
		countryStringMap.insert(CountryStringMap::value_type("SR", tr("Suriname")));
		countryStringMap.insert(CountryStringMap::value_type("SJ", tr("Svalbard And Jan Mayen")));
		countryStringMap.insert(CountryStringMap::value_type("SZ", tr("Swaziland")));
		countryStringMap.insert(CountryStringMap::value_type("SE", tr("Sweden")));
		countryStringMap.insert(CountryStringMap::value_type("CH", tr("Switzerland")));
		countryStringMap.insert(CountryStringMap::value_type("SY", tr("Syrian Arab Republic")));
		countryStringMap.insert(CountryStringMap::value_type("TW", tr("Taiwan, Province Of China")));
		countryStringMap.insert(CountryStringMap::value_type("TJ", tr("Tajikistan")));
		countryStringMap.insert(CountryStringMap::value_type("TZ", tr("Tanzania, United Republic Of")));
		countryStringMap.insert(CountryStringMap::value_type("TH", tr("Thailand")));
		countryStringMap.insert(CountryStringMap::value_type("TL", tr("Timor-Leste")));
		countryStringMap.insert(CountryStringMap::value_type("TG", tr("Togo")));
		countryStringMap.insert(CountryStringMap::value_type("TK", tr("Tokelau")));
		countryStringMap.insert(CountryStringMap::value_type("TO", tr("Tonga")));
		countryStringMap.insert(CountryStringMap::value_type("TT", tr("Trinidad And Tobago")));
		countryStringMap.insert(CountryStringMap::value_type("TN", tr("Tunisia")));
		countryStringMap.insert(CountryStringMap::value_type("TR", tr("Turkey")));
		countryStringMap.insert(CountryStringMap::value_type("TM", tr("Turkmenistan")));
		countryStringMap.insert(CountryStringMap::value_type("TC", tr("Turks And Caicos Islands")));
		countryStringMap.insert(CountryStringMap::value_type("TV", tr("Tuvalu")));
		countryStringMap.insert(CountryStringMap::value_type("UG", tr("Uganda")));
		countryStringMap.insert(CountryStringMap::value_type("UA", tr("Ukraine")));
		countryStringMap.insert(CountryStringMap::value_type("AE", tr("United Arab Emirates")));
		countryStringMap.insert(CountryStringMap::value_type("GB", tr("United Kingdom")));
		countryStringMap.insert(CountryStringMap::value_type("US", tr("United States")));
		countryStringMap.insert(CountryStringMap::value_type("UM", tr("United States Minor Outlying Islands")));
		countryStringMap.insert(CountryStringMap::value_type("UY", tr("Uruguay")));
		countryStringMap.insert(CountryStringMap::value_type("UZ", tr("Uzbekistan")));
		countryStringMap.insert(CountryStringMap::value_type("VU", tr("Vanuatu")));
		countryStringMap.insert(CountryStringMap::value_type("VE", tr("Venezuela")));
		countryStringMap.insert(CountryStringMap::value_type("VN", tr("Viet Nam")));
		countryStringMap.insert(CountryStringMap::value_type("VG", tr("Virgin Islands, British")));
		countryStringMap.insert(CountryStringMap::value_type("VI", tr("Virgin Islands, U.S.")));
		countryStringMap.insert(CountryStringMap::value_type("WF", tr("Wallis And Futuna")));
		countryStringMap.insert(CountryStringMap::value_type("EH", tr("Western Sahara")));
		countryStringMap.insert(CountryStringMap::value_type("YE", tr("Yemen")));
		countryStringMap.insert(CountryStringMap::value_type("ZM", tr("Zambia")));
		countryStringMap.insert(CountryStringMap::value_type("ZW", tr("Zimbabwe")));
		countryStringMap.insert(CountryStringMap::value_type("CATALONIA", tr("Catalonia")));
	}

	QString fullString("");
	if(!cs.isEmpty()) {
		CountryStringMap::const_iterator pos = countryStringMap.find(cs);
		if (pos != countryStringMap.end()) {
			fullString = pos->second;
		}
	}
	return fullString;
}

void gameLobbyDialogImpl::stopWaitStartGameMsgBoxTimer()
{
	waitStartGameMsgBoxTimer->stop();
}

void gameLobbyDialogImpl::closeAllChildDialogs()
{
	inviteOnlyInfoMsgBox->hide();
	waitStartGameMsgBox->hide();
	waitRejoinStartGameMsgBox->hide();
}

void gameLobbyDialogImpl::reportBadGameName()
{
	assert(mySession);
	if (myGameListSelectionModel->hasSelection()) {
		unsigned gameId = myGameListSelectionModel->selectedRows().first().data(Qt::UserRole).toUInt();
		GameInfo info(mySession->getClientGameInfo(gameId));

		int ret = MyMessageBox::question(this, tr("PokerTH - Question"),
										 tr("Are you sure you want to report the game name:\n\"%1\" as inappropriate?").arg(QString::fromUtf8(info.name.c_str())), QMessageBox::Yes | QMessageBox::No);

		if(ret == QMessageBox::Yes) {
			mySession->reportBadGameName(gameId);
		}
	}
}


void gameLobbyDialogImpl::adminActionCloseGame()
{
	assert(mySession);
	if (myGameListSelectionModel->hasSelection()) {
		unsigned gameId = myGameListSelectionModel->selectedRows().first().data(Qt::UserRole).toUInt();
		GameInfo info(mySession->getClientGameInfo(gameId));

		int ret = MyMessageBox::question(this, tr("PokerTH - Question"),
										 tr("Are you sure you want to close the game:\n\"%1\"?").arg(QString::fromUtf8(info.name.c_str())), QMessageBox::Yes | QMessageBox::No);

		if (ret == QMessageBox::Yes) {
			mySession->adminActionCloseGame(gameId);
		}
	}
}

void gameLobbyDialogImpl::adminActionTotalKickBan()
{
	assert(mySession);
	if (myNickListSelectionModel->hasSelection()) {
		unsigned playerId = myNickListSelectionModel->selectedRows().first().data(Qt::UserRole).toUInt();
		PlayerInfo info(mySession->getClientPlayerInfo(playerId));

		int ret = MyMessageBox::question(this, tr("PokerTH - Question"),
										 tr("Are you sure you want to total kickban the player: \"%1\"?").arg(QString::fromUtf8(info.playerName.c_str())), QMessageBox::Yes | QMessageBox::No);

		if(ret == QMessageBox::Yes) {
			mySession->adminActionBanPlayer(playerId);
		}
	}
}
