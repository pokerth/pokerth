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
#include "chat.h"

#include "mainwindowimpl.h"
#include "session.h"
#include "chattools.h"
#include "configfile.h"

using namespace std;

Chat::Chat(mainWindowImpl* w, ConfigFile *c) : myW(w), myConfig(c)
{
	myW->setChat(this);

	myChatTools = new ChatTools(myW->lineEdit_ChatInput);
	connect(this, SIGNAL(signalChatMessage(QString, QString)), this, SLOT(receiveMessage(QString, QString)));
}

Chat::~Chat()
{
	delete myConfig;
	myConfig = 0;
}

void Chat::sendMessage() {

	myW->getSession().sendChatMessage(myW->lineEdit_ChatInput->text().toUtf8().constData());
	myW->lineEdit_ChatInput->setText("");
}

void Chat::receiveMessage(QString playerName, QString message) { 

	myW->textBrowser_Chat->append(playerName + ": " + message); 
	checkInvisible();
}

void Chat::checkInvisible() {
		
	switch (myW->tabWidget_Left->currentIndex()) {

		case 1: { myW->tabWidget_Left->stopBlinkChatTab();
			  myW->tabWidget_Left->showDefaultChatTab();
			}
		break;
		default: { myW->tabWidget_Left->startBlinkChatTab(); }
	} 
}

void Chat::checkInputLength(QString string) {

	 if(string.toUtf8().length() > 120) myW->lineEdit_ChatInput->setMaxLength(string.length());  
}

void Chat::clearNewGame() {

	myW->textBrowser_Chat->clear();
}

void Chat::setPlayerNicksList(QStringList nickList) {
	//fill playerNicksList for nick-autocompletition		
	myChatTools->setPlayerNicksList(nickList);
}

void Chat::fillChatLinesHistory(QString fillString) { myChatTools->fillChatLinesHistory(fillString); }
int Chat::getChatLinesHistorySize() { return myChatTools->getChatLinesHistorySize(); }
void Chat::showChatHistoryIndex(int index) { myChatTools->showChatHistoryIndex(index); }
void Chat::nickAutoCompletition() { myChatTools->nickAutoCompletition(); }
void Chat::setChatTextEdited() { myChatTools->setChatTextEdited(); }
