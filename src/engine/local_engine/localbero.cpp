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

#include "localbero.h"

#include "localexception.h"
#include "engine_msg.h"
#include <core/loghelper.h>

using namespace std;

LocalBeRo::LocalBeRo(HandInterface* hi, unsigned dP, int sB, GameState gS)
	: BeRoInterface(), myHand(hi), myBeRoID(gS), dealerPosition(dP), smallBlindPosition(0), dealerPositionId(dP), smallBlindPositionId(0), bigBlindPositionId(0), smallBlind(sB), highestSet(0), minimumRaise(2*sB), fullBetRule(false), firstRun(true), firstRunGui(true), firstRound(true), firstHeadsUpRound(true), currentPlayersTurnId(0), firstRoundLastPlayersTurnId(0), logBoardCardsDone(false)
{
	currentPlayersTurnIt = myHand->getRunningPlayerList()->begin();
	lastPlayersTurnIt = myHand->getRunningPlayerList()->begin();

	PlayerListConstIterator it_c;

	// determine bigBlindPosition
	for(it_c=myHand->getActivePlayerList()->begin(); it_c!=myHand->getActivePlayerList()->end(); ++it_c) {
		if((*it_c)->getMyButton() == BUTTON_BIG_BLIND) {
			bigBlindPositionId = (*it_c)->getMyUniqueID();
			break;
		}
	}
	if(it_c == myHand->getActivePlayerList()->end()) {
		throw LocalException(__FILE__, __LINE__, ERR_ACTIVE_PLAYER_NOT_FOUND);
	}

	// determine smallBlindPosition
	for(it_c=myHand->getActivePlayerList()->begin(); it_c!=myHand->getActivePlayerList()->end(); ++it_c) {
		if((*it_c)->getMyButton() == BUTTON_SMALL_BLIND) {
			smallBlindPositionId = (*it_c)->getMyUniqueID();
			break;
		}
	}
	if(it_c == myHand->getActivePlayerList()->end()) {
		throw LocalException(__FILE__, __LINE__, ERR_ACTIVE_PLAYER_NOT_FOUND);
	}

}


LocalBeRo::~LocalBeRo()
{
}

int LocalBeRo::getHighestCardsValue() const
{
	LOG_ERROR(__FILE__ << " (" << __LINE__ << "): getHighestCardsValue() in wrong BeRo");
	return 0;
}

void LocalBeRo::nextPlayer()
{

	PlayerListConstIterator currentPlayersTurnConstIt = myHand->getRunningPlayerIt(currentPlayersTurnId);
	if(currentPlayersTurnConstIt == myHand->getRunningPlayerList()->end()) {
		throw LocalException(__FILE__, __LINE__, ERR_RUNNING_PLAYER_NOT_FOUND);
	}

	(*currentPlayersTurnConstIt)->action();

}

void LocalBeRo::run()
{

	if(firstRunGui) {
		firstRunGui = false;
		myHand->setPreviousPlayerID(-1);
		myHand->getGuiInterface()->dealBeRoCards(myBeRoID);
	} else {

		if(firstRun) {

			firstRun = false;

			if(!(myHand->getAllInCondition())) {

				PlayerListIterator it_1, it_2;

				if(myHand->getActivePlayerList()->size() > 2) {

					// running player before smallBlind
					bool formerRunningPlayerFound = false;

					it_1 = myHand->getActivePlayerIt(smallBlindPositionId);
					if(it_1 == myHand->getActivePlayerList()->end()) {
						throw LocalException(__FILE__, __LINE__, ERR_ACTIVE_PLAYER_NOT_FOUND);
					}

					for(size_t i=0; i<myHand->getActivePlayerList()->size(); i++) {

						if(it_1 == myHand->getActivePlayerList()->begin()) it_1 = myHand->getActivePlayerList()->end();
						--it_1;

						it_2 = myHand->getRunningPlayerIt((*it_1)->getMyUniqueID());
						// running player found
						if(it_2 != myHand->getRunningPlayerList()->end()) {
							firstRoundLastPlayersTurnId = (*it_2)->getMyUniqueID();
							formerRunningPlayerFound = true;
							break;
						}
					}
					if(!formerRunningPlayerFound) {
						throw LocalException(__FILE__, __LINE__, ERR_FORMER_RUNNING_PLAYER_NOT_FOUND);
					}
				}
				// heads up: bigBlind begins -> dealer/smallBlind is running player before bigBlind
				else {
					firstRoundLastPlayersTurnId = smallBlindPositionId;
				}
				currentPlayersTurnId = firstRoundLastPlayersTurnId;
			}
		}

		//log the turned cards
		if(!logBoardCardsDone) {

			int tempBoardCardsArray[5];

			myHand->getBoard()->getMyCards(tempBoardCardsArray);

			switch(myBeRoID) {
			case GAME_STATE_FLOP:
				myHand->getGuiInterface()->logDealBoardCardsMsg(myBeRoID, tempBoardCardsArray[0], tempBoardCardsArray[1], tempBoardCardsArray[2]);
				break;
			case GAME_STATE_TURN:
				myHand->getGuiInterface()->logDealBoardCardsMsg(myBeRoID, tempBoardCardsArray[0], tempBoardCardsArray[1], tempBoardCardsArray[2], tempBoardCardsArray[3]);
				break;
			case GAME_STATE_RIVER:
				myHand->getGuiInterface()->logDealBoardCardsMsg(myBeRoID, tempBoardCardsArray[0], tempBoardCardsArray[1], tempBoardCardsArray[2], tempBoardCardsArray[3], tempBoardCardsArray[4]);

				break;
			default: {
				LOG_ERROR(__FILE__ << " (" << __LINE__ << "): ERROR - wrong myBeRoID");
			}
			}
			if(myHand->getLog()) myHand->getLog()->logBoardCards(tempBoardCardsArray);
			logBoardCardsDone = true;

		}

		bool allHighestSet = true;

		PlayerListIterator it_c;


		// check if all running players have same sets (else allHighestSet = false)
		for( it_c = myHand->getRunningPlayerList()->begin(); it_c != myHand->getRunningPlayerList()->end(); ++it_c) {
			if(highestSet != (*it_c)->getMySet()) {
				allHighestSet = false;
				break;
			}
		}

		// prfen, ob aktuelle bero wirklich dran ist
		if(!firstRound && allHighestSet) {

			// aktuelle bero nicht dran, weil alle Sets gleich sind
			//also gehe in naechste bero
			myHand->setCurrentRound(GameState(myBeRoID+1));

			//Action loeschen und ActionButtons refresh
			for(it_c=myHand->getRunningPlayerList()->begin(); it_c!=myHand->getRunningPlayerList()->end(); ++it_c) {
				(*it_c)->setMyAction(PLAYER_ACTION_NONE);
			}

			//Sets in den Pot verschieben und Sets = 0 und Pot-refresh
			myHand->getBoard()->collectSets();
			myHand->getBoard()->collectPot();
			myHand->getGuiInterface()->refreshPot();

			myHand->getGuiInterface()->refreshSet();
			myHand->getGuiInterface()->refreshCash();
			for(int i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
				myHand->getGuiInterface()->refreshAction(i,PLAYER_ACTION_NONE);
			}

			myHand->switchRounds();
		} else {
			// aktuelle bero ist wirklich dran

			// determine next running player
			PlayerListConstIterator currentPlayersTurnIt = myHand->getRunningPlayerIt( currentPlayersTurnId );
			if(currentPlayersTurnIt == myHand->getRunningPlayerList()->end()) {
				throw LocalException(__FILE__, __LINE__, ERR_RUNNING_PLAYER_NOT_FOUND);
			}

			++currentPlayersTurnIt;
			if(currentPlayersTurnIt == myHand->getRunningPlayerList()->end()) currentPlayersTurnIt = myHand->getRunningPlayerList()->begin();

			currentPlayersTurnId = (*currentPlayersTurnIt)->getMyUniqueID();

			//highlight active players groupbox and clear action
			myHand->getGuiInterface()->refreshGroupbox(currentPlayersTurnId,2);
			myHand->getGuiInterface()->refreshAction(currentPlayersTurnId,0);

			currentPlayersTurnIt = myHand->getRunningPlayerIt( currentPlayersTurnId );
			if(currentPlayersTurnIt == myHand->getRunningPlayerList()->end()) {
				throw LocalException(__FILE__, __LINE__, ERR_RUNNING_PLAYER_NOT_FOUND);
			}

			(*currentPlayersTurnIt)->setMyTurn(true);


			if( currentPlayersTurnId == firstRoundLastPlayersTurnId ) {
				firstRound = false;
			}

			if( currentPlayersTurnId == 0) {
				// Wir sind dran
				myHand->getGuiInterface()->meInAction();
			} else {

				//Gegner sind dran
				myHand->getGuiInterface()->beRoAnimation2(myBeRoID);
			}

		}
	}
}
