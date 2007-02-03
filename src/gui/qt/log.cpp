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
		myLogFile = new QFile(myLogDir->absolutePath()+"/pokerth-log-"+QDateTime::currentDateTime().toString("yyyy-MM-dd_hh:mm:ss")+".log");

		myLogFile->open( QIODevice::WriteOnly );
		QTextStream stream( myLogFile );
		stream << "### This is a Log File for PokerTH ### \n";
		myLogFile->close();
	} 

	

;
	
}

Log::~Log()
{
}

void Log::showPlayerActionLogMsg(string playerName, int action, int setValue) const {
	
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

}



