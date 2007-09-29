//
// C++ Interface: localbero
//
// Description: 
//
//
// Author: FThauer FHammer <webmaster@pokerth.net>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef LOCALBERO_H
#define LOCALBERO_H


#include <iostream>
#include "game_defs.h"

#include "berointerface.h"
#include "handinterface.h"

/**
	@author FThauer FHammer <webmaster@pokerth.net>
*/
class LocalBeRo : public BeRoInterface{
public:
	LocalBeRo(HandInterface* hi, int id, unsigned dP, int sB, GameState gS);
	~LocalBeRo();

	GameState getMyBeRoID() const { return myBeRoID; }

	int getHighestCardsValue() const { std::cout << "getHighestCardsValue() in wrong BeRo" << std::endl; return 0; }
	void setHighestCardsValue(int /*theValue*/) { }

	void setMinimumRaise ( int theValue ) { minimumRaise = theValue; }
	int getMinimumRaise() const { return minimumRaise; }

	void resetFirstRun() { firstRun = false; }

	void nextPlayer();
	void run();

	void postRiverRun() {};
	

protected:

	HandInterface* getMyHand() const { return myHand; }

	int getDealerPosition() const {return dealerPosition; }
	void setDealerPosition(int theValue) { dealerPosition = theValue; }

	void setPlayersTurn(int theValue) { playersTurn = theValue; }
	int getPlayersTurn() const { return playersTurn;}

	void setCurrentPlayersTurnId(unsigned theValue) { currentPlayersTurnId = theValue; }
	unsigned getCurrentPlayersTurnId() const { return currentPlayersTurnId;}

	void setFirstRoundLastPlayersTurnId(unsigned theValue) { firstRoundLastPlayersTurnId = theValue; }
	unsigned getFirstRoundLastPlayersTurnId() const { return firstRoundLastPlayersTurnId;}

	void setCurrentPlayersTurnIt(PlayerListIterator theValue) { currentPlayersTurnIt = theValue; }
	PlayerListIterator getCurrentPlayersTurnIt() const { return currentPlayersTurnIt; }

	void setLastPlayersTurnIt(PlayerListIterator theValue) { lastPlayersTurnIt = theValue; }
	PlayerListIterator getLastPlayersTurnIt() const { return lastPlayersTurnIt; }
	
	void setHighestSet(int theValue) { highestSet = theValue; }
	int getHighestSet() const { return highestSet;}

	void setFirstRun(bool theValue) { firstRun = theValue;}
	bool getFirstRun() const {  return firstRun;}

	void setFirstRound(bool theValue) { firstRound = theValue;}
	bool getFirstRound() const {  return firstRound;}


	void setDealerPositionId(unsigned theValue) { dealerPositionId = theValue;}
	unsigned getDealerPositionId() const { return dealerPositionId; }

	void setSmallBlindPositionId(unsigned theValue) { smallBlindPositionId = theValue;}
	unsigned getSmallBlindPositionId() const { return smallBlindPositionId; }

	void setBigBlindPositionId(unsigned theValue) { bigBlindPositionId = theValue;}
	unsigned getBigBlindPositionId() const { return bigBlindPositionId; }


	void setSmallBlindPosition(int theValue) { smallBlindPosition = theValue;}
	int getSmallBlindPosition() const { return smallBlindPosition; }

	void setSmallBlind(int theValue) { smallBlind = theValue; }
	int getSmallBlind() const { return smallBlind; }

	void setLastActionPlayer ( int theValue ) { lastActionPlayer = theValue; }
	int getLastActionPlayer() const { return lastActionPlayer; }
	



private:

	HandInterface* myHand;

	const GameState myBeRoID;
	int myID;
	int dealerPosition;
	int smallBlindPosition;

	unsigned dealerPositionId;
	unsigned smallBlindPositionId;
	unsigned bigBlindPositionId;


	int smallBlind;
	int highestSet;
	int minimumRaise;

	bool firstRun;
	bool firstRound;
	bool firstHeadsUpRound;

	int playersTurn; // TODO -> delete
	PlayerListIterator currentPlayersTurnIt; // iterator for runningPlayerList
	PlayerListIterator lastPlayersTurnIt; // iterator for runningPlayerList

	unsigned currentPlayersTurnId;
	unsigned firstRoundLastPlayersTurnId;

	bool logBoardCardsDone;

	int lastActionPlayer;


};

#endif
