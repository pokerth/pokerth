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
#ifndef LOBBYCHAT_H
#define LOBBYCHAT_H

#include <string>
#include <QtCore>
#include <QtGui>

class gameLobbyDialogImpl;
class Session;
class ConfigFile;
class ChatTools;

class LobbyChat : public QObject
{
Q_OBJECT

public:
	LobbyChat(gameLobbyDialogImpl*, ConfigFile*);

	~LobbyChat();

public slots:
	
	void sendMessage();
	void playerJoined(unsigned playerId, QString playerName);
	void playerChanged(unsigned playerId, QString newNick);
	void playerKicked(QString nickName, QString byWhom, QString reason);
	void playerLeft(unsigned playerId);
	void displayMessage(QString playerName, QString msg);
	void checkInputLength(QString);
	void clearChat();

	void chatError(int errorCode);
	void chatServerError(int errorCode);

	ChatTools* getMyChatTools() const { return myChatTools; }

	void fillChatLinesHistory(QString fillString);
	void showChatHistoryIndex(int index);
	int getChatLinesHistorySize();
	void nickAutoCompletition();
	void setChatTextEdited();

private:
	gameLobbyDialogImpl *myLobby;
	ConfigFile *myConfig;

	QString myNick;
	unsigned myLobbyPlayerId;
	ChatTools *myChatTools;
// friend class GuiWrapper;
};

#endif
