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
#ifndef CHATTOOLS_H
#define CHATTOOLS_H

#include <string>
#include <QtCore>
#if QT_VERSION >= 0x050000
#include <QtWidgets>
#endif
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

	void setSession(boost::shared_ptr<Session> session)
	{
		mySession = session;
	}

public slots:

	void sendMessage();
	void receiveMessage(QString playerName, QString message, bool pm=false);
	void privateMessage(QString playerName, QString message);
	void clearChat();
	void checkInputLength(QString string);

	void fillChatLinesHistory(QString fillString);
	void showChatHistoryIndex(int index);
	int getChatLinesHistorySize()
	{
		return chatLinesHistory.size();
	}

	void nickAutoCompletition();
	void setChatTextEdited();

	void setPlayerNicksList(QStringList value)
	{
		myNickStringList = value;
	}
	void setMyNick ( const QString& theValue )
	{
		myNick = theValue;
	}
	QString getMyNick ()
	{
		return myNick;
	}

	void setMyStyle ( GameTableStyleReader* theValue )
	{
		myStyle = theValue;
	}
	void refreshIgnoreList();
	QString checkForEmotes(QString);

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
