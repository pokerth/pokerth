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

using namespace std;

LocalBoard::LocalBoard() : BoardInterface(), playerArray(0), currentHand(0), pot(0), sets(0)
{
}


LocalBoard::~LocalBoard()
{
}

void LocalBoard::setPlayerLists(std::vector<boost::shared_ptr<PlayerInterface> > sl, PlayerList apl, PlayerList rpl) {
	playerArray = sl;
	activePlayerList = apl;
	runningPlayerList = rpl;
}

void LocalBoard::setHand(HandInterface* br) { currentHand = br; }

void LocalBoard::collectSets() {

	sets = 0;
	int i;
	for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) sets += playerArray[i]->getMySet();

}

void LocalBoard::collectPot() { 
	int i;
	pot += sets; 
	sets = 0;
	for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++){ playerArray[i]->setMySetNull(); }
}

void LocalBoard::distributePot() {

	size_t i,j,k,l;

	// filling player sets vector
	vector<int> playerSets;
	for(i=0; i<(size_t)MAX_NUMBER_OF_PLAYERS; i++) {
		if(playerArray[i]->getMyActiveStatus()) {
			playerSets.push_back(((playerArray[i]->getMyRoundStartCash())-(playerArray[i]->getMyCash())));
		} else {
			playerSets.push_back(0);
		}
	}

	// sort player sets asc
	vector<int> playerSetsSort = playerSets;
	sort(playerSetsSort.begin(), playerSetsSort.end());

	// potLevel[0] = amount, potLevel[1] = sum, potLevel[2..n] = winner
	vector<int> potLevel;

	// temp var
	int highestCardsValue;
	size_t winnerCount;
	size_t mod;
	int winnerPointer;
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
			for(j=0; j<MAX_NUMBER_OF_PLAYERS; j++) {

				if(playerArray[j]->getMyCardsValueInt() > highestCardsValue && playerArray[j]->getMyActiveStatus() && playerArray[j]->getMyAction() != PLAYER_ACTION_FOLD && playerSets[j] >= potLevel[0]) { 
					highestCardsValue = playerArray[j]->getMyCardsValueInt(); 
				}
			}

			// level winners
			for(j=0; j<MAX_NUMBER_OF_PLAYERS; j++) {
				if(highestCardsValue == playerArray[j]->getMyCardsValueInt() && playerArray[j]->getMyActiveStatus() && playerArray[j]->getMyAction() != PLAYER_ACTION_FOLD && playerSets[j] >= potLevel[0]) {
					potLevel.push_back(playerArray[j]->getMyID());
				}
			}

			// determine the number of level winners
			winnerCount = potLevel.size()-2;

			// distribute the pot level sum to level winners
			mod = (potLevel[1])%winnerCount;
			// pot level sum divisible by winnerCount
			if(mod == 0) {
				for(j=2; j<potLevel.size(); j++) {
					playerArray[potLevel[j]]->setMyCash(playerArray[potLevel[j]]->getMyCash() + ((potLevel[1])/winnerCount));
				}

			}
			// pot level sum not divisible by winnerCount
			// --> distribution after smallBlind (perhaps lastActionPlayer? - TODO)
			else {

				winnerPointer = currentHand->getDealerPosition();

				for(j=0; j<winnerCount; j++) {

					winnerHit = false;

					for(k=0; k<MAX_NUMBER_OF_PLAYERS && !winnerHit; k++){

						winnerPointer = (winnerPointer+1)%(MAX_NUMBER_OF_PLAYERS);

						winnerHit = false;

						for(l=2; l<potLevel.size(); l++) {
							if(winnerPointer == potLevel[l]) winnerHit = true;
						}

					}

					if(j<mod) {
						playerArray[winnerPointer]->setMyCash(playerArray[winnerPointer]->getMyCash() + (int)((potLevel[1])/winnerCount) + 1);
					} else {
						playerArray[winnerPointer]->setMyCash(playerArray[winnerPointer]->getMyCash() + (int)((potLevel[1])/winnerCount));
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

	// ERROR-Outputs

	if(pot!=0) cout << "distributePot-ERROR: Pot = " << pot << endl;

	int sum = 0;
	for(i=0; i<(size_t)MAX_NUMBER_OF_PLAYERS; i++) {
		if(playerArray[i]->getMyActiveStatus())
			sum += playerArray[i]->getMyCash();
	}

	if(sum != (currentHand->getStartQuantityPlayers() * currentHand->getStartCash()))
		cout << "distributePot-ERROR: PlayersSumCash = " << sum << endl;

}

