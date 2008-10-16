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
#define NOMINMAX // for Windows

#include "tools.h"
#include <core/loghelper.h>
#include <core/openssl_wrapper.h>

#include <limits>

using namespace std;

void Tools::getRandNumber(int start, int end, int howMany, int* randArray, bool different, int* bad) {

	int r = end-start+1;
	unsigned char rand_buf[4];
	unsigned int randNumber;

	int i,j;

	if (!different) {

		for (i=0; i<howMany; i++) {

			if(!RAND_bytes(rand_buf, 4))
			{
				LOG_MSG("RAND_bytes failed!");
			}

			randNumber = 0;
			for(j=0; j<4; j++) {
				randNumber += (rand_buf[j] << 8*j);
			}

			if(randNumber < ( (unsigned int) ( ((double)numeric_limits<unsigned int>::max()) / r ) ) * r) {
				randArray[i] = start + (randNumber % r);
			}

		}
	}
	else {

		int *tempArray = new int[end-start+1];
		for (i=0; i<(end-start+1); i++) tempArray[i]=1;

		if(bad) {
			for(i=0;i<(sizeof(bad)/sizeof(int));i++) {
				tempArray[bad[i]]=0;
			}
		}
		
		int counter(0);
		while (counter < howMany) {

			if(!RAND_bytes(rand_buf, 4))
			{
				LOG_MSG("RAND_bytes failed!");
			}

			randNumber = 0;
			for(j=0; j<4; j++) {
				randNumber += (rand_buf[j] << 8*j);
			}

			if(randNumber < ( (unsigned int) ( ((double)numeric_limits<unsigned int>::max()) / r ) ) * r) {
				randNumber = randNumber % r;

				if (tempArray[randNumber] == 1) { 
	
					randArray[counter] = start + randNumber; 
					tempArray[randNumber] = 0;
					counter++;
				}
			}
			
		}

		delete[] tempArray;
	}

}

int* Tools::calcCardsChance(GameState beRoID, int* playerCards, int* boardCards)
{
	CardsValue *myCardsValue = new CardsValue;

	int preflopCalcNumber = 10000;

	int i,j;

	int hand[10];
	int cards[7];
	int sum = 0;

	for(i=0;i<10;i++) hand[i] = 0;

	cards[0] = playerCards[0];
	cards[1] = playerCards[1];
	for(i=0;i<5;i++) cards[i+2] = boardCards[i];

	

	switch(beRoID) {
		case GAME_STATE_PREFLOP: {
// 			int rand[5];
// 			for(i=0;i<preflopCalcNumber;i++) {
// 				getRandNumber(0,51,5,rand,1,playerCards);
// 				
// 			}
		} break;
		case GAME_STATE_FLOP: {

			for(i=0;i<51;i++) {
				if(i!=cards[0] && i!=cards[1] && i!=cards[2] && i!=cards[3] && i!=cards[4]) {
				for(j=i+1;j<52;j++) {
					if(j!=cards[0] && j!=cards[1] && j!=cards[2] && j!=cards[3] && j!=cards[4]) {
						cards[5] = i;
						cards[6] = j;
						hand[(myCardsValue->cardsValue(cards,0))/100000000]++;
						sum++;
					}
				}
				}
			}
			for(i=0;i<10;i++) {
				hand[i] = (int)(((double)hand[i]/(double)sum)*100.0+0.5);
			}

		} break;
		case GAME_STATE_TURN: {

			for(i=0;i<52;i++) {
				if(i!=cards[0] && i!=cards[1] && i!=cards[2] && i!=cards[3] && i!=cards[4] && i!=cards[5]) {
					cards[6] = i;
					hand[(myCardsValue->cardsValue(cards,0))/100000000]++;
					sum++;
				}
			}
			for(i=0;i<10;i++) {
				hand[i] = (int)(((double)hand[i]/(double)sum)*100.0+0.5);
			}

		} break;
		case GAME_STATE_RIVER: {
			hand[(myCardsValue->cardsValue(cards,0))/100000000] = 100;
		} break;
		default: {
		}
	}

	return hand;
}
