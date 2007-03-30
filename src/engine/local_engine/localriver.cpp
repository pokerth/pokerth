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
#include "localriver.h"


using namespace std;

LocalRiver::LocalRiver(HandInterface* bR, int id, int qP, int dP, int sB) : RiverInterface(), myHand(bR), myID(id), actualQuantityPlayers(qP), dealerPosition(dP), smallBlindPosition(0), smallBlind(sB), highestSet(0), firstRiverRun(1), firstRiverRound(1), playersTurn(dP), highestCardsValue(0)

{	int i;

	//SmallBlind-Position ermitteln 
	for(i=0; i<myHand->getGuiInterface()->getMaxQuantityPlayers(); i++) {
		if (myHand->getPlayerArray()[i]->getMyButton() == 2) smallBlindPosition = i;
	}

}

LocalRiver::~LocalRiver()
{
}


void LocalRiver::riverRun() {

	int i;

	if (firstRiverRun) {
		myHand->getGuiInterface()->dealRiverCard();
		firstRiverRun = 0;
		
	}

	else {
		bool allHighestSet = 1;

		// prfe, ob alle Sets gleich sind ( falls nicht, dann allHighestSet = 0 )
		for(i=0; i<myHand->getGuiInterface()->getMaxQuantityPlayers(); i++) {
			if(myHand->getPlayerArray()[i]->getMyActiveStatus() && myHand->getPlayerArray()[i]->getMyAction() != 1 && myHand->getPlayerArray()[i]->getMyAction() != 6)	{
// 				cout << "Spieler " << i << " Set " << myHand->getPlayerArray()[i]->getMySet() << endl;
				if(highestSet != myHand->getPlayerArray()[i]->getMySet()) { allHighestSet=0; }
			}
		}
// 		cout << "allHighestSet " << allHighestSet << endl;

// 		cout << "firstflopround " << firstRiverRound << endl;

		// prfen, ob River wirklich dran ist
		if(!firstRiverRound && allHighestSet) { 
	
			// River nicht dran, weil alle Sets gleich sind
			//also gehe in Turn
			myHand->setActualRound(4);

			//Action lï¿œchen und ActionButtons refresh
			for(i=0; i<myHand->getGuiInterface()->getMaxQuantityPlayers(); i++) {
				if(myHand->getPlayerArray()[i]->getMyAction() != 1 && myHand->getPlayerArray()[i]->getMyAction() != 6) myHand->getPlayerArray()[i]->setMyAction(0);
			}
			//Sets in den Pot verschieben und Sets = 0 und Pot-refresh
			myHand->getBoard()->collectSets();
			myHand->getBoard()->collectPot();
			myHand->getGuiInterface()->refreshPot();

			myHand->getGuiInterface()->refreshSet();
			myHand->getGuiInterface()->refreshCash();
			myHand->getGuiInterface()->refreshAction();

			myHand->switchRounds();
	
		}
		else {
			// River ist wirklich dran

			// Anzahl der effektiv gespielten Runden (des human player) erhöhen
			if(myHand->getPlayerArray()[0]->getMyActiveStatus() && myHand->getPlayerArray()[0]->getMyAction() != 1) {
				myHand->setBettingRoundsPlayed(3);
			}
	
			// nï¿œhsten Spieler ermitteln
			do { playersTurn = (playersTurn+1)%(myHand->getGuiInterface()->getMaxQuantityPlayers());
			} while(!(myHand->getPlayerArray()[playersTurn]->getMyActiveStatus()) || (myHand->getPlayerArray()[playersTurn]->getMyAction())==1 || (myHand->getPlayerArray()[playersTurn]->getMyAction())==6);

			//Spieler-Position vor SmallBlind-Position ermitteln 
			int activePlayerBeforeSmallBlind = smallBlindPosition;
			do { activePlayerBeforeSmallBlind = (activePlayerBeforeSmallBlind + myHand->getGuiInterface()->getMaxQuantityPlayers() - 1 ) % (myHand->getGuiInterface()->getMaxQuantityPlayers());
			} while(!(myHand->getPlayerArray()[activePlayerBeforeSmallBlind]->getMyActiveStatus()) || (myHand->getPlayerArray()[activePlayerBeforeSmallBlind]->getMyAction())==1 || (myHand->getPlayerArray()[activePlayerBeforeSmallBlind]->getMyAction())==6);

			myHand->getPlayerArray()[playersTurn]->setMyTurn(1);
			myHand->getGuiInterface()->refreshGroupbox(playersTurn,2);

// 			cout << "activePlayerBeforeSmallBlind " << activePlayerBeforeSmallBlind << endl;
// 			cout << "playersTurn " << playersTurn << endl;
			// wenn wir letzter aktiver Spieler vor SmallBlind sind, dann flopFirstRound zuende
			if(myHand->getPlayerArray()[playersTurn]->getMyID() == activePlayerBeforeSmallBlind) { firstRiverRound = 0; }
		
			if(playersTurn == 0) {
				// Wir sind dran
// 				cout << "actualRound " << myHand->getActualRound() << endl;
// 				cout << "highestSet vor meInAction " << highestSet << endl;
				myHand->getGuiInterface()->meInAction();
			}
			else {
				//Gegner sind dran
				myHand->getGuiInterface()->riverAnimation2();
			}
		}
	}
}

void LocalRiver::postRiverRun() {

	int i;

	// für die Engine die durchschnittlichen Sets von Player 0 setzen --> UNUSED !!!
// 	if(myID < 5) {
// 		myHand->getPlayerArray()[0]->setMyAverageSets(((myHand->getPlayerArray()[0]->getMyAverageSets())*(myID-1))/myID + (myHand->getPlayerArray()[0]->getMyRoundStartCash()-myHand->getPlayerArray()[0]->getMyCash())/myID);
// 	} else {
// 		myHand->getPlayerArray()[0]->setMyAverageSets(((myHand->getPlayerArray()[0]->getMyAverageSets())*4)/5 + (myHand->getPlayerArray()[0]->getMyRoundStartCash()-myHand->getPlayerArray()[0]->getMyCash())/5);
// 	}

// 	cout << myHand->getPlayerArray()[0]->getMyAverageSets() << endl;

	//berechnen welcher Spieler gewonnen hat
	for(i=0; i<myHand->getGuiInterface()->getMaxQuantityPlayers(); i++) {

// 		cout << "Spieler: " << i << " hat: " << myHand->getPlayerArray()[i]->getMyCardsValueInt() << endl;

		if(myHand->getPlayerArray()[i]->getMyActiveStatus() && myHand->getPlayerArray()[i]->getMyAction() != 1 && myHand->getPlayerArray()[i]->getMyCardsValueInt() > highestCardsValue ) { 
			highestCardsValue = myHand->getPlayerArray()[i]->getMyCardsValueInt(); 
		}
	}

	// Durchschnittsets des human player ermitteln
	myHand->getPlayerArray()[0]->setMyAverageSets(((myHand->getPlayerArray()[0]->getMyRoundStartCash())-(myHand->getPlayerArray()[0]->getMyCash()))/(myHand->getBettingRoundsPlayed()+1));
// 	cout << myHand->getPlayerArray()[0]->getMyAverageSets() << endl;

	// Aggressivität des human player ermitteln
	// anzahl der player die möglichkeit haben am pot teilzuhaben
	int potPlayers = 0;
	for(i=0; i<myHand->getGuiInterface()->getMaxQuantityPlayers(); i++) {
		if(myHand->getPlayerArray()[i]->getMyActiveStatus() && myHand->getPlayerArray()[i]->getMyAction() != 1) {
			potPlayers++;
		}
	}

	// prüfen ob nur noch der human player an der verteilung teilnimmt und myAggressive für human player setzen
	myHand->getPlayerArray()[0]->setMyAggressive(potPlayers == 1 && myHand->getPlayerArray()[0]->getMyActiveStatus() && myHand->getPlayerArray()[0]->getMyAction() != 1);

	cout << "myAggressive: " << myHand->getPlayerArray()[0]->getMyAggressive() << endl;

	// Pot-Verteilung
	distributePot();

	//Pot auf 0 setzen
	myHand->getBoard()->setPot(0);
	
	//starte die Animaionsreihe
	myHand->getGuiInterface()->postRiverRunAnimation1();	
}

void LocalRiver::nextPlayer2() {

	myHand->getPlayerArray()[playersTurn]->action();

}

void LocalRiver::distributePot() {

// 	int potCalcAllinArray[5][2];  // 0 -> ID , 1 -> AllInCash

	int i, j;

	////////////////////
	// (1) EINLEITUNG //
	////////////////////
	
	// die Potverteilung gliedert sich in mehrere Runden:
	//	in der ersten Runden sucht sich der Winner der BettingRound seinen zustehenden Anteil am Pot raus. das kann alles oder nur ein Teil sein (z.b. wenn er mit wenig allin gegangen ist und der Rest weitergespielt hat)
	//	in den darauffolgenden Verteilungsrunden suchen sich die jeweils nï¿œhsten Winner aus dem noch brig gebliebenen Pot ihren zustehenden Anteil raus
	//	die gesamte Potverteilung ist zuende, wenn der Pot leer ist und nichts mehr zu holen ist
	
	
	/////////////////////
	// (2) HILFSMITTEL //
	/////////////////////
	
	// winnersArray erstellen -> hier werden die winner der jeweiligen Potverteilungsrunden eingetragen
	int *winnersArray = new int[myHand->getGuiInterface()->getMaxQuantityPlayers()];

	// Anzahl der winner in einer Potverteilungsrunde
	int winnersCounter = 0;
	
	// dieses Array ermittelt wieviel jeder Spieler wï¿œrend der gesamten BettingRound gesetzt hat
	int *roundSetArray = new int[myHand->getGuiInterface()->getMaxQuantityPlayers()];
	for(i=0; i<myHand->getGuiInterface()->getMaxQuantityPlayers(); i++) {
		// Standardwert fr nicht mehr aktive
		roundSetArray[i] = 0;
		// nur fr die Spieler ermitteln, die noch aktiv sind (inkl. der gefoldeten)
		if(myHand->getPlayerArray()[i]->getMyActiveStatus()) { 
			roundSetArray[i] = myHand->getPlayerArray()[i]->getMyRoundStartCash()-myHand->getPlayerArray()[i]->getMyCash();
		}
	}
	
	// hier steht der CardsValue der Spieler drin die an der Potverteilung teilnehmen (d.h. aktiv und nicht gefoldet), bei allen anderen steht ne 0
	int *cardsValueArray = new int[myHand->getGuiInterface()->getMaxQuantityPlayers()];
	int playerWantsPotCounter = 0;
	
	for(i=0; i<myHand->getGuiInterface()->getMaxQuantityPlayers(); i++) {
		// Standardwert
		cardsValueArray[i] = 0;
		if(myHand->getPlayerArray()[i]->getMyActiveStatus() && myHand->getPlayerArray()[i]->getMyAction() != 1) { 
			// nur bei den Spielen den Kartenwert eintragen, die an der Potverteilung teilnehmen
			cardsValueArray[i] = myHand->getPlayerArray()[i]->getMyCardsValueInt();
			playerWantsPotCounter++;
		}
	}
	
	int winner;
	int highestCardsValueTemp;

	// fr SplitPotBerechnung;
	int winnersSets;
	int winnersMaxSet;
	int winnersPot;
	bool playerIsWinner;
	
	
	
	//////////////////////
	// (3) DIE SCHLEIFE //
	//////////////////////
	
	
	// Potverteilungsrunden -> solange noch was aus dem pot zu vergeben ist
	while(playerWantsPotCounter != 0 ) {
	
		// highestCardsValueTemp zu Beginn der aktuellen Potverteilungsrunde auf Null setzen
		highestCardsValueTemp = 0;
	
		// aktuellen highestCardsValueTemp berechnen, von denen die noch an der Potverteilung teilnehmen
		for(i=0; i<myHand->getGuiInterface()->getMaxQuantityPlayers(); i++) {
			if(cardsValueArray[i] > highestCardsValueTemp ) { 
				highestCardsValueTemp = myHand->getPlayerArray()[i]->getMyCardsValueInt(); 
			}
		}
	
		// zu Beginn der Potverteilungsrunde die Anzahl der winner auf 0 setzen
		winnersCounter = 0;
		// Siegerposition und -anzahl von denen ermitteln, die noch an der Potverteilung teilnehmen
		for(i=0; i<myHand->getGuiInterface()->getMaxQuantityPlayers(); i++) {
			if(cardsValueArray[i] == highestCardsValueTemp ) { 
				winnersArray[winnersCounter] = i;
				winnersCounter++;
			}
		}
	
		// --------------------------------------------------------------
		// A) nur einer hat aktulle Potverteilungsrunde gewonnen gewonnen
		// --------------------------------------------------------------
		if(winnersCounter == 1) {
	
			// winnerID ist an der ersten Stelle im winnersArray
			winner = winnersArray[0];
	
			// Winner ist nicht All In gegangen -> bekommt den gesamten Pot
// 			if(myHand->getPlayerArray()[winner]->getMyAction() != 6) {
// 	
// 				myHand->getPlayerArray()[winner]->setMyCash(myHand->getPlayerArray()[winner]->getMyCash()+myHand->getBoard()->getPot());
// 				myHand->getBoard()->setPot(0);
// 				playerWantsPotCounter = 1;
// 	
// 			}
			// Winner ist All In gegangen -> von denen die weniger gesetzt oder gefoldet hatten alles nehmen und von denen die mehr gesetzt hatten nur die eigenen Sets nehmen
// 			else {
	
				// alle Spieler nacheinander durchgehen und prfen von wem man wieviel Geld aus dem Pot bekommt
				for(i=0; i<myHand->getGuiInterface()->getMaxQuantityPlayers(); i++) {
	
					// zuerst prfen ob ein gefoldeter Spieler was gesetzt hatte
					if(i != winner && myHand->getPlayerArray()[i]->getMyAction() == 1) {
	
						// diesen Teil aus dem Pot holen
						myHand->getPlayerArray()[winner]->setMyCash(myHand->getPlayerArray()[winner]->getMyCash()+roundSetArray[i]);
						// Pot dementsprechend verkleinern
						myHand->getBoard()->setPot(myHand->getBoard()->getPot() - roundSetArray[i]);
						// deren Sets auf Null seztzen -> in den nï¿œhsten Potverteilungsrunden nimmt der Winner dann von diesen Spielern 0 Dollar aus dem Pot
						roundSetArray[i] = 0;
	
					}
	
					// nun nur von den Spielern holen, die noch an der Potverteilung teilnehmen und natrlich nicht von einem selber
					if(i != winner && cardsValueArray[i] != 0) {
	
						// zuerst alle rauspicken, die weniger oder das gleiche gesetzt hatten
						if(roundSetArray[winner] >= roundSetArray[i]) { 
	
							// von denen alles holen
							myHand->getPlayerArray()[winner]->setMyCash(myHand->getPlayerArray()[winner]->getMyCash()+roundSetArray[i]);
							// Pot dementsprechend verkleinern
							myHand->getBoard()->setPot(myHand->getBoard()->getPot() - roundSetArray[i]);
							// diese Spieler dann von der weiteren Potverteilung ausschlieï¿œn
							cardsValueArray[i] = 0;
							playerWantsPotCounter--;
							// deren Sets auf Null seztzen, da sie schon alles abgegeben haben
							roundSetArray[i] = 0;
	
						}
						// nun alle Spieler die mehr gesetzt hatten
						else {
	
							// den eigenen Setanteil aus dem Pot holen
							myHand->getPlayerArray()[winner]->setMyCash(myHand->getPlayerArray()[winner]->getMyCash()+roundSetArray[winner]);
							myHand->getBoard()->setPot(myHand->getBoard()->getPot() - roundSetArray[winner]);
							// den Set des Gegners um den bereits abgenommenen Setanteil verkleinern
							roundSetArray[i] = roundSetArray[i] - roundSetArray[winner];
							// diese Spieler nehmen an der weiteren Potverteilung teil
	
						}
					}
				}
// // 			}
	
			// zum Scluss noch eingenen Set wieder holen
			myHand->getPlayerArray()[winner]->setMyCash(myHand->getPlayerArray()[winner]->getMyCash()+roundSetArray[winner]);
			myHand->getBoard()->setPot(myHand->getBoard()->getPot() - roundSetArray[winner]);
			// nicht mehr an der weiteren Potverteilung teilnehmen, da man sich alles geholt hat was einem zusteht
			cardsValueArray[winner] = 0;
			playerWantsPotCounter--;
			// eigene Sets auf Null seztzen, da sie eben eingesammelt wurden
			roundSetArray[winner] = 0;
	
		}
	
	
		// ----------------------------------------------------------------------
		// B) mehrere haben die aktuelle Potverteilungsrunde gewonnen -> SplitPot
		// ----------------------------------------------------------------------
		else {
	
			// hierfr wird ein temporï¿œer WinnersPot erstellt; dort flieï¿œn erstmal alle zustehenden Anteile aus dem Pot rauf; am Ende wird dies auf die Winners verteilt
			winnersPot = 0;
			// hï¿œhster Set aller Winner
			winnersMaxSet = 0;
			
			// den hï¿œhsten Set aller Winner ermitteln
			for(i=0; i<winnersCounter; i++) {
				if(roundSetArray[winnersArray[i]] > winnersMaxSet) winnersMaxSet = roundSetArray[i];
			}
	
			// gesamtSet aller Winner
			winnersSets = 0;
			for(i=0; i<winnersCounter; i++) {
				winnersSets += roundSetArray[winnersArray[i]];
			}
	
			// alle Spieler nacheinander durchgehen und prfen von wem die Winner wieviel Geld aus dem Pot bekommen
			for(i=0; i<myHand->getGuiInterface()->getMaxQuantityPlayers(); i++) {
	
				// prfen ob Spieler i zu den Winnern gehï¿œt
				playerIsWinner = 0;	
				for(j=0; j<winnersCounter; j++) {
					if(winnersArray[j] == i) playerIsWinner = 1; // -> Spieler i gehï¿œt zu den Winnern
				}
	
				// zuerst prfen ob ein gefoldeter Spieler was gesetzt hatte
				if(!playerIsWinner && myHand->getPlayerArray()[i]->getMyAction() == 1) {
	
					// diesen Teil aus dem Pot holen und dem winnersPot geben
					winnersPot += roundSetArray[i];
					// Pot dementsprechend verkleinern
					myHand->getBoard()->setPot(myHand->getBoard()->getPot() - roundSetArray[i]);
					// deren Sets auf Null seztzen -> in den nï¿œhsten Potverteilungsrunden nehmen die Winner dann von diesen Spielern 0 Dollar aus dem Pot
					roundSetArray[i] = 0;
	
				}
	
				// nun nur von den Spielern holen, die noch an der Potverteilung teilnehmen und natrlich nicht zu den Winnern gehï¿œen
				if(!playerIsWinner && cardsValueArray[i] != 0) {
	
					// zuerst alle rauspicken, die weniger oder das gleiche gesetzt hatten
					if(winnersMaxSet >= roundSetArray[i]) { 
	
						// von denen alles holen
						winnersPot += roundSetArray[i];
						// Pot dementsprechend verkleinern
						myHand->getBoard()->setPot(myHand->getBoard()->getPot() - roundSetArray[i]);
						// diese Spieler dann von der weiteren Potverteilung ausschlieï¿œn
						cardsValueArray[i] = 0;
						playerWantsPotCounter--;
						// deren Sets auf Null seztzen, da sie schon alles abgegeben haben
						roundSetArray[i] = 0;
	
					}
					// nun alle Spieler die mehr gesetzt hatten
					else {
	
						// den eigenen Setanteil aus dem Pot holen
						winnersPot += winnersMaxSet;
						myHand->getBoard()->setPot(myHand->getBoard()->getPot() - winnersMaxSet);
						// den Set des Gegners um den bereits abgenommenen Setanteil verkleinern
						roundSetArray[i] = roundSetArray[i] - winnersMaxSet;
						// diese Spieler nehmen an der weiteren Potverteilung teil
	
					}
				}
			}
	
			// zum Schluss noch die eingenen Set aus dem Pot holen
			winnersPot += winnersSets;
			myHand->getBoard()->setPot(myHand->getBoard()->getPot() - winnersSets);
	
			// winnersPot verteilen (anteilmï¿œig)
			for(i=0; i<winnersCounter; i++) {
				myHand->getPlayerArray()[winnersArray[i]]->setMyCash(myHand->getPlayerArray()[winnersArray[i]]->getMyCash()+((roundSetArray[winnersArray[i]]*winnersPot)/winnersSets));
			}
	
	
			for(i=0; i<winnersCounter; i++) {
				// nicht mehr an der weiteren Potverteilung teilnehmen, da man sich alle Winner dieser Verteilungsrunde ihren Anteil geholt haben
				cardsValueArray[winnersArray[i]] = 0;
				playerWantsPotCounter--;
				// Sets auf Null seztzen, da sie eben eingesammelt wurden	
				roundSetArray[winnersArray[i]] = 0;
			}
	
	
		}
	}

	if(myHand->getBoard()->getPot() != 0) cout << "!!! Pot: " << myHand->getBoard()->getPot() << endl;
	
	delete[] winnersArray;
	delete[] roundSetArray;
	delete[] cardsValueArray;


}
