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

#include "game.h"

#include <enginefactory.h>
#include <guiinterface.h>
#include "log.h"

#include "localexception.h"
#include "engine_msg.h"

#include <iostream>

using namespace std;

Game::Game(GuiInterface* gui, boost::shared_ptr<EngineFactory> factory,
		   const PlayerDataList &playerDataList, const GameData &gameData,
		   const StartData &startData, int gameId, Log* log)
	: myFactory(factory), myGui(gui), myLog(log), startQuantityPlayers(startData.numberOfPlayers),
	  startCash(gameData.startMoney), startSmallBlind(gameData.firstSmallBlind),
	  myGameID(gameId), currentSmallBlind(gameData.firstSmallBlind), currentHandID(0), dealerPosition(0),
	  lastHandBlindsRaised(1), lastTimeBlindsRaised(0), myGameData(gameData),
	  blindsTimer(boost::posix_time::time_duration(0, 0, 0), boost::timers::portable::second_timer::manual_start)
{

	blindsList = myGameData.manualBlindsList;

	dealerPosition = startData.startDealerPlayerId;

	if(DEBUG_MODE) {
		startSmallBlind = 10;
		currentSmallBlind = startSmallBlind;
		dealerPosition = 4;
	}

	int i;

	// determine dealer position
	PlayerDataList::const_iterator player_i = playerDataList.begin();
	PlayerDataList::const_iterator player_end = playerDataList.end();

	while (player_i != player_end) {
		if ((*player_i)->GetUniqueId() == dealerPosition)
			break;
		++player_i;
	}
	if (player_i == player_end)
		throw LocalException(__FILE__, __LINE__, ERR_DEALER_NOT_FOUND);

	// create board
	currentBoard = myFactory->createBoard();

	// create player lists
	seatsList.reset(new std::list<boost::shared_ptr<PlayerInterface> >);
	activePlayerList.reset(new std::list<boost::shared_ptr<PlayerInterface> >);
	runningPlayerList.reset(new std::list<boost::shared_ptr<PlayerInterface> >);

	// create player
	player_i = playerDataList.begin();
	player_end = playerDataList.end();
	for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {

		string myName;
		string myAvatarFile;
		string myGuid;
		unsigned uniqueId = 0;
		PlayerType type = PLAYER_TYPE_COMPUTER;
		int myStartCash = startCash;
		bool myStayOnTableStatus = false;

		if (player_i != player_end) {
			uniqueId = (*player_i)->GetUniqueId();
			type = (*player_i)->GetType();
			myName = (*player_i)->GetName();
			myAvatarFile = (*player_i)->GetAvatarFile();
			myGuid = (*player_i)->GetGuid();
			if ((*player_i)->GetStartCash() >= 0)
				myStartCash = (*player_i)->GetStartCash();
			myStayOnTableStatus = type == PLAYER_TYPE_HUMAN;
			++player_i;
		}

		// create player objects
		boost::shared_ptr<PlayerInterface> tmpPlayer = myFactory->createPlayer(i, uniqueId, type, myName, myAvatarFile, myStartCash, startQuantityPlayers > i, myStayOnTableStatus, 0);
		tmpPlayer->setIsSessionActive(true);
		tmpPlayer->setMyGuid(myGuid);

		// fill player lists
		seatsList->push_back(tmpPlayer);
		if(startQuantityPlayers > i) {
			activePlayerList->push_back(tmpPlayer);
		}
		(*runningPlayerList) = (*activePlayerList);

	}

	currentBoard->setPlayerLists(seatsList, activePlayerList, runningPlayerList);

	// log game data
	if(myLog) myLog->logNewGameMsg(myGameID, startCash, startSmallBlind, getPlayerByUniqueId(dealerPosition)->getMyID()+1, seatsList);

	//start timer
	blindsTimer.reset();
	blindsTimer.start();
}

Game::~Game()
{
}

boost::shared_ptr<HandInterface> Game::getCurrentHand()
{
	return currentHand;
}

const boost::shared_ptr<HandInterface> Game::getCurrentHand() const
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
	for(it=seatsList->begin(); it!=seatsList->end(); ++it) {
		(*it)->setMyAction(PLAYER_ACTION_NONE);
	}

	// set player with empty cash inactive
	it = activePlayerList->begin();
	while( it!=activePlayerList->end() ) {

		if((*it)->getMyCash() == 0) {
			(*it)->setMyActiveStatus(false);
			it = activePlayerList->erase(it);
		} else {
			++it;
		}
	}

	runningPlayerList->clear();
	(*runningPlayerList) = (*activePlayerList);

	// create Hand
	currentHand = myFactory->createHand(myFactory, myGui, currentBoard, myLog, seatsList, activePlayerList, runningPlayerList, currentHandID, startQuantityPlayers, dealerPosition, currentSmallBlind, startCash);

	// shifting dealer button -> TODO exception-rule !!!
	bool nextDealerFound = false;
	PlayerListConstIterator dealerPositionIt = currentHand->getSeatIt(dealerPosition);
	if(dealerPositionIt == seatsList->end()) {
		throw LocalException(__FILE__, __LINE__, ERR_SEAT_NOT_FOUND);
	}

	for(i=0; i<seatsList->size(); i++) {

		++dealerPositionIt;
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
	PlayerList tmpList = getSeatsList();
	PlayerListIterator i = tmpList->begin();
	PlayerListIterator end = tmpList->end();
	while (i != end) {
		if ((*i)->getMyUniqueID() == id) {
			tmpPlayer = *i;
			break;
		}
		++i;
	}
	return tmpPlayer;
}

boost::shared_ptr<PlayerInterface> Game::getPlayerByNumber(int number)
{
	boost::shared_ptr<PlayerInterface> tmpPlayer;
	PlayerList tmpList = getSeatsList();
	PlayerListIterator i = tmpList->begin();
	PlayerListIterator end = tmpList->end();
	while (i != end) {
		if ((*i)->getMyID() == number) {
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

boost::shared_ptr<PlayerInterface> Game::getPlayerByName(const std::string &name)
{
	boost::shared_ptr<PlayerInterface> tmpPlayer;
	PlayerList tmpList = getSeatsList();
	PlayerListIterator i = tmpList->begin();
	PlayerListIterator end = tmpList->end();
	while (i != end) {
		if ((*i)->getMyName() == name) {
			tmpPlayer = *i;
			break;
		}
		++i;
	}
	return tmpPlayer;
}

void Game::raiseBlinds()
{

	bool raiseBlinds = false;

	if (myGameData.raiseIntervalMode == RAISE_ON_HANDNUMBER) {
		if (lastHandBlindsRaised + myGameData.raiseSmallBlindEveryHandsValue <= currentHandID) {
			raiseBlinds = true;
			lastHandBlindsRaised = currentHandID;
		}
	} else {
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
		} else {
			if(!blindsList.empty()) {
				currentSmallBlind = blindsList.front();
				blindsList.pop_front();
			} else {
				// The position exceeds the list
				if (myGameData.afterManualBlindsMode == AFTERMB_DOUBLE_BLINDS) {
					currentSmallBlind *= 2;
				} else {
					if(myGameData.afterManualBlindsMode == AFTERMB_RAISE_ABOUT) {
						currentSmallBlind += myGameData.afterMBAlwaysRaiseValue;
					}
				}
			}
		}
		currentSmallBlind = min(currentSmallBlind,startQuantityPlayers*startCash/2);
	}
}
