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
#ifndef LOCALBEROPOSTRIVER_H
#define LOCALBEROPOSTRIVER_H

#include <iostream>
#include <localbero.h>

class HandInterface;


class LocalBeRoPostRiver : public LocalBeRo{
public:
	LocalBeRoPostRiver(HandInterface*, int, int, int, int);
	~LocalBeRoPostRiver();

	void setPlayersTurn(int theValue) { playersTurn = theValue; }
	int getPlayersTurn() const { return playersTurn; }
	
	void setHighestSet(int theValue) { highestSet = theValue; }
	int getHighestSet() const { return highestSet;}

	void setFirstRiverRound(bool theValue) { firstRiverRound = theValue;}
	bool getFirstRiverRound() const {  return firstRiverRound;}

	void setSmallBlindPosition(int theValue) { smallBlindPosition = theValue;}
	int getSmallBlindPosition() const { return smallBlindPosition; }

	void setSmallBlind(int theValue) { smallBlind = theValue; }
	int getSmallBlind() const { return smallBlind; }

	void setHighestCardsValue(int theValue) { highestCardsValue = theValue;}
	int getHighestCardsValue() const { return highestCardsValue;}

	void resetFirstRun() { firstRiverRun = false; }

	void riverRun();
	void postRiverRun();
	void nextPlayer2();
	void distributePot();

	//only until bero refactory is over
	void preflopRun() {}
	void turnRun() {}
	void flopRun() {}
	
private:

	HandInterface *myHand;

	int myID;
	int actualQuantityPlayers;	
	int dealerPosition;
	int smallBlindPosition;

	int smallBlind;
	int highestSet;

	bool firstRiverRun;
	bool firstRiverRound;
	bool firstHeadsUpRiverRound;
	int playersTurn;

	int highestCardsValue;

	bool logBoardCardsDone;

};

#endif
