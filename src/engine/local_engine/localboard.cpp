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
#include "localboard.h"

#include "handinterface.h"
#include <game_defs.h>

using namespace std;

LocalBoard::LocalBoard() : BoardInterface(), playerArray(0), actualHand(0), pot(0), sets(0)
{
}


LocalBoard::~LocalBoard()
{
}

void LocalBoard::setPlayer(PlayerInterface** p) { playerArray = p; }

void LocalBoard::setHand(HandInterface* br) { actualHand = br; }

void LocalBoard::collectSets() {

	sets = 0;
	int i;
	for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) sets += playerArray[i]->getMySet();

}

void LocalBoard::collectPot() { 
	int i;
	pot += sets; 
	sets = 0;
	for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++){ playerArray[i]->setMySetNull(); }
}

void LocalBoard::distributePot() {

	vector<int> playerSets;

}

