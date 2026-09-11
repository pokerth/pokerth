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

#include "localberopreflop.h"

#include "localexception.h"
#include "engine_msg.h"

#include <handinterface.h>
#include <game_defs.h>

using namespace std;

LocalBeRoPreflop::LocalBeRoPreflop(HandInterface* hi, unsigned dP, int sB) : LocalBeRo(hi, dP, sB, GAME_STATE_PREFLOP)
{
	setHighestSet(2*getSmallBlind());
}



LocalBeRoPreflop::~LocalBeRoPreflop()
{
}

void LocalBeRoPreflop::run()
{

	if(getFirstRun()) {

		PlayerListIterator it;

		// search bigBlindPosition in runningPlayerList
		PlayerListIterator bigBlindPositionIt = getMyHand()->getRunningPlayerIt(getBigBlindPositionId());

		// more than 2 players are still active -> runningPlayerList is not empty
		if(getMyHand()->getActivePlayerList()->size() > 2) {

			// bigBlindPlayer not found in runningPlayerList (he is all in) -> bigBlindPlayer is not the running player before first action player
			if(bigBlindPositionIt == getMyHand()->getRunningPlayerList()->end()) {

				// search smallBlindPosition in runningPlayerList
				PlayerListIterator smallBlindPositionIt = getMyHand()->getRunningPlayerIt(getSmallBlindPositionId());

				// smallBlindPlayer not found in runningPlayerList (he is all in) -> next active player before smallBlindPlayer is running player before first action player
				if(smallBlindPositionIt == getMyHand()->getRunningPlayerList()->end()) {

					it = getMyHand()->getActivePlayerIt(getSmallBlindPositionId());
					if(it == getMyHand()->getActivePlayerList()->end()) {
						throw LocalException(__FILE__, __LINE__, ERR_ACTIVE_PLAYER_NOT_FOUND);
					}

					if(it == getMyHand()->getActivePlayerList()->begin()) it = getMyHand()->getActivePlayerList()->end();
					--it;

					setFirstRoundLastPlayersTurnId( (*it)->getMyUniqueID() );

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

		setFirstRun(false);

	}

	bool allHighestSet = true;
	PlayerListConstIterator it_c;

	// check if all running players have same sets (else allHighestSet = false)
	for(it_c=getMyHand()->getRunningPlayerList()->begin(); it_c!=getMyHand()->getRunningPlayerList()->end(); ++it_c) {
		if(getHighestSet() != (*it_c)->getMySet()) {
			allHighestSet = false;
			break;
		}
	}

	// determine next player
	PlayerListConstIterator currentPlayersTurnIt = getMyHand()->getRunningPlayerIt( getCurrentPlayersTurnId() );
	if(currentPlayersTurnIt == getMyHand()->getRunningPlayerList()->end()) {
		throw LocalException(__FILE__, __LINE__, ERR_RUNNING_PLAYER_NOT_FOUND);
	}

	++currentPlayersTurnIt;
	if(currentPlayersTurnIt == getMyHand()->getRunningPlayerList()->end()) currentPlayersTurnIt = getMyHand()->getRunningPlayerList()->begin();

	setCurrentPlayersTurnId( (*currentPlayersTurnIt)->getMyUniqueID() );

	// check whether preflop is really up
	if(!getFirstRound() && allHighestSet) {

		// preflop is not up, because we are no longer in the first preflop round and all sets are equal
		//so go to the flop
		getMyHand()->setCurrentRound(GAME_STATE_FLOP);

		//clear the action and refresh the action buttons
		for(it_c=getMyHand()->getRunningPlayerList()->begin(); it_c!=getMyHand()->getRunningPlayerList()->end(); ++it_c) {
			(*it_c)->setMyAction(PLAYER_ACTION_NONE);
		}

		//move the sets into the pot, set sets = 0 and refresh the pot
		getMyHand()->getBoard()->collectSets();
		getMyHand()->getBoard()->collectPot();
		getMyHand()->getGuiInterface()->refreshPot();

		getMyHand()->getGuiInterface()->refreshSet();
		getMyHand()->getGuiInterface()->refreshCash();
		for(int i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
			getMyHand()->getGuiInterface()->refreshAction(i,PLAYER_ACTION_NONE);
		}

		getMyHand()->switchRounds();
	} else {
		// lastPlayersTurn -> PreflopFirstRound is over
		if( getCurrentPlayersTurnId() == getFirstRoundLastPlayersTurnId() ) {
			setFirstRound(false);
		}

		currentPlayersTurnIt = getMyHand()->getRunningPlayerIt( getCurrentPlayersTurnId() );
		if(currentPlayersTurnIt == getMyHand()->getRunningPlayerList()->end()) {
			throw LocalException(__FILE__, __LINE__, ERR_RUNNING_PLAYER_NOT_FOUND);
		}
		(*currentPlayersTurnIt)->setMyTurn(true);

		//highlight active players groupbox and clear action
		getMyHand()->getGuiInterface()->refreshGroupbox( getCurrentPlayersTurnId(), 2 );
		getMyHand()->getGuiInterface()->refreshAction( getCurrentPlayersTurnId(), PLAYER_ACTION_NONE );


		if( getCurrentPlayersTurnId() == 0) {
			// It is our turn
			getMyHand()->getGuiInterface()->meInAction();
		} else {
			//it is the opponents' turn
			getMyHand()->getGuiInterface()->beRoAnimation2(getMyBeRoID());
		}
	}
}
