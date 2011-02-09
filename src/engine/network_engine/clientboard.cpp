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
#include "clientboard.h"

#include <handinterface.h>
#include <game_defs.h>

using namespace std;

ClientBoard::ClientBoard(unsigned dp)
: pot(0), sets(0), dealerPosition(dp), allInCondition(false), lastActionPlayer(0)
{
	myCards[0] = myCards[1] = myCards[2] = myCards[3] = myCards[4] = 0;
}


ClientBoard::~ClientBoard()
{
}

void
ClientBoard::setPlayerLists(PlayerList sl,  PlayerList apl, PlayerList rpl)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	seatsList = sl;
	activePlayerList = apl;
	runningPlayerList = rpl;
}

void
ClientBoard::setMyCards(int* theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	for (int i = 0; i < 5; i++)
		myCards[i] = theValue[i];
}

void
ClientBoard::getMyCards(int* theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	for (int i = 0; i < 5; i++)
		theValue[i] = myCards[i];
}

int
ClientBoard::getPot() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return pot;
}

void
ClientBoard::setPot(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	pot = theValue;
}

int
ClientBoard::getSets() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return sets;
}

void
ClientBoard::setSets(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	sets = theValue;
}

void
ClientBoard::setAllInCondition(bool theValue)
{
    boost::recursive_mutex::scoped_lock lock(m_syncMutex);
    allInCondition = theValue;
}

void
ClientBoard::setLastActionPlayer(unsigned theValue)
{
    boost::recursive_mutex::scoped_lock lock(m_syncMutex);
    lastActionPlayer = theValue;
}

void
ClientBoard::collectSets()
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	sets = 0;
	PlayerListConstIterator it_c;
	for (it_c=seatsList->begin(); it_c!=seatsList->end(); ++it_c)
		sets += (*it_c)->getMySet();

}

void
ClientBoard::collectPot()
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	pot += sets; 
	sets = 0;
}

void
ClientBoard::distributePot()
{

}

void
ClientBoard::determinePlayerNeedToShowCards()
{

}

std::list<unsigned>
ClientBoard::getWinners() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return winners;
}

void
ClientBoard::setWinners(const std::list<unsigned> &w)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	winners = w;
}

std::list<unsigned>
ClientBoard::getPlayerNeedToShowCards() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return playerNeedToShowCards;
}

void
ClientBoard::setPlayerNeedToShowCards(const std::list<unsigned> &p)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	playerNeedToShowCards = p;
}
