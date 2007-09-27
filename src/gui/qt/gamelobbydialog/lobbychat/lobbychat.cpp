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
#include "lobbychat.h"

#include "gamelobbydialogimpl.h"
#include "session.h"

using namespace std;


LobbyChat::LobbyChat(gameLobbyDialogImpl* l) : myLobby(l)
{
}

LobbyChat::~LobbyChat()
{

}

void LobbyChat::sendMessage() {

	if (myNick.size())
	{
		QString tmpMsg(myLobby->lineEdit_ChatInput->text());
		if (tmpMsg.size())
		{
			myLobby->getSession().sendIrcChatMessage(tmpMsg.toUtf8().constData());
			myLobby->lineEdit_ChatInput->setText("");

			displayMessage(myNick, tmpMsg);
		}
	}
}

void LobbyChat::connected(QString server)
{
	myLobby->textBrowser_ChatDisplay->append(tr("Successfully connected to") + " " + server + ".");
}

void LobbyChat::selfJoined(QString ownName, QString channel)
{
	myNick = ownName;
	myLobby->textBrowser_ChatDisplay->append(tr("Joined channel:") + " " + channel + " " + tr("as user") + " " + ownName + ".");
	myLobby->textBrowser_ChatDisplay->append("");
	playerJoined(ownName);
	myLobby->lineEdit_ChatInput->setEnabled(true);
	myLobby->lineEdit_ChatInput->setFocus();
}

void LobbyChat::playerJoined(QString playerName)
{
	QTreeWidgetItem *item = new QTreeWidgetItem(myLobby->treeWidget_NickList, 0);
	item->setData(0, Qt::DisplayRole, playerName);
}

void LobbyChat::playerLeft(QString playerName)
{
	QList<QTreeWidgetItem *> tmpList(myLobby->treeWidget_NickList->findItems(playerName, Qt::MatchExactly));
	if (!tmpList.empty())
		myLobby->treeWidget_NickList->takeTopLevelItem(myLobby->treeWidget_NickList->indexOfTopLevelItem(tmpList.front()));
}

void LobbyChat::displayMessage(QString playerName, QString message) { 

	myLobby->textBrowser_ChatDisplay->append(playerName + ": " + message); 
}

void LobbyChat::checkInputLength(QString string) {

	 if(string.toUtf8().length() > 120) myLobby->lineEdit_ChatInput->setMaxLength(string.length());  
}

void LobbyChat::clearChat() {

	myNick = "";
	myLobby->treeWidget_NickList->clear();
	myLobby->textBrowser_ChatDisplay->clear();
	myLobby->textBrowser_ChatDisplay->append(tr("Connecting to IRC server..."));
	myLobby->lineEdit_ChatInput->setEnabled(false);
/*	QStringList wordList;
	wordList << "alpha" << "omega" << "omicron" << "zeta";*/
	
// 	QCompleter *completer = new QCompleter(wordList, this);
// 	completer->setCaseSensitivity(Qt::CaseInsensitive);
// 	completer->setCompletionMode(QCompleter::InlineCompletion);
// // 	lineEdit_ChatInput->setCompleter(completer);
	
}
