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

#include <boost/shared_ptr.hpp>
#include "gamedata.h"
#include "playerdata.h"
#include "log.h"

#include <third_party/boost/timers.hpp>

class GuiInterface;
class Log;
class HandInterface;
class PlayerInterface;
class BoardInterface;
class EngineFactory;
struct GameData;
struct StartData;

typedef boost::shared_ptr<std::list<boost::shared_ptr<PlayerInterface> > > PlayerList;
typedef std::list<boost::shared_ptr<PlayerInterface> >::iterator PlayerListIterator;
typedef std::list<boost::shared_ptr<PlayerInterface> >::const_iterator PlayerListConstIterator;

class Game {

public:
    Game(GuiInterface *gui, boost::shared_ptr<EngineFactory> factory,
		const PlayerDataList &playerDataList, const GameData &gameData,
        const StartData &startData, int gameId, Log *myLog = 0);

	~Game();

	void initHand();
	void startHand();

	HandInterface *getCurrentHand();
	const HandInterface *getCurrentHand() const;

	PlayerList getSeatsList() const {return seatsList;}
	PlayerList getActivePlayerList() const {return activePlayerList;}
	PlayerList getRunningPlayerList() const {return runningPlayerList;}

	void setStartQuantityPlayers(int theValue) { startQuantityPlayers = theValue; }
	int getStartQuantityPlayers() const { return startQuantityPlayers; }
	
	void setStartSmallBlind(int theValue) { startSmallBlind = theValue; }
	int getStartSmallBlind() const { return startSmallBlind; }

	void setStartCash(int theValue)	{ startCash = theValue; }
	int getStartCash() const { return startCash;	}

	int getMyGameID() const	{ return myGameID; }

	void setCurrentSmallBlind(int theValue) { currentSmallBlind = theValue; }
	int getCurrentSmallBlind() const { return currentSmallBlind; }

	void setCurrentHandID(int theValue) { currentHandID = theValue; }
	int getCurrentHandID() const { return currentHandID; }

        unsigned getDealerPosition() const { return dealerPosition; }

	boost::shared_ptr<PlayerInterface> getPlayerByUniqueId(unsigned id);
	boost::shared_ptr<PlayerInterface> getCurrentPlayer();

	void raiseBlinds();

private:
	boost::shared_ptr<EngineFactory> myFactory;

	GuiInterface *myGui;
    Log *myLog;
	HandInterface *currentHand;
	BoardInterface *currentBoard;

	PlayerList seatsList;
	PlayerList activePlayerList; // used seats
	PlayerList runningPlayerList; // nonfolded and nonallin active players

	// start variables
	int startQuantityPlayers;
	int startCash;
	int startSmallBlind;
	int myGameID;

	// running variables
	int currentSmallBlind;
	int currentHandID;
	unsigned dealerPosition;
	int lastHandBlindsRaised;
	int lastTimeBlindsRaised;
	const GameData myGameData;
	std::list<int> blindsList;

	//timer
	boost::timers::portable::second_timer blindsTimer;
};

#endif
