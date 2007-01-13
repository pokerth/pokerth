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
 *************************************************************************/
#ifndef LOCALPREFLOP_H
#define LOCALPREFLOP_H

#include <iostream>

class LocalHand;

class LocalPreflop {
 
public:
    LocalPreflop(LocalHand*, int, int, int, int);

    ~LocalPreflop();


	void setPlayersTurn(const int& theValue) { playersTurn = theValue; }
	int getPlayersTurn() const { return playersTurn; }
	
	void setHighestSet(const int& theValue) { highestSet = theValue; }
	int getHighestSet() const { return highestSet;}

	void setLocalPreflopFirstRound(bool theValue) { preflopFirstRound = theValue; }
	bool setLocalPreflopFirstRound() const { return preflopFirstRound; }

	void preflopRun();
	void nextPlayer2(); 


private:
	LocalHand *myHand;

	int myID;
	int actualQuantityPlayers;	
	int dealerPosition;
	int bigBlindPosition;

	int smallBlind;
	int highestSet;

	bool preflopFirstRound;
	int playersTurn;


};

#endif
