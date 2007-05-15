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
#ifndef LOCALRIVER_H
#define LOCALRIVER_H

#include <riverinterface.h>

class HandInterface;


class LocalRiver : public RiverInterface{
public:
	LocalRiver(HandInterface*, int, int, int, int);
	~LocalRiver();

	void setPlayersTurn(const int& theValue) { playersTurn = theValue; }
	int getPlayersTurn() const { return playersTurn; }
	
	void setHighestSet(const int& theValue) { highestSet = theValue; }
	int getHighestSet() const { return highestSet;}

	void setFirstRiverRound(bool theValue) { firstRiverRound = theValue;}
	bool getFirstRiverRound() const {  return firstRiverRound;}

	void setSmallBlindPosition(const int& theValue) { smallBlindPosition = theValue;}
	int getSmallBlindPosition() const { return smallBlindPosition; }

	void setSmallBlind(const int& theValue) { smallBlind = theValue; }
	int getSmallBlind() const { return smallBlind; }

	void setHighestCardsValue(const int& theValue) { highestCardsValue = theValue;}
	int getHighestCardsValue() const { return highestCardsValue;}

	void riverRun();
	void postRiverRun();
	void nextPlayer2();
	void distributePot();
	
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
	int playersTurn;

	int highestCardsValue;

};

#endif
