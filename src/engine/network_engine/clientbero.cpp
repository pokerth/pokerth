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

ClientBeRo::ClientBeRo(HandInterface* hi, int id, int qP, int dP, int sB, GameState gS)
: BeRoInterface(), myHand(hi), highestCardsValue(0), myBeRoID(gS), smallBlindPosition(0), smallBlind(sB), highestSet(0), firstRound(1), playersTurn(dP)
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

