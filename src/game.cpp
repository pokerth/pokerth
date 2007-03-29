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

#include "enginefactory.h"
#include "localenginefactory.h"

#include "guiinterface.h"
#include "boardinterface.h"
#include "playerinterface.h"
#include "handinterface.h"

#include "configfile.h"

using namespace std;

Game::Game(ConfigFile* c, GuiInterface* g, int qP, int sC, int sB, int gId) : myConfig(c), myGui(g), actualHand(0), actualBoard(0), startQuantityPlayers(qP), startCash(sC), startSmallBlind(sB), myGameID(gId), actualQuantityPlayers(qP), actualSmallBlind(sB), actualHandID(0), dealerPosition(0)
{
// 	cout << "Create Game Object" << "\n";
	int i;

	actualHandID = 0;

	for(i=0; i<myGui->getMaxQuantityPlayers(); i++) {
		playerArray[i] = 0;
	}

	myGui->setGame(this);

	//EngineFactory erstellen
	myFactory = new LocalEngineFactory; // LocalEngine erstellen
	

	// Board erstellen
	actualBoard = myFactory->createBoard();


	// ersten Dealer bestimmen
	Tools myTool;
	myTool.getRandNumber(0, startQuantityPlayers-1, 1, &dealerPosition, 0);

	// Player erstellen
	PlayerInterface *tempPlayer;
	for(i=0; i<myGui->getMaxQuantityPlayers(); i++) {

		//Namen und Avatarpfad abfragen 
		ostringstream myName;
		if (i==0) { myName << "MyName";	}
		else { myName << "Opponent" << i << "Name"; }
		ostringstream myAvatar;
		if (i==0) { myAvatar << "MyAvatar";	}
		else { myAvatar << "Opponent" << i << "Avatar"; }

		//PlayerObjekte erzeugen
		tempPlayer = myFactory->createPlayer(actualBoard, i, myConfig->readConfigString(myName.str()), myConfig->readConfigString(myAvatar.str()), startCash, startQuantityPlayers > i, 0);
		playerArray[i] = tempPlayer;
	}
	actualBoard->setPlayer(playerArray);
	
	//// SPIEL-SCHLEIFE

	startHand();

	// SPIEL-SCHLEIFE


}


Game::~Game()
{
// 	cout << "Delete Game Object" << "\n";
	int i;

	delete actualBoard;
	actualBoard = 0;
	delete actualHand;
	actualHand = 0;

	PlayerInterface *tempPlayer;
	for(i=0; i<myGui->getMaxQuantityPlayers(); i++) { 
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

	// smallBlind alle x Runden erhöhen
	int handsBeforeRaiseSmallBLind = myConfig->readConfigInt("HandsBeforeRaiseSmallBlind");
	if(actualHandID%handsBeforeRaiseSmallBLind == 0) actualSmallBlind *= 2;

	//Spieler Action auf 0 setzen 
	for(i=0; i<myGui->getMaxQuantityPlayers(); i++) {
		playerArray[i]->setMyAction(0);
	}

	// Spieler mit leerem Cash auf inactive setzen
	for(i=0; i<myGui->getMaxQuantityPlayers(); i++) {
		if(playerArray[i]->getMyCash() == 0) playerArray[i]->setMyActiveStatus(0);
	}

	// Anzahl noch aktiver Spieler ermitteln
	actualQuantityPlayers = 0;
	for(i=0; i<myGui->getMaxQuantityPlayers(); i++) {
		if(playerArray[i]->getMyActiveStatus() != 0) actualQuantityPlayers++;
	}

	// Hand erstellen
	actualHand = myFactory->createHand(myFactory, myGui, actualBoard, playerArray, actualHandID, startQuantityPlayers, actualQuantityPlayers, dealerPosition, actualSmallBlind, startCash);

	
	//GUI bereinigen 
	myGui->nextRoundCleanGui();

	//Spielernamen schreiben 
// 	myGui->refreshPlayerName();	

	// Dealer-Button weiterschieben --> Achtung inactive
	do {
		dealerPosition = (dealerPosition+1)%(myGui->getMaxQuantityPlayers());
	} while(!(playerArray[dealerPosition]->getMyActiveStatus()));


	myGui->logNewGameHandMsg(myGameID, actualHandID);


	// Abfrage Cash==0 -> player inactive -> actualQuantityPlayer--
}
