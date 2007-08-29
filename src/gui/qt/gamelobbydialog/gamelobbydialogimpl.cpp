//
// C++ Implementation: gamelobbydialogimpl
//
// Description: 
//
//
// Author: FThauer FHammer <webmaster@pokerth.net>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "gamelobbydialogimpl.h"
#include "session.h"
#include "configfile.h"
#include "gamedata.h"

gameLobbyDialogImpl::gameLobbyDialogImpl(QWidget *parent, ConfigFile *c)
 : QDialog(parent), myConfig(c), currentGameName("")
{
    setupUi(this);

	connect( pushButton_CreateGame, SIGNAL( clicked() ), this, SLOT( createGame() ) );
	connect( pushButton_JoinGame, SIGNAL( clicked() ), this, SLOT( joinGame() ) );
	connect( treeWidget_GameList, SIGNAL( itemClicked ( QTreeWidgetItem*, int) ), this, SLOT( gameSelected(QTreeWidgetItem*, int) ) );
	connect( treeWidget_GameList, SIGNAL( clear () ), this, SLOT( clearGames() ) );

	pushButton_JoinGame->setEnabled(false);
	
	pushButton_Leave->hide();
	pushButton_Kick->hide();
	pushButton_StartGame->hide();
	
	
	treeWidget_GameList->setColumnWidth(0,250);
	treeWidget_GameList->setColumnWidth(1,75);
	treeWidget_GameList->setColumnWidth(2,70);

	lineEdit_ChatInput->setFocus();
	
}

void gameLobbyDialogImpl::exec()
{

	QDialog::exec();
}


gameLobbyDialogImpl::~gameLobbyDialogImpl()
{
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
		gameData.smallBlind = myCreateInternetGameDialog->spinBox_smallBlind->value();
		gameData.handsBeforeRaise = myCreateInternetGameDialog->spinBox_handsBeforeRaiseSmallBlind->value();
		gameData.guiSpeed = myCreateInternetGameDialog->spinBox_gameSpeed->value();
		gameData.playerActionTimeoutSec = myCreateInternetGameDialog->spinBox_netTimeOutPlayerAction->value();
		
		mySession->clientCreateGame(gameData, myConfig->readConfigString("MyName") + "'s game", "");

		currentGameName = QString::fromUtf8(myConfig->readConfigString("MyName").c_str()) + QString("'s game");

		accept();
	}
}

void gameLobbyDialogImpl::joinGame()
{
	QTreeWidgetItem *item = treeWidget_GameList->currentItem();
	if (item)
	{
		QString gameName = item->text(0);

		assert(mySession);
		mySession->clientJoinGame(gameName.toUtf8().constData(), "");

		accept();
	}
}

void gameLobbyDialogImpl::gameSelected(QTreeWidgetItem* item, int)
{
	pushButton_JoinGame->setEnabled(true);

	currentGameName = item->text(0);

	groupBox_GameInfo->setEnabled(true);
	groupBox_GameInfo->setTitle(tr("Game Info") + " - " + currentGameName);

	assert(mySession);
	GameInfo info = mySession->getClientGameInfo(currentGameName.toUtf8().constData());
	label_SmallBlind->setText(QString::number(info.data.smallBlind));
	label_StartCash->setText(QString::number(info.data.startMoney));
	label_MaximumNumberOfPlayers->setText(QString::number(info.data.maxNumberOfPlayers));
	label_HandsToRaiseSmallBlind->setText(QString::number(info.data.handsBeforeRaise));
	label_TimeoutForPlayerAction->setText(QString::number(info.data.playerActionTimeoutSec));
}

void gameLobbyDialogImpl::addGame(QString gameName)
{
	QTreeWidgetItem *item = new QTreeWidgetItem(treeWidget_GameList,0);
	item->setData(0, 0, gameName);
}

void gameLobbyDialogImpl::removeGame(QString gameName)
{
	QList<QTreeWidgetItem *> list = treeWidget_GameList->findItems(gameName, Qt::MatchExactly, 0);
	if(!list.empty()) { 
		treeWidget_GameList->takeTopLevelItem(treeWidget_GameList->indexOfTopLevelItem(list[0]));
	}
}

void gameLobbyDialogImpl::clearGames()
{
	pushButton_JoinGame->setEnabled(false);
	groupBox_GameInfo->setTitle(tr("Game Info"));
	groupBox_GameInfo->setEnabled(false);
	currentGameName = "";

	label_SmallBlind->setText("");
	label_StartCash->setText("");
	label_MaximumNumberOfPlayers->setText("");
	label_HandsToRaiseSmallBlind->setText("");
	label_TimeoutForPlayerAction->setText("");
}

