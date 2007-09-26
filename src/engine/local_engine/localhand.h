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
#ifndef LOCALHAND_H
#define LOCALHAND_H

#include <vector>

#include <boost/shared_ptr.hpp>

#include <enginefactory.h>
#include <guiinterface.h>
#include <boardinterface.h>
#include <playerinterface.h>
#include <handinterface.h>
#include <berointerface.h>


class LocalHand : public HandInterface{
public:
	LocalHand(boost::shared_ptr<EngineFactory> f, GuiInterface*, BoardInterface*, std::vector<boost::shared_ptr<PlayerInterface> >, PlayerList, PlayerList, int, int, unsigned, int, int);
	~LocalHand();

	void start();

	std::vector<boost::shared_ptr<PlayerInterface> > getPlayerArray() const { return playerArray; }
	PlayerList getActivePlayerList() const {return activePlayerList;}
	PlayerList getRunningPlayerList() const {return runningPlayerList;}

	PlayerListIterator getActivePlayerIt(unsigned) const;
	PlayerListIterator getRunningPlayerIt(unsigned) const;

	BoardInterface* getBoard() const { return myBoard; }
	boost::shared_ptr<BeRoInterface> getPreflop() const { return myBeRo[GAME_STATE_PREFLOP]; }
	boost::shared_ptr<BeRoInterface> getFlop() const { return myBeRo[GAME_STATE_FLOP]; }
	boost::shared_ptr<BeRoInterface> getTurn() const { return myBeRo[GAME_STATE_TURN]; }
	boost::shared_ptr<BeRoInterface> getRiver() const { return myBeRo[GAME_STATE_RIVER]; }
	GuiInterface* getGuiInterface() const { return myGui; }
	boost::shared_ptr<BeRoInterface> getCurrentBeRo() const { return myBeRo[actualRound]; }

	void setMyID(int theValue) { myID = theValue; }
	int getMyID() const { return myID; }

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
	
	void setBettingRoundsPlayed(int theValue) { bettingRoundsPlayed = theValue; }
	int getBettingRoundsPlayed() const { return bettingRoundsPlayed; }

	void setLastPlayersTurn(int theValue) { lastPlayersTurn = theValue; }
	int getLastPlayersTurn() const { return lastPlayersTurn; }

	void setCardsShown(bool theValue) { cardsShown = theValue; }
	bool getCardsShown() const { return cardsShown; }

	void assignButtons();

	void switchRounds();


private:

	boost::shared_ptr<EngineFactory> myFactory;
	GuiInterface *myGui;
	BoardInterface *myBoard;

	std::vector<boost::shared_ptr<PlayerInterface> > playerArray;
	PlayerList activePlayerList;
	PlayerList runningPlayerList;

	std::vector<boost::shared_ptr<BeRoInterface> > myBeRo;

	int myID;
	int startQuantityPlayers;
	unsigned dealerPosition; // -1 -> neutral
	int actualRound; //0 = preflop, 1 = flop, 2 = turn, 3 = river
	int smallBlind;
	int startCash;

	int lastPlayersTurn;

	bool allInCondition;
	bool cardsShown;

	// hier steht bis zu welcher bettingRound der human player gespielt hat: 0 - nur Preflop, 1 - bis Flop, ...
	int bettingRoundsPlayed;
};

#endif


