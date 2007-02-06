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

		myLogDir = new QDir(QString::fromStdString(myConfig->readConfigString("LogDir")));
#ifdef _WIN32
// 		QString temp = myLogDir->absolutePath()+"\\pokerth-log-"+QDateTime::currentDateTime().toString("yyyy-MM-dd_hh:mm:ss")+".html";
// 		cout << temp.toStdString() << endl;
		myLogFile = new QFile(myLogDir->absolutePath()+"pokerth-log-"+QDateTime::currentDateTime().toString("yyyy-MM-dd_hh:mm:ss")+".html");
#else 
		myLogFile = new QFile(myLogDir->absolutePath()+"/pokerth-log-"+QDateTime::currentDateTime().toString("yyyy-MM-dd_hh:mm:ss")+".html");
#endif

		//Logo-Pixmap extrahieren
		QPixmap::QPixmap(":graphics/graphics/logo-140-100.png").save(myLogDir->absolutePath()+"/logo.png");

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

	myW->textBrowser_Log->append("## Game: "+QString::number(gameID,10)+" | Hand: "+QString::number(handID,10)+" ##");
	
	myLogFile->open( QIODevice::ReadWrite );
	QTextStream stream( myLogFile );
	for(i=0; i<=linesInFile; i++) { stream.readLine(); }
	
	QString msg("<p><b>##### Game: "+QString::number(gameID,10)+" | Hand: "+QString::number(handID,10)+" #####</b></br>");
	stream << msg << "\n";
	
	for(i=0; i<myW->getMaxQuantityPlayers(); i++) {
		if(myW->getActualHand()->getPlayerArray()[i]->getMyButton() == 1) {
			stream << QString::fromStdString(myW->getActualHand()->getPlayerArray()[i]->getMyName())+" (Dealer): "+QString::number(myW->getActualHand()->getPlayerArray()[i]->getMyCash(),10)+"$, ";	
		}
		else {
			stream << QString::fromStdString(myW->getActualHand()->getPlayerArray()[i]->getMyName())+": "+QString::number(myW->getActualHand()->getPlayerArray()[i]->getMyCash(),10)+"$, ";	
		}
	}
	stream << "</p>\n";
// 	stream << "</body>\n";
// 	stream << "</html>\n";
	myLogFile->close();

	linesInFile++;
	linesInFile++;

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
