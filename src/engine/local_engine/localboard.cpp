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
#include "localboard.h"

#include "handinterface.h"
#include <game_defs.h>
#include <core/loghelper.h>
#include "localexception.h"
#include "engine_msg.h"

using namespace std;

LocalBoard::LocalBoard(unsigned dp) : BoardInterface(), pot(0), sets(0), dealerPosition(dp), allInCondition(false), lastActionPlayer(0)
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

void LocalBoard::distributePot()
{

	winners.clear();

	size_t i,j,k,l;
	PlayerListIterator it;
	PlayerListConstIterator it_c;

	// filling player sets vector
	vector<unsigned> playerSets;
	for(it=seatsList->begin(); it!=seatsList->end(); ++it) {
		if((*it)->getMyActiveStatus()) {
			playerSets.push_back( ( ((*it)->getMyRoundStartCash()) - ((*it)->getMyCash()) ) );
		} else {
			playerSets.push_back(0);
		}
		(*it)->setLastMoneyWon(0);
	}

	// sort player sets asc
	vector<unsigned> playerSetsSort = playerSets;
	sort(playerSetsSort.begin(), playerSetsSort.end());

	// potLevel[0] = amount, potLevel[1] = sum, potLevel[2..n] = winner
	vector<unsigned> potLevel;

	// temp var
	int highestCardsValue;
	size_t winnerCount;
	size_t mod;
//	int winnerPointer;
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
			potLevel.push_back((playerSetsSort.size()-i)*potLevel[0]);

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

// 				winnerPointer = currentHand->getDealerPosition();

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

// 						winnerPointer = (winnerPointer+1)%(MAX_NUMBER_OF_PLAYERS);

// 						winnerHit = false;

						++it;
						if(it == seatsList->end())
							it = seatsList->begin();

						for(l=2; l<potLevel.size(); l++) {
// 							if(winnerPointer == potLevel[l]) winnerHit = true;
							if((*it)->getMyUniqueID() == potLevel[l]) winnerHit = true;
						}

					}

// 					it = currentHand->getSeatIt(winnerPointer);
// 					if(it == seatsList->end()) {
// 						throw LocalException(__FILE__, __LINE__, ERR_SEAT_NOT_FOUND);
// 					}
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

			// reevaluate the player sets
			for(j=0; j<playerSets.size(); j++) {
				if(playerSets[j]>0) {
					playerSets[j] -= potLevel[0];
				}
			}

			// sort player sets asc
			playerSetsSort = playerSets;
			sort(playerSetsSort.begin(), playerSetsSort.end());

			// pot refresh
			pot -= potLevel[1];

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

	for(it_c=activePlayerList->begin(); it_c!=activePlayerList->end(); it_c++) {
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
		//    playerNeedToShowCards = winners;

		//    std::_List_const_iterator<unsigned> it_c;

		std::list<int*> level;

		//   for(it_c=winners.begin(); it_c != winners.end(); it_c++) {
		//        cout << (*it_c) << endl;
		//    }
		//
		//    cout << "lastActionPlayer" << currentHand->getCurrentBeRo()->getLastActionPlayer() << endl;
		//
		//    int lastActionPlayer = ;
		//
		//    if(lastActionPlayer == -1) {
		//        lastActionPlayer = 0;
		//    }
		//
		//
		//


		PlayerListConstIterator lastActionPlayerIt;
		PlayerListConstIterator it_c;

		//    cout << "lAP-Ende: " << currentHand->getLastActionPlayer() << endl;
		// search lastActionPlayer
		for(it_c = activePlayerList->begin(); it_c != activePlayerList->end(); ++it_c) {
			if((*it_c)->getMyUniqueID() == lastActionPlayer && (*it_c)->getMyAction() != PLAYER_ACTION_FOLD) {
				lastActionPlayerIt = it_c;
//                    cout << (*it_c)->getMyUniqueID() << endl;
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

//            cout << (*lastActionPlayerIt)->getMyUniqueID() << " - " << (*lastActionPlayerIt)->getMyName() << endl;

		int *level_tmp = new int[2];
		// get position und cardsValue of the player who show his cards first
		level_tmp[0] = (*lastActionPlayerIt)->getMyCardsValueInt();
		level_tmp[1] = (*lastActionPlayerIt)->getMyRoundStartCash()-(*lastActionPlayerIt)->getMyCash();

		//    level_tmp = {(*lastActionPlayerIt)->getMyCardsValueInt(),(*lastActionPlayerIt)->getMyRoundStartCash()-(*lastActionPlayerIt)->getMyCash()};

//            cout << level_tmp[0] << "," << level_tmp[1] << endl;

		level.push_back(level_tmp);

		std::list<int*>::iterator level_it;
		std::list<int*>::iterator level_it_tmp;
		std::list<int*>::iterator next_level_it;

		//    PlayerListConstIterator firstAfterLastActionPlayerIt = lastActionPlayerIt;
		//    firstAfterLastActionPlayerIt++;

		it_c = lastActionPlayerIt;
		++it_c;

		for(unsigned i = 0; i < activePlayerList->size(); i++) {

			if(it_c == activePlayerList->end()) it_c = activePlayerList->begin();

			if((*it_c)->getMyAction() != PLAYER_ACTION_FOLD) {
				//    for(it_c = firstAfterLastActionPlayerIt; it_c != lastActionPlayerIt; it_c++) {

				//            if(it_c == lastActionPlayerIt) {
				//                break;
				//            }
				//        }
				for(level_it = level.begin(); level_it != level.end(); ++level_it) {
					if((*it_c)->getMyCardsValueInt() > (*level_it)[0]) {
						next_level_it = level_it;
						++next_level_it;
						if(next_level_it == level.end()) {
							playerNeedToShowCards.push_back((*it_c)->getMyUniqueID());
							level_tmp = new int[2];
							level_tmp[0] = (*it_c)->getMyCardsValueInt();
							level_tmp[1] = (*it_c)->getMyRoundStartCash()-(*it_c)->getMyCash();
							level.push_back(level_tmp);
							break;
						}
					} else {
						if((*it_c)->getMyCardsValueInt() == (*level_it)[0]) {
							next_level_it = level_it;
							++next_level_it;
//
//                                for(level_it_tmp = level.begin(); level_it_tmp != level.end(); level_it_tmp++) {
//                                    cout << (*level_it_tmp)[0] << "," << (*level_it_tmp)[1] << endl;
//                                }


							if(next_level_it == level.end() || (*it_c)->getMyRoundStartCash()-(*it_c)->getMyCash() > (*next_level_it)[1]) {
								playerNeedToShowCards.push_back((*it_c)->getMyUniqueID());
								if((*it_c)->getMyRoundStartCash()-(*it_c)->getMyCash() > (*level_it)[1]) {
									(*level_it)[1] = (*it_c)->getMyRoundStartCash()-(*it_c)->getMyCash();
								}
							}
							break;
						} else {
							if((*it_c)->getMyRoundStartCash()-(*it_c)->getMyCash() > (*level_it)[1]) {
								playerNeedToShowCards.push_back((*it_c)->getMyUniqueID());
								level_tmp = new int(2);
								level_tmp[0] = (*it_c)->getMyCardsValueInt();
								level_tmp[1] = (*it_c)->getMyRoundStartCash()-(*it_c)->getMyCash();
//
//                                    for(level_it_tmp = level.begin(); level_it_tmp != level.end(); level_it_tmp++) {
//                                        cout << (*level_it_tmp)[0] << "," << (*level_it_tmp)[1] << endl;
//                                    }

								level.insert(level_it,level_tmp);

//                                    for(level_it_tmp = level.begin(); level_it_tmp != level.end(); level_it_tmp++) {
//                                        cout << (*level_it_tmp)[0] << "," << (*level_it_tmp)[1] << endl;
//                                    }

								break;
							}
						}
					}
				}

			}

			++it_c;

		}



		//    bool showCards;

		// search for more player who have to show their cards (perhaps not all situation are considered yet)
		//    for(it_c = ++lastActionPlayerIt; it_c!=activePlayerList->end(); it_c++) {
		//
		//        showCards = false;
		//
		////        if((*it_c)->getMyCardsValueInt() >= highestCardsValueInt) {
		////            playerNeedToShowCards.push_back((*it_c)->getMyUniqueID());
		////            highestCardsValueInt = (*it_c)->getMyCardsValueInt();
		////        }
		//    }
		//
		//    for(it_c = activePlayerList->begin(); it_c!=lastActionPlayerIt; it_c++) {
		////        if((*it_c)->getMyCardsValueInt() >= highestCardsValueInt) {
		////            playerNeedToShowCards.push_back((*it_c)->getMyUniqueID());
		////            highestCardsValueInt = (*it_c)->getMyCardsValueInt();
		////        }
		//    }

	}


	// sort and unique the list
	playerNeedToShowCards.sort();
	playerNeedToShowCards.unique();

//    std::_List_iterator<unsigned> playerNeedToShowCardsIt;
//
//    for(playerNeedToShowCardsIt = playerNeedToShowCards.begin(); playerNeedToShowCardsIt!=playerNeedToShowCards.end(); playerNeedToShowCardsIt++) {
//        cout << (*playerNeedToShowCardsIt) << '\t';
//    }
//    cout << endl;

}
