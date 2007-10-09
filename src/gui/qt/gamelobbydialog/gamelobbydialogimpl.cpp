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
#include "lobbychat.h"
#include "changecompleteblindsdialogimpl.h"
#include "session.h"
#include "configfile.h"
#include "gamedata.h"
#include <net/socket_msg.h>

gameLobbyDialogImpl::gameLobbyDialogImpl(QWidget *parent, ConfigFile *c)
 : QDialog(parent), myW(NULL), myConfig(c), mySession(NULL), currentGameName(""), myPlayerId(0), isAdmin(false), inGame(false), myChat(NULL), keyUpCounter(0)
{
    setupUi(this);
	
	myChat = new LobbyChat(this, myConfig);

	myAppDataPath = QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str());

	connect( pushButton_CreateGame, SIGNAL( clicked() ), this, SLOT( createGame() ) );
	connect( pushButton_JoinGame, SIGNAL( clicked() ), this, SLOT( joinGame() ) );
	connect( treeWidget_GameList, SIGNAL( currentItemChanged ( QTreeWidgetItem*, QTreeWidgetItem*) ), this, SLOT( gameSelected(QTreeWidgetItem*, QTreeWidgetItem*) ) );
	connect( pushButton_StartGame, SIGNAL( clicked() ), this, SLOT( startGame() ) );
	connect( pushButton_Kick, SIGNAL( clicked() ), this, SLOT( kickPlayer() ) );
	connect( pushButton_Leave, SIGNAL( clicked() ), this, SLOT( leaveGame() ) );
	connect( treeWidget_connectedPlayers, SIGNAL( currentItemChanged ( QTreeWidgetItem*, QTreeWidgetItem*) ), this, SLOT( playerSelected(QTreeWidgetItem*, QTreeWidgetItem*) ) );
	connect( lineEdit_ChatInput, SIGNAL( returnPressed () ), this, SLOT( sendChatMessage() ) );
	connect( lineEdit_ChatInput, SIGNAL( textChanged (QString) ), this, SLOT( checkChatInputLength(QString) ) );

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

	QDialog::exec();
}


gameLobbyDialogImpl::~gameLobbyDialogImpl()
{
	delete myChat;
	myChat = NULL;

}

void gameLobbyDialogImpl::setSession(Session *session)
{
	mySession = session;
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
		gameData.smallBlind = myCreateInternetGameDialog->getChangeCompleteBlindsDialog()->spinBox_firstSmallBlind->value();//TODO remove
		gameData.firstSmallBlind = myCreateInternetGameDialog->getChangeCompleteBlindsDialog()->spinBox_firstSmallBlind->value();
		
		if(myCreateInternetGameDialog->getChangeCompleteBlindsDialog()->radioButton_raiseBlindsAtHands->isChecked()) { 
			gameData.raiseIntervalMode = RAISE_ON_HANDNUMBER;
			gameData.raiseSmallBlindEveryHandsValue = myCreateInternetGameDialog->getChangeCompleteBlindsDialog()->spinBox_raiseSmallBlindEveryHands->value();
			gameData.handsBeforeRaise = myCreateInternetGameDialog->getChangeCompleteBlindsDialog()->spinBox_raiseSmallBlindEveryHands->value(); //TODO remove
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

		QString gameString(tr("game"));
		currentGameName = QString::fromUtf8(myConfig->readConfigString("MyName").c_str()) + QString("'s "+ gameString);

		hideShowGameDescription(TRUE);

		label_SmallBlind->setText(QString::number(gameData.smallBlind));
		label_StartCash->setText(QString::number(gameData.startMoney));
		label_MaximumNumberOfPlayers->setText(QString::number(gameData.maxNumberOfPlayers));

		updateDialogBlinds(gameData);

		label_TimeoutForPlayerAction->setText(QString::number(gameData.playerActionTimeoutSec));

		mySession->clientCreateGame(gameData, myConfig->readConfigString("MyName") + "'s "+ gameString.toUtf8().constData(), myCreateInternetGameDialog->lineEdit_Password->text().toUtf8().constData());
	}
}

void gameLobbyDialogImpl::joinGame()
{
	assert(mySession);
	QTreeWidgetItem *item = treeWidget_GameList->currentItem();
	if (item)
	{
		unsigned gameId = item->data(0, Qt::UserRole).toUInt();
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

void gameLobbyDialogImpl::refresh(int actionID) {

	if (actionID == MSG_NET_GAME_CLIENT_START)
	{
		QTimer::singleShot(500, this, SLOT(accept()));
	}
}

void gameLobbyDialogImpl::removedFromGame(int reason)
{
	inGame = false;
	isAdmin = false;
	leftGameDialogUpdate();
}

void gameLobbyDialogImpl::gameSelected(QTreeWidgetItem* item, QTreeWidgetItem*)
{
	if (!inGame && item)
	{
		pushButton_JoinGame->setEnabled(true);

		currentGameName = item->text(0);

		groupBox_GameInfo->setEnabled(true);
		groupBox_GameInfo->setTitle(tr("Game Info") + " - " + currentGameName);

		assert(mySession);
		GameInfo info(mySession->getClientGameInfo(item->data(0, Qt::UserRole).toUInt()));

		hideShowGameDescription(TRUE);		
		label_SmallBlind->setText(QString::number(info.data.smallBlind));
		label_StartCash->setText(QString::number(info.data.startMoney));
		label_MaximumNumberOfPlayers->setText(QString::number(info.data.maxNumberOfPlayers));

		updateDialogBlinds(info.data);

		label_TimeoutForPlayerAction->setText(QString::number(info.data.playerActionTimeoutSec));

		treeWidget_connectedPlayers->clear();
		PlayerIdList::const_iterator i = info.players.begin();
		PlayerIdList::const_iterator end = info.players.end();
		while (i != end)
		{
			PlayerInfo info(mySession->getClientPlayerInfo(*i));
			addConnectedPlayer(*i, QString::fromUtf8(info.playerName.c_str()), PLAYER_RIGHTS_NORMAL);
			++i;
		}
	}
}

void gameLobbyDialogImpl::updateGameItem(QTreeWidgetItem *item, unsigned gameId)
{
	assert(mySession);
	GameInfo info(mySession->getClientGameInfo(gameId));

	item->setData(0, Qt::UserRole, gameId);
	item->setData(0, Qt::DisplayRole, QString::fromUtf8(info.name.c_str()));

	QString playerStr;
	playerStr.sprintf("%u/%u", info.players.size(), info.data.maxNumberOfPlayers);
	item->setData(1, Qt::DisplayRole, playerStr);

	if (info.mode == GAME_MODE_STARTED)
		item->setData(2, Qt::DisplayRole, tr("running"));
	else 
		item->setData(2, Qt::DisplayRole, tr("open"));

	if (info.isPasswordProtected)
		item->setIcon(3, QIcon(myAppDataPath+"gfx/gui/misc/lock.png"));
}

void gameLobbyDialogImpl::addGame(unsigned gameId)
{
	QTreeWidgetItem *item = new QTreeWidgetItem(treeWidget_GameList, 0);

	updateGameItem(item, gameId);
}

void gameLobbyDialogImpl::updateGameMode(unsigned gameId, int /*newMode*/)
{
	QTreeWidgetItemIterator it(treeWidget_GameList);
	while (*it) {
		if ((*it)->data(0, Qt::UserRole) == gameId)
		{
			updateGameItem(*it, gameId);
			break;
		}
		++it;
	}
}

void gameLobbyDialogImpl::removeGame(unsigned gameId)
{
	QTreeWidgetItemIterator it(treeWidget_GameList);
	while (*it) {
		if ((*it)->data(0, Qt::UserRole) == gameId)
		{
			treeWidget_GameList->takeTopLevelItem(treeWidget_GameList->indexOfTopLevelItem(*it));
			break;
		}
		++it;
	}
}

void gameLobbyDialogImpl::gameAddPlayer(unsigned gameId, unsigned playerId)
{
	if (!inGame)
	{
		QTreeWidgetItem *item = treeWidget_GameList->currentItem();
		if (item && item->data(0, Qt::UserRole) == gameId)
		{
			assert(mySession);
			PlayerInfo info(mySession->getClientPlayerInfo(playerId));
			addConnectedPlayer(playerId, QString::fromUtf8(info.playerName.c_str()), PLAYER_RIGHTS_NORMAL);
		}
	}

	QTreeWidgetItemIterator it(treeWidget_GameList);
	while (*it) {
		if ((*it)->data(0, Qt::UserRole) == gameId)
		{
			updateGameItem(*it, gameId);
			break;
		}
		++it;
	}
}

void gameLobbyDialogImpl::gameRemovePlayer(unsigned gameId, unsigned playerId)
{
	if (!inGame)
	{
		QTreeWidgetItem *item = treeWidget_GameList->currentItem();
		if (item && item->data(0, Qt::UserRole) == gameId)
		{
			assert(mySession);
			PlayerInfo info(mySession->getClientPlayerInfo(playerId));
			removePlayer(playerId, QString::fromUtf8(info.playerName.c_str()));
		}
	}

	QTreeWidgetItemIterator it(treeWidget_GameList);
	while (*it) {
		if ((*it)->data(0, Qt::UserRole) == gameId)
		{
			updateGameItem(*it, gameId);
			break;
		}
		++it;
	}
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

	treeWidget_GameList->clear();
	treeWidget_GameList->show();
	treeWidget_connectedPlayers->clear();
	
	pushButton_Leave->hide();
	pushButton_Kick->hide();
	pushButton_Kick->setEnabled(false);
	pushButton_StartGame->hide();
	checkBox_fillUpWithComputerOpponents->hide();
	pushButton_CreateGame->show();
	pushButton_JoinGame->show();
	pushButton_JoinGame->setEnabled(false);

	treeWidget_GameList->setColumnWidth(0,195);
	treeWidget_GameList->setColumnWidth(1,70);
	treeWidget_GameList->setColumnWidth(2,70);
	treeWidget_GameList->setColumnWidth(3,60);

	pushButton_CreateGame->clearFocus();
	lineEdit_ChatInput->setFocus();
	myChat->clearChat();
	inGame = false;
	isAdmin = false;
	myPlayerId = 0;

	hideShowGameDescription(FALSE);
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
	
	if(rights == PLAYER_RIGHTS_ADMIN) item->setBackground(0, QBrush(QColor(169,255,140)));

	if(this->isVisible() && inGame && myConfig->readConfigInt("PlayNetworkGameNotification")) {
		if(treeWidget_connectedPlayers->topLevelItemCount() < label_MaximumNumberOfPlayers->text().toInt()) {
			myW->getMySDLPlayer()->playSound("playerconnected", 0);
		}
		else {
			myW->getMySDLPlayer()->playSound("onlinegameready", 0);
		}
	}

	checkPlayerQuantity();
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
			(*it)->setBackground(0, QBrush(QColor(169,255,140)));
		}
		++it;
	}

	if (myPlayerId == playerId)
	{
		isAdmin = true;
		checkPlayerQuantity();
	}
}

void gameLobbyDialogImpl::joinedGameDialogUpdate() {
	groupBox_GameInfo->setEnabled(true);
	groupBox_GameInfo->setTitle(currentGameName);
	treeWidget_connectedPlayers->clear();
	pushButton_CreateGame->hide();
	pushButton_JoinGame->hide();
	pushButton_Leave->show();
}

void gameLobbyDialogImpl::leftGameDialogUpdate() {

	// un-select current game.
	treeWidget_GameList->clearSelection();

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
			blindsListString.append(QString::number(*it1,10)).append(", ");
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

void gameLobbyDialogImpl::sendChatMessage() { myChat->sendMessage(); }
void gameLobbyDialogImpl::checkChatInputLength(QString string) { myChat->checkInputLength(string); }

void gameLobbyDialogImpl::keyPressEvent ( QKeyEvent * event ) {

// 	std::cout << "key" << "\n";

	if (event->key() == Qt::Key_Enter && lineEdit_ChatInput->hasFocus()) { myChat->sendMessage(); }

	if (event->key() == Qt::Key_Up && lineEdit_ChatInput->hasFocus()) { 
		if((keyUpCounter + 1) <= myChat->getChatLinesHistorySize()) { keyUpCounter++; }
// 		std::cout << "Up keyUpCounter: " << keyUpCounter << "\n";
		myChat->showChatHistoryIndex(keyUpCounter); 
	}
	else if(event->key() == Qt::Key_Down && lineEdit_ChatInput->hasFocus()) { 
		if((keyUpCounter - 1) > 0) { keyUpCounter--; }
// 		std::cout << "Down keyUpCounter: " << keyUpCounter << "\n";
		myChat->showChatHistoryIndex(keyUpCounter); 
	}
	else { keyUpCounter = 0; }

	
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
