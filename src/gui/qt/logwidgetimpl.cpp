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
#include "logwidgetimpl.h"

#include "mainwindowimpl.h"

using namespace std;

LogWidgetImpl::LogWidgetImpl(mainWindowImpl* w) : myW(w)
{
	myW->setLogWidgetImpl(this);
	
}

LogWidgetImpl::~LogWidgetImpl()
{
}

void LogWidgetImpl::showPlayerActionLogMsg(string playerName, int action, int setValue) const {
	
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



