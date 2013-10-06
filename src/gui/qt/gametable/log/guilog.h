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
#ifndef GUILOG_H
#define GUILOG_H

#include <fstream>
#include <iostream>
#include <sstream>
#include <sqlite3.h>

#include "configfile.h"

#include <QtCore>
#include <QtGui>
#include <QtSql>
#if QT_VERSION >= 0x050000
#include <QtWidgets>
#endif

struct result_struct {
	char **result_Session;
	char **result_Game;
	char **result_Player;
	char **result_Hand;
	char **result_Hand_ID;
	char **result_Action;
};

class gameTableImpl;
class GameTableStyleReader;

class guiLog : public QObject
{
	Q_OBJECT

public:
	guiLog(gameTableImpl*, ConfigFile *c);

	~guiLog();

public slots:

	void logPlayerActionMsg(QString playerName, int action, int setValue);
	void logNewGameHandMsg(int gameID, int handID);
	void logNewBlindsSetsMsg(int sbSet, int bbSet, QString sbName, QString bbName);
	void logPlayerWinsMsg(QString playerName, int pot, bool main);
	void logPlayerSitsOut(QString playerName);
	void logDealBoardCardsMsg(int roundID, int card1, int card2, int card3, int card4 = -1, int card5 = -1);
	void logFlipHoleCardsMsg(QString playerName, int card1, int card2, int cardsValueInt = -1, QString showHas = "shows");
	void logPlayerLeftMsg(QString playerName, int wasKicked);
	void logPlayerJoinedMsg(QString playerName);
	void logNewGameAdminMsg(QString playerName);
	void logSpectatorLeftMsg(QString playerName, int wasKicked);
	void logSpectatorJoinedMsg(QString playerName);
	void logPlayerWinGame(QString playerName, int gameID);
	void flushLogAtGame(int gameID);
	void flushLogAtHand();
	void exportLogPdbToHtml(QString fileStringPdb, QString exportFileString);
	void exportLogPdbToTxt(QString fileStringPdb, QString exportFileString);
	void showLog(QString fileStringPdb, QTextBrowser *tb, int uniqueGameID = 0);
	int exportLog(QString fileStringPdb, int modus, int uniqueGameID = 0);
	QList<int> getGameList(QString fileStringPdb);

public:
	QStringList translateCardCode(int cardCode);

	void writeLogFileStream(QString string);

	void setMySqliteLogFileName(std::string theValue) {
		mySqliteLogFileName = theValue;
	}
	std::string getMySqliteLogFileName() {
		return mySqliteLogFileName;
	}

signals:
	void signalLogPlayerActionMsg(QString playerName, int action, int setValue);
	void signalLogNewGameHandMsg(int gameID, int handID);
	void signalLogNewBlindsSetsMsg(int sbSet, int bbSet, QString sbName, QString bbName);
	void signalLogPlayerWinsMsg(QString playerName, int pot, bool main);
	void signalLogPlayerSitsOut(QString playerName);
	void signalLogDealBoardCardsMsg(int roundID, int card1, int card2, int card3, int card4 = -1, int card5 = -1);
	void signalLogFlipHoleCardsMsg(QString playerName, int card1, int card2, int cardsValueInt = -1, QString showHas = "shows");
	void signalLogPlayerLeftMsg(QString playerName, int wasKicked);
	void signalLogPlayerJoinedMsg(QString playerName);
	void signalLogSpectatorLeftMsg(QString playerName, int wasKicked);
	void signalLogSpectatorJoinedMsg(QString playerName);
	void signalLogNewGameAdminMsg(QString playerName);
	void signalLogPlayerWinGame(QString playerName, int gameID);
	void signalFlushLogAtGame(int gameID);
	void signalFlushLogAtHand();


private:

	void writeLogFileStream(std::string log_string, QFile *LogFile);
	void writeLog(std::string log_string, int modus);
	void cleanUp(result_struct &results, sqlite3 *mySqliteLogDb);
	int convertCardStringToInt(std::string val, std::string col);
	std::string convertCardIntToString(int code, int modus);

	int lastGameID;

	gameTableImpl *myW;
	ConfigFile *myConfig;
	QTextStream stream_old;
	QDir *myLogDir;
	QFile *myHtmlLogFile;
	QFile *myHtmlLogFile_old;
	QFile *myTxtLogFile;
	QString logFileStreamString;
	QString myAppDataPath;
	QTextBrowser* tb;
	GameTableStyleReader *myStyle;
	std::string mySqliteLogFileName;

	friend class GuiWrapper;

};

#endif
