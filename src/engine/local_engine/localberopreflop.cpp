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
	highestSet = 2*smallBlind;

// // 	BigBlind ermitteln 
	bigBlindPosition = dealerPosition;
	while (myHand->getPlayerArray()[bigBlindPosition]->getMyButton() != 3) {
		bigBlindPosition = (bigBlindPosition+1)%(MAX_NUMBER_OF_PLAYERS);
	}

// // 	erste Spielernummer fr preflopRun() setzen
	playersTurn = bigBlindPosition;

	cout << "LocalBeroPreflop erstellt - " << myBeRoID << endl;
}



LocalBeRoPreflop::~LocalBeRoPreflop()
{



}

void LocalBeRoPreflop::run() {

	cout << "LocalBeroPreflopRun!!!" << endl;

// 	cout << "NextPlayerSpeed2 stop" << endl;
	int i;
	bool allHighestSet = 1;

	// prfe, ob alle Sets gleich sind ( falls nicht, dann allHighestSet = 0 )
	for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
		if(myHand->getPlayerArray()[i]->getMyActiveStatus() && myHand->getPlayerArray()[i]->getMyAction() != 1 && myHand->getPlayerArray()[i]->getMyAction() != 6)	{
			if(highestSet != myHand->getPlayerArray()[i]->getMySet()) { allHighestSet=0; }
		}
	}

	// BigBlind ermitteln
	bigBlindPosition = dealerPosition;
	while (myHand->getPlayerArray()[bigBlindPosition]->getMyButton() != 3) {
		bigBlindPosition = (bigBlindPosition+1)%(MAX_NUMBER_OF_PLAYERS);
	}

	// prfen, ob Preflop wirklich dran ist
	if(!firstRound && allHighestSet) { 

		// Preflop nicht dran, weil wir nicht mehr in erster PreflopRunde und alle Sets gleich sind
		//also gehe in Flop
		myHand->setActualRound(GAME_STATE_FLOP);
		
		//Action loeschen und ActionButtons refresh
		for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
			if(myHand->getPlayerArray()[i]->getMyAction() != 1 && myHand->getPlayerArray()[i]->getMyAction() != 6) myHand->getPlayerArray()[i]->setMyAction(0);
		}
		//Sets in den Pot verschieben und Sets = 0 und Pot-refresh
		myHand->getBoard()->collectSets();
		myHand->getBoard()->collectPot();
		myHand->getGuiInterface()->refreshPot();
		
		myHand->getGuiInterface()->refreshSet();
		myHand->getGuiInterface()->refreshCash();
		for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) { myHand->getGuiInterface()->refreshAction(i,0); }

		myHand->switchRounds();
	}
	else {
		// Preflop ist wirklich dran

		// naechsten Spieler ermitteln
		do {

			playersTurn = (playersTurn+1)%(MAX_NUMBER_OF_PLAYERS);
			// falls BigBlind, dann PreflopFirstRound zuende
			if(myHand->getPlayerArray()[playersTurn]->getMyButton() == 3) firstRound = 0;

		} while(!(myHand->getPlayerArray()[playersTurn]->getMyActiveStatus()) || myHand->getPlayerArray()[playersTurn]->getMyAction() == 1 || myHand->getPlayerArray()[playersTurn]->getMyAction() == 6);

		myHand->getPlayerArray()[playersTurn]->setMyTurn(1);
		//highlight active players groupbox and clear action
		myHand->getGuiInterface()->refreshGroupbox(playersTurn,2);
		myHand->getGuiInterface()->refreshAction(playersTurn,0);

		if(playersTurn == 0) {
			// Wir sind dran
			myHand->getGuiInterface()->meInAction();
		}
		else {
			//Gegner sind dran
//			cout << "NextPlayerSpeed3 start" << endl;
			myHand->getGuiInterface()->beRoAnimation2(myBeRoID);
		}
	}
}
