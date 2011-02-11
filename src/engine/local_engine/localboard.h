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
#ifndef LOCALBOARD_H
#define LOCALBOARD_H

#include <iostream>
#include <vector>
#include <boost/shared_ptr.hpp>

#include <boardinterface.h>

class PlayerInterface;
class HandInterface;


class LocalBoard : public BoardInterface
{
public:
	LocalBoard(unsigned dealerPosition);
	~LocalBoard();

	void setPlayerLists(PlayerList, PlayerList, PlayerList);

	void setMyCards(int* theValue) {
		int i;
		for(i=0; i<5; i++) myCards[i] = theValue[i];
	}
	void getMyCards(int* theValue) {
		int i;
		for(i=0; i<5; i++) theValue[i] = myCards[i];
	}

	void setAllInCondition(bool theValue) {
		allInCondition = theValue;
	}
	void setLastActionPlayer(unsigned theValue) {
		lastActionPlayer = theValue;
	}

	int getPot() const {
		return pot;
	}
	void setPot(int theValue) {
		pot = theValue;
	}
	int getSets() const {
		return sets;
	}
	void setSets(int theValue) {
		sets = theValue;
	}

	void collectSets() ;
	void collectPot() ;

	void distributePot();
	void determinePlayerNeedToShowCards();

	std::list<unsigned> getWinners() const {
		return winners;
	}
	void setWinners(const std::list<unsigned> &w) {
		winners = w;
	}

	std::list<unsigned> getPlayerNeedToShowCards() const {
		return playerNeedToShowCards;
	}
	void setPlayerNeedToShowCards(const std::list<unsigned> &p) {
		playerNeedToShowCards = p;
	}


private:
	PlayerList seatsList;
	PlayerList activePlayerList;
	PlayerList runningPlayerList;

	std::list<unsigned> winners;
	std::list<unsigned> playerNeedToShowCards;

	int myCards[5];
	int pot;
	int sets;
	unsigned dealerPosition;
	bool allInCondition;
	unsigned lastActionPlayer;

};

#endif
