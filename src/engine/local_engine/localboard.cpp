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

LocalBoard::LocalBoard() : BoardInterface(), currentHand(0), pot(0), sets(0)
{
}

LocalBoard::~LocalBoard()
{
}

void LocalBoard::setPlayerLists(PlayerList sl, PlayerList apl, PlayerList rpl) {
	seatsList = sl;
	activePlayerList = apl;
	runningPlayerList = rpl;
}

void LocalBoard::setHand(HandInterface* br) { currentHand = br; }

void LocalBoard::collectSets() {

	sets = 0;

	PlayerListConstIterator it_c;
	for(it_c=seatsList->begin(); it_c!=seatsList->end(); it_c++) {
		sets += (*it_c)->getMySet();
	}

}

void LocalBoard::collectPot() { 

	pot += sets; 
	sets = 0;

	PlayerListIterator it;
	for(it=seatsList->begin(); it!=seatsList->end(); it++) {
		(*it)->setMySetNull();
	}

}

void LocalBoard::distributePot() {

	winners.clear();

	size_t i,j,k,l;
	PlayerListIterator it;
	PlayerListConstIterator it_c;

	// filling player sets vector
	vector<unsigned> playerSets;
	for(it=seatsList->begin(); it!=seatsList->end(); it++) {
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
			for(it_c=seatsList->begin(), j=0; it_c!=seatsList->end(); it_c++,j++) {
				if((*it_c)->getMyActiveStatus() && (*it_c)->getMyCardsValueInt() > highestCardsValue && (*it_c)->getMyAction() != PLAYER_ACTION_FOLD && playerSets[j] >= potLevel[0]) { 
					highestCardsValue = (*it_c)->getMyCardsValueInt(); 
				}
			}

			// level winners
			for(it_c=seatsList->begin(), j=0; it_c!=seatsList->end(); it_c++,j++) {
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
					it = currentHand->getSeatIt(potLevel[j]);
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
				it = currentHand->getSeatIt(currentHand->getDealerPosition());
				if(it == seatsList->end()) {
					throw LocalException(__FILE__, __LINE__, ERR_SEAT_NOT_FOUND);
				}

				for(j=0; j<winnerCount; j++) {

					winnerHit = false;

					for(k=0; k<MAX_NUMBER_OF_PLAYERS && !winnerHit; k++){

// 						winnerPointer = (winnerPointer+1)%(MAX_NUMBER_OF_PLAYERS);

// 						winnerHit = false;

						it++;
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

	int sum = 0;

	for(it_c=activePlayerList->begin(); it_c!=activePlayerList->end(); it_c++) {
		sum += (*it_c)->getMyCash();
	}
}
