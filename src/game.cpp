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

#include <iostream>
#include <sstream>

using namespace std;

Game::Game(GuiInterface* gui, boost::shared_ptr<EngineFactory> factory,
		   const PlayerDataList &playerDataList, const GameData &gameData,
		   const StartData &startData, int gameId)
: myFactory(factory), myGui(gui), actualHand(0), actualBoard(0),
  startQuantityPlayers(startData.numberOfPlayers),
  startCash(gameData.startMoney), startSmallBlind(gameData.smallBlind),
  startHandsBeforeRaiseSmallBlind(gameData.handsBeforeRaise),
  myGameID(gameId), actualSmallBlind(gameData.smallBlind), actualHandID(0), dealerPosition(0), lastHandBlindsRaised(1), lastTimeBlindsRaised(0), myGameData(gameData)
{
	if(DEBUG_MODE) {
		startSmallBlind = 20;
		actualSmallBlind = startSmallBlind;
	}

// 	cout << "Create Game Object" << "\n";
	int i;

	actualHandID = 0;

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
	assert(player_i != player_end);
	dealerPosition = startData.startDealerPlayerId;

	// Board erstellen
	actualBoard = myFactory->createBoard();

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
		boost::shared_ptr<PlayerInterface> tmpPlayer = myFactory->createPlayer(actualBoard, i, uniqueId, type, myName, myAvatarFile, startCash, startQuantityPlayers > i, 0);

		tmpPlayer->setNetSessionData(myNetSession);

		// fill player lists
		playerArray.push_back(tmpPlayer); // delete
		seatsList->push_back(tmpPlayer);
		if(startQuantityPlayers > i) {
			activePlayerList->push_back(tmpPlayer);
		}

		(*runningPlayerList) = (*activePlayerList);

		playerArray[i]->setNetSessionData(myNetSession); // delete
		
	}
	actualBoard->setPlayerLists(playerArray, seatsList, activePlayerList, runningPlayerList); // delete playerArray

	//start timer
	blindsTimer.reset();
	blindsTimer.start();
}

Game::~Game()
{
// 	cout << "Delete Game Object" << "\n";

	delete actualBoard;
	actualBoard = 0;
	delete actualHand;
	actualHand = 0;

}

HandInterface *Game::getCurrentHand()
{
	return actualHand;
}

const HandInterface *Game::getCurrentHand() const
{
	return actualHand;
}

void Game::initHand()
{

	size_t i;
	PlayerListConstIterator it_c;
	PlayerListIterator it, it_2;

	actualHandID++;

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
	if(actualHand) {

// TODO HACK
		for(it=seatsList->begin(); it!=seatsList->end(); it++) {
			if (!(*it)->getMyActiveStatus())
			{
				if ((*it)->getMyUniqueID() == getCurrentHand()->getCurrentBeRo()->getCurrentPlayersTurnId())
				{
					it_2 = it;
					it_2++;
					if (it_2 == seatsList->end())
						it_2 = seatsList->begin();
					getCurrentHand()->getCurrentBeRo()->setCurrentPlayersTurnId((*it_2)->getMyUniqueID());
				}
			}
		}

		delete actualHand;
		actualHand = 0;
	}

	runningPlayerList->clear();
	(*runningPlayerList) = (*activePlayerList);

	// Hand erstellen
	actualHand = myFactory->createHand(myFactory, myGui, actualBoard, playerArray, seatsList, activePlayerList, runningPlayerList, actualHandID, startQuantityPlayers, dealerPosition, actualSmallBlind, startCash); // delete playerArray


	// Dealer-Button weiterschieben --> Achtung inactive -> TODO exception-rule !!! DELETE
// 	for(i=0; (i<MAX_NUMBER_OF_PLAYERS && !(playerArray[dealerPosition]->getMyActiveStatus())) || i==0; i++) {
// 	
// 		dealerPosition = (dealerPosition+1)%(MAX_NUMBER_OF_PLAYERS);
// 	}

	// Dealer-Button weiterschieben --> Achtung inactive -> TODO exception-rule !!!
	bool nextDealerFound = false;
	PlayerListConstIterator dealerPositionIt = actualHand->getSeatIt(dealerPosition);
	assert( dealerPositionIt != seatsList->end() );

	for(i=0; i<seatsList->size(); i++) {

		dealerPositionIt++;
		if(dealerPositionIt == seatsList->end()) dealerPositionIt = seatsList->begin();

		it = actualHand->getActivePlayerIt( (*dealerPositionIt)->getMyUniqueID() );
		if(it != activePlayerList->end() ) {
			nextDealerFound = true;
			dealerPosition = (*it)->getMyUniqueID();
			break;
		}

	}
	assert(nextDealerFound);


}

void Game::startHand()
{
	assert(actualHand);

	//GUI bereinigen 
	myGui->nextRoundCleanGui();

	//Log new Hand
	myGui->logNewGameHandMsg(myGameID, actualHandID);
	//Log blinds sets for new Hand
	PlayerListConstIterator it_sB, it_bB;
	it_sB = actualHand->getActivePlayerIt(actualHand->getCurrentBeRo()->getSmallBlindPositionId());
	it_bB = actualHand->getActivePlayerIt(actualHand->getCurrentBeRo()->getBigBlindPositionId());
	if(it_sB != actualHand->getActivePlayerList()->end() && it_bB != actualHand->getActivePlayerList()->end()) {
		myGui->logNewBlindsSetsMsg(myGameID, (*it_sB)->getMySet(), (*it_bB)->getMySet(), (*it_sB)->getMyName().c_str(), (*it_bB)->getMyName().c_str());
	}	
	else { cout << "Log Error: cannot find sBID or bBID" << "\n"; }
	
	actualHand->start();
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
	assert(tmpPlayer.get());
	return tmpPlayer;
}

void Game::raiseBlinds() {

// 	cout << "timer minutes " << blindsTimer.elapsed().total_seconds()/60 << "\n";
// 	cout << "gameData.RaiseIntervalMode " << myGameData.RaiseIntervalMode << "\n";
// 	cout << "gameData.raiseMode " << myGameData.raiseMode << "\n";
// 	cout << "gameData.afterManualBlindsMode " << myGameData.afterManualBlindsMode << "\n";

	bool raiseBlinds = false;

	if (myGameData.raiseIntervalMode == RAISE_ON_HANDNUMBER) {

		if (lastHandBlindsRaised + myGameData.raiseSmallBlindEveryHandsValue <= actualHandID) {
			raiseBlinds = true;
			lastHandBlindsRaised = actualHandID;

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
			actualSmallBlind *= 2; 
// 			cout << "double small blind \n";
		}	
		else {
			// Increase the position of the list
			list<int> blindsList = myGameData.manualBlindsList;
			list<int>::iterator it;
			
			if(!blindsList.empty()) {
				it = find(blindsList.begin(), blindsList.end(), actualSmallBlind);
				if(it !=  blindsList.end()) { 
					it++;
// 					cout << "increase position in blindslist \n";
				}
				else { 
// 					cout << "blindslist exceeds\n"; 
					if(actualSmallBlind == myGameData.firstSmallBlind) {
						it = blindsList.begin();
					}
				}	
			}
// 			else {	cout << "blindslist is empty \n"; }
			// Check if we can get an element of the list or the position exceeds the lis
			if (blindsList.empty() || it ==  blindsList.end()) {
				
				// The position exceeds the list
				if (myGameData.afterManualBlindsMode == AFTERMB_DOUBLE_BLINDS) { 
					actualSmallBlind *= 2; 
// 					cout << "after blindslist double blind\n";
				}
				else {
					if(myGameData.afterManualBlindsMode == AFTERMB_RAISE_ABOUT) { 
						actualSmallBlind += myGameData.afterMBAlwaysRaiseValue;
// 						cout << "after blindslist increase about x \n";
					}
// 					else { /* Stay at last blind */ cout << "after blindslist stay at last blind \n"; }
				}				
			} else {
				// Grab the blinds amount from the list
				actualSmallBlind = *it;
// 				cout << "set new small blind from blindslist \n";
			}
		}
	}
}
