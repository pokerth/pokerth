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
#ifndef MYAVATARLISTITEM_H
#define MYAVATARLISTITEM_H

#include <QtGui>
#include <QtCore>

#include <iostream>

class MyAvatarListItem : public QListWidgetItem
{
public:
    MyAvatarListItem(QListWidget*);

    ~MyAvatarListItem();

	void setMyLink(QString theValue){ myLink = theValue;}
	QString getMyLink() const{ return myLink;}
	


private:

	QString myLink;
};

#endif
