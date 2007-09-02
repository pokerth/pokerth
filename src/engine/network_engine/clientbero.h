//
// C++ Interface: clientbero
//
// Description: 
//
//
// Author: FThauer FHammer <webmaster@pokerth.net>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef CLIENTBERO_H
#define CLIENTBERO_H

#include <boost/thread.hpp>
#include "berointerface.h"

class HandInterface;

/**
	@author FThauer FHammer <webmaster@pokerth.net>
*/
class ClientBeRo : public BeRoInterface{
public:
	ClientBeRo(HandInterface* hi, int id, int qP, int dP, int sB, GameState gS);
	~ClientBeRo();

	GameState getMyBeRoID() const;

	int getHighestCardsValue() const;
	void setHighestCardsValue(int theValue);

	void setLastActionPlayer ( int theValue );
	int getLastActionPlayer() const;

	void setPlayersTurn(int theValue);
	int getPlayersTurn() const;
	
	void setHighestSet(int theValue);
	int getHighestSet() const;

	void setFirstRound(bool theValue);
	bool getFirstRound() const;

	void setSmallBlindPosition(int theValue);
	int getSmallBlindPosition() const;

	void setSmallBlind(int theValue);
	int getSmallBlind() const;

	void setMinimumRaise ( int theValue );
	int getMinimumRaise() const;


	void resetFirstRun();

	void nextPlayer();
	void run();

	void postRiverRun();

private:

	const GameState myBeRoID;

	mutable boost::recursive_mutex m_syncMutex;

	HandInterface *myHand;

	int highestCardsValue;
	int playersTurn;
	int highestSet;
	bool firstRound;
	int smallBlindPosition;
	int smallBlind;

	int minimumRaise;

	int lastActionPlayer;
};

#endif
