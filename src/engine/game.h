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

#ifndef GAME_H
#define GAME_H

#include <engine_defs.h>
#include "gamedata.h"
#include "playerdata.h"

#include <third_party/boost/timers.hpp>

class GuiInterface;
class Log;
class HandInterface;
class BoardInterface;
class EngineFactory;
struct GameData;
struct StartData;


class Game
{

public:
	Game(GuiInterface *gui, boost::shared_ptr<EngineFactory> factory,
		 const PlayerDataList &playerDataList, const GameData &gameData,
		 const StartData &startData, int gameId, Log *myLog);

	~Game();

	void initHand();
	void startHand();

	boost::shared_ptr<HandInterface> getCurrentHand();
	const boost::shared_ptr<HandInterface> getCurrentHand() const;

	PlayerList getSeatsList() const
	{
		return seatsList;
	}
	PlayerList getActivePlayerList() const
	{
		return activePlayerList;
	}
	PlayerList getRunningPlayerList() const
	{
		return runningPlayerList;
	}

	void setStartQuantityPlayers(int theValue)
	{
		startQuantityPlayers = theValue;
	}
	int getStartQuantityPlayers() const
	{
		return startQuantityPlayers;
	}

	void setStartSmallBlind(int theValue)
	{
		startSmallBlind = theValue;
	}
	int getStartSmallBlind() const
	{
		return startSmallBlind;
	}

	void setStartCash(int theValue)
	{
		startCash = theValue;
	}
	int getStartCash() const
	{
		return startCash;
	}

	int getMyGameID() const
	{
		return myGameID;
	}

	void setCurrentSmallBlind(int theValue)
	{
		currentSmallBlind = theValue;
	}
	int getCurrentSmallBlind() const
	{
		return currentSmallBlind;
	}

	void setCurrentHandID(int theValue)
	{
		currentHandID = theValue;
	}
	int getCurrentHandID() const
	{
		return currentHandID;
	}

	unsigned getDealerPosition() const
	{
		return dealerPosition;
	}

	void replaceDealer(unsigned oldDealer, unsigned newDealer)
	{
		if (dealerPosition == oldDealer)
			dealerPosition = newDealer;
	}

	boost::shared_ptr<PlayerInterface> getPlayerByUniqueId(unsigned id);
	boost::shared_ptr<PlayerInterface> getPlayerByNumber(int number);
	boost::shared_ptr<PlayerInterface> getPlayerByName(const std::string &name);
	boost::shared_ptr<PlayerInterface> getCurrentPlayer();

	void raiseBlinds();

private:
	boost::shared_ptr<EngineFactory> myFactory;

	GuiInterface *myGui;
	Log *myLog;
	boost::shared_ptr<HandInterface> currentHand;
	boost::shared_ptr<BoardInterface> currentBoard;

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
