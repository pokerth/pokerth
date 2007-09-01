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

LocalHand::LocalHand(boost::shared_ptr<EngineFactory> f, GuiInterface *g, BoardInterface *b, std::vector<boost::shared_ptr<PlayerInterface> > p, int id, int sP, int aP, int dP, int sB,int sC)
: myFactory(f), myGui(g),  myBoard(b), playerArray(p), myBeRo(0), myID(id), actualQuantityPlayers(aP), startQuantityPlayers(sP), dealerPosition(dP), actualRound(0), smallBlind(sB), startCash(sC), activePlayersCounter(aP), lastPlayersTurn(0), allInCondition(0),
  cardsShown(false), bettingRoundsPlayed(0)
{

	int i, j, k;

	CardsValue myCardsValue;

	myBoard->setHand(this);

	for(i=0; i<startQuantityPlayers; i++) {
		playerArray[i]->setHand(this);
	// myFlipCards auf 0 setzen
		playerArray[i]->setMyCardsFlip(0, 0);
	}


	// roundStartCashArray fllen
	for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
		playerArray[i]->setMyRoundStartCash(playerArray[i]->getMyCash());
	}

	// Karten generieren und Board sowie Player zuweisen
	int *cardsArray = new int[2*actualQuantityPlayers+5];
	Tools::getRandNumber(0, 51, 2*actualQuantityPlayers+5, cardsArray, 1);
	int tempBoardArray[5];
	int tempPlayerArray[2];
	int tempPlayerAndBoardArray[7];
	int sBluff;
	for(i=0; i<5; i++) {
		tempBoardArray[i] = cardsArray[i];
		tempPlayerAndBoardArray[i+2] = cardsArray[i];
	}

	k = 0;
	myBoard->setMyCards(tempBoardArray);
	for(i=0; i<startQuantityPlayers; i++) {
		if(playerArray[i]->getMyActiveStatus() != 0) {

			int bestHandPos[5];
			playerArray[i]->getMyBestHandPosition(bestHandPos);

			for(j=0; j<2; j++) {
				tempPlayerArray[j] = cardsArray[2*k+j+5];
				tempPlayerAndBoardArray[j] = cardsArray[2*k+j+5];
			}
			playerArray[i]->setMyCards(tempPlayerArray);
			playerArray[i]->setMyCardsValueInt(myCardsValue.cardsValue(tempPlayerAndBoardArray, bestHandPos));
			playerArray[i]->setMyBestHandPosition(bestHandPos);

			// myBestHandPosition auf Fehler ueberpruefen
			for(j=0; j<5; j++) {
				if (bestHandPos[j] == -1) {
					cout << "ERROR getMyBestHandPosition in localhand.cpp" << endl;
				}
			}

			// sBluff für alle aktiver Spieler außer human player setzen
			if(i) {
				Tools::getRandNumber(1,100,1,&sBluff,0);
				playerArray[i]->setSBluff(sBluff);
				playerArray[i]->setSBluffStatus(0);
			}

			k++;
		}
	}
	delete[] cardsArray;

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!   testing !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //

	if(DEBUG_MODE) {

		int temp5Array[5];

		switch(myID) {

			case 1: {
		
				tempBoardArray[0] = 43;
				tempBoardArray[1] = 22;
				tempBoardArray[2] = 35;
				tempBoardArray[3] = 39;
				tempBoardArray[4] = 13;
			
				myBoard->setMyCards(tempBoardArray);
			
				tempPlayerAndBoardArray[2] = tempBoardArray[0];
				tempPlayerAndBoardArray[3] = tempBoardArray[1];
				tempPlayerAndBoardArray[4] = tempBoardArray[2];
				tempPlayerAndBoardArray[5] = tempBoardArray[3];
				tempPlayerAndBoardArray[6] = tempBoardArray[4];
		
				// player0
			
				tempPlayerArray[0] = 1;
				tempPlayerArray[1] = 9;
				tempPlayerAndBoardArray[0] = tempPlayerArray[0];
				tempPlayerAndBoardArray[1] = tempPlayerArray[1];
			
				playerArray[0]->setMyCards(tempPlayerArray);
				playerArray[0]->setMyCardsValueInt(myCardsValue.cardsValue(tempPlayerAndBoardArray,temp5Array));
			
				playerArray[0]->setMyBestHandPosition(temp5Array);

				// player1
		
				tempPlayerArray[0] = 3;
				tempPlayerArray[1] = 4;
				tempPlayerAndBoardArray[0] = tempPlayerArray[0];
				tempPlayerAndBoardArray[1] = tempPlayerArray[1];
			
				playerArray[1]->setMyCards(tempPlayerArray);
				playerArray[1]->setMyCardsValueInt(myCardsValue.cardsValue(tempPlayerAndBoardArray,temp5Array));
			
				playerArray[1]->setMyBestHandPosition(temp5Array);
			
				// player2
		
				tempPlayerArray[0] = 7;
				tempPlayerArray[1] = 17;
				tempPlayerAndBoardArray[0] = tempPlayerArray[0];
				tempPlayerAndBoardArray[1] = tempPlayerArray[1];
			
				playerArray[2]->setMyCards(tempPlayerArray);
				playerArray[2]->setMyCardsValueInt(myCardsValue.cardsValue(tempPlayerAndBoardArray,temp5Array));
			
				playerArray[2]->setMyBestHandPosition(temp5Array);
	
				// player3
			
				tempPlayerArray[0] = 26;
				tempPlayerArray[1] = 28;
				tempPlayerAndBoardArray[0] = tempPlayerArray[0];
				tempPlayerAndBoardArray[1] = tempPlayerArray[1];
			
				playerArray[3]->setMyCards(tempPlayerArray);
				playerArray[3]->setMyCardsValueInt(myCardsValue.cardsValue(tempPlayerAndBoardArray,temp5Array));
			
				playerArray[3]->setMyBestHandPosition(temp5Array);

			} break;
			case 2: {
		
				tempBoardArray[0] = 32;
				tempBoardArray[1] = 26;
				tempBoardArray[2] = 28;
				tempBoardArray[3] = 38;
				tempBoardArray[4] = 7;
			
				myBoard->setMyCards(tempBoardArray);
			
				tempPlayerAndBoardArray[2] = tempBoardArray[0];
				tempPlayerAndBoardArray[3] = tempBoardArray[1];
				tempPlayerAndBoardArray[4] = tempBoardArray[2];
				tempPlayerAndBoardArray[5] = tempBoardArray[3];
				tempPlayerAndBoardArray[6] = tempBoardArray[4];
		
				// player0
			
				tempPlayerArray[0] = 45;
				tempPlayerArray[1] = 35;
				tempPlayerAndBoardArray[0] = tempPlayerArray[0];
				tempPlayerAndBoardArray[1] = tempPlayerArray[1];
			
				playerArray[0]->setMyCards(tempPlayerArray);
				playerArray[0]->setMyCardsValueInt(myCardsValue.cardsValue(tempPlayerAndBoardArray,temp5Array));
			
				playerArray[0]->setMyBestHandPosition(temp5Array);
			
				// player1
		
				tempPlayerArray[0] = 34;
				tempPlayerArray[1] = 39;
				tempPlayerAndBoardArray[0] = tempPlayerArray[0];
				tempPlayerAndBoardArray[1] = tempPlayerArray[1];
			
				playerArray[1]->setMyCards(tempPlayerArray);
				playerArray[1]->setMyCardsValueInt(myCardsValue.cardsValue(tempPlayerAndBoardArray,temp5Array));
			
				playerArray[1]->setMyBestHandPosition(temp5Array);
			
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

	myBeRo = myFactory->createBeRo(this, myID, actualQuantityPlayers, dealerPosition, smallBlind);
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

	//Aktive Spieler zählen
	int activePlayersCounter = 0;
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) { 
		if (playerArray[i]->getMyActiveStatus() == 1) activePlayersCounter++;
	}

	// alle Buttons loeschen
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) { playerArray[i]->setMyButton(0); }

	// DealerButton zuweisen
	playerArray[dealerPosition]->setMyButton(1);

	// assign Small Blind next to dealer. ATTENTION: in heads up it is big blind
	i = dealerPosition;

	for(j=0; (j<MAX_NUMBER_OF_PLAYERS && !(playerArray[i]->getMyActiveStatus())) || j==0; j++) {

		i = (i+1)%(MAX_NUMBER_OF_PLAYERS);
		if(playerArray[i]->getMyActiveStatus())	{
			if(activePlayersCounter > 2) playerArray[i]->setMyButton(2); //small blind normal
			else playerArray[i]->setMyButton(3); //big blind in heads up		
		}
	}

	// assign big blind next to small blind. ATTENTION: in heads up it is small blind
	for(j=0; (j<MAX_NUMBER_OF_PLAYERS && !(playerArray[i]->getMyActiveStatus())) || j==0; j++) {

		i = (i+1)%(MAX_NUMBER_OF_PLAYERS);
		if(playerArray[i]->getMyActiveStatus())	{
			if(activePlayersCounter > 2) playerArray[i]->setMyButton(3); //big blind normal
			else playerArray[i]->setMyButton(2); //small blind in heads up
			
		}
	}

	//do sets
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) { 

		if(playerArray[i]->getMyButton() == 2) { 
		//small blind
			// mit SmallBlind All In ?
			if(playerArray[i]->getMyCash() <= smallBlind) {

				playerArray[i]->setMySet(playerArray[i]->getMyCash());
				// 1 to do not log this
				playerArray[i]->setMyAction(6,1);

			}
			// sonst
			else { playerArray[i]->setMySet(smallBlind); }
		} 

		if(playerArray[i]->getMyButton() == 3) { 
		//big blind
			// mit BigBlind All In ?
			if(playerArray[i]->getMyCash() <= 2*smallBlind) {

				playerArray[i]->setMySet(playerArray[i]->getMyCash());
				// 1 to do not log this
				playerArray[i]->setMyAction(6,1);

			}
			// sonst
			else { playerArray[i]->setMySet(2*smallBlind);	}
		}

	}
}


void LocalHand::switchRounds() {

	
	int i;

// 	cout <<" ------- HandID: " << myID << " | actualround: " << actualRound << " ---------" << endl;

	//Aktive Spieler z�len
	activePlayersCounter = 0;
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) { 
		if (playerArray[i]->getMyAction() != 1 && playerArray[i]->getMyActiveStatus() == 1) activePlayersCounter++;
	}

	// Anzahl der Spieler ermitteln, welche All In sind
	int allInPlayersCounter = 0;
		for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
			if (playerArray[i]->getMyAction() == 6) allInPlayersCounter++;
	}
	 
	//wenn nur noch einer nicht-folded dann gleich den Pot verteilen
	if(activePlayersCounter==1) {
		myBoard->collectPot();	
		myGui->refreshPot();
		myGui->refreshSet();
		actualRound = 4; 
	}
	// prfen der All In Kondition
	// fr All In Prozedur mssen mindestens zwei aktive Player vorhanden sein
	else {

// 		cout << "activplayerscounter: " << activePlayersCounter << " | allInPlayersCounter: " << allInPlayersCounter << " | " << endl;

		// 1) wenn alle All In
		if(allInPlayersCounter == activePlayersCounter) {
			allInCondition = 1;
		}

		// 2) alle bis auf einen All In und der hat HighestSet
		int tempHighestSet;
		if(allInPlayersCounter+1 == activePlayersCounter) {
			for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) { 
				// Spieler ermitteln, der noch nicht All In ist
				if(playerArray[i]->getMyAction() != 1 && playerArray[i]->getMyAction() != 6 && playerArray[i]->getMyActiveStatus() == 1) {	
					tempHighestSet = 0;
					tempHighestSet = myBeRo[actualRound]->getHighestSet();
// 					cout << "tempHighestSet: " << tempHighestSet << "playerArray[i]->getMySet(): " << playerArray[i]->getMySet() << endl;
					
					if(playerArray[i]->getMySet() >= tempHighestSet) {
						allInCondition = 1;
					}

				}

				// HeadsUp-Ausnahme -> spieler ermitteln, der all in ist und als bb nur weniger als sb setzen konnte
				if(activePlayersCounter==2 && playerArray[i]->getMyAction() == 6 && playerArray[i]->getMyActiveStatus() == 1 && playerArray[i]->getMyButton()==3 && playerArray[i]->getMySet()<=smallBlind) {
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

	//unhighlight actual players groupbox
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
