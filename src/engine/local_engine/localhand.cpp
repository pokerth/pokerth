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
#include "localhand.h"
#include "tools.h"
#include "cardsvalue.h"
#include <game_defs.h>

#include <iostream>

using namespace std;

LocalHand::LocalHand(boost::shared_ptr<EngineFactory> f, GuiInterface *g, BoardInterface *b, std::vector<boost::shared_ptr<PlayerInterface> > sl, PlayerList apl, PlayerList rpl, int id, int sP, unsigned dP, int sB,int sC)
: myFactory(f), myGui(g),  myBoard(b), playerArray(sl), activePlayerList(apl), runningPlayerList(rpl), myBeRo(0), myID(id), startQuantityPlayers(sP), dealerPosition(dP), actualRound(0), smallBlind(sB), startCash(sC), lastPlayersTurn(0), allInCondition(false),
  cardsShown(false), bettingRoundsPlayed(0)
{

	int i, j, k;
	PlayerListConstIterator it_c;

	CardsValue myCardsValue;

	myBoard->setHand(this);

	for(i=0; i<startQuantityPlayers; i++) {
		playerArray[i]->setHand(this);
	// myFlipCards auf 0 setzen
		playerArray[i]->setMyCardsFlip(0, 0);
	}


	// roundStartCashArray fllen
// 	for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
// 		playerArray[i]->setMyRoundStartCash(playerArray[i]->getMyCash());
// 	}
	
	// Karten generieren und Board sowie Player zuweisen
	int *cardsArray = new int[2*activePlayerList->size()+5];
	Tools::getRandNumber(0, 51, 2*activePlayerList->size()+5, cardsArray, 1);
	int tempBoardArray[5];
	int tempPlayerArray[2];
	int tempPlayerAndBoardArray[7];
	int bestHandPos[5];
	int sBluff;
	for(i=0; i<5; i++) {
		tempBoardArray[i] = cardsArray[i];
		tempPlayerAndBoardArray[i+2] = cardsArray[i];
	}

	k = 0;
	myBoard->setMyCards(tempBoardArray);
	for(it_c=activePlayerList->begin(); it_c!=activePlayerList->end(); it_c++, k++) {

		(*it_c)->getMyBestHandPosition(bestHandPos);

		for(j=0; j<2; j++) {
			tempPlayerArray[j] = cardsArray[2*k+j+5];
			tempPlayerAndBoardArray[j] = cardsArray[2*k+j+5];
		}

		(*it_c)->setMyCards(tempPlayerArray);
		(*it_c)->setMyCardsValueInt(myCardsValue.cardsValue(tempPlayerAndBoardArray, bestHandPos));
		(*it_c)->setMyBestHandPosition(bestHandPos);
		(*it_c)->setMyRoundStartCash((*it_c)->getMyCash());

		// myBestHandPosition auf Fehler ueberpruefen
		for(j=0; j<5; j++) {
			if (bestHandPos[j] == -1) {
				cout << "ERROR getMyBestHandPosition in localhand.cpp" << endl;
			}
		}

		// sBluff für alle aktiver Spieler außer human player setzen --> TODO for ai-player in internet
		if((*it_c)->getMyID() != 0) {
			Tools::getRandNumber(1,100,1,&sBluff,0);
			(*it_c)->setSBluff(sBluff);
			(*it_c)->setSBluffStatus(0);
		}
	}

	delete[] cardsArray;

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!   DEBUGGER   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //

	if(DEBUG_MODE) {

// 		int temp5Array[5];

		switch(myID) {

			case 1: {
		
// 				tempBoardArray[0] = 5;
// 				tempBoardArray[1] = 51;
// 				tempBoardArray[2] = 3;
// 				tempBoardArray[3] = 21;
// 				tempBoardArray[4] = 13;
// 			
// 				myBoard->setMyCards(tempBoardArray);
// 			
// 				tempPlayerAndBoardArray[2] = tempBoardArray[0];
// 				tempPlayerAndBoardArray[3] = tempBoardArray[1];
// 				tempPlayerAndBoardArray[4] = tempBoardArray[2];
// 				tempPlayerAndBoardArray[5] = tempBoardArray[3];
// 				tempPlayerAndBoardArray[6] = tempBoardArray[4];
// 		
// 				// player0
// 			
// 				tempPlayerArray[0] = 7;
// 				tempPlayerArray[1] = 37;
// 				tempPlayerAndBoardArray[0] = tempPlayerArray[0];
// 				tempPlayerAndBoardArray[1] = tempPlayerArray[1];
// 			
// 				playerArray[0]->setMyCards(tempPlayerArray);
// 				playerArray[0]->setMyCardsValueInt(myCardsValue.cardsValue(tempPlayerAndBoardArray,temp5Array));
// 			
// 				playerArray[0]->setMyBestHandPosition(temp5Array);
// 
// 				// player1
// 		
// 				tempPlayerArray[0] = 24;
// 				tempPlayerArray[1] = 19;
// 				tempPlayerAndBoardArray[0] = tempPlayerArray[0];
// 				tempPlayerAndBoardArray[1] = tempPlayerArray[1];
// 			
// 				playerArray[1]->setMyCards(tempPlayerArray);
// 				playerArray[1]->setMyCardsValueInt(myCardsValue.cardsValue(tempPlayerAndBoardArray,temp5Array));
// 			
// 				playerArray[1]->setMyBestHandPosition(temp5Array);
			
				// player2
		
/*				tempPlayerArray[0] = 25;
				tempPlayerArray[1] = 36;
				tempPlayerAndBoardArray[0] = tempPlayerArray[0];
				tempPlayerAndBoardArray[1] = tempPlayerArray[1];
			
				playerArray[2]->setMyCards(tempPlayerArray);
				playerArray[2]->setMyCardsValueInt(myCardsValue.cardsValue(tempPlayerAndBoardArray,temp5Array));
			
				playerArray[2]->setMyBestHandPosition(temp5Array);*/
	
				// player3
			
// 				tempPlayerArray[0] = 38;
// 				tempPlayerArray[1] = 22;
// 				tempPlayerAndBoardArray[0] = tempPlayerArray[0];
// 				tempPlayerAndBoardArray[1] = tempPlayerArray[1];
// 			
// 				playerArray[3]->setMyCards(tempPlayerArray);
// 				playerArray[3]->setMyCardsValueInt(myCardsValue.cardsValue(tempPlayerAndBoardArray,temp5Array));
// 			
// 				playerArray[3]->setMyBestHandPosition(temp5Array);

				// player4
			
// 				tempPlayerArray[0] = 25;
// 				tempPlayerArray[1] = 16;
// 				tempPlayerAndBoardArray[0] = tempPlayerArray[0];
// 				tempPlayerAndBoardArray[1] = tempPlayerArray[1];
// 			
// 				playerArray[3]->setMyCards(tempPlayerArray);
// 				playerArray[3]->setMyCardsValueInt(myCardsValue.cardsValue(tempPlayerAndBoardArray,temp5Array));
// 			
// 				playerArray[3]->setMyBestHandPosition(temp5Array);


			} break;
			case 2: {
		
// 				tempBoardArray[0] = 32;
// 				tempBoardArray[1] = 26;
// 				tempBoardArray[2] = 28;
// 				tempBoardArray[3] = 38;
// 				tempBoardArray[4] = 7;
// 			
// 				myBoard->setMyCards(tempBoardArray);
// 			
// 				tempPlayerAndBoardArray[2] = tempBoardArray[0];
// 				tempPlayerAndBoardArray[3] = tempBoardArray[1];
// 				tempPlayerAndBoardArray[4] = tempBoardArray[2];
// 				tempPlayerAndBoardArray[5] = tempBoardArray[3];
// 				tempPlayerAndBoardArray[6] = tempBoardArray[4];
// 		
// 				// player0
// 			
// 				tempPlayerArray[0] = 45;
// 				tempPlayerArray[1] = 35;
// 				tempPlayerAndBoardArray[0] = tempPlayerArray[0];
// 				tempPlayerAndBoardArray[1] = tempPlayerArray[1];
// 			
// 				playerArray[0]->setMyCards(tempPlayerArray);
// 				playerArray[0]->setMyCardsValueInt(myCardsValue.cardsValue(tempPlayerAndBoardArray,temp5Array));
// 			
// 				playerArray[0]->setMyBestHandPosition(temp5Array);
// 			
// 				// player1
// 		
// 				tempPlayerArray[0] = 34;
// 				tempPlayerArray[1] = 39;
// 				tempPlayerAndBoardArray[0] = tempPlayerArray[0];
// 				tempPlayerAndBoardArray[1] = tempPlayerArray[1];
// 			
// 				playerArray[1]->setMyCards(tempPlayerArray);
// 				playerArray[1]->setMyCardsValueInt(myCardsValue.cardsValue(tempPlayerAndBoardArray,temp5Array));
// 			
// 				playerArray[1]->setMyBestHandPosition(temp5Array);
			
// 				// player2
// 		
// 				tempPlayerArray[0] = 7;
// 				tempPlayerArray[1] = 17;
// 				tempPlayerAndBoardArray[0] = tempPlayerArray[0];
// 				tempPlayerAndBoardArray[1] = tempPlayerArray[1];
// 			
// 				playerArray[2]->setMyCards(tempPlayerArray);
// 				playerArray[2]->setMyCardsValueInt(myCardsValue.cardsValue(tempPlayerAndBoardArray,temp5Array));
// 			
// 				playerArray[2]->setMyBestHandPosition(temp5Array);
// 	
// 				// player3
// 			
// 				tempPlayerArray[0] = 26;
// 				tempPlayerArray[1] = 28;
// 				tempPlayerAndBoardArray[0] = tempPlayerArray[0];
// 				tempPlayerAndBoardArray[1] = tempPlayerArray[1];
// 			
// 				playerArray[3]->setMyCards(tempPlayerArray);
// 				playerArray[3]->setMyCardsValueInt(myCardsValue.cardsValue(tempPlayerAndBoardArray,temp5Array));
// 			
// 				playerArray[3]->setMyBestHandPosition(temp5Array);

			} break;
			default: {}

		}



	}

// ----------------------------------------

	// Dealer, SB, BB bestimmen
	assignButtons();

	myBeRo = myFactory->createBeRo(this, myID, dealerPosition, smallBlind);
}



LocalHand::~LocalHand()
{
}

void LocalHand::start() {

	/////////////////////

	// Karten austeilen
	myGui->dealHoleCards();

	getBoard()->collectSets();
	getGuiInterface()->refreshPot();
	
	//Rundenwechsel | beim ersten Durchlauf --> Preflop starten
	myGui->nextPlayerAnimation();
}

void LocalHand::assignButtons() {

	int i,j;
	PlayerListConstIterator it_c;

	// alle Buttons loeschen
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) { playerArray[i]->setMyButton(0); }

	// DealerButton zuweisen
	playerArray[dealerPosition]->setMyButton(1);

	// assign Small Blind next to dealer. ATTENTION: in heads up it is big blind
	i = dealerPosition;

	for(j=0; (j<MAX_NUMBER_OF_PLAYERS && !(playerArray[i]->getMyActiveStatus())) || j==0; j++) {

		i = (i+1)%(MAX_NUMBER_OF_PLAYERS);
		if(playerArray[i]->getMyActiveStatus())	{
			if(activePlayerList->size() > 2) playerArray[i]->setMyButton(2); //small blind normal
			else playerArray[i]->setMyButton(3); //big blind in heads up		
		}
	}

	// assign big blind next to small blind. ATTENTION: in heads up it is small blind
	for(j=0; (j<MAX_NUMBER_OF_PLAYERS && !(playerArray[i]->getMyActiveStatus())) || j==0; j++) {

		i = (i+1)%(MAX_NUMBER_OF_PLAYERS);
		if(playerArray[i]->getMyActiveStatus())	{
			if(activePlayerList->size() > 2) playerArray[i]->setMyButton(3); //big blind normal
			else playerArray[i]->setMyButton(2); //small blind in heads up
			
		}
	}

	//do sets --> TODO switch?
	for (it_c=runningPlayerList->begin(); it_c!=runningPlayerList->end(); it_c++) { 

		//small blind
		if((*it_c)->getMyButton() == 2) { 

			// All in ?
			if((*it_c)->getMyCash() <= smallBlind) {

				(*it_c)->setMySet((*it_c)->getMyCash());
				// 1 to do not log this
				(*it_c)->setMyAction(6,1);
				// delete this player from runningPlayerList
// 				it_c = runningPlayerList->erase(it_c);

			}
			else {
				(*it_c)->setMySet(smallBlind);
			}
		}

		//big blind
		if((*it_c)->getMyButton() == 3) { 
	
			// all in ?
			if((*it_c)->getMyCash() <= 2*smallBlind) {

				(*it_c)->setMySet((*it_c)->getMyCash());
				// 1 to do not log this
				(*it_c)->setMyAction(6,1);
				// delete this player from runningPlayerList
// 				it_c = runningPlayerList->erase(it_c);
	
			}
			else {
				(*it_c)->setMySet(2*smallBlind);
			}
		}
	}
}


void LocalHand::switchRounds() {

// 	cout << "playerID begin switchRounds(): " << getCurrentBeRo()->get

	int i;
	PlayerListIterator it, it_1;
	PlayerListConstIterator it_c;

	// refresh runningPlayerList
	for(it=runningPlayerList->begin(); it!=runningPlayerList->end(); ) {
		if((*it)->getMyAction() == 1 || (*it)->getMyAction() == 6) {
			it = runningPlayerList->erase(it);
			if(!(runningPlayerList->empty())) {

				it_1 = it;
				if(it_1 == runningPlayerList->begin()) it_1 = runningPlayerList->end();
				it_1--;
				getCurrentBeRo()->setCurrentPlayersTurnId((*it_1)->getMyUniqueID());

			}
		} else {
			it++;
		}
	}

	// Anzahl der Spieler ermitteln, welche All In sind
	int allInPlayersCounter = 0;
	for (it_c=activePlayerList->begin(); it_c!=activePlayerList->end(); it_c++) {
		if ((*it_c)->getMyAction() == 6) allInPlayersCounter++;
	}

// 	cout << "allInPlayersCounter: " << allInPlayersCounter << endl;

	// TODO -> runningPlayerList.size()
	int nonFoldPlayerCounter = 0;
	for (it_c=activePlayerList->begin(); it_c!=activePlayerList->end(); it_c++) {
		if ((*it_c)->getMyAction() != 1) nonFoldPlayerCounter++;
	}

// 	cout << "nonFoldPlayerCounter: " << nonFoldPlayerCounter << endl;

	//wenn nur noch einer nicht-folded dann gleich den Pot verteilen
	if(nonFoldPlayerCounter==1) {
		myBoard->collectPot();	
		myGui->refreshPot();
		myGui->refreshSet();
		actualRound = 4; 
	}
	// prfen der All In Kondition
	// fr All In Prozedur mssen mindestens zwei aktive Player vorhanden sein
	else {

		// 1) wenn alle All In
		if(allInPlayersCounter == nonFoldPlayerCounter) {
			allInCondition = true;
		}

		// 2) alle bis auf einen All In und der hat HighestSet
		if(allInPlayersCounter+1 == nonFoldPlayerCounter) {

			// TODO -> runningPlayerList s.u.
			for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {

				if(playerArray[i]->getMyAction() != 1 && playerArray[i]->getMyAction() != 6 && playerArray[i]->getMyActiveStatus() == 1 && playerArray[i]->getMySet() >= myBeRo[actualRound]->getHighestSet()) {
						allInCondition = 1;
				}

			}

// 			if( (*(runningPlayerList.begin()))->getMySet() >= myBeRo[actualRound]->getHighestSet() ) {
// 				allInCondition = 1;
// 			}

			// HeadsUp-Ausnahme -> spieler ermitteln, der all in ist und als bb nur weniger als sb setzen konnte
			for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {

				if(activePlayerList->size()==2 && playerArray[i]->getMyAction() == 6 && playerArray[i]->getMyActiveStatus() == 1 && playerArray[i]->getMyButton()==3 && playerArray[i]->getMySet()<=smallBlind) {
						allInCondition = 1;
				}

			}
		}
	}

	// beim Vorliegen einer All In Kondition -> Ausfhrung einer Sonderprozedur
	if(allInCondition) {
		myBoard->collectPot();	
		myGui->refreshPot();
		myGui->refreshSet();
		myGui->flipHolecardsAllIn();
		
		if (actualRound < 4) // do not increment past 4
			actualRound++;
		
		//log board cards for allin
		if(actualRound >= 1) {
			int tempBoardCardsArray[5];
			
			myBoard->getMyCards(tempBoardCardsArray);
			myGui->logDealBoardCardsMsg(actualRound, tempBoardCardsArray[0], tempBoardCardsArray[1], tempBoardCardsArray[2], tempBoardCardsArray[3], tempBoardCardsArray[4]);
		}
	
	}

	//unhighlight current players groupbox
	if(playerArray[lastPlayersTurn]->getMyActiveStatus() == 1) myGui->refreshGroupbox(lastPlayersTurn,1);

	myGui->refreshGameLabels((GameState)getActualRound());
// 	/*/*/*/*cout <<*/*/*/*/ "NextPlayerSpeed1 stop" << endl;
// 


// 	cout << "NextPlayerSpeed2 start" << endl;
	switch(actualRound) {
		case 0: {
			// Preflop starten
			myGui->preflopAnimation1();
		} break;
		case 1: {
			// Flop starten
			myGui->flopAnimation1();
		} break;
		case 2: {
			// Turn starten
			myGui->turnAnimation1();

		} break;
		case 3: {
			// River starten
			myGui->riverAnimation1();

		} break;
		case 4: {
			// PostRiver starten
			myGui->postRiverAnimation1();

		} break;
		default: {}

	

	}

}

PlayerListIterator LocalHand::getActivePlayerIt(unsigned uniqueId) const {

	PlayerListIterator it;

	for(it=activePlayerList->begin(); it!=activePlayerList->end(); it++) {
		if((*it)->getMyUniqueID() == uniqueId) {
			break;
		}
	}

	return it;

}

PlayerListIterator LocalHand::getRunningPlayerIt(unsigned uniqueId) const {

	PlayerListIterator it;

	for(it=runningPlayerList->begin(); it!=runningPlayerList->end(); it++) {

// 		cout << (*it)->getMyUniqueID() << " ";

		if((*it)->getMyUniqueID() == uniqueId) {
			break;
		}
	}

// 	cout << endl;

	return it;

}
