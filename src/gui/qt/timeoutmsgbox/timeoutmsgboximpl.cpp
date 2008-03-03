//
// C++ Implementation: opengametimeoutmsgboximpl
//
// Description: 
//
//
// Author: Felix Hammer <f.hammer@web.de>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "timeoutmsgboximpl.h"
#include "session.h"

timeoutMsgBoxImpl::timeoutMsgBoxImpl(QDialog *parent, int id)
 : QMessageBox(parent), mySession(NULL), msgId(id)
{
	okButton = this->addButton(QMessageBox::Ok);

	this->setWindowTitle(tr("Timeout Warning"));
	this->setIcon(QMessageBox::Warning);
	this->setInformativeText(tr("Please click \"OK\" to stop timeout!"));
	
	timeOutTimer = new QTimer;

	connect(timeOutTimer, SIGNAL(timeout()), this, SLOT(timerRefresh()));
// 	connect(okButton, SIGNAL(clicked()), mySession, SLOT(spechialSessionSlot(msgId)));

}

timeoutMsgBoxImpl::~timeoutMsgBoxImpl()
{
}

void timeoutMsgBoxImpl::startTimeout() {
	//start the real timer
	realTimer.reset();
	realTimer.start();
	timerRefresh();
	timeOutTimer->start(1000);
}

void timeoutMsgBoxImpl::timerRefresh() {
	
	int sec = 60;
	unsigned int realTimerValue = realTimer.elapsed().total_milliseconds();
	sec -= realTimerValue/1000;
	if (sec < 0) sec = 0;
	switch (msgId) {
		case 0: { this->setText(tr("Your connection is about to time out due to inactivity in %1 seconds.").arg(sec,0,10)); }
		break;
		case 1: { this->setText(tr("Your open game reaches timeout in %1 seconds.").arg(sec,0,10)); }
		break;
	}
	
}

