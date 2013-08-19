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
#include "guilog.h"

#include "gametableimpl.h"
#include <handinterface.h>
#include <session.h>
#include <game.h>
#include <cardsvalue.h>
#include <game_defs.h>
#include "gametablestylereader.h"

using namespace std;

guiLog::guiLog(gameTableImpl* w, ConfigFile *c) : myW(w), myConfig(c), myLogDir(0), myHtmlLogFile(0), myHtmlLogFile_old(0), myTxtLogFile(0), tb(0)
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
	connect(this, SIGNAL(signalLogSpectatorLeftMsg(QString, int)), this, SLOT(logSpectatorLeftMsg(QString, int)));
	connect(this, SIGNAL(signalLogSpectatorJoinedMsg(QString)), this, SLOT(logSpectatorJoinedMsg(QString)));
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

			myLogDir = new QDir(QString::fromUtf8(myConfig->readConfigString("LogDir").c_str()));

			if(HTML_LOG) {

				QDateTime currentTime = QDateTime::currentDateTime();
				if(SQLITE_LOG) {
					myHtmlLogFile_old = new QFile(myLogDir->absolutePath()+"/pokerth-log-"+currentTime.toString("yyyy-MM-dd_hh.mm.ss")+"_old.html");
				} else {
					myHtmlLogFile_old = new QFile(myLogDir->absolutePath()+"/pokerth-log-"+currentTime.toString("yyyy-MM-dd_hh.mm.ss")+".html");
				}

				//Logo-Pixmap extrahieren
				QPixmap logoChipPixmapFile(":/gfx/logoChip3D.png");
				logoChipPixmapFile.save(myLogDir->absolutePath()+"/logo.png");

//                myW->textBrowser_Log->append(myHtmlLogFile_old->fileName());

				// erstelle html-Datei
				myHtmlLogFile_old->open( QIODevice::WriteOnly );
				QTextStream stream_old( myHtmlLogFile_old );
				stream_old << "<html>\n";
				stream_old << "<head>\n";
				stream_old << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf8\">";
				stream_old << "</head>\n";
#ifdef GUI_800x480
				stream_old << "<body style=\"font-size:14px\">\n";
#else
				stream_old << "<body>\n";
#endif
				stream_old << "<img src='logo.png'>\n";
				stream_old << QString("<h3><b>Log-File for PokerTH %1 Session started on ").arg(POKERTH_BETA_RELEASE_STRING)+QDate::currentDate().toString("yyyy-MM-dd")+" at "+QTime::currentTime().toString("hh:mm:ss")+"</b></h3>\n";
				myHtmlLogFile_old->close();

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
	delete myHtmlLogFile_old;

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

#ifdef GUI_800x480
	myW->tabs.textBrowser_Log->append("<span style=\"color:#"+myStyle->getChatLogTextColor()+";\">"+msg+"</span>");
#else
	myW->textBrowser_Log->append("<span style=\"color:#"+myStyle->getChatLogTextColor()+";\">"+msg+"</span>");
#endif


	if(HTML_LOG) {

		if(myConfig->readConfigInt("LogOnOff")) {
			//if write logfiles is enabled

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

#ifdef GUI_800x480
	myW->tabs.textBrowser_Log->append("<span style=\"color:#"+myStyle->getChatLogTextColor()+"; font-size:large; font-weight:bold\">## Game: "+QString::number(gameID,10)+" | Hand: "+QString::number(handID,10)+" ##</span>");
#else
	myW->textBrowser_Log->append("<span style=\"color:#"+myStyle->getChatLogTextColor()+"; font-size:large; font-weight:bold\">## Game: "+QString::number(gameID,10)+" | Hand: "+QString::number(handID,10)+" ##</span>");
#endif

	if(HTML_LOG) {

		if(myConfig->readConfigInt("LogOnOff")) {
			//if write logfiles is enabled

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
#ifdef GUI_800x480
	myW->tabs.textBrowser_Log->append("<span style=\"color:#"+myStyle->getChatLogTextColor()+";\">"+sbName+" posts small blind ($"+QString::number(sbSet,10)+")</span>");
	myW->tabs.textBrowser_Log->append("<span style=\"color:#"+myStyle->getChatLogTextColor()+";\">"+bbName+" posts big blind ($"+QString::number(bbSet,10)+")</span>");
#else
	myW->textBrowser_Log->append("<span style=\"color:#"+myStyle->getChatLogTextColor()+";\">"+sbName+" posts small blind ($"+QString::number(sbSet,10)+")</span>");
	myW->textBrowser_Log->append("<span style=\"color:#"+myStyle->getChatLogTextColor()+";\">"+bbName+" posts big blind ($"+QString::number(bbSet,10)+")</span>");
#endif

	if(HTML_LOG) {

		if(myConfig->readConfigInt("LogOnOff")) {
			//if write logfiles is enabled

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

#ifdef GUI_800x480
	if(main) {
		myW->tabs.textBrowser_Log->append("<span style=\"color:#"+myStyle->getLogWinnerMainPotColor()+";\">"+playerName+" wins $"+QString::number(pot,10)+"</span>");
	} else {
		myW->tabs.textBrowser_Log->append("<span style=\"color:#"+myStyle->getLogWinnerSidePotColor()+";\">"+playerName+" wins $"+QString::number(pot,10)+" (side pot)</span>");
	}
#else
	if(main) {
		myW->textBrowser_Log->append("<span style=\"color:#"+myStyle->getLogWinnerMainPotColor()+";\">"+playerName+" wins $"+QString::number(pot,10)+"</span>");
	} else {
		myW->textBrowser_Log->append("<span style=\"color:#"+myStyle->getLogWinnerSidePotColor()+";\">"+playerName+" wins $"+QString::number(pot,10)+" (side pot)</span>");
	}
#endif

	if(HTML_LOG) {

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
}

void guiLog::logPlayerSitsOut(QString playerName)
{

#ifdef GUI_800x480
	myW->tabs.textBrowser_Log->append("<i><span style=\"color:#"+myStyle->getLogPlayerSitsOutColor()+";\">"+playerName+" sits out</span></i>");
#else
	myW->textBrowser_Log->append("<i><span style=\"color:#"+myStyle->getLogPlayerSitsOutColor()+";\">"+playerName+" sits out</span></i>");
#endif

	if(HTML_LOG) {

		if(myConfig->readConfigInt("LogOnOff")) {

			logFileStreamString += "</br><i><span style=\"font-size:smaller;\">"+playerName+" sits out</span></i>\n";

			if(myConfig->readConfigInt("LogInterval") == 0) {
				writeLogFileStream(logFileStreamString);
				logFileStreamString = "";
			}
		}

	}
}

void guiLog::logDealBoardCardsMsg(int roundID, int card1, int card2, int card3, int card4, int card5)
{

	QString round;

	switch (roundID) {

	case 1:
		round = "Flop";
#ifdef GUI_800x480
		myW->tabs.textBrowser_Log->append("<span style=\"color:#"+myStyle->getLogPlayerSitsOutColor()+";\">--- "+round+" --- "+"["+translateCardCode(card1).at(0)+translateCardCode(card1).at(1)+","+translateCardCode(card2).at(0)+translateCardCode(card2).at(1)+","+translateCardCode(card3).at(0)+translateCardCode(card3).at(1)+"]</span>");
#else
		myW->textBrowser_Log->append("<span style=\"color:#"+myStyle->getLogPlayerSitsOutColor()+";\">--- "+round+" --- "+"["+translateCardCode(card1).at(0)+translateCardCode(card1).at(1)+","+translateCardCode(card2).at(0)+translateCardCode(card2).at(1)+","+translateCardCode(card3).at(0)+translateCardCode(card3).at(1)+"]</span>");
#endif
		break;
	case 2:
		round = "Turn";
#ifdef GUI_800x480
		myW->tabs.textBrowser_Log->append("<span style=\"color:#"+myStyle->getLogPlayerSitsOutColor()+";\">--- "+round+" --- "+"["+translateCardCode(card1).at(0)+translateCardCode(card1).at(1)+","+translateCardCode(card2).at(0)+translateCardCode(card2).at(1)+","+translateCardCode(card3).at(0)+translateCardCode(card3).at(1)+","+translateCardCode(card4).at(0)+translateCardCode(card4).at(1)+"]</span>");
#else
		myW->textBrowser_Log->append("<span style=\"color:#"+myStyle->getLogPlayerSitsOutColor()+";\">--- "+round+" --- "+"["+translateCardCode(card1).at(0)+translateCardCode(card1).at(1)+","+translateCardCode(card2).at(0)+translateCardCode(card2).at(1)+","+translateCardCode(card3).at(0)+translateCardCode(card3).at(1)+","+translateCardCode(card4).at(0)+translateCardCode(card4).at(1)+"]</span>");
#endif
		break;
	case 3:
		round = "River";
#ifdef GUI_800x480
		myW->tabs.textBrowser_Log->append("<span style=\"color:#"+myStyle->getLogPlayerSitsOutColor()+";\">--- "+round+" --- "+"["+translateCardCode(card1).at(0)+translateCardCode(card1).at(1)+","+translateCardCode(card2).at(0)+translateCardCode(card2).at(1)+","+translateCardCode(card3).at(0)+translateCardCode(card3).at(1)+","+translateCardCode(card4).at(0)+translateCardCode(card4).at(1)+","+translateCardCode(card5).at(0)+translateCardCode(card5).at(1)+"]</span>");
#else
		myW->textBrowser_Log->append("<span style=\"color:#"+myStyle->getLogPlayerSitsOutColor()+";\">--- "+round+" --- "+"["+translateCardCode(card1).at(0)+translateCardCode(card1).at(1)+","+translateCardCode(card2).at(0)+translateCardCode(card2).at(1)+","+translateCardCode(card3).at(0)+translateCardCode(card3).at(1)+","+translateCardCode(card4).at(0)+translateCardCode(card4).at(1)+","+translateCardCode(card5).at(0)+translateCardCode(card5).at(1)+"]</span>");
#endif
		break;
	default:
		round = "ERROR";
	}

	if(HTML_LOG) {

		if(myConfig->readConfigInt("LogOnOff")) {
			//if write logfiles is enabled

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

#ifdef GUI_800x480
		myW->tabs.textBrowser_Log->append("<span style=\"color:#"+myStyle->getChatLogTextColor()+";\">"+playerName+" "+showHas+" ["+translateCardCode(card1).at(0)+translateCardCode(card1).at(1)+","+translateCardCode(card2).at(0)+translateCardCode(card2).at(1)+"] - \""+tempHandName+"\"</span>");
#else
		myW->textBrowser_Log->append("<span style=\"color:#"+myStyle->getChatLogTextColor()+";\">"+playerName+" "+showHas+" ["+translateCardCode(card1).at(0)+translateCardCode(card1).at(1)+","+translateCardCode(card2).at(0)+translateCardCode(card2).at(1)+"] - \""+tempHandName+"\"</span>");
#endif

	} else {
#ifdef GUI_800x480
		myW->tabs.textBrowser_Log->append("<span style=\"color:#"+myStyle->getChatLogTextColor()+";\">"+playerName+" "+showHas+" ["+translateCardCode(card1).at(0)+translateCardCode(card1).at(1)+","+translateCardCode(card2).at(0)+translateCardCode(card2).at(1)+"]</span>");
#else
		myW->textBrowser_Log->append("<span style=\"color:#"+myStyle->getChatLogTextColor()+";\">"+playerName+" "+showHas+" ["+translateCardCode(card1).at(0)+translateCardCode(card1).at(1)+","+translateCardCode(card2).at(0)+translateCardCode(card2).at(1)+"]</span>");
#endif
	}

	if(HTML_LOG) {

		if(myConfig->readConfigInt("LogOnOff")) {
			//if write logfiles is enabled

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

#ifdef GUI_800x480
	myW->tabs.textBrowser_Log->append( "<span style=\"color:#"+myStyle->getChatLogTextColor()+";\"><i>"+playerName+" "+action+" the game!</i></span>");
#else
	myW->textBrowser_Log->append( "<span style=\"color:#"+myStyle->getChatLogTextColor()+";\"><i>"+playerName+" "+action+" the game!</i></span>");
#endif

	if(HTML_LOG) {

		if(myConfig->readConfigInt("LogOnOff")) {

			logFileStreamString += "<i>"+playerName+" "+action+" the game!</i><br>\n";

			if(myConfig->readConfigInt("LogInterval") == 0) {
				writeLogFileStream(logFileStreamString);
				logFileStreamString = "";
			}
		}

	}
}

void guiLog::logNewGameAdminMsg(QString playerName)
{

#ifdef GUI_800x480
	myW->tabs.textBrowser_Log->append( "<i><span style=\"color:#"+myStyle->getLogNewGameAdminColor()+";\">"+playerName+" is game admin now!</span></i>");
#else
	myW->textBrowser_Log->append( "<i><span style=\"color:#"+myStyle->getLogNewGameAdminColor()+";\">"+playerName+" is game admin now!</span></i>");
#endif

	if(HTML_LOG) {

		if(myConfig->readConfigInt("LogOnOff")) {

			logFileStreamString += "<i>"+playerName+" is game admin now!</i><br>\n";

			if(myConfig->readConfigInt("LogInterval") == 0) {
				writeLogFileStream(logFileStreamString);
				logFileStreamString = "";
			}
		}

	}
}

void guiLog::logPlayerJoinedMsg(QString playerName)
{
#ifdef GUI_800x480
	myW->tabs.textBrowser_Log->append("<span style=\"color:#"+myStyle->getChatLogTextColor()+";\"><i>"+playerName+" has joined the game!</i></span>");
#else
	myW->textBrowser_Log->append("<span style=\"color:#"+myStyle->getChatLogTextColor()+";\"><i>"+playerName+" has joined the game!</i></span>");
#endif
}

void guiLog::logSpectatorLeftMsg(QString playerName, int wasKicked)
{
	// TODO
}

void guiLog::logSpectatorJoinedMsg(QString playerName)
{
	// TODO
}

void guiLog::logPlayerWinGame(QString playerName, int gameID)
{

#ifdef GUI_800x480
	myW->tabs.textBrowser_Log->append( "<i><b>"+playerName+" wins game " + QString::number(gameID,10)  +"!</i></b><br>");
#else
	myW->textBrowser_Log->append( "<i><b>"+playerName+" wins game " + QString::number(gameID,10)  +"!</i></b><br>");
#endif

	if(HTML_LOG) {

		if(myConfig->readConfigInt("LogOnOff")) {

			logFileStreamString += "</br></br><i><b>"+playerName+" wins game " + QString::number(gameID,10)  +"!</i></b></br>\n";

			if(myConfig->readConfigInt("LogInterval") == 0) {
				writeLogFileStream(logFileStreamString);
				logFileStreamString = "";
			}
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
		cardString << "<font size=+1><b>&diams;</b></font>";
		break;
	case 1:
		cardString << "<font size=+1><b>&hearts;</b></font>";
		break;
	case 2:
		cardString << "<font size=+1><b>&spades;</b></font>";
		break;
	case 3:
		cardString << "<font size=+1><b>&clubs;</b></font>";
		break;
	default:
		cardString << "ERROR";
	}

	return cardString;
}

void guiLog::writeLogFileStream(QString streamString)
{

	if(myHtmlLogFile_old) {
		if(myHtmlLogFile_old->open( QIODevice::ReadWrite )) {
			QTextStream stream_old( myHtmlLogFile_old );
			stream_old.readAll();
			stream_old << streamString;
			myHtmlLogFile_old->close();
		} else {
			cout << "Could not open log-file to write log-messages!" << endl;
		}
	} else {
		cout << "Could not find log-file to write log-messages!" << endl;
	}
}

void guiLog::writeLogFileStream(string log_string, QFile *LogFile)
{

	QTextStream stream( LogFile );
	stream.readAll();
	stream << log_string.c_str();

}

void guiLog::writeLog(string log_string, int modus)
{

	switch(modus) {
	case 1:
		writeLogFileStream(log_string,myHtmlLogFile);
		break;
	case 2:
		writeLogFileStream(log_string,myTxtLogFile);
		break;
	case 3:
		tb->append(log_string.c_str());
		break;
	default:
		;
	}

}

void guiLog::flushLogAtHand()
{

	if(HTML_LOG) {

		if(myConfig->readConfigInt("LogOnOff")) {
			if(myConfig->readConfigInt("LogInterval") < 2) {
				// 	write for log after every action and after every hand
				writeLogFileStream(logFileStreamString);
				logFileStreamString = "";
			}
		}

	}
}

void guiLog::flushLogAtGame(int gameID)
{

	if(HTML_LOG) {

		if(myConfig->readConfigInt("LogOnOff")) {
			//	write for log after every game
			if(gameID > lastGameID) {
				writeLogFileStream(logFileStreamString);
				logFileStreamString = "";
				lastGameID = gameID;
			}
		}

	}
}

void guiLog::exportLogPdbToHtml(QString fileStringPdb, QString exportFileString)
{

	myHtmlLogFile = new QFile(exportFileString);

	myHtmlLogFile->open( QIODevice::ReadWrite | QFile::Truncate);

	string log_string = "<html>\n";
	log_string += "<head>\n";
	log_string += "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf8\">";
	log_string += "</head>\n";
	log_string += "<body style=\"font-size:smaller\">\n";
	writeLog(log_string,1);

	exportLog(fileStringPdb,1);

	myHtmlLogFile->close();

}

void guiLog::exportLogPdbToTxt(QString fileStringPdb, QString exportFileString)
{

	myTxtLogFile = new QFile(exportFileString);

	myTxtLogFile->open( QIODevice::ReadWrite | QFile::Truncate );

	exportLog(fileStringPdb,2);

	myTxtLogFile->close();

}

void guiLog::showLog(QString fileStringPdb, QTextBrowser *tb_tmp, int uniqueGameID)
{
	tb = tb_tmp;
	tb->clear();
	exportLog(fileStringPdb,3,uniqueGameID);

}

int guiLog::exportLog(QString fileStringPdb,int modus,int uniqueGameID_req)
{
	bool neu = false;

	result_struct results;
	results.result_Session = 0;
	results.result_Game = 0;
	results.result_Player = 0;
	results.result_Hand = 0;
	results.result_Hand_ID = 0;
	results.result_Action = 0;

	string sql = "";
	string log_string = "";
	string round_string = "";
	string action_string = "";
	bool data_found = false;
	int nRow_Session=0, nRow_Game=0, nRow_Player=0, nRow_Hand=0, nRow_Hand_ID=0, nRow_Action=0;
	int nCol_Session=0, nCol_Game=0, nCol_Player=0, nCol_Hand=0, nCol_Action=0;
	char *errmsg = 0;
	int game_ctr = 0, hand_ctr = 0, round_ctr = 0, action_ctr = 0;
	int i = 0, j = 0;
	int gameID = 0;
	int uniqueGameID = 0;
	string cmpString = "", string_tmp = "";
	string player[MAX_NUMBER_OF_PLAYERS];
	for(i=1; i<=MAX_NUMBER_OF_PLAYERS; i++) {
		player[i-1] = "";
	}

	// open sqlite log-db
	sqlite3 *mySqliteLogDb;
	sqlite3_open(fileStringPdb.toStdString().c_str(), &mySqliteLogDb);
	if( mySqliteLogDb != 0 ) {

		// read session
		sql = "SELECT * FROM Session";
		if(sqlite3_get_table(mySqliteLogDb,sql.c_str(),&results.result_Session,&nRow_Session,&nCol_Session,&errmsg) != SQLITE_OK) {
			cout << "Error in statement: " << sql.c_str() << "[" << errmsg << "]." << endl;
			cleanUp(results, mySqliteLogDb);
			return 1;
		}
		if(nRow_Session != 1) {
			cout << "Number of Sessions implausible!" << endl;
			cleanUp(results, mySqliteLogDb);
			return 1;
		}

		log_string += "Log-File for PokerTH ";

		// pokerth version
		data_found = false;
		for(i=0; i<nCol_Session; i++) {
			if(boost::lexical_cast<std::string>(results.result_Session[i]) == "PokerTH_Version") {
				log_string += boost::lexical_cast<std::string>(results.result_Session[i+nCol_Session]);
				data_found = true;
			}
		}
		if(!data_found) {
			cout << "Missing PokerTH version information!" << endl;
			cleanUp(results, mySqliteLogDb);
			return 1;
		}

		log_string += " Session started on ";

		// logging date
		data_found = false;
		for(i=0; i<nCol_Session; i++) {
			if(boost::lexical_cast<std::string>(results.result_Session[i]) == "Date") {
				log_string += boost::lexical_cast<std::string>(results.result_Session[i+nCol_Session]);
				data_found = true;
			}
		}
		if(!data_found) {
			cout << "Missing date information!" << endl;
			cleanUp(results, mySqliteLogDb);
			return 1;
		}

		log_string += " at ";

		// logging time
		data_found = false;
		for(i=0; i<nCol_Session; i++) {
			if(boost::lexical_cast<std::string>(results.result_Session[i]) == "Time") {
				log_string += boost::lexical_cast<std::string>(results.result_Session[i+nCol_Session]);
				data_found = true;
			}
		}
		if(!data_found) {
			cout << "Missing time information!" << endl;
			cleanUp(results, mySqliteLogDb);
			return 1;
		}

		switch(modus) {
		case 1:
			log_string = "<h3><b>" + log_string + "</b></h3>\n";
			// if(!neu) log_string = "<img src='logo.png'>\n" + log_string;
			break;
		case 2:
			log_string += "";
			break;
		case 3:
			log_string = "<h4><b>" + log_string + "</b></h4>";
			break;
		default:
			;
		}
		writeLog(log_string,modus);
		log_string = "";

		// read game
		if(uniqueGameID_req > 0) {
			sql = "SELECT * FROM Game WHERE UniqueGameID=" + boost::lexical_cast<std::string>(uniqueGameID_req);
		} else {
			sql = "SELECT * FROM Game";
		}
		if(sqlite3_get_table(mySqliteLogDb,sql.c_str(),&results.result_Game,&nRow_Game,&nCol_Game,&errmsg) != SQLITE_OK) {
			cout << "Error in statement: " << sql.c_str() << "[" << errmsg << "]." << endl;
			cleanUp(results, mySqliteLogDb);
			return 1;
		}

		// run through all games
		for(game_ctr=1; game_ctr<=nRow_Game; game_ctr++) {

			// unique game id
			data_found = false;
			for(i=0; i<nCol_Game; i++) {
				if(boost::lexical_cast<std::string>(results.result_Game[i]) == "UniqueGameID") {
					uniqueGameID = boost::lexical_cast<int>(results.result_Game[i+nCol_Game*game_ctr]);
					data_found = true;
				}
			}

			if(!data_found) {
				cleanUp(results, mySqliteLogDb);
				return 1;
			}

			// game id
			data_found = false;
			for(i=0; i<nCol_Game; i++) {
				if(boost::lexical_cast<std::string>(results.result_Game[i]) == "GameID") {
					gameID = boost::lexical_cast<int>(results.result_Game[i+nCol_Game*game_ctr]);
					data_found = true;
				}
			}

			if(!data_found) {
				cleanUp(results, mySqliteLogDb);
				return 1;
			}

			// read player
			sql  = "SELECT Player,Seat FROM Player WHERE UniqueGameID=";
			sql += boost::lexical_cast<std::string>(uniqueGameID);
			sql += " ORDER BY Seat;";
			if(sqlite3_get_table(mySqliteLogDb,sql.c_str(),&results.result_Player,&nRow_Player,&nCol_Player,&errmsg) != SQLITE_OK) {
				cout << "Error in statement: " << sql.c_str() << "[" << errmsg << "]." << endl;
				cleanUp(results, mySqliteLogDb);
				return 1;
			}
			for(i=1; i<=nRow_Player; i++) {
				player[i-1] = boost::lexical_cast<std::string>(results.result_Player[nCol_Player*i]);
			}

			// read all hand id
			sql = "SELECT HandID FROM Hand WHERE UniqueGameID=";
			sql+= boost::lexical_cast<std::string>(uniqueGameID);
			if(sqlite3_get_table(mySqliteLogDb,sql.c_str(),&results.result_Hand_ID,&nRow_Hand_ID,&nCol_Hand,&errmsg) != SQLITE_OK) {
				cout << "Error in statement: " << sql.c_str() << "[" << errmsg << "]." << endl;
				cleanUp(results, mySqliteLogDb);
				return 1;
			}

			// run through all hands
			for(hand_ctr=1; hand_ctr<=nRow_Hand_ID; hand_ctr++) {

				// log game and hand id
				log_string += "Game: ";
				log_string += boost::lexical_cast<std::string>(gameID);
				log_string += " | Hand: ";
				log_string += boost::lexical_cast<std::string>(results.result_Hand_ID[hand_ctr]);

				switch(modus) {
				case 1:
					log_string = "<table><tr><td width=\"600\" align=\"center\"><hr noshade size=\"3\"><b>" + log_string;
					if(!neu) log_string += "</b></td><td></td></tr></table>";
					else log_string += "</b></td></tr></table>";
					break;
				case 2:
					log_string = "\n\n----------- " + log_string;
					log_string += " -----------\n";
					break;
				case 3:
					log_string = "----------- <b>" + log_string;
					log_string += "</b> -----------<br />";
					break;
				default:
					;
				}

				// read current hand
				sql = "SELECT * FROM Hand WHERE UniqueGameID=";
				sql+= boost::lexical_cast<std::string>(uniqueGameID);
				sql+= " AND HandID=";
				sql+= boost::lexical_cast<std::string>(results.result_Hand_ID[hand_ctr]);
				if(sqlite3_get_table(mySqliteLogDb,sql.c_str(),&results.result_Hand,&nRow_Hand,&nCol_Hand,&errmsg) != SQLITE_OK) {
					cout << "Error in statement: " << sql.c_str() << "[" << errmsg << "]." << endl;
					cleanUp(results, mySqliteLogDb);
					return 1;
				}


				// log blind level
				log_string += "BLIND LEVEL: $";

				// read small blind amount
				data_found = false;
				for(i=0; i<nCol_Hand; i++) {
					if(boost::lexical_cast<std::string>(results.result_Hand[i]) == "Sb_Amount") {
						log_string += boost::lexical_cast<std::string>(results.result_Hand[i+nCol_Hand]);
						data_found = true;
					}
				}
				if(!data_found) {
					cout << "Missing small blind information!" << endl;
					cleanUp(results, mySqliteLogDb);
					return 1;
				}

				log_string += " / $";

				// read big blind amount
				data_found = false;
				for(i=0; i<nCol_Hand; i++) {
					if(boost::lexical_cast<std::string>(results.result_Hand[i]) == "Bb_Amount") {
						log_string += boost::lexical_cast<std::string>(results.result_Hand[i+nCol_Hand]);
						data_found = true;
					}
				}
				if(!data_found) {
					cout << "Missing big blind information!" << endl;
					cleanUp(results, mySqliteLogDb);
					return 1;
				}

				switch(modus) {
				case 1:
					if(!neu) log_string += "</br>";
					else log_string += "<br />";
					break;
				case 2:
					log_string += "\n";
					break;
				case 3:
					log_string += "<br />";
					break;
				default:
					;
				}

				// read seat cash
				for(i=1; i<=MAX_NUMBER_OF_PLAYERS; i++) {

					data_found = false;
					for(j=0; j<nCol_Hand; j++) {
						cmpString = "Seat_";
						cmpString+= boost::lexical_cast<std::string>(i);
						cmpString+= "_Cash";
						if(boost::lexical_cast<std::string>(results.result_Hand[j]) == cmpString) { // seat found
							if(results.result_Hand[j+nCol_Hand]) { // player has cash > 0
								log_string += "Seat ";
								log_string += boost::lexical_cast<std::string>(i);
								log_string += ": ";
								if(modus == 1 || modus == 3) {
									log_string += "<b>";
								}
								log_string += player[i-1];
								if(modus == 1 || modus == 3) {
									log_string += "</b>";
								}
								log_string += " ($";
								log_string += boost::lexical_cast<std::string>(results.result_Hand[j+nCol_Hand]);
								log_string += ")";
								switch(modus) {
								case 1:
									if(!neu) log_string += "</br>";
									else log_string += "<br />";
									break;
								case 2:
									log_string += "\n";
									break;
								case 3:
									log_string += "<br />";
									break;
								default:
									;
								}
							}
							data_found = true;
						}
					}
					if(!data_found) {
						cout << "Missing seat information in uniqueGame " << uniqueGameID << " and hand " << results.result_Hand_ID[hand_ctr] << "!" << endl;
						cleanUp(results, mySqliteLogDb);
						return 1;
					}
				}

				if(neu) {

					if(modus == 1) log_string += "<br />";

					// read dealer and blinds
					sql = "SELECT Player,Action,Amount FROM Action WHERE UniqueGameID=";
					sql += boost::lexical_cast<std::string>(uniqueGameID);
					sql += " AND HandID=";
					sql += boost::lexical_cast<std::string>(results.result_Hand_ID[hand_ctr]);
					sql += " AND BeRo=";
					sql += boost::lexical_cast<std::string>(GAME_STATE_PREFLOP);
					sql += " AND (Action='posts small blind' OR Action='posts big blind' OR Action='starts as dealer')";

					if(sqlite3_get_table(mySqliteLogDb,sql.c_str(),&results.result_Action,&nRow_Action,&nCol_Action,&errmsg) != SQLITE_OK) {
						cout << "Error in statement: " << sql.c_str() << "[" << errmsg << "]." << endl;
						cleanUp(results, mySqliteLogDb);
						return 1;
					}
					if(nRow_Action<1) {
						cout << "Missing information about dealer and blinds in uniqueGame " << uniqueGameID << " hand " << results.result_Hand_ID[hand_ctr] << "!" << endl;
						cleanUp(results, mySqliteLogDb);
						return 1;
					}
					// log dealer and blind setting
					for(i=1; i<=nRow_Action; i++) {
						log_string += player[boost::lexical_cast<int>(results.result_Action[3*i])-1];
						log_string += " ";
						log_string += boost::lexical_cast<std::string>(results.result_Action[3*i+1]);
						if(results.result_Action[3*i+2]) {
							// with amount
							log_string +=  " $";
							log_string += boost::lexical_cast<std::string>(results.result_Action[3*i+2]);
						}
						log_string += ".";
						switch(modus) {
						case 1:
							log_string += "<br />";
							break;
						case 2:
							log_string += "\n";
							break;
						case 3:
							log_string += "<br />";
							break;
						default:
							;
						}
					}

				} else {

					log_string += "BLINDS: ";

					// read small blind
					sql = "SELECT Player,Amount FROM Action WHERE UniqueGameID=";
					sql += boost::lexical_cast<std::string>(uniqueGameID);
					sql += " AND HandID=";
					sql += boost::lexical_cast<std::string>(results.result_Hand_ID[hand_ctr]);
					sql += " AND BeRo=0 AND Action='posts small blind'";

					if(sqlite3_get_table(mySqliteLogDb,sql.c_str(),&results.result_Action,&nRow_Action,&nCol_Action,&errmsg) != SQLITE_OK) {
						cout << "Error in statement: " << sql.c_str() << "[" << errmsg << "]." << endl;
						cleanUp(results, mySqliteLogDb);
						return 1;
					}
					if(nRow_Action<1 || nRow_Action>1) {
						cout << "Wrong information about small blind in uniqueGame " << uniqueGameID << " hand " << results.result_Hand_ID[hand_ctr] << "!" << endl;
						cleanUp(results, mySqliteLogDb);
						return 1;
					}

					// log small blind
					log_string += player[boost::lexical_cast<int>(results.result_Action[2])-1];
					log_string += " ($";
					log_string += boost::lexical_cast<std::string>(results.result_Action[3]);
					log_string += "), ";

					// read big blind
					sql = "SELECT Player,Amount FROM Action WHERE UniqueGameID=";
					sql += boost::lexical_cast<std::string>(uniqueGameID);
					sql += " AND HandID=";
					sql += boost::lexical_cast<std::string>(results.result_Hand_ID[hand_ctr]);
					sql += " AND BeRo=0 AND Action='posts big blind'";

					if(sqlite3_get_table(mySqliteLogDb,sql.c_str(),&results.result_Action,&nRow_Action,&nCol_Action,&errmsg) != SQLITE_OK) {
						cout << "Error in statement: " << sql.c_str() << "[" << errmsg << "]." << endl;
						cleanUp(results, mySqliteLogDb);
						return 1;
					}
					if(nRow_Action<1 || nRow_Action>1) {
						cout << "Wrong information about big blind in uniqueGame " << uniqueGameID << " hand " << results.result_Hand_ID[hand_ctr] << "!" << endl;
						cleanUp(results, mySqliteLogDb);
						return 1;
					}

					// log big blind
					log_string += player[boost::lexical_cast<int>(results.result_Action[2])-1];
					log_string += " ($";
					log_string += boost::lexical_cast<std::string>(results.result_Action[3]);
					log_string += ")";

					// read dealer
					sql = "SELECT Player,Amount FROM Action WHERE UniqueGameID=";
					sql += boost::lexical_cast<std::string>(uniqueGameID);
					sql += " AND HandID=";
					sql += boost::lexical_cast<std::string>(results.result_Hand_ID[hand_ctr]);
					sql += " AND BeRo=0 AND Action='starts as dealer'";

					if(sqlite3_get_table(mySqliteLogDb,sql.c_str(),&results.result_Action,&nRow_Action,&nCol_Action,&errmsg) != SQLITE_OK) {
						cout << "Error in statement: " << sql.c_str() << "[" << errmsg << "]." << endl;
						cleanUp(results, mySqliteLogDb);
						return 1;
					}
					if(nRow_Action>1) {
						cout << "Implausible information about dealer in uniqueGame " << uniqueGameID << " hand " << results.result_Hand_ID[hand_ctr] << "!" << endl;
						cleanUp(results, mySqliteLogDb);
						return 1;
					}

					if(nRow_Action == 1) {
						switch(modus) {
						case 1:
							if(!neu) log_string += "</br>";
							else log_string += "<br />";
							break;
						case 2:
							log_string += "\n";
							break;
						case 3:
							log_string += "<br />";
							break;
						default:
							;
						}
						log_string += player[boost::lexical_cast<int>(results.result_Action[2])-1];
						log_string += " starts as dealer.";
					}

				}

				if(!neu && modus==1) log_string += "</br>";

				writeLog(log_string,modus);
				log_string = "";

				for(round_ctr=GAME_STATE_PREFLOP; round_ctr<=GAME_STATE_POST_RIVER; round_ctr++) {

					round_string = "";
					// write round name and board cards
					if(round_ctr<=GAME_STATE_RIVER) {
						switch(round_ctr) {
						case GAME_STATE_PREFLOP:
							round_string += "PREFLOP";
							break;
						case GAME_STATE_FLOP:
							round_string += "FLOP";
							break;
						case GAME_STATE_TURN:
							round_string += "TURN";
							break;
						case GAME_STATE_RIVER:
							round_string += "RIVER";
							break;
						default:
							;
						}
						switch(modus) {
						case 1:
							if(!neu) round_string = "</br><b>" + round_string + "</b>";
							else round_string = "<br /><b>" + round_string + "</b>";
							if(round_ctr >= GAME_STATE_FLOP) {
								if(!neu) round_string = "</br>\n" + round_string;
								else round_string = "<br />\n" + round_string;
							}
							break;
						case 2:
							round_string = "\n\n" + round_string;
							break;
						case 3:
							round_string = "<b>" + round_string + "</b>";
							if(round_ctr >= GAME_STATE_FLOP) {
								round_string = "<br /><br />" + round_string;
							}
							break;
						default:
							;
						}
						if(round_ctr >= GAME_STATE_FLOP) {
							round_string += " [board cards ";
							for(i=1; i<=round_ctr+2; i++) {
								data_found = false;
								for(j=0; j<nCol_Hand; j++) {
									if(boost::lexical_cast<std::string>(results.result_Hand[j]) == "BoardCard_"+boost::lexical_cast<std::string>(i)) {
										if(results.result_Hand[j+nCol_Hand]) {
											if(modus == 1 || modus == 3) round_string += "<b>";
											string_tmp = convertCardIntToString(boost::lexical_cast<int>(results.result_Hand[j+nCol_Hand]),modus);
											if(string_tmp == "") {
												cout << "Implausible board card in uniqueGame " << uniqueGameID << " hand " << results.result_Hand_ID[hand_ctr] << "!" << endl;
												cleanUp(results, mySqliteLogDb);
												return 1;
											}
											round_string += boost::lexical_cast<std::string>(string_tmp.at(0));
											if(modus==1 || modus == 3) round_string += "</b>";
											round_string += boost::lexical_cast<std::string>(string_tmp.erase(0,1));
											if(round_ctr+2-i > 0) round_string += ",";

											data_found = true;
										}
									}
								}
							}
							round_string += "]";
						}
						if(data_found) {
							log_string += round_string;
						} else {
							continue;
						}
					}

					// read round action
					sql = "SELECT Player,Action,Amount FROM Action WHERE UniqueGameID=";
					sql += boost::lexical_cast<std::string>(uniqueGameID);
					sql += " AND HandID=";
					sql += boost::lexical_cast<std::string>(results.result_Hand_ID[hand_ctr]);
					sql += " AND BeRo=";
					sql += boost::lexical_cast<std::string>(round_ctr);
					sql += " AND Action<>'starts as dealer' AND Action<>'posts big blind' AND Action<>'posts small blind'";

					if(sqlite3_get_table(mySqliteLogDb,sql.c_str(),&results.result_Action,&nRow_Action,&nCol_Action,&errmsg) != SQLITE_OK) {
						cout << "Error in statement: " << sql.c_str() << "[" << errmsg << "]." << endl;
						cleanUp(results, mySqliteLogDb);
						return 1;
					}

					for(action_ctr=1; action_ctr<=nRow_Action; action_ctr++) {
						switch(modus) {
						case 1:
							if(!neu) {
								if(action_ctr>1 && (boost::lexical_cast<std::string>(results.result_Action[3*(action_ctr-1)+1]) == "wins" || boost::lexical_cast<std::string>(results.result_Action[3*(action_ctr-1)+1]) == "sits out" || boost::lexical_cast<std::string>(results.result_Action[3*(action_ctr-1)+1]) == "wins (side pot)"))
									log_string += "\n";
								else
									log_string += "</br>\n";
							} else {
								log_string += "<br />\n";
							}
							break;
						case 2:
							log_string += "\n";
							break;
						case 3:
							log_string += "<br />";
							break;
						default:
							;
						}
						if(!neu && boost::lexical_cast<std::string>(results.result_Action[3*action_ctr+1]) == "wins (side pot)") {
							action_string += player[boost::lexical_cast<int>(results.result_Action[3*action_ctr])-1];
							action_string += " wins $";
							action_string += boost::lexical_cast<std::string>(results.result_Action[3*action_ctr+2]);
							action_string += " (side pot)";
						} else {
							action_string += player[boost::lexical_cast<int>(results.result_Action[3*action_ctr])-1];
							action_string += " ";
							action_string += boost::lexical_cast<std::string>(results.result_Action[3*action_ctr+1]);
							if(results.result_Action[3*action_ctr+2]) {
								// with amount
								action_string += " $";
								action_string += boost::lexical_cast<std::string>(results.result_Action[3*action_ctr+2]);
							}
						}

						// wins game
						if(boost::lexical_cast<std::string>(results.result_Action[3*action_ctr+1]) == "wins game") {
							switch(modus) {
							case 1:
								if(!neu) action_string = "</br></br><i><b>" + action_string + " " + boost::lexical_cast<std::string>(gameID) + "!</i></b></br>";
								else action_string = "</br><i><b>" + action_string + " " + boost::lexical_cast<std::string>(gameID) + "!</b></i>";
								break;
							case 2:
								action_string += action_string + " " + boost::lexical_cast<std::string>(gameID) + "!";
								break;
							case 3:
								action_string = "<i><b>" + action_string + " " + boost::lexical_cast<std::string>(gameID) + "!</b></i>";
								break;
							default:
								;
							}
						}

						// wins
						if(boost::lexical_cast<std::string>(results.result_Action[3*action_ctr+1]) == "wins" || boost::lexical_cast<std::string>(results.result_Action[3*action_ctr+1]) == "wins (side pot)") {
							switch(modus) {
							case 1:
								if(!neu) action_string = "</br><i>" + action_string + "</i>";
								else action_string = "<i>" + action_string + "</i>";
								break;
							case 3:
								action_string = "<i>" + action_string + "</i>";
								break;
							default:
								;
							}
						}

						// network actions
						if(boost::lexical_cast<std::string>(results.result_Action[3*action_ctr+1]) == "has left the game" || boost::lexical_cast<std::string>(results.result_Action[3*action_ctr+1]) == "was kicked from the game" || boost::lexical_cast<std::string>(results.result_Action[3*action_ctr+1]) == "is game admin now" || boost::lexical_cast<std::string>(results.result_Action[3*action_ctr+1]) == "has joined the game") {
							switch(modus) {
							case 1:
								if(!neu) action_string = "<i>" + action_string + "!</i>";
								else action_string = "<i>" + action_string + "</i>";
								break;
							case 3:
								action_string = "<i>" + action_string + "</i>";
								break;
							default:
								;
							}
						}

						// sits out
						if(boost::lexical_cast<std::string>(results.result_Action[3*action_ctr+1]) == "sits out") {
							switch(modus) {
							case 1:
								if(!neu) action_string = "</br><i><span style=\"font-size:smaller;\">" + action_string + "</span></i>";
								else action_string = "<i><span style=\"font-size:smaller;\">" + action_string + "</span></i>";
								break;
							case 3:
								action_string = "<i><span style=\"font-size:smaller;\">" + action_string + "</span></i>";
								break;
							default:
								;
							}
						}

						log_string += action_string;
						action_string = "";

						// show cards
						if(boost::lexical_cast<std::string>(results.result_Action[3*action_ctr+1]) == "shows" || boost::lexical_cast<std::string>(results.result_Action[3*action_ctr+1]) == "has") {
							// log cards
							if(!neu && round_ctr == GAME_STATE_POST_RIVER) log_string += " [ ";
							else log_string += " [";
							if(modus == 1 || modus == 3) log_string += "<b>";

							// find hole card 1
							data_found = false;
							for(i=0; i<nCol_Hand; i++) {
								cmpString = "Seat_";
								cmpString += boost::lexical_cast<std::string>(results.result_Action[3*action_ctr]);
								cmpString += "_Card_1";
								if(boost::lexical_cast<std::string>(results.result_Hand[i]) == cmpString) {
									string_tmp = convertCardIntToString(boost::lexical_cast<int>(results.result_Hand[i+nCol_Hand]),modus);
									if(string_tmp == "") {
										cout << "Hole card information implausible in uniqueGame " << uniqueGameID << " hand " << results.result_Hand_ID[hand_ctr] << "!" << endl;
										cleanUp(results, mySqliteLogDb);
										return 1;
									}
									log_string += boost::lexical_cast<std::string>(string_tmp.at(0));
									if(modus == 1 || modus == 3) log_string += "</b>";
									log_string += boost::lexical_cast<std::string>(string_tmp.erase(0,1));
									log_string += ",";
									if(modus == 1 || modus == 3) log_string += "<b>";
									data_found = true;
								}
							}
							if(!data_found) {
								cout << "Missing hole card information in uniqueGame " << uniqueGameID << " hand " << results.result_Hand_ID[hand_ctr] << "!" << endl;
								cleanUp(results, mySqliteLogDb);
								return 1;
							}

							// find hole card 2
							data_found = false;
							for(i=0; i<nCol_Hand; i++) {
								cmpString = "Seat_";
								cmpString += boost::lexical_cast<std::string>(results.result_Action[3*action_ctr]);
								cmpString += "_Card_2";
								if(boost::lexical_cast<std::string>(results.result_Hand[i]) == cmpString) {
									string_tmp = convertCardIntToString(boost::lexical_cast<int>(results.result_Hand[i+nCol_Hand]),modus);
									if(string_tmp == "") {
										cout << "Hole card information implausible in uniqueGame " << uniqueGameID << " hand " << results.result_Hand_ID[hand_ctr] << "!" << endl;
										cleanUp(results, mySqliteLogDb);
										return 1;
									}
									log_string += boost::lexical_cast<std::string>(string_tmp.at(0));
									if(modus == 1 || modus == 3) log_string += "</b>";
									log_string += boost::lexical_cast<std::string>(string_tmp.erase(0,1));
									log_string += "]";
									data_found = true;
								}
							}
							if(!data_found) {
								cout << "Missing hole card information in uniqueGame " << uniqueGameID << " hand " << results.result_Hand_ID[hand_ctr] << "!" << endl;
								cleanUp(results, mySqliteLogDb);
								return 1;
							}

							if(round_ctr == GAME_STATE_POST_RIVER) {
								// find hand name
								for(i=0; i<nCol_Hand; i++) {
									cmpString = "Seat_";
									cmpString += boost::lexical_cast<std::string>(results.result_Action[3*action_ctr]);
									cmpString += "_Hand_text";
									if(boost::lexical_cast<std::string>(results.result_Hand[i]) == cmpString && results.result_Hand[i+nCol_Hand]) {
										log_string += " - " + boost::lexical_cast<std::string>(results.result_Hand[i+nCol_Hand]);
									}
								}
							}

						}

						if(!neu && boost::lexical_cast<std::string>(results.result_Action[3*action_ctr+1]) != "wins" && boost::lexical_cast<std::string>(results.result_Action[3*action_ctr+1]) != "shows" && boost::lexical_cast<std::string>(results.result_Action[3*action_ctr+1]) != "has" && boost::lexical_cast<std::string>(results.result_Action[3*action_ctr+1]) != "sits out" && boost::lexical_cast<std::string>(results.result_Action[3*action_ctr+1]) != "wins (side pot)" && boost::lexical_cast<std::string>(results.result_Action[3*action_ctr+1]) != "wins game" && boost::lexical_cast<std::string>(results.result_Action[3*action_ctr+1]) != "has left the game" && boost::lexical_cast<std::string>(results.result_Action[3*action_ctr+1]) != "was kicked from the game" && boost::lexical_cast<std::string>(results.result_Action[3*action_ctr+1]) != "is game admin now" && boost::lexical_cast<std::string>(results.result_Action[3*action_ctr+1]) != "has joined the game") {
							log_string += ".";
						}
						if(neu && boost::lexical_cast<std::string>(results.result_Action[3*action_ctr+1]) != "wins game" && boost::lexical_cast<std::string>(results.result_Action[3*action_ctr+1]) != "has left the game" && boost::lexical_cast<std::string>(results.result_Action[3*action_ctr+1]) != "was kicked from the game" && boost::lexical_cast<std::string>(results.result_Action[3*action_ctr+1]) != "is game admin now" && boost::lexical_cast<std::string>(results.result_Action[3*action_ctr+1]) != "has joined the game")
							log_string += ".";

					}

				}

				if(modus == 1) log_string += "\n";
				writeLog(log_string,modus);
				log_string = "";



			}

		}

	}

	cleanUp(results, mySqliteLogDb);

	return 0;

}

QList<int> guiLog::getGameList(QString fileStringPdb)
{

	result_struct results;
	results.result_Session = 0;
	results.result_Game = 0;
	results.result_Player = 0;
	results.result_Hand = 0;
	results.result_Hand_ID = 0;
	results.result_Action = 0;

	int nRow_Game=0;
	int nCol_Game=0;
	char *errmsg = 0;
	int game_ctr = 0;
	int i = 0;

	QList<int> gameList;

	// open sqlite log-db
	sqlite3 *mySqliteLogDb;
	sqlite3_open(fileStringPdb.toStdString().c_str(), &mySqliteLogDb);
	if( mySqliteLogDb != 0 ) {

		string sql = "SELECT * FROM Game";
		if(sqlite3_get_table(mySqliteLogDb,sql.c_str(),&results.result_Game,&nRow_Game,&nCol_Game,&errmsg) != SQLITE_OK) {
			cout << "Error in statement: " << sql.c_str() << "[" << errmsg << "]." << endl;
		} else {
			for(game_ctr=1; game_ctr<=nRow_Game; game_ctr++) {
				for(i=0; i<nCol_Game; i++) {
					if(boost::lexical_cast<std::string>(results.result_Game[i]) == "UniqueGameID") {
						gameList.append(boost::lexical_cast<int>(results.result_Game[i+nCol_Game*game_ctr]));
					}
				}
			}
		}
	}

	cleanUp(results, mySqliteLogDb);

	return gameList;

}

void guiLog::cleanUp(result_struct &results, sqlite3 *mySqliteLogDb)
{

	sqlite3_free_table(results.result_Session);
	sqlite3_free_table(results.result_Game);
	sqlite3_free_table(results.result_Hand);
	sqlite3_free_table(results.result_Hand_ID);
	sqlite3_free_table(results.result_Action);
	sqlite3_close(mySqliteLogDb);
}

int guiLog::convertCardStringToInt(string val, string col)
{

	int tmp;

	switch(*col.c_str()) {
	case 'd':
		tmp = 0;
		break;
	case 'h':
		tmp = 13;
		break;
	case 's':
		tmp = 26;
		break;
	case 'c':
		tmp = 39;
		break;
	default:
		return -1;
	}

	switch(*val.c_str()) {
	case '2':
		tmp += 0;
		break;
	case '3':
		tmp += 1;
		break;
	case '4':
		tmp += 2;
		break;
	case '5':
		tmp += 3;
		break;
	case '6':
		tmp += 4;
		break;
	case '7':
		tmp += 5;
		break;
	case '8':
		tmp += 6;
		break;
	case '9':
		tmp += 7;
		break;
	case 'T':
		tmp += 8;
		break;
	case 'J':
		tmp += 9;
		break;
	case 'Q':
		tmp += 10;
		break;
	case 'K':
		tmp += 11;
		break;
	case 'A':
		tmp += 12;
		break;
	default:
		return -1;
	}

	return tmp;

}

string guiLog::convertCardIntToString(int code, int modus)
{

	string tmp;

	switch(code%13) {
	case 0:
		tmp = "2";
		break;
	case 1:
		tmp = "3";
		break;
	case 2:
		tmp = "4";
		break;
	case 3:
		tmp = "5";
		break;
	case 4:
		tmp = "6";
		break;
	case 5:
		tmp = "7";
		break;
	case 6:
		tmp = "8";
		break;
	case 7:
		tmp = "9";
		break;
	case 8:
		tmp = "T";
		break;
	case 9:
		tmp = "J";
		break;
	case 10:
		tmp = "Q";
		break;
	case 11:
		tmp = "K";
		break;
	case 12:
		tmp = "A";
		break;
	default:
		return "";
	}

	if(modus==2) {

		switch(code/13) {
		case 0:
			tmp+= "d";
			break;
		case 1:
			tmp+= "h";
			break;
		case 2:
			tmp+= "s";
			break;
		case 3:
			tmp+= "c";
			break;
		default:
			return "";
		}

	} else {

		switch(code/13) {
		case 0:
			tmp+= "<font size=+1><b>&diams;</b></font>";
			break;
		case 1:
			tmp+= "<font size=+1><b>&hearts;</b></font>";
			break;
		case 2:
			tmp+= "<font size=+1><b>&spades;</b></font>";
			break;
		case 3:
			tmp+= "<font size=+1><b>&clubs;</b></font>";
			break;
		default:
			return "";
		}

	}

	return tmp;
}
