/***************************************************************************
 *   Copyright (C) 2007 by Lothar May                                      *
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
#ifndef CLIENTBOARD_H
#define CLIENTBOARD_H

#include <boardinterface.h>
#include <boost/thread.hpp>
#include <vector>


class PlayerInterface;
class HandInterface;


class ClientBoard : public BoardInterface
{
public:
    ClientBoard(unsigned dealerPosition);
	~ClientBoard();

	void setPlayerLists(PlayerList, PlayerList, PlayerList);
	void setHand(HandInterface*);

	void setMyCards(int* theValue);
	void getMyCards(int* theValue);

	int getPot() const;
	void setPot(int theValue);
	int getSets() const;
	void setSets(int theValue);

    void setAllInCondition(bool theValue);
    void setLastActionPlayer(unsigned theValue);

	void collectSets();
	void collectPot();

	void distributePot();
        void determinePlayerNeedToShowCards();

	std::list<unsigned> getWinners() const;
	void setWinners(const std::list<unsigned> &winners);

        std::list<unsigned> getPlayerNeedToShowCards() const;
        void setPlayerNeedToShowCards(const std::list<unsigned> &playerNeedToShowCards);

private:
	mutable boost::recursive_mutex m_syncMutex;

	PlayerList seatsList;
	PlayerList activePlayerList;
	PlayerList runningPlayerList;

	HandInterface *currentHand;

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
