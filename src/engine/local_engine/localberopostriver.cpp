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
#include "localberopostriver.h"
#include <handinterface.h>
#include <game_defs.h>

#include <iostream>

using namespace std;

LocalBeRoPostRiver::LocalBeRoPostRiver(HandInterface* hi, int id, int dP, int sB) : LocalBeRo(hi, id, dP, sB, GAME_STATE_POST_RIVER), highestCardsValue(0)
{
}

LocalBeRoPostRiver::~LocalBeRoPostRiver()
{
}

void LocalBeRoPostRiver::run() {
}

void LocalBeRoPostRiver::postRiverRun() {

	PlayerListConstIterator it_c;
	PlayerListIterator it;

	// who is the winner
	for(it_c=getMyHand()->getActivePlayerList()->begin(); it_c!=getMyHand()->getActivePlayerList()->end(); it_c++) {

		if( (*it_c)->getMyAction() != PLAYER_ACTION_FOLD && (*it_c)->getMyCardsValueInt() > highestCardsValue ) { 
			highestCardsValue = (*it_c)->getMyCardsValueInt(); 
		}
	}

	int potPlayers = 0;

	for(it_c=getMyHand()->getActivePlayerList()->begin(); it_c!=getMyHand()->getActivePlayerList()->end(); it_c++) {
		if( (*it_c)->getMyAction() != PLAYER_ACTION_FOLD) {
			potPlayers++;
		}
	}

	// prüfen ob nur noch human player an der verteilung teilnimmt und myAggressive für human player setzen
	if(potPlayers == 1) {
		for(it=getMyHand()->getActivePlayerList()->begin(); it!=getMyHand()->getActivePlayerList()->end(); it++) {
			if( (*it)->getMyAction() != PLAYER_ACTION_FOLD) {
				(*it)->setMyAggressive(true);
			}
		}
	}

// 	it = getMyHand()->getActivePlayerIt(0);
// 	if( it != getMyHand()->getActivePlayerList()->end() ) {
// 		if( potPlayers == 1 && (*it)->getMyAction() != PLAYER_ACTION_FOLD ) {
// 			(*it)->setMyAggressive(true);
// 		}
// 	}

	// Spieler ermitteln, welche die Karten auf jeden Fall umdrehen müssen
	getMyHand()->getBoard()->determinePlayerNeedToShowCards();

	// Pot-Verteilung
	getMyHand()->getBoard()->distributePot();

	//Pot auf 0 setzen
	getMyHand()->getBoard()->setPot(0);

	//starte die Animaionsreihe
	getMyHand()->getGuiInterface()->postRiverRunAnimation1();	
}
