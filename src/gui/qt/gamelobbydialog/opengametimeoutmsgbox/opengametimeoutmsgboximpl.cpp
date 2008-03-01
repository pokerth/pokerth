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
#include "opengametimeoutmsgboximpl.h"
#include "gamelobbydialogimpl.h"

openGameTimeoutMsgBoxImpl::openGameTimeoutMsgBoxImpl(QDialog *parent, gameLobbyDialogImpl *l)
 : QMessageBox(parent), myLobby(l)
{
	okButton = this->addButton(QMessageBox::Ok);

	this->setWindowTitle(tr("Open Game Timeout Warning"));
	this->setIcon(QMessageBox::Warning);
	this->setText(tr("Your open game reaches timeout in %1 seconds.").arg(60,0,10));
	this->setInformativeText(tr("Please click \"OK\" to stop timeout!"));
	
	timeOutTimer = new QTimer;

	connect(timeOutTimer, SIGNAL(timeout()), this, SLOT(timerRefresh()));
	connect(okButton, SIGNAL(clicked()), myLobby, SLOT(openGameTimeoutStoped()));

}

openGameTimeoutMsgBoxImpl::~openGameTimeoutMsgBoxImpl()
{
}

void openGameTimeoutMsgBoxImpl::startTimeout() {
	//start the real timer
	realTimer.reset();
	realTimer.start();
	timeOutTimer->start(1000);
}

void openGameTimeoutMsgBoxImpl::timerRefresh() {
	
	int sec = 60;
	unsigned int realTimerValue = realTimer.elapsed().total_milliseconds();
	sec -= realTimerValue/1000;
	if (sec < 0) sec = 0;
	this->setText(tr("Your open game reaches timeout in %1 seconds.").arg(sec,0,10));
}

