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

int CardsValue::holeCardsClass(int one, int two)
{

	if((one-1)%13<(two-1)%13) {
		int temp = one;
		one = two;
		two = temp;
	}


	if((one-1)%13 == (two-1)%13) {
		if((one-1)%13+2 > 10) return 10;
		else {
			switch((one-1)%13+2) {
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
	switch((one-1)%13+2) {
		//Ass
	case 14: {
		if((one-1)/13 == (two-1)/13) {
			switch((one-1)%13-(two-1)%13) {
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
			switch((one-1)%13-(two-1)%13) {
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
		if((one-1)/13 == (two-1)/13) {
			switch((one-1)%13-(two-1)%13) {
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
			switch((one-1)%13-(two-1)%13) {
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
		if((one-1)/13 == (two-1)/13) {
			switch((one-1)%13-(two-1)%13) {
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
			switch((one-1)%13-(two-1)%13) {
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
		if((one-1)/13 == (two-1)/13) {
			switch((one-1)%13-(two-1)%13) {
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
			switch((one-1)%13-(two-1)%13) {
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
		if((one-1)/13 == (two-1)/13) {
			switch((one-1)%13-(two-1)%13) {
			case 1:
				return 6;
			case 2:
				return 5;
			default:
				return 2;
			}
		} else {
			switch((one-1)%13-(two-1)%13) {
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
		if((one-1)%13 - (two-1)%13 <= 2) {
			if((one-1)/13 == (two-1)/13) return 5;
			else return 3;
		} else {
			if((one-1)%13 - (two-1)%13 == 3) return 2;
			else return 1;
		}
	}
	}


}

int CardsValue::holeCardsToIntCode(int holeCards[2])
{

	// Code der HoleCards ermitteln
	if(holeCards[0]%13 == holeCards[1]%13) {
		return ((holeCards[0]%13)*1000 + (holeCards[0]%13)*10);
	} else {
		if(holeCards[0]%13 < holeCards[1]%13) {
			if(holeCards[0]/13 == holeCards[1]/13) {
				return ((holeCards[0]%13)*1000 + (holeCards[1]%13)*10 + 1);
			} else {
				return ((holeCards[0]%13)*1000 + (holeCards[1]%13)*10);
			}
		} else {
			if(holeCards[0]/13 == holeCards[1]/13) {
				return ((holeCards[1]%13)*1000 + (holeCards[0]%13)*10 + 1);
			} else {
				return ((holeCards[1]%13)*1000 + (holeCards[0]%13)*10);
			}
		}
	}

}

static const int straight[10] = { 7936, 3968, 1984, 992, 496, 248, 124, 62, 31, 4111 };

int CardsValue::cardsValueShort(int cards[4])
{

	int color_idx;
	int card_idx;

	// Royal Flush, Straight Flush, Flush
	for(color_idx=0; color_idx<4; color_idx++) { // check all colors
		if(bitcount(cards[color_idx])>=5) { // check if at least 5 cards of one color
			if((cards[color_idx] & straight[0]) == straight[0]) // check for Royal Flush
				return 9;
			else { // check for Straight Flush
				for(card_idx=1; card_idx<10; card_idx++) {
					if((cards[color_idx] & straight[card_idx]) == straight[card_idx]) {
						return 8; // Straight Flush
					}
				}
			}
			return 5; // Flush
		}
	}

	int AND = cards[0] & cards[1] & cards[2] & cards[3];

	// Four of a Kind
	if(AND) {
		return 7;
	}

	int OR = cards[0] | cards[1] | cards[2] | cards[3];

	// Straight
	for(card_idx=0; card_idx<10; card_idx++) {
		if((OR & straight[card_idx]) == straight[card_idx]) {
			return 4;
		}
	}

	int color_1_idx, color_2_idx, color_3_idx, color_4_idx, color_5_idx;
	int temp;

	// Full House, Three of a Kind
	for(color_1_idx=0; color_1_idx<2; color_1_idx++) {
		for(color_2_idx=color_1_idx+1; color_2_idx<3; color_2_idx++) {
			for(color_3_idx=color_2_idx+1; color_3_idx<4; color_3_idx++) {
				temp = cards[color_1_idx] & cards[color_2_idx] & cards[color_3_idx];
				if(bitcount(temp) == 2) { // two times Three of a Kind
					return 6; // Full House
				} else {
					if(temp) {
						for(color_4_idx=0; color_4_idx<3; color_4_idx++) {
							for(color_5_idx=color_4_idx+1; color_5_idx<4; color_5_idx++) {
								if(~temp & cards[color_4_idx] & cards[color_5_idx]) { // search for additional pair
									return 6;  // Full House
								}
							}
						}
						return 3; // Three of a Kind
					}
				}
			}
		}
	}

	// Two Pairs, Two of a Kind
	for(color_1_idx=0; color_1_idx<3; color_1_idx++) {
		for(color_2_idx=color_1_idx+1; color_2_idx<4; color_2_idx++) {
			temp = cards[color_1_idx] & cards[color_2_idx];
			if(bitcount(temp) >= 2) { // at least two times Two of a Kind
				return 2; // Two Pairs
			} else {
				if(temp) { // search for second pair
					for(color_3_idx=0; color_3_idx<3; color_3_idx++) {
						for(color_4_idx=color_3_idx+1; color_4_idx<4; color_4_idx++) {
							if(~temp & cards[color_3_idx] & cards[color_4_idx]) {
								return 2; // Two Pairs
							}
						}
					}
					return 1; // Two of a Kind
				}
			}
		}
	}

	return 0; // High Card
}

int CardsValue::cardsValue(int cards[4], int bestHand[4])
{
	int color_1_idx;
	int card_idx;
	KickerValue kickerValue1;

	// Royal Flush, Straight Flush, Flush
	for(color_1_idx=0; color_1_idx<4; color_1_idx++) { // check all colors
		if(bitcount(cards[color_1_idx])>=5) { // check if at least 5 cards of one color
			if((cards[color_1_idx] & straight[0]) == straight[0]) { // check for Royal Flush
				if(bestHand) bestHand[color_1_idx] = straight[0];
				return 900000000; // Royal Flush
			} else {
				for(card_idx=1; card_idx<10; card_idx++) {
					if((cards[color_1_idx] & straight[card_idx]) == straight[card_idx]) { // check for Straight Flush
						if(bestHand)  bestHand[color_1_idx] = straight[card_idx];
						return (800000000+(12-card_idx)*1000000); // Straight Flush
					}
				}
			}
			// Flush
			kickerValue1 = determineKickerValue(cards[color_1_idx],0,4);
			if(bestHand) bestHand[color_1_idx] = kickerValue1.select;
			return 500000000 + kickerValue1.factorValue;
		}
	}

	int AND = cards[0] & cards[1] & cards[2] & cards[3];
	int OR = cards[0] | cards[1] | cards[2] | cards[3];
	int temp1;
	KickerValue kickerValue2;

	// Four of a Kind
	if(AND) {
		kickerValue1 = determineKickerValue(AND,0,0);
		kickerValue2 = determineKickerValue(OR & ~AND,1,1);
		if(bestHand) {
			temp1 = kickerValue2.select;
			for(color_1_idx=3; color_1_idx>=0; color_1_idx--) {
				bestHand[color_1_idx] = (cards[color_1_idx] & (kickerValue1.select | temp1));
				if(bestHand[color_1_idx] & temp1) temp1 = 0;
			}
		}
		return 700000000 + kickerValue1.factorValue + kickerValue2.factorValue;
	}

	// Straight
	for(card_idx=0; card_idx<10; card_idx++) {
		if((OR & straight[card_idx]) == straight[card_idx]) {
			if(bestHand) {
				temp1 = straight[card_idx];
				for(color_1_idx=3; color_1_idx>=0; color_1_idx--) {
					bestHand[color_1_idx] += (temp1 & cards[color_1_idx]);
					temp1 &= ~bestHand[color_1_idx];
				}
			}
			return 400000000 + (12-card_idx)*1000000;
		}
	}

	int color_2_idx, color_3_idx;

	// Full House, Three of a Kind
	temp1 = 0;
	for(color_1_idx=0; color_1_idx<2; color_1_idx++) {
		for(color_2_idx=color_1_idx+1; color_2_idx<3; color_2_idx++) {
			for(color_3_idx=color_2_idx+1; color_3_idx<4; color_3_idx++) {
				temp1 |= cards[color_1_idx] & cards[color_2_idx] & cards[color_3_idx];
			}
		}
	}
	if(temp1) {
		if(bitcount(temp1) == 2) {
			// two times Three of a Kind
			if(bestHand) {
				kickerValue1 = determineKickerValue(temp1,0,0);
				kickerValue2 = determineKickerValue(kickerValue1.remain,1,1);
				int temp2 = 0;
				for(color_1_idx=3; color_1_idx>=0; color_1_idx--) {
					if(temp2<2) bestHand[color_1_idx] += (cards[color_1_idx] & (kickerValue1.select | kickerValue2.select));
					else bestHand[color_1_idx] += (cards[color_1_idx] & kickerValue1.select);
					if(cards[color_1_idx] & kickerValue2.select) temp2++;
				}
			}
			return 600000000 + determineKickerValue(temp1,0,1).factorValue;
		} else {
			// one times Three of a Kind
			int temp2 = temp1;
			temp1 = 0;
			// check for additional pair
			for(color_1_idx=0; color_1_idx<3; color_1_idx++) {
				for(color_2_idx=color_1_idx+1; color_2_idx<4; color_2_idx++) {
					temp1 |= cards[color_1_idx] & cards[color_2_idx];
				}
			}
			temp1 &= ~temp2; // remove Three of a Kind from found pairs
			if(temp1) {
				// with additional pair
				kickerValue1 = determineKickerValue(temp2,0,0);
				kickerValue2 = determineKickerValue(temp1,1,1);
				if(bestHand) {
					for(color_1_idx=3; color_1_idx>=0; color_1_idx--) bestHand[color_1_idx] = (cards[color_1_idx] & (kickerValue1.select | kickerValue2.select));
				}
				return 600000000 + kickerValue1.factorValue + kickerValue2.factorValue;  // Full House
			} else {
				// without addition pair
				kickerValue1 = determineKickerValue(temp2,0,0);
				kickerValue2 = determineKickerValue(OR & ~temp2,1,2);
				if(bestHand) {
					for(color_1_idx=3; color_1_idx>=0; color_1_idx--) bestHand[color_1_idx] = (cards[color_1_idx] & (kickerValue1.select | kickerValue2.select));
				}
				return 300000000 + kickerValue1.factorValue + kickerValue2.factorValue; // Three of a Kind
			}
		}
	}

	// Two Pairs, Two of a Kind
	temp1 = 0;
	for(color_1_idx=0; color_1_idx<3; color_1_idx++) {
		for(color_2_idx=color_1_idx+1; color_2_idx<4; color_2_idx++) {
			temp1  |= (cards[color_1_idx] & cards[color_2_idx]);
		}
	}
	if(temp1) {
		if(bitcount(temp1) >= 2) { // at least two times Two of a Kind
			kickerValue1 = determineKickerValue(temp1,0,1);
			kickerValue2 = determineKickerValue(OR & ~kickerValue1.select,2,2);
			if(bestHand) {
				temp1 = (kickerValue1.select | kickerValue2.select);
				for(color_1_idx=3; color_1_idx>=0; color_1_idx--) {
					bestHand[color_1_idx] += (cards[color_1_idx] & temp1);
					if(bestHand[color_1_idx] & kickerValue2.select) temp1 &= ~kickerValue2.select;
				}
			}
			return 200000000 + kickerValue1.factorValue + kickerValue2.factorValue; // Two Pairs
		} else {
			kickerValue1 = determineKickerValue(temp1,0,0);
			kickerValue2 = determineKickerValue(OR & ~temp1,1,3);
			if(bestHand) {
				for(color_1_idx=3; color_1_idx>=0; color_1_idx--) {
					bestHand[color_1_idx] += (cards[color_1_idx] & (kickerValue1.select | kickerValue2.select));
				}
			}
			return 100000000 + kickerValue1.factorValue + kickerValue2.factorValue; // Two of a Kind
		}
	}

	// High Card
	kickerValue1 = determineKickerValue(OR,0,4);
	if(bestHand) {
		for(color_1_idx=3; color_1_idx>=0; color_1_idx--) {
			bestHand[color_1_idx] += (cards[color_1_idx] & kickerValue1.select);
		}
	}
	return kickerValue1.factorValue;

}

static const int factor_kicker_short[4] = {1000000,10000,100,1};
static const int factor_kicker_long[5] = {1000000,10000,100,10,1};

KickerValue CardsValue::determineKickerValue(int testValue, int factorPointerStart, int factorPointerEnd)
{
	KickerValue kickerValue;
	kickerValue.factorValue = 0;
	kickerValue.remain = testValue;
	kickerValue.select = 0;
	int compareValue = 4096;
	int factorPointer = factorPointerStart;
	for(int card_idx=0; (factorPointer<=factorPointerEnd) & (card_idx<13); card_idx++) {
		if(kickerValue.remain >= compareValue) {
			if(factorPointerEnd - factorPointerStart==4) kickerValue.factorValue += (12-card_idx)*factor_kicker_long[factorPointer];
			else kickerValue.factorValue += (12-card_idx)*factor_kicker_short[factorPointer];
			kickerValue.remain &= ~compareValue;
			factorPointer++;
		}
		compareValue >>= 1;
	}
	kickerValue.select = testValue & ~kickerValue.remain;
	return kickerValue;
}

std::vector< std::vector<int> > CardsValue::calcCardsChance(GameState beRoID, int playerCards[2], int boardCards[5])
{
	std::vector< std::vector<int> > chance(2);
	chance[0].assign(10,0);
	chance[1].assign(10,0);

	int cards[4] = { 0,0,0,0 };
	int sum = 0;

	int card_idx_1;
	for(card_idx_1=0; card_idx_1<2; card_idx_1++) cards[playerCards[card_idx_1]/13] |= (1 << (playerCards[card_idx_1]%13));

	switch(beRoID) {
	case GAME_STATE_PREFLOP: {

		chance = ArrayData::getHandChancePreflop(holeCardsToIntCode(playerCards));

	}
	break;
	case GAME_STATE_FLOP: {

		for(card_idx_1=0; card_idx_1<3; card_idx_1++) cards[boardCards[card_idx_1]/13] |= (1 << (boardCards[card_idx_1]%13));

		for(card_idx_1=0; card_idx_1<51; card_idx_1++) {
			if((cards[card_idx_1/13] & (1 << (card_idx_1%13))) == 0) {
				cards[card_idx_1/13] |= (1 << (card_idx_1%13));
				for(int card_idx_2=card_idx_1+1; card_idx_2<52; card_idx_2++) {
					if((cards[card_idx_2/13] & (1 << (card_idx_2%13))) == 0) {
						cards[card_idx_2/13] |= (1 << (card_idx_2%13));
						(chance[0][cardsValueShort(cards)])++;
						sum++;
						cards[card_idx_2/13] &= ~(1 << (card_idx_2%13));
					}
				}
				cards[card_idx_1/13] &= ~(1 << (card_idx_1%13));
			}
		}

	}
	break;
	case GAME_STATE_TURN: {

		for(card_idx_1=0; card_idx_1<4; card_idx_1++) cards[boardCards[card_idx_1]/13] |= (1 << (boardCards[card_idx_1]%13));

		for(card_idx_1=0; card_idx_1<52; card_idx_1++) {
			if((cards[card_idx_1/13] & (1 << (card_idx_1%13))) == 0) {
				cards[card_idx_1/13] |= (1 << (card_idx_1%13));
				(chance[0][cardsValueShort(cards)])++;
				sum++;
				cards[card_idx_1/13] &= ~(1 << (card_idx_1%13));
			}
		}

	}
	break;
	case GAME_STATE_RIVER: {

		for(card_idx_1=0; card_idx_1<5; card_idx_1++) cards[boardCards[card_idx_1]/13] |= (1 << (boardCards[card_idx_1]%13));

		chance[0][cardsValueShort(cards)] = 1;
		sum = 1;

	}
	break;
	default: {
	}
	}

	if(beRoID>GAME_STATE_PREFLOP) {
		for(int hand_idx=0; hand_idx<10; hand_idx++) {
			if(chance[0][hand_idx] > 0) chance[1][hand_idx] = 1;
			chance[0][hand_idx] = (int)(((double)chance[0][hand_idx]/(double)sum)*100.0+0.5);
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
	default:
	{}
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

int CardsValue::bitcount(int in)
{
	int count=0 ;
	while (in) {
		count++ ;
		in &= (in - 1) ;

	}
	return count ;
}

int
CardsValue::bestHandToPosition(int bestHand[4], int cardArray[7], int position[5])
{

	for(int card_idx=0; card_idx<5; card_idx++) position[card_idx] = -1;

	int position_ctr = 0;
	for(int color_idx=0; color_idx<4; color_idx++) {
		for(int card_idx_1=0; card_idx_1<13; card_idx_1++) {
			if(bestHand[color_idx] & (1<<card_idx_1)) {
				for(int card_idx_2=0; card_idx_2<7; card_idx_2++) {
					if(cardArray[card_idx_2] == color_idx*13+card_idx_1) {
						if(position_ctr<5) {
							position[position_ctr] = card_idx_2;
							position_ctr++;
						} else {
							return 0;
						}
					}
				}
			}
		}
	}
	if(position_ctr!=5) return 0;
	else return 1;
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
