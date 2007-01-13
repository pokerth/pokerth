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

#include "localhand.h"
#include "guiinterface.h"


using namespace std;

Game::Game(mainWindowImpl *w, GuiInterface* g, int qP, int sC, int sB) : myW(w), myGui(g), actualHand(0), actualBoard(0), startQuantityPlayers(qP), startCash(sC), startSmallBlind(sB), actualQuantityPlayers(qP), actualSmallBlind(sB), actualHandID(0), dealerPosition(0)
{
	int i;

	for(i=0; i<myW->getMaxQuantityPlayers(); i++) {
		playerArray[i] = 0;
	}

// ssss	myEngine = new EngineWrapper();
	

	myW->setGame(this);
	
// 	myGui->setGameEngine(myEngine);

	// Board erstellen
	actualBoard = new LocalBoard();

	// ersten Dealer bestimmen
	Tools myTool;
	myTool.getRandNumber(0, startQuantityPlayers-1, 1, &dealerPosition, 0);

	// Player erstellen
	LocalPlayer *tempPlayer;
	for(i=0; i<myW->getMaxQuantityPlayers(); i++) {
		tempPlayer = new LocalPlayer(actualBoard, i, startCash, startQuantityPlayers > i, 0);
		playerArray[i] = tempPlayer;
	}
	actualBoard->setPlayer(playerArray);
	
	//// SPIEL-SCHLEIFE

	startHand();

	// SPIEL-SCHLEIFE


}


Game::~Game()
{
	int i;

	delete actualBoard;
	actualBoard = 0;
	delete actualHand;
	actualHand = 0;

	LocalPlayer *tempPlayer;
	for(i=0; i<myW->getMaxQuantityPlayers(); i++) { 
		tempPlayer = playerArray[i];
		playerArray[i] = 0;
		delete tempPlayer;
	}

}

void Game::startHand()
{
	int i;

	//eventuell vorhandene Hand l�chen
	if(actualHand) {
		delete actualHand;
		actualHand = 0;
	}
	
	actualHandID++;

	// smallBlind alle 9 Runden erh�en
	if(actualHandID%9 == 0) actualSmallBlind *= 2;

	//Spieler Action auf 0 setzen 
	for(i=0; i<myW->getMaxQuantityPlayers(); i++) {
		playerArray[i]->setMyAction(0);
	}

	// Spieler mit leerem Cash auf inactive setzen
	for(i=0; i<myW->getMaxQuantityPlayers(); i++) {
		if(playerArray[i]->getMyCash() == 0) playerArray[i]->setMyActiveStatus(0);
	}
	
	//Spiel-Men disablen
// 	myW->actionNewGame->setDisabled(TRUE);
	

	// Hand erstellen
	actualHand = new LocalHand(myW, myGui, actualBoard, playerArray, actualHandID, actualQuantityPlayers, dealerPosition, actualSmallBlind, startCash);

	//GUI bereinigen 
	myW->nextRoundCleanGui();

	//Spielernamen schreiben 
	myW->refreshPlayerName();	

	// Dealer-Button weiterschieben --> Achtung inactive
	do {
		dealerPosition = (dealerPosition+1)%(myW->getMaxQuantityPlayers());
	} while(!(playerArray[dealerPosition]->getMyActiveStatus()));


	// Abfrage Cash==0 -> player inactive -> actualQuantityPlayer--
}
