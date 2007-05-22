/***************************************************************************
 *   Copyright (C) 2007 by Lothar May                                      *
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
#include "clienthand.h"
#include <game_defs.h>

using namespace std;

ClientHand::ClientHand(boost::shared_ptr<EngineFactory> f, GuiInterface *g, BoardInterface *b, PlayerInterface **p, int id, int sP, int aP, int dP, int sB,int sC)
: myFactory(f), myGui(g),  myBoard(b), playerArray(p), myPreflop(0), myFlop(0), myTurn(0), myRiver(0), myID(id), actualQuantityPlayers(aP), startQuantityPlayers(sP), dealerPosition(dP), actualRound(0), smallBlind(sB), startCash(sC), allInCondition(0), bettingRoundsPlayed(0)
{
	int i;
	lastPlayersTurn = 0;

	myBoard->setHand(this);


	for(i=0; i<startQuantityPlayers; i++) {
		if(playerArray[i]->getMyActiveStatus() != 0) {
			playerArray[i]->setHand(this);
		}
		playerArray[i]->setMyCardsFlip(0);
	}


	// roundStartCashArray fllen
	for(i=0; i<MAX_NUMBER_OF_PLAYERS; i++) {
		playerArray[i]->setMyRoundStartCash(playerArray[i]->getMyCash());
	}

	assignButtons();
}



ClientHand::~ClientHand()
{
}

void
ClientHand::start()
{
}

void
ClientHand::assignButtons()
{
	int i;

	//Aktive Spieler zählen
	int activePlayersCounter = 0;
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) { 
		if (playerArray[i]->getMyActiveStatus() == 1) activePlayersCounter++;
	}

	// alle Buttons loeschen
	for (i=0; i<MAX_NUMBER_OF_PLAYERS; i++) { playerArray[i]->setMyButton(0); }

	// DealerButton zuweisen
	playerArray[dealerPosition]->setMyButton(1);

	// assign Small Blind next to dealer. ATTENTION: in heads up it is big blind
	i = dealerPosition;
	do {
		i = (i+1)%(MAX_NUMBER_OF_PLAYERS);
		if(playerArray[i]->getMyActiveStatus())	{
			if(activePlayersCounter > 2) playerArray[i]->setMyButton(2); //small blind normal
			else playerArray[i]->setMyButton(3); //big blind in heads up
		
		}

	} while(!(playerArray[i]->getMyActiveStatus()));

	// assign big blind next to small blind. ATTENTION: in heads up it is small blind
	do {
		i = (i+1)%(MAX_NUMBER_OF_PLAYERS);
		if(playerArray[i]->getMyActiveStatus())	{
			if(activePlayersCounter > 2) playerArray[i]->setMyButton(3); //big blind normal
			else playerArray[i]->setMyButton(2); //small blind in heads up
			
		}

	} while(!(playerArray[i]->getMyActiveStatus()));
}

void
ClientHand::switchRounds()
{
}

void
ClientHand::highlightRoundLabel()
{
}

