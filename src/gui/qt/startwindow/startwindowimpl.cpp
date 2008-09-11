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
#include "startwindowimpl.h"

#include <gamedata.h>
#include <generic/serverguiwrapper.h>
#include <net/socket_msg.h>

#include "session.h"
#include "game.h"
#include "guiwrapper.h"

#include "configfile.h"
#include "gametableimpl.h"
#include "newgamedialogimpl.h"
#include "aboutpokerthimpl.h"
#include "mymessagedialogimpl.h"
#include "settingsdialogimpl.h"
#include "selectavatardialogimpl.h"
#include "joinnetworkgamedialogimpl.h"
#include "connecttoserverdialogimpl.h"
#include "createnetworkgamedialogimpl.h"
#include "startnetworkgamedialogimpl.h"
#include "changehumanplayernamedialogimpl.h"
#include "changecompleteblindsdialogimpl.h"
#include "gamelobbydialogimpl.h"
#include "timeoutmsgboximpl.h"
#include "lobbychat.h"

startWindowImpl::startWindowImpl(ConfigFile *c)
    : myConfig(c)
{

	myGuiInterface.reset(new GuiWrapper(myConfig, this));
	{
		boost::shared_ptr<Session> session(new Session(myGuiInterface.get(), myConfig));
		session->init(); // TODO handle error
		myGuiInterface->setSession(session);
	}


// #ifdef __APPLE__
// 	setWindowModality(Qt::ApplicationModal);
// 	setWindowFlags(Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint | Qt::Dialog);
// #endif	
	setupUi(this);
	this->setWindowTitle(QString(tr("PokerTH %1 - The Open-Source Texas Holdem Engine").arg(POKERTH_BETA_RELEASE_STRING)));

// 	Dialogs
	myNewGameDialog = new newGameDialogImpl(this, myConfig);
	mySelectAvatarDialog = new selectAvatarDialogImpl(this, myConfig);
	mySettingsDialog = new settingsDialogImpl(this, myConfig, mySelectAvatarDialog);	
	myChangeHumanPlayerNameDialog = new changeHumanPlayerNameDialogImpl(this, myConfig);
	myJoinNetworkGameDialog = new joinNetworkGameDialogImpl(this, myConfig);
	myConnectToServerDialog = new connectToServerDialogImpl(this);
	myStartNetworkGameDialog = new startNetworkGameDialogImpl(this, myConfig);
	myCreateNetworkGameDialog = new createNetworkGameDialogImpl(this, myConfig);
	myAboutPokerthDialog = new aboutPokerthImpl(this, myConfig);
	myGameLobbyDialog = new gameLobbyDialogImpl(this, myConfig);

	myStartNetworkGameDialog->setMyW(myGuiInterface->getMyW());
	myGameLobbyDialog->setMyW(myGuiInterface->getMyW());
	
	myTimeoutDialog = new timeoutMsgBoxImpl(this);


	connect( actionStart_Local_Game, SIGNAL( triggered() ), this, SLOT( callNewGameDialog() ) );
	connect( pushButtonStart_Local_Game, SIGNAL( clicked() ), this, SLOT( callNewGameDialog() ) );
	connect( actionInternet_Game, SIGNAL( triggered() ), this, SLOT( callGameLobbyDialog() ) );
	connect( pushButtonInternet_Game, SIGNAL( clicked() ), this, SLOT( callGameLobbyDialog() ) );
	connect( actionCreate_Network_Game, SIGNAL( triggered() ), this, SLOT( callCreateNetworkGameDialog() ) );
	connect( pushButton_Create_Network_Game, SIGNAL( clicked() ), this, SLOT( callCreateNetworkGameDialog() ) );
	connect( actionJoin_Network_Game, SIGNAL( triggered() ), this, SLOT( callJoinNetworkGameDialog() ) );
	connect( pushButton_Join_Network_Game, SIGNAL( clicked() ), this, SLOT( callJoinNetworkGameDialog() ) );
	
	connect( actionAbout_PokerTH, SIGNAL( triggered() ), this, SLOT( callAboutPokerthDialog() ) );
	connect( actionConfigure_PokerTH, SIGNAL( triggered() ), this, SLOT( callSettingsDialog() ) );


	connect(this, SIGNAL(signalShowClientDialog()), this, SLOT(showClientDialog()));

	connect(this, SIGNAL(signalNetClientConnect(int)), myConnectToServerDialog, SLOT(refresh(int)));
	connect(this, SIGNAL(signalNetClientGameInfo(int)), myStartNetworkGameDialog, SLOT(refresh(int)));
	connect(this, SIGNAL(signalNetClientGameInfo(int)), myGameLobbyDialog, SLOT(refresh(int)));

	connect(this, SIGNAL(signalNetClientSelfJoined(unsigned, QString, int)), myStartNetworkGameDialog, SLOT(joinedNetworkGame(unsigned, QString, int)));
	connect(this, SIGNAL(signalNetClientPlayerJoined(unsigned, QString, int)), myStartNetworkGameDialog, SLOT(addConnectedPlayer(unsigned, QString, int)));
	connect(this, SIGNAL(signalNetClientPlayerChanged(unsigned, QString)), myStartNetworkGameDialog, SLOT(updatePlayer(unsigned, QString)));
	connect(this, SIGNAL(signalNetClientPlayerLeft(unsigned, QString)), myStartNetworkGameDialog, SLOT(removePlayer(unsigned, QString)));
	connect(this, SIGNAL(signalNetClientNewGameAdmin(unsigned, QString)), myStartNetworkGameDialog, SLOT(newGameAdmin(unsigned, QString)));
	connect(this, SIGNAL(signalNetClientGameListNew(unsigned)), myStartNetworkGameDialog, SLOT(gameCreated(unsigned)));

	connect(this, SIGNAL(signalNetClientSelfJoined(unsigned, QString, int)), myGameLobbyDialog, SLOT(joinedNetworkGame(unsigned, QString, int)));
	connect(this, SIGNAL(signalNetClientPlayerJoined(unsigned, QString, int)), myGameLobbyDialog, SLOT(addConnectedPlayer(unsigned, QString, int)));
	connect(this, SIGNAL(signalNetClientPlayerChanged(unsigned, QString)), myGameLobbyDialog, SLOT(updatePlayer(unsigned, QString)));
	connect(this, SIGNAL(signalNetClientPlayerLeft(unsigned, QString)), myGameLobbyDialog, SLOT(removePlayer(unsigned, QString)));
	connect(this, SIGNAL(signalNetClientNewGameAdmin(unsigned, QString)), myGameLobbyDialog, SLOT(newGameAdmin(unsigned, QString)));

	connect(this, SIGNAL(signalNetClientGameListNew(unsigned)), myGameLobbyDialog, SLOT(addGame(unsigned)));
	connect(this, SIGNAL(signalNetClientGameListRemove(unsigned)), myGameLobbyDialog, SLOT(removeGame(unsigned)));
	connect(this, SIGNAL(signalNetClientGameListUpdateMode(unsigned, int)), myGameLobbyDialog, SLOT(updateGameMode(unsigned, int)));
	connect(this, SIGNAL(signalNetClientGameListUpdateAdmin(unsigned, unsigned)), myGameLobbyDialog, SLOT(updateGameAdmin(unsigned, unsigned)));
	connect(this, SIGNAL(signalNetClientGameListPlayerJoined(unsigned, unsigned)), myGameLobbyDialog, SLOT(gameAddPlayer(unsigned, unsigned)));
	connect(this, SIGNAL(signalNetClientGameListPlayerLeft(unsigned, unsigned)), myGameLobbyDialog, SLOT(gameRemovePlayer(unsigned, unsigned)));
	connect(this, SIGNAL(signalNetClientRemovedFromGame(int)), myGameLobbyDialog, SLOT(removedFromGame(int)));
	connect(this, SIGNAL(signalNetClientStatsUpdate(ServerStats)), myGameLobbyDialog, SLOT(updateStats(ServerStats)));

	connect(this, SIGNAL(signalNetClientChatMsg(QString, QString)), myStartNetworkGameDialog, SLOT(receiveChatMsg(QString, QString)));

	connect(this, SIGNAL(signalIrcConnect(QString)), myGameLobbyDialog->getLobbyChat(), SLOT(connected(QString)));
	connect(this, SIGNAL(signalIrcSelfJoined(QString, QString)), myGameLobbyDialog->getLobbyChat(), SLOT(selfJoined(QString, QString)));
	connect(this, SIGNAL(signalIrcPlayerJoined(QString)), myGameLobbyDialog->getLobbyChat(), SLOT(playerJoined(QString)));
	connect(this, SIGNAL(signalIrcPlayerChanged(QString, QString)), myGameLobbyDialog->getLobbyChat(), SLOT(playerChanged(QString, QString)));
	connect(this, SIGNAL(signalIrcPlayerKicked(QString, QString, QString)), myGameLobbyDialog->getLobbyChat(), SLOT(playerKicked(QString, QString, QString)));
	connect(this, SIGNAL(signalIrcPlayerLeft(QString)), myGameLobbyDialog->getLobbyChat(), SLOT(playerLeft(QString)));
	connect(this, SIGNAL(signalIrcChatMessage(QString, QString)), myGameLobbyDialog->getLobbyChat(), SLOT(displayMessage(QString, QString)));
	connect(this, SIGNAL(signalIrcError(int)), myGameLobbyDialog->getLobbyChat(), SLOT(chatError(int)));
	connect(this, SIGNAL(signalIrcServerError(int)), myGameLobbyDialog->getLobbyChat(), SLOT(chatServerError(int)));
	
	this->show();

}

void startWindowImpl::callNewGameDialog() {

	//wenn Dialogfenster gezeigt werden soll
	if(myConfig->readConfigInt("ShowGameSettingsDialogOnNewGame")){

		myNewGameDialog->exec();
		if (myNewGameDialog->result() == QDialog::Accepted ) { myGuiInterface->getMyW()->startNewLocalGame(myNewGameDialog); }
	}
	// sonst mit gespeicherten Werten starten
	else { myGuiInterface->getMyW()->startNewLocalGame(); }
}

void startWindowImpl::callGameLobbyDialog() {

	//Avoid join Lobby with "Human Player" nick
	if(QString::fromUtf8(myConfig->readConfigString("MyName").c_str()) == QString("Human Player")) {
		myChangeHumanPlayerNameDialog->label_Message->setText(tr("You cannot join Internet-Game-Lobby with \"Human Player\" as nickname.\nPlease choose another one."));
		myChangeHumanPlayerNameDialog->exec();

		if(myChangeHumanPlayerNameDialog->result() == QDialog::Accepted) {
			joinGameLobby();
		}
	}
	else {	
		joinGameLobby();
	}
}

void startWindowImpl::joinGameLobby() {

	// Stop local game.
	myGuiInterface->getMyW()->stopTimer();

// Join Lobby
	mySession->terminateNetworkClient();
	if (myServerGuiInterface)
		myServerGuiInterface->getSession().terminateNetworkServer();
		myGameLobbyDialog->setSession(&getSession());
		myStartNetworkGameDialog->setSession(&getSession());

	// Clear Lobby dialog.
	myGameLobbyDialog->clearDialog();

	//set clean irc nick
	QString myNick(QString::fromUtf8(myConfig->readConfigString("MyName").c_str()));
	myNick.replace(QString::fromUtf8("ä"),"ae");
	myNick.replace(QString::fromUtf8("Ä"),"Ae");
	myNick.replace(QString::fromUtf8("ü"),"ue");
	myNick.replace(QString::fromUtf8("Ü"),"Ue");
	myNick.replace(QString::fromUtf8("ö"),"oe");
	myNick.replace(QString::fromUtf8("Ö"),"Oe");
	myNick.replace(QString::fromUtf8("é"),"e");
	myNick.replace(QString::fromUtf8("è"),"e");
	myNick.replace(QString::fromUtf8("á"),"a");
	myNick.replace(QString::fromUtf8("à"),"a");	
	myNick.replace(QString::fromUtf8("ó"),"o");
	myNick.replace(QString::fromUtf8("ò"),"o");
	myNick.replace(QString::fromUtf8("ú"),"u");
	myNick.replace(QString::fromUtf8("ù"),"u");
	myNick.replace(QString::fromUtf8("É"),"E");
	myNick.replace(QString::fromUtf8("È"),"E");
	myNick.replace(QString::fromUtf8("Á"),"A");
	myNick.replace(QString::fromUtf8("À"),"A");	
	myNick.replace(QString::fromUtf8("Ó"),"O");
	myNick.replace(QString::fromUtf8("Ò"),"O");
	myNick.replace(QString::fromUtf8("Ú"),"U");
	myNick.replace(QString::fromUtf8("Ù"),"U");
	myNick.remove(QRegExp("[^A-Z^a-z^0-9|\\-_\\\\^`]*"));
	myNick = myNick.mid(0,16);

 	mySession->setIrcNick(myNick.toUtf8().constData());
	
	// Start client for dedicated server.
	mySession->startInternetClient();
	
	//Dialog mit Statusbalken
	myConnectToServerDialog->exec();
	
	if (myConnectToServerDialog->result() == QDialog::Rejected ) {
		mySession->terminateNetworkClient();
	}
	else
	{
		this->hide();
		showLobbyDialog();
	}
}

void startWindowImpl::callCreateNetworkGameDialog() {
	
	myCreateNetworkGameDialog->exec();
// 
	if (myCreateNetworkGameDialog->result() == QDialog::Accepted ) {

		// Stop local game.
		myGuiInterface->getMyW()->stopTimer();

		if (!myServerGuiInterface)
		{
			// Create pseudo Gui Wrapper for the server.
			myServerGuiInterface.reset(new ServerGuiWrapper(myConfig, mySession->getGui(), mySession->getGui(), mySession->getGui()));
			{
				boost::shared_ptr<Session> session(new Session(myServerGuiInterface.get(), myConfig));
				session->init(mySession->getAvatarManager());
				myServerGuiInterface->setSession(session);
			}
		}

		// Terminate existing network games.
		mySession->terminateNetworkClient();
		myServerGuiInterface->getSession().terminateNetworkServer();

		GameData gameData;
		gameData.maxNumberOfPlayers = myCreateNetworkGameDialog->spinBox_quantityPlayers->value();
		gameData.startMoney = myCreateNetworkGameDialog->spinBox_startCash->value();
		gameData.firstSmallBlind = myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->spinBox_firstSmallBlind->value();
		
		if(myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->radioButton_raiseBlindsAtHands->isChecked()) { 
			gameData.raiseIntervalMode = RAISE_ON_HANDNUMBER;
			gameData.raiseSmallBlindEveryHandsValue = myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->spinBox_raiseSmallBlindEveryHands->value();
		}
		else { 
			gameData.raiseIntervalMode = RAISE_ON_MINUTES; 
			gameData.raiseSmallBlindEveryMinutesValue = myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->spinBox_raiseSmallBlindEveryMinutes->value();
		}
		
		if(myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->radioButton_alwaysDoubleBlinds->isChecked()) { 
			gameData.raiseMode = DOUBLE_BLINDS; 
		}
		else { 
			gameData.raiseMode = MANUAL_BLINDS_ORDER;
			std::list<int> tempBlindList;
			int i;
			bool ok = TRUE;
			for(i=0; i<myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->listWidget_blinds->count(); i++) {
				tempBlindList.push_back(myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->listWidget_blinds->item(i)->text().toInt(&ok,10));		
			}
			gameData.manualBlindsList = tempBlindList;
			
			if(myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->radioButton_afterThisAlwaysDoubleBlinds->isChecked()) { gameData.afterManualBlindsMode = AFTERMB_DOUBLE_BLINDS; }
			else {
				if(myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->radioButton_afterThisAlwaysRaiseAbout->isChecked()) {
					gameData.afterManualBlindsMode = AFTERMB_RAISE_ABOUT;
					gameData.afterMBAlwaysRaiseValue = myCreateNetworkGameDialog->getChangeCompleteBlindsDialog()->spinBox_afterThisAlwaysRaiseValue->value();
				}
				else { gameData.afterManualBlindsMode = AFTERMB_STAY_AT_LAST_BLIND; }	
			}
		}

		gameData.guiSpeed = myCreateNetworkGameDialog->spinBox_gameSpeed->value();
		gameData.playerActionTimeoutSec = myCreateNetworkGameDialog->spinBox_netTimeOutPlayerAction->value();

		myGameLobbyDialog->setSession(&getSession());
		myStartNetworkGameDialog->setSession(&getSession());

		// Clear network game dialog.
		myStartNetworkGameDialog->clearDialog();

		myServerGuiInterface->getSession().startNetworkServer();
		mySession->startNetworkClientForLocalServer(gameData);

		myStartNetworkGameDialog->setMaxPlayerNumber(gameData.maxNumberOfPlayers);

		myStartNetworkGameDialog->setWindowTitle("Start Network Game");

		showNetworkStartDialog();
	}

}

void startWindowImpl::callJoinNetworkGameDialog() {

	myJoinNetworkGameDialog->exec();

	if (myJoinNetworkGameDialog->result() == QDialog::Accepted ) {

		// Stop local game.
		myGuiInterface->getMyW()->stopTimer();

		mySession->terminateNetworkClient();
		if (myServerGuiInterface)
			myServerGuiInterface->getSession().terminateNetworkServer();

		myGameLobbyDialog->setSession(&getSession());
		myStartNetworkGameDialog->setSession(&getSession());
		// Clear network game dialog
		myStartNetworkGameDialog->clearDialog();
		// Maybe use QUrl::toPunycode.
		mySession->startNetworkClient(
			myJoinNetworkGameDialog->lineEdit_ipAddress->text().toUtf8().constData(),
			myJoinNetworkGameDialog->spinBox_port->value(),
			myJoinNetworkGameDialog->checkBox_ipv6->isChecked(),
			myJoinNetworkGameDialog->checkBox_sctp->isChecked(),
			myJoinNetworkGameDialog->lineEdit_password->text().toUtf8().constData());

		//Dialog mit Statusbalken
		myConnectToServerDialog->exec();

		if (myConnectToServerDialog->result() == QDialog::Rejected ) {
			mySession->terminateNetworkClient();
			actionJoin_Network_Game->trigger(); // re-trigger
		}
		else {
			//needed for join and ready sounds - TODO
			//myStartNetworkGameDialog->setMaxPlayerNumber(gameData.maxNumberOfPlayers);
			myStartNetworkGameDialog->setWindowTitle("Start Network Game");

			showNetworkStartDialog();
		}
	}
}


void startWindowImpl::showClientDialog()
{
	if (mySession->getGameType() == Session::GAME_TYPE_NETWORK)
	{
		if (myGuiInterface->getMyW()->isVisible())
			myGuiInterface->getMyW()->hide();
		if (!myStartNetworkGameDialog->isVisible())
			showNetworkStartDialog();
	}
	else if (mySession->getGameType() == Session::GAME_TYPE_INTERNET)
	{
		if (myGuiInterface->getMyW()->isVisible())
			myGuiInterface->getMyW()->hide();
		if (!myGameLobbyDialog->isVisible())
			showLobbyDialog();
	}
}

void startWindowImpl::showLobbyDialog()
{
	myGameLobbyDialog->exec(); 

	if (myGameLobbyDialog->result() == QDialog::Accepted)
	{
		//some gui modifications
		myGuiInterface->getMyW()->networkGameModification();
	}
	else
	{
		mySession->terminateNetworkClient();
	}
}

void startWindowImpl::showNetworkStartDialog()
{
	myStartNetworkGameDialog->exec();

	if (myStartNetworkGameDialog->result() == QDialog::Accepted ) {
		
		//some gui modifications
		myGuiInterface->getMyW()->networkGameModification();
	}
	else {
		mySession->terminateNetworkClient();
		if (myServerGuiInterface)
			myServerGuiInterface->getSession().terminateNetworkServer();
	}
}

void startWindowImpl::callAboutPokerthDialog() { myAboutPokerthDialog->exec(); }

void startWindowImpl::callSettingsDialog() {

	mySettingsDialog->exec();
	
	if (mySettingsDialog->result() == QDialog::Accepted && mySettingsDialog->getSettingsCorrect()) {
		myGuiInterface->getMyW()->applySettings();
	}
}
