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

using namespace std;

LocalHand::LocalHand(EngineFactory *f, GuiInterface *g, BoardInterface *b, PlayerInterface **p, int id, int qP, int dP, int sB,int sC) : HandInterface(), myFactory(f), myGui(g),  myBoard(b), playerArray(p), myPreflop(0), myFlop(0), myTurn(0), myRiver(0), myID(id), actualQuantityPlayers(qP), dealerPosition(dP), actualRound(0), smallBlind(sB), startCash(sC), allInCondition(0)
{

	int i, j;
	CardsValue myCardsValue;

	myGui->setHand(this);
	myBoard->setHand(this);


	for(i=0; i<actualQuantityPlayers; i++) playerArray[i]->setHand(this);


	// roundStartCashArray fllen
	for(i=0; i<5; i++) {
		playerArray[i]->setMyRoundStartCash(playerArray[i]->getMyCash());
	}
		

	// Dealer, SB, BB bestimmen
	assignButtons();

	myGui->refreshAll();

	// Karten generieren und Board sowie Player zuweisen
	Tools myTool;
	int *cardsArray = new int[2*actualQuantityPlayers+5];
	myTool.getRandNumber(0, 51, 2*actualQuantityPlayers+5, cardsArray, 1);
	int tempBoardArray[5];
	int tempPlayerArray[2];
	int tempPlayerAndBoardArray[7];
	for(i=0; i<5; i++) {
		tempBoardArray[i] = cardsArray[i];
		tempPlayerAndBoardArray[i+2] = cardsArray[i];
	}
	myBoard->setMyCards(tempBoardArray);
	for(i=0; i<actualQuantityPlayers; i++) {
		for(j=0; j<2; j++) {
			tempPlayerArray[j] = cardsArray[2*i+j+5];
			tempPlayerAndBoardArray[j] = cardsArray[2*i+j+5];
		}
		playerArray[i]->setMyCards(tempPlayerArray);
		playerArray[i]->setMyCardsValueInt(myCardsValue.cardsValue(tempPlayerAndBoardArray,playerArray[i]->getMyBestHandPosition()));
		// myBestHandPosition auf Fehler ueberpruefen
		for(j=0; j<5; j++) {
			if((playerArray[i]->getMyBestHandPosition())[i] == -1) {
				cout << "FEHLER bei myBestHandPosition-Ermittlung!!!" << endl;
			}
		}
	}

	// Karten austeilen
	myGui->dealHoleCards();

	

	// Preflop, Flop, Turn und River erstellen
	myPreflop =  myFactory->createPreflop(this, myID, actualQuantityPlayers, dealerPosition, smallBlind);
	myFlop = myFactory->createFlop(this, myID, actualQuantityPlayers, dealerPosition, smallBlind);
	myTurn = myFactory->createTurn(this, myID, actualQuantityPlayers, dealerPosition, smallBlind);
	myRiver = myFactory->createRiver(this, myID, actualQuantityPlayers, dealerPosition, smallBlind);
	
	//Rundenwechsel | beim ersten Durchlauf --> Preflop starten
	myGui->nextPlayerAnimation();

	delete[] cardsArray;
}



LocalHand::~LocalHand()
{

	delete myPreflop;
	myPreflop = 0;
	delete myFlop;
	myFlop = 0;
	delete myTurn;
	myTurn = 0;
	delete myRiver;
	myRiver = 0;

}

void LocalHand::assignButtons() {

	int i;

	// alle Buttons loeschen
	for (i=0; i<myGui->getMaxQuantityPlayers(); i++) { playerArray[i]->setMyButton(0); }

	// DealerButton zuweisen
	playerArray[dealerPosition]->setMyButton(1);

	// Small Blind zuweisen und setzen
	i = dealerPosition;
	do {
		i = (i+1)%(myGui->getMaxQuantityPlayers());
		if(playerArray[i]->getMyActiveStatus())	{
			playerArray[i]->setMyButton(2);
			// mit SmallBlind All In ?
			if(playerArray[i]->getMyCash() <= smallBlind) {

				playerArray[i]->setMySet(playerArray[i]->getMyCash());
				playerArray[i]->setMyAction(6);

			}
			// sonst
			else { 
				playerArray[i]->setMySet(smallBlind);
			}
		}

	} while(!(playerArray[i]->getMyActiveStatus()));

	// Big Blind zuweisen
	do {
		i = (i+1)%(myGui->getMaxQuantityPlayers());
		if(playerArray[i]->getMyActiveStatus())	{
			playerArray[i]->setMyButton(3);
			// mit BigBlind All In ?
			if(playerArray[i]->getMyCash() <= 2*smallBlind) {

				playerArray[i]->setMySet(playerArray[i]->getMyCash());
				playerArray[i]->setMyAction(6);

			}
			// sonst
			else { 
				playerArray[i]->setMySet(2*smallBlind);
			}
		}

	} while(!(playerArray[i]->getMyActiveStatus()));
}


void LocalHand::switchRounds() {

	int i;

// 	cout << "switchrounds" << endl;

	//Aktive Spieler z�len --> wenn nur noch einer nicht-folded dann gleich den Pot verteilen
	activePlayersCounter = 0;
	for (i=0; i<myGui->getMaxQuantityPlayers(); i++) { 
		if (playerArray[i]->getMyAction() != 1 && playerArray[i]->getMyActiveStatus() == 1) activePlayersCounter++;
	}

	// Anzahl der Spieler ermitteln, welche All In sind
	int allInPlayersCounter = 0;
		for (i=0; i<myGui->getMaxQuantityPlayers(); i++) { 
			if (playerArray[i]->getMyAction() == 6) allInPlayersCounter++;
	}

	if(activePlayersCounter==1) {
		myBoard->collectPot();	
		myGui->refreshPot();
		myGui->refreshSet();
		actualRound = 4; 
	}

	// prfen der All In Kondition
	// fr All In Prozedur mssen mindestens zwei aktive Player vorhanden sein
	else {
		// 1) wenn alle All In
		if(allInPlayersCounter == activePlayersCounter) {
			allInCondition = 1;
		}

		// 2) alle bis auf einen All In und der hat HighestSet
		int tempHighestSet;
		if(allInPlayersCounter+1 == activePlayersCounter) {
			// Spieler ermitteln, der noch nicht All In ist
			for (i=0; i<myGui->getMaxQuantityPlayers(); i++) { 
				if(playerArray[i]->getMyAction() != 1 && playerArray[i]->getMyAction() != 6 && playerArray[i]->getMyActiveStatus() == 1) {	
					tempHighestSet = 0;
					switch (actualRound) {
						case 0: {tempHighestSet = myPreflop->getHighestSet();}
						break;
						case 1: {tempHighestSet = myFlop->getHighestSet();}
						break;
						case 2: {tempHighestSet = myTurn->getHighestSet();}
						break;
						case 3: {tempHighestSet = myRiver->getHighestSet();}
						break;
						default: {}	
					}
					if(playerArray[i]->getMySet() >= tempHighestSet) {
						allInCondition = 1;
					}
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
		actualRound++;
	}


	myGui->refreshGroupbox();
	highlightRoundLabel();
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

void LocalHand::highlightRoundLabel() {
	switch(actualRound) {
		case 0: {
			myGui->highlightRoundLabel("Preflop");
		} break;
		case 1: {
			myGui->highlightRoundLabel("Flop");
		} break;
		case 2: {
			myGui->highlightRoundLabel("Turn");
		} break;
		case 3: {
			myGui->highlightRoundLabel("River");
		} break;
		case 4: {
			myGui->highlightRoundLabel("");
		} break;
		default: {
			myGui->highlightRoundLabel("!!! FEHLER !!!");
		}
	}
}

