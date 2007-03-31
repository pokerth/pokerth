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
#include "localplayer.h"

#include "localhand.h"

using namespace std;

LocalPlayer::LocalPlayer(BoardInterface *b, int id, std::string name, std::string avatar, int sC, bool aS, int mB) : PlayerInterface(), actualHand(0), actualBoard(b), myCardsValue(0), myID(id), myName(name), myAvatar(avatar), myDude(0), myDude4(0), myCardsValueInt(0), myOdds(-1.0), myCash(sC), mySet(0), myAction(0), myButton(mB), myActiveStatus(aS), myTurn(0), myRoundStartCash(0), sBluff(0), sBluffStatus(0)
{
	// myBestHandPosition mit -1 initialisieren
	int i;
	for(i=0; i<5; i++) {
		myBestHandPosition[i] = -1;
	}

	// myAverageSets initialisieren
	for(i=0; i<4; i++) {
		myAverageSets[i] = 0;
	}

	// myAggressive initialisieren
	for(i=0; i<7; i++) {
		myAggressive[i] = 0;
	}

	Tools myTool;
	// Dude zuweisen
	myTool.getRandNumber(3 , 5, 1, &myDude, 0);
// 	cout << "Spieler: " << myID << " Dude: " << myDude << " Cash: " << myCash << " ActiveStatus: " << myActiveStatus << " Button: " << myButton << endl;

	// Dude4 zuweisen
	int interval = 7;
	int count = 4;

	int tempArray[4];
	myTool.getRandNumber(0, 2*interval, count, tempArray, 0);
	for(i=0; i<count; i++) {
		myDude4 += tempArray[i];
	}
	myDude4 = (myDude4/count)-interval;


	myCardsValue = new CardsValue;


}


LocalPlayer::~LocalPlayer()
{
}


void LocalPlayer::setHand(HandInterface* br) { actualHand = br; }


void LocalPlayer::action() {

	ConfigFile myConfig;

	switch(actualHand->getActualRound()) {
		case 0: {

			if(myConfig.readConfigInt("EngineVersion")) preflopEngine3();
			else preflopEngine();

			actualBoard->collectSets();
			actualHand->getGuiInterface()->refreshPot();

		} break;
		case 1: {

			if(myConfig.readConfigInt("EngineVersion")) flopEngine3();
			else flopEngine();

			actualBoard->collectSets();
			actualHand->getGuiInterface()->refreshPot();

		} break;
		case 2: {

			if(myConfig.readConfigInt("EngineVersion")) turnEngine3();
			else turnEngine();

			actualBoard->collectSets();
			actualHand->getGuiInterface()->refreshPot();

		} break;
		case 3: {

			if(myConfig.readConfigInt("EngineVersion")) riverEngine3();
			else riverEngine();

			actualBoard->collectSets();
			actualHand->getGuiInterface()->refreshPot();

		} break;
		default: {}
	}

	myTurn = 0;
// 	cout << "jetzt" << endl;
	
	//set that i was the last active player. need this for unhighlighting groupbox
	actualHand->setLastPlayersTurn(myID);

	actualHand->getGuiInterface()->logPlayerActionMsg(myName, myAction, mySet);
	actualHand->getGuiInterface()->nextPlayerAnimation();


}


void LocalPlayer::preflopEngine() {
	
	int bet = 0;
	int raise = 0;
	int cBluff;
	Tools myTool;

	// temporär solange preflopValue und flopValue noch nicht bereinigt für sechs und sieben spieler
	int players = actualHand->getActualQuantityPlayers();
	if(players > 5) players = 5;

	// myOdds auslesen
	readFile();

	// Niveaus setzen + Dude + Anzahl Gegenspieler
	// 1. Fold -- Call
	myNiveau[0] = 43 + myDude4 - 6*(players - 2);
	// 3. Call -- Raise
	myNiveau[2] = 50 + myDude4 - 6*(players - 2);

	// eigenes mögliches highestSet
	int individualHighestSet = actualHand->getPreflop()->getHighestSet();
	if(individualHighestSet > myCash) individualHighestSet = myCash;

	// Verhaeltnis Set / Cash für call	
	if(myCash/individualHighestSet >= 25) {
		myNiveau[0] += (25-myCash/individualHighestSet)/10;
	} else {
		myNiveau[0] += (25-myCash/individualHighestSet)/3;
	}

	// Verhaeltnis Set / Cash für raise
	if(myCash/individualHighestSet < 10) {
		myNiveau[2] += (10-myCash/individualHighestSet)/2;
	}

//	cout << myID << ": " << myHoleCardsValue << " - " << myNiveau[0] << " " << myNiveau[2] << " - " << myCards[0] << " " << myCards[1] << endl;

	// Aggresivität des humanPlayers auslesen
	int aggValue = (int)(((actualHand->getPlayerArray()[0]->getMyAggressive()*1.0)/7.0 - 1.0/actualHand->getActualQuantityPlayers())*21.0);

// 	cout << aggValue << "  ";

	myNiveau[0] -= aggValue;
	myNiveau[2] -= aggValue;

	
//	cout << "Spieler " << myID << ": Dude " << myDude4 << "\t Wert " <<  myHoleCardsValue << "\t Niveau " << myNiveau[0] << " " << myNiveau[1] << " " << myNiveau[2] << "\t Agg " << aggValue << " " << endl;

	// Check-Bluff generieren
	myTool.getRandNumber(1,100,1,&cBluff,0);

	// aktive oder passivie Situation ? -> im preflop nur passiv

	// raise (bei hohem Niveau)
	if(myOdds >= myNiveau[2]) {

		// raise-loop unterbinden -> d.h. entweder call oder bei superblatt all in
		if(actualHand->getPreflop()->getHighestSet() >= 12*actualHand->getSmallBlind()) {
			// all in
			if(myOdds >= myNiveau[2] + 8) {
				raise = myCash;
				myAction = 5;
			}
			// nur call
			else {
				myAction = 3;
			}
	
		// Standard-Raise-Routine
		} else {
			// raise-Betrag ermitteln
			raise = (((int)myOdds-myNiveau[2])/2)*2*actualHand->getSmallBlind();
			// raise-Betrag zu klein -> mindestens Standard-raise
			if(raise < actualHand->getPreflop()->getHighestSet()) {
				raise = actualHand->getPreflop()->getHighestSet();
			}
			// all in bei nur wenigen Chips oder knappem raise
			if(myCash/(2*actualHand->getSmallBlind()) <= 6 || raise >= (myCash*4)/5) {
				raise = myCash;
			}
			myAction = 5;
		}

		// auf cBluff testen --> call statt raise
		if(cBluff > 80) myAction = 3;
		if(cBluff > 70 && myOdds >= myNiveau[2] + 4) myAction = 3;
		if(cBluff > 60 && myOdds >= myNiveau[2] + 8) myAction = 3;
		if(cBluff > 50 && myOdds >= myNiveau[2] + 12) myAction = 3;

	}
	else {
		// call
		if(myOdds >= myNiveau[0] || (mySet >= actualHand->getPreflop()->getHighestSet()/2 && myOdds >= myNiveau[0]-5)) {
			// bigBlind --> check
			if(myButton == 3 && mySet == actualHand->getPreflop()->getHighestSet()) myAction = 2;
			else myAction = 3;
		}
		// fold
		else {
			// bigBlind -> check
			if(myButton == 3 && mySet == actualHand->getPreflop()->getHighestSet()) myAction = 2;
			else myAction = 1;
		}
	}

// 	cout << sBluff << endl;

	// auf sBluff testen --> raise statt call oder fold
	if(sBluff < 100/(((actualHand->getActualQuantityPlayers()-2)*6)+3) && myOdds < myNiveau[2] && actualHand->getPreflop()->getHighestSet() == 2*actualHand->getSmallBlind()) {

// 		cout << "sBLUFF!" << endl;
		sBluffStatus = 1;

		// Gegner raisen ebenfalls -> call
		if(actualHand->getPreflop()->getHighestSet() >= 12*actualHand->getSmallBlind()) {
			myAction = 3;
		}
		// Standard-Raise-Routine
		else {
			// raise-Betrag ermitteln
			raise = (sBluff/(8-actualHand->getActualQuantityPlayers()))*actualHand->getSmallBlind();
			// raise-Betrag zu klein -> mindestens Standard-raise
			if(raise < actualHand->getPreflop()->getHighestSet()) {
				raise = actualHand->getPreflop()->getHighestSet();
			}
			// all in bei nur wenigen Chips oder knappem raise
			if(myCash/(2*actualHand->getSmallBlind()) <= 6 || raise >= (myCash*4)/5) {
				raise = myCash;
			}
			myAction = 5;
		}


	}

// 	cout << myID << ": " << myOdds << " - " << myNiveau[0] << " " << myNiveau[2] << " - " << "Bluff: " << sBluffStatus << endl;

	evaluation(bet, raise);
}


void LocalPlayer::flopEngine() {

	int raise = 0;
	int bet = 0;
	int i;
	int cBluff;
	int pBluff;
	Tools myTool;
	int rand;

	// übergang solange preflopValue und flopValue noch nicht bereinigt
	int players = actualHand->getActualQuantityPlayers();
	if(players > 5) players = 5;

	readFile();

	// Niveaus setzen + Dude + Anzahl Gegenspieler
	// 1. Fold -- Call
	myNiveau[0] = 49 + myDude4 - 6*(players - 2);
	// 2. Check -- Bet
	myNiveau[1] = 54 + myDude4 - 6*(players - 2);
	// 3. Call -- Raise
	myNiveau[2] = 67 + myDude4 - 6*(players - 2);

	// eigenes mögliches highestSet
	int individualHighestSet = actualHand->getFlop()->getHighestSet();
	if(individualHighestSet > myCash) individualHighestSet = myCash;

	// Aggresivität des humanPlayers auslesen
	int aggValue = (int)(((actualHand->getPlayerArray()[0]->getMyAggressive()*1.0)/7.0 - 1.0/actualHand->getActualQuantityPlayers())*21.0);

	for(i=0; i<3; i++) {
		myNiveau[i] -= aggValue;
	}

	// Check-Bluff generieren
	myTool.getRandNumber(1,100,1,&cBluff,0);

	// aktiv oder passiv?
	if(actualHand->getFlop()->getHighestSet() > 0) {

		// Verhaeltnis Set / Cash für call
		if(myCash/individualHighestSet >= 25) {
			myNiveau[0] += (25-myCash/individualHighestSet)/20;
		} else {
			myNiveau[0] += (25-myCash/individualHighestSet)/3;
		}

		// Verhaeltnis Set / Cash für raise
		if(myCash/individualHighestSet < 10) {
			myNiveau[2] += (10-myCash/individualHighestSet)/2;
		}

		// raise (bei hohem Niveau)
		if(myOdds >= myNiveau[2]) {
	
			// raise-loop unterbinden -> d.h. entweder call oder bei superblatt all in
			if(actualHand->getFlop()->getHighestSet() >= 12*actualHand->getSmallBlind()) {
				// all in
				if(myOdds >= myNiveau[2] + 8) {
					raise = myCash;
					myAction = 5;
				}
				// nur call
				else {
					myAction = 3;
				}
	
			// Standard-Raise-Routine
			} else {
				// raise-Betrag ermitteln
				raise = (((int)myOdds-myNiveau[2])/5)*2*actualHand->getSmallBlind();
				// raise-Betrag zu klein -> mindestens Standard-raise
				if(raise < actualHand->getFlop()->getHighestSet()) {
					raise = actualHand->getFlop()->getHighestSet();
				}
				// all in bei nur wenigen Chips oder knappem raise
				if(myCash/(2*actualHand->getSmallBlind()) <= 6 || raise >= (myCash*4.0)/5.0) {
					raise = myCash;
				}
				myAction = 5;
			}

			// auf cBluff testen --> call statt raise
			if(cBluff > 90) myAction = 3;
			if(cBluff > 80 && myOdds >= myNiveau[2] + 4) myAction = 3;
			if(cBluff > 70 && myOdds >= myNiveau[2] + 8) myAction = 3;
			if(cBluff > 60 && myOdds >= myNiveau[2] + 12) myAction = 3;

		}
		else {
			// call -> über niveau0, schon einiges gesetzt im flop, schon einiges insgesamt gesetzt 
			if(myOdds >= myNiveau[0] || (mySet >= actualHand->getFlop()->getHighestSet()/2 && myOdds >= myNiveau[0]-5) || (myRoundStartCash-myCash > individualHighestSet && myNiveau[0]-3)) {
				// all in bei knappem call
				if(actualHand->getFlop()->getHighestSet() > (myCash*3.0)/4.0) {
					raise = myCash;
					myAction = 5;
				}
				else myAction = 3;
			}
			// fold
			else {
				myAction = 1;
			}
		}
	}
	else {
		// bet
		if(myOdds >= myNiveau[1]) {
			bet = (((int)myOdds-myNiveau[1])/8)*2*actualHand->getSmallBlind();
			// bet zu klein
			if(bet == 0) {
				bet = 2*actualHand->getSmallBlind();
			}
			// all in bei nur wenigen Chips
			if(myCash/(2*actualHand->getSmallBlind()) <= 6) {
				bet = myCash;
			}
			// all in bei knappem bet
			if(bet > (myCash*4.0)/5.0) {
				bet = myCash;
			}
			myAction = 4;
			
			// auf cBluff testen --> check statt bet
			if(cBluff > 80) myAction = 2;
			if(cBluff > 70 && myOdds >= myNiveau[1] + 4) myAction = 2;
			if(cBluff > 60 && myOdds >= myNiveau[1] + 8) myAction = 2;
			if(cBluff > 50 && myOdds >= myNiveau[1] + 12) myAction = 2;
		}
		// check
		else {
			myAction = 2;
			// Position
			if(myButton == 1) {
				// Position-Bluff generieren
				myTool.getRandNumber(1,100,1,&pBluff,0);
				if(pBluff <= 16) {
					bet = (pBluff/4)*2*actualHand->getSmallBlind();
					// bet zu klein
					if(bet == 0) {
						bet = 2*actualHand->getSmallBlind();
					}
					// all in bei nur wenigen Chips
					if(myCash/(2*actualHand->getSmallBlind()) <= 6) {
						bet = myCash;
					}
					// all in bei knappem bet
					if(bet > (myCash*4.0)/5.0) {
						bet = myCash;
					}
					myAction = 4;
				}
			}
		}

	}

	// auf sBluffStatus testen --> raise statt call und bet statt check

	// aktiv oder passiv?
	if(actualHand->getFlop()->getHighestSet() > 0) {

		if(sBluffStatus && myOdds < myNiveau[2]) {
	
	// 		cout << "sBLUFF!" << endl;
	
			// Gegner setzen -> call
			if(actualHand->getFlop()->getHighestSet() >= 4*actualHand->getSmallBlind()) {
				myAction = 3;
			}
			// Standard-Raise-Routine
			else {
				// raise-Betrag ermitteln
				myTool.getRandNumber(1,8,1,&rand,0);
				raise = rand*actualHand->getSmallBlind();
				// raise-Betrag zu klein -> mindestens Standard-raise
				if(raise < actualHand->getFlop()->getHighestSet()) {
					raise = actualHand->getFlop()->getHighestSet();
				}
				// all in bei nur wenigen Chips oder knappem raise
				if(myCash/(2*actualHand->getSmallBlind()) <= 6 || raise >= (myCash*4)/5) {
					raise = myCash;
				}
				myAction = 5;
			}

			// extrem hoher set der gegner -> bluff beenden
			if(actualHand->getFlop()->getHighestSet() >= 10*actualHand->getSmallBlind()) {
				myAction = 1;
			}
		}
	}
	else {
		if(sBluffStatus && myOdds < myNiveau[1]) {
	
	// 		cout << "sBLUFF!" << endl;

			myTool.getRandNumber(1,8,1,&rand,0);
			bet = rand*actualHand->getSmallBlind();
			// bet zu klein
			if(bet == 0) {
				bet = 2*actualHand->getSmallBlind();
			}
			// all in bei nur wenigen Chips
			if(myCash/(2*actualHand->getSmallBlind()) <= 6) {
				bet = myCash;
			}
			// all in bei knappem bet
			if(bet > (myCash*4.0)/5.0) {
				bet = myCash;
			}
			myAction = 4;
		}

	}

// 	cout << myID << ": Dude " << myDude4 << "\t Wert " <<  myOdds << "\t Niveau " << myNiveau[0] << " " << myNiveau[1] << " " << myNiveau[2] << endl;

// 	cout << myID << ": " << myOdds << " - " << myNiveau[0] << " " << myNiveau[1] << " " << myNiveau[2] << " - " << "Bluff: " << sBluffStatus << endl;

	evaluation(bet, raise);




// 		cout << "Engine 0.4" << endl;
// 		int tempArray[5];
// 		int boardCards[5];
// 		int info[4];
// 		int cBluff;
// 		int sBluff;
// 		int bet = 0;
// 		int raise = 0;
// 
// 		int i;
// 		
// 		Tools myTools;
// 
// 		for(i=0; i<2; i++) tempArray[i] = myCards[i];
// 		actualBoard->getMyCards(boardCards);
// 		for(i=0; i<3; i++) tempArray[2+i] = boardCards[i];
// 
// 		for(i=0; i<4; i++) info[i] = -1;
// 
// // 		for(i=0; i<5; i++) cout << tempArray[i] << " ";
// // 		cout << endl;
// 
// 		cout << myID << ": ";
// 
// 		flopCardsValue(tempArray);
// 
// 		// aktive Situation --> check / bet
// 		if(actualHand->getFlop()->getHighestSet() == 0) {
// 
// 			switch(info[0]) {
// 				case 9: {}
// 				case 8: {}
// 				case 7: {}
// 				case 6: {}
// 				case 5: {}
// 				case 4: {
// 					myTools.getRandNumber(0,100,1,&cBluff,0);
// 					if(cBluff < 35) {
// 						myAction = 2;
// 					}
// 					else {
// 						if(myCash/(2*actualHand->getSmallBlind()) <= 8) {
// 							myAction = 6;
// 						} else {
// 							if(cBluff < 60) {
// 								bet = (7-myDude4)*2*actualHand->getSmallBlind();
// 							} else {
// 								bet = (((100-cBluff)/20)+2)*actualHand->getSmallBlind();
// 							}
// 							if(bet < 2*actualHand->getSmallBlind()) bet = 2*actualHand->getSmallBlind();
// 							myAction = 4;
// 						}
// 					}
// 				}
// 				break;
// 				case 3: {
// 					switch(info[3]) {
// 						case 2: {
// 							myTools.getRandNumber(0,100,1,&cBluff,0);
// 							if(cBluff < 35) {
// 								myAction = 2;
// 							}
// 							else {
// 								if(myCash/(2*actualHand->getSmallBlind()) <= 8) {
// 									myAction = 6;
// 								} else {
// 									if(cBluff < 60) {
// 										bet = (5-myDude4)*2*actualHand->getSmallBlind();
// 									} else {
// 										bet = (((100-cBluff)/20)+2)*actualHand->getSmallBlind();
// 									}
// 									if(bet < 2*actualHand->getSmallBlind()) bet = 2*actualHand->getSmallBlind();
// 									myAction = 4;
// 								}
// 							}
// 						}
// 						break;
// 						case 1: {
// 							myTools.getRandNumber(0,100,1,&cBluff,0);
// 							if(cBluff < 40) {
// 								myAction = 2;
// 							}
// 							else {
// 								if(myCash/(2*actualHand->getSmallBlind()) <= 6) {
// 									myAction = 6;
// 								} else {
// 									if(cBluff < 60) {
// 										bet = (3-myDude4)*2*actualHand->getSmallBlind();
// 									} else {
// 										bet = (((100-cBluff)/20)+2)*actualHand->getSmallBlind();
// 									}
// 									if(bet < 2*actualHand->getSmallBlind()) bet = 2*actualHand->getSmallBlind();
// 									myAction = 4;
// 								}
// 							}
// 						}
// 						break;
// 						default: {
// 							myTools.getRandNumber(0,100,1,&sBluff,0);
// 							if(info[1] >= 10 && sBluff <= 40) {
// 								bet = (sBluff/10)*2*actualHand->getSmallBlind();
// 								if(bet < 2*actualHand->getSmallBlind()) bet = 2*actualHand->getSmallBlind();
// 								myAction = 4;
// 							}
// 							else {
// 								myAction = 2;
// 							}
// 						}
// 					}
// 				}
// 				break;
// 				case 2: {
// 					switch(info[3]) {
// 						case 2: {
// 							myTools.getRandNumber(0,100,1,&cBluff,0);
// 							if(cBluff > 80) {
// 								myAction = 2;
// 							}
// 							else {
// 								if(myCash/(2*actualHand->getSmallBlind()) <= 6) {
// 									myAction = 6;
// 								} else {
// 									bet = (cBluff/20)*2*actualHand->getSmallBlind();
// 									if(bet < 2*actualHand->getSmallBlind()) bet = 2*actualHand->getSmallBlind();
// 									myAction = 4;
// 								}
// 							}
// 						}
// 						break;
// 						default: {
// 							myTools.getRandNumber(0,100,1,&cBluff,0);
// 							if(cBluff > 90) {
// 								myAction = 2;
// 							}
// 							else {
// 								if(myCash/(2*actualHand->getSmallBlind()) <= 5) {
// 									myAction = 6;
// 								} else {
// 									bet = (cBluff/30)*2*actualHand->getSmallBlind();
// 									if(bet < 2*actualHand->getSmallBlind()) bet = 2*actualHand->getSmallBlind();
// 									myAction = 4;
// 								}
// 							}
// 						}
// 					}
// 				}
// 				break;
// 				case 1: {
// 					if(!info[2]) {
// 					myTools.getRandNumber(0,100,1,&sBluff,0);
// 					switch(info[3]) {
// 						case 2: {}
// 						case 1: {
// 							if(actualHand->getActualQuantityPlayers() == 2) {
// 								bet = (1-myDude4)*2*actualHand->getSmallBlind();
// 								if(bet < 2*actualHand->getSmallBlind()) bet = 2*actualHand->getSmallBlind();
// 								myAction = 4;
// 
// 							}
// 							else {
// 								switch(info[1]) {
// 									case 3: {
// 										if(sBluff <= 10) {
// 											bet = (sBluff/5 + 2)*actualHand->getSmallBlind();
// 											myAction = 4;
// 										}
// 										else {
// 											myAction = 2;
// 										}
// 									}
// 									break;
// 									case 2: {
// 										if(sBluff <= 15) {
// 											bet = (sBluff/5 + 2)*actualHand->getSmallBlind();
// 											myAction = 4;
// 										}
// 										else {
// 											myAction = 2;
// 										}
// 									}
// 									break;
// 									case 1: {
// 										if(sBluff <= 50) {
// 											bet = (sBluff/15 + 2)*actualHand->getSmallBlind();
// 											myAction = 4;
// 										}
// 										else {
// 											myAction = 2;
// 										}
// 									}
// 									break;
// 									default: {
// 										if(myCash/(2*actualHand->getSmallBlind()) <= 5) {
// 											myAction = 6;
// 										} else {
// 											bet = (sBluff/15)*2*actualHand->getSmallBlind();
// 											if(bet < 2*actualHand->getSmallBlind()) bet = 2*actualHand->getSmallBlind();
// 											myAction = 4;
// 										}
// 									}
// 									break;
// 								}
// 							}
// 						}
// 						break;
// 						default: {
// 						}
// 					}
// 					}
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 				}
// 				break;
// 				default: {}
// 			}
// 
// 
// 
// 		}
// 		// passive Situation --> fold / call / raise
// 		else {
// 
// 
// 
// 		}


}


void LocalPlayer::turnEngine() {

	ConfigFile myConfig;

// 		int tempArray[6];
// 		int boardCards[5];
// 		int i;

// 		for(i=0; i<2; i++) tempArray[i] = myCards[i];
// 		actualBoard->getMyCards(boardCards);
// 		for(i=0; i<4; i++) tempArray[2+i] = boardCards[i];

// 		for(i=0; i<5; i++) cout << tempArray[i] << " ";
// 		cout << endl;

// 		cout << myID << ": ";

// 		turnCardsValue(tempArray);

	int raise = 0;
	int bet = 0;
	int i;
	int cBluff;
	int pBluff;
	Tools myTool;
	int rand;

	readFile();

	// Niveaus setzen + Dude + Anzahl Gegenspieler
	// 1. Fold -- Call
	myNiveau[0] = 49 + myDude4/* - 6*(actualHand->getActualQuantityPlayers() - 2)*/;
	// 2. Check -- Bet
	myNiveau[1] = 54 + myDude4/* - 6*(actualHand->getActualQuantityPlayers() - 2)*/;
	// 3. Call -- Raise
	myNiveau[2] = 66 + myDude4/* - 6*(actualHand->getActualQuantityPlayers() - 2)*/;

// Aggresivität des humanPlayers auslesen
	int aggValue = (int)(((actualHand->getPlayerArray()[0]->getMyAggressive()*1.0)/7.0 - 1.0/actualHand->getActualQuantityPlayers())*21.0);

	for(i=0; i<3; i++) {
		myNiveau[i] -= aggValue;
	}
	
//	cout << "Spieler " << myID << ": Dude " << myDude4 << "\t Wert " <<  myHoleCardsValue << "\t Niveau " << myNiveau[0] << " " << myNiveau[1] << " " << myNiveau[2] << "\t Agg " << aggValue << " " << endl;


	// eigenes mögliches highestSet
	int individualHighestSet = actualHand->getTurn()->getHighestSet();
	if(individualHighestSet > myCash) individualHighestSet = myCash;

	// Check-Bluff generieren
	myTool.getRandNumber(1,100,1,&cBluff,0);

	// aktiv oder passiv?
	if(actualHand->getTurn()->getHighestSet() > 0) {

//		Verhaeltnis Set / Cash
		if(myCash/individualHighestSet >= 25) {
			myNiveau[0] += (25-myCash/individualHighestSet)/10;
		} else {
			myNiveau[0] += (25-myCash/individualHighestSet)/3;
		}

		// Verhaeltnis Set / Cash für raise
		if(myCash/individualHighestSet < 10) {
			myNiveau[2] += (10-myCash/individualHighestSet)/2;
		}

		// raise (bei hohem Niveau)
		if(myOdds >= myNiveau[2]) {
	
			// raise-loop unterbinden -> d.h. entweder call oder bei superblatt all in
			if(actualHand->getTurn()->getHighestSet() >= 12*actualHand->getSmallBlind()) {
				// all in
				if(myOdds >= myNiveau[2] + 8) {
					raise = myCash;
					myAction = 5;
				}
					// nur call
				else {
					myAction = 3;
				}
	
			// Standard-Raise-Routine
			} else {
				// raise-Betrag ermitteln
				raise = (((int)myOdds-myNiveau[2])/4)*2*actualHand->getSmallBlind();
				// raise-Betrag zu klein -> mindestens Standard-raise
				if(raise < actualHand->getTurn()->getHighestSet()) {
					raise = actualHand->getTurn()->getHighestSet();
				}
				// all in bei nur wenigen Chips oder knappem raise
				if(myCash/(2*actualHand->getSmallBlind()) <= 6 || raise >= (myCash*4.0)/5.0) {
					raise = myCash;
				}
				myAction = 5;
			}
			// auf cBluff testen --> call statt raise
			if(cBluff > 90) myAction = 3;
			if(cBluff > 80 && myOdds >= myNiveau[2] + 5) myAction = 3;
			if(cBluff > 70 && myOdds >= myNiveau[2] + 10) myAction = 3;
			if(cBluff > 60 && myOdds >= myNiveau[2] + 15) myAction = 3;
		}
		else {
			// call -> über niveau0, schon einiges gesetzt im flop, schon einiges insgesamt gesetzt 
			if(myOdds >= myNiveau[0] || (mySet >= actualHand->getTurn()->getHighestSet()/2 && myOdds >= myNiveau[0]-5) || (myRoundStartCash-myCash > individualHighestSet && myNiveau[0]-3)) {
				// all in bei knappem call
				if(actualHand->getTurn()->getHighestSet() > (myCash*3.0)/4.0) {
					raise = myCash;
					myAction = 5;
				}
				else myAction = 3;
			}
			// fold
			else {
				myAction = 1;
			}
		}
	}
	// aktiv
	else {
		// bet
		if(myOdds >= myNiveau[1]) {
			bet = (((int)myOdds-myNiveau[1])/6)*2*actualHand->getSmallBlind();
			if(bet == 0) {
				bet = 2*actualHand->getSmallBlind();
			}
			// all in bei nur wenigen Chips
			if(myCash/(2*actualHand->getSmallBlind()) <= 6) {
				bet = myCash;
			}
			// all in bei knappem bet
			if(bet > (myCash*4.0)/5.0) {
				bet = myCash;
			}
			myAction = 4;

			// auf cBluff testen --> call statt raise
			if(cBluff > 90) myAction = 2;
			if(cBluff > 80 && myOdds >= myNiveau[2] + 5) myAction = 2;
			if(cBluff > 70 && myOdds >= myNiveau[2] + 10) myAction = 2;
			if(cBluff > 60 && myOdds >= myNiveau[2] + 15) myAction = 2;
		}
		// check
		else {
			myAction = 2;
			// Position
			if(myButton == 1) {
				// Position-Bluff generieren
				myTool.getRandNumber(1,100,1,&pBluff,0);
				if(pBluff <= 16) {
					bet = (pBluff/4)*2*actualHand->getSmallBlind();
					// bet zu klein
					if(bet == 0) {
						bet = 2*actualHand->getSmallBlind();
					}
					// all in bei nur wenigen Chips
					if(myCash/(2*actualHand->getSmallBlind()) <= 6) {
						bet = myCash;
					}
					// all in bei knappem bet
					if(bet > (myCash*4.0)/5.0) {
						bet = myCash;
					}
					myAction = 4;
				}
			}
		}
	}

	// auf sBluffStatus testen --> raise statt call und bet statt check

	// aktiv oder passiv?
	if(actualHand->getTurn()->getHighestSet() > 0) {

		if(sBluffStatus && myOdds < myNiveau[2]) {
	
	// 		cout << "sBLUFF!" << endl;
	
			// Gegner setzen -> call
			if(actualHand->getTurn()->getHighestSet() >= 4*actualHand->getSmallBlind()) {
				myAction = 3;
			}
			// Standard-Raise-Routine
			else {
				// raise-Betrag ermitteln
				myTool.getRandNumber(1,8,1,&rand,0);
				raise = rand*actualHand->getSmallBlind();
				// raise-Betrag zu klein -> mindestens Standard-raise
				if(raise < actualHand->getTurn()->getHighestSet()) {
					raise = actualHand->getTurn()->getHighestSet();
				}
				// all in bei nur wenigen Chips oder knappem raise
				if(myCash/(2*actualHand->getSmallBlind()) <= 6 || raise >= (myCash*4)/5) {
					raise = myCash;
				}
				myAction = 5;
			}

			// extrem hoher set der gegner -> bluff beenden
			if(actualHand->getTurn()->getHighestSet() >= 10*actualHand->getSmallBlind()) {
				myAction = 1;
			}
		}
	}
	else {
		if(sBluffStatus && myOdds < myNiveau[1]) {
	
	// 		cout << "sBLUFF!" << endl;

			myTool.getRandNumber(1,8,1,&rand,0);
			bet = rand*actualHand->getSmallBlind();
			// bet zu klein
			if(bet == 0) {
				bet = 2*actualHand->getSmallBlind();
			}
			// all in bei nur wenigen Chips
			if(myCash/(2*actualHand->getSmallBlind()) <= 6) {
				bet = myCash;
			}
			// all in bei knappem bet
			if(bet > (myCash*4.0)/5.0) {
				bet = myCash;
			}
			myAction = 4;
		}

	}

// 	cout << myID << ": " << myOdds << " - " << myNiveau[0] << " " << myNiveau[1] << " " << myNiveau[2] << " - " << "Bluff: " << sBluffStatus << endl;

	evaluation(bet, raise);

}


void LocalPlayer::riverEngine() {

	ConfigFile myConfig;

// 	int tempArray[6];
// 	int boardCards[5];
// 	int i;

// 	for(i=0; i<2; i++) tempArray[i] = myCards[i];
// 	actualBoard->getMyCards(boardCards);
// 	for(i=0; i<4; i++) tempArray[2+i] = boardCards[i];

// 	for(i=0; i<5; i++) cout << tempArray[i] << " ";
// 	cout << endl;

// 	cout << myID << ": ";

// 	turnCardsValue(tempArray);


	int raise = 0;
	int bet = 0;
	int i;
	Tools myTool;
	int rand;
	int pBluff;

	readFile();

	// Niveaus setzen + Dude + Anzahl Gegenspieler
	// 1. Fold -- Call
	myNiveau[0] = 49 + myDude4/* - 6*(actualHand->getActualQuantityPlayers() - 2)*/;
	// 2. Check -- Bet
	myNiveau[1] = 54 + myDude4/* - 6*(actualHand->getActualQuantityPlayers() - 2)*/;
	// 3. Call -- Raise
	myNiveau[2] = 66 + myDude4/* - 6*(actualHand->getActualQuantityPlayers() - 2)*/;

	// Aggresivität des humanPlayers auslesen
	int aggValue = (int)(((actualHand->getPlayerArray()[0]->getMyAggressive()*1.0)/7.0 - 1.0/actualHand->getActualQuantityPlayers())*21.0);

	for(i=0; i<3; i++) {
		myNiveau[i] -= aggValue;
	}
	
//	cout << "Spieler " << myID << ": Dude " << myDude4 << "\t Wert " <<  myHoleCardsValue << "\t Niveau " << myNiveau[0] << " " << myNiveau[1] << " " << myNiveau[2] << "\t Agg " << aggValue << " " << endl;

	// eigenes mögliches highestSet
	int individualHighestSet = actualHand->getRiver()->getHighestSet();
	if(individualHighestSet > myCash) individualHighestSet = myCash;

	// aktiv oder passiv?
	if(actualHand->getRiver()->getHighestSet() > 0) {

		// Verhaeltnis Set / Cash
		if(myCash/individualHighestSet >= 25) {
			myNiveau[0] += (25-myCash/individualHighestSet)/10;
		} else {
			myNiveau[0] += (25-myCash/individualHighestSet)/3;
		}

		// Verhaeltnis Set / Cash für raise
		if(myCash/individualHighestSet < 10) {
			myNiveau[2] += (10-myCash/individualHighestSet)/2;
		}

		// raise (bei hohem Niveau)
		if(myOdds >= myNiveau[2]) {
			// raise-loop unterbinden -> d.h. entweder call oder bei superblatt all in
			if(actualHand->getRiver()->getHighestSet() >= 12*actualHand->getSmallBlind()) {
				// all in
				if(myOdds >= myNiveau[2] + 8) {
					raise = myCash;
					myAction = 5;
				}
				// nur call
				else {
					myAction = 3;
				}
			}
			// Standard-Raise-Routine
			else {
				// raise-Betrag ermitteln
				raise = (((int)myOdds-myNiveau[2])/2)*2*actualHand->getSmallBlind();
				// raise-Betrag zu klein -> mindestens Standard-raise
				if(raise == 0) {
					raise = actualHand->getRiver()->getHighestSet();
				}
				// all in bei nur wenigen Chips
				if(myCash/(2*actualHand->getSmallBlind()) <= 8) {
					raise = myCash;
				}
				myAction = 5;
			}
		}
		else {
			// call -> über niveau0, schon einiges gesetzt im flop, schon einiges insgesamt gesetzt 
			if(myOdds >= myNiveau[0] || (mySet >= actualHand->getRiver()->getHighestSet()/2 && myOdds >= myNiveau[0]-5) || (myRoundStartCash-myCash > individualHighestSet && myNiveau[0]-3)) {
				// all in bei knappem call
				if(myCash-actualHand->getRiver()->getHighestSet() <= (myCash*1)/4) {
					raise = myCash;
					myAction = 5;
				}
				else myAction = 3;
			}
			// fold
			else {
				myAction = 1;
			}
		}
	}
	else {
		// bet
		if(myOdds >= myNiveau[1]) {
			bet = (((int)myOdds-myNiveau[1])/3)*2*actualHand->getSmallBlind();
			if(bet == 0) {
				bet = 2*actualHand->getSmallBlind();
			}
			// all in bei nur wenigen Chips
			if(myCash/(2*actualHand->getSmallBlind()) <= 6) {
				raise = myCash;
			}
			// all in bei knappem bet
			if(bet > (myCash*4.0)/5.0) {
				bet = myCash;
			}
			myAction = 4;
		}
		// check
		else {
			myAction = 2;
			// Position
			if(myButton == 1) {
				// Position-Bluff generieren
				myTool.getRandNumber(1,100,1,&pBluff,0);
				if(pBluff <= 20) {
					bet = (pBluff/4)*2*actualHand->getSmallBlind();
					// bet zu klein
					if(bet == 0) {
						bet = 2*actualHand->getSmallBlind();
					}
					// all in bei nur wenigen Chips
					if(myCash/(2*actualHand->getSmallBlind()) <= 6) {
						bet = myCash;
					}
					// all in bei knappem bet
					if(bet > (myCash*4.0)/5.0) {
						bet = myCash;
					}
					myAction = 4;
				}
			}
		}
	}

	// auf sBluffStatus testen --> raise statt call und bet statt check

	// aktiv oder passiv?
	if(actualHand->getRiver()->getHighestSet() > 0) {

		if(sBluffStatus && myOdds < myNiveau[2]) {
	
	// 		cout << "sBLUFF!" << endl;
	
			// Gegner setzen -> call
			if(actualHand->getRiver()->getHighestSet() >= 4*actualHand->getSmallBlind()) {
				myAction = 3;
			}
			// Standard-Raise-Routine
			else {
				// raise-Betrag ermitteln
				myTool.getRandNumber(1,8,1,&rand,0);
				raise = rand*actualHand->getSmallBlind();
				// raise-Betrag zu klein -> mindestens Standard-raise
				if(raise < actualHand->getRiver()->getHighestSet()) {
					raise = actualHand->getRiver()->getHighestSet();
				}
				// all in bei nur wenigen Chips oder knappem raise
				if(myCash/(2*actualHand->getSmallBlind()) <= 6 || raise >= (myCash*4)/5) {
					raise = myCash;
				}
				myAction = 5;
			}

			// extrem hoher set der gegner -> bluff beenden
			if(actualHand->getRiver()->getHighestSet() >= 10*actualHand->getSmallBlind()) {
				myAction = 1;
			}
		}
	}
	else {
		if(sBluffStatus && myOdds < myNiveau[1]) {
	
	// 		cout << "sBLUFF!" << endl;

			myTool.getRandNumber(1,8,1,&rand,0);
			bet = rand*actualHand->getSmallBlind();
			// bet zu klein
			if(bet == 0) {
				bet = 2*actualHand->getSmallBlind();
			}
			// all in bei nur wenigen Chips
			if(myCash/(2*actualHand->getSmallBlind()) <= 6) {
				bet = myCash;
			}
			// all in bei knappem bet
			if(bet > (myCash*4.0)/5.0) {
				bet = myCash;
			}
			myAction = 4;
		}

	}

// 	cout << myID << ": " << myOdds << " - " << myNiveau[0] << " " << myNiveau[1] << " " << myNiveau[2] << " - " << "Bluff: " << sBluffStatus << endl;

	evaluation(bet, raise);




}


void LocalPlayer::evaluation(int bet, int raise) {

	int highestSet = 0;

	switch(actualHand->getActualRound()) {
		case 0: highestSet = actualHand->getPreflop()->getHighestSet();
		break;
		case 1: highestSet = actualHand->getFlop()->getHighestSet();
		break;
		case 2: highestSet = actualHand->getTurn()->getHighestSet();
		break;
		case 3: highestSet = actualHand->getRiver()->getHighestSet();
		break;
		default: cout << "ERROR - wrong init of actualRound" << endl;
	}

	switch(myAction) {
		// none
		case 0: {}
		break;
		// fold
		case 1: {}
		break;
		// check
		case 2: {}
		break;
		// call
		case 3: {
			// all in
			if(highestSet >= myCash) {
					mySet += myCash;
					myCash = 0;
					myAction = 6;
					}
			// sonst
			else {
				myCash = myCash - highestSet + mySet;
				mySet = highestSet;
			}
		}
		break;
		// bet
		case 4: {
			// all in
			if(bet >= myCash) {
				mySet += myCash;
				myCash = 0;
				myAction = 6;
				highestSet = mySet;
			}
			// sonst
			else {
				myCash = myCash - bet;
				mySet = bet;
				highestSet = mySet;
			}
		}
		break;
		// raise
		case 5: {
			// all in
			if(highestSet + raise >= myCash) {
				mySet += myCash;
				myCash = 0;
				myAction = 6;
				if(mySet > highestSet) highestSet = mySet;
			}
			// sonst
			else {
				myCash = myCash + mySet - highestSet - raise;
				mySet = highestSet + raise;
				highestSet = mySet;
			}
		}
		break;
		// all in
		case 6: {}
		break;
		default: {}
	}

	switch(actualHand->getActualRound()) {
		case 0: actualHand->getPreflop()->setHighestSet(highestSet);
		break;
		case 1: actualHand->getFlop()->setHighestSet(highestSet);
		break;
		case 2: actualHand->getTurn()->setHighestSet(highestSet);
		break;
		case 3: actualHand->getRiver()->setHighestSet(highestSet);
		break;
		default: cout << "ERROR - wrong init of actualRound" << endl;
	}


}


int LocalPlayer::preflopCardsValue(int* cards) {

	// Code der HoleCards ermitteln
	if(cards[0]%13 == cards[1]%13) {
		return ((cards[0]%13)*1000 + (cards[0]%13)*10);
	} else {
		if(cards[0]%13 < cards[1]%13) {
			if(cards[0]/13 == cards[1]/13) {
				return ((cards[0]%13)*1000 + (cards[1]%13)*10 + 1);
			} else {
				return ((cards[0]%13)*1000 + (cards[1]%13)*10);
			}
		} else {
			if(cards[0]/13 == cards[1]/13) {
				return ((cards[1]%13)*1000 + (cards[0]%13)*10 + 1);
			} else {
				return ((cards[1]%13)*1000 + (cards[0]%13)*10);
			}
		}
	}

}


int LocalPlayer::flopCardsValue(int* cards) {

	int array[5][3];
	int j1, j2, j3, j4, j5, k1, k2, ktemp[3];
	int temp = 0;
	int temp1 = 0;
	int temp2 = 0;
	int temp2Array[2];
	int tempValue = -1;
	bool breakLoop = 0;

	// Kartenwerte umwandeln (z.B. [ 11 (Karo K�ig) -> 0 11 ] oder [ 31 (Pik 7) -> 2 5 ] )
	for(j1=0; j1<5; j1++) {
		array[j1][0] = cards[j1]/13;
		array[j1][1] = cards[j1]%13;
		array[j1][2] = j1;
	}

	// Karten nach Farben sortieren: Kreuz - Pik - Herz - Karo
	for(k1=0; k1<5; k1++) {
		for(k2=k1+1; k2<5; k2++) {
			if(array[k1][0]<array[k2][0]) {
				ktemp[0] = array[k1][0];
				ktemp[1] = array[k1][1];
				ktemp[2] = array[k1][2];
				array[k1][0] = array[k2][0];
				array[k1][1] = array[k2][1];
				array[k1][2] = array[k2][2];
				array[k2][0] = ktemp[0];
				array[k2][1] = ktemp[1];
				array[k2][2] = ktemp[2];
			}
		}
	}

	// Karten innerhalb der Farben nach der Gr�e sortieren: Ass - K�ig - Dame - ... - 4 - 3 - 2
	for(k1=0; k1<5; k1++) {
		for(k2=k1+1; k2<5; k2++) {
			if(array[k1][0]==array[k2][0] && array[k1][1]<array[k2][1]) {
				ktemp[0] = array[k1][0];
				ktemp[1] = array[k1][1];
				ktemp[2] = array[k1][2];
				array[k1][0] = array[k2][0];
				array[k1][1] = array[k2][1];
				array[k1][2] = array[k2][2];
				array[k2][0] = ktemp[0];
				array[k2][1] = ktemp[1];
				array[k2][2] = ktemp[2];
			}
		}
	}

// auf Straight Flush und Flush testen	
	// 5 Karten gleiche Farbe ?
   	if(array[0][0] == array[1][0] && array[0][0] == array[2][0] && array[0][0] == array[3][0] && array[0][0] == array[4][0]) {
		// Straight Flush?
		if(array[0][1]-4 == array[4][1]) {
//              	cout << "Straight Flush";
             		return 80000;
		}
		else {
		     	// Straight Flush Ausnahme: 5-4-3-2-A
             		if(array[0][1]==12 && array[1][1]==3 && array[2][1]==2 && array[3][1]==1 && array[4][1]==0) {
//                   		cout << "Straight Flush Ass unten";
                  		return 80000;
             		}
             		// Flush
             		else {
//                   		cout << "Flush";
                  		return 80000;
             		}
        	}
	}
	
// auf Straight Flush Draw und Flush Draw testen
   	for(j1=0; j1<2 && !breakLoop; j1++) {
		// 4 Karten gleiche Farbe ?
		if(array[j1][0] == array[j1+1][0] && array[j1][0] == array[j1+2][0] && array[j1][0] == array[j1+3][0]) {
			// zusammenhaengender Strassenansatz ?
			if(array[j1][1]-3 == array[j1+3][1]) {
                  		// Strassenansatz am Rand?
                  		if(array[j1][1] == 12) {
//                        		cout << "zusammenhaengender Straight-Flush-Draw mit Ass high";
					for(j2=0; j2<4; j2++) {
						if(array[j1+j2][2] <= 1) temp++;
					}
                       			return (70012 + temp*100);
                  		}
                  		// Strassenansatz in der Mitte
                  		else {
//                        		cout << "zusammenhaengender Straight-Flush-Draw in der Mitte";
					for(j2=0; j2<4; j2++) {
						if(array[j1+j2][2] <= 1) temp++;
					}
                       			return (70000 + temp*100 + array[j1][1]);
                  		}
              		}
              		else {
                   		// Bauchschuss ?
                   		if(array[j1][1]-4 == array[j1+3][1]) {
//                         		cout << "Straight-Flush-Bauchschuss";
					for(j2=0; j2<4; j2++) {
						if(array[j1+j2][2] <= 1) temp++;
					}
                       			return (71000 + temp*100 + array[j1][1]);
                   		}
                   		else {
                        		// Test auf Straight-Flush-Ausnahme 5-4-3-2-A
                        		if(array[j1][1] == 12 && (array[j1+1][1]<=3 || (array[j1+2][1]<=3 && array[j1][0]==array[j1+4][0]))) {
//                              		cout << "Straight-Flush-Draw Ass unten";
						for(j2=0; j2<4; j2++) {
							if(array[j1+j2][2] <= 1) temp++;
						}
                       				return (71012 + temp*100);
					}
                        		// Flush Draw
                        		else {
//                              		cout << "Flush Draw";

						// Anteil ermitteln
						for(j2=0; j2<4; j2++) {
							if(array[j1+j2][2] <= 1) {
								temp2Array[temp] = array[j1+j2][1];
								temp++;
							}
						}

						// Anteil 2
                       				if(temp==2) {
							for(j2=0; j2<4; j2++) {
								if(temp2Array[1] > array[j1+j2][1]) temp1++;
								if(temp2Array[0] > array[j1+j2][1]) temp2++;
							}
							return (60000 + temp1*1000 + temp2*100 + array[j1][1]);
						}
						// Anteil 1
						else {

							// 2.Stelle
							for(j2=0; j2<4; j2++) {
								if(j1==0) {
									if(array[4][1] < array[j1+j2][1]) temp1++;
								} else {
									if(array[0][1] < array[j1+j2][1]) temp1++;
								}
							}
							if(temp1 >= 2) temp1 = 1;
							if(temp1 == 4) temp1 = 2;

							// 3.Stelle
							for(j2=0; j2<4; j2++) {
								if(temp2Array[0] < array[j1+j2][1]) temp2++;
							}
							tempValue = (50000 + temp1*1000 + temp2*100 + temp2Array[0]);
							breakLoop = 1;
						}
                        		}
                   		}
              		}
         	}
    	}
	
// Karten fr den Vierling-, Full-House-, Drilling- und Paartest umsortieren
	for(k1=0; k1<5; k1++) {
		for(k2=k1+1; k2<5; k2++) {
			if(array[k1][1]<array[k2][1]) {
				ktemp[0] = array[k1][0];
				ktemp[1] = array[k1][1];
				ktemp[2] = array[k1][2];
				array[k1][0] = array[k2][0];
				array[k1][1] = array[k2][1];
				array[k1][2] = array[k2][2];
				array[k2][0] = ktemp[0];
				array[k2][1] = ktemp[1];
				array[k2][2] = ktemp[2];
			}
		}
	}
	
// auf Vierling testen
	for(j1=0; j1<2; j1++) {
		if(array[j1][1] == array[j1+1][1] && array[j1][1] == array[j1+2][1] && array[j1][1] == array[j1+3][1]) {
//			cout << "Vierling ";
             		return 80000;
        	}
	}


// auf Straight und Full House testen
     // Straight
     	if((array[0][1]-1 == array[1][1] || array[0][1]-9 == array[1][1] ) && array[1][1]-1 == array[2][1] && array[2][1]-1 == array[3][1] && array[3][1]-1 == array[4][1]) {
//           	cout << "Straight";
          	return 80000;
 	 }
 	 // Full House
	 if((array[0][1] == array[1][1] && array[0][1] == array[2][1] && array[3][1] == array[4][1]) || (array[2][1] == array[3][1] && array[2][1] == array[4][1] && array[0][1] == array[1][1])) {
//           	cout << "Full House";
          	return 80000;	
	 }


// auf Straßenansatz testen
	for(j1=0; j1<5 && !breakLoop; j1++) {
		for(j2=j1+1; j2<5 && !breakLoop; j2++) {
			for(j3=j2+1; j3<5 && !breakLoop; j3++) {
				for(j4=j3+1; j4<5 && !breakLoop; j4++) {
					// zusammenhaengender Strassenansatz ?
					if((array[j1][1]-1 == array[j2][1] || (array[j1][1]-9 == array[j2][1] && array[j1][1] == 12)) && array[j2][1]-1 == array[j3][1] && array[j3][1]-1 == array[j4][1]) { 
                  				// Strassenansatz am Rand?
                  				if(array[j1][1] == 12) {
// 							cout << "zusammenhaengender Straight-Draw mit Ass high";

							// Anteil ermitteln
							if(array[j1][2] <= 1) {
								temp2Array[temp] = array[j1][1];
								temp++;
							}
							if(array[j2][2] <= 1) {
								temp2Array[temp] = array[j2][1];
								temp++;
							}
							if(array[j3][2] <= 1) {
								temp2Array[temp] = array[j3][1];
								temp++;
							}
							if(array[j4][2] <= 1) {
								temp2Array[temp] = array[j4][1];
								temp++;
							}
	
							// Anteil 2
							if(temp==2) {
								if(temp2Array[0] > array[j1][1]) {
									temp1++;
								}
								if(temp2Array[0] > array[j2][1]) {
									temp1++;
								}
								if(temp2Array[0] > array[j3][1]) {
									temp1++;
								}
								if(temp2Array[0] > array[j4][1]) {
									temp1++;
								}
	
								if(temp1 >= 3) temp1 = 2;
	
								tempValue = (40012 + temp1*2000);
							}
							// Anteil 1
							else {
	
								// 2.Stelle
								if(temp2Array[0] > array[j1][1]) {
									temp1++;
								}
								if(temp2Array[0] > array[j2][1]) {
									temp1++;
								}
								if(temp2Array[0] > array[j3][1]) {
									temp1++;
								}
								if(temp2Array[0] > array[j4][1]) {
									temp1++;
								}
	
								if(temp1 >= 1) temp1 = 2;

								tempValue = (40012 + (temp1+1)*1000);
							}
							breakLoop = 1;
						}
                  				// Strassenansatz in der Mitte
                  				else {
//                        				cout << "zusammenhaengender Straight-Draw in der Mitte";

							// Anteil ermitteln
							if(array[j1][2] <= 1) {
								temp2Array[temp] = array[j1][1];
								temp++;
							}
							if(array[j2][2] <= 1) {
								temp2Array[temp] = array[j2][1];
								temp++;
							}
							if(array[j3][2] <= 1) {
								temp2Array[temp] = array[j3][1];
								temp++;
							}
							if(array[j4][2] <= 1) {
								temp2Array[temp] = array[j4][1];
								temp++;
							}
	
							// Anteil 2
							if(temp==2) {
								if(temp2Array[0] > array[j1][1]) {
									temp1++;
								}
								if(temp2Array[0] > array[j2][1]) {
									temp1++;
								}
								if(temp2Array[0] > array[j3][1]) {
									temp1++;
								}
								if(temp2Array[0] > array[j4][1]) {
									temp1++;
								}
	
								if(temp1 >= 3) temp1 = 2;
	
								tempValue = (40000 + (temp1+2)*2000 + array[j1][1]);
							}
							// Anteil 1
							else {
	
								// 2.Stelle
								if(temp2Array[0] > array[j1][1]) {
									temp1++;
								}
								if(temp2Array[0] > array[j2][1]) {
									temp1++;
								}
								if(temp2Array[0] > array[j3][1]) {
									temp1++;
								}
								if(temp2Array[0] > array[j4][1]) {
									temp1++;
								}
	
								if(temp1 >= 1) temp1 = 2;
	
								// 3.Stelle
								for(j5=0; j5<5; j5++) {
									if(j5 != j1 && j5 != j2 && j5 != j3 && j5 != j4) {
										if(array[j5][1] < array[j4][1]) {
											temp2 = 0;
										}
										else {
											temp2 = 1;
										}
									}
								}

								tempValue = (40000 + (temp1+5)*1000 + temp2*100 + array[j1][1]);
							}
							breakLoop = 1;
                  				}
            				}
              				else {
                   				// Bauchschuss ?
                   				if((array[j1][1]-2 == array[j2][1] && array[j2][1]-1 == array[j3][1] && array[j3][1]-1 == array[j4][1]) || (array[j1][1]-1 == array[j2][1] && array[j2][1]-2 == array[j3][1] && array[j3][1]-1 == array[j4][1]) || (array[j1][1]-1 == array[j2][1] && array[j2][1]-1 == array[j3][1] && array[j3][1]-2 == array[j4][1])) {
// 							cout << "Straight-Draw Bauchschuss";

							// Anteil ermitteln
							if(array[j1][2] <= 1) {
								temp2Array[temp] = array[j1][1];
								temp++;
							}
							if(array[j2][2] <= 1) {
								temp2Array[temp] = array[j2][1];
								temp++;
							}
							if(array[j3][2] <= 1) {
								temp2Array[temp] = array[j3][1];
								temp++;
							}
							if(array[j4][2] <= 1) {
								temp2Array[temp] = array[j4][1];
								temp++;
							}
	
							// Anteil 2
							if(temp==2) {
								if(temp2Array[0] > array[j1][1]) {
									temp1++;
								}
								if(temp2Array[0] > array[j2][1]) {
									temp1++;
								}
								if(temp2Array[0] > array[j3][1]) {
									temp1++;
								}
								if(temp2Array[0] > array[j4][1]) {
									temp1++;
								}
	
								if(temp1 >= 3) temp1 = 2;
	
								tempValue = (40000 + temp1*2000 + array[j1][1]);
							}
							// Anteil 1
							else {
	
								// 2.Stelle
								if(temp2Array[0] > array[j1][1]) {
									temp1++;
								}
								if(temp2Array[0] > array[j2][1]) {
									temp1++;
								}
								if(temp2Array[0] > array[j3][1]) {
									temp1++;
								}
								if(temp2Array[0] > array[j4][1]) {
									temp1++;
								}
	
								if(temp1 >= 1) temp1 = 2;
	
								// 3.Stelle
								for(j5=0; j5<5; j5++) {
									if(j5 != j1 && j5 != j2 && j5 != j3 && j5 != j4) {
										if(array[j5][1] < array[j4][1]) {
											temp2 = 0;
										}
										else {
											temp2 = 1;
										}
									}
								}

								tempValue = (40000 + (temp1+1)*1000 + temp2*100 + array[j1][1]);
							}
							breakLoop = 1;
                   				}
                   				else {
                        			// Test auf Straßenansatz-Ausnahme 5-4-3-2-A
                        				if(array[j1][1] == 12 && ((array[j1][1]-9 == array[j2][1] && array[j2][1]-1 == array[j3][1] && array[j3][1]-1 == array[j4][1]) || (array[j1][1]-9 == array[j2][1] && array[j2][1]-1 == array[j3][1] && array[j3][1]-2 == array[j4][1]) || (array[j1][1]-9 == array[j2][1] && array[j2][1]-2 == array[j3][1] && array[j3][1]-1 == array[j4][1]) || (array[j1][1]-10 == array[j2][1] && array[j2][1]-1 == array[j3][1] && array[j3][1]-1 == array[j4][1]))) {
//                              				cout << "Straight-Draw Ass unten";

								// Anteil ermitteln
								if(array[j1][2] <= 1) {
									temp2Array[temp] = array[j1][1];
									temp++;
								}
								if(array[j2][2] <= 1) {
									temp2Array[temp] = array[j2][1];
									temp++;
								}
								if(array[j3][2] <= 1) {
									temp2Array[temp] = array[j3][1];
									temp++;
								}
								if(array[j4][2] <= 1) {
									temp2Array[temp] = array[j4][1];
									temp++;
								}
		
								// Anteil 2
								if(temp==2) {
									if(temp2Array[0] > array[j1][1]) {
										temp1++;
									}
									if(temp2Array[0] > array[j2][1]) {
										temp1++;
									}
									if(temp2Array[0] > array[j3][1]) {
										temp1++;
									}
									if(temp2Array[0] > array[j4][1]) {
										temp1++;
									}
		
									if(temp1 >= 3) temp1 = 2;
		
									tempValue = (40012 + temp1*2000);
								}
								// Anteil 1
								else {
		
									// 2.Stelle
									if(temp2Array[0] > array[j1][1]) {
										temp1++;
									}
									if(temp2Array[0] > array[j2][1]) {
										temp1++;
									}
									if(temp2Array[0] > array[j3][1]) {
										temp1++;
									}
									if(temp2Array[0] > array[j4][1]) {
										temp1++;
									}
		
									if(temp1 >= 1) temp1 = 2;
	
									tempValue = (40004 + (temp1+1)*1000 + 100);
								}

                       						breakLoop = 1;
					    		}
						}
					}
				}
			}
		}
	}

	 
// auf Drilling testen
	for(j1=0; j1<3; j1++) {
		if(array[j1][1] == array[j1+1][1] && array[j1][1] == array[j1+2][1]) {
//              	cout << "Drilling";
			for(j2=0; j2<3; j2++) {
				if(array[j1+j2][2] <= 1) temp++;
			}
			if(temp >=1) {
				return 80000;
			} else {
				if(j1==0) {
					return (30000 + array[j1+3][1]);
				} else {
					return (30100 + array[0][1]);
				}
			}
		}
	}

	// auf Zwei Paare testen
	for(j1=0; j1<2; j1++) {
		for(j2=j1+2; j2<4; j2++) {
			if(array[j1][1] == array[j1+1][1] && array[j2][1] == array[j2+1][1]) {
//              		cout << "Zwei Paare";
				// Anteil ermitteln
				for(j3=0; j3<2; j3++) {
					if(array[j1+j3][2] <= 1) {
						temp2Array[temp] = array[j1+j3][1];
						temp++;
					}
				}
				for(j3=0; j3<2; j3++) {
					if(array[j2+j3][2] <= 1) {
						temp2Array[temp] = array[j2+j3][1];
						temp++;
					}
				}

				// Anteil 2
				if(temp == 2) {
					if(temp2Array[0] != temp2Array[1]) {
						return (22200 + temp2Array[0]);
					}
					else {
						if(temp2Array[0] == array[j1][1]) {
							return (22100 + temp2Array[0]);
						} else {
							return (22000 + temp2Array[0]);
						}
					}

				}
				// Anteil 1
				else {
					if(temp2Array[0] == array[j1][1]) {
						return 21100 + array[j1][1];
					} else {
						return 21000 + array[j2][1];
					}
				}
			}
		}
	}

	temp = 0;
	temp1 = 0;
	temp2 = 0;

	// auf Paar testen
	for(j1=0; j1<4; j1++) {
		if(array[j1][1] == array[j1+1][1]) {
// 			cout << "Paar";
			// ohne Straight- und Flush-Draw
			if(!breakLoop) {
				// Anteil ermitteln
				for(j2=0; j2<2; j2++) {
					if(array[j1+j2][2] <= 1) temp++;
				}
				// Anteil 2
				if(temp == 2) {
					return (12000 + j1*100 + array[j1][1]);
				} else {
					// Anteil 1
					if(temp == 1) {
						for(j2=0; j2<5; j2++) {
							if(array[j2][2] >= 2 && array[j2][1] > array[j1][1]) temp1++;
						}
						return (11000 + temp1*100 + array[j1][1]);
					}
					// Anteil 0
					else {
						for(j2=0; j2<5; j2++) {
							if(array[j2][2] <= 1 && array[j2][1] > temp1) temp1 = array[j2][1];
						}
						for(j2=0; j2<5; j2++) {
							if(array[j2][2] >= 2 && array[j2][1] > temp1) temp2++;
						}
						if(temp2 == 2) temp2 = 1;
						if(temp2 == 3) temp2 = 2;
						return (10000 + temp2*100 + temp1);
					}
				}
			}
			else {
				// STraight (==4)
				if(((int)(tempValue/10000)) == 4) {
					return (((int)(tempValue/1000))*1000 + 200+ (tempValue - ((int)(tempValue/100))*100));
				}
				// Flush Anteil 1 (==5)
				else {
					return (((int)(tempValue/10000))*10000 + 3000 + (tempValue - ((int)(tempValue/1000))*1000));
				}
			}
		}
	}

	// Highest Card (Klasse 0) + Kicker

	// ohne Straight- und Flush-Draw
	if(!breakLoop) {
// 		cout << "Highest Card";
		// Anteil ermitteln
		for(j1=0; j1<5; j1++) {
			if(array[j1][2] <= 1) {
				temp2Array[temp] = array[j1][1];
				temp++;
			}
		}
		for(j1=0; j1<5; j1++) {
			if(temp2Array[1] > array[j1][1]) temp1++;
			if(temp2Array[0] > array[j1][1]) temp2++;
		}
		return (temp1*1000 + temp2*100 + array[0][1]);

	} else {
		return tempValue;
	}

	
}


void LocalPlayer::readFile() {

	ConfigFile myConfig;
	int handCode;

	switch(actualHand->getActualRound()) {

		case 0: {
			
			handCode = preflopCardsValue(myCards);

			std::string fileName = myConfig.readConfigString("DataDir")+"preflopValues";
			
			ifstream fin;
			
			fin.open(fileName.c_str());
			if(!fin) {
				cout << "Es ist nicht möglich " << fileName << " zum lesen zu oeffnen." << endl;
			}
		
			// übergang solange preflopValue und flopValue noch nicht bereinigt
			int players = actualHand->getActualQuantityPlayers();
			if(players > 5) players = 5;
			
			char buffer[50];
			char hand[6];
			hand[5] = '\0';
			int i,j,k;
			char preflopValue[8];
			while(fin.getline(buffer,50)) {
				for(i=0; i<5; i++) hand[i] = buffer[i];
				if(handCode == atoi(hand)) {
					j = 0;
					k = 2;
					while(buffer[j] != '|' || k < players) {
						if(buffer[j] == '|') k++;
						j++;
					}
					for(k=0; k<8 ; k++) preflopValue[k] = buffer[k+j+1];
					myOdds = 100.0*atof(preflopValue);
					break;
				}
			}
			fin.close();
			if(myOdds == -1) cout << "ERROR myOdds - " << handCode << endl;

		}
		break;
		case 1: {
		
			int tempArray[5];
			int boardCards[5];
		
			int i,j,k;
		
			for(i=0; i<2; i++) tempArray[i] = myCards[i];
			actualBoard->getMyCards(boardCards);
			for(i=0; i<3; i++) tempArray[2+i] = boardCards[i];
		
		// 		cout << myID << ": ";
		
			handCode = flopCardsValue(tempArray);
		
		// 		cout << "\t" << handCode << endl;
		
			std::string fileName = myConfig.readConfigString("DataDir")+"flopValues";
			
			ifstream fin;
			
			fin.open(fileName.c_str());
			if(!fin) {
				cout << "Es ist nicht möglich " << fileName << " zum lesen zu oeffnen." << endl;
			}
			
			// übergang solange preflopValue und flopValue noch nicht bereinigt
			int players = actualHand->getActualQuantityPlayers();
			if(players > 5) players = 5;
		
			if(handCode != 80000) {
				char buffer[50];
				char hand[6];
				hand[5] = '\0';
				char preflopValue[8];
				while(fin.getline(buffer,50)) {
					for(i=0; i<5; i++) hand[i] = buffer[i];
					if(handCode == atoi(hand)) {
						j = 0;
						k = 2;
						while(buffer[j] != '|' || k < players) {
							if(buffer[j] == '|') k++;
							j++;
						}
						for(k=0; k<8 ; k++) preflopValue[k] = buffer[k+j+1];
						myOdds = 100.0*atof(preflopValue);
						break;
					}
				}
				fin.close();
				if(myOdds == -1) {
					cout << "ERROR" << endl;
					for(i=0; i<5; i++) cout << tempArray[i] << " ";
// 					cout << "\t" << handCode << "\t" << myID << endl;
				} else {
		// 			cout << myHoleCardsValue << endl;
			}
			} else {
				myOdds = 100;
			}

		}
		break;
		case 2: {

			// Prozent ausrechnen
	
			int i, j, k;
			int tempBoardCardsArray[5];
			int tempMyCardsArray[7];
			int tempOpponentCardsArray[7];
			actualBoard->getMyCards(tempBoardCardsArray);
		
			tempMyCardsArray[0] = myCards[0];
			tempMyCardsArray[1] = myCards[1];
			tempMyCardsArray[2] = tempBoardCardsArray[0];
			tempMyCardsArray[3] = tempBoardCardsArray[1];
			tempMyCardsArray[4] = tempBoardCardsArray[2];
			tempMyCardsArray[5] = tempBoardCardsArray[3];
		
			tempOpponentCardsArray[2] = tempBoardCardsArray[0];
			tempOpponentCardsArray[3] = tempBoardCardsArray[1];
			tempOpponentCardsArray[4] = tempBoardCardsArray[2];
			tempOpponentCardsArray[5] = tempBoardCardsArray[3];
		
			int tempMyCardsValue;
			int tempOpponentCardsValue;
		
			int countAll = 0;
			int countMy = 0;
		
			for(i=0; i<49; i++) {
				if(i != myCards[0] && i != myCards[1] && i != tempBoardCardsArray[0] && i != tempBoardCardsArray[1] && i != tempBoardCardsArray[2]) {
					for(j=i+1; j<50; j++) {
						if(j != myCards[0] && j != myCards[1] && j != tempBoardCardsArray[0] && j != tempBoardCardsArray[1] && j != tempBoardCardsArray[2]) {
							for(k=j+1; k<51; k++) {
								if(k != myCards[0] && k != myCards[1] && k != tempBoardCardsArray[0] && k != tempBoardCardsArray[1] && k != tempBoardCardsArray[2]) {
				
									countAll++;
				
									tempOpponentCardsArray[0] = i;
									tempOpponentCardsArray[1] = j;
									tempOpponentCardsArray[6] = k;
									tempMyCardsArray[6] = k;
									tempMyCardsValue = myCardsValue->cardsValue(tempMyCardsArray,0);
									tempOpponentCardsValue = myCardsValue->cardsValue(tempOpponentCardsArray,0);
				
									if(tempMyCardsValue>=tempOpponentCardsValue) countMy++;
								}
							}
						}
					}
				}
			}
		
			myOdds = 100.0*(countMy*1.0)/(countAll*1.0);

		}
		break;
		case 3: {

			// Prozent ausrechnen
	
			int i, j;
			int tempBoardCardsArray[5];
			int tempMyCardsArray[7];
			int tempOpponentCardsArray[7];
			actualBoard->getMyCards(tempBoardCardsArray);
		
			tempMyCardsArray[0] = myCards[0];
			tempMyCardsArray[1] = myCards[1];
			tempMyCardsArray[2] = tempBoardCardsArray[0];
			tempMyCardsArray[3] = tempBoardCardsArray[1];
			tempMyCardsArray[4] = tempBoardCardsArray[2];
			tempMyCardsArray[5] = tempBoardCardsArray[3];
			tempMyCardsArray[6] = tempBoardCardsArray[4];
		
			tempOpponentCardsArray[2] = tempBoardCardsArray[0];
			tempOpponentCardsArray[3] = tempBoardCardsArray[1];
			tempOpponentCardsArray[4] = tempBoardCardsArray[2];
			tempOpponentCardsArray[5] = tempBoardCardsArray[3];
			tempOpponentCardsArray[6] = tempBoardCardsArray[4];
		
			int tempMyCardsValue;
			int tempOpponentCardsValue;
		
			int countAll = 0;
			int countMy = 0;
		
			for(i=0; i<49; i++) {
				if(i != myCards[0] && i != myCards[1] && i != tempBoardCardsArray[0] && i != tempBoardCardsArray[1] && i != tempBoardCardsArray[2]) {
				for(j=i+1; j<50; j++) {
					if(j != myCards[0] && j != myCards[1] && j != tempBoardCardsArray[0] && j != tempBoardCardsArray[1] && j != tempBoardCardsArray[2]) {
		
						countAll++;
		
						tempOpponentCardsArray[0] = i;
						tempOpponentCardsArray[1] = j;
						tempMyCardsValue = myCardsValue->cardsValue(tempMyCardsArray,0);
						tempOpponentCardsValue = myCardsValue->cardsValue(tempOpponentCardsArray,0);
		
						if(tempMyCardsValue>=tempOpponentCardsValue) countMy++;
					}
				}
				}
			}
		
			myOdds = 100.0*(countMy*1.0)/(countAll*1.0);

		}
		break;
		default: cout <<"ERROR - wrong init of actualRound" << endl;


	}
}














int LocalPlayer::turnCardsValue(int* cards) {

	int array[6][3];
	int j1, j2, j3, j4, j5, k1, k2, ktemp[3];

	// Kartenwerte umwandeln (z.B. [ 11 (Karo K�ig) -> 0 11 ] oder [ 31 (Pik 7) -> 2 5 ] )
	for(j1=0; j1<6; j1++) {
		array[j1][0] = cards[j1]/13;
		array[j1][1] = cards[j1]%13;
		array[j1][2] = j1;
	}

	// Karten nach Farben sortieren: Kreuz - Pik - Herz - Karo
	for(k1=0; k1<6; k1++) {
		for(k2=k1+1; k2<6; k2++) {
			if(array[k1][0]<array[k2][0]) {
				ktemp[0] = array[k1][0];
				ktemp[1] = array[k1][1];
				ktemp[2] = array[k1][2];
				array[k1][0] = array[k2][0];
				array[k1][1] = array[k2][1];
				array[k1][2] = array[k2][2];
				array[k2][0] = ktemp[0];
				array[k2][1] = ktemp[1];
				array[k2][2] = ktemp[2];
			}
		}
	}

	// Karten innerhalb der Farben nach der Gr�e sortieren: Ass - K�ig - Dame - ... - 4 - 3 - 2
	for(k1=0; k1<6; k1++) {
		for(k2=k1+1; k2<6; k2++) {
			if(array[k1][0]==array[k2][0] && array[k1][1]<array[k2][1]) {
				ktemp[0] = array[k1][0];
				ktemp[1] = array[k1][1];
				ktemp[2] = array[k1][2];
				array[k1][0] = array[k2][0];
				array[k1][1] = array[k2][1];
				array[k1][2] = array[k2][2];
				array[k2][0] = ktemp[0];
				array[k2][1] = ktemp[1];
				array[k2][2] = ktemp[2];
			}
		}
	}

// auf Straight Flush und Flush testen	
   	// 5 Karten gleiche Farbe ?
	for(j1=0; j1<2; j1++) {
		// 5 Karten gleiche Farbe ?
		if(array[j1][0] == array[j1+1][0] && array[j1][0] == array[j1+2][0] && array[j1][0] == array[j1+3][0] && array[j1][0] == array[j1+4][0]) {
			// Straight Flush?
			if(array[j1][1]-4 == array[j1+4][1]) {
//              			cout << "Straight Flush" << endl;
             			// -> Sieg -> alles mitgehen
             			return 100;
			}
			else {
		     		// Straight Flush Ausnahme: 5-4-3-2-A
             			for(j2=j1+1; j2<3; j2++) {
					if(array[j1][1]-9==array[j2][1] && array[j2][1]-1==array[j2+1][1] && array[j2+1][1]-1==array[j2+2][1] && array[j2+2][1]-1==array[j2+3][1] && array[j1][0]==array[j2+2][0] && array[j1][0]==array[j2+3][0]) {
//                   				cout << "Straight Flush Ass unten" << endl;
                  				// -> fast sicherer Sieg -> alles mitgehen
                  				return 99;
             				}
        			}
			}
		}
	}

	// auf Flush testen
	for(j1=0; j1<2; j1++) {
		if(array[j1][0] == array[j1+1][0] && array[j1][0] == array[j1+2][0] && array[j1][0] == array[j1+3][0] && array[j1][0] == array[j1+4][0]) {
// 			cout << "Flush" << endl;
                  	// -> sehr gutes Blatt -> eigenen Anteil ermitteln und auf andere achten
                  	return 70;
		}
	}
	
	
// auf Straight Flush Draw und Flush Draw testen
   for(j1=0; j1<3; j1++) {
		// 4 Karten gleiche Farbe ?
		if(array[j1][0] == array[j1+1][0] && array[j1][0] == array[j1+2][0] && array[j1][0] == array[j1+3][0]) {
			 // zusammenhaengender Strassenansatz ?
			 if(array[j1][1]-3 == array[j1+3][1]) {
                  // Strassenansatz am Rand?
                  if(array[j1][1] == 12) {
//                        cout << "zusammenhaengender Straight-Flush-Draw mit Ass high   ";
                       break;
                  }
                  // Strassenansatz in der Mitte
                  else {
//                        cout << "zusammenhaengender Straight-Flush-Draw in der Mitte   ";
                       break;
                  }
              }
              else {
                   // Bauchschuss ?
                   if(array[j1][1]-4 == array[j1+3][1]) {
//                         cout << "Straight-Flush-Bauchschuss   ";
                        break;
                   }
                   else {
                        // Test auf Straight-Flush-Ausnahme 5-4-3-2-A
                        if(array[j1][1] == 12 && (array[j1+1][1]<=3 || (array[j1+2][1]<=3 && array[j1][0]==array[j1+4][0]) || (array[j1+3][1]<=3 && array[j1][0]==array[j1+4][0] && array[j1][0]==array[j1+4][0]))) {
//                              cout << "Straight-Flush-Draw Ass unten   ";
                             break;
					    }
                        // Flush Draw
                        else {
//                              cout << "Flush Draw   ";
                             break;
                        }
                   }
                   
              }
         }
    }
	
// Karten fr den Vierling-, Full-House-, Drilling- und Paartest umsortieren
	for(k1=0; k1<6; k1++) {
		for(k2=k1+1; k2<6; k2++) {
			if(array[k1][1]<array[k2][1]) {
				ktemp[0] = array[k1][0];
				ktemp[1] = array[k1][1];
				ktemp[2] = array[k1][2];
				array[k1][0] = array[k2][0];
				array[k1][1] = array[k2][1];
				array[k1][2] = array[k2][2];
				array[k2][0] = ktemp[0];
				array[k2][1] = ktemp[1];
				array[k2][2] = ktemp[2];
			}
		}
	}
	
// auf Vierling testen
	for(j1=0; j1<3; j1++) {
		if(array[j1][1] == array[j1+1][1] && array[j1][1] == array[j1+2][1] && array[j1][1] == array[j1+3][1]) {
//              		cout << "Vierling" << endl;
             		// -> Sieg (nur von Sraight Flush schlagbar) -> alles mitgehn
             		return 100;
        	}
	}


// auf Straight und Full House testen
	for(j1=0; j1<6; j1++) {
		for(j2=j1+1; j2<6; j2++) {
			for(j3=j2+1; j3<6; j3++) {
				for(j4=j3+1; j4<6; j4++) {
					for(j5=j4+1; j5<6; j5++) {
						// Straight
						if((array[j1][1]-1 == array[j2][1] || array[j1][1]-9 == array[j2][1] ) && array[j2][1]-1 == array[j3][1] && array[j3][1]-1 == array[j4][1] && array[j4][1]-1 == array[j5][1]) {
// 							cout << "Straight" << endl;
          						// -> super Blatt -> auf andere achten
          						return 70;
						}
						// Full House
						if((array[j1][1] == array[j2][1] && array[j1][1] == array[j3][1] && array[j4][1] == array[j5][1]) || (array[j3][1] == array[j4][1] && array[j3][1] == array[j5][1] && array[j1][1] == array[j2][1])) {
// 							cout << "Full House" << endl;
							// -> super Blatt -> auf andere achten
							return 70;	
						}
					}
				}
			}
		}
	}

// auf Straßenansatz testen
	for(j1=0; j1<6; j1++) {
		for(j2=j1+1; j2<6; j2++) {
			for(j3=j2+1; j3<6; j3++) {
				for(j4=j3+1; j4<6; j4++) {
					// zusammenhaengender Strassenansatz ?
					if(array[j1][1]-1 == array[j2][1] && array[j2][1]-1 == array[j3][1] && array[j3][1]-1 == array[j4][1]) { 
                  				// Strassenansatz am Rand?
                  				if(array[j1][1] == 12) {
// 							cout << "zusammenhaengender Straight-Draw mit Ass high   ";
                  					break;
                  				}
                  				// Strassenansatz in der Mitte
                  				else {
//                        					cout << "zusammenhaengender Straight-Draw in der Mitte   ";
                       					break;
                  				}
            				}
              				else {
                   				// Bauchschuss ?
                   				if((array[j1][1]-2 == array[j2][1] && array[j2][1]-1 == array[j3][1] && array[j3][1]-1 == array[j4][1]) || (array[j1][1]-1 == array[j2][1] && array[j2][1]-2 == array[j3][1] && array[j3][1]-1 == array[j4][1]) || (array[j1][1]-1 == array[j2][1] && array[j2][1]-1 == array[j3][1] && array[j3][1]-2 == array[j4][1])) {
//                         				cout << "Straight-Bauchschuss   ";
                        				break;
                   				}
                   				else {
                        			// Test auf Straßenansatz-Ausnahme 5-4-3-2-A
                        				if((array[j1][1]-9 == array[j2][1] && array[j2][1]-1 == array[j3][1] && array[j3][1]-1 == array[j4][1]) || (array[j1][1]-9 == array[j2][1] && array[j2][1]-1 == array[j3][1] && array[j3][1]-2 == array[j4][1]) || (array[j1][1]-9 == array[j2][1] && array[j2][1]-2 == array[j3][1] && array[j3][1]-1 == array[j4][1]) || (array[j1][1]-10 == array[j2][1] && array[j2][1]-1 == array[j3][1] && array[j3][1]-1 == array[j4][1])) {
//                              					cout << "Straight-Draw Ass unten   ";
                             					break;
					    		}
						}
					}
				}
			}
		}
	}

	 
// auf Drilling testen
	for(j1=0; j1<4; j1++) {
		if(array[j1][1] == array[j1+1][1] && array[j1][1] == array[j1+2][1]) {
//              cout << "Drilling" << endl;
             // -> gutes Blatt -> eigenen Anteil ermitteln und auf andere achten
             return 50;
		}
	}

	// auf Zwei Paare testen
	for(j1=0; j1<3; j1++) {
		for(j2=j1+2; j2<5; j2++) {
			if(array[j1][1] == array[j1+1][1] && array[j2][1] == array[j2+1][1]) {
// 				cout << "Zwei Paare" << endl;
				// -> gutes Blatt -> eigenen Anteil ermitteln und auf andere achten
				return 40;
			}
		}
	}

	// auf Paar testen
	for(j1=0; j1<5; j1++) {
		if(array[j1][1] == array[j1+1][1]) {
// 			cout << "Paar" << endl;
			// -> gutes Blatt -> eigenen Anteil ermitteln und auf andere achten
			return 30;
		}
	}

	// Highest Card (Klasse 0) + Kicker
// 	cout << endl;
	return 10;
	
} 







void LocalPlayer::preflopEngine3() {
	
	// 	cout << "nextID " << actualHand->getPlayerArray()[(myID+1)%5]->getMyID() << endl;
	
		Tools myTool;
		int raise = 0;
	
	// 	Bauchgefhl (zufÃ¯Â¿Ålig)	
		int tempRand;
		myTool.getRandNumber(1,10,1,&tempRand,0);
	
		// bluff, checkbluff
		int bluff;
		myTool.getRandNumber(1,100,1,&bluff,0);
	
	// 	cout << "preflop-bluff " << bluff << endl;
	
		// Potential
		int potential = 10*(4*(myCardsValue->holeCardsClass(myCards[0], myCards[1]))+1*tempRand)/50-myDude;
	
		int setToHighest = actualHand->getPreflop()->getHighestSet() - mySet;
	
		// temp fr das Vielfache des Small Blind, sodass HighestSet zu hoch ist
		int tempFold;
	// 	tempFold = (actualHand->getPlayerArray()[0]->getMyAverageSets())/(8*actualHand->getSmallBlind());
		myTool.getRandNumber(2,3,1,&tempFold,0);
	
		// FOLD --> wenn Potential negativ oder HighestSet zu hoch
		if( (potential*setToHighest<0 || (setToHighest > tempFold * actualHand->getSmallBlind() &&  potential<1) || (setToHighest > 2 * tempFold * actualHand->getSmallBlind() &&  potential<2) || (setToHighest > 4 * tempFold * actualHand->getSmallBlind() &&  potential<3) || (setToHighest > 10 * tempFold * actualHand->getSmallBlind() &&  potential<4))  && myCardsValue->holeCardsClass(myCards[0], myCards[1]) < 9 && bluff > 15) {
			myAction=1;
		}
		else {
			// RAISE --> wenn hohes Potential
			if(potential >= 4 && 6 * actualHand->getSmallBlind() >= actualHand->getPreflop()->getHighestSet() || bluff <= 6) {
				// extrem hohes Potential --> groÃ¯Â¿År Raise
				if(potential>=6 || bluff <= 2) {
	
					// bluff - raise
					if(bluff <=2  && 4 * actualHand->getSmallBlind() > actualHand->getPreflop()->getHighestSet()) {
						raise = 3 * actualHand->getPreflop()->getHighestSet();
					}
					else {
						// bluff - call
						if(bluff >= 98) {
							// All In
							if(actualHand->getPreflop()->getHighestSet() >= myCash) {
		
								mySet += myCash;
								myCash = 0;
								myAction = 6;
			
							}
							// sonst
							else {
								myCash = myCash - actualHand->getPreflop()->getHighestSet() + mySet;
								mySet = actualHand->getPreflop()->getHighestSet();
								myAction = 3;
							}
						} else {
							// doch nich raisen, sondern nur checken, weil highestSets bereits sehr hoch !!!
							if(! (4 * actualHand->getSmallBlind() > actualHand->getPreflop()->getHighestSet())) {
	
								// All In
								if(actualHand->getPreflop()->getHighestSet() >= myCash) {
				
									mySet += myCash;
									myCash = 0;
									myAction = 6;
				
								}
								// sonst
								else {
									myCash = myCash - actualHand->getPreflop()->getHighestSet() + mySet;
									mySet = actualHand->getPreflop()->getHighestSet();
									myAction = 3;
								}
	
							}
							else raise = (potential - 4 ) * 2 * actualHand->getPreflop()->getHighestSet();
						}
					}
				}
				// hohes Potential --> gemäßigter Raise
				else {
					// bluff - raise
					if(bluff <= 6 && 4 * actualHand->getSmallBlind() > actualHand->getPreflop()->getHighestSet()) {
						raise = 2*actualHand->getPreflop()->getHighestSet();
					}
					else {
						// bluff - call
						if(bluff >= 93) {
	
							// All In
							if(actualHand->getPreflop()->getHighestSet() >= myCash) {
			
								mySet += myCash;
								myCash = 0;
								myAction = 6;
			
							}
							// sonst
							else {
								myCash = myCash - actualHand->getPreflop()->getHighestSet() + mySet;
								mySet = actualHand->getPreflop()->getHighestSet();
								myAction = 3;
							}
						}
						else {
							// doch nich raisen, sondern nur checken, weil highestSets bereits sehr hoch !!!
							if(! (4 * actualHand->getSmallBlind() > actualHand->getPreflop()->getHighestSet())) {
	
								// All In
								if(actualHand->getPreflop()->getHighestSet() >= myCash) {
				
									mySet += myCash;
									myCash = 0;
									myAction = 6;
				
								}
								// sonst
								else {
									myCash = myCash - actualHand->getPreflop()->getHighestSet() + mySet;
									mySet = actualHand->getPreflop()->getHighestSet();
									myAction = 3;
								}
							}
							else raise = (potential - 3 ) * actualHand->getPreflop()->getHighestSet();
						}
					}
				}
		
				if (raise > 0) {
					// All In
					if(actualHand->getPreflop()->getHighestSet() + raise >= myCash) {
	
						mySet += myCash;
						myCash = 0;
						myAction = 6;
						if(mySet > actualHand->getPreflop()->getHighestSet()) actualHand->getPreflop()->setHighestSet(mySet);
	
					}
					// sonst
					else {
	
						myCash = myCash + mySet - actualHand->getPreflop()->getHighestSet() - raise;
						mySet = actualHand->getPreflop()->getHighestSet() + raise;
						actualHand->getPreflop()->setHighestSet(mySet);
						myAction = 5;
					}
				}
			}
			//CHECK und CALL
			else {
				// CHECK --> wenn alle Sets glieich bei BigBlind und nich zu hohem Potential
				if(mySet == actualHand->getPreflop()->getHighestSet()) {
					myAction = 2;
				}
				// CALL --> bei normalen Potential
				else {
					// All In
					if(actualHand->getPreflop()->getHighestSet() >= myCash) {
	
						mySet += myCash;
						myCash = 0;
						myAction = 6;
	
					}
					// sonst
					else {
						myCash = myCash - actualHand->getPreflop()->getHighestSet() + mySet;
						mySet = actualHand->getPreflop()->getHighestSet();
						myAction = 3;
					}
				}
			}
		}
}

void LocalPlayer::flopEngine3() {

		// Prozent ausrechnen
	
		int i, j, k ,l;
		int tempBoardCardsArray[5];
		int tempMyCardsArray[7];
		int tempOpponentCardsArray[7];
		actualBoard->getMyCards(tempBoardCardsArray);
	
		tempMyCardsArray[0] = myCards[0];
		tempMyCardsArray[1] = myCards[1];
		tempMyCardsArray[2] = tempBoardCardsArray[0];
		tempMyCardsArray[3] = tempBoardCardsArray[1];
		tempMyCardsArray[4] = tempBoardCardsArray[2];
	
		tempOpponentCardsArray[2] = tempBoardCardsArray[0];
		tempOpponentCardsArray[3] = tempBoardCardsArray[1];
		tempOpponentCardsArray[4] = tempBoardCardsArray[2];
	
		int tempMyCardsValue;
		int tempOpponentCardsValue;
	
		int countAll = 0;
		int countMy = 0;
	
		for(i=0; i<49; i++) {
			if(i != myCards[0] && i != myCards[1] && i != tempBoardCardsArray[0] && i != tempBoardCardsArray[1] && i != tempBoardCardsArray[2]) {
			for(j=i+1; j<50; j++) {
				if(j != myCards[0] && j != myCards[1] && j != tempBoardCardsArray[0] && j != tempBoardCardsArray[1] && j != tempBoardCardsArray[2]) {
				for(k=j+1; k<51; k++) {
					if(k != myCards[0] && k != myCards[1] && k != tempBoardCardsArray[0] && k != tempBoardCardsArray[1] && k != tempBoardCardsArray[2]) {
					for(l=k+1; l<52; l++) {
						if(l != myCards[0] && l != myCards[1] && l != tempBoardCardsArray[0] && l != tempBoardCardsArray[1] && l != tempBoardCardsArray[2]) {
	
							countAll++;
	
							tempOpponentCardsArray[0] = i;
							tempOpponentCardsArray[1] = j;
							tempOpponentCardsArray[5] = k;
							tempOpponentCardsArray[6] = l;
							tempMyCardsArray[5] = k;
							tempMyCardsArray[6] = l;
							tempMyCardsValue = myCardsValue->cardsValue(tempMyCardsArray,0);
							tempOpponentCardsValue = myCardsValue->cardsValue(tempOpponentCardsArray,0);
	
							if(tempMyCardsValue>=tempOpponentCardsValue) countMy++;
	
						}
					}
					}
				}
				}
			}
			}
		}
	
		double percent = (countMy*1.0)/(countAll*1.0);
	// 	cout << "Prozent: " << percent << endl;
	
		Tools myTool;
	
		int raise = 0;
	
	// 	Bauchgefhl (zufÃ¯Â¿Ålig)	
		int tempRand;
		myTool.getRandNumber((int)(percent*10.)-2,(int)(percent*10.)+2,1,&tempRand,0);
	
		// bluff, checkbluff
		int bluff;
		myTool.getRandNumber(1,100,1,&bluff,0);
	
	// 	cout << "flop-bluff " << bluff << endl;
	
	// 	Potential
		int potential = (10*(5*(int)(percent*100.)+10*tempRand*2))/700-myDude;
	
		int setToHighest = actualHand->getFlop()->getHighestSet() - mySet;
	
		// temp fr das Vielfache des Small Blind, sodass HighestSet zu hoch ist
		int tempFold;
	// 	tempFold = (actualHand->getPlayerArray()[0]->getMyAverageSets())/(8*actualHand->getSmallBlind());
		myTool.getRandNumber(2,3,1,&tempFold,0);
	
		// FOLD --> wenn potential negativ oder HighestSet zu hoch
		if(( potential*setToHighest<0 || (setToHighest > tempFold * actualHand->getSmallBlind() &&  potential<1) || (setToHighest > 3 * tempFold * actualHand->getSmallBlind() &&  potential<2) || (setToHighest > 9 * tempFold * actualHand->getSmallBlind() &&  potential<3) || (setToHighest > 20*tempFold * actualHand->getSmallBlind() &&  potential<4) || (setToHighest > 40 *tempFold * actualHand->getSmallBlind() &&  potential<5)) && percent < 0.90 && bluff > 18) {
			myAction=1;
		}
		else {
			// CHECK und BET --> wenn noch keiner was gesetzt hat
			if(actualHand->getFlop()->getHighestSet() == 0) {
				// CHECK --> wenn Potential klein oder check-bluff sonst bet oder bet-bluff
				if((potential<3 || bluff >= 80) && bluff > 15) {
					// check
					myAction = 2;
				}
				// BET --> wenn Potential hoch
				else {
					if(bluff <= 5) mySet = (bluff+1) * actualHand->getSmallBlind();
					else {
						if(bluff <=15 ) mySet = 4 * actualHand->getSmallBlind();
						// je höher das Potential, desto höher der Einsatz (zur Basis SmallBlind)
						else mySet = (potential-1) * 2 * actualHand->getSmallBlind();
					}
					
					// All In
					if(mySet >= myCash) {
						mySet = myCash;
						myCash = 0;
						myAction = 6;
	
					}
					// sonst
					else {
						myCash -= mySet;
						myAction = 4;
					}
					actualHand->getFlop()->setHighestSet(mySet);
				}
	
			}
			// CALL und RAISE --> wenn bereits gesetzt wurde
			else {	
				// RAISE --> wenn Potential besonders gut
				if((potential >=4 && 2 * tempFold * actualHand->getSmallBlind() >= actualHand->getFlop()->getHighestSet()) || (bluff <= 5 && 4 * tempFold * actualHand->getSmallBlind() >= actualHand->getFlop()->getHighestSet())) {
	
					// bluff - raise
					if(bluff <=5) raise = ((bluff+1)/2) * actualHand->getFlop()->getHighestSet();
					// Betrag, der ber dem aktuell HighestSet gesetzt werden soll
					else raise = ((potential - 2 ) / 2) * actualHand->getFlop()->getHighestSet();
	
					// All In
					if(actualHand->getFlop()->getHighestSet() + raise >= myCash) {
	
						mySet += myCash;
						myCash = 0;
						myAction = 6;
						if(mySet > actualHand->getFlop()->getHighestSet()) actualHand->getFlop()->setHighestSet(mySet);
	
					}
					// sonst
					else {
	
						myCash = myCash + mySet - actualHand->getFlop()->getHighestSet() - raise;
						mySet = actualHand->getFlop()->getHighestSet() + raise;
						actualHand->getFlop()->setHighestSet(mySet);
						myAction = 5;
					}
				}
				// CALL --> bei normalen Potential
				else {
	
					// All In
					if(actualHand->getFlop()->getHighestSet() >= myCash) {
	
						mySet += myCash;
						myCash = 0;
						myAction = 6;
	
					}
					// sonst
					else {
						myCash = myCash - actualHand->getFlop()->getHighestSet() + mySet;
						mySet = actualHand->getFlop()->getHighestSet();
						myAction = 3;
					}
				}
			}
		}


}

void LocalPlayer::turnEngine3() {

		// Prozent ausrechnen
	
		int i, j, k;
		int tempBoardCardsArray[5];
		int tempMyCardsArray[7];
		int tempOpponentCardsArray[7];
		actualBoard->getMyCards(tempBoardCardsArray);
	
		tempMyCardsArray[0] = myCards[0];
		tempMyCardsArray[1] = myCards[1];
		tempMyCardsArray[2] = tempBoardCardsArray[0];
		tempMyCardsArray[3] = tempBoardCardsArray[1];
		tempMyCardsArray[4] = tempBoardCardsArray[2];
		tempMyCardsArray[5] = tempBoardCardsArray[3];
	
		tempOpponentCardsArray[2] = tempBoardCardsArray[0];
		tempOpponentCardsArray[3] = tempBoardCardsArray[1];
		tempOpponentCardsArray[4] = tempBoardCardsArray[2];
		tempOpponentCardsArray[5] = tempBoardCardsArray[3];
	
		int tempMyCardsValue;
		int tempOpponentCardsValue;
	
		int countAll = 0;
		int countMy = 0;
	
		for(i=0; i<49; i++) {
			if(i != myCards[0] && i != myCards[1] && i != tempBoardCardsArray[0] && i != tempBoardCardsArray[1] && i != tempBoardCardsArray[2]) {
			for(j=i+1; j<50; j++) {
				if(j != myCards[0] && j != myCards[1] && j != tempBoardCardsArray[0] && j != tempBoardCardsArray[1] && j != tempBoardCardsArray[2]) {
				for(k=j+1; k<51; k++) {
					if(k != myCards[0] && k != myCards[1] && k != tempBoardCardsArray[0] && k != tempBoardCardsArray[1] && k != tempBoardCardsArray[2]) {
	
						countAll++;
	
						tempOpponentCardsArray[0] = i;
						tempOpponentCardsArray[1] = j;
						tempOpponentCardsArray[6] = k;
						tempMyCardsArray[6] = k;
						tempMyCardsValue = myCardsValue->cardsValue(tempMyCardsArray,0);
						tempOpponentCardsValue = myCardsValue->cardsValue(tempOpponentCardsArray,0);
	
						if(tempMyCardsValue>=tempOpponentCardsValue) countMy++;
					}
				}
				}
			}
			}
		}
	
		double percent = (countMy*1.0)/(countAll*1.0);
	// 	cout << "Prozent: " << percent << endl;
	
		Tools myTool;
		int raise;
	
	// 	Bauchgefhl (zufÃ¯Â¿Ålig)	
		int tempRand;
		myTool.getRandNumber((int)(percent*10.)-2,(int)(percent*10.)+2,1,&tempRand,0);
	
		// bluff, checkbluff
		int bluff;
		myTool.getRandNumber(1,100,1,&bluff,0);
	
	// 	cout << "turn-bluff " << bluff << endl;
	
	// 	Potential
		int potential = (10*(5*(int)(percent*100.)+10*tempRand*2))/700-myDude;
	
		int setToHighest = actualHand->getTurn()->getHighestSet() - mySet;
	
		// temp fr das Vielfache des Small Blind, sodass HighestSet zu hoch ist
		int tempFold;
	// 	tempFold = (actualHand->getPlayerArray()[0]->getMyAverageSets())/(7*actualHand->getSmallBlind());
		myTool.getRandNumber(3,4,1,&tempFold,0);
	
		// FOLD
		// --> wenn potential negativ oder HighestSet zu hoch
		if( (potential*setToHighest<0 || (setToHighest > tempFold * actualHand->getSmallBlind() &&  potential<1) || (setToHighest > 3 * tempFold * actualHand->getSmallBlind() &&  potential<2) || (setToHighest > 9 * tempFold * actualHand->getSmallBlind() &&  potential<3) || (setToHighest > 20*tempFold * actualHand->getSmallBlind() &&  potential<4) || (setToHighest > 40 *tempFold * actualHand->getSmallBlind() &&  potential<5)) && percent < 0.90 && bluff > 15) {
			myAction=1;
		}
		else {
			// CHECK und BET --> wenn noch keiner was gesetzt hat
			if(actualHand->getTurn()->getHighestSet() == 0) {
				// CHECK --> wenn Potential klein
				if((potential<2 || bluff >= 80) && bluff > 10) {
					// check
					myAction = 2;
				}
				// BET --> wenn Potential hoch
				else {
	
					if(bluff <= 3) mySet = bluff * 2 * actualHand->getSmallBlind();
					else {
						if(bluff <=10 ) mySet = ((bluff+2)/3) * actualHand->getSmallBlind();
						// je hÃ¯Â¿Åer das Potential, desto hÃ¯Â¿Åher der Einsatz (zur Basis SmallBlind)
						else mySet = (potential-1) * 3 * actualHand->getSmallBlind();
					}
	
					// All In
					if(mySet >= myCash) {
						mySet = myCash;
						myCash = 0;
						myAction = 6;
	
					}
					// sonst
					else {
						myCash -= mySet;
						myAction = 4;
					}
					actualHand->getTurn()->setHighestSet(mySet);
				}
	
			}
			// CALL und RAISE --> wenn bereits gesetzt wurde
			else {	
				// RAISE --> wenn Potential besonders gut
				if(potential >=4 && 2 * tempFold * actualHand->getSmallBlind() >= actualHand->getTurn()->getHighestSet() || (bluff <= 4 && 3 * tempFold * actualHand->getSmallBlind() >= actualHand->getTurn()->getHighestSet())) {
	
					// bluff - raise
					if(bluff <= 4) raise = ((bluff+1)/2) * actualHand->getTurn()->getHighestSet();
					// Betrag, der ber dem aktuell HighestSet gesetzt werden soll
					else raise = ( potential - 3 ) * actualHand->getTurn()->getHighestSet();
	
					// All In
					if(actualHand->getTurn()->getHighestSet() + raise >= myCash) {
	
						mySet += myCash;
						myCash = 0;
						myAction = 6;
						if(mySet > actualHand->getTurn()->getHighestSet()) actualHand->getTurn()->setHighestSet(mySet);
	
					}
					// sonst
					else {
	
						myCash = myCash + mySet - actualHand->getTurn()->getHighestSet() - raise;
						mySet = actualHand->getTurn()->getHighestSet() + raise;
						actualHand->getTurn()->setHighestSet(mySet);
						myAction = 5;
					}
				}
				// CALL --> bei normalen Potential
				else {
					// All In
					if(actualHand->getTurn()->getHighestSet() >= myCash) {
	
						mySet += myCash;
						myCash = 0;
						myAction = 6;
	
					}
					// sonst
					else {
						myCash = myCash - actualHand->getTurn()->getHighestSet() + mySet;
						mySet = actualHand->getTurn()->getHighestSet();
						myAction = 3;
					}
				}
			}
		}	

}

void LocalPlayer::riverEngine3() {

		// Prozent ausrechnen
	
		int i, j;
		int tempBoardCardsArray[5];
		int tempMyCardsArray[7];
		int tempOpponentCardsArray[7];
		actualBoard->getMyCards(tempBoardCardsArray);
	
		tempMyCardsArray[0] = myCards[0];
		tempMyCardsArray[1] = myCards[1];
		tempMyCardsArray[2] = tempBoardCardsArray[0];
		tempMyCardsArray[3] = tempBoardCardsArray[1];
		tempMyCardsArray[4] = tempBoardCardsArray[2];
		tempMyCardsArray[5] = tempBoardCardsArray[3];
		tempMyCardsArray[6] = tempBoardCardsArray[4];
	
		tempOpponentCardsArray[2] = tempBoardCardsArray[0];
		tempOpponentCardsArray[3] = tempBoardCardsArray[1];
		tempOpponentCardsArray[4] = tempBoardCardsArray[2];
		tempOpponentCardsArray[5] = tempBoardCardsArray[3];
		tempOpponentCardsArray[6] = tempBoardCardsArray[4];
	
		int tempMyCardsValue;
		int tempOpponentCardsValue;
	
		int countAll = 0;
		int countMy = 0;
	
		for(i=0; i<49; i++) {
			if(i != myCards[0] && i != myCards[1] && i != tempBoardCardsArray[0] && i != tempBoardCardsArray[1] && i != tempBoardCardsArray[2]) {
			for(j=i+1; j<50; j++) {
				if(j != myCards[0] && j != myCards[1] && j != tempBoardCardsArray[0] && j != tempBoardCardsArray[1] && j != tempBoardCardsArray[2]) {
	
					countAll++;
	
					tempOpponentCardsArray[0] = i;
					tempOpponentCardsArray[1] = j;
					tempMyCardsValue = myCardsValue->cardsValue(tempMyCardsArray,0);
					tempOpponentCardsValue = myCardsValue->cardsValue(tempOpponentCardsArray,0);
	
					if(tempMyCardsValue>=tempOpponentCardsValue) countMy++;
				}
			}
			}
		}
	
		double percent = (countMy*1.0)/(countAll*1.0);
	// 	cout << "Prozent: " << percent << endl;
	
		Tools myTool;
		int raise;
	
	// 	Bauchgefhl (zufÃ¯Â¿Ålig)	
		int tempRand;
		myTool.getRandNumber((int)(percent*10.)-2,(int)(percent*10.)+2,1,&tempRand,0);
	
		// bluff, checkbluff
		int bluff;
		myTool.getRandNumber(1,100,1,&bluff,0);
	
	// 	cout << "river-bluff " << bluff << endl;
	
	// 	Potential
		int potential = (10*(5*(int)(percent*100.)+10*tempRand*1))/600-myDude;
	
		int setToHighest = actualHand->getRiver()->getHighestSet() - mySet;
	
		// temp fr das Vielfache des Small Blind, sodass HighestSet zu hoch ist
		int tempFold;
		tempFold = (actualHand->getPlayerArray()[0]->getMyAverageSets())/(6*actualHand->getSmallBlind());
		// myTool.getRandNumber(4,6,1,&tempFold,0);
	
		// FOLD
		// --> wenn potential negativ oder HighestSet zu hoch
		if( (potential*setToHighest<0 || (setToHighest > tempFold * actualHand->getSmallBlind() &&  potential<1) || (setToHighest > 3 * tempFold * actualHand->getSmallBlind() &&  potential<2) || (setToHighest > 9 * tempFold * actualHand->getSmallBlind() &&  potential<3) || (setToHighest > 20*tempFold * actualHand->getSmallBlind() &&  potential<4) || (setToHighest > 40 *tempFold * actualHand->getSmallBlind() &&  potential<5)) && percent < 0.90 && bluff > 15) {
			myAction=1;
		}
		else {
			// CHECK und BET --> wenn noch keiner was gesetzt hat
			if(actualHand->getRiver()->getHighestSet() == 0) {
				// CHECK --> wenn Potential klein
				if((potential<2 || bluff >= 92) && bluff > 15) {
					// check
					myAction = 2;
				}
				// BET --> wenn Potential hoch
				else {
	
					if(bluff <= 5) mySet = (bluff+3) * actualHand->getSmallBlind();
					else {
						if(bluff <= 15 ) mySet = ((bluff-1)/5) * 2 * actualHand->getSmallBlind();
						// je hÃ¯Â¿Åer das Potential, desto hÃ¯Â¿Åher der Einsatz (zur Basis SmallBlind)
						else mySet = (potential-1) * 4 * actualHand->getSmallBlind();
					}
	
					// All In
					if(mySet >= myCash) {
						mySet = myCash;
						myCash = 0;
						myAction = 6;
	
					}
					// sonst
					else {
						myCash -= mySet;
						myAction = 4;
					}
					actualHand->getRiver()->setHighestSet(mySet);
				}
	
			}
			// CALL und RAISE --> wenn bereits gesetzt wurde
			else {	
				// RAISE --> wenn Potential besonders gut
				if(potential >=4 && 2 * tempFold * actualHand->getSmallBlind() >= actualHand->getRiver()->getHighestSet() || (bluff <= 2 && 4 * tempFold * actualHand->getSmallBlind() >= actualHand->getRiver()->getHighestSet())) {
	
					// bluff - raise
					if(bluff <= 2 ) raise = bluff * actualHand->getRiver()->getHighestSet();
					// Betrag, der ber dem aktuell HighestSet gesetzt werden soll
					else raise = ( potential - 3 ) * actualHand->getRiver()->getHighestSet();
	
					// All In
					if(actualHand->getRiver()->getHighestSet() + raise >= myCash) {
	
						mySet += myCash;
						myCash = 0;
						myAction = 6;
						if(mySet > actualHand->getRiver()->getHighestSet()) actualHand->getRiver()->setHighestSet(mySet);
	
					}
					// sonst
					else {
	
						myCash = myCash + mySet - actualHand->getRiver()->getHighestSet() - raise;
						mySet = actualHand->getRiver()->getHighestSet() + raise;
						actualHand->getRiver()->setHighestSet(mySet);
						myAction = 5;
					}
				}
				// CALL --> bei normalen Potential
				else {
					// All In
					if(actualHand->getRiver()->getHighestSet() >= myCash) {
	
						mySet += myCash;
						myCash = 0;
						myAction = 6;
	
					}
					// sonst
					else {
						myCash = myCash - actualHand->getRiver()->getHighestSet() + mySet;
						mySet = actualHand->getRiver()->getHighestSet();
						myAction = 3;
					}
				}
			}
		}

}

