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
#include "localpreflop.h"
#include "localhand.h"

using namespace std;

LocalPreflop::LocalPreflop(LocalHand* bR, int id, int qP, int dP, int sB) : myHand(bR), myID(id), actualQuantityPlayers(qP), dealerPosition(dP), bigBlindPosition(0), smallBlind(sB), highestSet(2*sB), preflopFirstRound(TRUE), playersTurn(0)

{

	myHand->getBoard()->collectSets();
	myHand->getMainWindowImpl()->refreshPot();

// // 	BigBlind ermitteln 
	bigBlindPosition = dealerPosition;
	while (myHand->getPlayerArray()[bigBlindPosition]->getMyButton() != 3) {
		bigBlindPosition = (bigBlindPosition+1)%(myHand->getMainWindowImpl()->getMaxQuantityPlayers());
	}

// // 	erste Spielernummer fr preflopRun() setzen
	playersTurn = bigBlindPosition;
}



LocalPreflop::~LocalPreflop()
{



}

void LocalPreflop::preflopRun() {

// 	cout << "NextPlayerSpeed2 stop" << endl;
	int i;
	bool allHighestSet = 1;

	// prfe, ob alle Sets gleich sind ( falls nicht, dann allHighestSet = 0 )
	for(i=0; i<myHand->getMainWindowImpl()->getMaxQuantityPlayers(); i++) {
		if(myHand->getPlayerArray()[i]->getMyActiveStatus() && myHand->getPlayerArray()[i]->getMyAction() != 1 && myHand->getPlayerArray()[i]->getMyAction() != 6)	{
			if(highestSet != myHand->getPlayerArray()[i]->getMySet()) { allHighestSet=0; }
		}
	}

	// BigBlind ermitteln
	bigBlindPosition = dealerPosition;
	while (myHand->getPlayerArray()[bigBlindPosition]->getMyButton() != 3) {
		bigBlindPosition = (bigBlindPosition+1)%(myHand->getMainWindowImpl()->getMaxQuantityPlayers());
	}

	// prfen, ob Preflop wirklich dran ist
	if(!preflopFirstRound && allHighestSet) { 

		// Preflop nicht dran, weil wir nicht mehr in erster PreflopRunde und alle Sets gleich sind
		//also gehe in Flop
		myHand->setActualRound(1);
		
		//Action l�chen und ActionButtons refresh
		for(i=0; i<myHand->getMainWindowImpl()->getMaxQuantityPlayers(); i++) {
			if(myHand->getPlayerArray()[i]->getMyAction() != 1 && myHand->getPlayerArray()[i]->getMyAction() != 6) myHand->getPlayerArray()[i]->setMyAction(0);
		}
		//Sets in den Pot verschieben und Sets = 0 und Pot-refresh
		myHand->getBoard()->collectSets();
		myHand->getBoard()->collectPot();
		myHand->getMainWindowImpl()->refreshPot();
		myHand->getMainWindowImpl()->refreshAll();
		
		myHand->switchRounds();

	}
	else {
		// Preflop ist wirklich dran

		// n�hsten Spieler ermitteln
		do {

			playersTurn = (playersTurn+1)%(myHand->getMainWindowImpl()->getMaxQuantityPlayers());
			// falls BigBlind, dann PreflopFirstRound zuende
			if(myHand->getPlayerArray()[playersTurn]->getMyButton() == 3) preflopFirstRound = FALSE;

		} while(!(myHand->getPlayerArray()[playersTurn]->getMyActiveStatus()) || myHand->getPlayerArray()[playersTurn]->getMyAction() == 1 || myHand->getPlayerArray()[playersTurn]->getMyAction() == 6);

		myHand->getPlayerArray()[playersTurn]->setMyTurn(TRUE);
		myHand->getMainWindowImpl()->refreshGroupbox();

		if(playersTurn == 0) {
			// Wir sind dran
			myHand->getMainWindowImpl()->meInAction();
		}
		else {
			//Gegner sind dran
// 			cout << "NextPlayerSpeed3 start" << endl;
			myHand->getGuiInterface()->preflopAnimation2();
		}
	}
}

void LocalPreflop::nextPlayer2() {
// 	cout << "NextPlayerSpeed3 stop" << endl;
	myHand->getPlayerArray()[playersTurn]->action();

}
