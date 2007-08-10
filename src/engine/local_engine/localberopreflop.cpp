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

LocalBeRoPreflop::LocalBeRoPreflop(HandInterface* hi, int id, int qP, int dP, int sB) : LocalBeRo(hi, id, qP, dP, sB, GAME_STATE_PREFLOP), bigBlindPosition(0)

{
	setHighestSet(2*getSmallBlind());

// // 	BigBlind ermitteln 
	bigBlindPosition = getDealerPosition();

	int i;
	for(i=0; i<MAX_NUMBER_OF_PLAYERS && getMyHand()->getPlayerArray()[bigBlindPosition]->getMyButton() != BUTTON_BIG_BLIND; i++) {
		bigBlindPosition = (bigBlindPosition+1)%(MAX_NUMBER_OF_PLAYERS);
	}

// // 	erste Spielernummer fr preflopRun() setzen
	setPlayersTurn(bigBlindPosition);

}



LocalBeRoPreflop::~LocalBeRoPreflop()
{



}

void LocalBeRoPreflop::run() {

	int i;
	bool allHighestSet = 1;

	// prfe, ob alle Sets gleich sind ( falls nicht, dann allHighestSet = 0 )
	for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
		if(getMyHand()->getPlayerArray()[i]->getMyActiveStatus() && getMyHand()->getPlayerArray()[i]->getMyAction() != PLAYER_ACTION_FOLD && getMyHand()->getPlayerArray()[i]->getMyAction() != PLAYER_ACTION_ALLIN)	{
			if(getHighestSet() != getMyHand()->getPlayerArray()[i]->getMySet()) { allHighestSet=0; }
		}
	}

	// BigBlind ermitteln
	bigBlindPosition = getDealerPosition();

	for(i=0; i<MAX_NUMBER_OF_PLAYERS && getMyHand()->getPlayerArray()[bigBlindPosition]->getMyButton() != BUTTON_BIG_BLIND; i++) {
		bigBlindPosition = (bigBlindPosition+1)%(MAX_NUMBER_OF_PLAYERS);
	}

	// naechsten Spieler ermitteln
	for(i=0; (i<MAX_NUMBER_OF_PLAYERS && (!(getMyHand()->getPlayerArray()[getPlayersTurn()]->getMyActiveStatus()) || getMyHand()->getPlayerArray()[getPlayersTurn()]->getMyAction() == PLAYER_ACTION_FOLD || getMyHand()->getPlayerArray()[getPlayersTurn()]->getMyAction() == PLAYER_ACTION_ALLIN)) || i==0; i++) {

		setPlayersTurn((getPlayersTurn()+1)%(MAX_NUMBER_OF_PLAYERS));
		// falls BigBlind, dann PreflopFirstRound zuende
		if(getMyHand()->getPlayerArray()[getPlayersTurn()]->getMyButton() == BUTTON_BIG_BLIND && getMyHand()->getPlayerArray()[bigBlindPosition]->getMySet() < 2*getSmallBlind()) setFirstRound(0);

	}

	// prfen, ob Preflop wirklich dran ist
	if(!getFirstRound() && allHighestSet) { 

		// Preflop nicht dran, weil wir nicht mehr in erster PreflopRunde und alle Sets gleich sind
		//also gehe in Flop
		getMyHand()->setActualRound(GAME_STATE_FLOP);
		
		//Action loeschen und ActionButtons refresh
		for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
			if(getMyHand()->getPlayerArray()[i]->getMyAction() != PLAYER_ACTION_FOLD && getMyHand()->getPlayerArray()[i]->getMyAction() != PLAYER_ACTION_ALLIN) getMyHand()->getPlayerArray()[i]->setMyAction(PLAYER_ACTION_NONE);
		}
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
		if(getMyHand()->getPlayerArray()[getPlayersTurn()]->getMyButton() == BUTTON_BIG_BLIND) setFirstRound(0);

		getMyHand()->getPlayerArray()[getPlayersTurn()]->setMyTurn(1);
		//highlight active players groupbox and clear action
		getMyHand()->getGuiInterface()->refreshGroupbox(getPlayersTurn(),2);
		getMyHand()->getGuiInterface()->refreshAction(getPlayersTurn(),PLAYER_ACTION_NONE);

		if(getPlayersTurn() == 0) {
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
