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
	waitStartGameMsgBox->setStandardButtons(QMessageBox::NoButton);

	waitStartGameMsgBoxTimer = new QTimer(this);
	waitStartGameMsgBoxTimer->setSingleShot(TRUE);

	treeWidget_GameList->setStyleSheet("QTreeWidget {background-color: white; background-image: url("+myAppDataPath +"gfx/gui/misc/background_gamelist.png); background-attachment: fixed; background-position: top center ; background-repeat: no-repeat;}");
	treeWidget_GameList->setAutoFillBackground(TRUE);

	connect( pushButton_CreateGame, SIGNAL( clicked() ), this, SLOT( createGame() ) );
	connect( pushButton_JoinGame, SIGNAL( clicked() ), this, SLOT( joinGame() ) );
	connect( pushButton_joinAnyGame, SIGNAL( clicked() ), this, SLOT( joinAnyGame() ) );
	connect( pushButton_StartGame, SIGNAL( clicked() ), this, SLOT( startGame() ) );
	connect( pushButton_Kick, SIGNAL( clicked() ), this, SLOT( kickPlayer() ) );
	connect( pushButton_Leave, SIGNAL( clicked() ), this, SLOT( leaveGame() ) );
	connect( treeWidget_GameList, SIGNAL( currentItemChanged ( QTreeWidgetItem*, QTreeWidgetItem*) ), this, SLOT( gameSelected(QTreeWidgetItem*, QTreeWidgetItem*) ) );
	connect( treeWidget_connectedPlayers, SIGNAL( currentItemChanged ( QTreeWidgetItem*, QTreeWidgetItem*) ), this, SLOT( playerSelected(QTreeWidgetItem*, QTreeWidgetItem*) ) );
	connect( lineEdit_ChatInput, SIGNAL( returnPressed () ), myChat, SLOT( sendMessage() ) );
	connect( lineEdit_ChatInput, SIGNAL( textChanged (QString) ), myChat, SLOT( checkInputLength(QString) ) );
	connect( lineEdit_ChatInput, SIGNAL( textEdited (QString) ), myChat, SLOT( setChatTextEdited() ) );
	connect( waitStartGameMsgBoxTimer, SIGNAL(timeout()), this, SLOT( showWaitStartGameMsgBox() ));


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

	QDialog::exec();

	waitStartGameMsgBoxTimer->stop();
	waitStartGameMsgBox->hide();

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

		label_SmallBlind->setText(QString::number(gameData.firstSmallBlind));
		label_StartCash->setText(QString::number(gameData.startMoney));
		label_MaximumNumberOfPlayers->setText(QString::number(gameData.maxNumberOfPlayers));

		updateDialogBlinds(gameData);

		label_TimeoutForPlayerAction->setText(QString::number(gameData.playerActionTimeoutSec));

		mySession->clientCreateGame(gameData, currentGameName.toUtf8().constData(), myCreateInternetGameDialog->lineEdit_Password->text().toUtf8().constData());
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

void gameLobbyDialogImpl::joinAnyGame() {

	bool found = FALSE;

	QTreeWidgetItemIterator it(treeWidget_GameList);
	while (*it) {

		bool ok;
		int players = (*it)->data(1, Qt::DisplayRole).toString().section("/",0,0).toInt(&ok,10);
		int maxPlayers = (*it)->data(1, Qt::DisplayRole).toString().section("/",1,1).toInt(&ok,10);
		if ((*it)->data(2, Qt::DisplayRole) == tr("open") && (*it)->data(3, Qt::UserRole) == 0 && players < maxPlayers)
		{
			treeWidget_GameList->setCurrentItem((*it));
			found = TRUE;
			break;
		}
		++it;
	}

	if(found) { joinGame(); }
	
}

void gameLobbyDialogImpl::refresh(int actionID) {

	if (actionID == MSG_NET_GAME_CLIENT_START)
	{
		treeWidget_GameList->clear();
		this->accept();
// 		TODO doitux: hier gametable holen
	}
}

void gameLobbyDialogImpl::removedFromGame(int /*reason*/)
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
		label_SmallBlind->setText(QString::number(info.data.firstSmallBlind));
		label_StartCash->setText(QString::number(info.data.startMoney));
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

	if (info.isPasswordProtected) {
		item->setIcon(3, QIcon(myAppDataPath+"gfx/gui/misc/lock.png"));
		item->setData(3, Qt::UserRole, 1);
	}
	else {
		item->setData(3, Qt::UserRole, 0);
	}

	refreshGameStats();
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

void gameLobbyDialogImpl::updateGameAdmin(unsigned /*gameId*/, unsigned adminPlayerId)
{
	newGameAdmin(adminPlayerId, "");
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
	
	refreshGameStats();
}

void gameLobbyDialogImpl::refreshGameStats() {

	int runningGamesCounter = 0;
	int openGamesCounter = 0;

	QTreeWidgetItemIterator it(treeWidget_GameList);
	while (*it) {
		if ((*it)->data(2, Qt::DisplayRole) ==  tr("running")) { runningGamesCounter++; }
		if ((*it)->data(2, Qt::DisplayRole) ==  tr("open")) { openGamesCounter++; }
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
		QTreeWidgetItem *item = treeWidget_GameList->currentItem();
		if (item && item->data(0, Qt::UserRole) == gameId)
		{
			assert(mySession);
			GameInfo info(mySession->getClientGameInfo(gameId));
			PlayerRights tmpRights = info.adminPlayerId == playerId ? PLAYER_RIGHTS_ADMIN : PLAYER_RIGHTS_NORMAL;
			PlayerInfo playerInfo(mySession->getClientPlayerInfo(playerId));

			addConnectedPlayer(playerId, QString::fromUtf8(playerInfo.playerName.c_str()), tmpRights);
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
	pushButton_joinAnyGame->show();
	pushButton_joinAnyGame->setEnabled(false);

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

	label_nickListCounter->setText("| "+tr("players in chat: %1").arg(0));
	label_connectedPlayersCounter->setText(tr("connected players: %1").arg(0));	
	label_openGamesCounter->setText("| "+tr("running games: %1").arg(0));
	label_runningGamesCounter->setText("| "+tr("open games: %1").arg(0));


// 	QTreeWidgetItem *item = new QTreeWidgetItem(treeWidget_GameList, 0);
// 	item->setData(0, Qt::DisplayRole, "43324434");
// 	item->setData(1, Qt::DisplayRole, "sfdsfsdfsd");
// 	item->setData(2, Qt::DisplayRole, "33223");
// 
// 	QTreeWidgetItem *item1 = new QTreeWidgetItem(treeWidget_GameList, 0);
// 	item1->setData(0, Qt::DisplayRole, "43324434");
// 	item1->setData(1, Qt::DisplayRole, "sfdsfsdfsd");
// 	item1->setData(2, Qt::DisplayRole, "33223");
// 	QTreeWidgetItem *item2 = new QTreeWidgetItem(treeWidget_GameList, 0);
// 	item2->setData(0, Qt::DisplayRole, "43324434");
// 	item2->setData(1, Qt::DisplayRole, "sfdsfsdfsd");
// 	item2->setData(2, Qt::DisplayRole, "33223");
// QTreeWidgetItem *item3 = new QTreeWidgetItem(treeWidget_GameList, 0);
// 	item3->setData(0, Qt::DisplayRole, "43324434");
// 	item3->setData(1, Qt::DisplayRole, "sfdsfsdfsd");
// 	item3->setData(2, Qt::DisplayRole, "33223");
// QTreeWidgetItem *item4 = new QTreeWidgetItem(treeWidget_GameList, 0);
// 	item4->setData(0, Qt::DisplayRole, "43324434");
// 	item4->setData(1, Qt::DisplayRole, "sfdsfsdfsd");
// 	item4->setData(2, Qt::DisplayRole, "33223");
// QTreeWidgetItem *item5 = new QTreeWidgetItem(treeWidget_GameList, 0);
// 	item5->setData(0, Qt::DisplayRole, "43324434");
// 	item5->setData(1, Qt::DisplayRole, "sfdsfsdfsd");
// 	item5->setData(2, Qt::DisplayRole, "33223");
// QTreeWidgetItem *item6 = new QTreeWidgetItem(treeWidget_GameList, 0);
// 	item6->setData(0, Qt::DisplayRole, "43324434");
// 	item6->setData(1, Qt::DisplayRole, "sfdsfsdfsd");
// 	item6->setData(2, Qt::DisplayRole, "33223");
// QTreeWidgetItem *item7 = new QTreeWidgetItem(treeWidget_GameList, 0);
// 	item7->setData(0, Qt::DisplayRole, "43324434");
// 	item7->setData(1, Qt::DisplayRole, "sfdsfsdfsd");
// 	item7->setData(2, Qt::DisplayRole, "33223");
// QTreeWidgetItem *item8 = new QTreeWidgetItem(treeWidget_GameList, 0);
// 	item8->setData(0, Qt::DisplayRole, "43324434");
// 	item8->setData(1, Qt::DisplayRole, "sfdsfsdfsd");
// 	item8->setData(2, Qt::DisplayRole, "33223");
// QTreeWidgetItem *item9 = new QTreeWidgetItem(treeWidget_GameList, 0);
// 	item9->setData(0, Qt::DisplayRole, "43324434");
// 	item9->setData(1, Qt::DisplayRole, "sfdsfsdfsd");
// 	item9->setData(2, Qt::DisplayRole, "33223");
// QTreeWidgetItem *item10 = new QTreeWidgetItem(treeWidget_GameList, 0);
// 	item10->setData(0, Qt::DisplayRole, "43324434");
// 	item10->setData(1, Qt::DisplayRole, "sfdsfsdfsd");
// 	item10->setData(2, Qt::DisplayRole, "33223");
// QTreeWidgetItem *item11 = new QTreeWidgetItem(treeWidget_GameList, 0);
// 	item11->setData(0, Qt::DisplayRole, "43324434");
// 	item11->setData(1, Qt::DisplayRole, "sfdsfsdfsd");
// 	item11->setData(2, Qt::DisplayRole, "33223");
// QTreeWidgetItem *item12 = new QTreeWidgetItem(treeWidget_GameList, 0);
// 	item12->setData(0, Qt::DisplayRole, "43324434");
// 	item12->setData(1, Qt::DisplayRole, "sfdsfsdfsd");
// 	item12->setData(2, Qt::DisplayRole, "33223");
// QTreeWidgetItem *item13 = new QTreeWidgetItem(treeWidget_GameList, 0);
// 	item13->setData(0, Qt::DisplayRole, "43324434");
// 	item13->setData(1, Qt::DisplayRole, "sfdsfsdfsd");
// 	item13->setData(2, Qt::DisplayRole, "33223");
// QTreeWidgetItem *item14 = new QTreeWidgetItem(treeWidget_GameList, 0);
// 	item14->setData(0, Qt::DisplayRole, "43324434");
// 	item14->setData(1, Qt::DisplayRole, "sfdsfsdfsd");
// 	item14->setData(2, Qt::DisplayRole, "33223");
// QTreeWidgetItem *item15 = new QTreeWidgetItem(treeWidget_GameList, 0);
// 	item15->setData(0, Qt::DisplayRole, "43324434");
// 	item15->setData(1, Qt::DisplayRole, "sfdsfsdfsd");
// 	item15->setData(2, Qt::DisplayRole, "33223");
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
			(*it)->setBackground(0, QBrush(QColor(169,255,140)));
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

	QTreeWidgetItemIterator it(treeWidget_GameList);
	while (*it) {
		bool ok;
		int players = (*it)->data(1, Qt::DisplayRole).toString().section("/",0,0).toInt(&ok,10);
		int maxPlayers = (*it)->data(1, Qt::DisplayRole).toString().section("/",1,1).toInt(&ok,10);
		if ((*it)->data(2, Qt::DisplayRole) == tr("open") && (*it)->data(3, Qt::UserRole) == 0 && players < maxPlayers) { openNonPrivateNonFullGamesCounter++; }
		++it;
	}

	if(openNonPrivateNonFullGamesCounter) pushButton_joinAnyGame->setEnabled(TRUE);
	else pushButton_joinAnyGame->setEnabled(FALSE);
}

