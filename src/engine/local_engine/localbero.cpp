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
#include "localbero.h"

#include "localexception.h"
#include "engine_msg.h"

using namespace std;

LocalBeRo::LocalBeRo(HandInterface* hi, int id, unsigned dP, int sB, GameState gS)
: BeRoInterface(), myHand(hi), myBeRoID(gS), myID(id), dealerPosition(dP), smallBlindPosition(0), dealerPositionId(dP), smallBlindPositionId(0), bigBlindPositionId(0), smallBlind(sB), highestSet(0), minimumRaise(2*sB), firstRun(true), firstRunGui(true), firstRound(true), firstHeadsUpRound(true), currentPlayersTurnId(0), firstRoundLastPlayersTurnId(0), logBoardCardsDone(false)
{
	currentPlayersTurnIt = myHand->getRunningPlayerList()->begin();
	lastPlayersTurnIt = myHand->getRunningPlayerList()->begin();

	PlayerListConstIterator it_c;

	// determine bigBlindPosition
	for(it_c=myHand->getActivePlayerList()->begin(); it_c!=myHand->getActivePlayerList()->end(); it_c++) {
		if((*it_c)->getMyButton() == BUTTON_BIG_BLIND) {
			bigBlindPositionId = (*it_c)->getMyUniqueID();
			break;
		}
	}
	if(it_c == myHand->getActivePlayerList()->end()) {
		throw LocalException(__FILE__, __LINE__, ERR_ACTIVE_PLAYER_NOT_FOUND);
	}

	// determine smallBlindPosition
	for(it_c=myHand->getActivePlayerList()->begin(); it_c!=myHand->getActivePlayerList()->end(); it_c++) {
		if((*it_c)->getMyButton() == BUTTON_SMALL_BLIND) {
			smallBlindPositionId = (*it_c)->getMyUniqueID();
			break;
		}
	}
	if(it_c == myHand->getActivePlayerList()->end()) {
		throw LocalException(__FILE__, __LINE__, ERR_ACTIVE_PLAYER_NOT_FOUND);
	}











// 	int i;

	//SmallBlind-Position ermitteln 
// 	for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
// 		if (myHand->getPlayerArray()[i]->getMyButton() == 2) smallBlindPosition = i;
// 	}

}		


LocalBeRo::~LocalBeRo()
{
}

void LocalBeRo::nextPlayer() {

// 	myHand->getPlayerArray()[playersTurn]->action();

	
// 	cout << "playerID in nextPlayer(): " << (*currentPlayersTurnIt)->getMyID() << endl;

	PlayerListConstIterator currentPlayersTurnConstIt = myHand->getRunningPlayerIt(currentPlayersTurnId);
	if(currentPlayersTurnConstIt == myHand->getRunningPlayerList()->end()) {
		throw LocalException(__FILE__, __LINE__, ERR_RUNNING_PLAYER_NOT_FOUND);
	}

	(*currentPlayersTurnConstIt)->action();

}

void LocalBeRo::run() {

	if(firstRunGui) {
		firstRunGui = false;
		myHand->getGuiInterface()->dealBeRoCards(myBeRoID);
	}
	else {

		if(firstRun) {

			firstRun = false;
	
	/*
	
			// determine smallBlindPosition
			PlayerListIterator smallBlindPositionIt;
		
			for(smallBlindPositionIt=myHand->getActivePlayerList()->begin(); smallBlindPositionIt!=myHand->getActivePlayerList()->end(); smallBlindPositionIt++) {
				if((*smallBlindPositionIt)->getMyButton() == BUTTON_SMALL_BLIND) break;
			}
		
			// determine running player before smallBlind (for heads up: determine dealer/smallblind)
			PlayerListIterator it_1, it_2;
			size_t i;
		
			// running player before smallBlind
			if(myHand->getActivePlayerList()->size() > 2) {
				it_1 = smallBlindPositionIt;
				for(i=0; i<myHand->getActivePlayerList()->size(); i++) {	
					if(it_1 == myHand->getActivePlayerList()->begin()) it_1 = myHand->getActivePlayerList()->end();
					it_1--;
					it_2 = find(myHand->getRunningPlayerList()->begin(), myHand->getRunningPlayerList()->end(), *it_1);
					// running player found
					if(it_2 != myHand->getRunningPlayerList()->end()) {
						lastPlayersTurnIt = it_2;
						break;
					}
				}
			}
			// heads up: bigBlind begins -> dealer/smallBlind is running player before bigBlind
			else {
				it_1 = find(myHand->getRunningPlayerList()->begin(), myHand->getRunningPlayerList()->end(), *smallBlindPositionIt);
				if( it_1 == myHand->getRunningPlayerList()->end() ) {
					cout << "ERROR - lastPlayersTurnIt-detection in localBeRo" << endl;
				}
				// smallBlind found
				else {
					lastPlayersTurnIt = it_1;
				}
			}
		
			currentPlayersTurnIt = lastPlayersTurnIt;
	
	
	
	*/
	
	
	
	
	
	
	
			if(!(myHand->getAllInCondition())) {
		
		
		
				// determine smallBlindPosition
		// 		PlayerListIterator smallBlindPositionIt;
			
		// 		for(smallBlindPositionIt=myHand->getActivePlayerList()->begin(); smallBlindPositionIt!=myHand->getActivePlayerList()->end(); smallBlindPositionIt++) {
		// 			if((*smallBlindPositionIt)->getMyButton() == BUTTON_SMALL_BLIND) break;
		// 		}
			
				// determine running player before smallBlind (for heads up: determine dealer/smallblind)
				PlayerListIterator it_1, it_2;
				size_t i;
			
				// running player before smallBlind
				bool formerRunningPlayerFound = false;
				if(myHand->getActivePlayerList()->size() > 2) {
		
					it_1 = myHand->getActivePlayerIt(smallBlindPositionId);
					if(it_1 == myHand->getActivePlayerList()->end()) {
						throw LocalException(__FILE__, __LINE__, ERR_ACTIVE_PLAYER_NOT_FOUND);
					}
		
					for(i=0; i<myHand->getActivePlayerList()->size(); i++) {	
		
						if(it_1 == myHand->getActivePlayerList()->begin()) it_1 = myHand->getActivePlayerList()->end();
						it_1--;
		
						it_2 = myHand->getRunningPlayerIt((*it_1)->getMyUniqueID());
						// running player found
						if(it_2 != myHand->getRunningPlayerList()->end()) {
							firstRoundLastPlayersTurnId = (*it_2)->getMyUniqueID();
							formerRunningPlayerFound = true;
							break;
						}
					}
					if(!formerRunningPlayerFound) {
						throw LocalException(__FILE__, __LINE__, ERR_FORMER_RUNNING_PLAYER_NOT_FOUND);
					}
				}
				// heads up: bigBlind begins -> dealer/smallBlind is running player before bigBlind
				else {
					firstRoundLastPlayersTurnId = smallBlindPositionId;
				}
			
				currentPlayersTurnId = firstRoundLastPlayersTurnId;
		
		
		
			}
	
	
	
	
	
	
		}











		//log the turned cards
		if(!logBoardCardsDone) {

			int tempBoardCardsArray[5];

			myHand->getBoard()->getMyCards(tempBoardCardsArray);

			switch(myBeRoID) {
				case GAME_STATE_FLOP: myHand->getGuiInterface()->logDealBoardCardsMsg(myBeRoID, tempBoardCardsArray[0], tempBoardCardsArray[1], tempBoardCardsArray[2]);
				break;
				case GAME_STATE_TURN: myHand->getGuiInterface()->logDealBoardCardsMsg(myBeRoID, tempBoardCardsArray[0], tempBoardCardsArray[1], tempBoardCardsArray[2], tempBoardCardsArray[3]);
				break;
				case GAME_STATE_RIVER: myHand->getGuiInterface()->logDealBoardCardsMsg(myBeRoID, tempBoardCardsArray[0], tempBoardCardsArray[1], tempBoardCardsArray[2], tempBoardCardsArray[3], tempBoardCardsArray[4]);
				break;
				default: { cout << "ERROR in localbero.cpp - wrong myBeRoID" << endl;}
			}
			logBoardCardsDone = true;

		}

		bool allHighestSet = true;

		// prfe, ob alle Sets gleich sind ( falls nicht, dann allHighestSet = 0 )
// 		for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
// 			if(myHand->getPlayerArray()[i]->getMyActiveStatus() && myHand->getPlayerArray()[i]->getMyAction() != 1 && myHand->getPlayerArray()[i]->getMyAction() != 6)	{
// 				if(highestSet != myHand->getPlayerArray()[i]->getMySet()) { allHighestSet=0; }
// 			}
// 		}

		PlayerListIterator it;
		PlayerListIterator it_c;


		// test if all running players have same sets (else allHighestSet = false)
		for( it_c = myHand->getRunningPlayerList()->begin(); it_c != myHand->getRunningPlayerList()->end(); it_c++) {
			if(highestSet != (*it_c)->getMySet()) {
				allHighestSet = false;
				break;
			}
		}

		int i;

		// prfen, ob aktuelle bero wirklich dran ist
		if(!firstRound && allHighestSet) { 
	
			// aktuelle bero nicht dran, weil alle Sets gleich sind
			//also gehe in naechste bero
			myHand->setCurrentRound(myBeRoID+1);

			//Action loeschen und ActionButtons refresh
// 			for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
// 				if(myHand->getPlayerArray()[i]->getMyAction() != 1 && myHand->getPlayerArray()[i]->getMyAction() != 6) myHand->getPlayerArray()[i]->setMyAction(0);
// 			}

			//Action loeschen und ActionButtons refresh
			for(it_c=myHand->getRunningPlayerList()->begin(); it_c!=myHand->getRunningPlayerList()->end(); it_c++) {
				(*it_c)->setMyAction(PLAYER_ACTION_NONE);
			}

			//Sets in den Pot verschieben und Sets = 0 und Pot-refresh
			myHand->getBoard()->collectSets();
			myHand->getBoard()->collectPot();
			myHand->getGuiInterface()->refreshPot();
			
			myHand->getGuiInterface()->refreshSet();
			myHand->getGuiInterface()->refreshCash();
			for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) { myHand->getGuiInterface()->refreshAction(i,PLAYER_ACTION_NONE); }
			
			myHand->switchRounds();
		}
		else {
			// aktuelle bero ist wirklich dran

			// Anzahl der effektiv gespielten Runden (des human player) erhöhen
// 			if(myHand->getPlayerArray()[0]->getMyActiveStatus() && myHand->getPlayerArray()[0]->getMyAction() != 1) {
// 				myHand->setBettingRoundsPlayed(myBeRoID);
// 			}

			// Anzahl der effektiv gespielten Runden (des human player) erhöhen
			it_c = myHand->getActivePlayerIt(0);
			if( it_c != myHand->getActivePlayerList()->end() ) {
				// human player is active
				if( (*it_c)->getMyAction() != PLAYER_ACTION_FOLD ) {
					myHand->setBettingRoundsPlayed(myBeRoID);
				}
			}
			
			//// !!!!!!!!!!!!!!!!1!!!! very buggy, rule breaking -> TODO !!!!!!!!!!!!11!!!!!!!! //////////////

			//// headsup:
			//// preflop: dealer is small blind and begins
			//// flop, trun, river: big blind begins

			//// !!!!!!!!! attention: exception if player failed before !!!!!!!!

// 			if( !(myHand->getCurrentQuantityPlayers() < 3 && firstHeadsUpRound == 1) || myHand->getPlayerArray()[playersTurn]->getMyActiveStatus() == 0 ) { 
// 			not first round in heads up (for headsup dealer is smallblind so it is dealers turn)
		
				// naechsten Spieler ermitteln
// 				int i;
// 				for(i=0; (i<MAX_NUMBER_OF_PLAYERS && ((myHand->getPlayerArray()[playersTurn]->getMyActiveStatus()) == 0 || (myHand->getPlayerArray()[playersTurn]->getMyAction())==1 || (myHand->getPlayerArray()[playersTurn]->getMyAction())==6)) || i==0; i++) {
// 					playersTurn = (playersTurn+1)%(MAX_NUMBER_OF_PLAYERS);
// 				}
// 				firstHeadsUpRound = 0; 




				// determine next running player
// 				currentPlayersTurnIt++;
// 				if(currentPlayersTurnIt == myHand->getRunningPlayerList()->end()) currentPlayersTurnIt = myHand->getRunningPlayerList()->begin();


				// determine next running player
				PlayerListConstIterator currentPlayersTurnIt = myHand->getRunningPlayerIt( currentPlayersTurnId );
				if(currentPlayersTurnIt == myHand->getRunningPlayerList()->end()) {
					throw LocalException(__FILE__, __LINE__, ERR_RUNNING_PLAYER_NOT_FOUND);
				}
			
				currentPlayersTurnIt++;
				if(currentPlayersTurnIt == myHand->getRunningPlayerList()->end()) currentPlayersTurnIt = myHand->getRunningPlayerList()->begin();
			
				currentPlayersTurnId = (*currentPlayersTurnIt)->getMyUniqueID();









// 			}
// 			else { firstHeadsUpRound = 0; }

			////// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! ////////////////

			//Spieler-Position vor SmallBlind-Position ermitteln 
// 			int activePlayerBeforeSmallBlind = smallBlindPosition;

// 			for(i=0; (i<MAX_NUMBER_OF_PLAYERS && !(myHand->getPlayerArray()[activePlayerBeforeSmallBlind]->getMyActiveStatus()) || (myHand->getPlayerArray()[activePlayerBeforeSmallBlind]->getMyAction())==1 || (myHand->getPlayerArray()[activePlayerBeforeSmallBlind]->getMyAction())==6) || i==0; i++) {

// 				activePlayerBeforeSmallBlind = (activePlayerBeforeSmallBlind + MAX_NUMBER_OF_PLAYERS - 1 ) % (MAX_NUMBER_OF_PLAYERS);
// 			}

// 			myHand->getPlayerArray()[playersTurn]->setMyTurn(1);


			//highlight active players groupbox and clear action
			myHand->getGuiInterface()->refreshGroupbox(currentPlayersTurnId,2);
			myHand->getGuiInterface()->refreshAction(currentPlayersTurnId,0);

			// wenn wir letzter aktiver Spieler vor SmallBlind sind, dann flopFirstRound zuende
			// ausnahme bei heads up !!! --> TODO
// 			if(myHand->getPlayerArray()[playersTurn]->getMyID() == activePlayerBeforeSmallBlind && myHand->getActivePlayerList()->size() >= 3) { firstRound = 0; }
// 			if(myHand->getActivePlayerList()->size() < 3 && (myHand->getPlayerArray()[playersTurn]->getMyID() == dealerPosition || myHand->getPlayerArray()[playersTurn]->getMyID() == smallBlindPosition)) { firstRound = 0; }

			currentPlayersTurnIt = myHand->getRunningPlayerIt( currentPlayersTurnId );
			if(currentPlayersTurnIt == myHand->getRunningPlayerList()->end()) {
				throw LocalException(__FILE__, __LINE__, ERR_RUNNING_PLAYER_NOT_FOUND);
			}

			(*currentPlayersTurnIt)->setMyTurn(true);


			if( currentPlayersTurnId == firstRoundLastPlayersTurnId ) {
				firstRound = false;
			}






			if( currentPlayersTurnId == 0) {
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


