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
#include "clientflop.h"
#include <game_defs.h>
#include <handinterface.h>

//using namespace std;

ClientBeRoFlop::ClientBeRoFlop(HandInterface* bR, int id, int qP, int dP, int sB)
: myHand(bR), myID(id), actualQuantityPlayers(qP), dealerPosition(dP), smallBlindPosition(0), smallBlind(sB), highestSet(0), firstFlopRun(1), firstFlopRound(1), playersTurn(dP)
{
}


ClientBeRoFlop::~ClientBeRoFlop()
{
}

int
ClientBeRoFlop::getMyBeRoID() const
{
	return GAME_STATE_FLOP;
}

void
ClientBeRoFlop::setPlayersTurn(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	playersTurn = theValue;
}

int
ClientBeRoFlop::getPlayersTurn() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return playersTurn;
}
	
void
ClientBeRoFlop::setHighestSet(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	highestSet = theValue;
}

int
ClientBeRoFlop::getHighestSet() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return highestSet;
}

void
ClientBeRoFlop::setFirstFlopRound(bool theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	firstFlopRound = theValue;
}

bool
ClientBeRoFlop::getFirstFlopRound() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return firstFlopRound;
}

void
ClientBeRoFlop::setSmallBlindPosition(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	smallBlindPosition = theValue;
}

int
ClientBeRoFlop::getSmallBlindPosition() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return smallBlindPosition;
}

void
ClientBeRoFlop::setSmallBlind(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	smallBlind = theValue;
}

int
ClientBeRoFlop::getSmallBlind() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return smallBlind;
}

void
ClientBeRoFlop::resetFirstRun()
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	firstFlopRun = false;
}

void
ClientBeRoFlop::run()
{
}

