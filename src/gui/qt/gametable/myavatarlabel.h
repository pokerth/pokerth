/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/
#ifndef MYAVATARLABEL_H
#define MYAVATARLABEL_H

#include <iostream>
#include "startwindowimpl.h"
#include <QtGui>
#include <QtCore>
#if QT_VERSION >= 0x050000
#include <QtWidgets>
#endif

class gameTableImpl;
class startWindowImpl;

class MyAvatarLabel : public QLabel
{
	Q_OBJECT
public:
	MyAvatarLabel(QGroupBox*);
	~MyAvatarLabel();

	void setMyW(gameTableImpl* theValue) {
		myW = theValue;
	}
	void setMyId ( int theValue ) {
		myId = theValue;
	}
	void setMyUniqueId ( int theValue ) {
		myUniqueId = theValue;
	}
	void contextMenuEvent ( QContextMenuEvent * event );
	QString getPlayerTip(QString);
	int getPlayerRating(QString);

public slots:

	void showContextMenu(const QPoint &pos);
	void sendTriggerVoteOnKickSignal();
	void setEnabledContextMenu(bool);
	void setVoteOnKickContextMenuEnabled(bool);
	void setVoteRunning ( bool theValue ) {
		voteRunning = theValue;
	}
	void setPixmap ( const QPixmap &, const bool = false);
	void setPixmapAndCountry ( const QPixmap &, QString country, int seatPlace, const bool = false);
	void setPixmapPath ( const QString theValue) {
		myPath = theValue;
	}
	void paintEvent(QPaintEvent*);
	void putPlayerOnIgnoreList();
	void removePlayerFromIgnoreList();
	bool playerIsOnIgnoreList(QString playerName);
	void reportBadAvatar();
	void startEditTip();
	void startChangePlayerTip(QString playerName);
	void setPlayerTip();
	void setPlayerRating(QString);
	void refreshTooltips();
	void refreshStars();
	void refreshPing(unsigned, unsigned, unsigned);

private:

	gameTableImpl *myW;
	QMenu *myContextMenu;
	QAction *action_VoteForKick;
	QAction *action_IgnorePlayer;
	QAction *action_UnignorePlayer;
	QAction *action_ReportBadAvatar;
	QAction *action_EditTip;

	QPixmap myPixmap;
	QString myPath;

	bool myContextMenuEnabled;
	bool voteRunning;
	bool transparent;
	int myId;
	int myUniqueId;
	unsigned myPingState;
	unsigned myAvgPing;
	unsigned myMinPing;
	unsigned myMaxPing;
};

#endif

