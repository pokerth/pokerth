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
#ifndef LOCALBERO_H
#define LOCALBERO_H


#include "game_defs.h"

#include "berointerface.h"
#include "handinterface.h"

class LocalBeRo : public BeRoInterface{
public:
	LocalBeRo(HandInterface* hi, int id, unsigned dP, int sB, GameState gS);
	~LocalBeRo();

	GameState getMyBeRoID() const { return myBeRoID; }

	int getHighestCardsValue() const;
	void setHighestCardsValue(int /*theValue*/) { }

	void setMinimumRaise ( int theValue ) { minimumRaise = theValue; }
	int getMinimumRaise() const { return minimumRaise; }

        void setFullBetRule ( bool theValue ) { fullBetRule = theValue; }
        bool getFullBetRule() const { return fullBetRule; }

	void skipFirstRunGui() { firstRunGui = false; }

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
        bool fullBetRule;

	bool firstRun;
	bool firstRunGui; // HACK
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
