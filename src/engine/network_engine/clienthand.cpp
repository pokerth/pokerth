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
#include "clienthand.h"
#include <game_defs.h>

using namespace std;

ClientHand::ClientHand(boost::shared_ptr<EngineFactory> f, GuiInterface *g, BoardInterface *b, PlayerInterface **p, int id, int sP, int aP, int dP, int sB,int sC)
: myFactory(f), myGui(g),  myBoard(b), playerArray(p), myPreflop(0), myFlop(0), myTurn(0), myRiver(0),
  myID(id), actualQuantityPlayers(aP), startQuantityPlayers(sP), dealerPosition(dP), actualRound(0),
  smallBlind(sB), startCash(sC), activePlayersCounter(aP), lastPlayersTurn(0), allInCondition(0),
  cardsShown(false), bettingRoundsPlayed(0)
{
	int i;
	lastPlayersTurn = 0;

	myBoard->setHand(this);


	for(i=0; i<startQuantityPlayers; i++) {
		if(playerArray[i]->getMyActiveStatus() != 0) {
			playerArray[i]->setHand(this);
		}
	// myFlipCards auf 0 setzen
		playerArray[i]->setMyCardsFlip(0, 0);
	}


	// roundStartCashArray fuellen
	// cardsvalue zuruecksetzen
	// remove all buttons
	for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
		playerArray[i]->setMyRoundStartCash(playerArray[i]->getMyCash());
		playerArray[i]->setMyCardsValueInt(0);
		playerArray[i]->setMyButton(0);
	}

	// assign dealer button
	playerArray[dealerPosition]->setMyButton(1);

	// the rest of the buttons are assigned later as received from the server.

	// Preflop, Flop, Turn und River erstellen
	myPreflop =  myFactory->createPreflop(this, myID, actualQuantityPlayers, dealerPosition, smallBlind);
	myFlop = myFactory->createFlop(this, myID, actualQuantityPlayers, dealerPosition, smallBlind);
	myTurn = myFactory->createTurn(this, myID, actualQuantityPlayers, dealerPosition, smallBlind);
	myRiver = myFactory->createRiver(this, myID, actualQuantityPlayers, dealerPosition, smallBlind);
}



ClientHand::~ClientHand()
{
}

void
ClientHand::start()
{
}

PlayerInterface**
ClientHand::getPlayerArray() const
{
	return playerArray;
}

BoardInterface*
ClientHand::getBoard() const
{
	return myBoard;
}

PreflopInterface*
ClientHand::getPreflop() const
{
	return myPreflop;
}

FlopInterface*
ClientHand::getFlop() const
{
	return myFlop;
}

TurnInterface*
ClientHand::getTurn() const
{
	return myTurn;
}

RiverInterface*
ClientHand::getRiver() const
{
	return myRiver;
}

GuiInterface*
ClientHand::getGuiInterface() const
{
	return myGui;
}

void
ClientHand::setMyID(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	myID = theValue;
}

int
ClientHand::getMyID() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return myID;
}

void
ClientHand::setActualQuantityPlayers(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	actualQuantityPlayers = theValue;
}

int
ClientHand::getActualQuantityPlayers() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return actualQuantityPlayers;
}

void
ClientHand::setStartQuantityPlayers(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	startQuantityPlayers = theValue;
}

int
ClientHand::getStartQuantityPlayers() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return startQuantityPlayers;
}

void
ClientHand::setActualRound(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	actualRound = theValue;
}

int
ClientHand::getActualRound() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return actualRound;
}

void
ClientHand::setDealerPosition(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	dealerPosition = theValue;
}

int
ClientHand::getDealerPosition() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return dealerPosition;
}

void
ClientHand::setSmallBlind(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	smallBlind = theValue;
}

int
ClientHand::getSmallBlind() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return smallBlind;
}

void
ClientHand::setAllInCondition(bool theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	allInCondition = theValue;
}

bool
ClientHand::getAllInCondition() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return allInCondition;
}

void
ClientHand::setStartCash(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	startCash = theValue;
}

int
ClientHand::getStartCash() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return startCash;
}

void
ClientHand::setActivePlayersCounter(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	activePlayersCounter = theValue;
}

int
ClientHand::getActivePlayersCounter() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return activePlayersCounter;
}

void
ClientHand::setBettingRoundsPlayed(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	bettingRoundsPlayed = theValue;
}

int
ClientHand::getBettingRoundsPlayed() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return bettingRoundsPlayed;
}

void
ClientHand::setLastPlayersTurn(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	lastPlayersTurn = theValue;
}

int
ClientHand::getLastPlayersTurn() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return lastPlayersTurn;
}

void
ClientHand::setCardsShown(bool theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	cardsShown = theValue;
}

bool
ClientHand::getCardsShown() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return cardsShown;
}

void
ClientHand::switchRounds()
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	// update active players counter.
	activePlayersCounter = 0;
	for (int i = 0; i < MAX_NUMBER_OF_PLAYERS; i++) { 
		if (playerArray[i]->getMyAction() != 1 && playerArray[i]->getMyActiveStatus() == 1) activePlayersCounter++;
	}
}

