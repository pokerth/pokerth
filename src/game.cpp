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
#include "game.h"

#include <enginefactory.h>
#include <guiinterface.h>

#include "localexception.h"
#include "engine_msg.h"

#include <iostream>

using namespace std;

Game::Game(GuiInterface* gui, boost::shared_ptr<EngineFactory> factory,
		   const PlayerDataList &playerDataList, const GameData &gameData,
		   const StartData &startData, int gameId)
: myFactory(factory), myGui(gui), currentHand(0), currentBoard(0),
  startQuantityPlayers(startData.numberOfPlayers),
  startCash(gameData.startMoney), startSmallBlind(gameData.firstSmallBlind),
  myGameID(gameId), currentSmallBlind(gameData.firstSmallBlind), currentHandID(0), dealerPosition(0), lastHandBlindsRaised(1), lastTimeBlindsRaised(0), myGameData(gameData)
{

	blindsList = myGameData.manualBlindsList;


	if(DEBUG_MODE) {
		startSmallBlind = 10;
		currentSmallBlind = startSmallBlind;
	}

// 	cout << "Create Game Object" << "\n";
	int i;

	currentHandID = 0;

// 	for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
// 		playerArray[i] = 0;
// 	}

	// Dealer Position bestimmen
	PlayerDataList::const_iterator player_i = playerDataList.begin();
	PlayerDataList::const_iterator player_end = playerDataList.end();

	while (player_i != player_end)
	{
		if ((*player_i)->GetUniqueId() == startData.startDealerPlayerId)
			break;
		++player_i;
	}
	if (player_i == player_end)
		throw LocalException(__FILE__, __LINE__, ERR_DEALER_NOT_FOUND);
	dealerPosition = startData.startDealerPlayerId;

	// Board erstellen
	currentBoard = myFactory->createBoard();

	seatsList = PlayerList(new std::list<boost::shared_ptr<PlayerInterface> >);
	activePlayerList = PlayerList(new std::list<boost::shared_ptr<PlayerInterface> >);
	runningPlayerList = PlayerList(new std::list<boost::shared_ptr<PlayerInterface> >);

	// Player erstellen

	player_i = playerDataList.begin();
	player_end = playerDataList.end();
	for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {

		string myName;
		string myAvatarFile;
		unsigned uniqueId = 0;
		PlayerType type = PLAYER_TYPE_COMPUTER;
		boost::shared_ptr<SessionData> myNetSession;

		if (player_i != player_end)
		{
			uniqueId = (*player_i)->GetUniqueId();
			type = (*player_i)->GetType();
			myName = (*player_i)->GetName();
			myAvatarFile = (*player_i)->GetAvatarFile();
			myNetSession = (*player_i)->GetNetSessionData();
			// TODO: set player type
			++player_i;
		}

		// create Player Objects
		boost::shared_ptr<PlayerInterface> tmpPlayer = myFactory->createPlayer(currentBoard, i, uniqueId, type, myName, myAvatarFile, startCash, startQuantityPlayers > i, 0);

		tmpPlayer->setNetSessionData(myNetSession);

		// fill player lists
// 		playerArray.push_back(tmpPlayer); // delete
		seatsList->push_back(tmpPlayer);
		if(startQuantityPlayers > i) {
			activePlayerList->push_back(tmpPlayer);
		}

		(*runningPlayerList) = (*activePlayerList);

// 		playerArray[i]->setNetSessionData(myNetSession); // delete
		
	}
	currentBoard->setPlayerLists(/*playerArray,*/ seatsList, activePlayerList, runningPlayerList); // delete playerArray

	//start timer
	blindsTimer.reset();
	blindsTimer.start();
}

Game::~Game()
{
// 	cout << "Delete Game Object" << "\n";

	delete currentBoard;
	currentBoard = 0;
	delete currentHand;
	currentHand = 0;

}

HandInterface *Game::getCurrentHand()
{
	return currentHand;
}

const HandInterface *Game::getCurrentHand() const
{
	return currentHand;
}

void Game::initHand()
{

	size_t i;
	PlayerListConstIterator it_c;
	PlayerListIterator it, it_2;

	currentHandID++;

	// calculate smallBlind
	raiseBlinds();

	//Spieler Action auf 0 setzen
// 	for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
// 		playerArray[i]->setMyAction(0);
// 	}

	//Spieler Action auf 0 setzen
	for(it_c=seatsList->begin(); it_c!=seatsList->end(); it_c++) {
		(*it_c)->setMyAction(PLAYER_ACTION_NONE);
	}

	// Spieler mit leerem Cash auf inactive setzen
	it = activePlayerList->begin();

	while( it!=activePlayerList->end() ) {

		if((*it)->getMyCash() == 0) {
// 			cout << "playerID: " << (*it)->getMyID() << endl;
			(*it)->setMyActiveStatus(0);
			it = activePlayerList->erase(it);
		} else {
			it++;
		}
	}

	// eventuell vorhandene Hand löschen
	if(currentHand) {
		delete currentHand;
		currentHand = 0;
	}

	runningPlayerList->clear();
	(*runningPlayerList) = (*activePlayerList);

	// Hand erstellen
	currentHand = myFactory->createHand(myFactory, myGui, currentBoard, /*playerArray,*/ seatsList, activePlayerList, runningPlayerList, currentHandID, startQuantityPlayers, dealerPosition, currentSmallBlind, startCash); // delete playerArray


	// Dealer-Button weiterschieben --> Achtung inactive -> TODO exception-rule !!! DELETE
// 	for(i=0; (i<MAX_NUMBER_OF_PLAYERS && !(playerArray[dealerPosition]->getMyActiveStatus())) || i==0; i++) {
// 	
// 		dealerPosition = (dealerPosition+1)%(MAX_NUMBER_OF_PLAYERS);
// 	}

	// Dealer-Button weiterschieben --> Achtung inactive -> TODO exception-rule !!!
	bool nextDealerFound = false;
	PlayerListConstIterator dealerPositionIt = currentHand->getSeatIt(dealerPosition);
	if(dealerPositionIt == seatsList->end()) {
		throw LocalException(__FILE__, __LINE__, ERR_SEAT_NOT_FOUND);
	}

	for(i=0; i<seatsList->size(); i++) {

		dealerPositionIt++;
		if(dealerPositionIt == seatsList->end()) dealerPositionIt = seatsList->begin();

		it = currentHand->getActivePlayerIt( (*dealerPositionIt)->getMyUniqueID() );
		if(it != activePlayerList->end() ) {
			nextDealerFound = true;
			dealerPosition = (*it)->getMyUniqueID();
			break;
		}

	}
	if(!nextDealerFound) {
		throw LocalException(__FILE__, __LINE__, ERR_NEXT_DEALER_NOT_FOUND);
	}


}

void Game::startHand()
{
	//GUI bereinigen 
	myGui->nextRoundCleanGui();

	//Log new Hand
	myGui->logNewGameHandMsg(myGameID, currentHandID);
	myGui->flushLogAtGame(myGameID);	

	currentHand->start();
}

boost::shared_ptr<PlayerInterface> Game::getPlayerByUniqueId(unsigned id)
{
	boost::shared_ptr<PlayerInterface> tmpPlayer;
	PlayerListIterator i = getSeatsList()->begin();
	PlayerListIterator end = getSeatsList()->end();
	while (i != end)
	{
		if ((*i)->getMyUniqueID() == id)
		{
			tmpPlayer = *i;
			break;
		}
		++i;
	}
	return tmpPlayer;
}

boost::shared_ptr<PlayerInterface> Game::getCurrentPlayer()
{
	boost::shared_ptr<PlayerInterface> tmpPlayer = getPlayerByUniqueId(getCurrentHand()->getCurrentBeRo()->getCurrentPlayersTurnId());
	if (!tmpPlayer.get())
		throw LocalException(__FILE__, __LINE__, ERR_CURRENT_PLAYER_NOT_FOUND);
	return tmpPlayer;
}

void Game::raiseBlinds() {

// 	cout << "timer minutes " << blindsTimer.elapsed().total_seconds()/60 << "\n";
// 	cout << "gameData.RaiseIntervalMode " << myGameData.RaiseIntervalMode << "\n";
// 	cout << "gameData.raiseMode " << myGameData.raiseMode << "\n";
// 	cout << "gameData.afterManualBlindsMode " << myGameData.afterManualBlindsMode << "\n";

	bool raiseBlinds = false;

	if (myGameData.raiseIntervalMode == RAISE_ON_HANDNUMBER) {

		if (lastHandBlindsRaised + myGameData.raiseSmallBlindEveryHandsValue <= currentHandID) {
			raiseBlinds = true;
			lastHandBlindsRaised = currentHandID;

// 			cout << "raise now on hands \n";
		}
	}
	else {
		if (lastTimeBlindsRaised + myGameData.raiseSmallBlindEveryMinutesValue <= blindsTimer.elapsed().total_seconds()/60) {
			raiseBlinds = true;
			lastTimeBlindsRaised = blindsTimer.elapsed().total_seconds()/60;

// 			cout << "raise now on minutes \n";
		}
	}

	if (raiseBlinds) {

		// At this point, the blinds must be raised
		// Now we check how the blinds should be raised
	
		if (myGameData.raiseMode == DOUBLE_BLINDS) { 
			currentSmallBlind *= 2; 
// 			cout << "double small blind \n";
		}	
		else {
			// Increase the position of the list
/*			list<int> blindsList = myGameData.manualBlindsList;*/
// 			list<int>::iterator it;
			
			if(!blindsList.empty()) {

				currentSmallBlind = blindsList.front();
				blindsList.pop_front();

// 				it = find(blindsList.begin(), blindsList.end(), currentSmallBlind);
// 				if(it !=  blindsList.end()) { 
// 					it++;
// 					cout << "increase position in blindslist \n";
// 				}
// 				else { 
// 					cout << "blindslist exceeds\n"; 
// 					if(currentSmallBlind == myGameData.firstSmallBlind) {
// 						it = blindsList.begin();
// 					}
// 				}	
			}
// 			else {	cout << "blindslist is empty \n"; }
			// Check if we can get an element of the list or the position exceeds the lis
// 			if (blindsList.empty() || it ==  blindsList.end()) {

			else {
				
				// The position exceeds the list
				if (myGameData.afterManualBlindsMode == AFTERMB_DOUBLE_BLINDS) { 
					currentSmallBlind *= 2; 
// 					cout << "after blindslist double blind\n";
				}
				else {
					if(myGameData.afterManualBlindsMode == AFTERMB_RAISE_ABOUT) { 
						currentSmallBlind += myGameData.afterMBAlwaysRaiseValue;
// 						cout << "after blindslist increase about x \n";
					}
// 					else { /* Stay at last blind */ cout << "after blindslist stay at last blind \n"; }
				}				
// 			} else {
				// Grab the blinds amount from the list
// 				currentSmallBlind = *it;
// 				cout << "set new small blind from blindslist \n";
			}
		}
	}
}
