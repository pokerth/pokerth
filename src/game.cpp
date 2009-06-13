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
		startSmallBlind = 320;
		currentSmallBlind = startSmallBlind;
	}

	int i;

	// determine dealer position
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

	// create board
	currentBoard = myFactory->createBoard();

	// create player lists
	seatsList = PlayerList(new std::list<boost::shared_ptr<PlayerInterface> >);
	activePlayerList = PlayerList(new std::list<boost::shared_ptr<PlayerInterface> >);
	runningPlayerList = PlayerList(new std::list<boost::shared_ptr<PlayerInterface> >);

	// create player
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
			++player_i;
		}

		// create player objects
		boost::shared_ptr<PlayerInterface> tmpPlayer = myFactory->createPlayer(currentBoard, i, uniqueId, type, myName, myAvatarFile, startCash, startQuantityPlayers > i, 0);

		tmpPlayer->setNetSessionData(myNetSession);

		// fill player lists
		seatsList->push_back(tmpPlayer);
		if(startQuantityPlayers > i) {
			activePlayerList->push_back(tmpPlayer);
		}
		(*runningPlayerList) = (*activePlayerList);

	}

	currentBoard->setPlayerLists(seatsList, activePlayerList, runningPlayerList);

	//start timer
	blindsTimer.reset();
	blindsTimer.start();
}

Game::~Game()
{
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
	PlayerListIterator it;

	currentHandID++;

	// calculate smallBlind
	raiseBlinds();

	// set player action none
	for(it=seatsList->begin(); it!=seatsList->end(); it++) {
		(*it)->setMyAction(PLAYER_ACTION_NONE);
	}

	// set player with empty cash inactive
	it = activePlayerList->begin();
	while( it!=activePlayerList->end() ) {

		if((*it)->getMyCash() == 0) {
                        (*it)->setMyActiveStatus(false);
			it = activePlayerList->erase(it);
		} else {
			it++;
		}
	}

	// delete possible existing hands
	if(currentHand) {
		delete currentHand;
		currentHand = 0;
	}

	runningPlayerList->clear();
	(*runningPlayerList) = (*activePlayerList);

	// create Hand
	currentHand = myFactory->createHand(myFactory, myGui, currentBoard, seatsList, activePlayerList, runningPlayerList, currentHandID, startQuantityPlayers, dealerPosition, currentSmallBlind, startCash);

	// shifting dealer button -> TODO exception-rule !!!
	bool nextDealerFound = false;
	PlayerListConstIterator dealerPositionIt = currentHand->getSeatIt(dealerPosition);
	if(dealerPositionIt == seatsList->end()) {
		throw LocalException(__FILE__, __LINE__, ERR_SEAT_NOT_FOUND);
	}

	for(i=0; i<seatsList->size(); i++) {

		dealerPositionIt++;
		if(dealerPositionIt == seatsList->end()) dealerPositionIt = seatsList->begin();

		it_c = currentHand->getActivePlayerIt( (*dealerPositionIt)->getMyUniqueID() );
		if(it_c != activePlayerList->end() ) {
			nextDealerFound = true;
			dealerPosition = (*it_c)->getMyUniqueID();
			break;
		}
	}
	if(!nextDealerFound) {
		throw LocalException(__FILE__, __LINE__, ERR_NEXT_DEALER_NOT_FOUND);
	}
}

void Game::startHand()
{
	myGui->nextRoundCleanGui();

	// log new hand
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

	bool raiseBlinds = false;

	if (myGameData.raiseIntervalMode == RAISE_ON_HANDNUMBER) {
		if (lastHandBlindsRaised + myGameData.raiseSmallBlindEveryHandsValue <= currentHandID) {
			raiseBlinds = true;
			lastHandBlindsRaised = currentHandID;
		}
	}
	else {
		if (lastTimeBlindsRaised + myGameData.raiseSmallBlindEveryMinutesValue <= blindsTimer.elapsed().total_seconds()/60) {
			raiseBlinds = true;
			lastTimeBlindsRaised = blindsTimer.elapsed().total_seconds()/60;
		}
	}
	if (raiseBlinds) {
		// At this point, the blinds must be raised
		// Now we check how the blinds should be raised	
		if (myGameData.raiseMode == DOUBLE_BLINDS) { 
			currentSmallBlind *= 2; 
		}	
		else {
			if(!blindsList.empty()) {
				currentSmallBlind = blindsList.front();
				blindsList.pop_front();
			}
			else {
				// The position exceeds the list
				if (myGameData.afterManualBlindsMode == AFTERMB_DOUBLE_BLINDS) { 
					currentSmallBlind *= 2; 
				}
				else {
					if(myGameData.afterManualBlindsMode == AFTERMB_RAISE_ABOUT) { 
						currentSmallBlind += myGameData.afterMBAlwaysRaiseValue;
					}
				}
			}
		}
	}
}
