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
#ifndef CHATTOOLS_H
#define CHATTOOLS_H

#include <string>
#include <QtCore>
#include <QtGui>

class Session;
class ConfigFile;

class ChatTools : public QObject
{
Q_OBJECT

public:
	ChatTools(QLineEdit* l, ConfigFile *c, QTreeWidget *t = NULL, QTextBrowser *b = NULL, int notifyMode = 0);

	~ChatTools();

       	void setSession(Session *session) { mySession = session; }

public slots:
	
	void sendMessage();
	void receiveMessage(QString playerName, QString message);
	void clearChat();
	void checkInputLength(QString string);
	
	void fillChatLinesHistory(QString fillString);
	void showChatHistoryIndex(int index);
	int getChatLinesHistorySize() { return chatLinesHistory.size(); }
	
	void nickAutoCompletition();
	void setChatTextEdited();

	void setPlayerNicksList(QStringList value) { myNickStringList = value; }

private:
	QStringList chatLinesHistory;
	QString lastChatString;
	QStringList lastMatchStringList;
	int nickAutoCompletitionCounter;

	QLineEdit *myLineEdit;
	QTreeWidget *myNickTreeWidget;
	QStringList myNickStringList;
	QTextBrowser *myTextBrowser;
	Session *mySession; 
	int myNotifyMode; // 0 == no notification, 1 == bold notification, 2 == yellow notification	
	ConfigFile *myConfig;


// friend class GuiWrapper;
};

#endif
