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
#include "timeoutmsgboximpl.h"
#include "session.h"

timeoutMsgBoxImpl::timeoutMsgBoxImpl(QMainWindow *parent)
	: QMessageBox(parent), msgID(NETWORK_TIMEOUT_GENERIC)
{
	okButton = this->addButton(QMessageBox::Ok);

	this->setWindowTitle(tr("Timeout Warning"));
	this->setIcon(QMessageBox::Warning);
	this->setInformativeText(tr("Please click \"OK\" to stop the countdown!"));
#ifdef ANDROID
	this->setWindowModality(Qt::WindowModal);
	this->setWindowFlags(Qt::ToolTip);
	this->setStyleSheet("QDialog{ border: 1px solid black; border-style: solid; border-radius: 4px; }");
#endif
	timeOutTimer = new QTimer;

	connect(timeOutTimer, SIGNAL(timeout()), this, SLOT(timerRefresh()));
	connect(okButton, SIGNAL(clicked()), this, SLOT(stopTimeout()));

}

timeoutMsgBoxImpl::~timeoutMsgBoxImpl()
{
}

void timeoutMsgBoxImpl::startTimeout()
{
	//start the real timer
	realTimer.reset();
	realTimer.start();
	timerRefresh();
	timeOutTimer->start(1000);
}

void timeoutMsgBoxImpl::timerRefresh()
{

	int sec = timeoutDuration;
	unsigned int realTimerValue = realTimer.elapsed().total_milliseconds();
	sec -= realTimerValue/1000;
	if (sec < 0) sec = 0;
	switch (msgID) {
	case NETWORK_TIMEOUT_GAME_ADMIN_IDLE:
		this->setText(tr("You are game-admin of an open game which will time out in %1 seconds.").arg(sec,0,10));
		break;
	case NETWORK_TIMEOUT_KICK_AFTER_AUTOFOLD:
		this->setText(tr("You did not act in the game recently. You will be removed from the game in %1 seconds.").arg(sec,0,10));
		break;
	default:
		this->setText(tr("Your connection is about to time out due to inactivity in %1 seconds.").arg(sec,0,10));
		break;
	}
}

void timeoutMsgBoxImpl::stopTimeout()
{

	mySession->resetNetworkTimeout();
}
