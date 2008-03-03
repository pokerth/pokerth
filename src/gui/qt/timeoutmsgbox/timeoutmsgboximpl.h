//
// C++ Interface: opengametimeoutmsgboximpl
//
// Description: 
//
//
// Author: Felix Hammer <f.hammer@web.de>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TIMEOUTMSGBOXIMPL_H
#define TIMEOUTMSGBOXIMPL_H

#include <QMessageBox>
#include <QtGui>
#include <QtCore>
#include <core/boost/timers.hpp>
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

	void setMySession ( boost::shared_ptr<Session>  theValue ) { mySession = theValue; }
	void setMsgID ( int theValue ) { msgID = theValue; }
	void setTimeoutDuration ( int theValue ) { timeoutDuration = theValue; }
	
private: 

	QTimer *timeOutTimer;
	QPushButton *okButton;
	boost::shared_ptr<Session> mySession;
	int msgID;
	int timeoutDuration;
	boost::timers::portable::microsec_timer realTimer;
};

#endif
