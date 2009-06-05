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

MyStyleListItem::MyStyleListItem(QString s, QListWidget *w) 
 : QListWidgetItem(s, w, QListWidgetItem::UserType)
{
}

MyStyleListItem::~MyStyleListItem() 
{
}

bool MyStyleListItem::operator<( const QListWidgetItem &other ) const
{
	return text().toLower() < other.text().toLower();
}

