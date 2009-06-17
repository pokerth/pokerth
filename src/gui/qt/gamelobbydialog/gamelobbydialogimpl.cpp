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
#include "startwindowimpl.h"
#include "lobbychat.h"
#include "changecompleteblindsdialogimpl.h"
#include "session.h"
#include "configfile.h"
#include "gamedata.h"
#include <net/socket_msg.h>

gameLobbyDialogImpl::gameLobbyDialogImpl(startWindowImpl *parent, ConfigFile *c)
 : QDialog(parent), myW(NULL), myStartWindow(parent), myConfig(c), currentGameName(""), myPlayerId(0), myCurrentGameId(0), isAdmin(false), inGame(false), blinkingButtonAnimationState(true), myChat(NULL), keyUpCounter(0)
{

#ifdef __APPLE__
	setWindowModality(Qt::ApplicationModal);
	setWindowFlags(Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint | Qt::Dialog);
#endif	
    	setupUi(this);
	
	myChat = new LobbyChat(this, myConfig);

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
	
	//fetch enabled start button colors for blinking
	groupBox_GameInfo->setEnabled(true);
	pushButton_StartGame->setEnabled(true);
	defaultStartButtonColor = pushButton_StartGame->palette().button().color();
	defaultStartButtonTextColor = pushButton_StartGame->palette().buttonText().color();
	pushButton_StartGame->setEnabled(false);
	groupBox_GameInfo->setEnabled(false);
	
	myGameListModel = new QStandardItemModel(this);
	myGameListSortFilterProxyModel = new MyGameListSortFilterProxyModel(this);
	myGameListSortFilterProxyModel->setSourceModel(myGameListModel);
	myGameListSortFilterProxyModel->setDynamicSortFilter(TRUE);
	treeView_GameList->setModel(myGameListSortFilterProxyModel);
	
	myGameListSelectionModel = treeView_GameList->selectionModel();
	
	QStringList headerList;
	headerList << tr("Game") << tr("Players") << tr("State") << tr("Private"); 
	myGameListModel->setHorizontalHeaderLabels(headerList);
	
	treeView_GameList->setColumnWidth(0,185);
	treeView_GameList->setColumnWidth(1,65);
	treeView_GameList->setColumnWidth(2,65);
	treeView_GameList->setColumnWidth(3,65);
	
	treeView_GameList->setStyleSheet("QTreeView {background-color: white; background-image: url(\""+myAppDataPath +"gfx/gui/misc/background_gamelist.png\"); background-attachment: fixed; background-position: top center ; background-repeat: no-repeat;}");
	treeView_GameList->setAutoFillBackground(TRUE);

	connect( pushButton_CreateGame, SIGNAL( clicked() ), this, SLOT( createGame() ) );
	connect( pushButton_JoinGame, SIGNAL( clicked() ), this, SLOT( joinGame() ) );
	connect( pushButton_joinAnyGame, SIGNAL( clicked() ), this, SLOT( joinAnyGame() ) );
	connect( pushButton_StartGame, SIGNAL( clicked() ), this, SLOT( startGame() ) );
	connect( pushButton_Kick, SIGNAL( clicked() ), this, SLOT( kickPlayer() ) );
	connect( pushButton_Leave, SIGNAL( clicked() ), this, SLOT( leaveGame() ) );
	connect( myGameListSelectionModel, SIGNAL( currentChanged (const QModelIndex &, const QModelIndex &) ), this, SLOT( gameSelected(const QModelIndex &, const QModelIndex &) ) );
	connect( treeView_GameList, SIGNAL( doubleClicked (const QModelIndex &) ), this, SLOT( joinGame() ) );
	connect( treeView_GameList->header(), SIGNAL( sortIndicatorChanged ( int , Qt::SortOrder )), this, SLOT( changeGameListSorting() ) );
	connect( treeWidget_connectedPlayers, SIGNAL( currentItemChanged ( QTreeWidgetItem*, QTreeWidgetItem*) ), this, SLOT( playerSelected(QTreeWidgetItem*, QTreeWidgetItem*) ) );
	connect( lineEdit_ChatInput, SIGNAL( returnPressed () ), myChat, SLOT( sendMessage() ) );
	connect( lineEdit_ChatInput, SIGNAL( textChanged (QString) ), myChat, SLOT( checkInputLength(QString) ) );
	connect( lineEdit_ChatInput, SIGNAL( textEdited (QString) ), myChat, SLOT( setChatTextEdited() ) );
	connect( waitStartGameMsgBoxTimer, SIGNAL(timeout()), this, SLOT( showWaitStartGameMsgBox() ));
	connect( blinkingButtonAnimationTimer, SIGNAL(timeout()), this, SLOT( blinkingStartButtonAnimation() ));
	connect( comboBox_gameListFilter, SIGNAL(currentIndexChanged(int)), this, SLOT(changeGameListFilter(int)));
	
	lineEdit_ChatInput->installEventFilter(this);

	clearDialog();
}

void gameLobbyDialogImpl::exec()
{
	if(myConfig->readConfigInt("UseIRCLobbyChat"))  { 
		groupBox_lobbyChat->show(); 
	}
	else { 
		groupBox_lobbyChat->hide(); 
	}
	readDialogSettings();
	
	QDialog::exec();

	waitStartGameMsgBoxTimer->stop();
	waitStartGameMsgBox->hide();
}


gameLobbyDialogImpl::~gameLobbyDialogImpl()
{
	delete myChat;
	myChat = NULL;

}

void gameLobbyDialogImpl::createGame()
{
	assert(mySession);

	myCreateInternetGameDialog = new createInternetGameDialogImpl(this, myConfig);
	myCreateInternetGameDialog->exec();
	
	if (myCreateInternetGameDialog->result() == QDialog::Accepted ) {

		GameData gameData;
		// Set Game Data
		gameData.maxNumberOfPlayers = myCreateInternetGameDialog->spinBox_quantityPlayers->value();
		gameData.startMoney = myCreateInternetGameDialog->spinBox_startCash->value();
		gameData.firstSmallBlind = myCreateInternetGameDialog->getChangeCompleteBlindsDialog()->spinBox_firstSmallBlind->value();
		
		if(myCreateInternetGameDialog->getChangeCompleteBlindsDialog()->radioButton_raiseBlindsAtHands->isChecked()) { 
			gameData.raiseIntervalMode = RAISE_ON_HANDNUMBER;
			gameData.raiseSmallBlindEveryHandsValue = myCreateInternetGameDialog->getChangeCompleteBlindsDialog()->spinBox_raiseSmallBlindEveryHands->value();
		}
		else { 
			gameData.raiseIntervalMode = RAISE_ON_MINUTES; 
			gameData.raiseSmallBlindEveryMinutesValue = myCreateInternetGameDialog->getChangeCompleteBlindsDialog()->spinBox_raiseSmallBlindEveryMinutes->value();
		}
		
		if(myCreateInternetGameDialog->getChangeCompleteBlindsDialog()->radioButton_alwaysDoubleBlinds->isChecked()) { 
			gameData.raiseMode = DOUBLE_BLINDS; 
		}
		else { 
			gameData.raiseMode = MANUAL_BLINDS_ORDER;
			std::list<int> tempBlindList;
			int i;
			bool ok = TRUE;
			for(i=0; i<myCreateInternetGameDialog->getChangeCompleteBlindsDialog()->listWidget_blinds->count(); i++) {
				tempBlindList.push_back(myCreateInternetGameDialog->getChangeCompleteBlindsDialog()->listWidget_blinds->item(i)->text().toInt(&ok,10));		
			}
			gameData.manualBlindsList = tempBlindList;
			
			if(myCreateInternetGameDialog->getChangeCompleteBlindsDialog()->radioButton_afterThisAlwaysDoubleBlinds->isChecked()) { gameData.afterManualBlindsMode = AFTERMB_DOUBLE_BLINDS; }
			else {
				if(myCreateInternetGameDialog->getChangeCompleteBlindsDialog()->radioButton_afterThisAlwaysRaiseAbout->isChecked()) {
					gameData.afterManualBlindsMode = AFTERMB_RAISE_ABOUT;
					gameData.afterMBAlwaysRaiseValue = myCreateInternetGameDialog->getChangeCompleteBlindsDialog()->spinBox_afterThisAlwaysRaiseValue->value();
				}
				else { gameData.afterManualBlindsMode = AFTERMB_STAY_AT_LAST_BLIND; }	
			}
		}

		gameData.guiSpeed = myCreateInternetGameDialog->spinBox_gameSpeed->value();
		gameData.playerActionTimeoutSec = myCreateInternetGameDialog->spinBox_netTimeOutPlayerAction->value();

		QString gameString(tr("%1's game"));
		currentGameName = gameString.arg(QString::fromUtf8(myConfig->readConfigString("MyName").c_str()));

		hideShowGameDescription(TRUE);

		label_SmallBlind->setText(QString("%L1").arg(gameData.firstSmallBlind));
		label_StartCash->setText(QString("%L1").arg(gameData.startMoney));
		label_MaximumNumberOfPlayers->setText(QString::number(gameData.maxNumberOfPlayers));

		updateDialogBlinds(gameData);

		label_TimeoutForPlayerAction->setText(QString::number(gameData.playerActionTimeoutSec));

		mySession->clientCreateGame(gameData, currentGameName.toUtf8().constData(), myCreateInternetGameDialog->lineEdit_Password->text().toUtf8().constData());
		
	}
}

void gameLobbyDialogImpl::joinGame()
{
	assert(mySession);
	QItemSelectionModel *selection = treeView_GameList->selectionModel();
	if (!inGame && selection->hasSelection())
	{
		unsigned gameId = selection->selectedRows().first().data(Qt::UserRole).toUInt();
		myCurrentGameId = gameId;
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

void gameLobbyDialogImpl::joinAnyGame() {

	if(comboBox_gameListFilter->currentIndex() != 3 && comboBox_gameListFilter->currentIndex() != 0) comboBox_gameListFilter->setCurrentIndex(0);
	
	bool found = FALSE;

	int it = 0; 
	int gameToJoinId = 0;
	int mostConnectedPlayers = 0;
	
	while (myGameListModel->item(it)) {

		int players = myGameListModel->item(it, 1)->data(Qt::DisplayRole).toString().section("/",0,0).toInt();
		int maxPlayers = myGameListModel->item(it, 1)->data(Qt::DisplayRole).toString().section("/",1,1).toInt();
		if (myGameListModel->item(it, 2)->data(16) == "open" && myGameListModel->item(it, 3)->data(16) == "nonpriv" && players < maxPlayers)
		{
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

void gameLobbyDialogImpl::refresh(int actionID) {

	if (actionID == MSG_NET_GAME_CLIENT_START)
	{
		myGameListModel->clear();
		myGameListSelectionModel->clear();
		myGameListSelectionModel->clearSelection();
		myGameListSortFilterProxyModel->clear();
	
		QStringList headerList;
		headerList << tr("Game") << tr("Players") << tr("State") << tr("Private"); 
		myGameListModel->setHorizontalHeaderLabels(headerList);
		treeView_GameList->setColumnWidth(0,185);
		treeView_GameList->setColumnWidth(1,65);
		treeView_GameList->setColumnWidth(2,65);
		treeView_GameList->setColumnWidth(3,65);
		
		this->accept();
		myW->show();
	}
}

void gameLobbyDialogImpl::removedFromGame(int /*reason*/)
{
	inGame = false;
	isAdmin = false;
	leftGameDialogUpdate();
}

void gameLobbyDialogImpl::gameSelected(const QModelIndex &index, const QModelIndex &)
{

	if (!inGame && index.isValid())
	{
		pushButton_JoinGame->setEnabled(true);

		currentGameName = myGameListModel->item(myGameListSortFilterProxyModel->mapToSource(index).row(), 0)->text();

		groupBox_GameInfo->setEnabled(true);
		groupBox_GameInfo->setTitle(tr("Game Info") + " - " + currentGameName);

		assert(mySession);
		GameInfo info(mySession->getClientGameInfo(myGameListModel->item(myGameListSortFilterProxyModel->mapToSource(index).row(), 0)->data(Qt::UserRole).toUInt()));

		hideShowGameDescription(TRUE);		
		label_SmallBlind->setText(QString("%L1").arg(info.data.firstSmallBlind));
		label_StartCash->setText(QString("%L1").arg(info.data.startMoney));
		label_MaximumNumberOfPlayers->setText(QString::number(info.data.maxNumberOfPlayers));

		updateDialogBlinds(info.data);

		label_TimeoutForPlayerAction->setText(QString::number(info.data.playerActionTimeoutSec));

		treeWidget_connectedPlayers->clear();
		PlayerIdList::const_iterator i = info.players.begin();
		PlayerIdList::const_iterator end = info.players.end();
		while (i != end)
		{
			PlayerRights tmpRights = info.adminPlayerId == *i ? PLAYER_RIGHTS_ADMIN : PLAYER_RIGHTS_NORMAL;
			PlayerInfo playerInfo(mySession->getClientPlayerInfo(*i));
			addConnectedPlayer(*i, QString::fromUtf8(playerInfo.playerName.c_str()), tmpRights);
			++i;
		}
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
	
	while (i != end)
	{
		if(myPlayerId == *i) {
			itemList.at(0)->setData( "MeInThisGame", 16);
			itemList.at(0)->setBackground(QBrush(QColor(0, 255, 0, 127)));
			itemList.at(1)->setBackground(QBrush(QColor(0, 255, 0, 127)));
			itemList.at(2)->setBackground(QBrush(QColor(0, 255, 0, 127)));
			itemList.at(3)->setBackground(QBrush(QColor(0, 255, 0, 127)));
			break;
		}
		else {
			itemList.at(0)->setData( "", 16);
			itemList.at(0)->setBackground(QBrush());
			itemList.at(1)->setBackground(QBrush());
			itemList.at(2)->setBackground(QBrush());
			itemList.at(3)->setBackground(QBrush());
		}
		++i;
	}
		
	QString playerStr;
	playerStr.sprintf("%u/%u", (unsigned)info.players.size(), (unsigned)info.data.maxNumberOfPlayers);
	itemList.at(1)->setData(playerStr, Qt::DisplayRole);
	if((unsigned)info.players.size() == (unsigned)info.data.maxNumberOfPlayers) {	itemList.at(1)->setData("totalfull", 16); }
	else { itemList.at(1)->setData("nonfull", 16); }\
	
	if (info.mode == GAME_MODE_STARTED) {
		itemList.at(2)->setData(tr("running"), Qt::DisplayRole);
		itemList.at(2)->setData("running", 16);
	}
	else {
		itemList.at(2)->setData(tr("open"), Qt::DisplayRole);
		itemList.at(2)->setData("open", 16);
	}

	if (info.isPasswordProtected) {
		itemList.at(3)->setIcon(QIcon(myAppDataPath+"gfx/gui/misc/lock.png"));
		itemList.at(3)->setData(" ", Qt::DisplayRole);
		itemList.at(3)->setData("private", 16);
	}
	else {
		itemList.at(3)->setData("", Qt::DisplayRole);
		itemList.at(3)->setData("nonpriv", 16);
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
	itemList << item1 << item2 << item3 << item4;
	
	myGameListModel->appendRow(itemList);
	
	updateGameItem(itemList, gameId);
}

void gameLobbyDialogImpl::updateGameMode(unsigned gameId, int /*newMode*/)
{
	int it = 0;
	while (myGameListModel->item(it)) {
		if (myGameListModel->item(it, 0)->data(Qt::UserRole) == gameId)
		{
			QList <QStandardItem*> itemList;
			itemList << myGameListModel->item(it, 0) << myGameListModel->item(it, 1) << myGameListModel->item(it, 2) << myGameListModel->item(it, 3);
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
		if (myGameListModel->item(it, 0)->data(Qt::UserRole) == gameId)
		{
			myGameListModel->removeRow(it);
			break;
		}
		it++;
	}
	
	refreshGameStats();
}

void gameLobbyDialogImpl::refreshGameStats() {

	int runningGamesCounter = 0;
	int openGamesCounter = 0;

	int it = 0;
	while (myGameListModel->item(it)) {
		if (myGameListModel->item(it, 2)->data(16) == "running") { runningGamesCounter++; }
		if (myGameListModel->item(it, 2)->data(16) == "open") { openGamesCounter++; }
		++it;
	}

	label_openGamesCounter->setText("| "+tr("running games: %1").arg(runningGamesCounter));
	label_runningGamesCounter->setText("| "+tr("open games: %1").arg(openGamesCounter));

	//refresh joinAnyGameButton state
	joinAnyGameButtonRefresh();
	
}

void gameLobbyDialogImpl::refreshPlayerStats() {

	ServerStats stats = mySession->getClientStats();
	label_nickListCounter->setText("| "+tr("players in chat: %1").arg(treeWidget_NickList->topLevelItemCount()));
	label_connectedPlayersCounter->setText(tr("connected players: %1").arg(stats.numberOfPlayersOnServer));
}

void gameLobbyDialogImpl::gameAddPlayer(unsigned gameId, unsigned playerId)
{
	if (!inGame)
	{
		QItemSelectionModel *selection = treeView_GameList->selectionModel();
		if (selection->hasSelection()) {
			if(selection->selectedRows().at(0).data(Qt::UserRole).toUInt() == gameId) {
				assert(mySession);
				GameInfo info(mySession->getClientGameInfo(gameId));
				PlayerRights tmpRights = info.adminPlayerId == playerId ? PLAYER_RIGHTS_ADMIN : PLAYER_RIGHTS_NORMAL;
				PlayerInfo playerInfo(mySession->getClientPlayerInfo(playerId));
	
				addConnectedPlayer(playerId, QString::fromUtf8(playerInfo.playerName.c_str()), tmpRights);
			}
		}
	}

	int it = 0;
	while (myGameListModel->item(it)) {
		if (myGameListModel->item(it, 0)->data(Qt::UserRole) == gameId) {
			QList <QStandardItem*> itemList;
			itemList << myGameListModel->item(it, 0) << myGameListModel->item(it, 1) << myGameListModel->item(it, 2) << myGameListModel->item(it, 3);
			
			updateGameItem(itemList, gameId);
			break;
		}
		it++;
	}
}

void gameLobbyDialogImpl::gameRemovePlayer(unsigned gameId, unsigned playerId)
{
	if (!inGame)
	{
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
			itemList << myGameListModel->item(it, 0) << myGameListModel->item(it, 1) << myGameListModel->item(it, 2) << myGameListModel->item(it, 3);
			
			updateGameItem(itemList, gameId);
			break;
		}
		++it;
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

	hideShowGameDescription(FALSE);		
	label_SmallBlind->setText("");
	label_StartCash->setText("");
	label_MaximumNumberOfPlayers->setText("");
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
	headerList << tr("Game") << tr("Players") << tr("State") << tr("Private"); 
	myGameListModel->setHorizontalHeaderLabels(headerList);
	treeView_GameList->setColumnWidth(0,185);
	treeView_GameList->setColumnWidth(1,65);
	treeView_GameList->setColumnWidth(2,65);
	treeView_GameList->setColumnWidth(3,65);

	pushButton_CreateGame->clearFocus();
	lineEdit_ChatInput->setFocus();
	myChat->clearChat();
	inGame = false;
	isAdmin = false;
	myPlayerId = 0;

	hideShowGameDescription(FALSE);

	label_nickListCounter->setText("| "+tr("players in chat: %1").arg(0));
	label_connectedPlayersCounter->setText(tr("connected players: %1").arg(0));	
	label_openGamesCounter->setText("| "+tr("running games: %1").arg(0));
	label_runningGamesCounter->setText("| "+tr("open games: %1").arg(0));

	readDialogSettings();
}

void gameLobbyDialogImpl::checkPlayerQuantity() {

	if(isAdmin){
		pushButton_Kick->show();
		pushButton_StartGame->show();
		checkBox_fillUpWithComputerOpponents->show();
		
		if (treeWidget_connectedPlayers->topLevelItemCount() >= 2) {
			pushButton_StartGame->setEnabled(true);
		}
		else {
			pushButton_StartGame->setEnabled(false);
		}
		
		if(treeWidget_connectedPlayers->topLevelItemCount() == label_MaximumNumberOfPlayers->text().toInt()) { 
			blinkingButtonAnimationTimer->start(); 
		}
		else {
			blinkingButtonAnimationTimer->stop();
			blinkingButtonAnimationState = false;
			blinkingStartButtonAnimation();
		}
	}
}

void gameLobbyDialogImpl::blinkingStartButtonAnimation() {
	
	if(blinkingButtonAnimationState) {
		QPalette p = pushButton_StartGame->palette();
		p.setColor(QPalette::Button, QColor(Qt::red));
		p.setColor(QPalette::ButtonText, QColor(Qt::white));
		pushButton_StartGame->setPalette(p);
		blinkingButtonAnimationState = false;	
	}
	else {
		QPalette p = pushButton_StartGame->palette();
		p.setColor(QPalette::Button, defaultStartButtonColor);
		p.setColor(QPalette::ButtonText, defaultStartButtonTextColor);
		pushButton_StartGame->setPalette(p);
		blinkingButtonAnimationState = true;
	}
}

void gameLobbyDialogImpl::joinedNetworkGame(unsigned playerId, QString playerName, int rights) {

	// Update dialog
	inGame = true;
	joinedGameDialogUpdate();

	myPlayerId = playerId;
	isAdmin = rights == PLAYER_RIGHTS_ADMIN;
	addConnectedPlayer(playerId, playerName, rights);
}


void gameLobbyDialogImpl::addConnectedPlayer(unsigned playerId, QString playerName, int rights) {

	QTreeWidgetItem *item = new QTreeWidgetItem(treeWidget_connectedPlayers, 0);
	item->setData(0, Qt::UserRole, playerId);
	item->setData(0, Qt::DisplayRole, playerName);
	
	if(rights == PLAYER_RIGHTS_ADMIN) item->setBackground(0, QBrush(QColor(0, 255, 0, 127)));

	if(this->isVisible() && inGame && myConfig->readConfigInt("PlayNetworkGameNotification")) {
		if(treeWidget_connectedPlayers->topLevelItemCount() < label_MaximumNumberOfPlayers->text().toInt()) {
			myW->getMySDLPlayer()->playSound("playerconnected", 0);
		}
		else {
			myW->getMySDLPlayer()->playSound("onlinegameready", 0);
		}
	}

	checkPlayerQuantity();

	if (inGame)
		refreshConnectedPlayerAvatars();
}

void gameLobbyDialogImpl::updatePlayer(unsigned playerId, QString newPlayerName) {

	QTreeWidgetItemIterator it(treeWidget_connectedPlayers);
	while (*it) {
		if ((*it)->data(0, Qt::UserRole) == playerId)
		{
			(*it)->setData(0, Qt::DisplayRole, newPlayerName);
			break;
		}
		++it;
	}

	if (inGame)
		refreshConnectedPlayerAvatars();
}

void gameLobbyDialogImpl::removePlayer(unsigned playerId, QString) {

	QTreeWidgetItemIterator it(treeWidget_connectedPlayers);
	while (*it) {
		if ((*it)->data(0, Qt::UserRole) == playerId)
		{
			treeWidget_connectedPlayers->takeTopLevelItem(treeWidget_connectedPlayers->indexOfTopLevelItem(*it));
			break;
		}
		++it;
	}

	checkPlayerQuantity();
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

	if (inGame && myPlayerId == playerId)
	{
		isAdmin = true;
		checkPlayerQuantity();
	}
}

void gameLobbyDialogImpl::refreshConnectedPlayerAvatars() {

	QTreeWidgetItemIterator it(treeWidget_connectedPlayers);
	while (*it) {

		std::string myAvatarFileName;
		PlayerInfo playerInfo(mySession->getClientPlayerInfo((*it)->data(0, Qt::UserRole).toUInt()));
		
		if(mySession->getAvatarFile(playerInfo.avatar, myAvatarFileName)) {

			QString myAvatarString(QString::fromUtf8(myAvatarFileName.c_str()));
			if(QFile::QFile(myAvatarString).exists()) {
	
				QPixmap myAvatarPixmap(25,26);
				myAvatarPixmap.fill(Qt::transparent);
				QPixmap tempPixmap(myAvatarString);
				QPainter p(&myAvatarPixmap);
				p.drawPixmap (0,0,25,25,tempPixmap);
				(*it)->setIcon(0, QIcon(myAvatarPixmap));
			}
		}		
		++it;
	}
}

void gameLobbyDialogImpl::joinedGameDialogUpdate() {
	groupBox_GameInfo->setEnabled(true);
	groupBox_GameInfo->setTitle(currentGameName);
	treeWidget_connectedPlayers->clear();
	pushButton_CreateGame->hide();
	pushButton_JoinGame->hide();
	pushButton_joinAnyGame->hide();
	pushButton_Leave->show();
}

void gameLobbyDialogImpl::leftGameDialogUpdate() {

	// un-select current game.
	treeView_GameList->clearSelection();

	groupBox_GameInfo->setTitle(tr("Game Info"));
	groupBox_GameInfo->setEnabled(false);
	currentGameName = "";

	hideShowGameDescription(FALSE);		
	label_SmallBlind->setText("");
	label_StartCash->setText("");
	label_MaximumNumberOfPlayers->setText("");
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

void gameLobbyDialogImpl::updateDialogBlinds(const GameData &gameData) {

	if(gameData.raiseIntervalMode == RAISE_ON_HANDNUMBER) { label_blindsRaiseIntervall->setText(QString::number(gameData.raiseSmallBlindEveryHandsValue)+" "+tr("hands")); }
	else { label_blindsRaiseIntervall->setText(QString::number(gameData.raiseSmallBlindEveryMinutesValue)+" "+tr("minutes")); }
	if(gameData.raiseMode == DOUBLE_BLINDS) { 
		label_blindsRaiseMode->setText(tr("double blinds")); 
		label_blindsList->hide();
		label_gameDesc6->hide();
	}
	else { 
		label_blindsRaiseMode->setText(tr("manual blinds order"));
		label_blindsList->show();
		label_gameDesc6->show();
		
		QString blindsListString;
		std::list<int>::const_iterator it1;
		for(it1= gameData.manualBlindsList.begin(); it1 != gameData.manualBlindsList.end(); it1++) {
			blindsListString.append(QString("%L1").arg(*it1)).append(", "); 
		}
		blindsListString.remove(blindsListString.length()-2,2);
		label_blindsList->setText(blindsListString);
	}
}

void gameLobbyDialogImpl::playerSelected(QTreeWidgetItem* item, QTreeWidgetItem*) {

	if (item)
		pushButton_Kick->setEnabled(isAdmin);
}

void gameLobbyDialogImpl::startGame() {

	assert(mySession);
	mySession->sendStartEvent(checkBox_fillUpWithComputerOpponents->isChecked());
	waitStartGameMsgBoxTimer->start(1000);
}

void gameLobbyDialogImpl::leaveGame() {

	assert(mySession);
	mySession->sendLeaveCurrentGame();
}

void gameLobbyDialogImpl::kickPlayer() {

	QTreeWidgetItem *item = treeWidget_connectedPlayers->currentItem();
	if (item)
	{
		QString playerName = item->text(0);
		if(playerName == QString::fromUtf8(myConfig->readConfigString("MyName").c_str())) {
			{ QMessageBox::warning(this, tr("Server Error"),
					tr("You should not kick yourself from this game!"),
					QMessageBox::Close); }
		}
		else {
			assert(mySession);
			mySession->kickPlayer(item->data(0, Qt::UserRole).toUInt());
		}
	}
	pushButton_Kick->setEnabled(false);
}

void gameLobbyDialogImpl::keyPressEvent ( QKeyEvent * event ) {

// 	std::cout << "key" << "\n";

	if (event->key() == Qt::Key_Enter && lineEdit_ChatInput->hasFocus()) { myChat->sendMessage(); }

	if (event->key() == Qt::Key_Up && lineEdit_ChatInput->hasFocus()) { 
		if((keyUpCounter + 1) <= myChat->getChatLinesHistorySize()) { keyUpCounter++; }
// 		std::cout << "Up keyUpCounter: " << keyUpCounter << "\n";
		myChat->showChatHistoryIndex(keyUpCounter); 
	}
	else if(event->key() == Qt::Key_Down && lineEdit_ChatInput->hasFocus()) { 
		if((keyUpCounter - 1) >= 0) { keyUpCounter--; }
// 		std::cout << "Down keyUpCounter: " << keyUpCounter << "\n";
		myChat->showChatHistoryIndex(keyUpCounter); 
	}
	else { keyUpCounter = 0; }

// 	if (event->key() == Qt::Key_Tab) event->ignore(); else { /*blah*/ }

	
}

bool gameLobbyDialogImpl::eventFilter(QObject *obj, QEvent *event)
{
	QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

	if (obj == lineEdit_ChatInput && lineEdit_ChatInput->text() != "" && event->type() == QEvent::KeyPress && keyEvent->key() == Qt::Key_Tab) 
	{
		myChat->nickAutoCompletition();
		return true;
	} else {
		// pass the event on to the parent class
		return QDialog::eventFilter(obj, event);
	}
}

bool gameLobbyDialogImpl::event ( QEvent * event ) { 

	return QDialog::event(event);
}

void gameLobbyDialogImpl::hideShowGameDescription(bool show) {

	if(show) {
		label_gameDesc1->show();
		label_gameDesc2->show();
		label_gameDesc3->show();
		label_gameDesc4->show();
		label_gameDesc5->show();
		label_gameDesc6->show();
		label_gameDesc7->show();
	}
	else {
		label_gameDesc1->hide();
		label_gameDesc2->hide();
		label_gameDesc3->hide();
		label_gameDesc4->hide();
		label_gameDesc5->hide();
		label_gameDesc6->hide();
		label_gameDesc7->hide();
	}
}

void gameLobbyDialogImpl::showWaitStartGameMsgBox() { 
	waitStartGameMsgBox->show();
	waitStartGameMsgBox->raise();
	waitStartGameMsgBox->activateWindow();
}

void gameLobbyDialogImpl::joinAnyGameButtonRefresh() {

	int openNonPrivateNonFullGamesCounter = 0;

	int it = 0;
	while (myGameListModel->item(it)) {

		int players = myGameListModel->item(it, 1)->data(Qt::DisplayRole).toString().section("/",0,0).toInt();
		int maxPlayers = myGameListModel->item(it, 1)->data(Qt::DisplayRole).toString().section("/",1,1).toInt();
		if (myGameListModel->item(it, 2)->data(16) == "open" && myGameListModel->item(it, 3)->data(16) == "nonpriv" && players < maxPlayers) { 
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
}


void gameLobbyDialogImpl::writeDialogSettings(int saveMode)
{
	switch(saveMode) {
		case 0: {
				myConfig->writeConfigInt("DlgGameLobbyGameListSortingSection", myGameListSortFilterProxyModel->sortColumn());
				myConfig->writeConfigInt("DlgGameLobbyGameListSortingOrder", myGameListSortFilterProxyModel->sortOrder());
			}
		break;
		case 1: {
				myConfig->writeConfigInt("DlgGameLobbyGameListFilterIndex", comboBox_gameListFilter->currentIndex());		
			}
		break;
		default:;
	}
	myConfig->writeBuffer();
}

void gameLobbyDialogImpl::readDialogSettings()
{
	comboBox_gameListFilter->setCurrentIndex(myConfig->readConfigInt("DlgGameLobbyGameListFilterIndex"));
	treeView_GameList->sortByColumn(myConfig->readConfigInt("DlgGameLobbyGameListSortingSection"), (Qt::SortOrder)myConfig->readConfigInt("DlgGameLobbyGameListSortingOrder") );
}

void gameLobbyDialogImpl::changeGameListFilter(int index) {

	switch(index) {
		case 0: {
				myGameListSortFilterProxyModel->setColumn1RegExp(QRegExp());
				myGameListSortFilterProxyModel->setColumn2RegExp(QRegExp());
				myGameListSortFilterProxyModel->setColumn3RegExp(QRegExp());
		}
		break;
		case 1: {
				myGameListSortFilterProxyModel->setColumn1RegExp(QRegExp());
				myGameListSortFilterProxyModel->setColumn2RegExp(QRegExp("open", Qt::CaseInsensitive, QRegExp::FixedString));
				myGameListSortFilterProxyModel->setColumn3RegExp(QRegExp());
		}
		break;
		case 2: {
				myGameListSortFilterProxyModel->setColumn1RegExp(QRegExp("nonfull", Qt::CaseInsensitive, QRegExp::FixedString));
				myGameListSortFilterProxyModel->setColumn2RegExp(QRegExp("open", Qt::CaseInsensitive, QRegExp::FixedString));
				myGameListSortFilterProxyModel->setColumn3RegExp(QRegExp());
		}
		break;
		case 3: {
				myGameListSortFilterProxyModel->setColumn1RegExp(QRegExp("nonfull", Qt::CaseInsensitive, QRegExp::FixedString));
				myGameListSortFilterProxyModel->setColumn2RegExp(QRegExp("open", Qt::CaseInsensitive, QRegExp::FixedString));
				myGameListSortFilterProxyModel->setColumn3RegExp(QRegExp("nonpriv", Qt::CaseInsensitive, QRegExp::FixedString));
		}
		break;
		case 4: {
				myGameListSortFilterProxyModel->setColumn1RegExp(QRegExp("nonfull", Qt::CaseInsensitive, QRegExp::FixedString));
				myGameListSortFilterProxyModel->setColumn2RegExp(QRegExp("open", Qt::CaseInsensitive, QRegExp::FixedString));
				myGameListSortFilterProxyModel->setColumn3RegExp(QRegExp("private", Qt::CaseInsensitive, QRegExp::FixedString));
		}
		break;
//		case 5: {
//				myGameListSortFilterProxyModel->setColumn1RegExp(QRegExp());
//				myGameListSortFilterProxyModel->setColumn2RegExp(QRegExp("open", Qt::CaseInsensitive, QRegExp::FixedString));
//				myGameListSortFilterProxyModel->setColumn3RegExp(QRegExp("nonpriv", Qt::CaseInsensitive, QRegExp::FixedString));
//		}
//		break;
//		case 6: {
//				myGameListSortFilterProxyModel->setColumn1RegExp(QRegExp());
//				myGameListSortFilterProxyModel->setColumn2RegExp(QRegExp("open", Qt::CaseInsensitive, QRegExp::FixedString));
//				myGameListSortFilterProxyModel->setColumn3RegExp(QRegExp("private", Qt::CaseInsensitive, QRegExp::FixedString));
//		}
//		break;
//		case 7: {
//				myGameListSortFilterProxyModel->setColumn1RegExp(QRegExp("nonfull", Qt::CaseInsensitive, QRegExp::FixedString));
//				myGameListSortFilterProxyModel->setColumn2RegExp(QRegExp());
//				myGameListSortFilterProxyModel->setColumn3RegExp(QRegExp());
//		}
//		break;
//		case 8: {
//				myGameListSortFilterProxyModel->setColumn1RegExp(QRegExp("nonfull", Qt::CaseInsensitive, QRegExp::FixedString));
//				myGameListSortFilterProxyModel->setColumn2RegExp(QRegExp());
//				myGameListSortFilterProxyModel->setColumn3RegExp(QRegExp("nonpriv", Qt::CaseInsensitive, QRegExp::FixedString));
//		}
//		break;
//		case 9: {
//				myGameListSortFilterProxyModel->setColumn1RegExp(QRegExp("nonfull", Qt::CaseInsensitive, QRegExp::FixedString));
//				myGameListSortFilterProxyModel->setColumn2RegExp(QRegExp());
//				myGameListSortFilterProxyModel->setColumn3RegExp(QRegExp("private", Qt::CaseInsensitive, QRegExp::FixedString));
//		}
//		break;
//		case 10: {
//				myGameListSortFilterProxyModel->setColumn1RegExp(QRegExp());
//				myGameListSortFilterProxyModel->setColumn2RegExp(QRegExp());
//				myGameListSortFilterProxyModel->setColumn3RegExp(QRegExp("nonpriv", Qt::CaseInsensitive, QRegExp::FixedString));
//		}
//		break;
//		case 11: {
//				myGameListSortFilterProxyModel->setColumn1RegExp(QRegExp());
//				myGameListSortFilterProxyModel->setColumn2RegExp(QRegExp());
//				myGameListSortFilterProxyModel->setColumn3RegExp(QRegExp("private", Qt::CaseInsensitive, QRegExp::FixedString));
//		}
//		break;
		default:;
	}
	myGameListSortFilterProxyModel->setFilterRegExp(QString());
	myGameListSortFilterProxyModel->setFilterKeyColumn(0);
			
	writeDialogSettings(1);
	
	if(index) treeView_GameList->setStyleSheet("QTreeView { border-radius: 4px; border: 2px solid blue; background-color: white; background-image: url(\""+myAppDataPath +"gfx/gui/misc/background_gamelist.png\"); background-attachment: fixed; background-position: top center ; background-repeat: no-repeat;}");
	else treeView_GameList->setStyleSheet("QTreeView { background-color: white; background-image: url(\""+myAppDataPath +"gfx/gui/misc/background_gamelist.png\"); background-attachment: fixed; background-position: top center ; background-repeat: no-repeat;}");

}

void gameLobbyDialogImpl::changeGameListSorting() {
	
	writeDialogSettings(0);
}

//<item>
//      <property name="text">
//       <string>5 - Show open &amp; non-private games</string>
//      </property>
//     </item>
//     <item>
//      <property name="text">
//       <string>6 - Show open &amp; private games</string>
//      </property>
//     </item>
//     <item>
//      <property name="text">
//       <string>7 - Show non-full games</string>
//      </property>
//     </item>
//     <item>
//      <property name="text">
//       <string>8 - Show non-full &amp; non-private games</string>
//      </property>
//     </item>
//     <item>
//      <property name="text">
//       <string>9 - Show non-full &amp; private games</string>
//      </property>
//     </item>
//     <item>
//      <property name="text">
//       <string>10 - Show non-private games</string>
//      </property>
//     </item>
//     <item>
//      <property name="text">
//       <string>11 - Show private games</string>
//      </property>
//     </item>
