/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/

#include "clienthand.h"
#include <game_defs.h>

using namespace std;

ClientHand::ClientHand(boost::shared_ptr<EngineFactory> f, GuiInterface *g, boost::shared_ptr<BoardInterface> b, Log *l, PlayerList sl, PlayerList apl, PlayerList rpl, int id, int sP, int dP, int sB,int sC)
	: myFactory(f), myGui(g),  myBoard(b), myLog(l), seatsList(sl), activePlayerList(apl), runningPlayerList(rpl), myID(id), startQuantityPlayers(sP), dealerPosition(dP), currentRound(GAME_STATE_PREFLOP), roundBeforePostRiver(GAME_STATE_PREFLOP),
	  smallBlind(sB), startCash(sC), previousPlayerID(-1), allInCondition(0),
	  cardsShown(false)
{
	PlayerListIterator it;

	for(it=seatsList->begin(); it!=seatsList->end(); ++it) {
		(*it)->setHand(this);
		// myFlipCards auf 0 setzen
		(*it)->setMyCardsFlip(0, 0);
	}

	// roundStartCashArray fuellen
	// cardsvalue zuruecksetzen
	// remove all buttons
	for(it=activePlayerList->begin(); it!=activePlayerList->end(); ++it) {

		boost::shared_ptr<PlayerInterface> tmpPlayer = *it;

		tmpPlayer->setMyRoundStartCash(tmpPlayer->getMyCash());
		tmpPlayer->setMyCardsValueInt(0);
		tmpPlayer->setMyButton(0);
		if (tmpPlayer->getMyUniqueID() == dealerPosition)
			tmpPlayer->setMyButton(1);
	}

	// the rest of the buttons are assigned later as received from the server.

	// Preflop, Flop, Turn und River erstellen
	myBeRo = myFactory->createBeRo(this, dealerPosition, smallBlind);
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
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return seatsList;
}

PlayerList
ClientHand::getActivePlayerList() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return activePlayerList;
}

PlayerList
ClientHand::getRunningPlayerList() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return runningPlayerList;
}

PlayerListIterator
ClientHand::getSeatIt(unsigned uniqueId) const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	PlayerListIterator it;

	for(it=seatsList->begin(); it!=seatsList->end(); ++it) {
		if((*it)->getMyUniqueID() == uniqueId) {
			break;
		}
	}

	return it;
}

PlayerListIterator
ClientHand::getActivePlayerIt(unsigned uniqueId) const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	PlayerListIterator it;

	for(it=activePlayerList->begin(); it!=activePlayerList->end(); ++it) {
		if((*it)->getMyUniqueID() == uniqueId) {
			break;
		}
	}

	return it;
}

PlayerListIterator
ClientHand::getRunningPlayerIt(unsigned uniqueId) const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	PlayerListIterator it;

	for(it=runningPlayerList->begin(); it!=runningPlayerList->end(); ++it) {
		if((*it)->getMyUniqueID() == uniqueId) {
			break;
		}
	}

	return it;
}

boost::shared_ptr<BoardInterface>
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
ClientHand::setCurrentRound(GameState theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	currentRound = theValue;
}

GameState
ClientHand::getCurrentRound() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return currentRound;
}

GameState
ClientHand::getRoundBeforePostRiver() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return roundBeforePostRiver;
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
ClientHand::setPreviousPlayerID(int theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	previousPlayerID = theValue;
}

int
ClientHand::getPreviousPlayerID() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return previousPlayerID;
}

void
ClientHand::setLastActionPlayerID (unsigned theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	lastActionPlayerID = theValue;
}

unsigned
ClientHand::getLastActionPlayerID() const
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	return lastActionPlayerID;
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
				--it_1;
				getCurrentBeRo()->setCurrentPlayersTurnId((*it_1)->getMyUniqueID());

			}
		} else {
			++it;
		}
	}
}

