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
#ifndef OPENGAMETIMEOUTMSGBOXIMPL_H
#define OPENGAMETIMEOUTMSGBOXIMPL_H

#include <QMessageBox>
#include <QtGui>
#include <QtCore>
#include <core/boost/timers.hpp>

/**
	@author Felix Hammer <f.hammer@web.de>
*/
class gameLobbyDialogImpl;

class openGameTimeoutMsgBoxImpl : public QMessageBox
{
Q_OBJECT
public:
    openGameTimeoutMsgBoxImpl(QDialog*, gameLobbyDialogImpl*);

    ~openGameTimeoutMsgBoxImpl();

public slots:

	void startTimeout();
	void timerRefresh();

private: 
	QTimer *timeOutTimer;
	QPushButton *okButton;
	gameLobbyDialogImpl *myLobby;
	boost::timers::portable::microsec_timer realTimer;
};

#endif
