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

#include "localboard.h"

#include "handinterface.h"
#include <game_defs.h>
#include <core/loghelper.h>
#include "localexception.h"
#include "engine_msg.h"

LocalBoard::LocalBoard() : BoardInterface(), pot(0), sets(0), allInCondition(false), lastActionPlayerID(0)
{
	myCards[0] = myCards[1] = myCards[2] = myCards[3] = myCards[4] = 0;
}

LocalBoard::~LocalBoard()
{
}

void LocalBoard::setPlayerLists(PlayerList sl, PlayerList apl, PlayerList rpl)
{
	seatsList = sl;
	activePlayerList = apl;
	runningPlayerList = rpl;
}

void LocalBoard::collectSets()
{

	sets = 0;

	PlayerListConstIterator it_c;
	for(it_c=seatsList->begin(); it_c!=seatsList->end(); ++it_c) {
		sets += (*it_c)->getMySet();
	}

}

void LocalBoard::collectPot()
{

	pot += sets;
	sets = 0;

	PlayerListIterator it;
	for(it=seatsList->begin(); it!=seatsList->end(); ++it) {
		(*it)->setMySetNull();
	}

}

void LocalBoard::distributePot(unsigned dealerPosition)
{

	winners.clear();

	size_t i,j,k,l;
	PlayerListIterator it;
	PlayerListConstIterator it_c;

	// filling player sets vector
	std::vector<unsigned> playerSets;
	for(it=seatsList->begin(); it!=seatsList->end(); ++it) {
		if((*it)->getMyActiveStatus()) {
			playerSets.push_back( ( ((*it)->getMyRoundStartCash()) - ((*it)->getMyCash()) ) );
		} else {
			playerSets.push_back(0);
		}
		(*it)->setLastMoneyWon(0);
	}

	// sort player sets asc
	std::vector<unsigned> playerSetsSort = playerSets;
	sort(playerSetsSort.begin(), playerSetsSort.end());

	// potLevel[0] = amount, potLevel[1] = sum, potLevel[2..n] = winner
	std::vector<unsigned> potLevel;

	// temp var
	int highestCardsValue;
	size_t winnerCount;
	bool finalPot;
	int potCarryOver = 0;
	size_t mod;
	bool winnerHit;

	// level loop
	for(i=0; i<playerSetsSort.size(); i++) {

		// restart levelHighestCardsValue
		highestCardsValue = 0;

		// level detection
		if(playerSetsSort[i] > 0) {

			// level amount
			potLevel.push_back(playerSetsSort[i]);

			// level sum
			potLevel.push_back((playerSetsSort.size()-i)*potLevel[0] + potCarryOver);

			// determine level highestCardsValue
			for(it_c=seatsList->begin(), j=0; it_c!=seatsList->end(); ++it_c,j++) {
				if((*it_c)->getMyActiveStatus() && (*it_c)->getMyCardsValueInt() > highestCardsValue && (*it_c)->getMyAction() != PLAYER_ACTION_FOLD && playerSets[j] >= potLevel[0]) {
					highestCardsValue = (*it_c)->getMyCardsValueInt();
				}
			}

			// level winners
			for(it_c=seatsList->begin(), j=0; it_c!=seatsList->end(); ++it_c,j++) {
				if((*it_c)->getMyActiveStatus() && highestCardsValue == (*it_c)->getMyCardsValueInt() && (*it_c)->getMyAction() != PLAYER_ACTION_FOLD && playerSets[j] >= potLevel[0]) {
					potLevel.push_back((*it_c)->getMyUniqueID());
				}
			}

			// determine the number of level winners
			winnerCount = potLevel.size()-2;
			if (!winnerCount) {
				throw LocalException(__FILE__, __LINE__, ERR_NO_WINNER);
			}

			// check if this is the final pot level for at least one winner
			finalPot = false;
			for(j=2; j<potLevel.size(); j++) {
				// find seat with potLevel[j]-ID
				for(it=seatsList->begin(), k=0; it!=seatsList->end(); ++it, k++) {
					if((*it)->getMyUniqueID() == potLevel[j] && potLevel[0] == playerSets[k]) {
						finalPot = true;
						break;
					}
				}
				if(finalPot) break;
			}

			if(finalPot) {
				// distribute the pot level sum to level winners
				mod = (potLevel[1])%winnerCount;
				// pot level sum divisible by winnerCount
				if(mod == 0) {

					for(j=2; j<potLevel.size(); j++) {
						// find seat with potLevel[j]-ID
						for(it=seatsList->begin(); it!=seatsList->end(); ++it) {
							if((*it)->getMyUniqueID() == potLevel[j]) {
								break;
							}
						}
						if(it == seatsList->end()) {
							throw LocalException(__FILE__, __LINE__, ERR_SEAT_NOT_FOUND);
						}
						(*it)->setMyCash( (*it)->getMyCash() + ((potLevel[1])/winnerCount));

						// filling winners vector
						winners.push_back((*it)->getMyUniqueID());
						(*it)->setLastMoneyWon( (*it)->getLastMoneyWon() + (potLevel[1])/winnerCount );
					}

				}
				// pot level sum not divisible by winnerCount
				// --> distribution after smallBlind
				else {

					// find Seat with dealerPosition
					for(it=seatsList->begin(); it!=seatsList->end(); ++it) {
						if((*it)->getMyUniqueID() == dealerPosition) {
							break;
						}
					}
					if(it == seatsList->end()) {
						throw LocalException(__FILE__, __LINE__, ERR_SEAT_NOT_FOUND);
					}

					for(j=0; j<winnerCount; j++) {

						winnerHit = false;

						for(k=0; k<MAX_NUMBER_OF_PLAYERS && !winnerHit; k++) {

							++it;
							if(it == seatsList->end())
								it = seatsList->begin();

							for(l=2; l<potLevel.size(); l++) {
								if((*it)->getMyActiveStatus() && (*it)->getMyUniqueID() == potLevel[l])
									winnerHit = true;
							}

						}

						if(j<mod) {
							(*it)->setMyCash( (*it)->getMyCash() + (int)((potLevel[1])/winnerCount) + 1);
							// filling winners vector
							winners.push_back((*it)->getMyUniqueID());
							(*it)->setLastMoneyWon( (*it)->getLastMoneyWon() + ((potLevel[1])/winnerCount) + 1 );
						} else {
							(*it)->setMyCash( (*it)->getMyCash() + (int)((potLevel[1])/winnerCount));
							// filling winners vector
							winners.push_back((*it)->getMyUniqueID());
							(*it)->setLastMoneyWon( (*it)->getLastMoneyWon() + (potLevel[1])/winnerCount );
						}
					}
				}
				potCarryOver = 0;

				// pot refresh
				pot -= potLevel[1];

			} else {
				potCarryOver = potLevel[1];
			}

			// reevaluate the player sets
			for(j=0; j<playerSets.size(); j++) {
				if(playerSets[j]>0) {
					playerSets[j] -= potLevel[0];
				}
			}

			// sort player sets asc
			playerSetsSort = playerSets;
			sort(playerSetsSort.begin(), playerSetsSort.end());

			// clear potLevel
			potLevel.clear();

		}
	}

	// winners sort and unique
	winners.sort();
	winners.unique();


	// ERROR-Outputs

	if(pot!=0) LOG_ERROR(__FILE__ << " (" << __LINE__ << "): distributePot-ERROR: Pot = " << pot);

	/*int sum = 0;

	for(it_c=activePlayerList->begin(); it_c!=activePlayerList->end(); ++it_c) {
		sum += (*it_c)->getMyCash();
	}*/
}

void LocalBoard::determinePlayerNeedToShowCards()
{

	playerNeedToShowCards.clear();

	// in All In Condition everybody have to show the cards
	if(allInCondition) {

		PlayerListConstIterator it_c;

		for(it_c = activePlayerList->begin(); it_c != activePlayerList->end(); ++it_c) {
			if((*it_c)->getMyAction() != PLAYER_ACTION_FOLD) {
				playerNeedToShowCards.push_back((*it_c)->getMyUniqueID());
			}
		}

	}

	else {

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
