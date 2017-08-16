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
#ifndef STARTWINDOWIMPL_H
#define STARTWINDOWIMPL_H

#include <boost/shared_ptr.hpp>
#include <assert.h>

#ifdef GUI_800x480
#include "ui_startwindow_800x480.h"
#else
#include "ui_startwindow.h"
#endif

#include <game_defs.h>
#include <serverdata.h>
#include <QMessageBox>
#include "mymessagebox.h"

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
class serverListDialogImpl;
class internetGameLoginDialogImpl;
class guiLog;
class Log;
class LogFileDialog;

class startWindowImpl: public QMainWindow, public Ui::startWindow
{
	Q_OBJECT
public:
	startWindowImpl(ConfigFile *c, Log *l);
	~startWindowImpl();

	void setSession(boost::shared_ptr<Session> session)
	{
		mySession = session;
	}
	boost::shared_ptr<Session> getSession()
	{
		assert(mySession.get());
		return mySession;
	}
	boost::shared_ptr< GuiInterface > getMyServerGuiInterface() const
	{
		return myServerGuiInterface;
	}
	connectToServerDialogImpl* getMyConnectToServerDialog() const
	{
		return myConnectToServerDialog;
	}

	void setGuiLog(guiLog* l)
	{
		myGuiLog = l;
	}

	//	void keyPressEvent( QKeyEvent *);
	bool eventFilter(QObject *obj, QEvent *event);

signals:
	void signalShowClientDialog();

	void signalNetClientConnect(int actionID);
	void signalNetClientServerListShow();
	void signalNetClientServerListClear();
	void signalNetClientServerListAdd(unsigned serverId);
	void signalNetClientLoginShow();
	void signalNetClientRejoinPossible(unsigned gameId);
	void signalNetClientGameInfo(int actionID);
	void signalNetClientError(int errorID, int osErrorID);
	void signalNetClientNotification(int notificationId);
	void signalNetClientStatsUpdate(ServerStats stats);
	void signalNetClientShowTimeoutDialog(int, unsigned);
	void signalNetClientRemovedFromGame(int notificationId);
	void signalNetServerError(int errorID, int osErrorID);
	void signalNetClientSelfJoined(unsigned playerId, QString playerName, bool isGameAdmin);
	void signalNetClientPlayerJoined(unsigned playerId, QString playerName, bool isGameAdmin);
	void signalNetClientPlayerChanged(unsigned playerId, QString newPlayerName);
	void signalNetClientPlayerLeft(unsigned playerId, QString playerName);
	void signalNetClientSpectatorJoined(unsigned playerId, QString playerName);
	void signalNetClientSpectatorLeft(unsigned playerId, QString playerName);
	void signalNetClientNewGameAdmin(unsigned playerId, QString playerName);
	void signalNetClientGameListNew(unsigned gameId);
	void signalNetClientGameListRemove(unsigned gameId);
	void signalNetClientGameListUpdateMode(unsigned gameId, int mode);
	void signalNetClientGameListUpdateAdmin(unsigned gameId, unsigned adminPlayerId);
	void signalNetClientGameListPlayerJoined(unsigned gameId, unsigned playerId);
	void signalNetClientGameListPlayerLeft(unsigned gameId, unsigned playerId);
	void signalNetClientGameListSpectatorJoined(unsigned gameId, unsigned playerId);
	void signalNetClientGameListSpectatorLeft(unsigned gameId, unsigned playerId);
	void signalNetClientGameStart(boost::shared_ptr<Game> game);
	void signalNetClientGameChatMsg(QString nickName, QString msg);
	void signalNetClientLobbyChatMsg(QString nickName, QString msg);
	void signalNetClientPrivateChatMsg(QString nickName, QString msg);
	void signalNetClientMsgBox(QString msg);
	void signalNetClientMsgBox(unsigned msgId);

	void signalLobbyPlayerJoined(unsigned playerId, QString nickName);
	void signalLobbyPlayerKicked(QString nickName, QString byWhom, QString reason);
	void signalLobbyPlayerLeft(unsigned playerId);

	void signalSelfGameInvitation(unsigned gameId, unsigned playerIdFrom);
	void signalPlayerGameInvitation(unsigned gameId, unsigned playerIdWho, unsigned playerIdFrom);
	void signalRejectedGameInvitation(unsigned gameId, unsigned playerIdWho, DenyGameInvitationReason reason);

public slots:

	void callAboutPokerthDialog();
	void callSettingsDialog(bool);
	void callSettingsDialogFromStartwindow();
	void callNewGameDialog();
	void callGameLobbyDialog();
	void callCreateNetworkGameDialog();
	void callJoinNetworkGameDialog();
	void showLobbyDialog();
	void callInternetGameLoginDialog();
	void callRejoinPossibleDialog(unsigned);
	void joinGameLobby();
	void showClientDialog();
	void showNetworkStartDialog();
	void callLogFileDialog();

	void startNewLocalGame(newGameDialogImpl* =0);

	void showTimeoutDialog(int msgID, unsigned duration);
	void hideTimeoutDialog();

	void networkError(int, int);
	void networkNotification(int);
	void networkMessage(QString);
	void networkMessage(unsigned);

	void networkStart(boost::shared_ptr<Game> game);
	QStringList getPlayerNicksList();

	QString checkForFirstStartAfterUpdated();

private:
	ConfigFile *myConfig;
	guiLog *myGuiLog;
	Log *myLog;

	boost::shared_ptr<GuiInterface> myGuiInterface;
	boost::shared_ptr<Session> mySession;
	boost::shared_ptr<GuiInterface> myServerGuiInterface;

	// 	Dialogs
	aboutPokerthImpl *myAboutPokerthDialog;
	newGameDialogImpl *myNewGameDialog;
	settingsDialogImpl *mySettingsDialog;
	selectAvatarDialogImpl *mySelectAvatarDialog;
	joinNetworkGameDialogImpl *myJoinNetworkGameDialog;
	connectToServerDialogImpl *myConnectToServerDialog;
	startNetworkGameDialogImpl *myStartNetworkGameDialog;
	createNetworkGameDialogImpl *myCreateNetworkGameDialog;
	gameLobbyDialogImpl *myGameLobbyDialog;
	timeoutMsgBoxImpl *myTimeoutDialog;
	startWindowImpl *myStartWindow;
	serverListDialogImpl *myServerListDialog;
	internetGameLoginDialogImpl *myInternetGameLoginDialog;
	LogFileDialog *myLogFileDialog;

	MyMessageBox msgBoxOutdatedVersion;
	bool msgBoxOutdatedVersionActive;

	friend class GuiWrapper;
};

#endif
