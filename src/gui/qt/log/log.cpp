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
	
	stream << "<p><b>####################&#160;&#160;&#160;Game: "+QString::number(gameID,10)+" | Hand: "+QString::number(handID,10)+"&#160;&#160;&#160;####################</b></br>";
	stream << "CASH: ";

	int k = 0;
	//Aktive Spieler zählen
	int activePlayersCounter = 0;
	for (k=0; k<myW->getMaxQuantityPlayers(); k++) { 
		if (myW->getActualHand()->getPlayerArray()[k]->getMyActiveStatus() == 1) activePlayersCounter++;
	}
	if(activePlayersCounter > 2) { 

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
	}
	else {
		for(i=0; i<myW->getActualHand()->getActualQuantityPlayers(); i++) {
	
			if(myW->getActualHand()->getPlayerArray()[i]->getMyButton() == 3) {
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

	stream << "</br></br><b>PREFLOP</b>";
	stream << "</br>\n";

	myLogFile->close();

	linesInFile = linesInFile++;

}

void Log::logPlayerWinsMsg(int playerID, int pot) {

	int i;

	myLogFile->open( QIODevice::ReadWrite );
	QTextStream stream( myLogFile );
	for(i=0; i<=linesInFile; i++) { stream.readLine(); }

// 	if (cardsValueInt != -1) {
// 		myW->textBrowser_Log->append(QString::fromStdString(myW->getActualHand()->getPlayerArray()[playerID]->getMyName())+" wins "+QString::number(pot,10)+"$ with "+translateCardsValueCode(cardsValueInt).at(0)+"!!! ");
// 		stream << "</br><i>"+QString::fromStdString(myW->getActualHand()->getPlayerArray()[playerID]->getMyName())+" wins "+QString::number(pot,10)+"$!!!</i></p>\n";
// 	}
// 	else {
	myW->textBrowser_Log->append(QString::fromStdString(myW->getActualHand()->getPlayerArray()[playerID]->getMyName())+" wins "+QString::number(pot,10)+"$!!! ");
	stream << "</br><i>"+QString::fromStdString(myW->getActualHand()->getPlayerArray()[playerID]->getMyName())+" wins "+QString::number(pot,10)+"$!!!</i></p>\n";
// 	}
	myLogFile->close();

	linesInFile++;
}

void Log::logDealBoardCardsMsg(int roundID, int card1, int card2, int card3, int card4, int card5) {  
	
	int i;

	myLogFile->open( QIODevice::ReadWrite );
	QTextStream stream( myLogFile );
	for(i=0; i<=linesInFile; i++) { stream.readLine(); }

	QString round;
	switch (roundID) {

		case 1: round = "Flop";
		myW->textBrowser_Log->append("--- "+round+" --- "+"["+translateCardCode(card1).at(0)+translateCardCode(card1).at(1)+","+translateCardCode(card2).at(0)+translateCardCode(card2).at(1)+","+translateCardCode(card3).at(0)+translateCardCode(card3).at(1)+"]");
		stream << "</br><b>"+round.toUpper()+"</b> [board cards <b>"+translateCardCode(card1).at(0)+"</b>"+translateCardCode(card1).at(1)+",<b>"+translateCardCode(card2).at(0)+"</b>"+translateCardCode(card2).at(1)+",<b>"+translateCardCode(card3).at(0)+"</b>"+translateCardCode(card3).at(1)+"]"+"</br>\n";
		break;
		case 2: round = "Turn";
		myW->textBrowser_Log->append("--- "+round+" --- "+"["+translateCardCode(card1).at(0)+translateCardCode(card1).at(1)+","+translateCardCode(card2).at(0)+translateCardCode(card2).at(1)+","+translateCardCode(card3).at(0)+translateCardCode(card3).at(1)+","+translateCardCode(card4).at(0)+translateCardCode(card4).at(1)+"]");
		stream << "</br><b>"+round.toUpper()+"</b> [board cards <b>"+translateCardCode(card1).at(0)+"</b>"+translateCardCode(card1).at(1)+",<b>"+translateCardCode(card2).at(0)+"</b>"+translateCardCode(card2).at(1)+",<b>"+translateCardCode(card3).at(0)+"</b>"+translateCardCode(card3).at(1)+",<b>"+translateCardCode(card4).at(0)+"</b>"+translateCardCode(card4).at(1)+"]"+"</br>\n";
		break;
		case 3: round = "River";
		myW->textBrowser_Log->append("--- "+round+" --- "+"["+translateCardCode(card1).at(0)+translateCardCode(card1).at(1)+","+translateCardCode(card2).at(0)+translateCardCode(card2).at(1)+","+translateCardCode(card3).at(0)+translateCardCode(card3).at(1)+","+translateCardCode(card4).at(0)+translateCardCode(card4).at(1)+","+translateCardCode(card5).at(0)+translateCardCode(card5).at(1)+"]");
		stream << "</br><b>"+round.toUpper()+"</b> [board cards <b>"+translateCardCode(card1).at(0)+"</b>"+translateCardCode(card1).at(1)+",<b>"+translateCardCode(card2).at(0)+"</b>"+translateCardCode(card2).at(1)+",<b>"+translateCardCode(card3).at(0)+"</b>"+translateCardCode(card3).at(1)+",<b>"+translateCardCode(card4).at(0)+"</b>"+translateCardCode(card4).at(1)+",<b>"+translateCardCode(card5).at(0)+"</b>"+translateCardCode(card5).at(1)+"]"+"</br>\n";
		break;
		default: round = "ERROR";
	}
	
	myLogFile->close();
	linesInFile++;
	
}

void Log::logFlipHoleCardsMsg(std::string playerName, int card1, int card2, int cardsValueInt) {

	int i;

	myLogFile->open( QIODevice::ReadWrite );
	QTextStream stream( myLogFile );
	for(i=0; i<=linesInFile; i++) { stream.readLine(); }

	if (cardsValueInt != -1) {
		myW->textBrowser_Log->append(QString::fromStdString(playerName)+" shows \""+translateCardsValueCode(cardsValueInt).at(0)+"\"");
		stream << QString::fromStdString(playerName)+" shows [ <b>"+translateCardCode(card1).at(0)+"</b>"+translateCardCode(card1).at(1)+",<b>"+translateCardCode(card2).at(0)+"</b>"+translateCardCode(card2).at(1)+"] - "+translateCardsValueCode(cardsValueInt).at(0)+"</br>\n";
	}
	else {
		myW->textBrowser_Log->append(QString::fromStdString(playerName)+" shows  "+"["+translateCardCode(card1).at(0)+translateCardCode(card1).at(1)+","+translateCardCode(card2).at(0)+translateCardCode(card2).at(1)+"]");
		stream << QString::fromStdString(playerName)+" shows [ <b>"+translateCardCode(card1).at(0)+"</b>"+translateCardCode(card1).at(1)+",<b>"+translateCardCode(card2).at(0)+"</b>"+translateCardCode(card2).at(1)+"]"+"</br>\n";

	}
	myLogFile->close();
	linesInFile++;
}

QStringList Log::translateCardCode(int cardCode) {

	int value = cardCode%13;
	int color = cardCode/13;
	
	QStringList cardString;
		
	switch (value) {
	
		case 0: cardString << "2";
		break;
		case 1: cardString << "3";
		break;
		case 2: cardString << "4";
		break;
		case 3: cardString << "5";
		break;
		case 4: cardString << "6";
		break;
		case 5: cardString << "7";
		break;
		case 6: cardString << "8";
		break;
		case 7: cardString << "9";
		break;
		case 8: cardString << "10";
		break;
		case 9: cardString << "J";
		break;
		case 10: cardString << "Q";
		break;
		case 11: cardString << "K";
		break;
		case 12: cardString << "A";
		break;
		default: cardString << "ERROR";
	}

	switch (color) {
	
		case 0: cardString << "d";
		break;
		case 1: cardString << "h";
		break;
		case 2: cardString << "s";
		break;
		case 3: cardString << "c";
		break;
		default: cardString << "ERROR";
	}
	
	return cardString;
}

QStringList Log::translateCardsValueCode(int cardsValueCode) {

	QStringList cardString;

	//erste Ziffer : Blattname
	int firstPart = cardsValueCode/100000000;
	//zweite und dritte Ziffer : Kicker, highest Card, usw.
	int secondPart = cardsValueCode/1000000 - firstPart*100;
	//vierte und fünfte Ziffer
	int thirdPart = cardsValueCode/10000 - firstPart*10000 - secondPart*100;
	// usw
	int fourthPart = cardsValueCode/100 - firstPart*1000000 - secondPart*10000 - thirdPart*100;
	//
	int fifthPart = cardsValueCode - firstPart*100000000 - secondPart*1000000 - thirdPart*10000 - fourthPart*100;
	// fuer highest Card
	int fifthPartA = cardsValueCode/10 - firstPart*10000000 - secondPart*100000 - thirdPart*1000 - fourthPart*10;
	int fifthPartB = cardsValueCode - firstPart*100000000 - secondPart*1000000 - thirdPart*10000 - fourthPart*100 - fifthPartA*10;


	switch (firstPart) {
		
		case 9: cardString << "Royal Flush";
		break;
		case 8: {
			cardString << "Straight Flush";
			switch(secondPart) {
				case 11: cardString << "King";
				break;
				case 10: cardString << "Queen";
				break;
				case 9: cardString << "Jack";
				break;
				case 8: cardString << "10";
				break;
				case 7: cardString << "9";
				break;
				case 6: cardString << "8";
				break;
				case 5: cardString << "7";
				break;
				case 4: cardString << "6";
				break;
				case 3: cardString << "5";
				break;
				default: cardString << "ERROR";
			}
		}
		break;
		case 7: {
			cardString << "Four of a Kind";
			switch(secondPart) {
				case 12: cardString << "Ace";
				break;
				case 11: cardString << "King";
				break;
				case 10: cardString << "Queen";
				break;
				case 9: cardString << "Jack";
				break;
				case 8: cardString << "10";
				break;
				case 7: cardString << "9";
				break;
				case 6: cardString << "8";
				break;
				case 5: cardString << "7";
				break;
				case 4: cardString << "6";
				break;
				case 3: cardString << "5";
				break;
				case 2: cardString << "4";
				break;
				case 1: cardString << "3";
				break;
				case 0: cardString << "2";
				break;
				default: cardString << "ERROR";
			}
		}
		break;
		case 6: {
			cardString << "Full House";
			//Drilling
			switch(secondPart) {
				case 12: cardString << "Ace";
				break;
				case 11: cardString << "King";
				break;
				case 10: cardString << "Queen";
				break;
				case 9: cardString << "Jack";
				break;
				case 8: cardString << "10";
				break;
				case 7: cardString << "9";
				break;
				case 6: cardString << "8";
				break;
				case 5: cardString << "7";
				break;
				case 4: cardString << "6";
				break;
				case 3: cardString << "5";
				break;
				case 2: cardString << "4";
				break;
				case 1: cardString << "3";
				break;
				case 0: cardString << "2";
				break;
				default: cardString << "ERROR";
			}
			//Pärchen
			switch(thirdPart) {
				case 12: cardString << "Ace";
				break;
				case 11: cardString << "King";
				break;
				case 10: cardString << "Queen";
				break;
				case 9: cardString << "Jack";
				break;
				case 8: cardString << "10";
				break;
				case 7: cardString << "9";
				break;
				case 6: cardString << "8";
				break;
				case 5: cardString << "7";
				break;
				case 4: cardString << "6";
				break;
				case 3: cardString << "5";
				break;
				case 2: cardString << "4";
				break;
				case 1: cardString << "3";
				break;
				case 0: cardString << "2";
				break;
				default: cardString << "ERROR";
			}
		}
		break;
		case 5: {
			cardString << "Flush";
			//highest Card
			switch(secondPart) {
				case 12: cardString << "Ace";
				break;
				case 11: cardString << "King";
				break;
				case 10: cardString << "Queen";
				break;
				case 9: cardString << "Jack";
				break;
				case 8: cardString << "10";
				break;
				case 7: cardString << "9";
				break;
				case 6: cardString << "8";
				break;
				case 5: cardString << "7";
				break;
				case 4: cardString << "6";
				break;
				default: cardString << "ERROR";
			}
		}
		break;
		case 4: {
			cardString << "Straight";
			switch(secondPart) {
				case 12: cardString << "Ace";
				break;
				case 11: cardString << "King";
				break;
				case 10: cardString << "Queen";
				break;
				case 9: cardString << "Jack";
				break;
				case 8: cardString << "10";
				break;
				case 7: cardString << "9";
				break;
				case 6: cardString << "8";
				break;
				case 5: cardString << "7";
				break;
				case 4: cardString << "6";
				break;
				case 3: cardString << "5";
				break;
				default: cardString << "ERROR";
			}
		}
		break;
		case 3: {
			cardString << "Three of a Kind";
			switch(secondPart) {
				case 12: cardString << "Ace";
				break;
				case 11: cardString << "King";
				break;
				case 10: cardString << "Queen";
				break;
				case 9: cardString << "Jack";
				break;
				case 8: cardString << "10";
				break;
				case 7: cardString << "9";
				break;
				case 6: cardString << "8";
				break;
				case 5: cardString << "7";
				break;
				case 4: cardString << "6";
				break;
				case 3: cardString << "5";
				break;
				case 2: cardString << "4";
				break;
				case 1: cardString << "3";
				break;
				case 0: cardString << "2";
				break;
				default: cardString << "ERROR";
			}
			//Kicker 1
			switch(thirdPart) {
				case 12: cardString << "Ace";
				break;
				case 11: cardString << "King";
				break;
				case 10: cardString << "Queen";
				break;
				case 9: cardString << "Jack";
				break;
				case 8: cardString << "10";
				break;
				case 7: cardString << "9";
				break;
				case 6: cardString << "8";
				break;
				case 5: cardString << "7";
				break;
				case 4: cardString << "6";
				break;
				case 3: cardString << "5";
				break;
				case 2: cardString << "4";
				break;
				case 1: cardString << "3";
				break;
				case 0: cardString << "2";
				break;
				default: cardString << "ERROR";
			}
			//Kicker 2
			switch(fourthPart) {
				case 12: cardString << "Ace";
				break;
				case 11: cardString << "King";
				break;
				case 10: cardString << "Queen";
				break;
				case 9: cardString << "Jack";
				break;
				case 8: cardString << "10";
				break;
				case 7: cardString << "9";
				break;
				case 6: cardString << "8";
				break;
				case 5: cardString << "7";
				break;
				case 4: cardString << "6";
				break;
				case 3: cardString << "5";
				break;
				case 2: cardString << "4";
				break;
				case 1: cardString << "3";
				break;
				case 0: cardString << "2";
				break;
				default: cardString << "ERROR";
			}
		}
		break;
		case 2: {
			cardString << "Two Pairs";
			// erster Pair
			switch(secondPart) {
				case 12: cardString << "Ace";
				break;
				case 11: cardString << "King";
				break;
				case 10: cardString << "Queen";
				break;
				case 9: cardString << "Jack";
				break;
				case 8: cardString << "10";
				break;
				case 7: cardString << "9";
				break;
				case 6: cardString << "8";
				break;
				case 5: cardString << "7";
				break;
				case 4: cardString << "6";
				break;
				case 3: cardString << "5";
				break;
				case 2: cardString << "4";
				break;
				case 1: cardString << "3";
				break;
				case 0: cardString << "2";
				break;
				default: cardString << "ERROR";
			}
			// zweiter Pair
			switch(thirdPart) {
				case 12: cardString << "Ace";
				break;
				case 11: cardString << "King";
				break;
				case 10: cardString << "Queen";
				break;
				case 9: cardString << "Jack";
				break;
				case 8: cardString << "10";
				break;
				case 7: cardString << "9";
				break;
				case 6: cardString << "8";
				break;
				case 5: cardString << "7";
				break;
				case 4: cardString << "6";
				break;
				case 3: cardString << "5";
				break;
				case 2: cardString << "4";
				break;
				case 1: cardString << "3";
				break;
				case 0: cardString << "2";
				break;
				default: cardString << "ERROR";
			}
			//Kicker
			switch(fourthPart) {
				case 12: cardString << "Ace";
				break;
				case 11: cardString << "King";
				break;
				case 10: cardString << "Queen";
				break;
				case 9: cardString << "Jack";
				break;
				case 8: cardString << "10";
				break;
				case 7: cardString << "9";
				break;
				case 6: cardString << "8";
				break;
				case 5: cardString << "7";
				break;
				case 4: cardString << "6";
				break;
				case 3: cardString << "5";
				break;
				case 2: cardString << "4";
				break;
				case 1: cardString << "3";
				break;
				case 0: cardString << "2";
				break;
				default: cardString << "ERROR";
			}
		}
		break;
		case 1: {
			cardString << "Pair";
			// Pair
			switch(secondPart) {
				case 12: cardString << "Ace";
				break;
				case 11: cardString << "King";
				break;
				case 10: cardString << "Queen";
				break;
				case 9: cardString << "Jack";
				break;
				case 8: cardString << "10";
				break;
				case 7: cardString << "9";
				break;
				case 6: cardString << "8";
				break;
				case 5: cardString << "7";
				break;
				case 4: cardString << "6";
				break;
				case 3: cardString << "5";
				break;
				case 2: cardString << "4";
				break;
				case 1: cardString << "3";
				break;
				case 0: cardString << "2";
				break;
				default: cardString << "ERROR";
			}
			// Kicker 1
			switch(thirdPart) {
				case 12: cardString << "Ace";
				break;
				case 11: cardString << "King";
				break;
				case 10: cardString << "Queen";
				break;
				case 9: cardString << "Jack";
				break;
				case 8: cardString << "10";
				break;
				case 7: cardString << "9";
				break;
				case 6: cardString << "8";
				break;
				case 5: cardString << "7";
				break;
				case 4: cardString << "6";
				break;
				case 3: cardString << "5";
				break;
				case 2: cardString << "4";
				break;
				case 1: cardString << "3";
				break;
				case 0: cardString << "2";
				break;
				default: cardString << "ERROR";
			}
			//Kicker 2
			switch(fourthPart) {
				case 12: cardString << "Ace";
				break;
				case 11: cardString << "King";
				break;
				case 10: cardString << "Queen";
				break;
				case 9: cardString << "Jack";
				break;
				case 8: cardString << "10";
				break;
				case 7: cardString << "9";
				break;
				case 6: cardString << "8";
				break;
				case 5: cardString << "7";
				break;
				case 4: cardString << "6";
				break;
				case 3: cardString << "5";
				break;
				case 2: cardString << "4";
				break;
				case 1: cardString << "3";
				break;
				case 0: cardString << "2";
				break;
				default: cardString << "ERROR";
			}
			//Kicker 3
			switch(fifthPart) {
				case 12: cardString << "Ace";
				break;
				case 11: cardString << "King";
				break;
				case 10: cardString << "Queen";
				break;
				case 9: cardString << "Jack";
				break;
				case 8: cardString << "10";
				break;
				case 7: cardString << "9";
				break;
				case 6: cardString << "8";
				break;
				case 5: cardString << "7";
				break;
				case 4: cardString << "6";
				break;
				case 3: cardString << "5";
				break;
				case 2: cardString << "4";
				break;
				case 1: cardString << "3";
				break;
				case 0: cardString << "2";
				break;
				default: cardString << "ERROR";
			}
		}
		break;
		case 0:  {
			cardString << "Highest Card";
			// Kicker 1
			switch(secondPart) {
				case 12: cardString << "Ace";
				break;
				case 11: cardString << "King";
				break;
				case 10: cardString << "Queen";
				break;
				case 9: cardString << "Jack";
				break;
				case 8: cardString << "10";
				break;
				case 7: cardString << "9";
				break;
				case 6: cardString << "8";
				break;
				case 5: cardString << "7";
				break;
				case 4: cardString << "6";
				break;
				case 3: cardString << "5";
				break;
				case 2: cardString << "4";
				break;
				case 1: cardString << "3";
				break;
				case 0: cardString << "2";
				break;
				default: cardString << "ERROR";
			}
			// Kicker 2
			switch(thirdPart) {
				case 12: cardString << "Ace";
				break;
				case 11: cardString << "King";
				break;
				case 10: cardString << "Queen";
				break;
				case 9: cardString << "Jack";
				break;
				case 8: cardString << "10";
				break;
				case 7: cardString << "9";
				break;
				case 6: cardString << "8";
				break;
				case 5: cardString << "7";
				break;
				case 4: cardString << "6";
				break;
				case 3: cardString << "5";
				break;
				case 2: cardString << "4";
				break;
				case 1: cardString << "3";
				break;
				case 0: cardString << "2";
				break;
				default: cardString << "ERROR";
			}
			//Kicker 3
			switch(fourthPart) {
				case 12: cardString << "Ace";
				break;
				case 11: cardString << "King";
				break;
				case 10: cardString << "Queen";
				break;
				case 9: cardString << "Jack";
				break;
				case 8: cardString << "10";
				break;
				case 7: cardString << "9";
				break;
				case 6: cardString << "8";
				break;
				case 5: cardString << "7";
				break;
				case 4: cardString << "6";
				break;
				case 3: cardString << "5";
				break;
				case 2: cardString << "4";
				break;
				case 1: cardString << "3";
				break;
				case 0: cardString << "2";
				break;
				default: cardString << "ERROR";
			}
			//Kicker 4
			switch(fifthPartA) {
				case 9: cardString << "Jack";
				break;
				case 8: cardString << "10";
				break;
				case 7: cardString << "9";
				break;
				case 6: cardString << "8";
				break;
				case 5: cardString << "7";
				break;
				case 4: cardString << "6";
				break;
				case 3: cardString << "5";
				break;
				case 2: cardString << "4";
				break;
				case 1: cardString << "3";
				break;
				case 0: cardString << "2";
				break;
				default: cardString << "ERROR";
			}
			//Kicker 5
			switch(fifthPartB) {
				case 9: cardString << "Jack";
				break;
				case 8: cardString << "10";
				break;
				case 7: cardString << "9";
				break;
				case 6: cardString << "8";
				break;
				case 5: cardString << "7";
				break;
				case 4: cardString << "6";
				break;
				case 3: cardString << "5";
				break;
				case 2: cardString << "4";
				break;
				case 1: cardString << "3";
				break;
				case 0: cardString << "2";
				break;
				default: cardString << "ERROR";
			}
		}
		break;
		default: cardString << "ERROR";

	}



	return cardString;

}
