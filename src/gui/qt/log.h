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
#include <iostream>

#include "configfile.h"


#include <QtCore>

class mainWindowImpl;


class Log{
public:
    Log(mainWindowImpl*);

    ~Log();

	
	void logPlayerActionMsg(std::string playName, int action, int setValue);
	void logNewGameHandMsg(int gameID, int handID);
	void logPlayerWinsMsg(int playerID);
	void logDealBoardCardsMsg(int roundID, int card1, int card2 = -1, int card3 = -1);

	void closeLogFile();

private:

	int linesInFile;
	
	mainWindowImpl *myW;
	ConfigFile *myConfig;
	QTextStream stream;
	QFile *myLogFile;
	QDir *myLogDir;

};

#endif
