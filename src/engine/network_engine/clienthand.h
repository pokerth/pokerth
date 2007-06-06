/***************************************************************************
 *   Copyright (C) 2007 by Lothar May                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
` *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef CLIENTHAND_H
#define CLIENTHAND_H

#include <enginefactory.h>
#include <guiinterface.h>
#include <boardinterface.h>
#include <playerinterface.h>
#include <handinterface.h>
#include <preflopinterface.h>
#include <flopinterface.h>
#include <turninterface.h>
#include <riverinterface.h>


class ClientHand : public HandInterface
{
public:
	ClientHand(boost::shared_ptr<EngineFactory> f, GuiInterface*, BoardInterface*, PlayerInterface**, int, int, int, int, int, int);
	~ClientHand();

	void start();

	PlayerInterface** getPlayerArray() const { return playerArray; }
	BoardInterface* getBoard() const { return myBoard; }
	PreflopInterface* getPreflop() const { return myPreflop; }
	FlopInterface* getFlop() const { return myFlop; }
	TurnInterface* getTurn() const { return myTurn; }
	RiverInterface* getRiver() const { return myRiver; }
	GuiInterface* getGuiInterface() const { return myGui; }

	void setMyID(int theValue) { myID = theValue; }
	int getMyID() const { return myID; }
	
	void setActualQuantityPlayers(int theValue) { actualQuantityPlayers = theValue; }
	int getActualQuantityPlayers() const { return actualQuantityPlayers; }

	void setStartQuantityPlayers(int theValue) { startQuantityPlayers = theValue; }
	int getStartQuantityPlayers() const { return startQuantityPlayers; }

	void setActualRound(int theValue) { actualRound = theValue; }
	int getActualRound() const { return actualRound; }

	void setDealerPosition(int theValue) { dealerPosition = theValue; }
	int getDealerPosition() const { return dealerPosition; }

	void setSmallBlind(int theValue) { smallBlind = theValue; }
	int getSmallBlind() const { return smallBlind; }

	void setAllInCondition(bool theValue) { allInCondition = theValue; }
	bool getAllInCondition() const { return allInCondition; }

	void setStartCash(int theValue)	{ startCash = theValue; }
	int getStartCash() const { return startCash;	}

	void setActivePlayersCounter(int theValue) { activePlayersCounter = theValue; }
	int getActivePlayersCounter() const { return activePlayersCounter; }
	
	void setBettingRoundsPlayed(int theValue) { bettingRoundsPlayed = theValue; }
	int getBettingRoundsPlayed() const { return bettingRoundsPlayed; }

	void setLastPlayersTurn(int theValue) { lastPlayersTurn = theValue; }
	int getLastPlayersTurn() const { return lastPlayersTurn; }

	void setCardsShown(bool theValue) { cardsShown = theValue; }
	bool getCardsShown() const { return cardsShown; }

	void switchRounds();


private:

	boost::shared_ptr<EngineFactory> myFactory;
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
	bool cardsShown;

	// hier steht bis zu welcher bettingRound der human player gespielt hat: 0 - nur Preflop, 1 - bis Flop, ...
	int bettingRoundsPlayed;
};

#endif


