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

#include "log.h"

#include "configfile.h"
#include "playerinterface.h"
#include "cardsvalue.h"

#include <sqlite3.h>
#include <iostream>
#include <dirent.h>
#include <boost/lexical_cast.hpp>

using namespace std;

Log::Log(string logDirString, int logOnOffInt) : curGameID(0), curHandID(0), logOnOff(false)
{

	if(SQLITE_LOG) {

		// logging activated
		if(logOnOffInt) {

			logOnOff = true;

			DIR *logDir;
			logDir = opendir(logDirString.c_str());
			bool dirExists = logDir != NULL;
			closedir(logDir);

			// check if logging path exist
			if(logDirString != "" && dirExists) {

				string sql;
				char *errmsg = NULL;

				// detect current time
				char curDateTime[20];
				char curDate[11];
				char curTime[9];
				time_t now = time(NULL);
				tm *z = localtime(&now);
				strftime(curDateTime,20,"%Y-%m-%d_%H.%M.%S",z);
				strftime(curDate,11,"%Y-%m-%d",z);
				strftime(curTime,9,"%H:%M:%S",z);

				string mySqliteLogFileName = boost::lexical_cast<string>(logDirString.c_str());
				mySqliteLogFileName += "pokerth-log-" + boost::lexical_cast<string>(curDateTime) + ".pdb";

				// open sqlite-db
				sqlite3_open(mySqliteLogFileName.data(), &mySqliteLogDb);
				if( mySqliteLogDb != 0 ) {

					int i;
					// create session table
					sql = "CREATE TABLE Session (";
					sql += "PokerTH_Version TEXT NOT NULL";
					sql += ",Date TEXT NOT NULL";
					sql += ",Time TEXT NOT NULL";
					sql += ", PRIMARY KEY(Date,Time))";
					if(sqlite3_exec(mySqliteLogDb, sql.data(), 0, 0, &errmsg) != SQLITE_OK) {
						cout << "Error in statement: " << sql.data() << "[" << errmsg << "]." << endl;
						sqlite3_free(errmsg);
						errmsg = NULL;
					}

					sql = "INSERT INTO Session (";
					sql += "PokerTH_Version";
					sql += ",Date";
					sql += ",Time";
					sql += ") VALUES (";
					sql += "\"" + boost::lexical_cast<string>(POKERTH_BETA_RELEASE_STRING) + "\",";
					sql += "\"" + boost::lexical_cast<string>(curDate) + "\",";
					sql += "\"" + boost::lexical_cast<string>(curTime) + "\")";
					if(sqlite3_exec(mySqliteLogDb, sql.data(), 0, 0, &errmsg) != SQLITE_OK) {
						cout << "Error in statement: " << sql.data() << "[" << errmsg << "]." << endl;
						sqlite3_free(errmsg);
						errmsg = NULL;
					}

					// create game table
					sql = "CREATE TABLE Game (";
					sql += "GameID INTEGER NOT NULL PRIMARY KEY";
					sql += ",Startmoney INTEGER NOT NULL";
					sql += ",StartSb INTEGER NOT NULL";
					sql += ",DealerPos INTEGER NOT NULL";
					for(i=1; i<=MAX_NUMBER_OF_PLAYERS; i++) {
						sql += ",Seat_" + boost::lexical_cast<std::string>(i) + " TEXT";
					}
					sql += ",Winner_Seat INTEGER";
					sql += ")";
					if(sqlite3_exec(mySqliteLogDb, sql.data(), 0, 0, &errmsg) != SQLITE_OK) {
						cout << "Error in statement: " << sql.data() << "[" << errmsg << "]." << endl;
						sqlite3_free(errmsg);
						errmsg = NULL;
					}

					// create hand table
					sql = "CREATE TABLE Hand (";
					sql += "HandID INTEGER NOT NULL";
					sql += ",GameID INTEGER NOT NULL";
					sql += ",Dealer_Seat INTEGER";
					sql += ",Sb_Amount INTEGER NOT NULL";
					sql += ",Sb_Seat INTEGER NOT NULL";
					sql += ",Bb_Amount INTEGER NOT NULL";
					sql += ",Bb_Seat INTEGER NOT NULL";
					for(i=1; i<=MAX_NUMBER_OF_PLAYERS; i++) {
						sql += ",Seat_" + boost::lexical_cast<std::string>(i) + "_Cash INTEGER";
						sql += ",Seat_" + boost::lexical_cast<std::string>(i) + "_Card_1 INTEGER";
						sql += ",Seat_" + boost::lexical_cast<std::string>(i) + "_Card_2 INTEGER";
						sql += ",Seat_" + boost::lexical_cast<std::string>(i) + "_Hand_text TEXT";
						sql += ",Seat_" + boost::lexical_cast<std::string>(i) + "_Hand_int INTEGER";
					}
					for(i=1; i<=5; i++) {
						sql += ",BoardCard_" + boost::lexical_cast<std::string>(i) + " INTEGER";
					}
					sql += ",PRIMARY KEY(HandID,GameID))";
					if(sqlite3_exec(mySqliteLogDb, sql.data(), 0, 0, &errmsg) != SQLITE_OK) {
						cout << "Error in statement: " << sql.data() << "[" << errmsg << "]." << endl;
						sqlite3_free(errmsg);
						errmsg = NULL;
					}

					// create action table
					sql = "CREATE TABLE Action (";
					sql += "ActionID INTEGER PRIMARY KEY AUTOINCREMENT";
					sql += ",HandID INTEGER NOT NULL";
					sql += ",GameID INTEGER NOT NULL";
					sql += ",BeRo INTEGER NOT NULL";
					sql += ",Player INTEGER NOT NULL";
					sql += ",Action TEXT NOT NULL";
					sql += ",Amount INTEGER";
					sql += ")";
					if(sqlite3_exec(mySqliteLogDb, sql.data(), 0, 0, &errmsg) != SQLITE_OK) {
						cout << "Error in statement: " << sql.data() << "[" << errmsg << "]." << endl;
						sqlite3_free(errmsg);
						errmsg = NULL;
					}
				}
			}
		}
	}
}

Log::~Log()
{
	if(SQLITE_LOG) {
		sqlite3_close(mySqliteLogDb);
	}
}

void Log::logNewGameMsg(int gameID, int startCash, int startSmallBlind, unsigned dealerPosition, PlayerList seatsList)
{

	curGameID = gameID;

	if(SQLITE_LOG) {

		if(logOnOff) {
			//if write logfiles is enabled

			PlayerListConstIterator it_c;
			string sql;
			char *errmsg = NULL;

			if( mySqliteLogDb != 0 ) {
				// sqlite-db is open
				int i;

				sql = "INSERT INTO Game (";
				sql += "GameID";
				sql += ",Startmoney";
				sql += ",StartSb";
				sql += ",DealerPos";
				for(i=1; i<=MAX_NUMBER_OF_PLAYERS; i++) {
					sql += ",Seat_" + boost::lexical_cast<std::string>(i);
				}
				sql += ") VALUES (";
				sql += boost::lexical_cast<string>(curGameID);
				sql += "," + boost::lexical_cast<string>(startCash);
				sql += "," + boost::lexical_cast<string>(startSmallBlind);
				sql += "," + boost::lexical_cast<string>(dealerPosition);
				for(it_c = seatsList->begin(); it_c!=seatsList->end(); it_c++) {
					if((*it_c)->getMyActiveStatus()) {
						sql += ",\"" + (*it_c)->getMyName() +"\"";
					} else {
						sql += ",NULL";
					}
				}
				sql += ")";
				if(sqlite3_exec(mySqliteLogDb, sql.data(), 0, 0, &errmsg) != SQLITE_OK) {
					cout << "Error in statement: " << sql.data() << "[" << errmsg << "]." << endl;
					sqlite3_free(errmsg);
					errmsg = NULL;
				}
			}
		}
	}
}

void Log::logNewHandMsg(int handID, unsigned dealerPosition, int smallBlind, unsigned smallBlindPosition, int bigBlind, unsigned bigBlindPosition, PlayerList seatsList)
{

	curHandID = handID;

	if(SQLITE_LOG) {

		if(logOnOff) {
			//if write logfiles is enabled

			PlayerListConstIterator it_c;
			string sql;
			char *errmsg = NULL;

			if( mySqliteLogDb != 0 ) {
				// sqlite-db is open
				int i;

				sql = "INSERT INTO Hand (";
				sql += "HandID";
				sql += ",GameID";
				sql += ",Dealer_Seat";
				sql += ",Sb_Amount";
				sql += ",Sb_Seat";
				sql += ",Bb_Amount";
				sql += ",Bb_Seat";
				for(i=1; i<=MAX_NUMBER_OF_PLAYERS; i++) {
					sql += ",Seat_" + boost::lexical_cast<std::string>(i) + "_Cash";
				}
				sql += ") VALUES (";
				sql += boost::lexical_cast<string>(curHandID);
				sql += "," + boost::lexical_cast<string>(curGameID);
				sql += "," + boost::lexical_cast<string>(dealerPosition);
				sql += "," + boost::lexical_cast<string>(smallBlind);
				sql += "," + boost::lexical_cast<string>(smallBlindPosition);
				sql += "," + boost::lexical_cast<string>(bigBlind);
				sql += "," + boost::lexical_cast<string>(bigBlindPosition);
				for(it_c = seatsList->begin(); it_c!=seatsList->end(); it_c++) {
					if((*it_c)->getMyActiveStatus()) {
						sql += "," + boost::lexical_cast<string>((*it_c)->getMyCash());
					} else {
						sql += ",NULL";
					}
				}
				sql += ")";
				if(sqlite3_exec(mySqliteLogDb, sql.data(), 0, 0, &errmsg) != SQLITE_OK) {
					cout << "Error in statement: " << sql.data() << "[" << errmsg << "]." << endl;
					sqlite3_free(errmsg);
					errmsg = NULL;
				}

				logPlayerAction(0,dealerPosition,LOG_ACTION_DEALER);
			}
		}
	}
}

void Log::logPlayerAction(int bero, int seat, PlayerActionLog action, int amount)
{

	if(SQLITE_LOG) {

		if(logOnOff) {
			//if write logfiles is enabled

			string sql;
			char *errmsg = NULL;

			if( mySqliteLogDb != 0 ) {
				// sqlite-db is open

				sql = "INSERT INTO Action (";
				sql += "HandID";
				sql += ",GameID";
				sql += ",BeRo";
				sql += ",Player";
				sql += ",Action";
				sql += ",Amount";
				sql += ") VALUES (";
				sql += boost::lexical_cast<string>(curHandID);
				sql += "," + boost::lexical_cast<string>(curGameID);
				sql += "," + boost::lexical_cast<string>(bero);;
				sql += "," + boost::lexical_cast<string>(seat);
				switch(action) {
				case LOG_ACTION_DEALER:
					sql += ",'starts as dealer'";
					break;
				case LOG_ACTION_SMALL_BLIND:
					sql += ",'posts small blind'";
					break;
				case LOG_ACTION_BIG_BLIND:
					sql += ",'posts big blind'";
					break;
				case LOG_ACTION_FOLD:
					sql += ",'folds'";
					break;
				case LOG_ACTION_CHECK:
					sql += ",'checks'";
					break;
				case LOG_ACTION_CALL:
					sql += ",'calls'";
					break;
				case LOG_ACTION_BET:
					sql += ",'bets'";
					break;
				case LOG_ACTION_ALL_IN:
					sql += ",'is all in with'";
					break;
				case LOG_ACTION_SHOW:
					sql += ",'shows'";
					break;
				case LOG_ACTION_HAS:
					sql += ",'has'";
					break;
				case LOG_ACTION_WIN:
					sql += ",'wins'";
					break;
				case LOG_ACTION_WIN_SIDE_POT:
					sql += ",'wins (side pot)'";
					break;
				case LOG_ACTION_SIT_OUT:
					sql += ",'sits out'";
					break;
				case LOG_ACTION_WIN_GAME:
					sql += ",'wins game'";
					break;
				default:
					return;
				}
				if(amount>0) {
					sql += "," + boost::lexical_cast<string>(amount);
				} else {
					sql += ",NULL";
				}
				sql += ")";
				if(sqlite3_exec(mySqliteLogDb, sql.data(), 0, 0, &errmsg) != SQLITE_OK) {
					cout << "Error in statement: " << sql.data() << "[" << errmsg << "]." << endl;
					sqlite3_free(errmsg);
					errmsg = NULL;
				}
			}
		}
	}
}

void Log::logBoardCards(int bero, int boardCards[5])
{
	if(SQLITE_LOG) {

		if(logOnOff) {
			//if write logfiles is enabled

			string sql;
			char *errmsg = NULL;

			if( mySqliteLogDb != 0 ) {
				// sqlite-db is open

				sql = "UPDATE Hand SET ";
				switch(bero) {
				case 2: {
					sql += "BoardCard_1=" + boost::lexical_cast<string>(boardCards[0]) + ",";
					sql += "BoardCard_2=" + boost::lexical_cast<string>(boardCards[1]) + ",";
					sql += "BoardCard_3=" + boost::lexical_cast<string>(boardCards[2]);
				}
				break;
				case 3: {
					sql += "BoardCard_4=" + boost::lexical_cast<string>(boardCards[3]);
				}
				break;
				case 4: {
					sql += "BoardCard_5=" + boost::lexical_cast<string>(boardCards[4]);
				}
				break;
				default:
					return;
				}
				sql += " WHERE ";
				sql += "GameID=" + boost::lexical_cast<string>(curGameID) + " AND ";
				sql += "HandID=" + boost::lexical_cast<string>(curHandID);
				if(sqlite3_exec(mySqliteLogDb, sql.data(), 0, 0, &errmsg) != SQLITE_OK) {
					cout << "Error in statement: " << sql.data() << "[" << errmsg << "]." << endl;
					sqlite3_free(errmsg);
					errmsg = NULL;
				}
			}
		}
	}
}

void Log::logHoleCards(int bero, int seat, int cards[2])
{
	if(SQLITE_LOG) {

		if(logOnOff) {
			//if write logfiles is enabled

			string sql;
			char *errmsg = NULL;

			if( mySqliteLogDb != 0 ) {
				// sqlite-db is open

				sql = "UPDATE Hand SET ";
				sql += "Seat_" + boost::lexical_cast<string>(seat) + "_Card_1=" + boost::lexical_cast<string>(cards[0]);
				sql += ",Seat_" + boost::lexical_cast<string>(seat) + "_Card_2=" + boost::lexical_cast<string>(cards[1]);
				sql += " WHERE ";
				sql += "GameID=" + boost::lexical_cast<string>(curGameID) + " AND ";
				sql += "HandID=" + boost::lexical_cast<string>(curHandID);
				if(sqlite3_exec(mySqliteLogDb, sql.data(), 0, 0, &errmsg) != SQLITE_OK) {
					cout << "Error in statement: " << sql.data() << "[" << errmsg << "]." << endl;
					sqlite3_free(errmsg);
					errmsg = NULL;
				}

				logPlayerAction(bero,seat,LOG_ACTION_SHOW);
			}
		}
	}
}

void Log::logHandName(int seat, int cardsValueInt, PlayerList activePlayerList)
{
	if(SQLITE_LOG) {

		if(logOnOff) {
			//if write logfiles is enabled

			string sql;
			char *errmsg = NULL;

			if( mySqliteLogDb != 0 ) {
				// sqlite-db is open

				sql = "UPDATE Hand SET ";
				sql += "Seat_" + boost::lexical_cast<string>(seat) + "_Hand_text=\"" + CardsValue::determineHandName(cardsValueInt,activePlayerList) + "\"";
				sql += ",Seat_" + boost::lexical_cast<string>(seat) + "_Hand_int=" + boost::lexical_cast<string>(cardsValueInt);
				sql += " WHERE ";
				sql += "GameID=" + boost::lexical_cast<string>(curGameID) + " AND ";
				sql += "HandID=" + boost::lexical_cast<string>(curHandID);
				if(sqlite3_exec(mySqliteLogDb, sql.data(), 0, 0, &errmsg) != SQLITE_OK) {
					cout << "Error in statement: " << sql.data() << "[" << errmsg << "]." << endl;
				}

				sqlite3_free(errmsg);
				errmsg = NULL;

				logPlayerAction(5,seat,LOG_ACTION_HAS);
			}
		}
	}
}

void Log::logHoleCardsHandName(int seat, int cards[2], int cardsValueInt, PlayerList activePlayerList)
{
	if(SQLITE_LOG) {

		if(logOnOff) {
			//if write logfiles is enabled

			string sql;
			char *errmsg = NULL;

			if( mySqliteLogDb != 0 ) {
				// sqlite-db is open

				sql = "UPDATE Hand SET ";
				sql += "Seat_" + boost::lexical_cast<string>(seat) + "_Card_1=" + boost::lexical_cast<string>(cards[0]);
				sql += ",Seat_" + boost::lexical_cast<string>(seat) + "_Card_2=" + boost::lexical_cast<string>(cards[1]);
				sql += ",Seat_" + boost::lexical_cast<string>(seat) + "_Hand_text=\"" + CardsValue::determineHandName(cardsValueInt,activePlayerList) + "\"";
				sql += ",Seat_" + boost::lexical_cast<string>(seat) + "_Hand_int=" + boost::lexical_cast<string>(cardsValueInt);
				sql += " WHERE ";
				sql += "GameID=" + boost::lexical_cast<string>(curGameID) + " AND ";
				sql += "HandID=" + boost::lexical_cast<string>(curHandID);
				if(sqlite3_exec(mySqliteLogDb, sql.data(), 0, 0, &errmsg) != SQLITE_OK) {
					cout << "Error in statement: " << sql.data() << "[" << errmsg << "]." << endl;
					sqlite3_free(errmsg);
					errmsg = NULL;
				}

				logPlayerAction(5,seat,LOG_ACTION_SHOW);
			}
		}
	}
}

//void
//Log::closeLogDbAtExit()
//{
//    if(SQLITE_LOG) {
//        // close sqlite-db
//        sqlite3_close(mySqliteLogDb);
//        mySqliteLogDb = NULL;
//    }

//}



// ACTION-Code
// 0    starts as dealer
// 1    posts small blind
// 2    posts big blind
// 3    folds
// 4    checks
// 5    calls
// 6    bets
// 7    is all in with
// 8    shows
// 9    has
// 10   wins
// 11   wins (side pot)
// 11   sits out
// 12   wins game
