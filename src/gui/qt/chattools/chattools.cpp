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
#include <iostream>

using namespace std;


ChatTools::ChatTools(QLineEdit* l, QTreeWidget *t) : nickAutoCompletitionCounter(0), myLineEdit(l), myNickTreeWidget(t), myNickStringList(NULL)
{

}

ChatTools::~ChatTools()
{

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

		cout << nickAutoCompletitionCounter << endl;

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
