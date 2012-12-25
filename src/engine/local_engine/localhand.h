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

#ifndef LOCALHAND_H
#define LOCALHAND_H

#include <enginefactory.h>
#include <guiinterface.h>
#include <boardinterface.h>
#include <playerinterface.h>
#include <handinterface.h>
#include <berointerface.h>

#include <vector>

class Log;

class LocalHand : public HandInterface
{
public:
	LocalHand(boost::shared_ptr<EngineFactory> f, GuiInterface*, boost::shared_ptr<BoardInterface>, Log*, PlayerList, PlayerList, PlayerList, int, int, unsigned, int, int);
	~LocalHand();

	void start();

	PlayerList getSeatsList() const {
		return seatsList;
	}
	PlayerList getActivePlayerList() const {
		return activePlayerList;
	}
	PlayerList getRunningPlayerList() const {
		return runningPlayerList;
	}

	boost::shared_ptr<BoardInterface> getBoard() const {
		return myBoard;
	}
	boost::shared_ptr<BeRoInterface> getPreflop() const {
		return myBeRo[GAME_STATE_PREFLOP];
	}
	boost::shared_ptr<BeRoInterface> getFlop() const {
		return myBeRo[GAME_STATE_FLOP];
	}
	boost::shared_ptr<BeRoInterface> getTurn() const {
		return myBeRo[GAME_STATE_TURN];
	}
	boost::shared_ptr<BeRoInterface> getRiver() const {
		return myBeRo[GAME_STATE_RIVER];
	}
	GuiInterface* getGuiInterface() const {
		return myGui;
	}
	boost::shared_ptr<BeRoInterface> getCurrentBeRo() const {
		return myBeRo[currentRound];
	}

	Log* getLog() const {
		return myLog;
	}

	void setMyID(int theValue) {
		myID = theValue;
	}
	int getMyID() const {
		return myID;
	}

	void setStartQuantityPlayers(int theValue) {
		startQuantityPlayers = theValue;
	}
	int getStartQuantityPlayers() const {
		return startQuantityPlayers;
	}

	void setCurrentRound(GameState theValue) {
		currentRound = theValue;
		if(myLog) myLog->setCurrentRound(currentRound);
	}
	GameState getCurrentRound() const {
		return currentRound;
	}
	GameState getRoundBeforePostRiver() const {
		return roundBeforePostRiver;
	}

	void setDealerPosition(int theValue) {
		dealerPosition = theValue;
	}
	int getDealerPosition() const {
		return dealerPosition;
	}

	void setSmallBlind(int theValue) {
		smallBlind = theValue;
	}
	int getSmallBlind() const {
		return smallBlind;
	}

	void setAllInCondition(bool theValue) {
		allInCondition = theValue;
	}
	bool getAllInCondition() const {
		return allInCondition;
	}

	void setStartCash(int theValue)	{
		startCash = theValue;
	}
	int getStartCash() const {
		return startCash;
	}

	void setPreviousPlayerID(int theValue) {
		previousPlayerID = theValue;
	}
	int getPreviousPlayerID() const {
		return previousPlayerID;
	}

	void setLastActionPlayerID ( unsigned theValue );
	unsigned getLastActionPlayerID() const {
		return lastActionPlayerID;
	}

	void setCardsShown(bool theValue) {
		cardsShown = theValue;
	}
	bool getCardsShown() const {
		return cardsShown;
	}

	void assignButtons();
	void setBlinds();

	void switchRounds();

protected:
	PlayerListIterator getSeatIt(unsigned) const;
	PlayerListIterator getActivePlayerIt(unsigned) const;
	PlayerListIterator getRunningPlayerIt(unsigned) const;

private:

	boost::shared_ptr<EngineFactory> myFactory;
	GuiInterface *myGui;
	boost::shared_ptr<BoardInterface> myBoard;
	Log *myLog;

	PlayerList seatsList; // all player
	PlayerList activePlayerList; // all player who are not out
	PlayerList runningPlayerList; // all player who are not folded, not all in and not out

	std::vector<boost::shared_ptr<BeRoInterface> > myBeRo;

	int myID;
	int startQuantityPlayers;
	unsigned dealerPosition;
	unsigned smallBlindPosition;
	unsigned bigBlindPosition;
	GameState currentRound;
	GameState roundBeforePostRiver;
	int smallBlind;
	int startCash;

	int previousPlayerID;
	unsigned lastActionPlayerID;

	bool allInCondition;
	bool cardsShown;
};

#endif


