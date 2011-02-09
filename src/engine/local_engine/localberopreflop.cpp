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

#include "localexception.h"
#include "engine_msg.h"

#include <handinterface.h>
#include <game_defs.h>

using namespace std;

LocalBeRoPreflop::LocalBeRoPreflop(HandInterface* hi, int id, unsigned dP, int sB) : LocalBeRo(hi, id, dP, sB, GAME_STATE_PREFLOP)
{
	setHighestSet(2*getSmallBlind());
}



LocalBeRoPreflop::~LocalBeRoPreflop()
{
}

void LocalBeRoPreflop::run() {

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
	
	// prfen, ob Preflop wirklich dran ist
	if(!getFirstRound() && allHighestSet) { 

		// Preflop nicht dran, weil wir nicht mehr in erster PreflopRunde und alle Sets gleich sind
		//also gehe in Flop
		getMyHand()->setCurrentRound(GAME_STATE_FLOP);

		//Action loeschen und ActionButtons refresh
		for(it_c=getMyHand()->getRunningPlayerList()->begin(); it_c!=getMyHand()->getRunningPlayerList()->end(); ++it_c) {
			(*it_c)->setMyAction(PLAYER_ACTION_NONE);
		}

		//Sets in den Pot verschieben und Sets = 0 und Pot-refresh
		getMyHand()->getBoard()->collectSets();
		getMyHand()->getBoard()->collectPot();
		getMyHand()->getGuiInterface()->refreshPot();
		
		getMyHand()->getGuiInterface()->refreshSet();
		getMyHand()->getGuiInterface()->refreshCash();
		for(int i=0; i<MAX_NUMBER_OF_PLAYERS; i++) { getMyHand()->getGuiInterface()->refreshAction(i,PLAYER_ACTION_NONE); }

		getMyHand()->switchRounds();
	}
	else {
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
		getMyHand()->getGuiInterface()->refreshGroupbox( getCurrentPlayersTurnId() , 2 );
		getMyHand()->getGuiInterface()->refreshAction( getCurrentPlayersTurnId() , PLAYER_ACTION_NONE );


		if( getCurrentPlayersTurnId() == 0) {
			// Wir sind dran
			getMyHand()->getGuiInterface()->meInAction();
		}
		else {
			//Gegner sind dran
			getMyHand()->getGuiInterface()->beRoAnimation2(getMyBeRoID());
		}
	}
}
