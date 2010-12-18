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

#include "log.h"

using namespace std;

Log::Log(ConfigFile *c) : myConfig(c)
{

    if(SQLITE_LOG) {

        // logging activated
        if(myConfig->readConfigInt("LogOnOff")) {

            DIR *logDir;
            logDir = opendir(myConfig->readConfigString("LogDir").c_str());

            // check if logging path exist
            if(myConfig->readConfigString("LogDir") != "" && logDir) {

                int i;
                string sql;
                char *errmsg;

                // detect current time
                char curDateTime[20];
                char curDate[11];
                char curTime[9];
                time_t now = time(NULL);
                tm *z = localtime(&now);
                strftime(curDateTime,20,"%Y-%m-%d_%H.%M.%S",z);
                strftime(curDate,11,"%Y-%m-%d",z);
                strftime(curTime,9,"%H:%M:%S",z);

                string mySqliteLogFileName = boost::lexical_cast<string>(myConfig->readConfigString("LogDir").c_str());
                       mySqliteLogFileName += "pokerth-log-" + boost::lexical_cast<string>(curDateTime) + ".pdb";

                // open sqlite-db
                sqlite3_open(mySqliteLogFileName.data(), &mySqliteLogDb);
                if( mySqliteLogDb != 0 ) {

                    // create session table
                    sql = "CREATE TABLE Session (";
                        sql += "PokerTH_Version TEXT NOT NULL";
                        sql += ",Date TEXT NOT NULL";
                        sql += ",Time TEXT NOT NULL";
                    sql += ", PRIMARY KEY(Date,Time))";
                    if(sqlite3_exec(mySqliteLogDb, sql.data(), 0, 0, &errmsg) != SQLITE_OK) {
                        cout << "Error in statement: " << sql.data() << "[" << errmsg << "]." << endl;
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

void
Log::logNewGameMsg(int gameID, Game *currentGame) {

    if(SQLITE_LOG) {

        PlayerListConstIterator it_c;

        if(myConfig->readConfigInt("LogOnOff")) {
            //if write logfiles is enabled

            string sql;
            int i;
            char *errmsg;

            if( mySqliteLogDb != 0 ) {

                sql = "INSERT INTO Game (";
                    sql += "GameID";
                    sql += ",Startmoney";
                    sql += ",StartSb";
                    sql += ",DealerPos";
                    for(i=1; i<=MAX_NUMBER_OF_PLAYERS; i++) {
                        sql += ",Seat_" + boost::lexical_cast<std::string>(i);
                    }
                sql += ") VALUES (";
                    sql += boost::lexical_cast<string>(gameID);
                    sql += "," + boost::lexical_cast<string>(currentGame->getStartCash());
                    sql += "," + boost::lexical_cast<string>(currentGame->getStartSmallBlind());
                    sql += "," + boost::lexical_cast<string>(currentGame->getDealerPosition());

                    for(it_c = currentGame->getCurrentHand()->getSeatsList()->begin();it_c!=currentGame->getCurrentHand()->getSeatsList()->end();it_c++) {
                        if((*it_c)->getMyActiveStatus()) {
                            sql += ",\"" + (*it_c)->getMyName() +"\"";
                        } else {
                            sql += ",NULL";
                        }
                    }
                    sql += ")";
                if(sqlite3_exec(mySqliteLogDb, sql.data(), 0, 0, &errmsg) != SQLITE_OK) {
                    cout << "Error in statement: " << sql.data() << "[" << errmsg << "]." << endl;
                }
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
