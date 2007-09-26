//
// C++ Implementation: clientbero
//
// Description: 
//
//
// Author: FThauer FHammer <webmaster@pokerth.net>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "clientbero.h"

ClientBeRo::ClientBeRo(HandInterface* hi, int /*id*/, unsigned dP, int sB, GameState gS)
: BeRoInterface(), myBeRoID(gS), myHand(hi), highestCardsValue(0), playersTurn(dP), highestSet(0), firstRound(true), smallBlindPosition(0), smallBlind(sB), minimumRaise(0)
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
ClientBeRo::setLastActionPlayer ( int theValue )
{
	lastActionPlayer = theValue;
}

int
ClientBeRo::getLastActionPlayer() const
{
	return lastActionPlayer;
}

void
ClientBeRo::setPlayersTurn(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	playersTurn = theValue;
}

int
ClientBeRo::getPlayersTurn() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return playersTurn;
}

void
ClientBeRo::setCurrentPlayersTurnIt(PlayerListIterator theValue)
{
	currentPlayersTurnIt = theValue;
}

PlayerListIterator
ClientBeRo::getCurrentPlayersTurnIt() const
{
	return currentPlayersTurnIt;
}

void
ClientBeRo::setLastPlayersTurnIt(PlayerListIterator theValue)
{
	lastPlayersTurnIt = theValue;
}

PlayerListIterator
ClientBeRo::getLastPlayersTurnIt() const
{
	return lastPlayersTurnIt;
}

void
ClientBeRo::setCurrentPlayersTurnId(unsigned theValue)
{
	currentPlayersTurnId = theValue;
}

unsigned
ClientBeRo::getCurrentPlayersTurnId() const
{
	return currentPlayersTurnId;
}

void
ClientBeRo::setFirstRoundLastPlayersTurnId(unsigned theValue)
{
	firstRoundLastPlayersTurnId = theValue;
}

unsigned
ClientBeRo::getFirstRoundLastPlayersTurnId() const
{
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
	minimumRaise = theValue;
}

int
ClientBeRo::getMinimumRaise() const
{
	return minimumRaise;
}


void
ClientBeRo::resetFirstRun()
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

