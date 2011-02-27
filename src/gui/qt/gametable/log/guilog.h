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
#ifndef GUILOG_H
#define GUILOG_H

#include <fstream>
#include <iostream>
#include <sstream>

#include "configfile.h"
#include <sqlite3.h>

#include <QtCore>
#include <QtSql>

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
	void logNewGameAdminMsg(QString playerName);
	void logPlayerWinGame(QString playerName, int gameID);
	void flushLogAtGame(int gameID);
	void flushLogAtHand();


public:
	QStringList translateCardsValueCode(int cardsValueCode);
	QStringList translateCardCode(int cardCode);
	QString determineHandName(int cardsValueInt);

	void writeLogFileStream(QString string);

signals:
	void signalLogPlayerActionMsg(QString playerName, int action, int setValue);
	void signalLogNewGameHandMsg(int gameID, int handID);
	void signalLogNewBlindsSetsMsg(int sbSet, int bbSet, QString sbName, QString bbName);
	void signalLogPlayerWinsMsg(QString playerName, int pot, bool main);
	void signalLogPlayerSitsOut(QString playerName);
	void signalLogDealBoardCardsMsg(int roundID, int card1, int card2, int card3, int card4 = -1, int card5 = -1);
	void signalLogFlipHoleCardsMsg(QString playerName, int card1, int card2, int cardsValueInt = -1, QString showHas = "shows");
	void signalLogPlayerLeftMsg(QString playerName, int wasKicked);
	void signalLogNewGameAdminMsg(QString playerName);
	void signalLogPlayerWinGame(QString playerName, int gameID);
	void signalFlushLogAtGame(int gameID);
	void signalFlushLogAtHand();


private:
	int lastGameID;

	gameTableImpl *myW;
	ConfigFile *myConfig;
	QTextStream stream;
	QDir *myLogDir;
	QFile *myHtmlLogFile;
	sqlite3 *mySqliteLogDb;
	QString logFileStreamString;
	QString myAppDataPath;

	GameTableStyleReader *myStyle;

	friend class GuiWrapper;
};

#endif
