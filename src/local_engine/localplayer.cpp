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

LocalPlayer::LocalPlayer(LocalBoard *b, int id, int sC, bool aS, int mB) : actualHand(0), actualBoard(b), myCardsValue(0), myID(id), myDude(0), myCardsValueInt(0), myCash(sC), mySet(0), myAction(0), myButton(mB), myActiveStatus(aS), myTurn(0), myRoundStartCash(0), myAverageSets(0)
{

	Tools myTool;
	// Dude zuweisen
	myTool.getRandNumber(3 , 5, 1, &myDude, 0);
// 	cout << "Spieler: " << myID << " Dude: " << myDude << " Cash: " << myCash << " ActiveStatus: " << myActiveStatus << " Button: " << myButton << endl;

	myCardsValue = new CardsValue;

	ostringstream temp;
	temp << "Player " << myID;
	myName = temp.str();

}


LocalPlayer::~LocalPlayer()
{
}

void LocalPlayer::setHand(LocalHand* br) { actualHand = br; }

void LocalPlayer::action() {

	switch(actualHand->getActualRound()) {
		case 0: {

			preflopEngine();

			actualBoard->collectSets();
			actualHand->getMainWindowImpl()->refreshPot();

		} break;
		case 1: {

			flopEngine();

			actualBoard->collectSets();
			actualHand->getMainWindowImpl()->refreshPot();

		} break;
		case 2: {

			turnEngine();

			actualBoard->collectSets();
			actualHand->getMainWindowImpl()->refreshPot();

		} break;
		case 3: {

			riverEngine();

			actualBoard->collectSets();
			actualHand->getMainWindowImpl()->refreshPot();

		} break;
		default: {}
	}
	


	myTurn = 0;
// 	cout << "jetzt" << endl;
	
	actualHand->getGuiInterface()->showPlayerActionLogMsg(myName, myAction, mySet);
	actualHand->getGuiInterface()->nextPlayerAnimation();


}

void LocalPlayer::preflopEngine() {

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

void LocalPlayer::flopEngine() {

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
						tempMyCardsValue = myCardsValue->cardsValue(tempMyCardsArray);
						tempOpponentCardsValue = myCardsValue->cardsValue(tempOpponentCardsArray);

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

void LocalPlayer::turnEngine() {

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
					tempMyCardsValue = myCardsValue->cardsValue(tempMyCardsArray);
					tempOpponentCardsValue = myCardsValue->cardsValue(tempOpponentCardsArray);

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

void LocalPlayer::riverEngine() {

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
				tempMyCardsValue = myCardsValue->cardsValue(tempMyCardsArray);
				tempOpponentCardsValue = myCardsValue->cardsValue(tempOpponentCardsArray);

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
