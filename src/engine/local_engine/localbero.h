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

#include "berointerface.h"
#include "handinterface.h"

/**
	@author FThauer FHammer <webmaster@pokerth.net>
*/
class LocalBeRo : public BeRoInterface{
public:
	LocalBeRo(HandInterface* hi, int id, int qP, int dP, int sB, GameState gS);
	~LocalBeRo();

	GameState getMyBeRoID() const { return myBeRoID; }

	int getHighestCardsValue() const { std::cout << "getHighestCardsValue() in wrong BeRo" << std::endl; return 0; }
	void setHighestCardsValue(int theValue) { }

	void resetFirstRun() { firstRun = false; }

	void nextPlayer();
	void run();

protected:

	HandInterface* getMyHand() const { return myHand; }

	int getDealerPosition() const {return dealerPosition; }
	void setDealerPosition(int theValue) { dealerPosition = theValue; }

	void setPlayersTurn(int theValue) { playersTurn = theValue; }
	int getPlayersTurn() const { return playersTurn; }
	
	void setHighestSet(int theValue) { highestSet = theValue; }
	int getHighestSet() const { return highestSet;}

	void setFirstRound(bool theValue) { firstRound = theValue;}
	bool getFirstRound() const {  return firstRound;}

	void setSmallBlindPosition(int theValue) { smallBlindPosition = theValue;}
	int getSmallBlindPosition() const { return smallBlindPosition; }

	void setSmallBlind(int theValue) { smallBlind = theValue; }
	int getSmallBlind() const { return smallBlind; }
	

private:

	HandInterface* myHand;

	const GameState myBeRoID;
	int myID;
	int actualQuantityPlayers;
	int dealerPosition;
	int smallBlindPosition;

	int smallBlind;
	int highestSet;

	bool firstRun;
	bool firstRound;
	bool firstHeadsUpRound;
	int playersTurn;

	bool logBoardCardsDone;


};

#endif
