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

/**
	@author Felix Hammer <f.hammer@web.de>
*/
class Session;

class timeoutMsgBoxImpl : public QMessageBox
{
Q_OBJECT
public:
    timeoutMsgBoxImpl(QDialog*, Session*, int);

    ~timeoutMsgBoxImpl();

public slots:

	void startTimeout();
	void timerRefresh();

private: 

	QTimer *timeOutTimer;
	QPushButton *okButton;
	Session *mySession;
	int msgId;
	boost::timers::portable::microsec_timer realTimer;
};

#endif
