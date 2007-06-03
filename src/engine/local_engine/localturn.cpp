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
#include <handinterface.h>
#include <game_defs.h>


//using namespace std;

LocalTurn::LocalTurn(HandInterface* bR, int id, int qP, int dP, int sB) : TurnInterface(), myHand(bR), myID(id), actualQuantityPlayers(qP), dealerPosition(dP), smallBlindPosition(0), smallBlind(sB), highestSet(0), firstTurnRun(1), firstTurnRound(1), firstHeadsUpTurnRound(1), playersTurn(dP), logBoardCardsDone(0)

{	int i;

	//SmallBlind-Position ermitteln 
	for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
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
		firstTurnRun = 0;
		
	}

	else {
		//log the turned cards
		if(!logBoardCardsDone) {
			int tempBoardCardsArray[5];
			
			myHand->getBoard()->getMyCards(tempBoardCardsArray);
			myHand->getGuiInterface()->logDealBoardCardsMsg(2, tempBoardCardsArray[0], tempBoardCardsArray[1], tempBoardCardsArray[2], tempBoardCardsArray[3]);
			logBoardCardsDone = 1;
		}
		
		bool allHighestSet = 1;

		// prfe, ob alle Sets gleich sind ( falls nicht, dann allHighestSet = 0 )
		for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
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
			// Turn ist wirklich dran

			// Anzahl der effektiv gespielten Runden (des human player) erhöhen
			if(myHand->getPlayerArray()[0]->getMyActiveStatus() && myHand->getPlayerArray()[0]->getMyAction() != 1) {
				myHand->setBettingRoundsPlayed(2);
			}
	
			if( !(myHand->getActualQuantityPlayers() < 3 && firstHeadsUpTurnRound == 1) ) { 
// 			not first round in heads up (for headsup dealer is smallblind so it is dealers turn)
		
				// naechsten Spieler ermitteln
				do { playersTurn = (playersTurn+1)%(MAX_NUMBER_OF_PLAYERS);
				} while(!(myHand->getPlayerArray()[playersTurn]->getMyActiveStatus()) || (myHand->getPlayerArray()[playersTurn]->getMyAction())==1 || (myHand->getPlayerArray()[playersTurn]->getMyAction())==6);
			}
			else { firstHeadsUpTurnRound = 0; }

			//Spieler-Position vor SmallBlind-Position ermitteln
			int activePlayerBeforeSmallBlind = smallBlindPosition;
			do { activePlayerBeforeSmallBlind = (activePlayerBeforeSmallBlind + MAX_NUMBER_OF_PLAYERS - 1 ) % (MAX_NUMBER_OF_PLAYERS);
			} while(!(myHand->getPlayerArray()[activePlayerBeforeSmallBlind]->getMyActiveStatus()) || (myHand->getPlayerArray()[activePlayerBeforeSmallBlind]->getMyAction())==1 || (myHand->getPlayerArray()[activePlayerBeforeSmallBlind]->getMyAction())==6);

			myHand->getPlayerArray()[playersTurn]->setMyTurn(1);
			//highlight active players groupbox and clear action
			myHand->getGuiInterface()->refreshGroupbox(playersTurn,2);
			myHand->getGuiInterface()->refreshAction(playersTurn,0);

//			cout << "activePlayerBeforeSmallBlind " << activePlayerBeforeSmallBlind << endl;
//			cout << "playersTurn " << playersTurn << endl;
			// wenn wir letzter aktiver Spieler vor SmallBlind sind, dann flopFirstRound zuende
			if(myHand->getPlayerArray()[playersTurn]->getMyID() == activePlayerBeforeSmallBlind) { firstTurnRound = 0; }

			if(playersTurn == 0) {
				// Wir sind dran
//				cout << "actualRound " << myHand->getActualRound() << endl;
//				cout << "highestSet vor meInAction " << highestSet << endl;
				myHand->getGuiInterface()->meInAction();
			}
			else {
				//Gegner sind dran
//				cout << "NextPlayerSpeed3 start" << endl;
				myHand->getGuiInterface()->turnAnimation2();
			}
		}
	}
}

void LocalTurn::nextPlayer2() {
// 	cout << "NextPlayerSpeed3 stop" << endl;
	myHand->getPlayerArray()[playersTurn]->action();

}


