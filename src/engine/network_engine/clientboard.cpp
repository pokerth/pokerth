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

#include "clientboard.h"

#include <handinterface.h>
#include <game_defs.h>

using namespace std;

ClientBoard::ClientBoard()
	: pot(0), sets(0), allInCondition(false), lastActionPlayerID(0)
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
ClientBoard::setLastActionPlayerID(unsigned theValue)
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	lastActionPlayerID = theValue;
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
	// CRITICAL: Summiere zuerst alle Spieler-Sets zu Board.sets
	sets = 0;
	PlayerListIterator it;
	for(it = seatsList->begin(); it != seatsList->end(); ++it) {
		sets += (*it)->getMySet();
	}
	// Dann addiere sets zum pot und setze Board.sets auf 0
	pot += sets;
	sets = 0;
	// Dann setze alle Spieler-Sets auf 0
	for(it = seatsList->begin(); it != seatsList->end(); ++it) {
		(*it)->setMySetNull();
	}
}

void
ClientBoard::distributePot(unsigned)
{

}

void
ClientBoard::determinePlayerNeedToShowCards()
{
	boost::recursive_mutex::scoped_lock lock(m_syncMutex);
	playerNeedToShowCards.clear();

	// in All In Condition everybody have to show the cards
	if(allInCondition) {
		PlayerListConstIterator it_c;
		for(it_c = activePlayerList->begin(); it_c != activePlayerList->end(); ++it_c) {
			if((*it_c)->getMyAction() != PLAYER_ACTION_FOLD) {
				playerNeedToShowCards.push_back((*it_c)->getMyUniqueID());
			}
		}
	} else {
		// all winners have to show their cards
		std::list<std::pair<int,int> > level;
		PlayerListConstIterator lastActionPlayerIt;
		PlayerListConstIterator it_c;

		// search lastActionPlayer
		for(it_c = activePlayerList->begin(); it_c != activePlayerList->end(); ++it_c) {
			if((*it_c)->getMyUniqueID() == lastActionPlayerID && (*it_c)->getMyAction() != PLAYER_ACTION_FOLD) {
				lastActionPlayerIt = it_c;
				break;
			}
		}

		if(it_c == activePlayerList->end()) {
			for(it_c = activePlayerList->begin(); it_c != activePlayerList->end(); ++it_c) {
				if((*it_c)->getMyAction() != PLAYER_ACTION_FOLD) {
					lastActionPlayerIt = it_c;
					break;
				}
			}
		}

		// the player who has done the last action has to show his cards first
		playerNeedToShowCards.push_back((*lastActionPlayerIt)->getMyUniqueID());

		std::pair<int,int> level_tmp;
		// get position und cardsValue of the player who show his cards first
		level_tmp.first = (*lastActionPlayerIt)->getMyCardsValueInt();
		level_tmp.second = (*lastActionPlayerIt)->getMyRoundStartCash()-(*lastActionPlayerIt)->getMyCash();

		level.push_back(level_tmp);

		std::list<std::pair<int,int> >::iterator level_it;
		std::list<std::pair<int,int> >::iterator next_level_it;

		it_c = lastActionPlayerIt;
		++it_c;

		for(unsigned i = 0; i < activePlayerList->size(); i++) {
			if(it_c == activePlayerList->end()) it_c = activePlayerList->begin();

			if((*it_c)->getMyAction() != PLAYER_ACTION_FOLD) {
				for(level_it = level.begin(); level_it != level.end(); ++level_it) {
					if((*it_c)->getMyCardsValueInt() > (*level_it).first) {
						next_level_it = level_it;
						++next_level_it;
						if(next_level_it == level.end()) {
							playerNeedToShowCards.push_back((*it_c)->getMyUniqueID());
							level_tmp.first = (*it_c)->getMyCardsValueInt();
							level_tmp.second = (*it_c)->getMyRoundStartCash()-(*it_c)->getMyCash();
							level.push_back(level_tmp);
							break;
						}
					} else {
						if((*it_c)->getMyCardsValueInt() == (*level_it).first) {
							next_level_it = level_it;
							++next_level_it;

							if(next_level_it == level.end() || (*it_c)->getMyRoundStartCash()-(*it_c)->getMyCash() > (*next_level_it).second) {
								playerNeedToShowCards.push_back((*it_c)->getMyUniqueID());
								if((*it_c)->getMyRoundStartCash()-(*it_c)->getMyCash() > (*level_it).second) {
									(*level_it).second = (*it_c)->getMyRoundStartCash()-(*it_c)->getMyCash();
								}
							}
							break;
						} else {
							if((*it_c)->getMyRoundStartCash()-(*it_c)->getMyCash() > (*level_it).second) {
								playerNeedToShowCards.push_back((*it_c)->getMyUniqueID());
								level_tmp.first = (*it_c)->getMyCardsValueInt();
								level_tmp.second = (*it_c)->getMyRoundStartCash()-(*it_c)->getMyCash();

								level.insert(level_it,level_tmp);

								break;
							}
						}
					}
				}
			}
			++it_c;
		}
		level.clear();
	}

	// sort and unique the list
	playerNeedToShowCards.sort();
	playerNeedToShowCards.unique();
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
