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



startWindowImpl::startWindowImpl(gameTableImpl *w, ConfigFile *c)
    : myW(w), myConfig(c)
{
// #ifdef __APPLE__
// 	setWindowModality(Qt::ApplicationModal);
// 	setWindowFlags(Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint | Qt::Dialog);
// #endif	
	setupUi(this);
	this->setWindowTitle(QString(tr("PokerTH %1 - The Open-Source Texas Holdem Engine").arg(POKERTH_BETA_RELEASE_STRING)));

// 	Dialogs
	myNewGameDialog = boost::shared_ptr<newGameDialogImpl>(new newGameDialogImpl(this, myConfig));
	mySelectAvatarDialog = boost::shared_ptr<selectAvatarDialogImpl>(new selectAvatarDialogImpl(this, myConfig));
	mySettingsDialog = boost::shared_ptr<settingsDialogImpl>(new settingsDialogImpl(this, myConfig, mySelectAvatarDialog.get()));	
	myChangeHumanPlayerNameDialog = boost::shared_ptr<changeHumanPlayerNameDialogImpl>(new changeHumanPlayerNameDialogImpl(this, myConfig));
	myJoinNetworkGameDialog = boost::shared_ptr<joinNetworkGameDialogImpl>(new joinNetworkGameDialogImpl(this, myConfig));
	myConnectToServerDialog = boost::shared_ptr<connectToServerDialogImpl>(new connectToServerDialogImpl(this));
	myStartNetworkGameDialog = boost::shared_ptr<startNetworkGameDialogImpl>(new startNetworkGameDialogImpl(this, myConfig));
	myCreateNetworkGameDialog = boost::shared_ptr<createNetworkGameDialogImpl>(new createNetworkGameDialogImpl(this, myConfig));
	myAboutPokerthDialog = boost::shared_ptr<aboutPokerthImpl>(new aboutPokerthImpl(this, myConfig));
	myGameLobbyDialog = boost::shared_ptr<gameLobbyDialogImpl>(new gameLobbyDialogImpl(this, myConfig));

	myStartNetworkGameDialog->setMyW(myW);
	myGameLobbyDialog->setMyW(myW);
	
	myTimeoutDialog.reset(new timeoutMsgBoxImpl(this));


	connect( actionStart_Local_Game, SIGNAL( triggered() ), this, SLOT( callNewGameDialog() ) );
	connect( pushButtonStart_Local_Game, SIGNAL( clicked() ), this, SLOT( callNewGameDialog() ) );
	connect( actionInternet_Game, SIGNAL( triggered() ), this, SLOT( callGameLobbyDialog() ) );

	connect(this, SIGNAL(signalShowClientDialog()), this, SLOT(showClientDialog()));

	connect(this, SIGNAL(signalNetClientConnect(int)), myConnectToServerDialog.get(), SLOT(refresh(int)));
	connect(this, SIGNAL(signalNetClientGameInfo(int)), myStartNetworkGameDialog.get(), SLOT(refresh(int)));
	connect(this, SIGNAL(signalNetClientGameInfo(int)), myGameLobbyDialog.get(), SLOT(refresh(int)));

	connect(this, SIGNAL(signalNetClientSelfJoined(unsigned, QString, int)), myStartNetworkGameDialog.get(), SLOT(joinedNetworkGame(unsigned, QString, int)));
	connect(this, SIGNAL(signalNetClientPlayerJoined(unsigned, QString, int)), myStartNetworkGameDialog.get(), SLOT(addConnectedPlayer(unsigned, QString, int)));
	connect(this, SIGNAL(signalNetClientPlayerChanged(unsigned, QString)), myStartNetworkGameDialog.get(), SLOT(updatePlayer(unsigned, QString)));
	connect(this, SIGNAL(signalNetClientPlayerLeft(unsigned, QString)), myStartNetworkGameDialog.get(), SLOT(removePlayer(unsigned, QString)));
	connect(this, SIGNAL(signalNetClientNewGameAdmin(unsigned, QString)), myStartNetworkGameDialog.get(), SLOT(newGameAdmin(unsigned, QString)));
	connect(this, SIGNAL(signalNetClientGameListNew(unsigned)), myStartNetworkGameDialog.get(), SLOT(gameCreated(unsigned)));

	connect(this, SIGNAL(signalNetClientSelfJoined(unsigned, QString, int)), myGameLobbyDialog.get(), SLOT(joinedNetworkGame(unsigned, QString, int)));
	connect(this, SIGNAL(signalNetClientPlayerJoined(unsigned, QString, int)), myGameLobbyDialog.get(), SLOT(addConnectedPlayer(unsigned, QString, int)));
	connect(this, SIGNAL(signalNetClientPlayerChanged(unsigned, QString)), myGameLobbyDialog.get(), SLOT(updatePlayer(unsigned, QString)));
	connect(this, SIGNAL(signalNetClientPlayerLeft(unsigned, QString)), myGameLobbyDialog.get(), SLOT(removePlayer(unsigned, QString)));
	connect(this, SIGNAL(signalNetClientNewGameAdmin(unsigned, QString)), myGameLobbyDialog.get(), SLOT(newGameAdmin(unsigned, QString)));

	connect(this, SIGNAL(signalNetClientGameListNew(unsigned)), myGameLobbyDialog.get(), SLOT(addGame(unsigned)));
	connect(this, SIGNAL(signalNetClientGameListRemove(unsigned)), myGameLobbyDialog.get(), SLOT(removeGame(unsigned)));
	connect(this, SIGNAL(signalNetClientGameListUpdateMode(unsigned, int)), myGameLobbyDialog.get(), SLOT(updateGameMode(unsigned, int)));
	connect(this, SIGNAL(signalNetClientGameListUpdateAdmin(unsigned, unsigned)), myGameLobbyDialog.get(), SLOT(updateGameAdmin(unsigned, unsigned)));
	connect(this, SIGNAL(signalNetClientGameListPlayerJoined(unsigned, unsigned)), myGameLobbyDialog.get(), SLOT(gameAddPlayer(unsigned, unsigned)));
	connect(this, SIGNAL(signalNetClientGameListPlayerLeft(unsigned, unsigned)), myGameLobbyDialog.get(), SLOT(gameRemovePlayer(unsigned, unsigned)));
	connect(this, SIGNAL(signalNetClientRemovedFromGame(int)), myGameLobbyDialog.get(), SLOT(removedFromGame(int)));
	connect(this, SIGNAL(signalNetClientStatsUpdate(ServerStats)), myGameLobbyDialog.get(), SLOT(updateStats(ServerStats)));

	connect(this, SIGNAL(signalNetClientChatMsg(QString, QString)), myStartNetworkGameDialog.get(), SLOT(receiveChatMsg(QString, QString)));

	connect(this, SIGNAL(signalIrcConnect(QString)), myGameLobbyDialog->getLobbyChat(), SLOT(connected(QString)));
	connect(this, SIGNAL(signalIrcSelfJoined(QString, QString)), myGameLobbyDialog->getLobbyChat(), SLOT(selfJoined(QString, QString)));
	connect(this, SIGNAL(signalIrcPlayerJoined(QString)), myGameLobbyDialog->getLobbyChat(), SLOT(playerJoined(QString)));
	connect(this, SIGNAL(signalIrcPlayerChanged(QString, QString)), myGameLobbyDialog->getLobbyChat(), SLOT(playerChanged(QString, QString)));
	connect(this, SIGNAL(signalIrcPlayerKicked(QString, QString, QString)), myGameLobbyDialog->getLobbyChat(), SLOT(playerKicked(QString, QString, QString)));
	connect(this, SIGNAL(signalIrcPlayerLeft(QString)), myGameLobbyDialog->getLobbyChat(), SLOT(playerLeft(QString)));
	connect(this, SIGNAL(signalIrcChatMessage(QString, QString)), myGameLobbyDialog->getLobbyChat(), SLOT(displayMessage(QString, QString)));
	connect(this, SIGNAL(signalIrcError(int)), myGameLobbyDialog->getLobbyChat(), SLOT(chatError(int)));
	connect(this, SIGNAL(signalIrcServerError(int)), myGameLobbyDialog->getLobbyChat(), SLOT(chatServerError(int)));
	

}

void startWindowImpl::callNewGameDialog() {

	//wenn Dialogfenster gezeigt werden soll
	if(myConfig->readConfigInt("ShowGameSettingsDialogOnNewGame")){

		myNewGameDialog->exec();
		if (myNewGameDialog->result() == QDialog::Accepted ) { myW->startNewLocalGame(myNewGameDialog.get()); }
	}
	// sonst mit gespeicherten Werten starten
	else { myW->startNewLocalGame(); }
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
	myW->stopTimer();

// Join Lobby
	mySession->terminateNetworkClient();
	if (myServerGuiInterface.get())
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
		showLobbyDialog();
	}
}

void startWindowImpl::showClientDialog()
{
	if (mySession->getGameType() == Session::GAME_TYPE_NETWORK)
	{
		if (!myStartNetworkGameDialog->isVisible())
			showNetworkStartDialog();
	}
	else if (mySession->getGameType() == Session::GAME_TYPE_INTERNET)
	{
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
		myW->networkGameModification();
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
		myW->networkGameModification();
	}
	else {
		mySession->terminateNetworkClient();
		if (myServerGuiInterface)
			myServerGuiInterface->getSession().terminateNetworkServer();
	}
}
