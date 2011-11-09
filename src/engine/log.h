/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2011 Felix Hammer, Florian Thauer, Lothar May          *
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
 *****************************************************************************/

#ifndef LOG_H
#define LOG_H

#include "engine_defs.h"
#include "game_defs.h"

struct sqlite3;

class ConfigFile;

class Log
{

public:
	Log(ConfigFile *c);

	~Log();

	void logNewGameMsg(int gameID, int startCash, int startSmallBlind, unsigned dealerPosition, PlayerList seatsList);
	void logNewHandMsg(int handID, unsigned dealerPosition, int smallBlind, unsigned smallBlindPosition, int bigBlind, unsigned bigBlindPosition, PlayerList seatsList);
	void logPlayerAction(GameState bero, int seat, PlayerActionLog action, int amount = 0);
	PlayerActionLog transformPlayerActionLog(PlayerAction action);
	void logBoardCards(GameState bero, int boardCards[5]);
	void logHoleCardsHandName(GameState bero, PlayerList activePlayerList);
	void logAfterHand();
	void logAfterGame();
	void exec_transaction();
//    void closeLogDbAtExit();


private:
	sqlite3 *mySqliteLogDb;
	ConfigFile *myConfig;
	int curGameID;
	int curHandID;
	std::string sql;
	bool logHoleCardsDone;
};

#endif // LOG_H
