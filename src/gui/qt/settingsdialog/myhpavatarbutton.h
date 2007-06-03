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
#ifndef MYHPAVATARBUTTON_H
#define MYHPAVATARBUTTON_H

#include <QtGui>
#include <QtCore>

#include <iostream>

class MyHPAvatarButton : public QPushButton
{
public:
    MyHPAvatarButton(QWidget*);

    ~MyHPAvatarButton();

	void setMyLink(QString theValue){ myLink = theValue;}
	QString getMyLink() const{ return myLink;}
	


private:

	QString myLink;
};

#endif
