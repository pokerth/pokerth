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
#ifndef HandInterface_H
#define HandInterface_H

#include "enginefactory.h"

#include "handinterface.h"
#include "guiinterface.h"


#include "tools.h"
#include "cardsvalue.h"
#include "localpreflop.h"
#include "localflop.h"
#include "localturn.h" 
#include "localriver.h"



class LocalHand : public HandInterface{
public:
	LocalHand(EngineFactory*, GuiInterface*, BoardInterface*, PlayerInterface**, int, int, int, int, int, int);

	~LocalHand();

	void start();

	PlayerInterface** getPlayerArray() const { return playerArray; }
	BoardInterface* getBoard() const { return myBoard; }
	PreflopInterface* getPreflop() const { return myPreflop; }
	FlopInterface* getFlop() const { return myFlop; }
	TurnInterface* getTurn() const { return myTurn; }
	RiverInterface* getRiver() const { return myRiver; }
	GuiInterface* getGuiInterface() const { return myGui; }

	void setMyID(const int& theValue) { myID = theValue; }
	int getMyID() const { return myID; }
	
	void setActualQuantityPlayers(const int& theValue) { actualQuantityPlayers = theValue; }
	int getActualQuantityPlayers() const { return actualQuantityPlayers; }

	void setStartQuantityPlayers(const int& theValue) { startQuantityPlayers = theValue; }
	int getStartQuantityPlayers() const { return startQuantityPlayers; }

	void setActualRound(const int& theValue) { actualRound = theValue; }
	int getActualRound() const { return actualRound; }

	void setDealerPosition(const int& theValue) { dealerPosition = theValue; }
	int getDealerPosition() const { return dealerPosition; }

	void setSmallBlind(const int& theValue) { smallBlind = theValue; }
	int getSmallBlind() const { return smallBlind; }

	void setAllInCondition(bool theValue) { allInCondition = theValue; }
	bool getAllInCondition() const { return allInCondition; }

	void setStartCash(const int& theValue)	{ startCash = theValue; }
	int getStartCash() const { return startCash;	}

	void setActivePlayersCounter(const int& theValue) { activePlayersCounter = theValue; }
	int getActivePlayersCounter() const { return activePlayersCounter; }
	
	void setBettingRoundsPlayed(const int& theValue) { bettingRoundsPlayed = theValue; }
	int getBettingRoundsPlayed() const { return bettingRoundsPlayed; }

	void setLastPlayersTurn(const int& theValue) { lastPlayersTurn = theValue; }
	int getLastPlayersTurn() const { return lastPlayersTurn; }


	void assignButtons();

	void highlightRoundLabel();
	void switchRounds();
	

	


	



private:

	EngineFactory *myFactory;
	GuiInterface *myGui;
	BoardInterface *myBoard;
	PlayerInterface **playerArray;
	PreflopInterface *myPreflop;
	FlopInterface *myFlop;
	TurnInterface *myTurn;
	RiverInterface *myRiver;

	int myID;
	int actualQuantityPlayers;
	int startQuantityPlayers;
	int dealerPosition; // -1 -> neutral
	int actualRound; //0 = preflop, 1 = flop, 2 = turn, 3 = river
	int smallBlind;
	int startCash;
	int activePlayersCounter;

	int lastPlayersTurn;

	bool allInCondition;

	// hier steht bis zu welcher bettingRound der human player gespielt hat: 0 - nur Preflop, 1 - bis Flop, ...
	int bettingRoundsPlayed;
};

#endif


