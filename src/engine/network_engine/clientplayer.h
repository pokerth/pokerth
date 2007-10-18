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
#include <boost/thread.hpp>
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

	int getMyID() const;
	unsigned getMyUniqueID() const;
	PlayerType getMyType() const;

	void setMyDude(int theValue);
	int getMyDude() const;

	void setMyDude4(int theValue);
	int getMyDude4() const;

	void setMyName(const std::string& theValue);
	std::string getMyName() const;

	void setMyAvatar(const std::string& theValue);
	std::string getMyAvatar() const;

	void setMyCash(int theValue);
	int getMyCash() const;

	void setMySet(int theValue);
	void setMySetAbsolute(int theValue);
	void setMySetNull();
	int getMySet() const;
	int getMyLastRelativeSet() const;

	void setMyAction(int theValue, bool blind);
	int getMyAction() const;

	void setMyButton(int theValue);
	int getMyButton() const;

	void setMyActiveStatus(bool theValue);
	bool getMyActiveStatus() const;

	void setMyCards(int* theValue);
	void getMyCards(int* theValue) const;

	void setMyTurn(bool theValue);
	bool getMyTurn() const;

	void setMyCardsFlip(bool theValue, int state);
	bool getMyCardsFlip() const;

	void setMyCardsValueInt(int theValue);
	int getMyCardsValueInt() const;

	void setMyBestHandPosition(int* theValue);
	void getMyBestHandPosition(int* theValue) const;

	void setMyRoundStartCash(int theValue);
	int getMyRoundStartCash() const;

	void setMyAverageSets(int theValue);
	int getMyAverageSets() const;

	void setMyAggressive(bool theValue);
	int getMyAggressive() const;

	void setSBluff (int theValue);
	int getSBluff() const;

	void setSBluffStatus (bool theValue);
	bool getSBluffStatus() const;

	void setMyWinnerState (bool theValue, int pot);
	bool getMyWinnerState() const;

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

	int preflopCardsValue(int*);
	int flopCardsValue(int*);
	int turnCardsValue(int*);

	void readFile();

	void evaluation(int, int);

	void setNetSessionData(boost::shared_ptr<SessionData> session);
	boost::shared_ptr<SessionData> getNetSessionData();

private:
	mutable boost::recursive_mutex m_syncMutex;

	ConfigFile *myConfig;
	HandInterface *currentHand;
	BoardInterface *currentBoard;

	CardsValue *myCardsValue;

	// Konstanten
	const int myID;
	const unsigned myUniqueID;
	const PlayerType myType;
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
