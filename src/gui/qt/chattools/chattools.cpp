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
#include "chattools.h"
#include "session.h"
#include "configfile.h"
#include <iostream>

using namespace std;


ChatTools::ChatTools(QLineEdit* l, ConfigFile *c, int notifyMode, QTextBrowser *b, QTreeWidget *t) : nickAutoCompletitionCounter(0), myLineEdit(l), myNickTreeWidget(t), myNickStringList(NULL), myTextBrowser(b), myNotifyMode(notifyMode), myConfig(c), myNick("")
{

}

ChatTools::~ChatTools()
{

}

void ChatTools::sendMessage() {
	
	fillChatLinesHistory(myLineEdit->text());
	if(mySession) {
		mySession->sendChatMessage(myLineEdit->text().toUtf8().constData());
		myLineEdit->setText("");
	}
	else { cout << "Session is not valid" << endl;}
}

void ChatTools::receiveMessage(QString playerName, QString message) { 

	if(myTextBrowser) {

		message = message.replace("<","&lt;");
		message = message.replace(">","&gt;");

		QString tempMsg;
		QString nickString;

		if(myNick == "") { nickString = QString::fromUtf8(myConfig->readConfigString("MyName").c_str()); }
		else { nickString = myNick;  }

		if(message.contains(nickString, Qt::CaseInsensitive)) {

			switch (myNotifyMode) {
				case 0: tempMsg = message;
				break;
				case 1:	tempMsg = QString("<span style=\"font-weight:bold;\">"+message+"</span>");
				break;
				case 2: tempMsg = QString("<span style=\"color:#FFFF00;\">"+message+"</span>");
				break;
				default:;
			}
		}
		else {
			switch (myNotifyMode) {
				case 0: tempMsg = message;
				break;
				case 1:	tempMsg = QString("<span style=\"font-weight:normal;\">"+message+"</span>");
				break;
				case 2: tempMsg = QString("<span style=\"color:#FFFFFF;\">"+message+"</span>");
				break;
				default:;
			}
			
		}
		myTextBrowser->append(playerName + ": " + tempMsg); 
	}
}

void ChatTools::clearChat() {

	if(myTextBrowser)
		myTextBrowser->clear();
}

void ChatTools::checkInputLength(QString string) {

	 if(string.toUtf8().length() > 120) myLineEdit->setMaxLength(string.length());  
}

void ChatTools::fillChatLinesHistory(QString fillString) {

	chatLinesHistory << fillString;
	if(chatLinesHistory.size() > 50) chatLinesHistory.removeFirst();


}

void ChatTools::showChatHistoryIndex(int index) { 

	if(index <= chatLinesHistory.size()) {

// 		cout << chatLinesHistory.size() << " : " <<  index << endl;
		if(index > 0)
			myLineEdit->setText(chatLinesHistory.at(chatLinesHistory.size()-(index)));  
		else
			myLineEdit->setText("");
	}
}

void ChatTools::nickAutoCompletition() {

	QString myChatString = myLineEdit->text();
	QStringList myChatStringList = myChatString.split(" ");

	QStringList matchStringList;

	if(nickAutoCompletitionCounter == 0) {

		if(myNickTreeWidget) {
// 			cout << "use Treewidget niclist" << endl;
			QTreeWidgetItemIterator it(myNickTreeWidget);
			while (*it) {
				if ((*it)->text(0).startsWith(myChatStringList.last(), Qt::CaseInsensitive) && myChatStringList.last() != "")
				matchStringList << (*it)->text(0);
				++it;
			}
		}

		if(!myNickStringList.isEmpty()) {
// 			cout << "use static nickList" << endl;

			QStringListIterator it(myNickStringList);
     			while (it.hasNext()) {
				QString next = it.next();
          			if (next.startsWith(myChatStringList.last(), Qt::CaseInsensitive) && myChatStringList.last() != "")
					matchStringList << next;
			}
			//TODO code for static nickList
		}
	}

// 	QStringList::const_iterator constIterator;
//      	for (constIterator = matchStringList.constBegin(); constIterator != matchStringList.constEnd(); ++constIterator) {
//          	cout << (*constIterator).toLocal8Bit().constData() << endl;
// 	}

	if(!matchStringList.isEmpty() || nickAutoCompletitionCounter > 0) {
		
		myChatStringList.removeLast();

// 		cout << nickAutoCompletitionCounter << endl;

		if(nickAutoCompletitionCounter == 0) {
		//first one
			lastChatString = myChatStringList.join(" ");
			lastMatchStringList = matchStringList;
		}

		if(nickAutoCompletitionCounter == lastMatchStringList.size()) nickAutoCompletitionCounter = 0;

// 		cout << nickAutoCompletitionCounter << "\n";
			
		if(lastChatString == "")	
			myLineEdit->setText(lastMatchStringList.at(nickAutoCompletitionCounter)+": ");
		else 
			myLineEdit->setText(lastChatString+" "+lastMatchStringList.at(nickAutoCompletitionCounter)+" ");
		
		nickAutoCompletitionCounter++;	
	}
}

void ChatTools::setChatTextEdited() {

	nickAutoCompletitionCounter = 0;
}
