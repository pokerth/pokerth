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
#include "clientbero.h"

ClientBeRo::ClientBeRo(HandInterface* hi, int /*id*/, unsigned /*dP*/, int sB, GameState gS)
: BeRoInterface(), myBeRoID(gS), myHand(hi), highestCardsValue(0), smallBlindPositionId(0), bigBlindPositionId(0), currentPlayersTurnId(0),
  firstRoundLastPlayersTurnId(0), highestSet(0), firstRound(true), smallBlindPosition(0), smallBlind(sB), minimumRaise(2*sB), fullBetRule(false),
  lastActionPlayer(0)
{
}


ClientBeRo::~ClientBeRo()
{
}

GameState
ClientBeRo::getMyBeRoID() const
{
	return myBeRoID;
}

int
ClientBeRo::getHighestCardsValue() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return highestCardsValue;
}

void
ClientBeRo::setHighestCardsValue(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	highestCardsValue = theValue;
}

void
ClientBeRo::setLastActionPlayer ( unsigned theValue )
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	lastActionPlayer = theValue;
}

unsigned
ClientBeRo::getLastActionPlayer() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return lastActionPlayer;
}

void
ClientBeRo::setCurrentPlayersTurnIt(PlayerListIterator theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	currentPlayersTurnIt = theValue;
}

PlayerListIterator
ClientBeRo::getCurrentPlayersTurnIt() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return currentPlayersTurnIt;
}

void
ClientBeRo::setLastPlayersTurnIt(PlayerListIterator theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	lastPlayersTurnIt = theValue;
}

PlayerListIterator
ClientBeRo::getLastPlayersTurnIt() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return lastPlayersTurnIt;
}

void
ClientBeRo::setSmallBlindPositionId(unsigned theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	smallBlindPositionId = theValue;
}

unsigned
ClientBeRo::getSmallBlindPositionId() const
{
	return smallBlindPositionId;
}

void
ClientBeRo::setBigBlindPositionId(unsigned theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	bigBlindPositionId = theValue;
}

unsigned
ClientBeRo::getBigBlindPositionId() const
{
	return bigBlindPositionId;
}

void
ClientBeRo::setCurrentPlayersTurnId(unsigned theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	currentPlayersTurnId = theValue;
}

unsigned
ClientBeRo::getCurrentPlayersTurnId() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return currentPlayersTurnId;
}

void
ClientBeRo::setFirstRoundLastPlayersTurnId(unsigned theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	firstRoundLastPlayersTurnId = theValue;
}

unsigned
ClientBeRo::getFirstRoundLastPlayersTurnId() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return firstRoundLastPlayersTurnId;
}

void
ClientBeRo::setHighestSet(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	highestSet = theValue;
}

int
ClientBeRo::getHighestSet() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return highestSet;
}

void
ClientBeRo::setFirstRound(bool theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	firstRound = theValue;
}

bool
ClientBeRo::getFirstRound() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return firstRound;
}

void
ClientBeRo::setSmallBlindPosition(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	smallBlindPosition = theValue;
}

int
ClientBeRo::getSmallBlindPosition() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return smallBlindPosition;
}

void
ClientBeRo::setSmallBlind(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	smallBlind = theValue;
}

int
ClientBeRo::getSmallBlind() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return smallBlind;
}

void
ClientBeRo::setMinimumRaise ( int theValue )
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	minimumRaise = theValue;
}

int
ClientBeRo::getMinimumRaise() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return minimumRaise;
}

void
ClientBeRo::setFullBetRule ( bool theValue )
{
        boost::recursive_mutex::scoped_lock lock(m_syncMutex);
        fullBetRule = theValue;
}

bool
ClientBeRo::getFullBetRule() const
{
        boost::recursive_mutex::scoped_lock lock(m_syncMutex);
        return fullBetRule;
}

void
ClientBeRo::skipFirstRunGui()
{
}

void
ClientBeRo::nextPlayer()
{
}

void
ClientBeRo::run()
{
}

void
ClientBeRo::postRiverRun()
{
}

