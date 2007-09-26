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

	connect(this, SIGNAL(signalChatMessage(QString, QString)), this, SLOT(receiveMessage(QString, QString)));
}

LobbyChat::~LobbyChat()
{

}

void LobbyChat::sendMessage() {

	QString tmpMsg(myLobby->lineEdit_ChatInput->text());
	myLobby->getSession().sendIrcChatMessage(tmpMsg.toUtf8().constData());
	myLobby->lineEdit_ChatInput->setText("");

	// TODO: just temporary instead of callback
	receiveMessage("(me)", tmpMsg);
}

void LobbyChat::receiveMessage(QString playerName, QString message) { 

	myLobby->textBrowser_ChatDisplay->append(playerName + ": " + message); 
}

void LobbyChat::checkInputLength(QString string) {

	 if(string.toUtf8().length() > 120) myLobby->lineEdit_ChatInput->setMaxLength(string.length());  
}

void LobbyChat::clearChat() {

	myLobby->textBrowser_ChatDisplay->clear();
/*	QStringList wordList;
	wordList << "alpha" << "omega" << "omicron" << "zeta";*/
	
// 	QCompleter *completer = new QCompleter(wordList, this);
// 	completer->setCaseSensitivity(Qt::CaseInsensitive);
// 	completer->setCompletionMode(QCompleter::InlineCompletion);
// // 	lineEdit_ChatInput->setCompleter(completer);
	
}
