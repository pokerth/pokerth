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
#include "tools.h"

using namespace std;

Tools::Tools()
{
}


Tools::~Tools()
{
}


void Tools::getRandNumber(int start, int end, int howMany, int* randArray, bool different) {

	double r = end-start+1;

	if (!different) {
			
		int i;
		for (i=0; i<howMany; i++) {
			
			randArray[i] = start+(int)(r*rand()/(RAND_MAX+1.0));
			
		}
	}
	else {

		
		int tempArray[end-start];
		int i;
		for (i=0; i<(end-start); i++) tempArray[i]=1;
		
		int temp;
		int counter(0);
		while (counter < howMany) {

			temp = (int)(r*rand()/(RAND_MAX+1.0));
			if (tempArray[temp] == 1) { 

				randArray[counter] = temp+start; 
				tempArray[temp] = 0;
				counter++;
			}
			else {	continue; }
		
			
		}

	}

}
