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
#ifndef GAME_H
#define GAME_H

#include <iostream>
#include <string>
#include <sstream>

#include "game_defs.h"
#include "playerdata.h"

class GuiInterface;
class HandInterface;
class PlayerInterface;
class BoardInterface;
class EngineFactory;


class Game {

public:
    Game(GuiInterface *gui, const PlayerDataList &playerData, int sC, int sB, int hbrsB, int gameId);

    ~Game();

	//Zufgriff Startvariablen
	void setStartQuantityPlayers(const int& theValue) { startQuantityPlayers = theValue; }
	int getStartQuantityPlayers() const { return startQuantityPlayers; }
	
	void setStartSmallBlind(const int& theValue) { startSmallBlind = theValue; }
	int getStartSmallBlind() const { return startSmallBlind; }

	void setStartCash(const int& theValue)	{ startCash = theValue; }
	int getStartCash() const { return startCash;	}

	void setDealerPosition(const int& theValue) { dealerPosition = theValue; }
	int getDealerPosition() const { return dealerPosition; }
	
	void setMyGameID(const int& theValue) { myGameID = theValue;}
	int getMyGameID() const	{ return myGameID; }
	

	
	//Zufgriff Laufvariablen
	void setActualQuantityPlayers(const int& theValue) { actualQuantityPlayers = theValue; }
	int getActualQuantityPlayers() const { return actualQuantityPlayers; }

	void setActualSmallBlind(const int& theValue) { actualSmallBlind = theValue; }
	int getActualSmallBlind() const { return actualSmallBlind; }

	void setActualHandID(const int& theValue) { actualHandID = theValue; }
	int getActualHandID() const { return actualHandID; }

	void startHand();

	
	
	
	

private:
	EngineFactory *myFactory;

	GuiInterface *myGui;
	HandInterface *actualHand;
	BoardInterface *actualBoard;
	PlayerInterface *playerArray[7];

	//Startvariablen	
	int startQuantityPlayers;
	int startCash;
	int startSmallBlind;
	int startHandsBeforeRaiseSmallBlind;
	int myGameID;

	//Laufvariablen
	int actualQuantityPlayers;
	int actualSmallBlind;
	int actualHandID;
	int dealerPosition;
};

#endif
