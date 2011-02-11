//
// C++ Interface: mycardspixmaplabel
//
// Description:
//
//
// Author: FThauer FHammer <f.thauer@web.de>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef MYNAMELABEL_H
#define MYNAMELABEL_H

#include <iostream>

#include <QtGui>
#include <QtCore>

class gameTableImpl;

class MyNameLabel : public QLabel
{
	Q_OBJECT
public:
	MyNameLabel(QGroupBox*);
	~MyNameLabel();

	void setMyW(gameTableImpl* theValue) {
		myW = theValue;
	}

public slots:

	void setText ( const QString &, bool = FALSE, bool = FALSE, bool = FALSE);
private:

	QString myText;
	gameTableImpl *myW;

};

#endif
