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

#ifndef LOCALBERO_H
#define LOCALBERO_H


#include "game_defs.h"

#include "berointerface.h"
#include "handinterface.h"

class LocalBeRo : public BeRoInterface
{
public:
	LocalBeRo(HandInterface* hi, unsigned dP, int sB, GameState gS);
	~LocalBeRo();

	GameState getMyBeRoID() const {
		return myBeRoID;
	}

	int getHighestCardsValue() const;
	void setHighestCardsValue(int /*theValue*/) { }

	void setMinimumRaise ( int theValue ) {
		minimumRaise = theValue;
	}
	int getMinimumRaise() const {
		return minimumRaise;
	}

	void setFullBetRule ( bool theValue ) {
		fullBetRule = theValue;
	}
	bool getFullBetRule() const {
		return fullBetRule;
	}

	void skipFirstRunGui() {
		firstRunGui = false;
	}

	void nextPlayer();
	void run();

	void postRiverRun() {};


protected:

	HandInterface* getMyHand() const {
		return myHand;
	}

	int getDealerPosition() const {
		return dealerPosition;
	}
	void setDealerPosition(int theValue) {
		dealerPosition = theValue;
	}

	void setCurrentPlayersTurnId(unsigned theValue) {
		currentPlayersTurnId = theValue;
	}
	unsigned getCurrentPlayersTurnId() const {
		return currentPlayersTurnId;
	}

	void setFirstRoundLastPlayersTurnId(unsigned theValue) {
		firstRoundLastPlayersTurnId = theValue;
	}
	unsigned getFirstRoundLastPlayersTurnId() const {
		return firstRoundLastPlayersTurnId;
	}

	void setCurrentPlayersTurnIt(PlayerListIterator theValue) {
		currentPlayersTurnIt = theValue;
	}
	PlayerListIterator getCurrentPlayersTurnIt() const {
		return currentPlayersTurnIt;
	}

	void setLastPlayersTurnIt(PlayerListIterator theValue) {
		lastPlayersTurnIt = theValue;
	}
	PlayerListIterator getLastPlayersTurnIt() const {
		return lastPlayersTurnIt;
	}

	void setHighestSet(int theValue) {
		highestSet = theValue;
	}
	int getHighestSet() const {
		return highestSet;
	}

	void setFirstRun(bool theValue) {
		firstRun = theValue;
	}
	bool getFirstRun() const {
		return firstRun;
	}

	void setFirstRound(bool theValue) {
		firstRound = theValue;
	}
	bool getFirstRound() const {
		return firstRound;
	}


	void setDealerPositionId(unsigned theValue) {
		dealerPositionId = theValue;
	}
	unsigned getDealerPositionId() const {
		return dealerPositionId;
	}

	void setSmallBlindPositionId(unsigned theValue) {
		smallBlindPositionId = theValue;
	}
	unsigned getSmallBlindPositionId() const {
		return smallBlindPositionId;
	}

	void setBigBlindPositionId(unsigned theValue) {
		bigBlindPositionId = theValue;
	}
	unsigned getBigBlindPositionId() const {
		return bigBlindPositionId;
	}


	void setSmallBlindPosition(int theValue) {
		smallBlindPosition = theValue;
	}
	int getSmallBlindPosition() const {
		return smallBlindPosition;
	}

	void setSmallBlind(int theValue) {
		smallBlind = theValue;
	}
	int getSmallBlind() const {
		return smallBlind;
	}




private:

	HandInterface* myHand;

	const GameState myBeRoID;
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

	PlayerListIterator currentPlayersTurnIt; // iterator for runningPlayerList
	PlayerListIterator lastPlayersTurnIt; // iterator for runningPlayerList

	unsigned currentPlayersTurnId;
	unsigned firstRoundLastPlayersTurnId;

	bool logBoardCardsDone;


};

#endif
