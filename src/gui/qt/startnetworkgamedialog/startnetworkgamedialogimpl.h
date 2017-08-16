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
#ifndef STARTNETWORKGAMEDIALOGIMPL_H
#define STARTNETWORKGAMEDIALOGIMPL_H

#ifdef GUI_800x480
#include "ui_startnetworkgamedialog_800x480.h"
#else
#include "ui_startnetworkgamedialog.h"
#endif

#include <QtGui>
#include <QtCore>
#include "gametableimpl.h"

class Session;
class ConfigFile;
class ChatTools;
class startWindowImpl;

class startNetworkGameDialogImpl: public QDialog, public Ui::startNetworkGameDialog
{
	Q_OBJECT
public:
	startNetworkGameDialogImpl(startWindowImpl *parent = 0, ConfigFile *config = 0);

	void setSession(boost::shared_ptr<Session> session);

public slots:

	void setMyW ( gameTableImpl* theValue )
	{
		myW = theValue;
	}
	ChatTools *getMyChat()
	{
		return myChat;
	}

	void startGame();
	void cancel();
	void refresh(int actionID);
	void accept();
	void reject();
	void joinedNetworkGame(unsigned playerId, QString playerName, bool admin);
	void addConnectedPlayer(unsigned playerId, QString playerName, bool admin);
	void updatePlayer(unsigned playerId, QString newPlayerName);
	void removePlayer(unsigned playerId, QString playerName);
	void newGameAdmin(unsigned playerId, QString playerName);

	void gameCreated(unsigned gameId);

	void playerSelected(QTreeWidgetItem*, QTreeWidgetItem*);
	void kickPlayer();
	void checkPlayerQuantity();
	void clearDialog();

	void keyPressEvent ( QKeyEvent*);
	bool eventFilter(QObject *obj, QEvent *event);

	void setMaxPlayerNumber ( int theValue )
	{
		maxPlayerNumber = theValue;
		label_maxPlayerNumber->setText(QString::number(theValue,10));
	}
	int getMaxPlayerNumber() const
	{
		return maxPlayerNumber;
	}

	int exec();

private:

	gameTableImpl* myW;
	startWindowImpl* myStartWindow;
	int maxPlayerNumber;
	int keyUpDownChatCounter;
	unsigned myPlayerId;
	bool isAdmin;
	ConfigFile *myConfig;
	boost::shared_ptr<Session> mySession;
	ChatTools *myChat;

};

#endif
