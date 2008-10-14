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

void Tools::getRandNumber(int start, int end, int howMany, int* randArray, bool different) {

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

double* Tools::calcCardsChance()
{
	double array[10] = { 30.12, 11.11, 100.00, 0.00, 0.00, 99.22, 78.56, 34.22, 67.22, 21.00 };
	return array;
}
