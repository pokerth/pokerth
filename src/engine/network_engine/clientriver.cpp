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
#include "clientriver.h"
#include <game_defs.h>


//using namespace std;

ClientBeRoRiver::ClientBeRoRiver(HandInterface* bR, int id, int qP, int dP, int sB)
: myHand(bR), myID(id), actualQuantityPlayers(qP), dealerPosition(dP), smallBlindPosition(0), smallBlind(sB), highestSet(0), firstRiverRun(1), firstRiverRound(1), playersTurn(dP), highestCardsValue(0)
{
}

ClientBeRoRiver::~ClientBeRoRiver()
{
}

int
ClientBeRoRiver::getMyBeRoID() const
{
	return GAME_STATE_RIVER;
}

void
ClientBeRoRiver::setPlayersTurn(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	playersTurn = theValue;
}

int
ClientBeRoRiver::getPlayersTurn() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return playersTurn;
}

void
ClientBeRoRiver::setHighestSet(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	highestSet = theValue;
}

int
ClientBeRoRiver::getHighestSet() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return highestSet;
}

void
ClientBeRoRiver::setFirstRiverRound(bool theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	firstRiverRound = theValue;
}

bool
ClientBeRoRiver::getFirstRiverRound() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return firstRiverRound;
}

void
ClientBeRoRiver::setSmallBlindPosition(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	smallBlindPosition = theValue;
}

int
ClientBeRoRiver::getSmallBlindPosition() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return smallBlindPosition;
}

void
ClientBeRoRiver::setSmallBlind(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	smallBlind = theValue;
}

int
ClientBeRoRiver::getSmallBlind() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return smallBlind;
}

void
ClientBeRoRiver::setHighestCardsValue(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	highestCardsValue = theValue;
}

int
ClientBeRoRiver::getHighestCardsValue() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return highestCardsValue;
}

void
ClientBeRoRiver::resetFirstRun()
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	firstRiverRun = false;
}

void
ClientBeRoRiver::distributePot()
{
}

void
ClientBeRoRiver::run()
{
}

