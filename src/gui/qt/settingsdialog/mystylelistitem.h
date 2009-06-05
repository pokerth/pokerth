// Author: FThauer FHammer <f.thauer@web.de>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef MYSTYLELISTITEM_H
#define MYSTYLELISTITEM_H

#include <QtGui>
#include <QtCore>

class MyStyleListItem : public QListWidgetItem {	
public:
	MyStyleListItem(QString, QListWidget * =0);
    ~MyStyleListItem();
	
	virtual bool operator<( const QListWidgetItem &) const;
};

#endif // MYSTYLELISTITEM_H
