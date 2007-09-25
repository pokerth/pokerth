/***************************************************************************
 *   Copyright (C) 2006 by FThauer FHammer   *
 *   f.thauer@web.de   *Irische Segenswnsche, 
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
#include "localberopreflop.h"

#include <handinterface.h>
#include <game_defs.h>

using namespace std;

LocalBeRoPreflop::LocalBeRoPreflop(HandInterface* hi, int id, int dP, int sB) : LocalBeRo(hi, id, dP, sB, GAME_STATE_PREFLOP)
{
	setHighestSet(2*getSmallBlind());

	// determine bigBlindPosition
	for(bigBlindPositionIt=getMyHand()->getActivePlayerList()->begin(); bigBlindPositionIt!=getMyHand()->getActivePlayerList()->end(); bigBlindPositionIt++) {
		if((*bigBlindPositionIt)->getMyButton() == BUTTON_BIG_BLIND) break;
	}

	// search player in runningPlayerList before first action player -> run() determine next player who will do first action
	PlayerListIterator it_1, it_2, it_3;

	// search bigBlindPosition in runningPlayerList
	it_1 = find(getMyHand()->getRunningPlayerList()->begin(), getMyHand()->getRunningPlayerList()->end(), *bigBlindPositionIt);

	// more than 2 players are still active -> runningPlayerList is not empty
	if(getMyHand()->getActivePlayerList()->size() > 2) {

		// bigBlindPlayer not found in runningPlayerList (he is all in) -> bigBlindPlayer is not the running player before first action player
		if(it_1 == getMyHand()->getRunningPlayerList()->end()) {

			// search smallBlindPosition in runningPlayerList
			PlayerListIterator smallBlindPositionIt = bigBlindPositionIt;
			if(smallBlindPositionIt == getMyHand()->getActivePlayerList()->begin()) smallBlindPositionIt = getMyHand()->getActivePlayerList()->end();
			smallBlindPositionIt--;
			it_2 = find(getMyHand()->getRunningPlayerList()->begin(), getMyHand()->getRunningPlayerList()->end(), *smallBlindPositionIt);

			// smallBlindPlayer not found in runningPlayerList (he is all in) -> next active player before smallBlindPlayer is running player before first action player
			if(it_2 == getMyHand()->getRunningPlayerList()->end()) {
				it_2 = smallBlindPositionIt;
				if(it_2 == getMyHand()->getActivePlayerList()->begin()) it_2 = getMyHand()->getActivePlayerList()->end();
				it_2--;
				it_3 = find(getMyHand()->getRunningPlayerList()->begin(), getMyHand()->getRunningPlayerList()->end(), *it_2);
				if(it_3 == getMyHand()->getRunningPlayerList()->end()) {
					cout << "ERROR - lastPlayersTurnIt-detection in localBeRoPreflop" << endl;
				} else {
					setLastPlayersTurnIt(it_3);
				}
			}
			// smallBlindPlayer found in runningPlayerList -> running player before first action player
			else {
				setLastPlayersTurnIt(it_2);
			}
		}
		// bigBlindPlayer found in runningPlayerList -> player before first action player
		else {
			setLastPlayersTurnIt(it_1);
		}
	}
	// heads up -> dealer/smallBlindPlayer is first action player and bigBlindPlayer is player before
	else {
		setLastPlayersTurnIt(it_1);
	}

	setCurrentPlayersTurnIt(getLastPlayersTurnIt());

	cout << "playerID constructor preflopRun(): " << (*(getCurrentPlayersTurnIt()))->getMyID() << endl;


	/////////////// testing




	it_1 = find(getMyHand()->getRunningPlayerList()->begin(), getMyHand()->getRunningPlayerList()->end(), *bigBlindPositionIt);

	// bigBlindPosition is not all in
// 	if(it!=getMyHand()->getRunningPlayerList()->end()) {
// 		
// 	}


// 	if(it_1!=getMyHand()->getRunningPlayerList()->end()) {
// 		cout << getMyHand()->getMyID() << ": " << "gefunden!" << endl;
// 		cout << "\t PlayerID: " << (*it_1)->getMyID() << endl;
// 		it_2 = it_1;
// 		if(it_1 == getMyHand()->getRunningPlayerList()->begin()) it_1 = getMyHand()->getRunningPlayerList()->end();
// 		it_1--;
// 		cout << "\t PlayerID vor bigBlind: " << (*it_1)->getMyID() << endl;
// 		cout << "\t PlayerID vor bigBlind: " << (*it_2)->getMyID() << endl;
// 	} else {
// 		cout << getMyHand()->getMyID() << ": "  << "nicht gefunden" << endl;
// 		
// 	}


	//////////////// old

	// set first runningPlayer before bigBlindPlayer to playersTurn -> run() determine next one who will do first action
// 	setPlayersTurn((*bigBlindPositionIt)->getMyID());




}



LocalBeRoPreflop::~LocalBeRoPreflop()
{



}

void LocalBeRoPreflop::run() {

	int i;
	bool allHighestSet = true;
	PlayerListIterator it;



	// test if all running players have same sets (else allHighestSet = false)
	for(it=getMyHand()->getRunningPlayerList()->begin(); it!=getMyHand()->getRunningPlayerList()->end(); it++) {
		if(getHighestSet() != (*it)->getMySet()) {
			allHighestSet = false;
		}
	}

	// determine next player

	cout << "playerID begin preflopRun(): " << (*(getCurrentPlayersTurnIt()))->getMyID() << endl;

	it = getCurrentPlayersTurnIt();

	it++;

	setCurrentPlayersTurnIt(it);

	if(getCurrentPlayersTurnIt() == getMyHand()->getRunningPlayerList()->end()) setCurrentPlayersTurnIt(getMyHand()->getRunningPlayerList()->begin());

	if(getCurrentPlayersTurnIt() == getMyHand()->getRunningPlayerList()->end()) cout << "!!!" << endl;

	cout << "playerID middle preflopRun(): " << (*(getCurrentPlayersTurnIt()))->getMyID() << endl;

	// exceptions
		// if next player is small blind and only all-in-big-blind with less than smallblind amount is nonfold too -> preflop is over
		if((*(getCurrentPlayersTurnIt()))->getMyButton() == BUTTON_SMALL_BLIND && (*bigBlindPositionIt)->getMySet() <= getSmallBlind() && getMyHand()->getRunningPlayerList()->size() == 1) {
			setFirstRound(false);
			allHighestSet = true;
		}


	// determine next player
// 	for(i=0; (i<MAX_NUMBER_OF_PLAYERS && (!(getMyHand()->getPlayerArray()[getPlayersTurn()]->getMyActiveStatus()) || getMyHand()->getPlayerArray()[getPlayersTurn()]->getMyAction() == PLAYER_ACTION_FOLD || getMyHand()->getPlayerArray()[getPlayersTurn()]->getMyAction() == PLAYER_ACTION_ALLIN)) || i==0; i++) {
// 
// 		setPlayersTurn((getPlayersTurn()+1)%(MAX_NUMBER_OF_PLAYERS));
// 		// falls BigBlind, dann PreflopFirstRound zuende
// 		if(getMyHand()->getPlayerArray()[getPlayersTurn()]->getMyButton() == BUTTON_BIG_BLIND && (*bigBlindPositionIt)->getMySet() < 2*getSmallBlind()) setFirstRound(false);
// 		// if next player is small blind and only all-in-big-blind with <= 2*smallblind is nonfold too
// 		if(getMyHand()->getPlayerArray()[getPlayersTurn()]->getMyButton() == BUTTON_SMALL_BLIND && (*bigBlindPositionIt)->getMySet() <= getSmallBlind() && 		getMyHand()->getRunningPlayerList()->size() == 1) {
// 			setFirstRound(false);
// 			allHighestSet = true;
// 		}
// 	}

	// prfen, ob Preflop wirklich dran ist
	if(!getFirstRound() && allHighestSet) { 

		// Preflop nicht dran, weil wir nicht mehr in erster PreflopRunde und alle Sets gleich sind
		//also gehe in Flop
		getMyHand()->setActualRound(GAME_STATE_FLOP);
		
		//Action loeschen und ActionButtons refresh
		for(it=getMyHand()->getRunningPlayerList()->begin(); it!=getMyHand()->getRunningPlayerList()->end(); it++) {
			(*it)->setMyAction(PLAYER_ACTION_NONE);
		}

		//Action loeschen und ActionButtons refresh
// 		for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
// 			if(getMyHand()->getPlayerArray()[i]->getMyAction() != PLAYER_ACTION_FOLD && getMyHand()->getPlayerArray()[i]->getMyAction() != PLAYER_ACTION_ALLIN) getMyHand()->getPlayerArray()[i]->setMyAction(PLAYER_ACTION_NONE);
// 		}

		//Sets in den Pot verschieben und Sets = 0 und Pot-refresh
		getMyHand()->getBoard()->collectSets();
		getMyHand()->getBoard()->collectPot();
		getMyHand()->getGuiInterface()->refreshPot();
		
		getMyHand()->getGuiInterface()->refreshSet();
		getMyHand()->getGuiInterface()->refreshCash();
		for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) { getMyHand()->getGuiInterface()->refreshAction(i,PLAYER_ACTION_NONE); }

		getMyHand()->switchRounds();
	}
	else {
		// Preflop ist wirklich dran

		// falls BigBlind, dann PreflopFirstRound zuende
// 		if(getMyHand()->getPlayerArray()[getPlayersTurn()]->getMyButton() == BUTTON_BIG_BLIND) setFirstRound(0);

		// lastPlayersTurn -> PreflopFirstRound is over
		if( (*(getCurrentPlayersTurnIt())) == (*(getLastPlayersTurnIt())) ) {
			setFirstRound(false);
		}

// 		getMyHand()->getPlayerArray()[getPlayersTurn()]->setMyTurn(1);

		(*(getCurrentPlayersTurnIt()))->setMyTurn(true);

		//highlight active players groupbox and clear action
// 		getMyHand()->getGuiInterface()->refreshGroupbox(getPlayersTurn(),2);
// 		getMyHand()->getGuiInterface()->refreshAction(getPlayersTurn(),PLAYER_ACTION_NONE);

		//highlight active players groupbox and clear action
		getMyHand()->getGuiInterface()->refreshGroupbox( (*(getCurrentPlayersTurnIt()))->getMyID() , 2 );
		getMyHand()->getGuiInterface()->refreshAction( (*(getCurrentPlayersTurnIt()))->getMyID() , PLAYER_ACTION_NONE );


		if((*(getCurrentPlayersTurnIt()))->getMyID() == 0) {
			// Wir sind dran
			getMyHand()->getGuiInterface()->meInAction();
		}
		else {
			//Gegner sind dran
//			cout << "NextPlayerSpeed3 start" << endl;
			getMyHand()->getGuiInterface()->beRoAnimation2(getMyBeRoID());
		}
	}
}
