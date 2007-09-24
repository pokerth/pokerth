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

ClientHand::ClientHand(boost::shared_ptr<EngineFactory> f, GuiInterface *g, BoardInterface *b, std::vector<boost::shared_ptr<PlayerInterface> > sl, PlayerList apl, PlayerList rpl, int id, int sP, int dP, int sB,int sC)
: myFactory(f), myGui(g),  myBoard(b), playerArray(sl), activePlayerList(apl), runningPlayerList(rpl), myID(id), startQuantityPlayers(sP), dealerPosition(dP), actualRound(0),
  smallBlind(sB), startCash(sC), lastPlayersTurn(0), allInCondition(0),
  cardsShown(false), bettingRoundsPlayed(0)
{
	activePlayersCounter = activePlayerList->size();

	int i;
	lastPlayersTurn = 0;

	myBoard->setHand(this);


	for(i=0; i<startQuantityPlayers; i++) {
		playerArray[i]->setHand(this);
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
	myBeRo = myFactory->createBeRo(this, myID, dealerPosition, smallBlind);
}



ClientHand::~ClientHand()
{
}

void
ClientHand::start()
{
}

std::vector<boost::shared_ptr<PlayerInterface> >
ClientHand::getPlayerArray() const
{
	return playerArray;
}

BoardInterface*
ClientHand::getBoard() const
{
	return myBoard;
}

boost::shared_ptr<BeRoInterface>
ClientHand::getPreflop() const
{
	return myBeRo[actualRound];
}

boost::shared_ptr<BeRoInterface>
ClientHand::getFlop() const
{
	return myBeRo[actualRound];
}

boost::shared_ptr<BeRoInterface>
ClientHand::getTurn() const
{
	return myBeRo[actualRound];
}

boost::shared_ptr<BeRoInterface>
ClientHand::getRiver() const
{
	return myBeRo[actualRound];
}

boost::shared_ptr<BeRoInterface>
ClientHand::getCurrentBeRo() const
{
	return myBeRo[actualRound];
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

