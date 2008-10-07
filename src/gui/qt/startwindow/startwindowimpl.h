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
#ifndef STARTWINDOWIMPL_H
#define STARTWINDOWIMPL_H

#include <boost/shared_ptr.hpp>
#include "ui_startwindow.h"
#include "game_defs.h"

class GuiInterface;
class GuiWrapper;
class Session;
class Game;

class ConfigFile;
class gameTableImpl;
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


class startWindowImpl: public QMainWindow, public Ui::startWindow {
Q_OBJECT
public:
    startWindowImpl(ConfigFile *c =0);

	void setSession(boost::shared_ptr<Session> session) { mySession = session; }
	boost::shared_ptr<Session> getSession() { assert(mySession.get()); return mySession; }

signals: 
	void signalShowClientDialog();
	
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

	void callAboutPokerthDialog();
	void callSettingsDialog();
	void callNewGameDialog();
	void callGameLobbyDialog();
	void callCreateNetworkGameDialog();
	void callJoinNetworkGameDialog();
	void showLobbyDialog();
	void joinGameLobby();
	void showClientDialog();
	void showNetworkStartDialog();

	void startNewLocalGame(newGameDialogImpl* =0);

	void showTimeoutDialog(int msgID, unsigned duration);
	void hideTimeoutDialog();

	void networkError(int, int);
	void networkNotification(int);

	void networkStart(boost::shared_ptr<Game> game);
	QStringList getPlayerNicksList();

private:
	ConfigFile *myConfig;

	boost::shared_ptr<GuiInterface> myGuiInterface;
	boost::shared_ptr<Session> mySession;
	boost::shared_ptr<GuiInterface> myServerGuiInterface;

// 	Dialogs
	aboutPokerthImpl *myAboutPokerthDialog;
	newGameDialogImpl *myNewGameDialog;
	settingsDialogImpl *mySettingsDialog;
	selectAvatarDialogImpl *mySelectAvatarDialog;
	changeHumanPlayerNameDialogImpl *myChangeHumanPlayerNameDialog;
	joinNetworkGameDialogImpl *myJoinNetworkGameDialog;
	connectToServerDialogImpl *myConnectToServerDialog;
	startNetworkGameDialogImpl *myStartNetworkGameDialog;
	createNetworkGameDialogImpl *myCreateNetworkGameDialog;
	gameLobbyDialogImpl *myGameLobbyDialog;
	timeoutMsgBoxImpl *myTimeoutDialog;
	startWindowImpl *myStartWindow;

	
friend class GuiWrapper;
};

#endif
