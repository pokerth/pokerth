
//
// C++ Implementation: gamelobbydialogimpl
//
// Description:
//
//
// Author: FThauer FHammer LMay <webmaster@pokerth.net>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
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

using namespace std;

gameLobbyDialogImpl::gameLobbyDialogImpl(startWindowImpl *parent, ConfigFile *c)
	: QDialog(parent), myW(NULL), myStartWindow(parent), myConfig(c), currentGameName(""), myPlayerId(0), isGameAdministrator(false), inGame(false), guestMode(false), blinkingButtonAnimationState(true), myChat(NULL), keyUpCounter(0), infoMsgToShowId(0), currentInvitationGameId(0), inviteDialogIsCurrentlyShown(false), autoStartTimerCounter(0)
{

#ifdef __APPLE__
	setWindowModality(Qt::ApplicationModal);
	setWindowFlags(Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint | Qt::Dialog);
#endif
	setupUi(this);

	myAppDataPath = QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str());

	//wait start game message
	waitStartGameMsgBox = new QMessageBox(this);
	waitStartGameMsgBox->setText(tr("Starting game. Please wait ..."));
	waitStartGameMsgBox->setWindowModality(Qt::NonModal);
#ifdef __APPLE__
	waitStartGameMsgBox->setWindowFlags(Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint | Qt::Dialog);
#endif
	waitStartGameMsgBox->setStandardButtons(QMessageBox::NoButton);

	waitStartGameMsgBoxTimer = new QTimer(this);
	waitStartGameMsgBoxTimer->setSingleShot(TRUE);
	blinkingButtonAnimationTimer = new QTimer(this);
	blinkingButtonAnimationTimer->setInterval(1000);
	autoStartTimer = new QTimer(this);
	autoStartTimer->setInterval(1000);
	showInfoMsgBoxTimer = new QTimer(this);
	showInfoMsgBoxTimer->setSingleShot(TRUE);

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
	autoStartTimerOverlay->setWordWrap(TRUE);
	autoStartTimerOverlay->setMaximumWidth(190);
	autoStartTimerOverlay->setMinimumWidth(190);
	autoStartTimerOverlay->setTextFormat(Qt::RichText);
	autoStartTimerOverlay->setAlignment(Qt::AlignCenter);
	autoStartTimerOverlay->setAutoFillBackground(TRUE);
	autoStartTimerOverlay->setFrameStyle(QFrame::StyledPanel);
	QPalette p;
	p.setColor(QPalette::Background, QColor(255, 255, 255, 210));
	autoStartTimerOverlay->setPalette(p);


	myGameListModel = new QStandardItemModel(this);
	myGameListSortFilterProxyModel = new MyGameListSortFilterProxyModel(this);
	myGameListSortFilterProxyModel->setSourceModel(myGameListModel);
	myGameListSortFilterProxyModel->setDynamicSortFilter(TRUE);
	treeView_GameList->setModel(myGameListSortFilterProxyModel);

	myGameListSelectionModel = treeView_GameList->selectionModel();

	QStringList headerList;
	headerList << tr("Game") << tr("Players") << tr("State") << tr("T") << tr("P");
	myGameListModel->setHorizontalHeaderLabels(headerList);

	treeView_GameList->setColumnWidth(0,190);
	treeView_GameList->setColumnWidth(1,65);
	treeView_GameList->setColumnWidth(2,65);
	treeView_GameList->setColumnWidth(3,40);
	treeView_GameList->setColumnWidth(4,40);

	treeView_GameList->setStyleSheet("QTreeView {background-color: white; background-image: url(\""+myAppDataPath +"gfx/gui/misc/background_gamelist.png\"); background-attachment: fixed; background-position: top center ; background-repeat: no-repeat;}");
	treeView_GameList->setAutoFillBackground(TRUE);

	myNickListModel = new QStandardItemModel(this);
	myNickListSortFilterProxyModel = new MyNickListSortFilterProxyModel(this);
	myNickListSortFilterProxyModel->setSourceModel(myNickListModel);
	myNickListSortFilterProxyModel->setDynamicSortFilter(TRUE);
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
	nickListPlayerInfoSubMenu = nickListContextMenu->addMenu(QIcon(":/gfx/dialog-information.png"), tr("Player infos ..."));
	nickListPlayerInGameInfo = new QAction(nickListContextMenu);
	nickListPlayerInfoSubMenu->addAction(nickListPlayerInGameInfo);
	nickListOpenPlayerStats = new QAction(QIcon(":/gfx/view-statistics.png"), tr("Show player stats"), nickListContextMenu);
	nickListContextMenu->addAction(nickListOpenPlayerStats);

	connectedPlayersListPlayerInfoSubMenu = new QMenu();
	connectedPlayersListPlayerInfoSubMenu->addAction(nickListOpenPlayerStats);

	connect( pushButton_CreateGame, SIGNAL( clicked() ), this, SLOT( createGame() ) );
	connect( pushButton_JoinGame, SIGNAL( clicked() ), this, SLOT( joinGame() ) );
	connect( pushButton_joinAnyGame, SIGNAL( clicked() ), this, SLOT( joinAnyGame() ) );
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
	connect( nickListInviteAction, SIGNAL(triggered()), this, SLOT( invitePlayerToCurrentGame() ));
	connect( nickListIgnorePlayerAction, SIGNAL(triggered()), this, SLOT( putPlayerOnIgnoreList() ));
	connect( nickListOpenPlayerStats, SIGNAL(triggered()), this, SLOT( openPlayerStats() ));
	connect( lineEdit_searchForPlayers, SIGNAL(textChanged(QString)),this, SLOT(searchForPlayerRegExpChanged()));

	lineEdit_searchForPlayers->installEventFilter(this);
	lineEdit_ChatInput->installEventFilter(this);

	clearDialog();

}

void gameLobbyDialogImpl::exec()
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


	QDialog::exec();

	waitStartGameMsgBoxTimer->stop();
	waitStartGameMsgBox->hide();
}


gameLobbyDialogImpl::~gameLobbyDialogImpl()
{
	delete myChat;
	myChat = NULL;

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
			bool ok = TRUE;
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

		currentGameName = myCreateInternetGameDialog->lineEdit_gameName->text();

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

		showGameDescription(TRUE);

		label_SmallBlind->setText(QString("%L1").arg(gameData.firstSmallBlind));
		label_StartCash->setText(QString("%L1").arg(gameData.startMoney));

		QTreeWidgetItem *header = treeWidget_connectedPlayers->headerItem();
		header->setText(0, tr("Connected players - max. %1").arg(gameData.maxNumberOfPlayers));
		header->setData(0, Qt::UserRole, gameData.maxNumberOfPlayers);

		updateDialogBlinds(gameData);

		label_TimeoutForPlayerAction->setText(QString::number(gameData.playerActionTimeoutSec));

		mySession->clientCreateGame(gameData, currentGameName.toUtf8().constData(), myCreateInternetGameDialog->lineEdit_Password->text().toUtf8().constData());

	}
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

void gameLobbyDialogImpl::joinAnyGame()
{

	if(comboBox_gameListFilter->currentIndex() != 3 && comboBox_gameListFilter->currentIndex() != 0) comboBox_gameListFilter->setCurrentIndex(0);

	bool found = FALSE;

	int it = 0;
	int gameToJoinId = 0;
	int mostConnectedPlayers = 0;

	while (myGameListModel->item(it)) {

		int players = myGameListModel->item(it, 1)->data(Qt::DisplayRole).toString().section("/",0,0).toInt();
		int maxPlayers = myGameListModel->item(it, 1)->data(Qt::DisplayRole).toString().section("/",1,1).toInt();
		if (myGameListModel->item(it, 2)->data(16) == "open" && myGameListModel->item(it, 4)->data(16) == "nonpriv" && players < maxPlayers) {
			if(players > mostConnectedPlayers) {
				mostConnectedPlayers = players;
				gameToJoinId = it;
			}
			found = TRUE;
		}
		it++;
	}

	if(found) {
		treeView_GameList->setCurrentIndex(myGameListSortFilterProxyModel->mapFromSource(myGameListModel->item(gameToJoinId)->index()));
		joinGame();
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
		headerList << tr("Game") << tr("Players") << tr("State") << tr("T") << tr("P");
		myGameListModel->setHorizontalHeaderLabels(headerList);
		treeView_GameList->setColumnWidth(0,190);
		treeView_GameList->setColumnWidth(1,65);
		treeView_GameList->setColumnWidth(2,65);
		treeView_GameList->setColumnWidth(3,40);
		treeView_GameList->setColumnWidth(4,40);

		QStringList headerList2;
		headerList2 << tr("Available Players");
		myNickListModel->setHorizontalHeaderLabels(headerList2);

		//stop waitStartGameMsgBox
		waitStartGameMsgBoxTimer->stop();
		waitStartGameMsgBox->hide();

		this->accept();
		myW->show();
	} else if(actionID == MSG_NET_GAME_CLIENT_SYNCSTART) {

		waitStartGameMsgBoxTimer->start(2000);
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

		showGameDescription(TRUE);
		label_SmallBlind->setText(QString("%L1").arg(info.data.firstSmallBlind));
		label_StartCash->setText(QString("%L1").arg(info.data.startMoney));
		//		label_MaximumNumberOfPlayers->setText(QString::number(info.data.maxNumberOfPlayers));s
		updateDialogBlinds(info.data);

		label_TimeoutForPlayerAction->setText(QString::number(info.data.playerActionTimeoutSec));

		treeWidget_connectedPlayers->clear();
		PlayerIdList::const_iterator i = info.players.begin();
		PlayerIdList::const_iterator end = info.players.end();
		while (i != end) {
			bool admin = info.adminPlayerId == *i;
			PlayerInfo playerInfo(mySession->getClientPlayerInfo(*i));
			addConnectedPlayer(*i, QString::fromUtf8(playerInfo.playerName.c_str()), admin);
			++i;
		}

		QTreeWidgetItem *header = treeWidget_connectedPlayers->headerItem();
		header->setText(0, tr("Connected players - Max. %1").arg(info.data.maxNumberOfPlayers));
		header->setData(0, Qt::UserRole, info.data.maxNumberOfPlayers);
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
			break;
		} else {
			itemList.at(0)->setData( "", 16);
			itemList.at(0)->setBackground(QBrush());
			itemList.at(1)->setBackground(QBrush());
			itemList.at(2)->setBackground(QBrush());
			itemList.at(3)->setBackground(QBrush());
			itemList.at(4)->setBackground(QBrush());
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

	//	treeView_GameList->sortByColumn(myConfig->readConfigInt("DlgGameLobbyGameListSortingSection"), (Qt::SortOrder)myConfig->readConfigInt("DlgGameLobbyGameListSortingOrder") );
	refreshGameStats();
}

void gameLobbyDialogImpl::addGame(unsigned gameId)
{
	QList <QStandardItem*> itemList;
	QStandardItem *item1 = new QStandardItem();
	QStandardItem *item2 = new QStandardItem();
	QStandardItem *item3 = new QStandardItem();
	QStandardItem *item4 = new QStandardItem();
	QStandardItem *item5 = new QStandardItem();
	itemList << item1 << item2 << item3 << item4 << item5;

	myGameListModel->appendRow(itemList);

	updateGameItem(itemList, gameId);

}

void gameLobbyDialogImpl::updateGameMode(unsigned gameId, int /*newMode*/)
{
	int it = 0;
	while (myGameListModel->item(it)) {
		if (myGameListModel->item(it, 0)->data(Qt::UserRole) == gameId) {
			QList <QStandardItem*> itemList;
			itemList << myGameListModel->item(it, 0) << myGameListModel->item(it, 1) << myGameListModel->item(it, 2) << myGameListModel->item(it, 3) << myGameListModel->item(it, 4);
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

	label_openGamesCounter->setText("| "+tr("running games: %1").arg(runningGamesCounter));
	label_runningGamesCounter->setText("| "+tr("open games: %1").arg(openGamesCounter));

	//refresh joinAnyGameButton state
	joinAnyGameButtonRefresh();

}

void gameLobbyDialogImpl::refreshPlayerStats()
{

	ServerStats stats = mySession->getClientStats();
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
			itemList << myGameListModel->item(it, 0) << myGameListModel->item(it, 1) << myGameListModel->item(it, 2) << myGameListModel->item(it, 3) << myGameListModel->item(it, 4);

			updateGameItem(itemList, gameId);
			break;
		}
		it++;
	}

	//mark player as active
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
			itemList << myGameListModel->item(it, 0) << myGameListModel->item(it, 1) << myGameListModel->item(it, 2) << myGameListModel->item(it, 3) << myGameListModel->item(it, 4);

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

void gameLobbyDialogImpl::updateStats(ServerStats /*stats*/)
{
	refreshPlayerStats();
}

void gameLobbyDialogImpl::clearDialog()
{
	groupBox_GameInfo->setTitle(tr("Game Info"));
	groupBox_GameInfo->setEnabled(false);
	currentGameName = "";

	QTreeWidgetItem *header = treeWidget_connectedPlayers->headerItem();
	header->setText(0, tr("Connected players"));
	header->setData(0, Qt::UserRole, 0);

	showGameDescription(FALSE);
	label_typeIcon->setText(" ");
	label_typeText->setText(" ");
	label_SmallBlind->setText("");
	label_StartCash->setText("");
	label_blindsRaiseIntervall->setText("");
	label_blindsRaiseMode->setText("");
	label_blindsList->setText("");
	label_TimeoutForPlayerAction->setText("");

	myGameListModel->clear();
	myGameListSelectionModel->clear();
	myGameListSelectionModel->clearSelection();
	myGameListSortFilterProxyModel->clear();
	treeView_GameList->show();
	treeWidget_connectedPlayers->clear();

	pushButton_Leave->hide();
	pushButton_Kick->hide();
	pushButton_Kick->setEnabled(false);
	pushButton_StartGame->hide();
	checkBox_fillUpWithComputerOpponents->hide();
	pushButton_CreateGame->show();
	pushButton_JoinGame->show();
	pushButton_JoinGame->setEnabled(false);
	pushButton_joinAnyGame->show();
	pushButton_joinAnyGame->setEnabled(false);

	QStringList headerList;
	headerList << tr("Game") << tr("Players") << tr("State") << tr("T") << tr("P");
	myGameListModel->setHorizontalHeaderLabels(headerList);
	treeView_GameList->setColumnWidth(0,190);
	treeView_GameList->setColumnWidth(1,65);
	treeView_GameList->setColumnWidth(2,65);
	treeView_GameList->setColumnWidth(3,40);
	treeView_GameList->setColumnWidth(4,40);

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

	showGameDescription(FALSE);

	label_connectedPlayersCounter->setText(tr("connected players: %1").arg(0));
	label_openGamesCounter->setText("| "+tr("running games: %1").arg(0));
	label_runningGamesCounter->setText("| "+tr("open games: %1").arg(0));

	readDialogSettings();
}

void gameLobbyDialogImpl::checkPlayerQuantity()
{

	assert(mySession);
	GameInfo info(mySession->getClientGameInfo(mySession->getClientCurrentGameId()));

	if(isGameAdministrator && info.data.gameType != GAME_TYPE_RANKING) {

		pushButton_Kick->show();
		pushButton_StartGame->show();
		checkBox_fillUpWithComputerOpponents->show();

		if (treeWidget_connectedPlayers->topLevelItemCount() >= 2) {
			pushButton_StartGame->setEnabled(true);

			if(treeWidget_connectedPlayers->topLevelItemCount() == treeWidget_connectedPlayers->headerItem()->data(0, Qt::UserRole).toInt()) {
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
	if(treeWidget_connectedPlayers->topLevelItemCount() < treeWidget_connectedPlayers->headerItem()->data(0, Qt::UserRole).toInt()) {
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

	QTreeWidgetItem *item = new QTreeWidgetItem(treeWidget_connectedPlayers, 0);
	item->setData(0, Qt::UserRole, playerId);
	item->setData(0, Qt::DisplayRole, playerName);

	if(isGameAdmin) item->setBackground(0, QBrush(QColor(0, 255, 0, 127)));

	if(this->isVisible() && inGame && myConfig->readConfigInt("PlayNetworkGameNotification")) {
		if(treeWidget_connectedPlayers->topLevelItemCount() < treeWidget_connectedPlayers->headerItem()->data(0, Qt::UserRole).toInt()) {
			myW->getMySDLPlayer()->playSound("playerconnected", 0);
		} else {
			myW->getMySDLPlayer()->playSound("onlinegameready", 0);
			showAutoStartTimer();
		}
	}

	checkPlayerQuantity();

	if (inGame)
		refreshConnectedPlayerAvatars();
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
			} else {
				myNickListModel->item(it1, 0)->setIcon(QIcon(QString(":/cflags/cflags/%1.png").arg(countryString)));
				myNickListModel->item(it1, 0)->setToolTip(getFullCountryString(countryString.toUpper()));
			}

			unsigned gameIdOfPlayer = mySession->getGameIdOfPlayer(playerId);
			if(gameIdOfPlayer) {
				myNickListModel->item(it1, 0)->setData("active", 34);
			} else {
				myNickListModel->item(it1, 0)->setData("idle", 34);
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

	unsigned gameIdOfPlayer = mySession->getGameIdOfPlayer(playerId);
	if(gameIdOfPlayer) {
		item->setData("active", 34);
	} else {
		item->setData("idle", 34);
	}


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
	pushButton_joinAnyGame->hide();
	pushButton_Leave->show();

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

	showGameDescription(TRUE);
	label_SmallBlind->setText(QString("%L1").arg(info.data.firstSmallBlind));
	label_StartCash->setText(QString("%L1").arg(info.data.startMoney));
	updateDialogBlinds(info.data);
	label_TimeoutForPlayerAction->setText(QString::number(info.data.playerActionTimeoutSec));

	QTreeWidgetItem *header = treeWidget_connectedPlayers->headerItem();
	header->setText(0, tr("Connected players - Max. %1").arg(info.data.maxNumberOfPlayers));
	header->setData(0, Qt::UserRole, info.data.maxNumberOfPlayers);
}

void gameLobbyDialogImpl::leftGameDialogUpdate()
{

	// un-select current game.
	treeView_GameList->clearSelection();

	groupBox_GameInfo->setTitle(tr("Game Info"));
	groupBox_GameInfo->setEnabled(false);
	currentGameName = "";

	QTreeWidgetItem *header = treeWidget_connectedPlayers->headerItem();
	header->setText(0, tr("Connected players"));
	header->setData(0, Qt::UserRole, 0);

	showGameDescription(FALSE);
	label_typeIcon->setText(" ");
	label_typeText->setText(" ");
	label_SmallBlind->setText("");
	label_StartCash->setText("");
	label_blindsRaiseIntervall->setText("");
	label_blindsRaiseMode->setText("");
	label_blindsList->setText("");
	label_TimeoutForPlayerAction->setText("");

	treeWidget_connectedPlayers->clear();
	pushButton_StartGame->hide();
	pushButton_Leave->hide();
	pushButton_Kick->hide();
	pushButton_Kick->setEnabled(false);
	checkBox_fillUpWithComputerOpponents->hide();
	pushButton_CreateGame->show();
	pushButton_JoinGame->show();
	pushButton_JoinGame->setEnabled(false);
	pushButton_joinAnyGame->show();
	joinAnyGameButtonRefresh();

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

	//stop waitStartGameMsgBox
	waitStartGameMsgBoxTimer->stop();
	waitStartGameMsgBox->hide();
}

void gameLobbyDialogImpl::kickPlayer()
{

	QTreeWidgetItem *item = treeWidget_connectedPlayers->currentItem();
	if (item) {
		QString playerName = item->text(0);
		if(playerName == QString::fromUtf8(myConfig->readConfigString("MyName").c_str())) {
			{
				QMessageBox::warning(this, tr("Server Error"),
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
		label_gameType->show();
		label_gameDesc2->show();
		label_gameDesc3->show();
		label_gameDesc4->show();
		label_gameDesc5->show();
		label_gameDesc6->show();
		label_gameDesc7->show();
	} else {
		label_gameType->hide();
		label_gameDesc2->hide();
		label_gameDesc3->hide();
		label_gameDesc4->hide();
		label_gameDesc5->hide();
		label_gameDesc6->hide();
		label_gameDesc7->hide();
	}
}

void gameLobbyDialogImpl::showWaitStartGameMsgBox()
{

	if(this->isVisible()) {
		waitStartGameMsgBox->show();
		waitStartGameMsgBox->raise();
		waitStartGameMsgBox->activateWindow();
	}
}

void gameLobbyDialogImpl::joinAnyGameButtonRefresh()
{

	int openNonPrivateNonFullGamesCounter = 0;

	int it = 0;
	while (myGameListModel->item(it)) {

		int players = myGameListModel->item(it, 1)->data(Qt::DisplayRole).toString().section("/",0,0).toInt();
		int maxPlayers = myGameListModel->item(it, 1)->data(Qt::DisplayRole).toString().section("/",1,1).toInt();
		if (myGameListModel->item(it, 2)->data(16) == "open" && myGameListModel->item(it, 4)->data(16) == "nonpriv" && players < maxPlayers) {
			openNonPrivateNonFullGamesCounter++;
		}
		++it;
	}

	if(openNonPrivateNonFullGamesCounter) pushButton_joinAnyGame->setEnabled(TRUE);
	else pushButton_joinAnyGame->setEnabled(FALSE);
}

void gameLobbyDialogImpl::reject()
{
	myStartWindow->show();
	QDialog::reject();
}

void gameLobbyDialogImpl::closeEvent(QCloseEvent *event)
{
	event->accept();
	//stop waitStartGameMsgBox
	waitStartGameMsgBoxTimer->stop();
	waitStartGameMsgBox->hide();
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
	}
	break;
	case 1: {
		myGameListSortFilterProxyModel->setColumn1RegExp(QRegExp());
		myGameListSortFilterProxyModel->setColumn2RegExp(QRegExp("open", Qt::CaseInsensitive, QRegExp::FixedString));
		myGameListSortFilterProxyModel->setColumn3RegExp(QRegExp());
		myGameListSortFilterProxyModel->setColumn4RegExp(QRegExp());
	}
	break;
	case 2: {
		myGameListSortFilterProxyModel->setColumn1RegExp(QRegExp("nonfull", Qt::CaseInsensitive, QRegExp::FixedString));
		myGameListSortFilterProxyModel->setColumn2RegExp(QRegExp("open", Qt::CaseInsensitive, QRegExp::FixedString));
		myGameListSortFilterProxyModel->setColumn3RegExp(QRegExp());
		myGameListSortFilterProxyModel->setColumn4RegExp(QRegExp());
	}
	break;
	case 3: {
		myGameListSortFilterProxyModel->setColumn1RegExp(QRegExp("nonfull", Qt::CaseInsensitive, QRegExp::FixedString));
		myGameListSortFilterProxyModel->setColumn2RegExp(QRegExp("open", Qt::CaseInsensitive, QRegExp::FixedString));
		myGameListSortFilterProxyModel->setColumn3RegExp(QRegExp());
		myGameListSortFilterProxyModel->setColumn4RegExp(QRegExp("nonpriv", Qt::CaseInsensitive, QRegExp::FixedString));
	}
	break;
	case 4: {
		myGameListSortFilterProxyModel->setColumn1RegExp(QRegExp("nonfull", Qt::CaseInsensitive, QRegExp::FixedString));
		myGameListSortFilterProxyModel->setColumn2RegExp(QRegExp("open", Qt::CaseInsensitive, QRegExp::FixedString));
		myGameListSortFilterProxyModel->setColumn3RegExp(QRegExp());
		myGameListSortFilterProxyModel->setColumn4RegExp(QRegExp("private", Qt::CaseInsensitive, QRegExp::FixedString));
	}
	break;
	case 5: {
		myGameListSortFilterProxyModel->setColumn1RegExp(QRegExp("nonfull", Qt::CaseInsensitive, QRegExp::FixedString));
		myGameListSortFilterProxyModel->setColumn2RegExp(QRegExp("open", Qt::CaseInsensitive, QRegExp::FixedString));
		myGameListSortFilterProxyModel->setColumn3RegExp(QRegExp("ranking", Qt::CaseInsensitive, QRegExp::FixedString));
		myGameListSortFilterProxyModel->setColumn4RegExp(QRegExp());
	}
	break;
	default:
		;
	}
	myGameListSortFilterProxyModel->setFilterRegExp(QString());
	myGameListSortFilterProxyModel->setFilterKeyColumn(0);

	writeDialogSettings(1);

	if(index) treeView_GameList->setStyleSheet("QTreeView { border-radius: 4px; border: 2px solid blue; background-color: white; background-image: url(\""+myAppDataPath +"gfx/gui/misc/background_gamelist.png\"); background-attachment: fixed; background-position: top center ; background-repeat: no-repeat;}");
	else treeView_GameList->setStyleSheet("QTreeView { background-color: white; background-image: url(\""+myAppDataPath +"gfx/gui/misc/background_gamelist.png\"); background-attachment: fixed; background-position: top center ; background-repeat: no-repeat;}");

}

void gameLobbyDialogImpl::changeNickListFilter(int state)
{
	myNickListSortFilterProxyModel->setFilterState(state);
	myNickListModel->sort(0, Qt::DescendingOrder);

	myNickListSortFilterProxyModel->setFilterRegExp(QString());
	myNickListSortFilterProxyModel->setFilterKeyColumn(0);

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

		if(playerUid != mySession->getClientUniquePlayerId() && !mySession->getClientPlayerInfo(playerUid).isGuest) {

			nickListIgnorePlayerAction->setEnabled(true);
			nickListIgnorePlayerAction->setText(tr("Ignore %1").arg(QString::fromUtf8(mySession->getClientPlayerInfo(playerUid).playerName.c_str())));
		} else {

			nickListIgnorePlayerAction->setEnabled(false);
			nickListIgnorePlayerAction->setText(tr("Ignore player ..."));
		}


		unsigned gameIdOfPlayer = mySession->getGameIdOfPlayer(playerUid);
		QString playerInGameInfoString;
		if(gameIdOfPlayer) {
			playerInGameInfoString = tr("%1 is playing in \"%2\".").arg(QString::fromUtf8(mySession->getClientPlayerInfo(playerUid).playerName.c_str())).arg(QString::fromUtf8(mySession->getClientGameInfo(gameIdOfPlayer).name.c_str()));
		} else {
			playerInGameInfoString = tr("%1 is not playing at the moment.").arg(QString::fromUtf8(mySession->getClientPlayerInfo(playerUid).playerName.c_str()));
		}
		nickListPlayerInGameInfo->setText(playerInGameInfoString);

		//popup a little more to the right to avaoid double click action
		QPoint tempPoint = p;
		tempPoint.setX(p.x()+5);
		nickListContextMenu->popup(treeView_NickList->mapToGlobal(tempPoint));
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
		myMessageDialogImpl dialog(myConfig, this);
		dialog.exec(2, tr("You have entered a game with type \"invite-only\".\nFeel free to invite other players by right-clicking on their nick in the available players list."), tr("PokerTH - Info Message"), QPixmap(":/gfx/ktip.png"), QDialogButtonBox::Ok, true);
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
		if(dialog.exec(3, tr("You have been invited to the game <b>%1</b> by <b>%2</b>.<br>Would you like to join this game?").arg(QString::fromUtf8(mySession->getClientGameInfo(gameId).name.c_str())).arg(QString::fromUtf8(mySession->getClientPlayerInfo(playerIdFrom).playerName.c_str())), tr("PokerTH - Info Message"), QPixmap(":/gfx/list_add_user_64.png"), QDialogButtonBox::Yes|QDialogButtonBox::No, false)) {

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
			if(dialog.exec(4, tr("You will no longer receive chat messages or game invitations from this user.<br>Do you really want to put player <b>%1</b> on your ignore list?").arg(QString::fromUtf8(mySession->getClientPlayerInfo(playerId).playerName.c_str())), tr("PokerTH - Question"), QPixmap(":/gfx/im-ban-user_64.png"), QDialogButtonBox::Yes|QDialogButtonBox::No, false ) == QDialog::Accepted) {

				list<std::string> playerIgnoreList = myConfig->readConfigStringList("PlayerIgnoreList");
				playerIgnoreList.push_back(QString("%1").arg(QString::fromUtf8(mySession->getClientPlayerInfo(playerId).playerName.c_str())).toUtf8().constData());
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
	autoStartTimerOverlay->setText("<span style='color:#008B00; font-size:9pt;'>"+string+"</span>");
	autoStartTimerCounter = 6;
	autoStartTimer->start(1000);

}

void gameLobbyDialogImpl::updateAutoStartTimer()
{
	--autoStartTimerCounter;
	if(autoStartTimerCounter) {
		QString string(tr("The game will start in<br><b>%1</b> seconds.").arg(autoStartTimerCounter));
		autoStartTimerOverlay->setText("<span style='color:#008B00; font-size:9pt;'>"+string+"</span>");
	} else {
		autoStartTimer->stop();
		autoStartTimerOverlay->hide();
	}
}

void gameLobbyDialogImpl::openPlayerStats()
{
	if(myNickListSelectionModel->currentIndex().isValid()) {

		unsigned playerId = myNickListSelectionModel->currentIndex().data(Qt::UserRole).toUInt();
		if(!mySession->getClientPlayerInfo(playerId).isGuest) {
			QUrl url("http://pokerth.net/redirect_user_profile.php?nick="+QUrl::toPercentEncoding(myNickListSelectionModel->currentIndex().data(Qt::DisplayRole).toString()));
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
	QString fullString("");
	if(cs == "") fullString = "";
	else if(cs == "AF") fullString = tr("Afghanistan");
	else if(cs == "AX") fullString = tr("ALand Islands");
	else if(cs == "AL") fullString = tr("Albania");
	else if(cs == "DZ") fullString = tr("Algeria");
	else if(cs == "AS") fullString = tr("American Samoa");
	else if(cs == "AD") fullString = tr("Andorra");
	else if(cs == "AO") fullString = tr("Angola");
	else if(cs == "AI") fullString = tr("Anguilla");
	else if(cs == "AQ") fullString = tr("Antarctica");
	else if(cs == "AG") fullString = tr("Antigua And Barbuda");
	else if(cs == "AR") fullString = tr("Argentina");
	else if(cs == "AM") fullString = tr("Armenia");
	else if(cs == "AW") fullString = tr("Aruba");
	else if(cs == "AU") fullString = tr("Australia");
	else if(cs == "AT") fullString = tr("Austria");
	else if(cs == "AZ") fullString = tr("Azerbaijan");
	else if(cs == "BS") fullString = tr("Bahamas");
	else if(cs == "BH") fullString = tr("Bahrain");
	else if(cs == "BD") fullString = tr("Bangladesh");
	else if(cs == "BB") fullString = tr("Barbados");
	else if(cs == "BY") fullString = tr("Belarus");
	else if(cs == "BE") fullString = tr("Belgium");
	else if(cs == "BZ") fullString = tr("Belize");
	else if(cs == "BJ") fullString = tr("Benin");
	else if(cs == "BM") fullString = tr("Bermuda");
	else if(cs == "BT") fullString = tr("Bhutan");
	else if(cs == "BO") fullString = tr("Bolivia");
	else if(cs == "BA") fullString = tr("Bosnia And Herzegovina");
	else if(cs == "BW") fullString = tr("Botswana");
	else if(cs == "BV") fullString = tr("Bouvet Island");
	else if(cs == "BR") fullString = tr("Brazil");
	else if(cs == "IO") fullString = tr("British Indian Ocean Territory");
	else if(cs == "BN") fullString = tr("Brunei Darussalam");
	else if(cs == "BG") fullString = tr("Bulgaria");
	else if(cs == "BF") fullString = tr("Burkina Faso");
	else if(cs == "BI") fullString = tr("Burundi");
	else if(cs == "KH") fullString = tr("Cambodia");
	else if(cs == "CM") fullString = tr("Cameroon");
	else if(cs == "CA") fullString = tr("Canada");
	else if(cs == "CV") fullString = tr("Cape Verde");
	else if(cs == "KY") fullString = tr("Cayman Islands");
	else if(cs == "CF") fullString = tr("Central African Republic");
	else if(cs == "TD") fullString = tr("Chad");
	else if(cs == "CL") fullString = tr("Chile");
	else if(cs == "CN") fullString = tr("China");
	else if(cs == "CX") fullString = tr("Christmas Island");
	else if(cs == "CC") fullString = tr("Cocos (Keeling) Islands");
	else if(cs == "CO") fullString = tr("Colombia");
	else if(cs == "KM") fullString = tr("Comoros");
	else if(cs == "CG") fullString = tr("Congo");
	else if(cs == "CD") fullString = tr("Congo, The Democratic Republic Of The");
	else if(cs == "CK") fullString = tr("Cook Islands");
	else if(cs == "CR") fullString = tr("Costa Rica");
	else if(cs == "CI") fullString = tr("Cote D'Ivoire");
	else if(cs == "HR") fullString = tr("Croatia");
	else if(cs == "CU") fullString = tr("Cuba");
	else if(cs == "CY") fullString = tr("Cyprus");
	else if(cs == "CZ") fullString = tr("Czech Republic");
	else if(cs == "DK") fullString = tr("Denmark");
	else if(cs == "DJ") fullString = tr("Djibouti");
	else if(cs == "DM") fullString = tr("Dominica");
	else if(cs == "DO") fullString = tr("Dominican Republic");
	else if(cs == "EC") fullString = tr("Ecuador");
	else if(cs == "EG") fullString = tr("Egypt");
	else if(cs == "SV") fullString = tr("El Salvador");
	else if(cs == "GQ") fullString = tr("Equatorial Guinea");
	else if(cs == "ER") fullString = tr("Eritrea");
	else if(cs == "EE") fullString = tr("Estonia");
	else if(cs == "ET") fullString = tr("Ethiopia");
	else if(cs == "FK") fullString = tr("Falkland Islands (Malvinas)");
	else if(cs == "FO") fullString = tr("Faroe Islands");
	else if(cs == "FJ") fullString = tr("Fiji");
	else if(cs == "FI") fullString = tr("Finland");
	else if(cs == "FR") fullString = tr("France");
	else if(cs == "GF") fullString = tr("French Guiana");
	else if(cs == "PF") fullString = tr("French Polynesia");
	else if(cs == "TF") fullString = tr("French Southern Territories");
	else if(cs == "GA") fullString = tr("Gabon");
	else if(cs == "GM") fullString = tr("Gambia");
	else if(cs == "GE") fullString = tr("Georgia");
	else if(cs == "DE") fullString = tr("Germany");
	else if(cs == "GH") fullString = tr("Ghana");
	else if(cs == "GI") fullString = tr("Gibraltar");
	else if(cs == "GR") fullString = tr("Greece");
	else if(cs == "GL") fullString = tr("Greenland");
	else if(cs == "GD") fullString = tr("Grenada");
	else if(cs == "GP") fullString = tr("Guadeloupe");
	else if(cs == "GU") fullString = tr("Guam");
	else if(cs == "GT") fullString = tr("Guatemala");
	else if(cs == "GG") fullString = tr("Guernsey");
	else if(cs == "GN") fullString = tr("Guinea");
	else if(cs == "GW") fullString = tr("Guinea-Bissau");
	else if(cs == "GY") fullString = tr("Guyana");
	else if(cs == "HT") fullString = tr("Haiti");
	else if(cs == "HM") fullString = tr("Heard Island And Mcdonald Islands");
	else if(cs == "VA") fullString = tr("Holy See (Vatican City State)");
	else if(cs == "HN") fullString = tr("Honduras");
	else if(cs == "HK") fullString = tr("Hong Kong");
	else if(cs == "HU") fullString = tr("Hungary");
	else if(cs == "IS") fullString = tr("Iceland");
	else if(cs == "IN") fullString = tr("India");
	else if(cs == "ID") fullString = tr("Indonesia");
	else if(cs == "IR") fullString = tr("Iran, Islamic Republic Of");
	else if(cs == "IQ") fullString = tr("Iraq");
	else if(cs == "IE") fullString = tr("Ireland");
	else if(cs == "IM") fullString = tr("Isle Of Man");
	else if(cs == "IL") fullString = tr("Israel");
	else if(cs == "IT") fullString = tr("Italy");
	else if(cs == "JM") fullString = tr("Jamaica");
	else if(cs == "JP") fullString = tr("Japan");
	else if(cs == "JE") fullString = tr("Jersey");
	else if(cs == "JO") fullString = tr("Jordan");
	else if(cs == "KZ") fullString = tr("Kazakhstan");
	else if(cs == "KE") fullString = tr("Kenya");
	else if(cs == "KI") fullString = tr("Kiribati");
	else if(cs == "KP") fullString = tr("Korea, Democratic People'S Republic Of");
	else if(cs == "KR") fullString = tr("Korea, Republic Of");
	else if(cs == "KW") fullString = tr("Kuwait");
	else if(cs == "KG") fullString = tr("Kyrgyzstan");
	else if(cs == "LA") fullString = tr("Lao People'S Democratic Republic");
	else if(cs == "LV") fullString = tr("Latvia");
	else if(cs == "LB") fullString = tr("Lebanon");
	else if(cs == "LS") fullString = tr("Lesotho");
	else if(cs == "LR") fullString = tr("Liberia");
	else if(cs == "LY") fullString = tr("Libyan Arab Jamahiriya");
	else if(cs == "LI") fullString = tr("Liechtenstein");
	else if(cs == "LT") fullString = tr("Lithuania");
	else if(cs == "LU") fullString = tr("Luxembourg");
	else if(cs == "MO") fullString = tr("Macao");
	else if(cs == "MK") fullString = tr("Macedonia, The Former Yugoslav Republic Of");
	else if(cs == "MG") fullString = tr("Madagascar");
	else if(cs == "MW") fullString = tr("Malawi");
	else if(cs == "MY") fullString = tr("Malaysia");
	else if(cs == "MV") fullString = tr("Maldives");
	else if(cs == "ML") fullString = tr("Mali");
	else if(cs == "MT") fullString = tr("Malta");
	else if(cs == "MH") fullString = tr("Marshall Islands");
	else if(cs == "MQ") fullString = tr("Martinique");
	else if(cs == "MR") fullString = tr("Mauritania");
	else if(cs == "MU") fullString = tr("Mauritius");
	else if(cs == "YT") fullString = tr("Mayotte");
	else if(cs == "MX") fullString = tr("Mexico");
	else if(cs == "FM") fullString = tr("Micronesia, Federated States Of");
	else if(cs == "MD") fullString = tr("Moldova, Republic Of");
	else if(cs == "MC") fullString = tr("Monaco");
	else if(cs == "MN") fullString = tr("Mongolia");
	else if(cs == "MS") fullString = tr("Montserrat");
	else if(cs == "MA") fullString = tr("Morocco");
	else if(cs == "MZ") fullString = tr("Mozambique");
	else if(cs == "MM") fullString = tr("Myanmar");
	else if(cs == "NA") fullString = tr("Namibia");
	else if(cs == "NR") fullString = tr("Nauru");
	else if(cs == "NP") fullString = tr("Nepal");
	else if(cs == "NL") fullString = tr("Netherlands");
	else if(cs == "AN") fullString = tr("Netherlands Antilles");
	else if(cs == "NC") fullString = tr("New Caledonia");
	else if(cs == "NZ") fullString = tr("New Zealand");
	else if(cs == "NI") fullString = tr("Nicaragua");
	else if(cs == "NE") fullString = tr("Niger");
	else if(cs == "NG") fullString = tr("Nigeria");
	else if(cs == "NU") fullString = tr("Niue");
	else if(cs == "NF") fullString = tr("Norfolk Island");
	else if(cs == "MP") fullString = tr("Northern Mariana Islands");
	else if(cs == "NO") fullString = tr("Norway");
	else if(cs == "OM") fullString = tr("Oman");
	else if(cs == "PK") fullString = tr("Pakistan");
	else if(cs == "PW") fullString = tr("Palau");
	else if(cs == "PS") fullString = tr("Palestinian Territory, Occupied");
	else if(cs == "PA") fullString = tr("Panama");
	else if(cs == "PG") fullString = tr("Papua New Guinea");
	else if(cs == "PY") fullString = tr("Paraguay");
	else if(cs == "PE") fullString = tr("Peru");
	else if(cs == "PH") fullString = tr("Philippines");
	else if(cs == "PN") fullString = tr("Pitcairn");
	else if(cs == "PL") fullString = tr("Poland");
	else if(cs == "PT") fullString = tr("Portugal");
	else if(cs == "PR") fullString = tr("Puerto Rico");
	else if(cs == "QA") fullString = tr("Qatar");
	else if(cs == "RE") fullString = tr("Reunion");
	else if(cs == "RO") fullString = tr("Romania");
	else if(cs == "RU") fullString = tr("Russian Federation");
	else if(cs == "RW") fullString = tr("Rwanda");
	else if(cs == "SH") fullString = tr("Saint Helena");
	else if(cs == "KN") fullString = tr("Saint Kitts And Nevis");
	else if(cs == "LC") fullString = tr("Saint Lucia");
	else if(cs == "PM") fullString = tr("Saint Pierre And Miquelon");
	else if(cs == "VC") fullString = tr("Saint Vincent And The Grenadines");
	else if(cs == "WS") fullString = tr("Samoa");
	else if(cs == "SM") fullString = tr("San Marino");
	else if(cs == "ST") fullString = tr("Sao Tome And Principe");
	else if(cs == "SA") fullString = tr("Saudi Arabia");
	else if(cs == "SN") fullString = tr("Senegal");
	else if(cs == "CS") fullString = tr("Serbia And Montenegro");
	else if(cs == "SC") fullString = tr("Seychelles");
	else if(cs == "SL") fullString = tr("Sierra Leone");
	else if(cs == "SG") fullString = tr("Singapore");
	else if(cs == "SK") fullString = tr("Slovakia");
	else if(cs == "SI") fullString = tr("Slovenia");
	else if(cs == "SB") fullString = tr("Solomon Islands");
	else if(cs == "SO") fullString = tr("Somalia");
	else if(cs == "ZA") fullString = tr("South Africa");
	else if(cs == "GS") fullString = tr("South Georgia And The South Sandwich Islands");
	else if(cs == "ES") fullString = tr("Spain");
	else if(cs == "LK") fullString = tr("Sri Lanka");
	else if(cs == "SD") fullString = tr("Sudan");
	else if(cs == "SR") fullString = tr("Suriname");
	else if(cs == "SJ") fullString = tr("Svalbard And Jan Mayen");
	else if(cs == "SZ") fullString = tr("Swaziland");
	else if(cs == "SE") fullString = tr("Sweden");
	else if(cs == "CH") fullString = tr("Switzerland");
	else if(cs == "SY") fullString = tr("Syrian Arab Republic");
	else if(cs == "TW") fullString = tr("Taiwan, Province Of China");
	else if(cs == "TJ") fullString = tr("Tajikistan");
	else if(cs == "TZ") fullString = tr("Tanzania, United Republic Of");
	else if(cs == "TH") fullString = tr("Thailand");
	else if(cs == "TL") fullString = tr("Timor-Leste");
	else if(cs == "TG") fullString = tr("Togo");
	else if(cs == "TK") fullString = tr("Tokelau");
	else if(cs == "TO") fullString = tr("Tonga");
	else if(cs == "TT") fullString = tr("Trinidad And Tobago");
	else if(cs == "TN") fullString = tr("Tunisia");
	else if(cs == "TR") fullString = tr("Turkey");
	else if(cs == "TM") fullString = tr("Turkmenistan");
	else if(cs == "TC") fullString = tr("Turks And Caicos Islands");
	else if(cs == "TV") fullString = tr("Tuvalu");
	else if(cs == "UG") fullString = tr("Uganda");
	else if(cs == "UA") fullString = tr("Ukraine");
	else if(cs == "AE") fullString = tr("United Arab Emirates");
	else if(cs == "GB") fullString = tr("United Kingdom");
	else if(cs == "US") fullString = tr("United States");
	else if(cs == "UM") fullString = tr("United States Minor Outlying Islands");
	else if(cs == "UY") fullString = tr("Uruguay");
	else if(cs == "UZ") fullString = tr("Uzbekistan");
	else if(cs == "VU") fullString = tr("Vanuatu");
	else if(cs == "VE") fullString = tr("Venezuela");
	else if(cs == "VN") fullString = tr("Viet Nam");
	else if(cs == "VG") fullString = tr("Virgin Islands, British");
	else if(cs == "VI") fullString = tr("Virgin Islands, U.S.");
	else if(cs == "WF") fullString = tr("Wallis And Futuna");
	else if(cs == "EH") fullString = tr("Western Sahara");
	else if(cs == "YE") fullString = tr("Yemen");
	else if(cs == "ZM") fullString = tr("Zambia");
	else if(cs == "ZW") fullString = tr("Zimbabwe");

	return fullString;
}
