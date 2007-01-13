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
#ifndef LOCALBOARD_H
#define LOCALBOARD_H

#include <iostream>

class LocalPlayer;
class LocalHand;


class LocalBoard{
public:
    LocalBoard();

    ~LocalBoard();

	void setPlayer(LocalPlayer**);
	void setHand(LocalHand*);

	void setMyCards(int* theValue) { int i; for(i=0; i<5; i++) myCards[i] = theValue[i]; }
	void getMyCards(int* theValue) { int i; for(i=0; i<5; i++) theValue[i] = myCards[i]; }

	int getPot() const {  return pot;}
	void setPot(int theValue) {  pot = theValue;}
	int getSets() const { return sets; }

	void collectSets();
	void collectPot();
	

private:
	LocalPlayer **playerArray;
	LocalHand *actualHand;

	int myCards[5];
	int pot;
	int sets;

};

#endif
