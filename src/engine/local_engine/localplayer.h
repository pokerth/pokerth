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
#ifndef LOCALPLAYER_H
#define LOCALPLAYER_H

#include "playerinterface.h"

#include "cardsvalue.h"

#include <string>
#include <sstream>


class CardsValue;
class HandInterface;
class BoardInterface;

class LocalPlayer : public PlayerInterface{
public:
    LocalPlayer(BoardInterface*, int, int, bool, int);

    ~LocalPlayer();

	void setHand(HandInterface*);

	void setMyID(const int& theValue) { myID = theValue; }
	int getMyID() const { return myID; }

	void setMyDude(const int& theValue) { myDude = theValue; }
	int getMyDude() const { return myDude; }

	void setMyName(const std::string& theValue) { myName = theValue; }
	std::string getMyName() const { return myName; }

	void setMyCash(const int& theValue) { myCash = theValue; }
	int getMyCash() const { return myCash; }

	void setMySet(const int& theValue) { mySet += theValue; myCash -= theValue;	}
	void setMySetNull() { mySet = 0; }
	int getMySet() const { return mySet;}

	void setMyAction(const int& theValue) { myAction = theValue; }
	int getMyAction() const	{ return myAction; }

	void setMyButton(const int& theValue) { myButton = theValue; }
	int getMyButton() const	{ return myButton; }

	void setMyActiveStatus(bool theValue) { myActiveStatus = theValue; }
	bool getMyActiveStatus() const { return myActiveStatus; }

	void setMyCards(int* theValue) { int i; for(i=0; i<2; i++) myCards[i] = theValue[i]; }
	void getMyCards(int* theValue) { int i; for(i=0; i<2; i++) theValue[i] = myCards[i]; }

	void setMyTurn(bool theValue){ myTurn = theValue;}
	bool getMyTurn() const{ return myTurn;}

	void setMyCardsValueInt(const int& theValue) { myCardsValueInt = theValue;}
	int getMyCardsValueInt() const { return myCardsValueInt; }

	void setMyRoundStartCash(const int& theValue) { myRoundStartCash = theValue;}
	int getMyRoundStartCash() const { return myRoundStartCash; }

	void setMyAverageSets(const int& theValue) { myAverageSets = theValue; }
	int getMyAverageSets() const { return myAverageSets; }
	
	void action();
	
	void preflopEngine();
	void flopEngine();
	void turnEngine();
	void riverEngine();
	

	


private:
	HandInterface *actualHand;
	BoardInterface *actualBoard;

	CardsValue *myCardsValue;

	// Konstanten
	int myID;
	int myDude;
	std::string myName;

	// Laufvariablen
	int myCards[2];
	int myCardsValueInt;
	int myCash;
	int mySet;
	int myAction; // 0 = none, 1 = fold, 2 = check, 3 = call, 4 = bet, 5 = raise, 6 = allin
	int myButton; // 0 = none, 1 = dealer, 2 =small, 3 = big
	bool myActiveStatus; // 0 = inactive, 1 = active
	bool myTurn; // 0 = no, 1 = yes
	int myRoundStartCash;

	int myAverageSets;

};

#endif
