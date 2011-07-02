/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2011 Felix Hammer, Florian Thauer, Lothar May          *
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
 *****************************************************************************/ #ifndef MYCHANCELABEL_H
#define MYCHANCELABEL_H

#include <vector>

#include <QtGui>
#include <QtCore>

class gameTableImpl;
class GameTableStyleReader;

class MyChanceLabel : public QLabel
{
	Q_OBJECT
public:
	MyChanceLabel(QWidget*);

	~MyChanceLabel();

	void setMyW ( gameTableImpl* theValue ) {
		myW = theValue;
	}
	void setMyStyle ( GameTableStyleReader* theValue ) {
		myStyle = theValue;
	}
	void paintEvent(QPaintEvent * event);
	void refreshChance(std::vector< std::vector<int> >);
	void resetChance();

private:

	gameTableImpl *myW;
	GameTableStyleReader *myStyle;
	int RFChance[2];
	int SFChance[2];
	int FOAKChance[2];
	int FHChance[2];
	int FLChance[2];
	int STRChance[2];
	int TOAKChance[2];
	int TPChance[2];
	int OPChance[2];
	int HCChance[2];
};

#endif
