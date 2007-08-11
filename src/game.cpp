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
#include "gamedata.h"

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
  myGameID(gameId), actualQuantityPlayers(startData.numberOfPlayers),
  actualSmallBlind(gameData.smallBlind), actualHandID(0), dealerPosition(0)
{
// 	cout << "Create Game Object" << "\n";
	int i;

	actualHandID = 0;

// 	for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
// 		playerArray[i] = 0;
// 	}

	// Dealer Position bestimmen
	PlayerDataList::const_iterator player_i = playerDataList.begin();
	PlayerDataList::const_iterator player_end = playerDataList.end();

	bool dealerFound = false;
	i = 0;
	while (player_i != player_end)
	{
		if ((*player_i)->GetUniqueId() == startData.startDealerPlayerId)
		{
			dealerPosition = i;
			dealerFound = true;
			break;
		}
		++player_i;
		++i;
	}
	assert(dealerFound);

	// Board erstellen
	actualBoard = myFactory->createBoard();


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

		//PlayerObjekte erzeugen
		playerArray.push_back(myFactory->createPlayer(actualBoard, i, uniqueId, type, myName, myAvatarFile, startCash, startQuantityPlayers > i, 0));


// 		playerArray[i] = myFactory->createPlayer(actualBoard, i, uniqueId, type, myName, myAvatarFile, startCash, startQuantityPlayers > i, 0);
		playerArray[i]->setNetSessionData(myNetSession);
	}
	actualBoard->setPlayer(playerArray);
}


Game::~Game()
{
// 	cout << "Delete Game Object" << "\n";
	int i;

	delete actualBoard;
	actualBoard = 0;
	delete actualHand;
	actualHand = 0;

// 	boost::shared_ptr<PlayerInterface> tempPlayer;
// 	for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) { 
// 		tempPlayer = playerArray[i];
// 		playerArray[i] = 0;
// 		delete tempPlayer;
// 	}

}

HandInterface * Game::getCurrentHand()
{
	return actualHand;
}

void Game::initHand()
{

	int i;

	// eventuell vorhandene Hand löschen
	if(actualHand) {
		delete actualHand;
		actualHand = 0;
	}
	
	actualHandID++;

	// smallBlind alle x Runden erhöhen
	if(actualHandID%(startHandsBeforeRaiseSmallBlind+1) == 0) { actualSmallBlind *= 2; }

	

	// Spieler mit leerem Cash auf inactive setzen
	for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
		if(playerArray[i]->getMyCash() == 0) playerArray[i]->setMyActiveStatus(0);
	}

	// Anzahl noch aktiver Spieler ermitteln
	actualQuantityPlayers = 0;
	for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
		if(playerArray[i]->getMyActiveStatus()) actualQuantityPlayers++;
	}

	//Spieler Action auf 0 setzen 
	for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
		playerArray[i]->setMyAction(0);
	}

	// Hand erstellen
	actualHand = myFactory->createHand(myFactory, myGui, actualBoard, playerArray, actualHandID, startQuantityPlayers, actualQuantityPlayers, dealerPosition, actualSmallBlind, startCash);


	// Dealer-Button weiterschieben --> Achtung inactive
	for(i=0; (i<MAX_NUMBER_OF_PLAYERS && !(playerArray[dealerPosition]->getMyActiveStatus())) || i==0; i++) {
	
		dealerPosition = (dealerPosition+1)%(MAX_NUMBER_OF_PLAYERS);
	}

	// Abfrage Cash==0 -> player inactive -> actualQuantityPlayer--
}

void Game::startHand()
{
	assert(actualHand);

	//GUI bereinigen 
	myGui->nextRoundCleanGui();

	myGui->logNewGameHandMsg(myGameID, actualHandID);

	actualHand->start();
}

boost::shared_ptr<PlayerInterface> Game::getPlayerByUniqueId(unsigned id)
{
	boost::shared_ptr<PlayerInterface> tmpPlayer;
	for (int i = 0; i < startQuantityPlayers; i++)
	{
		if (playerArray[i]->getMyUniqueID() == id)
		{
			tmpPlayer = playerArray[i];
			break;
		}
	}
	return tmpPlayer;
}

boost::shared_ptr<PlayerInterface> Game::getCurrentPlayer()
{
	int curPlayerNum = getCurrentHand()->getCurrentBeRo()->getPlayersTurn();
	assert(curPlayerNum < getStartQuantityPlayers());
	return getPlayerArray()[curPlayerNum];
}

