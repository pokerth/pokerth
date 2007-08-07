//
// C++ Implementation: localbero
//
// Description: 
//
//
// Author: FThauer FHammer <webmaster@pokerth.net>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "localbero.h"

using namespace std;

LocalBeRo::LocalBeRo(HandInterface* hi, int id, int qP, int dP, int sB, GameState gS)
: BeRoInterface(), myHand(hi), myBeRoID(gS), myID(id), actualQuantityPlayers(qP), dealerPosition(dP), smallBlindPosition(0), smallBlind(sB), highestSet(0), firstRun(1), firstRound(1), firstHeadsUpRound(1), playersTurn(dP), logBoardCardsDone(0)
{

	int i;

	//SmallBlind-Position ermitteln 
	for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
		if (myHand->getPlayerArray()[i]->getMyButton() == 2) smallBlindPosition = i;
	}

}		


LocalBeRo::~LocalBeRo()
{
}

void LocalBeRo::nextPlayer() {

	myHand->getPlayerArray()[playersTurn]->action();

}

void LocalBeRo::run() {

	cout << "LocalBeRoRun-" << myBeRoID << endl;

	int i;

	if (firstRun) {

		myHand->getGuiInterface()->dealBeRoCards(myBeRoID);
		firstRun = 0;

	}

	else {
		//log the turned cards
		if(!logBoardCardsDone) {

			int tempBoardCardsArray[5];

			myHand->getBoard()->getMyCards(tempBoardCardsArray);
			myHand->getGuiInterface()->logDealBoardCardsMsg(myBeRoID, tempBoardCardsArray[0], tempBoardCardsArray[1], tempBoardCardsArray[2]);
			logBoardCardsDone = 1;

		}

		bool allHighestSet = 1;

		// prfe, ob alle Sets gleich sind ( falls nicht, dann allHighestSet = 0 )
		for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
			if(myHand->getPlayerArray()[i]->getMyActiveStatus() && myHand->getPlayerArray()[i]->getMyAction() != 1 && myHand->getPlayerArray()[i]->getMyAction() != 6)	{
				if(highestSet != myHand->getPlayerArray()[i]->getMySet()) { allHighestSet=0; }
			}
		}

		// prfen, ob Flop wirklich dran ist
		if(!firstRound && allHighestSet) { 
	
			// Flop nicht dran, weil alle Sets gleich sind
			//also gehe in Turn
			myHand->setActualRound(myBeRoID+1);

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
			// Flop ist wirklich dran

			// Anzahl der effektiv gespielten Runden (des human player) erhöhen
			if(myHand->getPlayerArray()[0]->getMyActiveStatus() && myHand->getPlayerArray()[0]->getMyAction() != 1) {
				myHand->setBettingRoundsPlayed(myBeRoID);
			}
			
			if( !(myHand->getActualQuantityPlayers() < 3 && firstHeadsUpRound == 1) ) { 
// 			not first round in heads up (for headsup dealer is smallblind so it is dealers turn)
		
				// naechsten Spieler ermitteln
				do { playersTurn = (playersTurn+1)%(MAX_NUMBER_OF_PLAYERS);
				} while(!(myHand->getPlayerArray()[playersTurn]->getMyActiveStatus()) || (myHand->getPlayerArray()[playersTurn]->getMyAction())==1 || (myHand->getPlayerArray()[playersTurn]->getMyAction())==6);
			}
			else { firstHeadsUpRound = 0; }

			//Spieler-Position vor SmallBlind-Position ermitteln 
			int activePlayerBeforeSmallBlind = smallBlindPosition;
			do { activePlayerBeforeSmallBlind = (activePlayerBeforeSmallBlind + MAX_NUMBER_OF_PLAYERS - 1 ) % (MAX_NUMBER_OF_PLAYERS);
			} while(!(myHand->getPlayerArray()[activePlayerBeforeSmallBlind]->getMyActiveStatus()) || (myHand->getPlayerArray()[activePlayerBeforeSmallBlind]->getMyAction())==1 || (myHand->getPlayerArray()[activePlayerBeforeSmallBlind]->getMyAction())==6);

			myHand->getPlayerArray()[playersTurn]->setMyTurn(1);
			//highlight active players groupbox and clear action
			myHand->getGuiInterface()->refreshGroupbox(playersTurn,2);
			myHand->getGuiInterface()->refreshAction(playersTurn,0);

			// wenn wir letzter aktiver Spieler vor SmallBlind sind, dann flopFirstRound zuende
			if(myHand->getPlayerArray()[playersTurn]->getMyID() == activePlayerBeforeSmallBlind) { firstRound = 0; }

			if(playersTurn == 0) {
				// Wir sind dran
				myHand->getGuiInterface()->meInAction();
			}
			else {
				//Gegner sind dran
				myHand->getGuiInterface()->beRoAnimation2(myBeRoID);
			}
		}
	}
}


