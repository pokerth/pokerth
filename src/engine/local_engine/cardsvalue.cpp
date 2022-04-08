/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/

#include "cardsvalue.h"
#include "arraydata.h"
#include "tools.h"
#include "playerinterface.h"

#include <list>
#include <boost/shared_ptr.hpp>

int CardsValue::holeCardsClass(int firstCard, int secondCard)
{
	int firstCardMinus1 = firstCard - 1;
	int secondCardMinus1 = secondCard - 1;
	int firstCardMinus1Value = firstCardMinus1%13;
	int secondCardMinus1Value = secondCardMinus1%13;
	int firstCardMinus1Suit = firstCardMinus1/13;
	int secondCardMinus1Suit = secondCardMinus1/13;

	if(firstCardMinus1Value<secondCardMinus1Value) { // if cards are in ascending order
		// order them by value
		int temp = firstCard;
		firstCard = secondCard;
		secondCard = temp;
	}


	if(firstCardMinus1Value == secondCardMinus1Value) {
		if(firstCardMinus1Value+2 > 10) return 10;
		else {
			switch(firstCardMinus1Value+2) {
			case 10:
				return 9;
			case 9:
				return 8;
			case 8:
				return 7;
			case 7:
				return 6;
			default:
				return 5;
			}
		}
	}
	switch(firstCardMinus1Value+2) {
	//Ass
	case 14: {
		if(firstCardMinus1Suit == secondCardMinus1Suit) {
			switch(firstCardMinus1Value-secondCardMinus1Value) {
			case 1:
				return 10;
			case 2:
				return 9;
			case 3:
				return 9;
			case 4:
				return 8;
			default:
				return 7;
			}
		} else {
			switch(firstCardMinus1Value-secondCardMinus1Value) {
			case 1:
				return 9;
			case 2:
				return 8;
			case 3:
				return 7;
			case 4:
				return 7;
			default:
				return 4;
			}
		}
	}
	break;
	//K�ig
	case 13: {
		if(firstCardMinus1Suit == secondCardMinus1Suit) {
			switch(firstCardMinus1Value-secondCardMinus1Value) {
			case 1:
				return 9;
			case 2:
				return 8;
			case 3:
				return 8;
			case 4:
				return 6;
			default:
				return 5;
			}
		} else {
			switch(firstCardMinus1Value-secondCardMinus1Value) {
			case 1:
				return 7;
			case 2:
				return 6;
			case 3:
				return 6;
			default:
				return 4;
			}
		}
	}
	break;
	//Dame
	case 12: {
		if(firstCardMinus1Suit == secondCardMinus1Suit) {
			switch(firstCardMinus1Value-secondCardMinus1Value) {
			case 1:
				return 8;
			case 2:
				return 7;
			case 3:
				return 6;
			case 4:
				return 5;
			default:
				return 4;
			}
		} else {
			switch(firstCardMinus1Value-secondCardMinus1Value) {
			case 1:
				return 6;
			case 2:
				return 6;
			case 3:
				return 4;
			default:
				return 3;
			}
		}
	}
	break;
	//Bube
	case 11: {
		if(firstCardMinus1Suit == secondCardMinus1Suit) {
			switch(firstCardMinus1Value-secondCardMinus1Value) {
			case 1:
				return 7;
			case 2:
				return 6;
			case 3:
				return 5;
			case 4:
				return 4;
			default:
				return 3;
			}
		} else {
			switch(firstCardMinus1Value-secondCardMinus1Value) {
			case 1:
				return 6;
			case 2:
				return 5;
			case 3:
				return 4;
			default:
				return 2;
			}
		}
	}
	break;
	//10
	case 10: {
		if(firstCardMinus1Suit == secondCardMinus1Suit) {
			switch(firstCardMinus1Value-secondCardMinus1Value) {
			case 1:
				return 6;
			case 2:
				return 5;
			default:
				return 2;
			}
		} else {
			switch(firstCardMinus1Value-secondCardMinus1Value) {
			case 1:
				return 5;
			case 2:
				return 4;
			default:
				return 1;
			}
		}
	}
	break;
	//Rest
	default: {
		if(firstCardMinus1Value - secondCardMinus1Value <= 2) {
			if(firstCardMinus1Suit == secondCardMinus1Suit) return 5;
			else return 3;
		} else {
			if(firstCardMinus1Value - secondCardMinus1Value == 3) return 2;
			else return 1;
		}
	}
	}


}

int CardsValue::holeCardsToIntCode(int* cards)
{
	int firstCard = cards[0];
	int secondCard = cards[1];
	int firstCardValue = firstCard%13;
	int secondCardValue = secondCard%13;
	int firstCardSuit = firstCard/13;
	int secondCardSuit = secondCard/13;
	// Code der HoleCards ermitteln
	if(firstCardValue == secondCardValue) { // If cards have same value (%13 is the value in a color)
		return ((firstCardValue)*1000 + (firstCardValue)*10); //Example : 3030 for 42 and 16
	} else {
		if(firstCardValue < secondCardValue) { // If first card is inferior
			if(firstCardSuit == secondCardSuit) { // If cards are in same color
				return ((firstCardValue)*1000 + (secondCardValue)*10 + 1); // Example : 3011 for 42 and 10
			} else {
				return ((firstCardValue)*1000 + (secondCardValue)*10); // Example : 3010 for 42 and 10
			}
		} else { // If second card is inferior
			if(firstCardSuit == secondCardSuit) { // If cards are in same color
				return ((secondCardValue)*1000 + (firstCardValue)*10 + 1); // Example : 1031 for 10 and 42
			} else {
				return ((secondCardValue)*1000 + (firstCardValue)*10); // Example : 1030 for 10 and 42
			}
		}
	}

}

/* DO NOT USE, THIS MAY LEAK MEMORY
int* CardsValue::intCodeToHoleCards(int code)
{

	// one possibility !!!

	int* cards = new int[2];

	cards[0] = code/1000;
	cards[1] = (code-cards[0]*1000)/10;

	if(cards[0]==cards[1]) {
		cards[1] +=13;
	} else {
		if(code%10 == 0) cards[1] +=13;
	}

	return cards;

}*/

int CardsValue::cardsValue(int* cards, int* position)
{

	int array[7][3];
	int j1, j2, j3, j4, j5, k1, k2, ktemp[3];

	int cucumbersCount = 0;
	// Kartenwerte umwandeln (z.B. [ 11 (Karo K�ig) -> 0 11 ] oder [ 31 (Pik 7) -> 2 5 ] )
	for(j1=0; j1<7; j1++) {
		array[j1][0] = cards[j1]/13; // Suit
		array[j1][1] = cards[j1]%13; // Value
		array[j1][2] = j1;
		if (array[j1][0] == 4) {
			cucumbersCount += 1;
		}
	}

	// Cucumber's pair
	if (cucumbersCount ==2) {
		return 1000000000;
	}
	// Karten nach Farben sortieren: Kreuz - Pik - Herz - Karo
	for(k1=0; k1<7; k1++) {
		for(k2=k1+1; k2<7; k2++) {
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
	for(k1=0; k1<7; k1++) {
		for(k2=k1+1; k2<7; k2++) {
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

	// Karten auf Bl�ter testen. Klasseneinteilung absteigend: 9 - Royal Flush, 8 - Straight Flush, ... 2 - Zwei Paare, 1 - Ein Paar, 0 - Nischt

	// auf Royal Flush (Klasse 9) und Straight Flush (Klasse 8) testen
	for(j1=0; j1<3; j1++) {
		// 5 Karten gleiche Farbe ?
		if(array[j1][0] == array[j1+1][0] && array[j1][0] == array[j1+2][0] && array[j1][0] == array[j1+3][0] && array[j1][0] == array[j1+4][0]) {
			// zus�zlich in Stra�nform ?
			if(array[j1][1]-1 == array[j1+1][1] && array[j1+1][1]-1 == array[j1+2][1] && array[j1+2][1]-1 == array[j1+3][1] && array[j1+3][1]-1 == array[j1+4][1]) {
				// mit Ass an der Spitze ?
				if(array[j1][1] == 12) {
					// Royal Flush (9*100000000)
					if(position) {
						// Position-Array fuellen
						for(j2=0; j2<5; j2++) {
							position[j2] = array[j1+j2][2];
						}
					}
					return 900000000;
				}
				// sonst nur Straight Flush (8*100000000 + (h�hste Straight-Karte)*1000000)
				else {
					if(position) {
						// Position-Array fuellen
						for(j2=0; j2<5; j2++) {
							position[j2] = array[j1+j2][2];
						}
					}
					return 800000000+array[j1][1]*1000000;
				}
			}
		}
	}

	// Straight Flush Ausnahme: 5-4-3-2-A
	for(j1=0; j1<3; j1++) {
		// 5 Karten gleiche Farbe ?
		if(array[j1][0] == array[j1+1][0] && array[j1][0] == array[j1+2][0] && array[j1][0] == array[j1+3][0] && array[j1][0] == array[j1+4][0]) {
			for(j2=j1+1; j2<4; j2++) {
				if(array[j1][1]-9==array[j2][1] && array[j2][1]-1==array[j2+1][1] && array[j2+1][1]-1==array[j2+2][1] && array[j2+2][1]-1==array[j2+3][1] && array[j1][0]==array[j2+2][0] && array[j1][0]==array[j2+3][0]) {
					// Straight Flush mit 5 als höchste Karte -> 8*100000000+3*1000000
					if(position) {
						// Position-Array fuellen
						position[0] = array[j1][2];
						for(j3=0; j3<4; j3++) {
							position[j3+1] = array[j2+j3][2];
						}
					}
					return 800000000+3*1000000;
				}
			}
		}
	}

	// auf Flush (Klasse 5) testen
	for(j1=0; j1<3; j1++) {
		if(array[j1][0] == array[j1+1][0] && array[j1][0] == array[j1+2][0] && array[j1][0] == array[j1+3][0] && array[j1][0] == array[j1+4][0]) {
			// Flush -> 5*10000000 + h�ste Flush Karten mit absteigender Wertung
			if(position) {
				// Position-Array fuellen
				for(j2=0; j2<5; j2++) {
					position[j2] = array[j1+j2][2];
				}
			}
			return 500000000+array[j1][1]*1000000+array[j1+1][1]*10000+array[j1+2][1]*100+array[j1+3][1]*10+array[j1+4][1];
		}
	}



	// Karten fr den Vierling-, Full-House-, Drilling- und Paartest umsortieren
	for(k1=0; k1<7; k1++) {
		for(k2=k1+1; k2<7; k2++) {
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

	// nach Position sortieren: erst board, dann hole cards
	for(k1=0; k1<7; k1++) {
		for(k2=k1+1; k2<7; k2++) {
			if(array[k1][1]==array[k2][1] && array[k1][2]<array[k2][2]) {
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

	// auf Vierling (Klasse 7) testen
	for(j1=0; j1<4; j1++) {
		if(array[j1][1] == array[j1+1][1] && array[j1][1] == array[j1+2][1] && array[j1][1] == array[j1+3][1]) {
			// Position des Kickers ermitteln und der Blattwertung als dritte Gewichtung hinzuaddieren
			if(j1==0) {
				if(position) {
					// Position-Array fuellen
					for(j2=0; j2<5; j2++) {
						position[j2] = array[j2][2];
					}
				}
				return 700000000+array[j1][1]*1000000+array[j1+4][1]*10000;
			} else {
				if(position) {
					// Position-Array fuellen
					for(j2=0; j2<4; j2++) {
						position[j2] = array[j1+j2][2];
					}
					position[4] = array[0][2];
				}
				return 700000000+array[j1][1]*1000000+array[0][1]*10000;
			}
		}
	}

	// Hilfsvariablen fr die Full-House-Paar- und -Drilling-Zuordnung
	int drei, zwei;

	// auf Straight (Klasse 4) und Full House (Klasse 6) testen
	for(j1=0; j1<7; j1++) {
		for(j2=j1+1; j2<7; j2++) {
			for(j3=j2+1; j3<7; j3++) {
				for(j4=j3+1; j4<7; j4++) {
					for(j5=j4+1; j5<7; j5++) {
						// Straight
						if(array[j1][1]-1 == array[j2][1] && array[j2][1]-1 == array[j3][1] && array[j3][1]-1 == array[j4][1] && array[j4][1]-1 == array[j5][1]) {
							if(position) {
								// Position-Array fuellen
								position[0] = array[j1][2];
								position[1] = array[j2][2];
								position[2] = array[j3][2];
								position[3] = array[j4][2];
								position[4] = array[j5][2];
							}
							return 400000000+array[j1][1]*1000000;
						}
						// Full House
						if((array[j1][1] == array[j2][1] && array[j1][1] == array[j3][1] && array[j4][1] == array[j5][1]) || (array[j3][1] == array[j4][1] && array[j3][1] == array[j5][1] && array[j1][1] == array[j2][1])) {
							if(position) {
								// Position-Array fuellen
								position[0] = array[j1][2];
								position[1] = array[j2][2];
								position[2] = array[j3][2];
								position[3] = array[j4][2];
								position[4] = array[j5][2];
							}
							// Paar und Drilling des Full House ermitteln ermitteln
							if(array[j3][1]==array[j1][1]) {
								drei = array[j1][1];
								zwei = array[j4][1];
							} else {
								drei = array[j4][1];
								zwei = array[j1][1];
							}
							return 600000000+drei*1000000+zwei*10000;
						}
					}
				}
			}
		}
	}

	// auf Straight-Spezialfall ( 5 4 3 2 A ) testen
	for(j1=0; j1<7; j1++) {
		for(j2=j1+1; j2<7; j2++) {
			for(j3=j2+1; j3<7; j3++) {
				for(j4=j3+1; j4<7; j4++) {
					for(j5=j4+1; j5<7; j5++) {
						if(array[j1][1]-9 == array[j2][1] && array[j2][1]-1 == array[j3][1] && array[j3][1]-1 == array[j4][1] && array[j4][1]-1 == array[j5][1]) {
							if(position) {
								// Position-Array fuellen
								position[0] = array[j1][2];
								position[1] = array[j2][2];
								position[2] = array[j3][2];
								position[3] = array[j4][2];
								position[4] = array[j5][2];
							}
							return 400000000+array[j2][1]*1000000;
						}
					}
				}
			}
		}
	}

	// auf Drilling (Klasse 3) testen
	for(j1=0; j1<5; j1++) {
		if(array[j1][1] == array[j1+1][1] && array[j1][1] == array[j1+2][1]) {
			// Kicker ermitteln
			if(j1==0) {
				if(position) {
					// Position-Array fuellen
					for(j2=0; j2<5; j2++) {
						position[j2] = array[j2][2];
					}
				}
				return 300000000+array[j1][1]*1000000+array[j1+3][1]*10000+array[j1+4][1]*100;
			} else {
				if(j1==1) {
					if(position) {
						// Position-Array fuellen
						for(j2=0; j2<5; j2++) {
							position[j2] = array[j2][2];
						}
					}
					return 300000000+array[j1][1]*1000000+array[j1-1][1]*10000+array[j1+3][1]*100;
				} else {
					if(position) {
						// Position-Array fuellen
						for(j2=0; j2<3; j2++) {
							position[j2] = array[j1+j2][2];
						}
						position[3] = array[0][2];
						position[4] = array[1][2];
					}
					return 300000000+array[j1][1]*1000000+array[0][1]*10000+array[1][1]*100;
				}
			}
		}
	}

	// auf Zwei Paare (Klasse 2) testen
	for(j1=0; j1<4; j1++) {
		for(j2=j1+2; j2<6; j2++) {
			if(array[j1][1] == array[j1+1][1] && array[j2][1] == array[j2+1][1]) {
				// Kicker ermitteln
				if(j1==0) {
					if(j2==2) {
						if(position) {
							// Position-Array fuellen
							position[0] = array[j1][2];
							position[1] = array[j1+1][2];
							position[2] = array[j2][2];
							position[3] = array[j2+1][2];
							position[4] = array[j2+2][2];
						}
						return 200000000+array[j1][1]*1000000+array[j2][1]*10000+array[j2+2][1]*100;
					} else {
						if(position) {
							// Position-Array fuellen
							position[0] = array[j1][2];
							position[1] = array[j1+1][2];
							position[2] = array[j2][2];
							position[3] = array[j2+1][2];
							position[4] = array[j1+2][2];
						}
						return 200000000+array[j1][1]*1000000+array[j2][1]*10000+array[j1+2][1]*100;
					}
				} else {
					if(position) {
						// Position-Array fuellen
						position[0] = array[j1][2];
						position[1] = array[j1+1][2];
						position[2] = array[j2][2];
						position[3] = array[j2+1][2];
						position[4] = array[0][2];
					}
					return 200000000+array[j1][1]*1000000+array[j2][1]*10000+array[0][1]*100;
				}
			}
		}
	}

	// auf Paar (Klasse 1) testen
	for(j1=0; j1<6; j1++) {
		if(array[j1][1] == array[j1+1][1]) {
			// Kicker ermitteln
			if(j1==0) {
				if(position) {
					// Position-Array fuellen
					for(j2=0; j2<5; j2++) {
						position[j2] = array[j2][2];
					}
				}
				return 100000000+array[j1][1]*1000000+array[j1+2][1]*10000+array[j1+3][1]*100+array[j1+4][1];
			}
			if(j1==1) {
				if(position) {
					// Position-Array fuellen
					for(j2=0; j2<5; j2++) {
						position[j2] = array[j2][2];
					}
				}
				return 100000000+array[j1][1]*1000000+array[j1-1][1]*10000+array[j1+2][1]*100+array[j1+3][1];
			}
			if(j1==2) {
				if(position) {
					// Position-Array fuellen
					for(j2=0; j2<5; j2++) {
						position[j2] = array[j2][2];
					}
				}
				return 100000000+array[j1][1]*1000000+array[j1-2][1]*10000+array[j1-1][1]*100+array[j1+2][1];
			} else {
				if(position) {
					// Position-Array fuellen
					for(j2=0; j2<2; j2++) {
						position[j2] = array[j1+j2][2];
					}
					position[2] = array[0][2];
					position[3] = array[1][2];
					position[4] = array[2][2];
				}
				return 100000000+array[j1][1]*1000000+array[0][1]*10000+array[1][1]*100+array[2][1];
			}
		}
	}

	// Highest Card (Klasse 0) + Kicker
	if(position) {
		// Position-Array fuellen
		for(j2=0; j2<5; j2++) {
			position[j2] = array[j2][2];
		}
	}
	return array[0][1]*1000000+array[1][1]*10000+array[2][1]*100+array[3][1]*10+array[4][1];
}


std::vector< std::vector<int> > CardsValue::calcCardsChance(GameState beRoID, int* playerCards, int* boardCards)
{
	int i,j;

	std::vector< std::vector<int> > chance(2);

	chance[0].resize(10);
	chance[1].resize(10);

	for(i=0; i<10; i++) {
		chance[0][i] = 0;
		chance[1][i] = 0;
	}

	int cards[7];
	int sum = 0;

	cards[0] = playerCards[0];
	cards[1] = playerCards[1];
	for(i=0; i<5; i++) cards[i+2] = boardCards[i];

	switch(beRoID) {
	case GAME_STATE_PREFLOP: {

		chance = ArrayData::getHandChancePreflop(holeCardsToIntCode(playerCards));

	}
	break;
	case GAME_STATE_FLOP: {

		for(i=0; i<51; i++) {
			if(i!=cards[0] && i!=cards[1] && i!=cards[2] && i!=cards[3] && i!=cards[4]) {
				for(j=i+1; j<52; j++) {
					if(j!=cards[0] && j!=cards[1] && j!=cards[2] && j!=cards[3] && j!=cards[4]) {
						cards[5] = i;
						cards[6] = j;
						(chance[0][cardsValue(cards,0)/100000000])++;
						sum++;
					}
				}
			}
		}
		for(i=0; i<10; i++) {
			if(chance[0][i] > 0) chance[1][i] = 1;
			chance[0][i] = (int)(((double)chance[0][i]/(double)sum)*100.0+0.5);
		}

	}
	break;
	case GAME_STATE_TURN: {

		for(i=0; i<52; i++) {
			if(i!=cards[0] && i!=cards[1] && i!=cards[2] && i!=cards[3] && i!=cards[4] && i!=cards[5]) {
				cards[6] = i;
				(chance[0][cardsValue(cards,0)/100000000])++;
				sum++;
			}
		}
		for(i=0; i<10; i++) {
			if(chance[0][i] > 0) chance[1][i] = 1;
			chance[0][i] = (int)(((double)chance[0][i]/(double)sum)*100.0+0.5);
		}

	}
	break;
	case GAME_STATE_RIVER: {
		chance[0][cardsValue(cards,0)/100000000] = 100;
		chance[1][cardsValue(cards,0)/100000000] = 1;
	}
	break;
	default: {
	}
	}

	return chance;
}

std::string CardsValue::determineHandName(int myCardsValueInt, PlayerList activePlayerList)
{

	std::list<int> shownCardsValueInt;
	std::list<int> sameHandCardsValueInt;
	bool different = false;
	bool equal = false;
//	boost::shared_ptr<Game> currentGame = myW->getSession()->getCurrentGame();
	PlayerListConstIterator it_c;
//	PlayerList activePlayerList = currentGame->getActivePlayerList();

	// collect cardsValueInt of all players who will show their cards
	for(it_c=activePlayerList->begin(); it_c!=activePlayerList->end(); ++it_c) {

		if( (*it_c)->getMyAction() != PLAYER_ACTION_FOLD) {
			shownCardsValueInt.push_back( (*it_c)->getMyCardsValueInt());
		}
	}

	// erase own cardsValueInt
	std::list<int>::iterator it;
	for(it = shownCardsValueInt.begin(); it != shownCardsValueInt.end(); ++it) {
		if((*it) == myCardsValueInt) {
			shownCardsValueInt.erase(it);
			break;
		}
	}

	std::list<std::string> cardString = translateCardsValueCode(myCardsValueInt);
	std::list<std::string>::const_iterator cardStringIt_c = cardString.begin();

	std::string handName;

	switch(myCardsValueInt/100000000) {
	// Cucumbers pair
	case 10: {

		handName = *cardStringIt_c;

	}
	// Royal Flush
	case 9: {

		handName = *cardStringIt_c;

	}
	break;
	// Straight Flush
	case 8: {

		handName = *cardStringIt_c;
		++cardStringIt_c;
		handName += *cardStringIt_c;

	}
	break;
	// Four of a kind
	case 7: {

		handName = *cardStringIt_c;
		++cardStringIt_c;
		handName += *cardStringIt_c;

		// same hand detection
		for(it = shownCardsValueInt.begin(); it != shownCardsValueInt.end(); ++it) {
			if(((*it)/1000000) == (myCardsValueInt/1000000)) {
				sameHandCardsValueInt.push_back(*it);
			}
		}

		// 1.same hands existing
		if(!(sameHandCardsValueInt.empty())) {
			// first kicker?
			for(it = sameHandCardsValueInt.begin(); it != sameHandCardsValueInt.end(); ) {
				if(((*it)/10000) == (myCardsValueInt/10000)) {
					equal = true;
					++it;
				} else {
					different = true;
					it = sameHandCardsValueInt.erase(it);
				}
			}
			if(different) {
				++cardStringIt_c;
				handName += ", fifth card " + *cardStringIt_c;
			}
		}
	}
	break;
	// Full House
	case 6: {

		handName = *cardStringIt_c;
		++cardStringIt_c;
		handName += *cardStringIt_c;
		++cardStringIt_c;
		handName += *cardStringIt_c;

	}
	break;
	// Flush
	case 5: {

		handName = *cardStringIt_c;
		++cardStringIt_c;
		handName += *cardStringIt_c;

		// same hand detection
		for(it = shownCardsValueInt.begin(); it != shownCardsValueInt.end(); ++it) {
			if(((*it)/1000000) == (myCardsValueInt/1000000)) {
				sameHandCardsValueInt.push_back(*it);
			}
		}

		// 1.same hands existing
		if(!(sameHandCardsValueInt.empty())) {
			// first kicker?
			for(it = sameHandCardsValueInt.begin(); it != sameHandCardsValueInt.end(); ) {
				if(((*it)/10000) == (myCardsValueInt/10000)) {
					equal = true;
					++it;
				} else {
					different = true;
					it = sameHandCardsValueInt.erase(it);
				}
			}
			++cardStringIt_c;
			if(different) {
				handName += ", second card " + *cardStringIt_c;
			}
			// 2.there are still same hands
			if(equal) {
				different = false;
				equal = false;
				// second kicker?
				for(it = sameHandCardsValueInt.begin(); it != sameHandCardsValueInt.end(); ) {
					if(((*it)/100) == (myCardsValueInt/100)) {
						equal = true;
						++it;
					} else {
						different = true;
						it = sameHandCardsValueInt.erase(it);
					}
				}
				++cardStringIt_c;
				if(different) {
					handName += ", third card " + *cardStringIt_c;
				}
				// 3.there are still same hands
				if(equal) {
					different = false;
					equal = false;
					// third kicker?
					for(it = sameHandCardsValueInt.begin(); it != sameHandCardsValueInt.end(); ) {
						if(((*it)/10) == (myCardsValueInt/10)) {
							equal = true;
							++it;
						} else {
							different = true;
							it = sameHandCardsValueInt.erase(it);
						}
					}
					++cardStringIt_c;
					if(different) {
						handName += ", fourth card " + *cardStringIt_c;
					}
					// 4.there are still same hands
					if(equal) {
						different = false;
						equal = false;
						// third kicker?
						for(it = sameHandCardsValueInt.begin(); it != sameHandCardsValueInt.end(); ) {
							if((*it) == myCardsValueInt) {
								equal = true;
								++it;
							} else {
								different = true;
								it = sameHandCardsValueInt.erase(it);
							}
						}
						if(different) {
							++cardStringIt_c;
							handName += ", fifth card " + *cardStringIt_c;
						}
					}
				}
			}
		}
	}
	break;
	// Straight
	case 4: {

		handName = *cardStringIt_c;
		++cardStringIt_c;
		handName += *cardStringIt_c;

	}
	break;
	// Three of a kind
	case 3: {

		handName = *cardStringIt_c;
		++cardStringIt_c;
		handName += *cardStringIt_c;

		// same hand detection
		for(it = shownCardsValueInt.begin(); it != shownCardsValueInt.end(); ++it) {
			if(((*it)/1000000) == (myCardsValueInt/1000000)) {
				sameHandCardsValueInt.push_back(*it);
			}
		}

		// 1.same hands existing
		if(!(sameHandCardsValueInt.empty())) {
			// first kicker?
			for(it = sameHandCardsValueInt.begin(); it != sameHandCardsValueInt.end(); ) {
				if(((*it)/10000) == (myCardsValueInt/10000)) {
					equal = true;
					++it;
				} else {
					different = true;
					it = sameHandCardsValueInt.erase(it);
				}
			}
			++cardStringIt_c;
			if(different) {
				handName += ", fourth card " + *cardStringIt_c;
			}
			// 2.there are still same hands
			if(equal) {
				different = false;
				equal = false;
				// second kicker?
				for(it = sameHandCardsValueInt.begin(); it != sameHandCardsValueInt.end(); ) {
					if(((*it)/100) == (myCardsValueInt/100)) {
						equal = true;
						++it;
					} else {
						different = true;
						it = sameHandCardsValueInt.erase(it);
					}
				}
				if(different) {
					++cardStringIt_c;
					handName += ", fifth card " + *cardStringIt_c;
				}
			}
		}
	}
	break;
	// Two Pair
	case 2: {

		handName = *cardStringIt_c;
		++cardStringIt_c;
		handName += *cardStringIt_c;
		++cardStringIt_c;
		handName += *cardStringIt_c;

		// same hand detection
		for(it = shownCardsValueInt.begin(); it != shownCardsValueInt.end(); ++it) {
			if(((*it)/10000) == (myCardsValueInt/10000)) {
				sameHandCardsValueInt.push_back(*it);
			}
		}

		// 1.same hands existing
		if(!(sameHandCardsValueInt.empty())) {
			// first kicker?
			for(it = sameHandCardsValueInt.begin(); it != sameHandCardsValueInt.end(); ) {
				if(((*it)/100) == (myCardsValueInt/100)) {
					equal = true;
					++it;
				} else {
					different = true;
					it = sameHandCardsValueInt.erase(it);
				}
			}
			if(different) {
				++cardStringIt_c;
				handName += ", fifth card " + *cardStringIt_c;
			}
		}
	}
	break;
	// Pair
	case 1: {

		handName = *cardStringIt_c;
		++cardStringIt_c;
		handName += *cardStringIt_c;

		// same hand detection
		for(it = shownCardsValueInt.begin(); it != shownCardsValueInt.end(); ++it) {
			if(((*it)/1000000) == (myCardsValueInt/1000000)) {
				sameHandCardsValueInt.push_back(*it);
			}
		}

		// 1.same hands existing
		if(!(sameHandCardsValueInt.empty())) {
			// first kicker?
			for(it = sameHandCardsValueInt.begin(); it != sameHandCardsValueInt.end(); ) {
				if(((*it)/10000) == (myCardsValueInt/10000)) {
					equal = true;
					++it;
				} else {
					different = true;
					it = sameHandCardsValueInt.erase(it);
				}
			}
			++cardStringIt_c;
			if(different) {
				handName += ", third card " + *cardStringIt_c;
			}
			// 2.there are still same hands
			if(equal) {
				different = false;
				equal = false;
				// second kicker?
				for(it = sameHandCardsValueInt.begin(); it != sameHandCardsValueInt.end(); ) {
					if(((*it)/100) == (myCardsValueInt/100)) {
						equal = true;
						++it;
					} else {
						different = true;
						it = sameHandCardsValueInt.erase(it);
					}
				}
				++cardStringIt_c;
				if(different) {
					handName += ", fourth card " + *cardStringIt_c;
				}
				// 3.there are still same hands
				if(equal) {
					different = false;
					equal = false;
					// third kicker?
					for(it = sameHandCardsValueInt.begin(); it != sameHandCardsValueInt.end(); ) {
						if((*it) == myCardsValueInt) {
							equal = true;
							++it;
						} else {
							different = true;
							it = sameHandCardsValueInt.erase(it);
						}
					}
					if(different) {
						++cardStringIt_c;
						handName += ", fifth card " + *cardStringIt_c;
					}
				}
			}
		}
	}
	break;
	// highestCard
	case 0: {

		handName = *cardStringIt_c;
		++cardStringIt_c;
		handName += *cardStringIt_c;

		// same hand detection
		for(it = shownCardsValueInt.begin(); it != shownCardsValueInt.end(); ++it) {
			if(((*it)/1000000) == (myCardsValueInt/1000000)) {
				sameHandCardsValueInt.push_back(*it);
			}
		}

		// 1.same hands existing
		if(!(sameHandCardsValueInt.empty())) {
			// first kicker?
			for(it = sameHandCardsValueInt.begin(); it != sameHandCardsValueInt.end(); ) {
				if(((*it)/10000) == (myCardsValueInt/10000)) {
					equal = true;
					++it;
				} else {
					different = true;
					it = sameHandCardsValueInt.erase(it);
				}
			}
			++cardStringIt_c;
			if(different) {
				handName += ", second card " + *cardStringIt_c;
			}
			// 2.there are still same hands
			if(equal) {
				different = false;
				equal = false;
				// second kicker?
				for(it = sameHandCardsValueInt.begin(); it != sameHandCardsValueInt.end(); ) {
					if(((*it)/100) == (myCardsValueInt/100)) {
						equal = true;
						++it;
					} else {
						different = true;
						it = sameHandCardsValueInt.erase(it);
					}
				}
				++cardStringIt_c;
				if(different) {
					handName += ", third card " + *cardStringIt_c;
				}
				// 3.there are still same hands
				if(equal) {
					different = false;
					equal = false;
					// third kicker?
					for(it = sameHandCardsValueInt.begin(); it != sameHandCardsValueInt.end(); ) {
						if(((*it)/10) == (myCardsValueInt/10)) {
							equal = true;
							++it;
						} else {
							different = true;
							it = sameHandCardsValueInt.erase(it);
						}
					}
					++cardStringIt_c;
					if(different) {
						handName += ", fourth card " + *cardStringIt_c;
					}
					// 4.there are still same hands
					if(equal) {
						different = false;
						// third kicker?
						for(it = sameHandCardsValueInt.begin(); it != sameHandCardsValueInt.end(); ) {
							if((*it) == myCardsValueInt) {
								++it;
							} else {
								different = true;
								it = sameHandCardsValueInt.erase(it);
							}
						}
						if(different) {
							++cardStringIt_c;
							handName += ", fifth card " + *cardStringIt_c;
						}
					}
				}
			}
		}
	}
	break;
	default: {
	}
	}

	return handName;

}

std::list<std::string> CardsValue::translateCardsValueCode(int cardsValueCode)
{

	std::list<std::string> cardString;

	//erste Ziffer : Blattname
	int firstPart = cardsValueCode/100000000;
	//zweite und dritte Ziffer : Kicker, highest Card, usw.
	int secondPart = cardsValueCode/1000000 - firstPart*100;
	//vierte und fünfte Ziffer
	int thirdPart = cardsValueCode/10000 - firstPart*10000 - secondPart*100;
	// usw
	int fourthPart = cardsValueCode/100 - firstPart*1000000 - secondPart*10000 - thirdPart*100;
	//
	int fifthPart = cardsValueCode - firstPart*100000000 - secondPart*1000000 - thirdPart*10000 - fourthPart*100;
	// fuer highest Card
	int fifthPartA = cardsValueCode/10 - firstPart*10000000 - secondPart*100000 - thirdPart*1000 - fourthPart*10;
	int fifthPartB = cardsValueCode - firstPart*100000000 - secondPart*1000000 - thirdPart*10000 - fourthPart*100 - fifthPartA*10;


	switch (firstPart) {
	// Cucumbers pair
	case 10:
		cardString.push_back("Cucumbers pair");
		break;
	// Royal Flush
	case 9:
		cardString.push_back("Royal Flush");
		break;
	// Straight Flush
	case 8: {
		cardString.push_back("Straight Flush, ");
		switch(secondPart) {
		case 11:
			cardString.push_back("King high");
			break;
		case 10:
			cardString.push_back("Queen high");
			break;
		case 9:
			cardString.push_back("Jack high");
			break;
		case 8:
			cardString.push_back("Ten high");
			break;
		case 7:
			cardString.push_back("Nine high");
			break;
		case 6:
			cardString.push_back("Eight high");
			break;
		case 5:
			cardString.push_back("Seven high");
			break;
		case 4:
			cardString.push_back("Six high");
			break;
		case 3:
			cardString.push_back("Five high");
			break;
		default:
			cardString.push_back("ERROR");
		}
	}
	break;
	// Four of a Kind
	case 7: {
		cardString.push_back("Four of a Kind, ");
		switch(secondPart) {
		case 12:
			cardString.push_back("Aces");
			break;
		case 11:
			cardString.push_back("Kings");
			break;
		case 10:
			cardString.push_back("Queens");
			break;
		case 9:
			cardString.push_back("Jacks");
			break;
		case 8:
			cardString.push_back("Tens");
			break;
		case 7:
			cardString.push_back("Nines");
			break;
		case 6:
			cardString.push_back("Eights");
			break;
		case 5:
			cardString.push_back("Sevens");
			break;
		case 4:
			cardString.push_back("Sixes");
			break;
		case 3:
			cardString.push_back("Fives");
			break;
		case 2:
			cardString.push_back("Fours");
			break;
		case 1:
			cardString.push_back("Threes");
			break;
		case 0:
			cardString.push_back("Deuces");
			break;
		default:
			cardString.push_back("ERROR");
		}
		// Kicker
		switch(thirdPart) {
		case 12:
			cardString.push_back("Ace");
			break;
		case 11:
			cardString.push_back("King");
			break;
		case 10:
			cardString.push_back("Queen");
			break;
		case 9:
			cardString.push_back("Jack");
			break;
		case 8:
			cardString.push_back("Ten");
			break;
		case 7:
			cardString.push_back("Nine");
			break;
		case 6:
			cardString.push_back("Eight");
			break;
		case 5:
			cardString.push_back("Seven");
			break;
		case 4:
			cardString.push_back("Six");
			break;
		case 3:
			cardString.push_back("Five");
			break;
		case 2:
			cardString.push_back("Four");
			break;
		case 1:
			cardString.push_back("Three");
			break;
		case 0:
			cardString.push_back("Deuce");
			break;
		default:
			cardString.push_back("ERROR");
		}
	}
	break;
	// Full House
	case 6: {
		cardString.push_back("Full House, ");
		//Drilling
		switch(secondPart) {
		case 12:
			cardString.push_back("Aces full of ");
			break;
		case 11:
			cardString.push_back("Kings full of ");
			break;
		case 10:
			cardString.push_back("Queens full of ");
			break;
		case 9:
			cardString.push_back("Jacks full of ");
			break;
		case 8:
			cardString.push_back("Tens full of ");
			break;
		case 7:
			cardString.push_back("Nines full of ");
			break;
		case 6:
			cardString.push_back("Eights full of ");
			break;
		case 5:
			cardString.push_back("Sevens full of ");
			break;
		case 4:
			cardString.push_back("Sixes full of ");
			break;
		case 3:
			cardString.push_back("Fives full of ");
			break;
		case 2:
			cardString.push_back("Fours full of ");
			break;
		case 1:
			cardString.push_back("Threes full of ");
			break;
		case 0:
			cardString.push_back("Deuces full of ");
			break;
		default:
			cardString.push_back("ERROR");
		}
		//Pärchen
		switch(thirdPart) {
		case 12:
			cardString.push_back("Aces");
			break;
		case 11:
			cardString.push_back("Kings");
			break;
		case 10:
			cardString.push_back("Queens");
			break;
		case 9:
			cardString.push_back("Jacks");
			break;
		case 8:
			cardString.push_back("Tens");
			break;
		case 7:
			cardString.push_back("Nines");
			break;
		case 6:
			cardString.push_back("Eights");
			break;
		case 5:
			cardString.push_back("Sevens");
			break;
		case 4:
			cardString.push_back("Sixes");
			break;
		case 3:
			cardString.push_back("Fives");
			break;
		case 2:
			cardString.push_back("Fours");
			break;
		case 1:
			cardString.push_back("Threes");
			break;
		case 0:
			cardString.push_back("Deuces");
			break;
		default:
			cardString.push_back("ERROR");
		}
	}
	break;
	// Flush
	case 5: {
		cardString.push_back("Flush, ");
		//highest Card
		switch(secondPart) {
		case 12:
			cardString.push_back("Ace high");
			break;
		case 11:
			cardString.push_back("King high");
			break;
		case 10:
			cardString.push_back("Queen high");
			break;
		case 9:
			cardString.push_back("Jack high");
			break;
		case 8:
			cardString.push_back("Ten high");
			break;
		case 7:
			cardString.push_back("Nine high");
			break;
		case 6:
			cardString.push_back("Eight high");
			break;
		case 5:
			cardString.push_back("Seven high");
			break;
		case 4:
			cardString.push_back("Six high");
			break;
		default:
			cardString.push_back("ERROR");
		}
		// Kicker 1
		switch(thirdPart) {
		case 12:
			cardString.push_back("Ace");
			break;
		case 11:
			cardString.push_back("King");
			break;
		case 10:
			cardString.push_back("Queen");
			break;
		case 9:
			cardString.push_back("Jack");
			break;
		case 8:
			cardString.push_back("Ten");
			break;
		case 7:
			cardString.push_back("Nine");
			break;
		case 6:
			cardString.push_back("Eight");
			break;
		case 5:
			cardString.push_back("Seven");
			break;
		case 4:
			cardString.push_back("Six");
			break;
		case 3:
			cardString.push_back("Five");
			break;
		case 2:
			cardString.push_back("Four");
			break;
		case 1:
			cardString.push_back("Three");
			break;
		case 0:
			cardString.push_back("Deuce");
			break;
		default:
			cardString.push_back("ERROR");
		}
		//Kicker 2
		switch(fourthPart) {
		case 12:
			cardString.push_back("Ace");
			break;
		case 11:
			cardString.push_back("King");
			break;
		case 10:
			cardString.push_back("Queen");
			break;
		case 9:
			cardString.push_back("Jack");
			break;
		case 8:
			cardString.push_back("Ten");
			break;
		case 7:
			cardString.push_back("Nine");
			break;
		case 6:
			cardString.push_back("Eight");
			break;
		case 5:
			cardString.push_back("Seven");
			break;
		case 4:
			cardString.push_back("Six");
			break;
		case 3:
			cardString.push_back("Five");
			break;
		case 2:
			cardString.push_back("Four");
			break;
		case 1:
			cardString.push_back("Three");
			break;
		case 0:
			cardString.push_back("Deuce");
			break;
		default:
			cardString.push_back("ERROR");
		}
		//Kicker 3
		switch(fifthPartA) {
		case 9:
			cardString.push_back("Jack");
			break;
		case 8:
			cardString.push_back("Ten");
			break;
		case 7:
			cardString.push_back("Nine");
			break;
		case 6:
			cardString.push_back("Eight");
			break;
		case 5:
			cardString.push_back("Seven");
			break;
		case 4:
			cardString.push_back("Six");
			break;
		case 3:
			cardString.push_back("Five");
			break;
		case 2:
			cardString.push_back("Four");
			break;
		case 1:
			cardString.push_back("Three");
			break;
		case 0:
			cardString.push_back("Deuce");
			break;
		default:
			cardString.push_back("ERROR");
		}
		//Kicker 4
		switch(fifthPartB) {
		case 9:
			cardString.push_back("Jack");
			break;
		case 8:
			cardString.push_back("Ten");
			break;
		case 7:
			cardString.push_back("Nine");
			break;
		case 6:
			cardString.push_back("Eight");
			break;
		case 5:
			cardString.push_back("Seven");
			break;
		case 4:
			cardString.push_back("Six");
			break;
		case 3:
			cardString.push_back("Five");
			break;
		case 2:
			cardString.push_back("Four");
			break;
		case 1:
			cardString.push_back("Three");
			break;
		case 0:
			cardString.push_back("Deuce");
			break;
		default:
			cardString.push_back("ERROR");
		}
	}
	break;
	// Straight
	case 4: {
		cardString.push_back("Straight, ");
		switch(secondPart) {
		case 12:
			cardString.push_back("Ace high");
			break;
		case 11:
			cardString.push_back("King high");
			break;
		case 10:
			cardString.push_back("Queen high");
			break;
		case 9:
			cardString.push_back("Jack high");
			break;
		case 8:
			cardString.push_back("Ten high");
			break;
		case 7:
			cardString.push_back("Nine high");
			break;
		case 6:
			cardString.push_back("Eight high");
			break;
		case 5:
			cardString.push_back("Seven high");
			break;
		case 4:
			cardString.push_back("Six high");
			break;
		case 3:
			cardString.push_back("Five high");
			break;
		default:
			cardString.push_back("ERROR");
		}
	}
	break;
	// Three of a Kind
	case 3: {
		cardString.push_back("Three of a Kind, ");
		switch(secondPart) {
		case 12:
			cardString.push_back("Aces");
			break;
		case 11:
			cardString.push_back("Kings");
			break;
		case 10:
			cardString.push_back("Queens");
			break;
		case 9:
			cardString.push_back("Jacks");
			break;
		case 8:
			cardString.push_back("Tens");
			break;
		case 7:
			cardString.push_back("Nines");
			break;
		case 6:
			cardString.push_back("Eights");
			break;
		case 5:
			cardString.push_back("Sevens");
			break;
		case 4:
			cardString.push_back("Sixes");
			break;
		case 3:
			cardString.push_back("Fives");
			break;
		case 2:
			cardString.push_back("Fours");
			break;
		case 1:
			cardString.push_back("Threes");
			break;
		case 0:
			cardString.push_back("Deuces");
			break;
		default:
			cardString.push_back("ERROR");
		}
		//Kicker 1
		switch(thirdPart) {
		case 12:
			cardString.push_back("Ace");
			break;
		case 11:
			cardString.push_back("King");
			break;
		case 10:
			cardString.push_back("Queen");
			break;
		case 9:
			cardString.push_back("Jack");
			break;
		case 8:
			cardString.push_back("Ten");
			break;
		case 7:
			cardString.push_back("Nine");
			break;
		case 6:
			cardString.push_back("Eight");
			break;
		case 5:
			cardString.push_back("Seven");
			break;
		case 4:
			cardString.push_back("Six");
			break;
		case 3:
			cardString.push_back("Five");
			break;
		case 2:
			cardString.push_back("Four");
			break;
		case 1:
			cardString.push_back("Three");
			break;
		case 0:
			cardString.push_back("Deuce");
			break;
		default:
			cardString.push_back("ERROR");
		}
		//Kicker 2
		switch(fourthPart) {
		case 12:
			cardString.push_back("Ace");
			break;
		case 11:
			cardString.push_back("King");
			break;
		case 10:
			cardString.push_back("Queen");
			break;
		case 9:
			cardString.push_back("Jack");
			break;
		case 8:
			cardString.push_back("Ten");
			break;
		case 7:
			cardString.push_back("Nine");
			break;
		case 6:
			cardString.push_back("Eight");
			break;
		case 5:
			cardString.push_back("Seven");
			break;
		case 4:
			cardString.push_back("Six");
			break;
		case 3:
			cardString.push_back("Five");
			break;
		case 2:
			cardString.push_back("Four");
			break;
		case 1:
			cardString.push_back("Three");
			break;
		case 0:
			cardString.push_back("Deuce");
			break;
		default:
			cardString.push_back("ERROR");
		}
	}
	break;
	// Two Pair
	case 2: {
		cardString.push_back("Two Pair, ");
		// erster Pair
		switch(secondPart) {
		case 12:
			cardString.push_back("Aces and ");
			break;
		case 11:
			cardString.push_back("Kings and ");
			break;
		case 10:
			cardString.push_back("Queens and ");
			break;
		case 9:
			cardString.push_back("Jacks and ");
			break;
		case 8:
			cardString.push_back("Tens and ");
			break;
		case 7:
			cardString.push_back("Nines and ");
			break;
		case 6:
			cardString.push_back("Eights and ");
			break;
		case 5:
			cardString.push_back("Sevens and ");
			break;
		case 4:
			cardString.push_back("Sixes and ");
			break;
		case 3:
			cardString.push_back("Fives and ");
			break;
		case 2:
			cardString.push_back("Fours and ");
			break;
		case 1:
			cardString.push_back("Threes and ");
			break;
		case 0:
			cardString.push_back("Deuces and ");
			break;
		default:
			cardString.push_back("ERROR");
		}
		// zweiter Pair
		switch(thirdPart) {
		case 12:
			cardString.push_back("Aces");
			break;
		case 11:
			cardString.push_back("Kings");
			break;
		case 10:
			cardString.push_back("Queens");
			break;
		case 9:
			cardString.push_back("Jacks");
			break;
		case 8:
			cardString.push_back("Tens");
			break;
		case 7:
			cardString.push_back("Nines");
			break;
		case 6:
			cardString.push_back("Eights");
			break;
		case 5:
			cardString.push_back("Sevens");
			break;
		case 4:
			cardString.push_back("Sixes");
			break;
		case 3:
			cardString.push_back("Fives");
			break;
		case 2:
			cardString.push_back("Fours");
			break;
		case 1:
			cardString.push_back("Threes");
			break;
		case 0:
			cardString.push_back("Deuces");
			break;
		default:
			cardString.push_back("ERROR");
		}
		//Kicker
		switch(fourthPart) {
		case 12:
			cardString.push_back("Ace");
			break;
		case 11:
			cardString.push_back("King");
			break;
		case 10:
			cardString.push_back("Queen");
			break;
		case 9:
			cardString.push_back("Jack");
			break;
		case 8:
			cardString.push_back("Ten");
			break;
		case 7:
			cardString.push_back("Nine");
			break;
		case 6:
			cardString.push_back("Eight");
			break;
		case 5:
			cardString.push_back("Seven");
			break;
		case 4:
			cardString.push_back("Six");
			break;
		case 3:
			cardString.push_back("Five");
			break;
		case 2:
			cardString.push_back("Four");
			break;
		case 1:
			cardString.push_back("Three");
			break;
		case 0:
			cardString.push_back("Deuce");
			break;
		default:
			cardString.push_back("ERROR");
		}
	}
	break;
	// One Pair
	case 1: {
		cardString.push_back("One Pair, ");
		// Pair
		switch(secondPart) {
		case 12:
			cardString.push_back("Aces");
			break;
		case 11:
			cardString.push_back("Kings");
			break;
		case 10:
			cardString.push_back("Queens");
			break;
		case 9:
			cardString.push_back("Jacks");
			break;
		case 8:
			cardString.push_back("Tens");
			break;
		case 7:
			cardString.push_back("Nines");
			break;
		case 6:
			cardString.push_back("Eights");
			break;
		case 5:
			cardString.push_back("Sevens");
			break;
		case 4:
			cardString.push_back("Sixes");
			break;
		case 3:
			cardString.push_back("Fives");
			break;
		case 2:
			cardString.push_back("Fours");
			break;
		case 1:
			cardString.push_back("Threes");
			break;
		case 0:
			cardString.push_back("Deuces");
			break;
		default:
			cardString.push_back("ERROR");
		}
		// Kicker 1
		switch(thirdPart) {
		case 12:
			cardString.push_back("Ace");
			break;
		case 11:
			cardString.push_back("King");
			break;
		case 10:
			cardString.push_back("Queen");
			break;
		case 9:
			cardString.push_back("Jack");
			break;
		case 8:
			cardString.push_back("Ten");
			break;
		case 7:
			cardString.push_back("Nine");
			break;
		case 6:
			cardString.push_back("Eight");
			break;
		case 5:
			cardString.push_back("Seven");
			break;
		case 4:
			cardString.push_back("Six");
			break;
		case 3:
			cardString.push_back("Five");
			break;
		case 2:
			cardString.push_back("Four");
			break;
		case 1:
			cardString.push_back("Three");
			break;
		case 0:
			cardString.push_back("Deuce");
			break;
		default:
			cardString.push_back("ERROR");
		}
		//Kicker 2
		switch(fourthPart) {
		case 12:
			cardString.push_back("Ace");
			break;
		case 11:
			cardString.push_back("King");
			break;
		case 10:
			cardString.push_back("Queen");
			break;
		case 9:
			cardString.push_back("Jack");
			break;
		case 8:
			cardString.push_back("Ten");
			break;
		case 7:
			cardString.push_back("Nine");
			break;
		case 6:
			cardString.push_back("Eight");
			break;
		case 5:
			cardString.push_back("Seven");
			break;
		case 4:
			cardString.push_back("Six");
			break;
		case 3:
			cardString.push_back("Five");
			break;
		case 2:
			cardString.push_back("Four");
			break;
		case 1:
			cardString.push_back("Three");
			break;
		case 0:
			cardString.push_back("Deuce");
			break;
		default:
			cardString.push_back("ERROR");
		}
		//Kicker 3
		switch(fifthPart) {
		case 12:
			cardString.push_back("Ace");
			break;
		case 11:
			cardString.push_back("King");
			break;
		case 10:
			cardString.push_back("Queen");
			break;
		case 9:
			cardString.push_back("Jack");
			break;
		case 8:
			cardString.push_back("Ten");
			break;
		case 7:
			cardString.push_back("Nine");
			break;
		case 6:
			cardString.push_back("Eight");
			break;
		case 5:
			cardString.push_back("Seven");
			break;
		case 4:
			cardString.push_back("Six");
			break;
		case 3:
			cardString.push_back("Five");
			break;
		case 2:
			cardString.push_back("Four");
			break;
		case 1:
			cardString.push_back("Three");
			break;
		case 0:
			cardString.push_back("Deuce");
			break;
		default:
			cardString.push_back("ERROR");
		}
	}
	break;
	// Highest Card
	case 0:  {
		cardString.push_back("High Card, ");
		// Kicker 1
		switch(secondPart) {
		case 12:
			cardString.push_back("Ace");
			break;
		case 11:
			cardString.push_back("King");
			break;
		case 10:
			cardString.push_back("Queen");
			break;
		case 9:
			cardString.push_back("Jack");
			break;
		case 8:
			cardString.push_back("Ten");
			break;
		case 7:
			cardString.push_back("Nine");
			break;
		case 6:
			cardString.push_back("Eight");
			break;
		case 5:
			cardString.push_back("Seven");
			break;
		case 4:
			cardString.push_back("Six");
			break;
		case 3:
			cardString.push_back("Five");
			break;
		case 2:
			cardString.push_back("Four");
			break;
		case 1:
			cardString.push_back("Three");
			break;
		case 0:
			cardString.push_back("Deuces");
			break;
		default:
			cardString.push_back("ERROR");
		}
		// Kicker 2
		switch(thirdPart) {
		case 12:
			cardString.push_back("Ace");
			break;
		case 11:
			cardString.push_back("King");
			break;
		case 10:
			cardString.push_back("Queen");
			break;
		case 9:
			cardString.push_back("Jack");
			break;
		case 8:
			cardString.push_back("Ten");
			break;
		case 7:
			cardString.push_back("Nine");
			break;
		case 6:
			cardString.push_back("Eight");
			break;
		case 5:
			cardString.push_back("Seven");
			break;
		case 4:
			cardString.push_back("Six");
			break;
		case 3:
			cardString.push_back("Five");
			break;
		case 2:
			cardString.push_back("Four");
			break;
		case 1:
			cardString.push_back("Three");
			break;
		case 0:
			cardString.push_back("Deuce");
			break;
		default:
			cardString.push_back("ERROR");
		}
		//Kicker 3
		switch(fourthPart) {
		case 12:
			cardString.push_back("Ace");
			break;
		case 11:
			cardString.push_back("King");
			break;
		case 10:
			cardString.push_back("Queen");
			break;
		case 9:
			cardString.push_back("Jack");
			break;
		case 8:
			cardString.push_back("Ten");
			break;
		case 7:
			cardString.push_back("Nine");
			break;
		case 6:
			cardString.push_back("Eight");
			break;
		case 5:
			cardString.push_back("Seven");
			break;
		case 4:
			cardString.push_back("Six");
			break;
		case 3:
			cardString.push_back("Five");
			break;
		case 2:
			cardString.push_back("Four");
			break;
		case 1:
			cardString.push_back("Three");
			break;
		case 0:
			cardString.push_back("Deuce");
			break;
		default:
			cardString.push_back("ERROR");
		}
		//Kicker 4
		switch(fifthPartA) {
		case 9:
			cardString.push_back("Jack");
			break;
		case 8:
			cardString.push_back("Ten");
			break;
		case 7:
			cardString.push_back("Nine");
			break;
		case 6:
			cardString.push_back("Eight");
			break;
		case 5:
			cardString.push_back("Seven");
			break;
		case 4:
			cardString.push_back("Six");
			break;
		case 3:
			cardString.push_back("Five");
			break;
		case 2:
			cardString.push_back("Four");
			break;
		case 1:
			cardString.push_back("Three");
			break;
		case 0:
			cardString.push_back("Deuce");
			break;
		default:
			cardString.push_back("ERROR");
		}
		//Kicker 5
		switch(fifthPartB) {
		case 9:
			cardString.push_back("Jack");
			break;
		case 8:
			cardString.push_back("Ten");
			break;
		case 7:
			cardString.push_back("Nine");
			break;
		case 6:
			cardString.push_back("Eight");
			break;
		case 5:
			cardString.push_back("Seven");
			break;
		case 4:
			cardString.push_back("Six");
			break;
		case 3:
			cardString.push_back("Five");
			break;
		case 2:
			cardString.push_back("Four");
			break;
		case 1:
			cardString.push_back("Three");
			break;
		case 0:
			cardString.push_back("Deuce");
			break;
		default:
			cardString.push_back("ERROR");
		}
	}
	break;
	default:
		cardString.push_back("ERROR");

	}

	return cardString;

}


//int** CardsValue::showdown(GameState beRoID, int** playerCards, int playerCount) {
//
// 	int i,j;
//
// 	int** chance = new int*[2];
//
// 	for(i=0;i<10;i++) {
// 		chance[i] = new int[2];
// 		for(j=0;j<2;j++) {
// 			chance[i][j] = 0;
// 		}
// 	}
//
// 	int rand[5];
//
// 	return chance;
//
//}
