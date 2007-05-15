/***************************************************************************
 *   Copyright (C) 2006 by FThauer FHammer   *
 *   f.thauer@web.de   *
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
#ifndef LOCALTURN_H
#define LOCALTURN_H

#include <turninterface.h>

class HandInterface;

class LocalTurn : public TurnInterface{
public:
	LocalTurn(HandInterface*, int, int, int, int);
	~LocalTurn();

	void setPlayersTurn(const int& theValue) { playersTurn = theValue; }
	int getPlayersTurn() const { return playersTurn; }
	
	void setHighestSet(const int& theValue) { highestSet = theValue; }
	int getHighestSet() const { return highestSet;}

	void setFirstTurnRound(bool theValue) { firstTurnRound = theValue;}
	bool getFirstTurnRound() const {  return firstTurnRound;}

	void setSmallBlindPosition(const int& theValue) { smallBlindPosition = theValue;}
	int getSmallBlindPosition() const { return smallBlindPosition; }

	void setSmallBlind(const int& theValue) { smallBlind = theValue; }
	int getSmallBlind() const { return smallBlind; }

	void turnRun();
	void nextPlayer2();

private:

	HandInterface *myHand;

	int myID;
	int actualQuantityPlayers;	
	int dealerPosition;
	int smallBlindPosition;

	int smallBlind;
	int highestSet;

	bool firstTurnRun;
	bool firstTurnRound;
	int playersTurn;

};

#endif
