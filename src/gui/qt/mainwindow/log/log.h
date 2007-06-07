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
#ifndef LOG_H
#define LOG_H

#include <string>

#include "configfile.h"


#include <QtCore>

class mainWindowImpl;


class Log : public QObject
{
Q_OBJECT

public:
	Log(mainWindowImpl*, ConfigFile *c);

	~Log();

public slots:
	void logPlayerActionMsg(QString playerName, int action, int setValue);
	void logNewGameHandMsg(int gameID, int handID);
	void logPlayerWinsMsg(int playerID, int pot);
	void logDealBoardCardsMsg(int roundID, int card1, int card2, int card3, int card4 = -1, int card5 = -1);
	void logFlipHoleCardsMsg(QString playerName, int card1, int card2, int cardsValueInt = -1, QString showHas = "shows");
	void logPlayerLeftMsg(QString playerName);


public:
	QStringList translateCardsValueCode(int cardsValueCode);
	QStringList translateCardCode(int cardCode);

	void writeLogFileStream(QString string);

signals:
	void signalLogPlayerActionMsg(QString playerName, int action, int setValue);
	void signalLogNewGameHandMsg(int gameID, int handID);
	void signalLogPlayerWinsMsg(int playerID, int pot);
	void signalLogDealBoardCardsMsg(int roundID, int card1, int card2, int card3, int card4 = -1, int card5 = -1);
	void signalLogFlipHoleCardsMsg(QString playerName, int card1, int card2, int cardsValueInt = -1, QString showHas = "shows");
	void signalLogPlayerLeftMsg(QString playerName);


private:
	int lastGameID;

	mainWindowImpl *myW;
	ConfigFile *myConfig;
	QTextStream stream;
	QDir *myLogDir;
	QFile *myLogFile;
	QString logFileStreamString;

friend class GuiWrapper;
};

#endif
