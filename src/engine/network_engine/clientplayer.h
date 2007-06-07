/***************************************************************************
 *   Copyright (C) 2007 by Lothar May                                      *
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
#ifndef CLIENTPLAYER_H
#define CLIENTPLAYER_H

#include <playerinterface.h>

#include <boost/shared_ptr.hpp>
#include <string>


class CardsValue;
class ConfigFile;
class HandInterface;
class BoardInterface;

class ClientPlayer : public PlayerInterface{
public:
	ClientPlayer(ConfigFile*, BoardInterface *b, int id, unsigned uniqueId, PlayerType type, std::string name, std::string avatar, int sC, bool aS, int mB);
	~ClientPlayer();

	void setHand(HandInterface*);

	int getMyID() const { return myID; }
	unsigned getMyUniqueID() const { return myUniqueID; }
	PlayerType getMyType() const { return myType; }

	void setMyDude(int theValue) { myDude = theValue; }
	int getMyDude() const { return myDude; }

	void setMyDude4(int theValue) { myDude4 = theValue; }
	int getMyDude4() const { return myDude4; }

	void setMyName(const std::string& theValue) { myName = theValue; }
	std::string getMyName() const { return myName; }

	void setMyAvatar(const std::string& theValue) { myAvatar = theValue; }
	std::string getMyAvatar() const { return myAvatar; }

	void setMyCash(int theValue) { myCash = theValue; }
	int getMyCash() const { return myCash; }

	void setMySet(int theValue) { myLastRelativeSet = theValue; }
	void setMySetAbsolute(int theValue) { mySet = theValue; }
	void setMySetNull() { mySet = 0; myLastRelativeSet = 0; }
	int getMySet() const { return mySet;}
	int getMyLastRelativeSet() const { return myLastRelativeSet; }

	void setMyAction(int theValue, bool blind) { myAction = theValue; }
	int getMyAction() const	{ return myAction; }

	void setMyButton(int theValue) { myButton = theValue; }
	int getMyButton() const	{ return myButton; }

	void setMyActiveStatus(bool theValue) { myActiveStatus = theValue; }
	bool getMyActiveStatus() const { return myActiveStatus; }

	void setMyCards(int* theValue) { int i; for(i=0; i<2; i++) myCards[i] = theValue[i]; }
	void getMyCards(int* theValue) { int i; for(i=0; i<2; i++) theValue[i] = myCards[i]; }

	void setMyTurn(bool theValue){ myTurn = theValue;}
	bool getMyTurn() const{ return myTurn;}

	void setMyCardsFlip(bool theValue, int state){ 
		myCardsFlip = theValue;
		// log flipping cards
		if(myCardsFlip) {
			switch(state) {
				case 1: actualHand->getGuiInterface()->logFlipHoleCardsMsg(myName, myCards[0], myCards[1], myCardsValueInt);
				break;
				case 2: actualHand->getGuiInterface()->logFlipHoleCardsMsg(myName, myCards[0], myCards[1]);
				break;
				case 3: actualHand->getGuiInterface()->logFlipHoleCardsMsg(myName, myCards[0], myCards[1], myCardsValueInt, "has");
				break;
				default: ;
			}
		}
	}
	bool getMyCardsFlip() const{ return myCardsFlip;}

	void setMyCardsValueInt(int theValue) { myCardsValueInt = theValue;}
	int getMyCardsValueInt() const { return myCardsValueInt; }

	int* getMyBestHandPosition() { return myBestHandPosition; }

	void setMyRoundStartCash(int theValue) { myRoundStartCash = theValue;}
	int getMyRoundStartCash() const { return myRoundStartCash; }

	void setMyAverageSets(int theValue) { myAverageSets[0] = myAverageSets[1]; myAverageSets[1] = myAverageSets[2]; myAverageSets[2] = myAverageSets[3]; myAverageSets[3] = theValue; }
	int getMyAverageSets() const { return (myAverageSets[0]+myAverageSets[1]+myAverageSets[2]+myAverageSets[3])/4; }
	
	void setMyAggressive(bool theValue) {
		int i;
		for(i=0; i<6; i++) {
			myAggressive[i] = myAggressive[i+1];
		}
		myAggressive[6] = theValue;
	}
	int getMyAggressive() const {
		int i, sum = 0;
		for(i=0; i<7; i++) {
			sum += myAggressive[i];
		}
		return sum;
	}

	void setSBluff ( int theValue ) { sBluff = theValue; }
	int getSBluff() const { return sBluff; }

	void setSBluffStatus ( bool theValue ) { sBluffStatus = theValue; }
	bool getSBluffStatus() const { return sBluffStatus; }

	void setMyWinnerState ( bool theValue, int pot ) {
		myWinnerState = theValue;
		if(theValue) actualHand->getGuiInterface()->logPlayerWinsMsg(myID, pot);	
	}
	bool getMyWinnerState() const { return myWinnerState;}

	void action();
	
	void preflopEngine();
	void flopEngine();
	void turnEngine();
	void riverEngine();

	void preflopEngine3();
	void flopEngine3();
	void turnEngine3();
	void riverEngine3();

	int preflopCardsValue(int*);
	int flopCardsValue(int*);
	int turnCardsValue(int*);

	void readFile();

	void evaluation(int, int);

	void setNetSessionData(boost::shared_ptr<SessionData> session);
	boost::shared_ptr<SessionData> getNetSessionData();

private:

	ConfigFile *myConfig;
	HandInterface *actualHand;
	BoardInterface *actualBoard;

	CardsValue *myCardsValue;

	// Konstanten
	int myID;
	unsigned myUniqueID;
	PlayerType myType;
	std::string myName;
	std::string myAvatar;
	int myDude;
	int myDude4;


	// Laufvariablen
	int myCardsValueInt;
	int myBestHandPosition[5];
	double myOdds;
	int myNiveau[3];

	int myCards[2];
	int myCash;
	int mySet;
	int myLastRelativeSet;
	int myAction; // 0 = none, 1 = fold, 2 = check, 3 = call, 4 = bet, 5 = raise, 6 = allin
	int myButton; // 0 = none, 1 = dealer, 2 =small, 3 = big
	bool myActiveStatus; // 0 = inactive, 1 = active
	bool myTurn; // 0 = no, 1 = yes
	bool myCardsFlip; // 0 = cards are not fliped, 1 = cards are already flipped,
	int myRoundStartCash;

	int myAverageSets[4];
	bool myAggressive[7];

	int sBluff;
	bool sBluffStatus;

	bool myWinnerState;

	boost::shared_ptr<SessionData> myNetSessionData;
};

#endif
