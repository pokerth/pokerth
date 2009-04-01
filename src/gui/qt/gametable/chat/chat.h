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
#ifndef CHAT_H
#define CHAT_H

#include <string>
#include <QtCore>


class gameTableImpl;
class ChatTools;
class ConfigFile;
class GameTableStyleReader;

class Chat : public QObject
{
Q_OBJECT

public:
	Chat(gameTableImpl*, ConfigFile *c);

	~Chat();

public slots:
	
	void sendMessage();
	void receiveMessage(QString playerName, QString msg);
	void checkInvisible();
	void checkInputLength(QString);
	void clearNewGame();
	void transportMyStyle(GameTableStyleReader*);

	ChatTools* getMyChatTools() const { return myChatTools; }

	void fillChatLinesHistory(QString fillString);
	void showChatHistoryIndex(int index);
	int getChatLinesHistorySize();
	void nickAutoCompletition();
	void setChatTextEdited();
	void setPlayerNicksList(QStringList);
		
private:
	
	gameTableImpl *myW;
	ConfigFile *myConfig;
	ChatTools *myChatTools;

	
friend class GuiWrapper;
};

#endif
