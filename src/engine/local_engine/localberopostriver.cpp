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

#include "localberopostriver.h"
#include <handinterface.h>
#include <game_defs.h>

#include <iostream>

using namespace std;

LocalBeRoPostRiver::LocalBeRoPostRiver(HandInterface* hi, int dP, int sB) : LocalBeRo(hi, dP, sB, GAME_STATE_POST_RIVER), highestCardsValue(0)
{
}

LocalBeRoPostRiver::~LocalBeRoPostRiver()
{
}

void LocalBeRoPostRiver::run()
{
}

void LocalBeRoPostRiver::postRiverRun()
{

	PlayerListConstIterator it_c;
	PlayerListIterator it;

	// who is the winner
	for(it_c=getMyHand()->getActivePlayerList()->begin(); it_c!=getMyHand()->getActivePlayerList()->end(); ++it_c) {

		if( (*it_c)->getMyAction() != PLAYER_ACTION_FOLD && (*it_c)->getMyCardsValueInt() > highestCardsValue ) {
			highestCardsValue = (*it_c)->getMyCardsValueInt();
		}
	}

	int potPlayers = 0;

	for(it_c=getMyHand()->getActivePlayerList()->begin(); it_c!=getMyHand()->getActivePlayerList()->end(); ++it_c) {
		if( (*it_c)->getMyAction() != PLAYER_ACTION_FOLD) {
			potPlayers++;
		}
	}

	// prüfen ob nur noch human player an der verteilung teilnimmt und myAggressive für human player setzen
	if(potPlayers == 1) {
		for(it=getMyHand()->getActivePlayerList()->begin(); it!=getMyHand()->getActivePlayerList()->end(); ++it) {
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
	getMyHand()->getBoard()->distributePot(getDealerPosition());

	//Pot auf 0 setzen
	getMyHand()->getBoard()->setPot(0);

	// logging
	int nonfoldPlayersCounter = 0;
	for (it_c=getMyHand()->getActivePlayerList()->begin(); it_c!=getMyHand()->getActivePlayerList()->end(); ++it_c) {
		if ((*it_c)->getMyAction() != PLAYER_ACTION_FOLD) nonfoldPlayersCounter++;
	}
	if(nonfoldPlayersCounter>1) {
		if(getMyHand()->getLog()) getMyHand()->getLog()->logHoleCardsHandName(getMyHand()->getActivePlayerList());
	}
	if(getMyHand()->getLog()) {
		getMyHand()->getLog()->logHandWinner(getMyHand()->getActivePlayerList(), highestCardsValue, getMyHand()->getBoard()->getWinners());
		getMyHand()->getLog()->logPlayerSitsOut(getMyHand()->getActivePlayerList());
		getMyHand()->getLog()->logGameWinner(getMyHand()->getActivePlayerList());
		getMyHand()->getLog()->logAfterHand();
	}

	//starte die Animaionsreihe
	getMyHand()->getGuiInterface()->postRiverRunAnimation1();
}
