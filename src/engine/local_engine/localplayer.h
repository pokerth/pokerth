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

#ifndef LOCALPLAYER_H
#define LOCALPLAYER_H

#include <playerinterface.h>

#include <boost/shared_ptr.hpp>
#include <string>


class ConfigFile;
class HandInterface;

class LocalPlayer : public PlayerInterface
{
public:
	LocalPlayer(ConfigFile*, int id, unsigned uniqueId, PlayerType type, std::string name, std::string avatar, int sC, bool aS, bool sotS, int mB);

	~LocalPlayer();

	void setHand(HandInterface *);

	int getMyID() const {
		return myID;
	}
	unsigned getMyUniqueID() const {
		return myUniqueID;
	}
	void setMyUniqueID(unsigned newId) {
		myUniqueID = newId;
	}

	void setMyGuid(const std::string &theValue) {
		myGuid = theValue;
	}

	std::string getMyGuid() const {
		return myGuid;
	}

	PlayerType getMyType() const {
		return myType;
	}

	void setMyDude(int theValue) {
		myDude = theValue;
	}
	int getMyDude() const {
		return myDude;
	}

	void setMyDude4(int theValue) {
		myDude4 = theValue;
	}
	int getMyDude4() const {
		return myDude4;
	}

	void setMyName(const std::string& theValue) {
		myName = theValue;
	}
	std::string getMyName() const {
		return myName;
	}

	void setMyAvatar(const std::string& theValue) {
		myAvatar = theValue;
	}
	std::string getMyAvatar() const {
		return myAvatar;
	}

	void setMyCash(int theValue) {
		myCash = theValue;
	}
	int getMyCash() const {
		return myCash;
	}

	void setMySet(int theValue) {
		myLastRelativeSet = theValue;
		mySet += theValue;
		myCash -= theValue;
	}
	void setMySetAbsolute(int theValue) {
		mySet = theValue;
	}
	void setMySetNull() {
		mySet = 0;
		myLastRelativeSet = 0;
	}
	int getMySet() const {
		return mySet;
	}
	int getMyLastRelativeSet() const {
		return myLastRelativeSet;
	}

	void setMyAction(PlayerAction theValue, bool human = 0) {
		myAction = theValue;
		// logging for seat 0
		if(myAction && human) currentHand->getGuiInterface()->logPlayerActionMsg(myName, myAction, myLastRelativeSet);
	}
	PlayerAction getMyAction() const	{
		return myAction;
	}

	void setMyButton(int theValue) {
		myButton = theValue;
	}
	int getMyButton() const	{
		return myButton;
	}

	void setMyActiveStatus(bool theValue) {
		myActiveStatus = theValue;
	}
	bool getMyActiveStatus() const {
		return myActiveStatus;
	}

	void setMyStayOnTableStatus(bool theValue) {
		myStayOnTableStatus = theValue;
	}
	bool getMyStayOnTableStatus() const {
		return myStayOnTableStatus;
	}

	void setMyCards(int* theValue) {
		int i;
		for(i=0; i<2; i++) myCards[i] = theValue[i];
	}
	void getMyCards(int* theValue) const {
		int i;
		for(i=0; i<2; i++) theValue[i] = myCards[i];
	}

	void setMyTurn(bool theValue) {
		myTurn = theValue;
	}
	bool getMyTurn() const {
		return myTurn;
	}

	void setMyCardsFlip(bool theValue, int state) {
		myCardsFlip = theValue;
		// log flipping cards
		if(myCardsFlip) {
			switch(state) {
			case 1:
				currentHand->getGuiInterface()->logFlipHoleCardsMsg(myName, myCards[0], myCards[1], myCardsValueInt);
				break;
			case 2:
				currentHand->getGuiInterface()->logFlipHoleCardsMsg(myName, myCards[0], myCards[1]);
				break;
			case 3:
				currentHand->getGuiInterface()->logFlipHoleCardsMsg(myName, myCards[0], myCards[1], myCardsValueInt, "has");
				break;
			default:
				;
			}
		}
	}
	bool getMyCardsFlip() const {
		return myCardsFlip;
	}

	void setMyCardsValueInt(int theValue) {
		myCardsValueInt = theValue;
	}
	int getMyCardsValueInt() const {
		return myCardsValueInt;
	}

	void setLogHoleCardsDone(bool theValue) {
		logHoleCardsDone = theValue;
	}

	bool getLogHoleCardsDone() const {
		return logHoleCardsDone;
	}

	void setMyBestHandPosition(int* theValue) {
		for (int i = 0; i < 5; i++)
			myBestHandPosition[i] = theValue[i];
	}
	void getMyBestHandPosition(int* theValue) const {
		for (int i = 0; i < 5; i++)
			theValue[i] = myBestHandPosition[i];
	}

	void setMyRoundStartCash(int theValue) {
		myRoundStartCash = theValue;
	}
	int getMyRoundStartCash() const {
		return myRoundStartCash;
	}

	void setLastMoneyWon ( int theValue ) {
		lastMoneyWon = theValue;
	}
	int getLastMoneyWon() const {
		return lastMoneyWon;
	}

	void setMyAverageSets(int theValue) {
		myAverageSets[0] = myAverageSets[1];
		myAverageSets[1] = myAverageSets[2];
		myAverageSets[2] = myAverageSets[3];
		myAverageSets[3] = theValue;
	}
	int getMyAverageSets() const {
		return (myAverageSets[0]+myAverageSets[1]+myAverageSets[2]+myAverageSets[3])/4;
	}

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

	void setSBluff ( int theValue ) {
		sBluff = theValue;
	}
	int getSBluff() const {
		return sBluff;
	}

	void setSBluffStatus ( bool theValue ) {
		sBluffStatus = theValue;
	}
	bool getSBluffStatus() const {
		return sBluffStatus;
	}

	void action();

	int checkMyAction(int targetAction, int targetBet, int highestSet, int minimumRaise, int smallBlind);

	void preflopEngine();
	void flopEngine();
	void turnEngine();
	void riverEngine();

	void preflopEngine3();
	void flopEngine3();
	void turnEngine3();
	void riverEngine3();

	int flopCardsValue(int*);
	int turnCardsValue(int*);

	void calcMyOdds();

	void evaluation(int, int);

	void setIsSessionActive(bool active);
	bool isSessionActive() const;
	void setIsKicked(bool kicked);
	bool isKicked() const;
	void setIsMuted(bool muted);
	bool isMuted() const;

	bool checkIfINeedToShowCards();

	void markRemoteAction();
	unsigned getTimeSecSinceLastRemoteAction() const;

private:

	ConfigFile *myConfig;
	HandInterface *currentHand;

	// Konstanten
	int myID;
	unsigned myUniqueID;
	std::string myGuid;
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
	bool logHoleCardsDone;

	int myCards[2];
	int myCash;
	int mySet;
	int myLastRelativeSet;
	PlayerAction myAction;
	int myButton; // 0 = none, 1 = dealer, 2 =small, 3 = big
	bool myActiveStatus; // 0 = inactive, 1 = active
	bool myStayOnTableStatus; // 0 = left, 1 = stay
	bool myTurn; // 0 = no, 1 = yes
	bool myCardsFlip; // 0 = cards are not fliped, 1 = cards are already flipped,
	int myRoundStartCash;
	int lastMoneyWon;

	int myAverageSets[4];
	bool myAggressive[7];

	int sBluff;
	bool sBluffStatus;

	unsigned m_actionTimeoutCounter;
	bool m_isSessionActive;
	bool m_isKicked;
	bool m_isMuted;
	boost::timers::portable::microsec_timer m_lastRemoteActionTimer;
};

#endif
