/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2011 Felix Hammer, Florian Thauer, Lothar May          *
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
	getMyHand()->getBoard()->distributePot();

	//Pot auf 0 setzen
	getMyHand()->getBoard()->setPot(0);

	// logging hole Cards / Hands
	int nonfoldPlayersCounter = 0;
	for (it_c=getMyHand()->getActivePlayerList()->begin(); it_c!=getMyHand()->getActivePlayerList()->end(); ++it_c) {
		if ((*it_c)->getMyAction() != PLAYER_ACTION_FOLD) nonfoldPlayersCounter++;
	}
	if(nonfoldPlayersCounter>1) {
		getMyHand()->getLog()->logHoleCardsHandName(5,getMyHand()->getActivePlayerList());
	}

	// logging winner of the hand
	for(it_c=getMyHand()->getActivePlayerList()->begin(); it_c!=getMyHand()->getActivePlayerList()->end(); ++it_c) {
		if((*it_c)->getMyAction() != PLAYER_ACTION_FOLD && (*it_c)->getMyCardsValueInt() == highestCardsValue) {
			getMyHand()->getLog()->logPlayerAction(5,(*it_c)->getMyID()+1,LOG_ACTION_WIN,(*it_c)->getLastMoneyWon());
		}
	}

	// log side pot winners
	list<unsigned> winners = getMyHand()->getBoard()->getWinners();
	list<unsigned>::iterator it_int;
	for(it_c=getMyHand()->getActivePlayerList()->begin(); it_c!=getMyHand()->getActivePlayerList()->end(); ++it_c) {
		if((*it_c)->getMyAction() != PLAYER_ACTION_FOLD && (*it_c)->getMyCardsValueInt() != highestCardsValue ) {

			for(it_int = winners.begin(); it_int != winners.end(); ++it_int) {
				if((*it_int) == (*it_c)->getMyUniqueID()) {
					getMyHand()->getLog()->logPlayerAction(5,(*it_c)->getMyID()+1,LOG_ACTION_WIN_SIDE_POT,(*it_c)->getLastMoneyWon());
				}
			}
		}
	}

	// log player sits out
	for(it_c=getMyHand()->getActivePlayerList()->begin(); it_c!=getMyHand()->getActivePlayerList()->end(); ++it_c) {
		if((*it_c)->getMyCash() == 0) {
			getMyHand()->getLog()->logPlayerAction(5, (*it_c)->getMyID()+1, LOG_ACTION_SIT_OUT);
		}
	}

	// for log after every hand
	getMyHand()->getLog()->logAfterHand();

	// log winner of the game if only one player is left
	int playersPositiveCashCounter = 0;
	for (it_c=getMyHand()->getActivePlayerList()->begin(); it_c!=getMyHand()->getActivePlayerList()->end(); ++it_c) {
		if ((*it_c)->getMyCash() > 0) playersPositiveCashCounter++;
	}
	if (playersPositiveCashCounter==1) {
		for (it_c=getMyHand()->getActivePlayerList()->begin(); it_c!=getMyHand()->getActivePlayerList()->end(); ++it_c) {
			if ((*it_c)->getMyCash() > 0) {
				getMyHand()->getLog()->logPlayerAction(5,(*it_c)->getMyID()+1,LOG_ACTION_WIN_GAME);
			}
		}
		// for log after every game
		getMyHand()->getLog()->logAfterGame();
	}

	//starte die Animaionsreihe
	getMyHand()->getGuiInterface()->postRiverRunAnimation1();
}
