/***************************************************************************
 *   Copyright (C) 2009 by Felix Hammer   *
 *   f.hammer@web.de   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef GAMETABLESTYLEREADER_H
#define GAMETABLESTYLEREADER_H

#include "tinyxml.h"
#include "gametableimpl.h"
#include "configfile.h"
#include <string>
#include <QtCore>
#include <QtGui>

class GameTableStyleReader : public QObject {
Q_OBJECT
public:
	GameTableStyleReader(ConfigFile *c, gameTableImpl *w =0 );
	~GameTableStyleReader();
	
	void readStyleFile(QString);

	void showErrorMessage(QString, QStringList, QString);

	QString getStyleDescription() const { return StyleDescription; }
	QString getPreview() const { return Preview; }
	
	
	

private:

	QString StyleDescription;
	QString StyleMaintainerEMail;
	QString Preview;
	QString IfFixedWindowSize;
	QString FixedWindowWidth;
	QString FixedWindowHeight;
	QString MinimumWindowWidth;
	QString MinimumWindowHeight;
	QString MaximumWindowWidth;
	QString MaximumWindowHeight;
	QString ActionAllIn;
	QString ActionRaise;
	QString ActionBet;
	QString ActionCall;
	QString ActionCheck;
	QString ActionFold;
	QString ActionWinner;
	QString BigBlindPuck;
	QString SmallBlindPuck;
	QString DealerPuck;
	QString DefaultAvatar;
	QString CardHolderFlop;
	QString CardHolderTurn;
	QString CardHolderRiver;
	QString FoldButtonDefault;
	QString FoldButtonHover;
	QString FoldButtonChecked;
	QString FoldButtonCheckedHover;
	QString CheckCallButtonDefault;
	QString CheckCallButtonHover;
	QString CheckCallButtonChecked;
	QString CheckCallButtonCheckedHover;
	QString BetRaiseButtonDefault;
	QString BetRaiseButtonHover;
	QString BetRaiseButtonChecked;
	QString BetRaiseButtonCheckedHover;
	QString AllInButtonDefault;
	QString AllInButtonHover;
	QString AllInButtonChecked;
	QString AllInButtonCheckedHover;
	QString PlayerSeatInActive;
	QString PlayerSeatActive;
	QString Table;

	QString currentFileName;
	QString currentFileDir;

	QStringList wrongItems;

	ConfigFile *myConfig;
	gameTableImpl *myW;
};

#endif
