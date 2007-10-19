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

ClientHand::ClientHand(boost::shared_ptr<EngineFactory> f, GuiInterface *g, BoardInterface *b, PlayerList sl, PlayerList apl, PlayerList rpl, int id, int sP, int dP, int sB,int sC)
: myFactory(f), myGui(g),  myBoard(b), seatsList(sl), activePlayerList(apl), runningPlayerList(rpl), myID(id), startQuantityPlayers(sP), dealerPosition(dP), currentRound(0),
  smallBlind(sB), startCash(sC), lastPlayersTurn(0), allInCondition(0),
  cardsShown(false), bettingRoundsPlayed(0)
{
	PlayerListIterator it;

	myBoard->setHand(this);

	for(it=seatsList->begin(); it!=seatsList->end(); it++) {
		(*it)->setHand(this);
	// myFlipCards auf 0 setzen
		(*it)->setMyCardsFlip(0, 0);
	}

	// roundStartCashArray fuellen
	// cardsvalue zuruecksetzen
	// remove all buttons
	for(it=activePlayerList->begin(); it!=activePlayerList->end(); it++) {

		boost::shared_ptr<PlayerInterface> tmpPlayer = *it;

		tmpPlayer->setMyRoundStartCash(tmpPlayer->getMyCash());
		tmpPlayer->setMyCardsValueInt(0);
		tmpPlayer->setMyButton(0);
		if (tmpPlayer->getMyUniqueID() == dealerPosition)
			tmpPlayer->setMyButton(1);
	}

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

PlayerList
ClientHand::getSeatsList() const
{
	return seatsList;
}

PlayerList
ClientHand::getActivePlayerList() const
{
	return activePlayerList;
}

PlayerList
ClientHand::getRunningPlayerList() const
{
	return runningPlayerList;
}

PlayerListIterator
ClientHand::getSeatIt(unsigned uniqueId) const
{
	PlayerListIterator it;

	for(it=seatsList->begin(); it!=seatsList->end(); it++) {
		if((*it)->getMyUniqueID() == uniqueId) {
			break;
		}
	}

	return it;
}

PlayerListIterator
ClientHand::getActivePlayerIt(unsigned uniqueId) const
{
	PlayerListIterator it;

	for(it=activePlayerList->begin(); it!=activePlayerList->end(); it++) {
		if((*it)->getMyUniqueID() == uniqueId) {
			break;
		}
	}

	return it;
}

PlayerListIterator
ClientHand::getRunningPlayerIt(unsigned uniqueId) const
{
	PlayerListIterator it;

	for(it=runningPlayerList->begin(); it!=runningPlayerList->end(); it++) {
		if((*it)->getMyUniqueID() == uniqueId) {
			break;
		}
	}

	return it;
}

BoardInterface*
ClientHand::getBoard() const
{
	return myBoard;
}

boost::shared_ptr<BeRoInterface>
ClientHand::getPreflop() const
{
	return myBeRo[GAME_STATE_PREFLOP];
}

boost::shared_ptr<BeRoInterface>
ClientHand::getFlop() const
{
	return myBeRo[GAME_STATE_FLOP];
}

boost::shared_ptr<BeRoInterface>
ClientHand::getTurn() const
{
	return myBeRo[GAME_STATE_TURN];
}

boost::shared_ptr<BeRoInterface>
ClientHand::getRiver() const
{
	return myBeRo[GAME_STATE_RIVER];
}

boost::shared_ptr<BeRoInterface>
ClientHand::getCurrentBeRo() const
{
	return myBeRo[currentRound];
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
ClientHand::setCurrentRound(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	currentRound = theValue;
}

int
ClientHand::getCurrentRound() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return currentRound;
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

	PlayerListIterator it, it_1;

	// refresh runningPlayerList
	for(it=runningPlayerList->begin(); it!=runningPlayerList->end(); ) {
		if((*it)->getMyAction() == PLAYER_ACTION_FOLD || (*it)->getMyAction() == PLAYER_ACTION_ALLIN) {
			it = runningPlayerList->erase(it);
			if(!(runningPlayerList->empty())) {

				it_1 = it;
				if(it_1 == runningPlayerList->begin()) it_1 = runningPlayerList->end();
				it_1--;
				getCurrentBeRo()->setCurrentPlayersTurnId((*it_1)->getMyUniqueID());

			}
		} else {
			it++;
		}
	}
}

