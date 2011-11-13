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
#include <dirent.h>
#include <boost/lexical_cast.hpp>

using namespace std;

Log::Log(ConfigFile *c) : mySqliteLogDb(0), myConfig(c), curGameID(0), curHandID(0), sql("")
{

	if(SQLITE_LOG) {

		// logging activated
		if(myConfig->readConfigInt("LogOnOff")) {

			DIR *logDir;
			logDir = opendir((myConfig->readConfigString("LogDir")).c_str());
			bool dirExists = logDir != NULL;
			closedir(logDir);

			// check if logging path exist
			if(myConfig->readConfigString("LogDir") != "" && dirExists) {

				// detect current time
				char curDateTime[20];
				char curDate[11];
				char curTime[9];
				time_t now = time(NULL);
				tm *z = localtime(&now);
				strftime(curDateTime,20,"%Y-%m-%d_%H.%M.%S",z);
				strftime(curDate,11,"%Y-%m-%d",z);
				strftime(curTime,9,"%H:%M:%S",z);

				string mySqliteLogFileName = boost::lexical_cast<string>((myConfig->readConfigString("LogDir")).c_str());
				mySqliteLogFileName += "pokerth-log-" + boost::lexical_cast<string>(curDateTime) + ".pdb";

				// open sqlite-db
				sqlite3_open(mySqliteLogFileName.data(), &mySqliteLogDb);
				if( mySqliteLogDb != 0 ) {

					int i;
					// create session table
					sql += "CREATE TABLE Session (";
					sql += "PokerTH_Version TEXT NOT NULL";
					sql += ",Date TEXT NOT NULL";
					sql += ",Time TEXT NOT NULL";
					sql += ", PRIMARY KEY(Date,Time));";

					sql += "INSERT INTO Session (";
					sql += "PokerTH_Version";
					sql += ",Date";
					sql += ",Time";
					sql += ") VALUES (";
					sql += "\"" + boost::lexical_cast<string>(POKERTH_BETA_RELEASE_STRING) + "\",";
					sql += "\"" + boost::lexical_cast<string>(curDate) + "\",";
					sql += "\"" + boost::lexical_cast<string>(curTime) + "\");";

					// create game table
					sql += "CREATE TABLE Game (";
					sql += "GameID INTEGER NOT NULL PRIMARY KEY";
					sql += ",Startmoney INTEGER NOT NULL";
					sql += ",StartSb INTEGER NOT NULL";
					sql += ",DealerPos INTEGER NOT NULL";
					for(i=1; i<=MAX_NUMBER_OF_PLAYERS; i++) {
						sql += ",Seat_" + boost::lexical_cast<std::string>(i) + " TEXT";
					}
					sql += ",Winner_Seat INTEGER";
					sql += ");";

					// create hand table
					sql += "CREATE TABLE Hand (";
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
					sql += ",PRIMARY KEY(HandID,GameID));";

					// create action table
					sql += "CREATE TABLE Action (";
					sql += "ActionID INTEGER PRIMARY KEY AUTOINCREMENT";
					sql += ",HandID INTEGER NOT NULL";
					sql += ",GameID INTEGER NOT NULL";
					sql += ",BeRo INTEGER NOT NULL";
					sql += ",Player INTEGER NOT NULL";
					sql += ",Action TEXT NOT NULL";
					sql += ",Amount INTEGER";
					sql += ");";

					exec_transaction();
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

void
Log::logNewGameMsg(int gameID, int startCash, int startSmallBlind, unsigned dealerPosition, PlayerList seatsList)
{

	curGameID = gameID;

	if(SQLITE_LOG) {

		if(myConfig->readConfigInt("LogOnOff")) {
			//if write logfiles is enabled

			PlayerListConstIterator it_c;

			if( mySqliteLogDb != 0 ) {
				// sqlite-db is open
				int i;

				sql += "INSERT INTO Game (";
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
				sql += ");";
				if(myConfig->readConfigInt("LogInterval") == 0) {
					exec_transaction();
				}
			}
		}
	}
}

void
Log::logNewHandMsg(int handID, unsigned dealerPosition, int smallBlind, unsigned smallBlindPosition, int bigBlind, unsigned bigBlindPosition, PlayerList seatsList)
{

	curHandID = handID;
	PlayerListConstIterator it_c;
	for(it_c = seatsList->begin(); it_c!=seatsList->end(); it_c++) {
		(*it_c)->setLogHoleCardsDone(false);
	}

	if(SQLITE_LOG) {

		if(myConfig->readConfigInt("LogOnOff")) {
			//if write logfiles is enabled

			if( mySqliteLogDb != 0 ) {
				// sqlite-db is open
				int i;

				sql += "INSERT INTO Hand (";
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
						sql += "," + boost::lexical_cast<string>((*it_c)->getMyRoundStartCash());
					} else {
						sql += ",NULL";
					}
				}
				sql += ");";
				if(myConfig->readConfigInt("LogInterval") == 0) {
					exec_transaction();
				}

				// !! TODO !! Hack, weil Button-Regel noch falsch und dealerPosition noch teilweise falsche ID enthält (HeadsUp: dealerPosition=bigBlindPosition <-- falsch)
				bool dealerButtonOnTable = false;
				int countActivePlayer = 0;
				for(it_c = seatsList->begin(); it_c!=seatsList->end(); it_c++) {
					if((*it_c)->getMyActiveStatus()) {
						countActivePlayer++;
						if((*it_c)->getMyButton()==BUTTON_DEALER && (*it_c)->getMyActiveStatus()) {
							dealerButtonOnTable = true;
						}
					}
				}
				if(countActivePlayer==2) {
					logPlayerAction(GAME_STATE_PREFLOP,smallBlindPosition,LOG_ACTION_DEALER);
				} else {
					if(dealerButtonOnTable) {
						logPlayerAction(GAME_STATE_PREFLOP,dealerPosition,LOG_ACTION_DEALER);
					}
				}

				// log blinds
				for(it_c = seatsList->begin(); it_c!=seatsList->end(); it_c++) {
					if((*it_c)->getMyButton() == BUTTON_SMALL_BLIND) {
						logPlayerAction(GAME_STATE_PREFLOP,smallBlindPosition,LOG_ACTION_SMALL_BLIND,(*it_c)->getMySet());
					}
				}
				for(it_c = seatsList->begin(); it_c!=seatsList->end(); it_c++) {
					if((*it_c)->getMyButton() == BUTTON_BIG_BLIND) {
						logPlayerAction(GAME_STATE_PREFLOP,bigBlindPosition,LOG_ACTION_BIG_BLIND,(*it_c)->getMySet());
					}
				}
				// !! TODO !! Hack

			}
		}
	}
}

void
Log::logPlayerAction(GameState bero, int seat, PlayerActionLog action, int amount)
{

	if(SQLITE_LOG) {

		if(myConfig->readConfigInt("LogOnOff")) {
			//if write logfiles is enabled

			if( mySqliteLogDb != 0 ) {
				// sqlite-db is open

				if(action!=LOG_ACTION_NONE) {
					sql += "INSERT INTO Action (";
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
						sql += ",NULL";
						break;
					case LOG_ACTION_SMALL_BLIND:
						sql += ",'posts small blind'";
						sql += "," + boost::lexical_cast<string>(amount);
						break;
					case LOG_ACTION_BIG_BLIND:
						sql += ",'posts big blind'";
						sql += "," + boost::lexical_cast<string>(amount);
						break;
					case LOG_ACTION_FOLD:
						sql += ",'folds'";
						sql += ",NULL";
						break;
					case LOG_ACTION_CHECK:
						sql += ",'checks'";
						sql += ",NULL";
						break;
					case LOG_ACTION_CALL:
						sql += ",'calls'";
						sql += "," + boost::lexical_cast<string>(amount);
						break;
					case LOG_ACTION_BET:
						sql += ",'bets'";
						sql += "," + boost::lexical_cast<string>(amount);
						break;
					case LOG_ACTION_ALL_IN:
						sql += ",'is all in with'";
						sql += "," + boost::lexical_cast<string>(amount);
						break;
					case LOG_ACTION_SHOW:
						sql += ",'shows'";
						sql += ",NULL";
						break;
					case LOG_ACTION_HAS:
						sql += ",'has'";
						sql += ",NULL";
						break;
					case LOG_ACTION_WIN:
						sql += ",'wins'";
						sql += "," + boost::lexical_cast<string>(amount);
						break;
					case LOG_ACTION_WIN_SIDE_POT:
						sql += ",'wins (side pot)'";
						sql += "," + boost::lexical_cast<string>(amount);
						break;
					case LOG_ACTION_SIT_OUT:
						sql += ",'sits out'";
						sql += ",NULL";
						break;
					case LOG_ACTION_WIN_GAME:
						sql += ",'wins game'";
						sql += ",NULL";
						break;
					default:
						return;
					}
					sql += ");";
					if(myConfig->readConfigInt("LogInterval") == 0) {
						exec_transaction();
					}
				}
			}
		}
	}
}

PlayerActionLog
Log::transformPlayerActionLog(PlayerAction action)
{
	switch(action) {
	case PLAYER_ACTION_FOLD:
		return LOG_ACTION_FOLD;
		break;
	case PLAYER_ACTION_CHECK:
		return LOG_ACTION_CHECK;
		break;
	case PLAYER_ACTION_CALL:
		return LOG_ACTION_CALL;
		break;
	case PLAYER_ACTION_BET:
	case PLAYER_ACTION_RAISE:
		return LOG_ACTION_BET;
		break;
	case PLAYER_ACTION_ALLIN:
		return LOG_ACTION_ALL_IN;
		break;
	default:
		return LOG_ACTION_NONE;
	}
}

void
Log::logBoardCards(GameState bero, int boardCards[5])
{
	if(SQLITE_LOG) {

		if(myConfig->readConfigInt("LogOnOff")) {
			//if write logfiles is enabled

			if( mySqliteLogDb != 0 ) {
				// sqlite-db is open

				switch(bero) {
				case GAME_STATE_FLOP: {
					sql += "UPDATE Hand SET ";
					sql += "BoardCard_1=" + boost::lexical_cast<string>(boardCards[0]) + ",";
					sql += "BoardCard_2=" + boost::lexical_cast<string>(boardCards[1]) + ",";
					sql += "BoardCard_3=" + boost::lexical_cast<string>(boardCards[2]);
				}
				break;
				case GAME_STATE_TURN: {
					sql += "UPDATE Hand SET ";
					sql += "BoardCard_4=" + boost::lexical_cast<string>(boardCards[3]);
				}
				break;
				case GAME_STATE_RIVER: {
					sql += "UPDATE Hand SET ";
					sql += "BoardCard_5=" + boost::lexical_cast<string>(boardCards[4]);
				}
				break;
				default:
					return;
				}
				sql += " WHERE ";
				sql += "GameID=" + boost::lexical_cast<string>(curGameID) + " AND ";
				sql += "HandID=" + boost::lexical_cast<string>(curHandID);
				sql += ";";
				if(myConfig->readConfigInt("LogInterval") == 0) {
					exec_transaction();
				}
			}
		}
	}
}

void
Log::logHoleCardsHandName(GameState bero, PlayerList activePlayerList)
{
	PlayerListConstIterator it_c;

	for(it_c=activePlayerList->begin(); it_c!=activePlayerList->end(); ++it_c) {

		if( (*it_c)->getMyAction() != PLAYER_ACTION_FOLD && ( ((*it_c)->checkIfINeedToShowCards() && bero==GAME_STATE_POST_RIVER ) || ( bero!=GAME_STATE_POST_RIVER && !(*it_c)->getLogHoleCardsDone()) ) ) {

			logHoleCardsHandName(bero, activePlayerList, *it_c);

		}
	}
}

void
Log::logHoleCardsHandName(GameState bero, PlayerList activePlayerList, boost::shared_ptr<PlayerInterface> player, bool forceExecLog)
{

	if(SQLITE_LOG) {

		if(myConfig->readConfigInt("LogOnOff")) {
			//if write logfiles is enabled

			if( mySqliteLogDb != 0) {

				int myCards[2];
				player->getMyCards(myCards);
				sql += "UPDATE Hand SET ";
				if(bero==GAME_STATE_POST_RIVER && player->getMyCardsValueInt()>0) {
					sql += "Seat_" + boost::lexical_cast<string>(player->getMyID()+1) + "_Hand_text=\"" + CardsValue::determineHandName(player->getMyCardsValueInt(),activePlayerList) + "\"";
					sql += ",Seat_" + boost::lexical_cast<string>(player->getMyID()+1) + "_Hand_int=" + boost::lexical_cast<string>(player->getMyCardsValueInt());
				}
				if(bero==GAME_STATE_POST_RIVER && player->getMyCardsValueInt()>0 && !player->getLogHoleCardsDone()) {
					sql+= ",";
				}
				if(!player->getLogHoleCardsDone()) {
					sql += "Seat_" + boost::lexical_cast<string>(player->getMyID()+1) + "_Card_1=" + boost::lexical_cast<string>(myCards[0]);
					sql += ",Seat_" + boost::lexical_cast<string>(player->getMyID()+1) + "_Card_2=" + boost::lexical_cast<string>(myCards[1]);
				}
				sql += " WHERE ";
				sql += "GameID=" + boost::lexical_cast<string>(curGameID) + " AND ";
				sql += "HandID=" + boost::lexical_cast<string>(curHandID);
				sql += ";";
				if(myConfig->readConfigInt("LogInterval") == 0 || forceExecLog) {
					exec_transaction();
				}

				if(!player->getLogHoleCardsDone()) {
					logPlayerAction(bero,player->getMyID()+1,LOG_ACTION_SHOW);
				} else {
					logPlayerAction(bero,player->getMyID()+1,LOG_ACTION_HAS);
				}

				player->setLogHoleCardsDone(true);

			}
		}
	}
}

void
Log::logHandWinner(PlayerList activePlayerList, int highestCardsValue, std::list<unsigned> winners)
{


	PlayerListConstIterator it_c;
	list<unsigned>::iterator it_int;

	// log winner
	for(it_c=activePlayerList->begin(); it_c!=activePlayerList->end(); ++it_c) {
		if((*it_c)->getMyAction() != PLAYER_ACTION_FOLD && (*it_c)->getMyCardsValueInt() == highestCardsValue) {
			logPlayerAction(GAME_STATE_POST_RIVER,(*it_c)->getMyID()+1,LOG_ACTION_WIN,(*it_c)->getLastMoneyWon());
		}
	}

	// log side pot winner
	for(it_c=activePlayerList->begin(); it_c!=activePlayerList->end(); ++it_c) {
		if((*it_c)->getMyAction() != PLAYER_ACTION_FOLD && (*it_c)->getMyCardsValueInt() != highestCardsValue ) {

			for(it_int = winners.begin(); it_int != winners.end(); ++it_int) {
				if((*it_int) == (*it_c)->getMyUniqueID()) {
					logPlayerAction(GAME_STATE_POST_RIVER,(*it_c)->getMyID()+1,LOG_ACTION_WIN_SIDE_POT,(*it_c)->getLastMoneyWon());
				}
			}
		}
	}

}

void
Log::logGameWinner(PlayerList activePlayerList)
{

	int playersPositiveCashCounter = 0;
	PlayerListConstIterator it_c;
	for (it_c=activePlayerList->begin(); it_c!=activePlayerList->end(); ++it_c) {
		if ((*it_c)->getMyCash() > 0) playersPositiveCashCounter++;
	}
	if (playersPositiveCashCounter==1) {
		for (it_c=activePlayerList->begin(); it_c!=activePlayerList->end(); ++it_c) {
			if ((*it_c)->getMyCash() > 0) {
				logPlayerAction(GAME_STATE_POST_RIVER,(*it_c)->getMyID()+1,LOG_ACTION_WIN_GAME);
			}
		}
		// for log after every game
		logAfterGame();
	}
}

void
Log::logPlayerSitsOut(PlayerList activePlayerList)
{

	PlayerListConstIterator it_c;

	for(it_c=activePlayerList->begin(); it_c!=activePlayerList->end(); ++it_c) {

		if((*it_c)->getMyCash() == 0) {
			logPlayerAction(GAME_STATE_POST_RIVER, (*it_c)->getMyID()+1, LOG_ACTION_SIT_OUT);
		}
	}

}

void
Log::logAfterHand()
{
	if(myConfig->readConfigInt("LogInterval") == 1) {
		exec_transaction();
	}
}

void
Log::logAfterGame()
{
	if(myConfig->readConfigInt("LogInterval") == 2) {
		exec_transaction();
	}
}

void
Log::exec_transaction()
{
	char *errmsg = NULL;

	string sql_transaction = "BEGIN;" + sql + "COMMIT;";
	sql = "";

	if(sqlite3_exec(mySqliteLogDb, sql_transaction.data(), 0, 0, &errmsg) != SQLITE_OK) {
		cout << "Error in statement: " << sql_transaction.data() << "[" << errmsg << "]." << endl;
		sqlite3_free(errmsg);
		errmsg = NULL;
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
