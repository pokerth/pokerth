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

#ifndef LOG_H
#define LOG_H

#include <string>
#include <boost/filesystem.hpp>

#include "engine_defs.h"
#include "game_defs.h"

struct sqlite3;

class ConfigFile;

class Log
{

public:
	Log(ConfigFile *c);

	~Log();

	void init();
	void logNewGameMsg(int gameID, int startCash, int startSmallBlind, unsigned dealerPosition, PlayerList seatsList);
	void logNewHandMsg(int handID, unsigned dealerPosition, int smallBlind, unsigned smallBlindPosition, int bigBlind, unsigned bigBlindPosition, PlayerList seatsList);
	void logPlayerAction(std::string playerName, PlayerActionLog action, int amount = 0);
	void logPlayerAction(int seat, PlayerActionLog action, int amount = 0);
	PlayerActionLog transformPlayerActionLog(PlayerAction action);
	void logBoardCards(int boardCards[5]);
	void logHoleCardsHandName(PlayerList activePlayerList);
	void logHoleCardsHandName(PlayerList activePlayerList, boost::shared_ptr<PlayerInterface> player, bool forceExecLog = 0);
	void logHandWinner(PlayerList activePlayerList, int highestCardsValue, std::list<unsigned> winners);
	void logGameWinner(PlayerList activePlayerList);
	void logPlayerSitsOut(PlayerList activePlayerList);
	void logAfterHand();
	void logAfterGame();
//    void closeLogDbAtExit();

	void setCurrentRound(GameState theValue) {
		currentRound = theValue;
	}

	std::string getMySqliteLogFileName() {
		return mySqliteLogFileName.directory_string();
	}

private:

	void exec_transaction();

	sqlite3 *mySqliteLogDb;
	boost::filesystem::path mySqliteLogFileName;
	ConfigFile *myConfig;
	int uniqueGameID;
	int currentHandID;
	GameState currentRound;
	std::string sql;
};

#endif // LOG_H
