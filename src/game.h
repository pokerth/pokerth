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

#include "game_defs.h"
#include "playerdata.h"
#include "configfile.h"

#include <string>
#include <boost/shared_ptr.hpp>

class GuiInterface;
class HandInterface;
class PlayerInterface;
class BoardInterface;
class EngineFactory;
struct GameData;
struct StartData;

class Game {

public:
	Game(GuiInterface *gui, boost::shared_ptr<EngineFactory> factory,
		const PlayerDataList &playerDataList, const GameData &gameData,
		const StartData &startData, int gameId);

	~Game();

	void initHand();
	void startHand();

	HandInterface *getCurrentHand();

	PlayerInterface** getPlayerArray() {return playerArray;}

	//Zufgriff Startvariablen
	void setStartQuantityPlayers(int theValue) { startQuantityPlayers = theValue; }
	int getStartQuantityPlayers() const { return startQuantityPlayers; }
	
	void setStartSmallBlind(int theValue) { startSmallBlind = theValue; }
	int getStartSmallBlind() const { return startSmallBlind; }

	void setStartCash(int theValue)	{ startCash = theValue; }
	int getStartCash() const { return startCash;	}

	int getMyGameID() const	{ return myGameID; }

	//Zufgriff Laufvariablen
	void setActualQuantityPlayers(int theValue) { actualQuantityPlayers = theValue; }
	int getActualQuantityPlayers() const { return actualQuantityPlayers; }

	void setActualSmallBlind(int theValue) { actualSmallBlind = theValue; }
	int getActualSmallBlind() const { return actualSmallBlind; }

	void setActualHandID(int theValue) { actualHandID = theValue; }
	int getActualHandID() const { return actualHandID; }

	PlayerInterface * getPlayerByUniqueId(unsigned id);

private:
	boost::shared_ptr<EngineFactory> myFactory;

	GuiInterface *myGui;
	HandInterface *actualHand;
	BoardInterface *actualBoard;
	PlayerInterface *playerArray[MAX_NUMBER_OF_PLAYERS];

	//Startvariablen	
	int startQuantityPlayers;
	int startCash;
	int startSmallBlind;
	int startHandsBeforeRaiseSmallBlind;
	int myGameID;
	int guiPlayerNum;

	//Laufvariablen
	int actualQuantityPlayers;
	int actualSmallBlind;
	int actualHandID;
	int dealerPosition;
};

#endif
