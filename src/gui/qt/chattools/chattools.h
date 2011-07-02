/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2011 Felix Hammer, Florian Thauer, Lothar May          *
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
 *****************************************************************************/ #ifndef CHATTOOLS_H
#define CHATTOOLS_H

#include <string>
#include <QtCore>
#include <QtGui>
#include <boost/shared_ptr.hpp>

enum ChatType { INET_LOBBY_CHAT, LAN_LOBBY_CHAT, INGAME_CHAT };

class Session;
class ConfigFile;
class GameTableStyleReader;
class gameLobbyDialogImpl;

class ChatTools : public QObject
{
	Q_OBJECT

public:
	ChatTools(QLineEdit*, ConfigFile*, ChatType, QTextBrowser *b = NULL, QStandardItemModel *m = NULL, gameLobbyDialogImpl *lo = NULL);

	~ChatTools();

	void setSession(boost::shared_ptr<Session> session) {
		mySession = session;
	}

public slots:

	void sendMessage();
	void receiveMessage(QString playerName, QString message);
	void privateMessage(QString playerName, QString message);
	void clearChat();
	void checkInputLength(QString string);

	void fillChatLinesHistory(QString fillString);
	void showChatHistoryIndex(int index);
	int getChatLinesHistorySize() {
		return chatLinesHistory.size();
	}

	void nickAutoCompletition();
	void setChatTextEdited();

	void setPlayerNicksList(QStringList value) {
		myNickStringList = value;
	}
	void setMyNick ( const QString& theValue ) {
		myNick = theValue;
	}
	QString getMyNick () {
		return myNick;
	}

	void setMyStyle ( GameTableStyleReader* theValue ) {
		myStyle = theValue;
	}
	void refreshIgnoreList();

protected:

	unsigned parsePrivateMessageTarget(QString &chatText);

private:

	QStringList chatLinesHistory;
	QString lastChatString;
	QStringList lastMatchStringList;
	int nickAutoCompletitionCounter;

	QLineEdit *myLineEdit;
	QStandardItemModel *myNickListModel;
	QStringList myNickStringList;
	QTextBrowser *myTextBrowser;
	boost::shared_ptr<Session> mySession;
	ChatType myChatType;
	ConfigFile *myConfig;

	QString myNick;

	GameTableStyleReader *myStyle;
	gameLobbyDialogImpl *myLobby;

	std::list<std::string> ignoreList;
};

#endif
