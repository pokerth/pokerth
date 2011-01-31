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
#include "startwindowimpl.h"
#include <QtGui>
#include <QtCore>

class gameTableImpl;
class startWindowImpl;

class MyAvatarLabel : public QLabel
{
Q_OBJECT
public:
    MyAvatarLabel(QGroupBox*);
    ~MyAvatarLabel();

	void setMyW(gameTableImpl* theValue) { myW = theValue; }
	void setMyId ( int theValue ) {	myId = theValue; }
	void contextMenuEvent ( QContextMenuEvent * event );
	QString getPlayerTip(QString);
	int getPlayerRating(QString);

public slots:
	
	void showContextMenu(const QPoint &pos);
	void sendTriggerVoteOnKickSignal();	
	void setEnabledContextMenu(bool);
	void setVoteOnKickContextMenuEnabled(bool);
	void setVoteRunning ( bool theValue ) {	voteRunning = theValue;	}
        void setPixmap ( const QPixmap &, const bool = FALSE);
	void setPixmapAndCountry ( const QPixmap &, QString country, int seatPlace, const bool = FALSE);
        void setPixmapPath ( const QString theValue) { myPath = theValue; }
        void paintEvent(QPaintEvent*);
        void putPlayerOnIgnoreList();
        bool playerIsOnIgnoreList(QString playerName);
        void reportBadAvatar();
	void startEditTip();
	void startChangePlayerTip(QString playerName);
	void setPlayerTip();
	void setPlayerRating(QString);
	void refreshTooltips();
	void refreshStars();
private: 

	gameTableImpl *myW;
	QMenu *myContextMenu;
	QAction *action_VoteForKick;
        QAction *action_IgnorePlayer;
        QAction *action_ReportBadAvatar;
	QAction *action_EditTip;

        QPixmap myPixmap;
        QString myPath;

	int myId;
	bool myContextMenuEnabled;	
	bool voteRunning;
        bool transparent;
};

#endif
