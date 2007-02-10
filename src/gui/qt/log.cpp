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
#include "log.h"

#include "mainwindowimpl.h"

using namespace std;

Log::Log(mainWindowImpl* w) : myW(w)
{
	myW->setLog(this);

	myConfig = new ConfigFile;
	if(myConfig->readConfigString("LogDir") != "" && QDir::QDir(QString::fromStdString(myConfig->readConfigString("LogDir"))).exists()) { 

#ifdef _WIN32
		myLogDir = new QDir(QString::fromStdString(myConfig->readConfigString("LogDir")));
#else
		myLogDir = new QDir(QString::fromStdString(myConfig->readConfigString("LogDir")));
#endif
		myLogFile = new QFile(myLogDir->absolutePath()+"/pokerth-log-"+QDateTime::currentDateTime().toString("yyyy-MM-dd_hh.mm.ss")+".html");

		//Logo-Pixmap extrahieren
		QPixmap::QPixmap(":graphics/graphics/logo-140-100.png").save(myLogDir->absolutePath()+"/logo.png");

// 		myW->textBrowser_Log->append(myLogFile->fileName());

		myLogFile->open( QIODevice::WriteOnly );
		QTextStream stream( myLogFile );
		stream << "<html>\n";
		stream << "<body>\n";
		stream << "<img src='logo.png'>\n";
		stream << "<h3><b>Log-File for PokerTH Session started on "+QDate::currentDate().toString("yyyy-MM-dd")+" at "+QTime::currentTime().toString("hh:mm:ss")+"</b></h3>\n";
// 		stream << "</body>\n";
// 		stream << "</html>\n";
		myLogFile->close();

		linesInFile = 3;
	} 

	//Zu alte Dateien löschen!!!
	int daysUntilWaste = myConfig->readConfigInt("LogStoreDuration");
	int i;
		
	QStringList filters("pokerth-log*");
	QStringList logFileList = myLogDir->entryList(filters, QDir::Files);
	
	for(i=0; i<logFileList.count(); i++) {

// 		cout << logFileList.at(i).toStdString() << endl;

		QString dateString = logFileList.at(i);
		dateString.remove("pokerth-log-");
		dateString.remove(10,14);
		
		QDate dateOfFile(QDate::fromString(dateString, Qt::ISODate));
		QDate today(QDate::currentDate());
		
// 		cout << dateOfFile.daysTo(today) << endl;

		if (dateOfFile.daysTo(today) > daysUntilWaste) {

// 			cout << QString::QString(myLogDir->absolutePath()+"/"+logFileList.at(i)).toStdString() << endl;
			QFile fileToDelete(myLogDir->absolutePath()+"/"+logFileList.at(i));
			fileToDelete.remove();
		}

	}
	
}

Log::~Log()
{
}

void Log::logPlayerActionMsg(string playerName, int action, int setValue) {

	int i;	

	QString msg;
	msg = QString::fromStdString(playerName);
	
	switch (action) {

		case 1: msg += " folds.";
		break;
		case 2: msg += " checks.";
		break;
		case 3: msg += " calls ";
		break;
		case 4: msg += " bets ";
		break;
		case 5: msg += " sets ";
		break;
		case 6: msg += " is all in with ";
		break;
		default: msg += "ERROR";
	}
	
	if (action >= 3) { msg += QString::number(setValue,10)+"$."; }
	
	myW->textBrowser_Log->append(msg);

	myLogFile->open( QIODevice::ReadWrite );
	QTextStream stream( myLogFile );
	for(i=0; i<=linesInFile; i++) { stream.readLine(); }
	stream << msg+"</br>\n";
// 	stream << "</body>\n";
// 	stream << "</html>\n";
	myLogFile->close();

	linesInFile++;

}

void Log::logNewGameHandMsg(int gameID, int handID) {

	int i;

	myW->textBrowser_Log->append("<b>## Game: "+QString::number(gameID,10)+" | Hand: "+QString::number(handID,10)+" ##</b>");
	
	myLogFile->open( QIODevice::ReadWrite );
	QTextStream stream( myLogFile );
	for(i=0; i<=linesInFile; i++) { stream.readLine(); }
	
	stream << "<p><b>##### Game: "+QString::number(gameID,10)+" | Hand: "+QString::number(handID,10)+" #####</b></br>" << "\n";
	stream << "CASH: ";
	for(i=0; i<myW->getActualHand()->getActualQuantityPlayers(); i++) {

		if(myW->getActualHand()->getPlayerArray()[i]->getMyButton() == 1) {
			if(i == myW->getActualHand()->getActualQuantityPlayers()-1) {
				stream << QString::fromStdString(myW->getActualHand()->getPlayerArray()[i]->getMyName())+" (Dealer): "+QString::number(myW->getActualHand()->getPlayerArray()[i]->getMyCash(),10)+"$";
			}
			else {
				stream << QString::fromStdString(myW->getActualHand()->getPlayerArray()[i]->getMyName())+" (Dealer): "+QString::number(myW->getActualHand()->getPlayerArray()[i]->getMyCash(),10)+"$, ";
			}		
		}
		else {
			if(i == myW->getActualHand()->getActualQuantityPlayers()-1) {
				stream << QString::fromStdString(myW->getActualHand()->getPlayerArray()[i]->getMyName())+": "+QString::number(myW->getActualHand()->getPlayerArray()[i]->getMyCash(),10)+"$";
			}
			else {
				stream << QString::fromStdString(myW->getActualHand()->getPlayerArray()[i]->getMyName())+": "+QString::number(myW->getActualHand()->getPlayerArray()[i]->getMyCash(),10)+"$, ";
			}
		}
	}
	stream << "</br>BLINDS: ";
	for(i=0; i<myW->getMaxQuantityPlayers(); i++) {
	
		int j,k = 0;
		//Aktive Spieler zählen
		int activePlayersCounter = 0;
		for (k=0; k<myW->getMaxQuantityPlayers(); k++) { 
			if (myW->getActualHand()->getPlayerArray()[k]->getMyAction() != 1 && myW->getActualHand()->getPlayerArray()[k]->getMyActiveStatus() == 1) activePlayersCounter++;
		}
		if(activePlayersCounter < 3) { j=1; }
		
// 		cout << activePlayersCounter << endl;
// 		cout << (i+myW->getActualHand()->getDealerPosition()+j)%5 << endl;

		switch (myW->getActualHand()->getPlayerArray()[(i+myW->getActualHand()->getDealerPosition()+j)%5]->getMyButton()) {
			case 2 : stream << QString::fromStdString(myW->getActualHand()->getPlayerArray()[(i+myW->getActualHand()->getDealerPosition()+j)%5]->getMyName())+" ("+QString::number(myW->getActualHand()->getPlayerArray()[(i+myW->getActualHand()->getDealerPosition()+j)%5]->getMySet(),10)+"$), ";
			break;
			case 3 : stream << QString::fromStdString(myW->getActualHand()->getPlayerArray()[(i+myW->getActualHand()->getDealerPosition()+j)%5]->getMyName())+" ("+QString::number(myW->getActualHand()->getPlayerArray()[(i+myW->getActualHand()->getDealerPosition()+j)%5]->getMySet(),10)+"$)";	
			break;
			default :;	
		}
	}
	stream << "</p>\n";

	myLogFile->close();

	linesInFile = linesInFile+3;

}

void Log::logPlayerWinsMsg(int playerID) {

	int i;

	myW->textBrowser_Log->append(QString::fromStdString(myW->getActualHand()->getPlayerArray()[playerID]->getMyName())+" wins!!! ");
	
	myLogFile->open( QIODevice::ReadWrite );
	QTextStream stream( myLogFile );
	for(i=0; i<=linesInFile; i++) { stream.readLine(); }
	
	QString msg("<p><i>"+QString::fromStdString(myW->getActualHand()->getPlayerArray()[playerID]->getMyName())+" wins!!!</i></p>\n");
	stream << msg;

	myLogFile->close();

	linesInFile++;
	linesInFile++;

}
