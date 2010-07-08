//
// C++ Implementation: mycardspixmaplabel
//
// Description: 
//
//
// Author: FThauer FHammer <f.thauer@web.de>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "mystylelistitem.h"

MyStyleListItem::MyStyleListItem(QStringList s, QTreeWidget *w)
 : QTreeWidgetItem(w, s, QTreeWidgetItem::UserType)
{
}

MyStyleListItem::~MyStyleListItem() 
{
}

bool MyStyleListItem::operator<( const QTreeWidgetItem &other ) const
{
        return text(0).toLower() < other.text(0).toLower();
}

