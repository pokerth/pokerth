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
 *****************************************************************************/
#include "guilog.h"

#include "gametableimpl.h"
#include <handinterface.h>
#include <session.h>
#include <game.h>
#include <cardsvalue.h>
#include <game_defs.h>
#include "gametablestylereader.h"

using namespace std;

guiLog::guiLog(gameTableImpl* w, ConfigFile *c) : myW(w), myConfig(c), myLogDir(0), myHtmlLogFile(0)
{

	myW->setGuiLog(this);
	myStyle = myW->getMyGameTableStyle();

	myAppDataPath = QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str());

	connect(this, SIGNAL(signalLogPlayerActionMsg(QString, int, int)), this, SLOT(logPlayerActionMsg(QString, int, int)));
	connect(this, SIGNAL(signalLogNewGameHandMsg(int, int)), this, SLOT(logNewGameHandMsg(int, int)));
	connect(this, SIGNAL(signalLogNewBlindsSetsMsg(int, int, QString, QString)), this, SLOT(logNewBlindsSetsMsg(int, int, QString, QString)));
	connect(this, SIGNAL(signalLogPlayerWinsMsg(QString, int, bool)), this, SLOT(logPlayerWinsMsg(QString, int, bool)));
	connect(this, SIGNAL(signalLogPlayerSitsOut(QString)), this, SLOT(logPlayerSitsOut(QString)));
	connect(this, SIGNAL(signalLogDealBoardCardsMsg(int, int, int, int, int, int)), this, SLOT(logDealBoardCardsMsg(int, int, int, int, int, int)));
	connect(this, SIGNAL(signalLogFlipHoleCardsMsg(QString, int, int, int, QString)), this, SLOT(logFlipHoleCardsMsg(QString, int, int, int, QString)));
	connect(this, SIGNAL(signalLogPlayerLeftMsg(QString, int)), this, SLOT(logPlayerLeftMsg(QString, int)));
	connect(this, SIGNAL(signalLogPlayerJoinedMsg(QString)), this, SLOT(logPlayerJoinedMsg(QString)));
	connect(this, SIGNAL(signalLogNewGameAdminMsg(QString)), this, SLOT(logNewGameAdminMsg(QString)));
	connect(this, SIGNAL(signalLogPlayerWinGame(QString, int)), this, SLOT(logPlayerWinGame(QString, int)));
	connect(this, SIGNAL(signalFlushLogAtGame(int)), this, SLOT(flushLogAtGame(int)));
	connect(this, SIGNAL(signalFlushLogAtHand()), this, SLOT(flushLogAtHand()));


	logFileStreamString = "";
	lastGameID = 0;

	if(myConfig->readConfigInt("LogOnOff")) {
		//if write logfiles is enabled

		QDir logDir(QString::fromUtf8(myConfig->readConfigString("LogDir").c_str()));

		if(myConfig->readConfigString("LogDir") != "" && logDir.exists()) {

			int i;

			if(HTML_LOG) {

				myLogDir = new QDir(QString::fromUtf8(myConfig->readConfigString("LogDir").c_str()));
				QDateTime currentTime = QDateTime::currentDateTime();
				myHtmlLogFile = new QFile(myLogDir->absolutePath()+"/pokerth-log-"+currentTime.toString("yyyy-MM-dd_hh.mm.ss")+".html");

				//Logo-Pixmap extrahieren
				QPixmap logoChipPixmapFile(":/gfx/logoChip3D.png");
				logoChipPixmapFile.save(myLogDir->absolutePath()+"/logo.png");

//                myW->textBrowser_Log->append(myHtmlLogFile->fileName());

				// erstelle html-Datei
				myHtmlLogFile->open( QIODevice::WriteOnly );
				QTextStream stream( myHtmlLogFile );
				stream << "<html>\n";
				stream << "<head>\n";
				stream << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf8\">";
				stream << "</head>\n";
				stream << "<body style=\"font-size:smaller\">\n";
				stream << "<img src='logo.png'>\n";
				stream << QString("<h3><b>Log-File for PokerTH %1 Session started on ").arg(POKERTH_BETA_RELEASE_STRING)+QDate::currentDate().toString("yyyy-MM-dd")+" at "+QTime::currentTime().toString("hh:mm:ss")+"</b></h3>\n";
				myHtmlLogFile->close();

			}

			// delete old log files
			int daysUntilWaste = myConfig->readConfigInt("LogStoreDuration");

			QStringList filters("pokerth-log*");
			QStringList logFileList = myLogDir->entryList(filters, QDir::Files);

			for(i=0; i<logFileList.count(); i++) {

//                cout << logFileList.at(i).toStdString() << endl;

				QString dateString = logFileList.at(i);
				dateString.remove("pokerth-log-");
				dateString.remove(10,14);

				QDate dateOfFile(QDate::fromString(dateString, Qt::ISODate));
				QDate today(QDate::currentDate());

//                cout << dateOfFile.daysTo(today) << endl;

				if (dateOfFile.daysTo(today) > daysUntilWaste) {

					//                cout << QString::QString(myLogDir->absolutePath()+"/"+logFileList.at(i)).toStdString() << endl;
					QFile fileToDelete(myLogDir->absolutePath()+"/"+logFileList.at(i));
					fileToDelete.remove();
				}

			}

		} else {
			cout << "Log directory doesn't exist. Cannot create log files";
		}
	}
}

guiLog::~guiLog()
{
	delete myLogDir;
	delete myHtmlLogFile;

}

void guiLog::logPlayerActionMsg(QString msg, int action, int setValue)
{

	switch (action) {

	case 1:
		msg += " folds.";
		break;
	case 2:
		msg += " checks.";
		break;
	case 3:
		msg += " calls ";
		break;
	case 4:
		msg += " bets ";
		break;
	case 5:
		msg += " bets ";
		break;
	case 6:
		msg += " is all in with ";
		break;
	default:
		msg += "ERROR";
	}

	if (action >= 3) {
		msg += "$"+QString::number(setValue,10)+".";
	}

	myW->textBrowser_Log->append("<span style=\"color:#"+myStyle->getChatLogTextColor()+";\">"+msg+"</span>");

	if(myConfig->readConfigInt("LogOnOff")) {
		//if write logfiles is enabled

		if(HTML_LOG) {

			logFileStreamString += msg+"</br>\n";

			if(myConfig->readConfigInt("LogInterval") == 0) {
				writeLogFileStream(logFileStreamString);
				logFileStreamString = "";
			}

		}

	}
}

void guiLog::logNewGameHandMsg(int gameID, int handID)
{

	PlayerListConstIterator it_c;
	boost::shared_ptr<HandInterface> currentHand = myW->getSession()->getCurrentGame()->getCurrentHand();

	PlayerList activePlayerList = currentHand->getActivePlayerList();

	myW->textBrowser_Log->append("<span style=\"color:#"+myStyle->getChatLogTextColor()+"; font-size:large; font-weight:bold\">## Game: "+QString::number(gameID,10)+" | Hand: "+QString::number(handID,10)+" ##</span>");

	if(myConfig->readConfigInt("LogOnOff")) {
		//if write logfiles is enabled

		if(HTML_LOG) {

			logFileStreamString += "<table><tr><td width=\"600\" align=\"center\"><hr noshade size=\"3\"><b>Game: "+QString::number(gameID,10)+" | Hand: "+QString::number(handID,10)+"</b></td><td></td></tr></table>";
			logFileStreamString += "BLIND LEVEL: $"+QString::number(currentHand->getSmallBlind())+" / $"+QString::number(currentHand->getSmallBlind()*2)+"</br>";

			//print cash only for active players
			for(it_c=activePlayerList->begin(); it_c!=activePlayerList->end(); ++it_c) {

				logFileStreamString += "Seat " + QString::number((*it_c)->getMyID()+1,10) + ": <b>" +  QString::fromUtf8((*it_c)->getMyName().c_str()) + "</b> ($" + QString::number((*it_c)->getMyCash()+(*it_c)->getMySet(),10)+")</br>";

			}

		}

	}

}

void guiLog::logNewBlindsSetsMsg(int sbSet, int bbSet, QString sbName, QString bbName)
{

	// log blinds
	myW->textBrowser_Log->append("<span style=\"color:#"+myStyle->getChatLogTextColor()+";\">"+sbName+" posts small blind ($"+QString::number(sbSet,10)+")</span>");
	myW->textBrowser_Log->append("<span style=\"color:#"+myStyle->getChatLogTextColor()+";\">"+bbName+" posts big blind ($"+QString::number(bbSet,10)+")</span>");

	if(myConfig->readConfigInt("LogOnOff")) {
		//if write logfiles is enabled

		if(HTML_LOG) {

			logFileStreamString += "BLINDS: ";

			logFileStreamString += sbName+" ($"+QString::number(sbSet,10)+"), ";
			logFileStreamString += bbName+" ($"+QString::number(bbSet,10)+")";

			PlayerListConstIterator it_c;
			boost::shared_ptr<Game> currentGame = myW->getSession()->getCurrentGame();
			PlayerList activePlayerList = currentGame->getActivePlayerList();

			for(it_c=activePlayerList->begin(); it_c!=activePlayerList->end(); ++it_c) {

				if(activePlayerList->size() > 2) {
					if((*it_c)->getMyButton() == BUTTON_DEALER) {

						logFileStreamString += "</br>" + QString::fromUtf8((*it_c)->getMyName().c_str()) + " starts as dealer.";
						break;
					}
				} else {
					if((*it_c)->getMyButton() == BUTTON_SMALL_BLIND) {

						logFileStreamString += "</br>" + QString::fromUtf8((*it_c)->getMyName().c_str()) + " starts as dealer.";
						break;
					}
				}
			}

			logFileStreamString += "</br></br><b>PREFLOP</b>";
			logFileStreamString += "</br>\n";

		}

	}

}

void guiLog::logPlayerWinsMsg(QString playerName, int pot, bool main)
{

	if(main) {
		myW->textBrowser_Log->append("<span style=\"color:#"+myStyle->getLogWinnerMainPotColor()+";\">"+playerName+" wins $"+QString::number(pot,10)+"</span>");
	} else {
		myW->textBrowser_Log->append("<span style=\"color:#"+myStyle->getLogWinnerSidePotColor()+";\">"+playerName+" wins $"+QString::number(pot,10)+" (side pot)</span>");
	}

	if(myConfig->readConfigInt("LogOnOff")) {
		//if write logfiles is enabled

		logFileStreamString += "</br><i>"+playerName+" wins $"+QString::number(pot,10);
		if(!main) {
			logFileStreamString += " (side pot)";
		}
		logFileStreamString += "</i>\n";

		if(myConfig->readConfigInt("LogInterval") == 0) {
			writeLogFileStream(logFileStreamString);
			logFileStreamString = "";
		}
	}
}

void guiLog::logPlayerSitsOut(QString playerName)
{

	myW->textBrowser_Log->append("<i><span style=\"color:#"+myStyle->getLogPlayerSitsOutColor()+";\">"+playerName+" sits out</span></i>");

	if(myConfig->readConfigInt("LogOnOff")) {

		logFileStreamString += "</br><i><span style=\"font-size:smaller;\">"+playerName+" sits out</span></i>\n";

		if(myConfig->readConfigInt("LogInterval") == 0) {
			writeLogFileStream(logFileStreamString);
			logFileStreamString = "";
		}
	}
}

void guiLog::logDealBoardCardsMsg(int roundID, int card1, int card2, int card3, int card4, int card5)
{

	QString round;

	switch (roundID) {

	case 1:
		round = "Flop";
		myW->textBrowser_Log->append("<span style=\"color:#"+myStyle->getLogPlayerSitsOutColor()+";\">--- "+round+" --- "+"["+translateCardCode(card1).at(0)+translateCardCode(card1).at(1)+","+translateCardCode(card2).at(0)+translateCardCode(card2).at(1)+","+translateCardCode(card3).at(0)+translateCardCode(card3).at(1)+"]</span>");
		break;
	case 2:
		round = "Turn";
		myW->textBrowser_Log->append("<span style=\"color:#"+myStyle->getLogPlayerSitsOutColor()+";\">--- "+round+" --- "+"["+translateCardCode(card1).at(0)+translateCardCode(card1).at(1)+","+translateCardCode(card2).at(0)+translateCardCode(card2).at(1)+","+translateCardCode(card3).at(0)+translateCardCode(card3).at(1)+","+translateCardCode(card4).at(0)+translateCardCode(card4).at(1)+"]</span>");
		break;
	case 3:
		round = "River";
		myW->textBrowser_Log->append("<span style=\"color:#"+myStyle->getLogPlayerSitsOutColor()+";\">--- "+round+" --- "+"["+translateCardCode(card1).at(0)+translateCardCode(card1).at(1)+","+translateCardCode(card2).at(0)+translateCardCode(card2).at(1)+","+translateCardCode(card3).at(0)+translateCardCode(card3).at(1)+","+translateCardCode(card4).at(0)+translateCardCode(card4).at(1)+","+translateCardCode(card5).at(0)+translateCardCode(card5).at(1)+"]</span>");
		break;
	default:
		round = "ERROR";
	}

	if(myConfig->readConfigInt("LogOnOff")) {
		//if write logfiles is enabled

		if(HTML_LOG) {

			switch (roundID) {

			case 1:
				round = "Flop";
				logFileStreamString += "</br><b>"+round.toUpper()+"</b> [board cards <b>"+translateCardCode(card1).at(0)+"</b>"+translateCardCode(card1).at(1)+",<b>"+translateCardCode(card2).at(0)+"</b>"+translateCardCode(card2).at(1)+",<b>"+translateCardCode(card3).at(0)+"</b>"+translateCardCode(card3).at(1)+"]"+"</br>\n";
				break;
			case 2:
				round = "Turn";
				logFileStreamString += "</br><b>"+round.toUpper()+"</b> [board cards <b>"+translateCardCode(card1).at(0)+"</b>"+translateCardCode(card1).at(1)+",<b>"+translateCardCode(card2).at(0)+"</b>"+translateCardCode(card2).at(1)+",<b>"+translateCardCode(card3).at(0)+"</b>"+translateCardCode(card3).at(1)+",<b>"+translateCardCode(card4).at(0)+"</b>"+translateCardCode(card4).at(1)+"]"+"</br>\n";
				break;
			case 3:
				round = "River";
				logFileStreamString += "</br><b>"+round.toUpper()+"</b> [board cards <b>"+translateCardCode(card1).at(0)+"</b>"+translateCardCode(card1).at(1)+",<b>"+translateCardCode(card2).at(0)+"</b>"+translateCardCode(card2).at(1)+",<b>"+translateCardCode(card3).at(0)+"</b>"+translateCardCode(card3).at(1)+",<b>"+translateCardCode(card4).at(0)+"</b>"+translateCardCode(card4).at(1)+",<b>"+translateCardCode(card5).at(0)+"</b>"+translateCardCode(card5).at(1)+"]"+"</br>\n";
				break;
			default:
				round = "ERROR";
			}

			if(myConfig->readConfigInt("LogInterval") == 0) {
				writeLogFileStream(logFileStreamString);
				logFileStreamString = "";
			}

		}

	}
}

void guiLog::logFlipHoleCardsMsg(QString playerName, int card1, int card2, int cardsValueInt, QString showHas)
{

	QString tempHandName;

	if (cardsValueInt != -1) {

		tempHandName = CardsValue::determineHandName(cardsValueInt,myW->getSession()->getCurrentGame()->getActivePlayerList()).c_str();

		myW->textBrowser_Log->append("<span style=\"color:#"+myStyle->getChatLogTextColor()+";\">"+playerName+" "+showHas+" ["+translateCardCode(card1).at(0)+translateCardCode(card1).at(1)+","+translateCardCode(card2).at(0)+translateCardCode(card2).at(1)+"] - \""+tempHandName+"\"</span>");

	} else {

		myW->textBrowser_Log->append("<span style=\"color:#"+myStyle->getChatLogTextColor()+";\">"+playerName+" "+showHas+" ["+translateCardCode(card1).at(0)+translateCardCode(card1).at(1)+","+translateCardCode(card2).at(0)+translateCardCode(card2).at(1)+"]</span>");

	}

	if(myConfig->readConfigInt("LogOnOff")) {
		//if write logfiles is enabled

		if(HTML_LOG) {

			if (cardsValueInt != -1) {

				tempHandName.fromStdString(CardsValue::determineHandName(cardsValueInt,myW->getSession()->getCurrentGame()->getActivePlayerList()));

				logFileStreamString += playerName+" "+showHas+" [ <b>"+translateCardCode(card1).at(0)+"</b>"+translateCardCode(card1).at(1)+",<b>"+translateCardCode(card2).at(0)+"</b>"+translateCardCode(card2).at(1)+"] - "+tempHandName+"</br>\n";

				if(myConfig->readConfigInt("LogInterval") == 0) {
					writeLogFileStream(logFileStreamString);
					logFileStreamString = "";
				}

			} else {

				logFileStreamString += playerName+" "+showHas+" [<b>"+translateCardCode(card1).at(0)+"</b>"+translateCardCode(card1).at(1)+",<b>"+translateCardCode(card2).at(0)+"</b>"+translateCardCode(card2).at(1)+"]"+"</br>\n";
				if(myConfig->readConfigInt("LogInterval") == 0) {
					writeLogFileStream(logFileStreamString);
					logFileStreamString = "";
				}
			}
		}

	}

}

void guiLog::logPlayerLeftMsg(QString playerName, int wasKicked)
{

	QString action;
	if(wasKicked) action = "was kicked from";
	else action = "has left";

	myW->textBrowser_Log->append( "<span style=\"color:#"+myStyle->getChatLogTextColor()+";\"><i>"+playerName+" "+action+" the game!</i></span>");

	if(myConfig->readConfigInt("LogOnOff")) {

		logFileStreamString += "<i>"+playerName+" "+action+" the game!</i><br>\n";

		if(myConfig->readConfigInt("LogInterval") == 0) {
			writeLogFileStream(logFileStreamString);
			logFileStreamString = "";
		}
	}
}

void guiLog::logNewGameAdminMsg(QString playerName)
{

	myW->textBrowser_Log->append( "<i><span style=\"color:#"+myStyle->getLogNewGameAdminColor()+";\">"+playerName+" is game admin now!</span></i>");

	if(myConfig->readConfigInt("LogOnOff")) {

		logFileStreamString += "<i>"+playerName+" is game admin now!</i><br>\n";

		if(myConfig->readConfigInt("LogInterval") == 0) {
			writeLogFileStream(logFileStreamString);
			logFileStreamString = "";
		}
	}
}

void guiLog::logPlayerJoinedMsg(QString playerName)
{
	myW->textBrowser_Log->append("<span style=\"color:#"+myStyle->getChatLogTextColor()+";\"><i>"+playerName+" has joined the game!</i></span>");
}


void guiLog::logPlayerWinGame(QString playerName, int gameID)
{

	myW->textBrowser_Log->append( "<i><b>"+playerName+" wins game " + QString::number(gameID,10)  +"!</i></b><br>");

	if(myConfig->readConfigInt("LogOnOff")) {

		logFileStreamString += "</br></br><i><b>"+playerName+" wins game " + QString::number(gameID,10)  +"!</i></b></br>\n";

		if(myConfig->readConfigInt("LogInterval") == 0) {
			writeLogFileStream(logFileStreamString);
			logFileStreamString = "";
		}
	}

}

QStringList guiLog::translateCardCode(int cardCode)
{

	int value = cardCode%13;
	int color = cardCode/13;

	QStringList cardString;

	switch (value) {

	case 0:
		cardString << "2";
		break;
	case 1:
		cardString << "3";
		break;
	case 2:
		cardString << "4";
		break;
	case 3:
		cardString << "5";
		break;
	case 4:
		cardString << "6";
		break;
	case 5:
		cardString << "7";
		break;
	case 6:
		cardString << "8";
		break;
	case 7:
		cardString << "9";
		break;
	case 8:
		cardString << "T";
		break;
	case 9:
		cardString << "J";
		break;
	case 10:
		cardString << "Q";
		break;
	case 11:
		cardString << "K";
		break;
	case 12:
		cardString << "A";
		break;
	default:
		cardString << "ERROR";
	}

	switch (color) {

	case 0:
		cardString << "d";
		break;
	case 1:
		cardString << "h";
		break;
	case 2:
		cardString << "s";
		break;
	case 3:
		cardString << "c";
		break;
	default:
		cardString << "ERROR";
	}

	return cardString;
}

void guiLog::writeLogFileStream(QString streamString)
{

	if(myHtmlLogFile) {
		if(myHtmlLogFile->open( QIODevice::ReadWrite )) {
			QTextStream stream( myHtmlLogFile );
			stream.readAll();
			stream << streamString;
			myHtmlLogFile->close();
		} else {
			cout << "Could not open log-file to write log-messages!" << endl;
		}
	} else {
		cout << "Could not find log-file to write log-messages!" << endl;
	}
}

void guiLog::flushLogAtHand()
{

	if(myConfig->readConfigInt("LogOnOff")) {
		if(myConfig->readConfigInt("LogInterval") < 2) {
			// 	write for log after every action and after every hand
			writeLogFileStream(logFileStreamString);
			logFileStreamString = "";
		}
	}
}

void guiLog::flushLogAtGame(int gameID)
{

	if(myConfig->readConfigInt("LogOnOff")) {
		//	write for log after every game
		if(gameID > lastGameID) {
			writeLogFileStream(logFileStreamString);
			logFileStreamString = "";
			lastGameID = gameID;
		}
	}
}
