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
#ifndef TIMEOUTMSGBOXIMPL_H
#define TIMEOUTMSGBOXIMPL_H

#include <QMessageBox>
#include <QtGui>
#include <QtCore>
#if QT_VERSION >= 0x050000
#include <QtWidgets>
#endif
#ifndef Q_MOC_RUN
#include <third_party/boost/timers.hpp>
#endif
#include "game_defs.h"

/**
	@author Felix Hammer <f.hammer@web.de>
*/
class Session;

class timeoutMsgBoxImpl : public QMessageBox
{
	Q_OBJECT
public:
	timeoutMsgBoxImpl(QMainWindow*);

	~timeoutMsgBoxImpl();

public slots:

	void startTimeout();
	void timerRefresh();
	void stopTimeout();

	void setMySession ( boost::shared_ptr<Session>  theValue )
	{
		mySession = theValue;
	}
	void setMsgID ( NetTimeoutReason theValue )
	{
		msgID = theValue;
	}
	void setTimeoutDuration ( int theValue )
	{
		timeoutDuration = theValue;
	}

private:

	QTimer *timeOutTimer;
	QPushButton *okButton;
	boost::shared_ptr<Session> mySession;
	NetTimeoutReason msgID;
	int timeoutDuration;
	boost::timers::portable::microsec_timer realTimer;
};

#endif
