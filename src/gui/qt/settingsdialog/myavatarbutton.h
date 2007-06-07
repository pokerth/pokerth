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
#ifndef MYAVATARBUTTON_H
#define MYAVATARBUTTON_H

#include <QtGui>
#include <QtCore>

class MyAvatarButton : public QPushButton
{
public:
    MyAvatarButton(QGroupBox*);

    ~MyAvatarButton();

	void setMyLink(QString theValue){ myLink = theValue;}
	QString getMyLink() const{ return myLink;}
	


private:

	QString myLink;
};

#endif
