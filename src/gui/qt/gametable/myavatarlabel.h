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
#ifndef MYAVATARLABEL_H
#define MYAVATARLABEL_H

#include <iostream>

#include <QtGui>
#include <QtCore>

class gameTableImpl;

class MyAvatarLabel : public QLabel
{
Q_OBJECT
public:
    MyAvatarLabel(QGroupBox*);
    ~MyAvatarLabel();

	void setMyW(gameTableImpl* theValue) { myW = theValue; }
	void setMyId ( int theValue ) {	myId = theValue; }
	void contextMenuEvent ( QContextMenuEvent * event );

public slots:
	
	void showContextMenu(const QPoint &pos);
	void sendTriggerVoteOnKickSignal();	
	void setEnabledContextMenu(bool);
	void setVoteOnKickContextMenuEnabled(bool);

private: 

	gameTableImpl *myW;
	QMenu *myContextMenu;
	QAction *action_VoteForKick;

	int myId;
	bool myContextMenuEnabled;
};

#endif
