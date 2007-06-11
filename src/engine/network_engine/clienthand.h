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

#include <boost/thread.hpp>

class ClientHand : public HandInterface
{
public:
	ClientHand(boost::shared_ptr<EngineFactory> f, GuiInterface*, BoardInterface*, PlayerInterface**, int, int, int, int, int, int);
	~ClientHand();

	void start();

	PlayerInterface** getPlayerArray() const;
	BoardInterface* getBoard() const;
	PreflopInterface* getPreflop() const;
	FlopInterface* getFlop() const;
	TurnInterface* getTurn() const;
	RiverInterface* getRiver() const;
	GuiInterface* getGuiInterface() const;

	void setMyID(int theValue);
	int getMyID() const;
	
	void setActualQuantityPlayers(int theValue);
	int getActualQuantityPlayers() const;

	void setStartQuantityPlayers(int theValue);
	int getStartQuantityPlayers() const;

	void setActualRound(int theValue);
	int getActualRound() const;

	void setDealerPosition(int theValue);
	int getDealerPosition() const;

	void setSmallBlind(int theValue);
	int getSmallBlind() const;

	void setAllInCondition(bool theValue);
	bool getAllInCondition() const;

	void setStartCash(int theValue);
	int getStartCash() const;

	void setActivePlayersCounter(int theValue);
	int getActivePlayersCounter() const;
	
	void setBettingRoundsPlayed(int theValue);
	int getBettingRoundsPlayed() const;

	void setLastPlayersTurn(int theValue);
	int getLastPlayersTurn() const;

	void setCardsShown(bool theValue);
	bool getCardsShown() const;

	void switchRounds();


private:
	mutable boost::recursive_mutex m_syncMutex;

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


