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

LocalBeRoPreflop::LocalBeRoPreflop(HandInterface* hi, int id, unsigned dP, int sB) : LocalBeRo(hi, id, dP, sB, GAME_STATE_PREFLOP)
{
// 	PlayerListConstIterator it_c;

	setHighestSet(2*getSmallBlind());




}



LocalBeRoPreflop::~LocalBeRoPreflop()
{



}

void LocalBeRoPreflop::run() {

	if(getFirstRun()) {








	// determine bigBlindPosition -> old: delete
// 	for(bigBlindPositionIt=getMyHand()->getActivePlayerList()->begin(); bigBlindPositionIt!=getMyHand()->getActivePlayerList()->end(); bigBlindPositionIt++) {
// 		if((*bigBlindPositionIt)->getMyButton() == BUTTON_BIG_BLIND) break;
// 	}


	// search player in runningPlayerList before first action player -> run() determine next player who will do first action
// 	PlayerListIterator it_1, it_2, it_3;
	PlayerListIterator it;

	// search bigBlindPosition in runningPlayerList -> old: delete
// 	it_1 = find(getMyHand()->getRunningPlayerList()->begin(), getMyHand()->getRunningPlayerList()->end(), *bigBlindPositionIt);

	// search bigBlindPosition in runningPlayerList
	PlayerListIterator bigBlindPositionIt = getMyHand()->getRunningPlayerIt(getBigBlindPositionId());

	// more than 2 players are still active -> runningPlayerList is not empty
	if(getMyHand()->getActivePlayerList()->size() > 2) {

		// bigBlindPlayer not found in runningPlayerList (he is all in) -> bigBlindPlayer is not the running player before first action player
		if(bigBlindPositionIt == getMyHand()->getRunningPlayerList()->end()) {

			// search smallBlindPosition in runningPlayerList
			PlayerListIterator smallBlindPositionIt = getMyHand()->getRunningPlayerIt(getSmallBlindPositionId());
// 			if(smallBlindPositionIt == getMyHand()->getActivePlayerList()->begin()) smallBlindPositionIt = getMyHand()->getActivePlayerList()->end();
// 			smallBlindPositionIt--;
// 			it_2 = find(getMyHand()->getRunningPlayerList()->begin(), getMyHand()->getRunningPlayerList()->end(), *smallBlindPositionIt);

			// smallBlindPlayer not found in runningPlayerList (he is all in) -> next active player before smallBlindPlayer is running player before first action player
			if(smallBlindPositionIt == getMyHand()->getRunningPlayerList()->end()) {

				it = getMyHand()->getActivePlayerIt(getSmallBlindPositionId());
				assert(it != getMyHand()->getActivePlayerList()->end());

				if(it == getMyHand()->getActivePlayerList()->begin()) it = getMyHand()->getActivePlayerList()->end();
				it--;

				setFirstRoundLastPlayersTurnId( (*it)->getMyUniqueID() );


// 				it_3 = find(getMyHand()->getRunningPlayerList()->begin(), getMyHand()->getRunningPlayerList()->end(), *it_2);
// 				if(it_3 == getMyHand()->getRunningPlayerList()->end()) {
// 					cout << "ERROR - lastPlayersTurnIt-detection in localBeRoPreflop" << endl;
// 				} else {
// 					setLastPlayersTurnIt(it_3);
// 				}
			}
			// smallBlindPlayer found in runningPlayerList -> running player before first action player
			else {
				setFirstRoundLastPlayersTurnId( getSmallBlindPositionId() );
			}
		}
		// bigBlindPlayer found in runningPlayerList -> player before first action player
		else {
			setFirstRoundLastPlayersTurnId( getBigBlindPositionId() );
		}
	}
	// heads up -> dealer/smallBlindPlayer is first action player and bigBlindPlayer is player before
	else {

		// bigBlindPlayer not found in runningPlayerList (he is all in) -> only smallBlind has to choose fold or call the bigBlindAmount
		if(bigBlindPositionIt == getMyHand()->getRunningPlayerList()->end()) {

			// search smallBlindPosition in runningPlayerList
			PlayerListIterator smallBlindPositionIt = getMyHand()->getRunningPlayerIt(getSmallBlindPositionId());

			// smallBlindPlayer not found in runningPlayerList (he is all in) -> no running player -> showdown and no firstRoundLastPlayersTurnId is used
			if(smallBlindPositionIt == getMyHand()->getRunningPlayerList()->end()) {

			}
			// smallBlindPlayer found in runningPlayerList -> running player before first action player (himself)
			else {
				setFirstRoundLastPlayersTurnId( getSmallBlindPositionId() );
			}


		} else {
			setFirstRoundLastPlayersTurnId( getBigBlindPositionId() );
		}
	}

	setCurrentPlayersTurnId( getFirstRoundLastPlayersTurnId() );

// 	cout << "playerID constructor beropreflop: " << (*(getCurrentPlayersTurnIt()))->getMyID() << endl;


	/////////////// testing




// 	it_1 = find(getMyHand()->getRunningPlayerList()->begin(), getMyHand()->getRunningPlayerList()->end(), *bigBlindPositionIt);

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











		setFirstRun(false);
	}








	int i;
	bool allHighestSet = true;
// 	PlayerListIterator it;
	PlayerListConstIterator it_c;



	// test if all running players have same sets (else allHighestSet = false)
	for(it_c=getMyHand()->getRunningPlayerList()->begin(); it_c!=getMyHand()->getRunningPlayerList()->end(); it_c++) {
		if(getHighestSet() != (*it_c)->getMySet()) {
			allHighestSet = false;
			break;
		}
	}

	// determine next player

// 	cout << "playerID begin preflopRun(): " << (*(getCurrentPlayersTurnIt()))->getMyID() << endl;

// 	it++;



// 	cout << "size: " << getMyHand()->getRunningPlayerList()->size() << endl;

// 	cout << "currentPlayerID: " << getCurrentPlayersTurnId() << endl;

	PlayerListConstIterator currentPlayersTurnIt = getMyHand()->getRunningPlayerIt( getCurrentPlayersTurnId() );
	assert( currentPlayersTurnIt != getMyHand()->getRunningPlayerList()->end() );

	currentPlayersTurnIt++;
	if(currentPlayersTurnIt == getMyHand()->getRunningPlayerList()->end()) currentPlayersTurnIt = getMyHand()->getRunningPlayerList()->begin();

	setCurrentPlayersTurnId( (*currentPlayersTurnIt)->getMyUniqueID() );

// 	setCurrentPlayersTurnIt(++getCurrentPlayersTurnIt());
// 	if(getCurrentPlayersTurnIt() == getMyHand()->getRunningPlayerList()->end()) setCurrentPlayersTurnIt(getMyHand()->getRunningPlayerList()->begin());

// 	if(getCurrentPlayersTurnIt() == getMyHand()->getRunningPlayerList()->end()) cout << "!!!" << endl;

// 	cout << "playerID middle preflopRun(): " << (*(getCurrentPlayersTurnIt()))->getMyID() << endl;








	PlayerListConstIterator currentPlayersTurnConstIt;
	// exceptions
		// no.1: if in first Preflop Round next player is small blind and only all-in-big-blind with less than smallblind amount is nonfold too -> preflop is over
// 		if(getFirstRound()) {
// 			PlayerListConstIterator bigBlindPositionIt = getMyHand()->getActivePlayerIt(getBigBlindPositionId());
// 			assert(bigBlindPositionIt != getMyHand()->getActivePlayerList()->end());
// 	
// 			currentPlayersTurnConstIt = getMyHand()->getRunningPlayerIt( getCurrentPlayersTurnId() );
// 			assert(currentPlayersTurnConstIt != getMyHand()->getRunningPlayerList()->end());
// 	
// 			int nonFoldPlayerCounter = 0;
// 			for (it_c=getMyHand()->getActivePlayerList()->begin(); it_c!=getMyHand()->getActivePlayerList()->end(); it_c++) {
// 				if ((*it_c)->getMyAction() != PLAYER_ACTION_FOLD) nonFoldPlayerCounter++;
// 			}
// 	
// 			if((*currentPlayersTurnConstIt)->getMyButton() == BUTTON_SMALL_BLIND && (*bigBlindPositionIt)->getMySet() <= getSmallBlind() && nonFoldPlayerCounter == 2) {
// 				setFirstRound(false);
// 				allHighestSet = true;
// 				getMyHand()->setAllInCondition(true);
// 			}
// 		}
















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

// 	cout << "firstRound. " << getFirstRound() << " - allHighestSet: " << allHighestSet << endl;
	
	// prfen, ob Preflop wirklich dran ist
	if(!getFirstRound() && allHighestSet) { 

		// Preflop nicht dran, weil wir nicht mehr in erster PreflopRunde und alle Sets gleich sind
		//also gehe in Flop
		getMyHand()->setActualRound(GAME_STATE_FLOP);

		// exception no.1 - see above
// 		if(getMyHand()->getAllInCondition()) getMyHand()->setActualRound(GAME_STATE_PREFLOP);;
		
		//Action loeschen und ActionButtons refresh
		for(it_c=getMyHand()->getRunningPlayerList()->begin(); it_c!=getMyHand()->getRunningPlayerList()->end(); it_c++) {
			(*it_c)->setMyAction(PLAYER_ACTION_NONE);
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
// 		if( (*(getCurrentPlayersTurnIt())) == (*(getLastPlayersTurnIt())) ) {
// 			setFirstRound(false);
// 		}

		// lastPlayersTurn -> PreflopFirstRound is over
		if( getCurrentPlayersTurnId() == getFirstRoundLastPlayersTurnId() ) {
			setFirstRound(false);
		}


// 		getMyHand()->getPlayerArray()[getPlayersTurn()]->setMyTurn(1);

		currentPlayersTurnConstIt = getMyHand()->getRunningPlayerIt( getCurrentPlayersTurnId() );
		assert(currentPlayersTurnConstIt != getMyHand()->getRunningPlayerList()->end());
		(*currentPlayersTurnConstIt)->setMyTurn(true);

// 		(*(getCurrentPlayersTurnIt()))->setMyTurn(true);

		//highlight active players groupbox and clear action
// 		getMyHand()->getGuiInterface()->refreshGroupbox(getPlayersTurn(),2);
// 		getMyHand()->getGuiInterface()->refreshAction(getPlayersTurn(),PLAYER_ACTION_NONE);

		//highlight active players groupbox and clear action
		getMyHand()->getGuiInterface()->refreshGroupbox( getCurrentPlayersTurnId() , 2 );
		getMyHand()->getGuiInterface()->refreshAction( getCurrentPlayersTurnId() , PLAYER_ACTION_NONE );


		if( getCurrentPlayersTurnId() == 0) {
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
