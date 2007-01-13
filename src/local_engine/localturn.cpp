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
#include "localturn.h"
#include "localhand.h"

using namespace std;

LocalTurn::LocalTurn(LocalHand* bR, int id, int qP, int dP, int sB) : myHand(bR), myID(id), actualQuantityPlayers(qP), dealerPosition(dP), smallBlindPosition(0), smallBlind(sB), highestSet(0), firstTurnRun(TRUE), firstTurnRound(TRUE), playersTurn(dP)

{	int i;

	//SmallBlind-Position ermitteln 
	for(i=0; i<myHand->getMainWindowImpl()->getMaxQuantityPlayers(); i++) {
		if (myHand->getPlayerArray()[i]->getMyButton() == 2) smallBlindPosition = i;
	}

}

LocalTurn::~LocalTurn()
{
}


void LocalTurn::turnRun() {

// 	cout << "NextPlayerSpeed2 stop" << endl;
	int i;

	if (firstTurnRun) {
		myHand->getGuiInterface()->dealTurnCard();
		firstTurnRun = FALSE;
		
	}

	else {
		bool allHighestSet = 1;

		// prfe, ob alle Sets gleich sind ( falls nicht, dann allHighestSet = 0 )
		for(i=0; i<myHand->getMainWindowImpl()->getMaxQuantityPlayers(); i++) {
			if(myHand->getPlayerArray()[i]->getMyActiveStatus() && myHand->getPlayerArray()[i]->getMyAction() != 1 && myHand->getPlayerArray()[i]->getMyAction() != 6)	{
// 				cout << "Spieler " << i << " Set " << myHand->getPlayerArray()[i]->getMySet() << endl;
				if(highestSet != myHand->getPlayerArray()[i]->getMySet()) { allHighestSet=0; }
			}
		}
// 		cout << "allHighestSet " << allHighestSet << endl;

// 		cout << "firstflopround " << firstTurnRound << endl;

		// prfen, ob Turn wirklich dran ist
		if(!firstTurnRound && allHighestSet) { 
	
			// Turn nicht dran, weil alle Sets gleich sind
			//also gehe in Turn
			myHand->setActualRound(3);

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
			// Turn ist wirklich dran
	
			// n�hsten Spieler ermitteln
			do { playersTurn = (playersTurn+1)%(myHand->getMainWindowImpl()->getMaxQuantityPlayers());
			} while(!(myHand->getPlayerArray()[playersTurn]->getMyActiveStatus()) || (myHand->getPlayerArray()[playersTurn]->getMyAction())==1 || (myHand->getPlayerArray()[playersTurn]->getMyAction())==6);

			//Spieler-Position vor SmallBlind-Position ermitteln
			int activePlayerBeforeSmallBlind = smallBlindPosition;
			do { activePlayerBeforeSmallBlind = (activePlayerBeforeSmallBlind + myHand->getMainWindowImpl()->getMaxQuantityPlayers() - 1 ) % (myHand->getMainWindowImpl()->getMaxQuantityPlayers());
			} while(!(myHand->getPlayerArray()[activePlayerBeforeSmallBlind]->getMyActiveStatus()) || (myHand->getPlayerArray()[activePlayerBeforeSmallBlind]->getMyAction())==1 || (myHand->getPlayerArray()[activePlayerBeforeSmallBlind]->getMyAction())==6);

			myHand->getPlayerArray()[playersTurn]->setMyTurn(TRUE);
			myHand->getMainWindowImpl()->refreshGroupbox();

// 			cout << "activePlayerBeforeSmallBlind " << activePlayerBeforeSmallBlind << endl;
// 			cout << "playersTurn " << playersTurn << endl;
			// wenn wir letzter aktiver Spieler vor SmallBlind sind, dann flopFirstRound zuende
			if(myHand->getPlayerArray()[playersTurn]->getMyID() == activePlayerBeforeSmallBlind) { firstTurnRound = FALSE; }
		
			if(playersTurn == 0) {
				// Wir sind dran
// 				cout << "actualRound " << myHand->getActualRound() << endl;
// 				cout << "highestSet vor meInAction " << highestSet << endl;
				myHand->getMainWindowImpl()->meInAction();
			}
			else {
				//Gegner sind dran
// 				cout << "NextPlayerSpeed3 start" << endl;
				myHand->getGuiInterface()->turnAnimation2();
			}
		}
	}
}

void LocalTurn::nextPlayer2() {
// 	cout << "NextPlayerSpeed3 stop" << endl;
	myHand->getPlayerArray()[playersTurn]->action();

}


