/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 *****************************************************************************/

#include "log.h"

#include "configfile.h"
#include "playerinterface.h"
#include "cardsvalue.h"

#include <algorithm>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDateTime>
#include <QDir>
#include <QThread>
#include <QMutex>

#include <dirent.h>
#include <boost/lexical_cast.hpp>

using namespace std;

Log::Log(ConfigFile *c) : myConnectionName(), mySqliteLogFileName(""), myConfig(c), uniqueGameID(0), currentHandID(0), currentRound(GAME_STATE_PREFLOP), sql("")
{
}

Log::~Log()
{
    // Flush any pending SQL statements before destruction
    // This is critical when LogInterval > 0 (batch logging)
    if (!sql.empty()) {
        exec_transaction();
    }
    // Qt will automatically clean up QSqlDatabase connections on application exit
    // Attempting to manually close/remove here can cause crashes during shutdown
    // when Qt's SQL driver manager is already being destroyed
}

QSqlDatabase
Log::getDatabase() const
{
    if (myConnectionName.isEmpty() || myDatabaseFileName.isEmpty()) {
        return QSqlDatabase();
    }
    
    // Create a thread-specific connection name
    QString threadConnName = QString("%1_thread_%2")
        .arg(myConnectionName)
        .arg((qulonglong)QThread::currentThreadId());
    
    // Try to get the thread-specific connection (use false to avoid warnings)
    QSqlDatabase threadDb = QSqlDatabase::database(threadConnName, false);
    
    // If connection exists but is not open, try to open it
    if (threadDb.isValid()) {
        if (!threadDb.isOpen()) {
            threadDb.open();
        }
        return threadDb;
    }
    
    // Connection doesn't exist yet for this thread, create a new one
    // Don't touch the original connection from another thread!
    threadDb = QSqlDatabase::addDatabase("QSQLITE", threadConnName);
    threadDb.setDatabaseName(myDatabaseFileName);
    if (threadDb.open()) {
        return threadDb;
    }
    
    return QSqlDatabase();
}

void
Log::init()
{

    // SQLITE_LOG wird weiterhin als Konfig-Flag benutzt
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
                strftime(curDateTime,20,"%Y-%m-%d_%H%M%S",z);
                strftime(curDate,11,"%Y-%m-%d",z);
                strftime(curTime,9,"%H:%M:%S",z);

                mySqliteLogFileName.clear();
                mySqliteLogFileName /= myConfig->readConfigString("LogDir");
                mySqliteLogFileName /= string("pokerth-log-") + curDateTime + ".pdb";

                myConnectionName = QString("pokerth_log_%1").arg((qulonglong)QDateTime::currentMSecsSinceEpoch());
                myDatabaseFileName = QString::fromStdString(mySqliteLogFileName.string());
                QSqlDatabase mySqliteLogDb = QSqlDatabase::addDatabase("QSQLITE", myConnectionName);
                mySqliteLogDb.setDatabaseName(QString::fromStdString(mySqliteLogFileName.string()));

                if (mySqliteLogDb.open()) {

                    int i;
                    // create session table
                    sql += "CREATE TABLE Session (";
                    sql += "PokerTH_Version TEXT NOT NULL";
                    sql += ",Date TEXT NOT NULL";
                    sql += ",Time TEXT NOT NULL";
                    sql += ",LogVersion INTEGER NOT NULL";
                    sql += ", PRIMARY KEY(Date,Time));";

                    sql += "INSERT INTO Session (";
                    sql += "PokerTH_Version";
                    sql += ",Date";
                    sql += ",Time";
                    sql += ",LogVersion";
                    sql += ") VALUES (";
                    sql += "\"" + boost::lexical_cast<string>(POKERTH_BETA_RELEASE_STRING) + "\",";
                    sql += "\"" + boost::lexical_cast<string>(curDate) + "\",";
                    sql += "\"" + boost::lexical_cast<string>(curTime) + "\",";
                    sql += boost::lexical_cast<string>(SQLITE_LOG_VERSION) + ");";

                    // create game table
                    sql += "CREATE TABLE Game (";
                    sql += "UniqueGameID INTEGER PRIMARY KEY";
                    sql += ",GameID INTEGER NOT NULL";
                    sql += ",Startmoney INTEGER NOT NULL";
                    sql += ",StartSb INTEGER NOT NULL";
                    sql += ",DealerPos INTEGER NOT NULL";
                    sql += ",Winner_Seat INTEGER";
                    sql += ");";

                    // create player table
                    sql += "CREATE TABLE Player (";
                    sql += "UniqueGameID INTEGER NOT NULL";
                    sql += ",Seat INTEGER NOT NULL";
                    sql += ",Player TEXT NOT NULL";
                    sql += ",PRIMARY KEY(UniqueGameID,Seat));";

                    // create hand table
                    sql += "CREATE TABLE Hand (";
                    sql += "HandID INTEGER NOT NULL";
                    sql += ",UniqueGameID INTEGER NOT NULL";
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
                    sql += ",PRIMARY KEY(HandID,UniqueGameID));";

                    // create action table
                    sql += "CREATE TABLE Action (";
                    sql += "ActionID INTEGER PRIMARY KEY AUTOINCREMENT";
                    sql += ",HandID INTEGER NOT NULL";
                    sql += ",UniqueGameID INTEGER NOT NULL";
                    sql += ",BeRo INTEGER NOT NULL";
                    sql += ",Player INTEGER NOT NULL";
                    sql += ",Action TEXT NOT NULL";
                    sql += ",Amount INTEGER";
                    sql += ");";

                    // Execute initial setup in the current thread using the just-opened connection
                    QSqlError err;
                    if(!mySqliteLogDb.transaction()) {
                        err = mySqliteLogDb.lastError();
                        cout << "Failed to begin transaction: " << err.text().toStdString() << endl;
                    }

                    // Split the SQL buffer by ';' and execute each statement separately
                    std::string buf = sql;
                    sql.clear();

                    size_t start = 0;
                    while(true) {
                        size_t pos = buf.find(';', start);
                        std::string stmt;
                        if(pos == std::string::npos) {
                            stmt = buf.substr(start);
                        } else {
                            stmt = buf.substr(start, pos - start);
                        }
                        // trim whitespace
                        auto l = stmt.find_first_not_of(" \t\r\n");
                        auto r = stmt.find_last_not_of(" \t\r\n");
                        if(l != std::string::npos && r != std::string::npos && l <= r) {
                            stmt = stmt.substr(l, r - l + 1);
                            QSqlQuery q(mySqliteLogDb);
                            if(!q.exec(QString::fromStdString(stmt))) {
                                QSqlError qe = q.lastError();
                                cout << "Error in statement: " << stmt << " [" << qe.text().toStdString() << "]." << endl;
                            }
                        }
                        if(pos == std::string::npos) break;
                        start = pos + 1;
                    }

                    if(!mySqliteLogDb.commit()) {
                        err = mySqliteLogDb.lastError();
                        cout << "Failed to commit transaction: " << err.text().toStdString() << endl;
                    }
                } else {
                    // open failed: du kannst hier Fehlerlog ergänzen
                    cout << "Failed to open sqlite (Qt)" << endl;
                }
            }
        }
    }
}

void
Log::logNewGameMsg(int gameID, int startCash, int startSmallBlind, unsigned dealerPosition, PlayerList seatsList)
{
	uniqueGameID++;
	loggedSitsOut.clear();  // Reset sits out tracking for new game

	if(SQLITE_LOG) {

		if(myConfig->readConfigInt("LogOnOff")) {
			//if write logfiles is enabled

			PlayerListConstIterator it_c;

			QSqlDatabase db = getDatabase();
			if(db.isValid() && db.isOpen()) {
				// sqlite-db is open
				int i;

				sql += "INSERT INTO Game (";
				sql += "UniqueGameID";
				sql += ",GameID";
				sql += ",Startmoney";
			 sql += ",StartSb";
			 sql += ",DealerPos";
			 sql += ") VALUES (";
			 sql += boost::lexical_cast<string>(uniqueGameID);
			 sql += "," + boost::lexical_cast<string>(gameID);
			 sql += "," + boost::lexical_cast<string>(startCash);
			 sql += "," + boost::lexical_cast<string>(startSmallBlind);
			 sql += "," + boost::lexical_cast<string>(dealerPosition);
			 sql += ");";

				i = 1;
				for(it_c = seatsList->begin(); it_c!=seatsList->end(); ++it_c) {
					if((*it_c)->getMyActiveStatus()) {
						sql += "INSERT INTO Player (";
						sql += "UniqueGameID";
						sql += ",Seat";
					 sql += ",Player";
					 sql += ") VALUES (";
					 sql += boost::lexical_cast<string>(uniqueGameID);
					 sql += "," + boost::lexical_cast<string>(i);
					 sql += ",\"" + (*it_c)->getMyName() +"\"";
					 sql += ");";
					}
					i++;
				}

				exec_transaction();
			}
		}
	}
}

void
Log::logNewHandMsg(int handID, unsigned dealerPosition, int smallBlind, unsigned smallBlindPosition, int bigBlind, unsigned bigBlindPosition, PlayerList seatsList)
{

	currentRound = GAME_STATE_PREFLOP;
	currentHandID = handID;
	PlayerListConstIterator it_c;
	for(it_c = seatsList->begin(); it_c!=seatsList->end(); ++it_c) {
		(*it_c)->setLogHoleCardsDone(false);
	}

	if(SQLITE_LOG) {

		if(myConfig->readConfigInt("LogOnOff")) {
			//if write logfiles is enabled

			QSqlDatabase db = getDatabase();
			if(db.isValid() && db.isOpen()) {
				// sqlite-db is open
			 int i;

				sql += "INSERT INTO Hand (";
				sql += "HandID";
				sql += ",UniqueGameID";
				sql += ",Dealer_Seat";
			 sql += ",Sb_Amount";
			 sql += ",Sb_Seat";
			 sql += ",Bb_Amount";
			 sql += ",Bb_Seat";
			 for(i=1; i<=MAX_NUMBER_OF_PLAYERS; i++) {
				 sql += ",Seat_" + boost::lexical_cast<std::string>(i) + "_Cash";
			 }
			 sql += ") VALUES (";
			 sql += boost::lexical_cast<string>(currentHandID);
			 sql += "," + boost::lexical_cast<string>(uniqueGameID);
			 sql += "," + boost::lexical_cast<string>(dealerPosition);
			 sql += "," + boost::lexical_cast<string>(smallBlind);
			 sql += "," + boost::lexical_cast<string>(smallBlindPosition);
			 sql += "," + boost::lexical_cast<string>(bigBlind);
			 sql += "," + boost::lexical_cast<string>(bigBlindPosition);
			 for(it_c = seatsList->begin(); it_c!=seatsList->end(); ++it_c) {
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
				for(it_c = seatsList->begin(); it_c!=seatsList->end(); ++it_c) {
					if((*it_c)->getMyActiveStatus()) {
						countActivePlayer++;
						if((*it_c)->getMyButton()==BUTTON_DEALER && (*it_c)->getMyActiveStatus()) {
							dealerButtonOnTable = true;
						}
					}
				}
				if(countActivePlayer==2) {
					logPlayerAction(smallBlindPosition,LOG_ACTION_DEALER);
				} else {
					if(dealerButtonOnTable) {
						logPlayerAction(dealerPosition,LOG_ACTION_DEALER);
					}
				}

				// log blinds
				for(it_c = seatsList->begin(); it_c!=seatsList->end(); ++it_c) {
					if((*it_c)->getMyButton() == BUTTON_SMALL_BLIND && (*it_c)->getMySet()>0) {
						logPlayerAction(smallBlindPosition,LOG_ACTION_SMALL_BLIND,(*it_c)->getMySet());
					}
				}
				for(it_c = seatsList->begin(); it_c!=seatsList->end(); ++it_c) {
					if((*it_c)->getMyButton() == BUTTON_BIG_BLIND && (*it_c)->getMySet()>0) {
						logPlayerAction(bigBlindPosition,LOG_ACTION_BIG_BLIND,(*it_c)->getMySet());
					}
				}

				// (*it_c)->getMySet() ist ein Hack, da es im Internetspiel vorkam, dass ein Spieler zweimal geloggt wurde mit Blind - einmal jedoch mit $0

				// !! TODO !! Hack

			}
		}
	}
}

void
Log::logPlayerAction(string playerName, PlayerActionLog action, int amount)
{

    if(SQLITE_LOG) {

        if(myConfig->readConfigInt("LogOnOff")) {
            //if write logfiles is enabled

            if(QSqlDatabase::contains(myConnectionName) && getDatabase().isOpen()) {
                // sqlite-db (Qt) is open

                // read seat using QSqlQuery
                QSqlQuery q(getDatabase());
                q.prepare(QString::fromUtf8("SELECT Seat FROM Player WHERE UniqueGameID = ? AND Player = ?"));
                q.addBindValue(uniqueGameID);
                q.addBindValue(QString::fromStdString(playerName));
                if(!q.exec()) {
                    QSqlError err = q.lastError();
                    cout << "Error in statement: SELECT Seat ... [" << err.text().toStdString() << "]." << endl;
                } else {
                    if(q.next()) {
                        int seat = q.value(0).toInt();
                        logPlayerAction(seat, action, amount);
                    } else {
                        cout << "Implausible information about player " << playerName << " in log-db!" << endl;
                    }
                }
            }
        }
    }
}

void
Log::logPlayerAction(int seat, PlayerActionLog action, int amount)
{

    if(SQLITE_LOG) {

        if(myConfig->readConfigInt("LogOnOff")) {
            //if write logfiles is enabled

            QSqlDatabase db = getDatabase();
            if(db.isValid() && db.isOpen()) {
                // sqlite-db (Qt) is open

                if(action!=LOG_ACTION_NONE) {
                    sql += "INSERT INTO Action (";
                    sql += "HandID";
                    sql += ",UniqueGameID";
                    sql += ",BeRo";
                    sql += ",Player";
                    sql += ",Action";
                    sql += ",Amount";
                    sql += ") VALUES (";
                    sql += boost::lexical_cast<string>(currentHandID);
                    sql += "," + boost::lexical_cast<string>(uniqueGameID);
                    sql += "," + boost::lexical_cast<string>(currentRound);
                    sql += "," + boost::lexical_cast<string>(seat);

                    // Erzeuge Action-Text und einen einzelnen Wert für Amount (Zahl oder NULL)
                    std::string actionText;
                    std::string amountText = "NULL";
                    switch(action) {
                    case LOG_ACTION_DEALER:
                        actionText = "starts as dealer";
                        break;
                    case LOG_ACTION_SMALL_BLIND:
                        actionText = "posts small blind";
                        amountText = boost::lexical_cast<string>(amount);
                        break;
                    case LOG_ACTION_BIG_BLIND:
                        actionText = "posts big blind";
                        amountText = boost::lexical_cast<string>(amount);
                        break;
                    case LOG_ACTION_FOLD:
                        actionText = "folds";
                        break;
                    case LOG_ACTION_CHECK:
                        actionText = "checks";
                        break;
                    case LOG_ACTION_CALL:
                        actionText = "calls";
                        amountText = boost::lexical_cast<string>(amount);
                        break;
                    case LOG_ACTION_BET:
                        actionText = "bets";
                        amountText = boost::lexical_cast<string>(amount);
                        break;
                    case LOG_ACTION_ALL_IN:
                        actionText = "is all in with";
                        amountText = boost::lexical_cast<string>(amount);
                        break;
                    case LOG_ACTION_SHOW:
                        actionText = "shows";
                        break;
                    case LOG_ACTION_HAS:
                        actionText = "has";
                        break;
                    case LOG_ACTION_WIN:
                        actionText = "wins";
                        amountText = boost::lexical_cast<string>(amount);
                        break;
                    case LOG_ACTION_WIN_SIDE_POT:
                        actionText = "wins (side pot)";
                        amountText = boost::lexical_cast<string>(amount);
                        break;
                    case LOG_ACTION_SIT_OUT:
                        actionText = "sits out";
                        break;
                    case LOG_ACTION_WIN_GAME:
                        actionText = "wins game";
                        break;
                    case LOG_ACTION_LEFT:
                        actionText = "has left the game";
                        break;
                    case LOG_ACTION_KICKED:
                        actionText = "was kicked from the game";
                        break;
                    case LOG_ACTION_ADMIN:
                        actionText = "is game admin now";
                        break;
                    case LOG_ACTION_JOIN:
                        actionText = "has joined the game";
                        break;
                    default:
                        return;
                    }

                    sql += ",'" + actionText + "'";
                    sql += "," + amountText;
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
Log::logBoardCards(int boardCards[5])
{
    if(SQLITE_LOG) {

        if(myConfig->readConfigInt("LogOnOff")) {
            //if write logfiles is enabled

            QSqlDatabase db = getDatabase();
            if(db.isValid() && db.isOpen()) {
                // sqlite-db is open

                switch(currentRound) {
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
                sql += "UniqueGameID=" + boost::lexical_cast<string>(uniqueGameID) + " AND ";
                sql += "HandID=" + boost::lexical_cast<string>(currentHandID);
                sql += ";";
                if(myConfig->readConfigInt("LogInterval") == 0) {
                    exec_transaction();
                }
            }
        }
    }
}

void
Log::logHoleCardsHandName(PlayerList activePlayerList)
{
	PlayerListConstIterator it_c;

	for(it_c=activePlayerList->begin(); it_c!=activePlayerList->end(); ++it_c) {

		if( (*it_c)->getMyAction() != PLAYER_ACTION_FOLD && ( ((*it_c)->checkIfINeedToShowCards() && currentRound==GAME_STATE_POST_RIVER ) || ( currentRound!=GAME_STATE_POST_RIVER && !(*it_c)->getLogHoleCardsDone()) ) ) {

			logHoleCardsHandName(activePlayerList, *it_c);

		}
	}
}

void
Log::logHoleCardsHandName(PlayerList activePlayerList, boost::shared_ptr<PlayerInterface> player, bool forceExecLog)
{

	if(SQLITE_LOG) {

		if(myConfig->readConfigInt("LogOnOff")) {
			//if write logfiles is enabled

			QSqlDatabase db = getDatabase();
			if(db.isValid() && db.isOpen()) {
                // sqlite-db (Qt) is open

				int myCards[2];
				player->getMyCards(myCards);
				sql += "UPDATE Hand SET ";
				if(currentRound==GAME_STATE_POST_RIVER && player->getMyCardsValueInt()>0) {
					sql += "Seat_" + boost::lexical_cast<string>(player->getMyID()+1) + "_Hand_text=\"" + CardsValue::determineHandName(player->getMyCardsValueInt(),activePlayerList) + "\"";
					sql += ",Seat_" + boost::lexical_cast<string>(player->getMyID()+1) + "_Hand_int=" + boost::lexical_cast<string>(player->getMyCardsValueInt());
				}
				if(currentRound==GAME_STATE_POST_RIVER && player->getMyCardsValueInt()>0 && !player->getLogHoleCardsDone()) {
					sql+= ",";
				}
				if(!player->getLogHoleCardsDone()) {
					sql += "Seat_" + boost::lexical_cast<string>(player->getMyID()+1) + "_Card_1=" + boost::lexical_cast<string>(myCards[0]);
					sql += ",Seat_" + boost::lexical_cast<string>(player->getMyID()+1) + "_Card_2=" + boost::lexical_cast<string>(myCards[1]);
				}
				sql += " WHERE ";
				sql += "UniqueGameID=" + boost::lexical_cast<string>(uniqueGameID) + " AND ";
				sql += "HandID=" + boost::lexical_cast<string>(currentHandID);
				sql += ";";
				if(myConfig->readConfigInt("LogInterval") == 0 || forceExecLog) {
					exec_transaction();
				}

				if(!player->getLogHoleCardsDone()) {
					logPlayerAction(player->getMyName(),LOG_ACTION_SHOW);
				} else {
					logPlayerAction(player->getMyName(),LOG_ACTION_HAS);
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

	// log all winners - Server determines who wins and sends correct MoneyWon values
	for(it_c=activePlayerList->begin(); it_c!=activePlayerList->end(); ++it_c) {
		// Check if player is in winners list
		bool isWinner = std::find(winners.begin(), winners.end(), (*it_c)->getMyUniqueID()) != winners.end();
		if(isWinner && (*it_c)->getMyAction() != PLAYER_ACTION_FOLD && (*it_c)->getLastMoneyWon() > 0) {
			logPlayerAction((*it_c)->getMyName(),LOG_ACTION_WIN,(*it_c)->getLastMoneyWon());
		}
	}

	// Side pot logging removed - all winners logged above as main winners

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
				logPlayerAction((*it_c)->getMyName(),LOG_ACTION_WIN_GAME);
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
			// Only log if not already logged as "sits out" in this game
			if(loggedSitsOut.find((*it_c)->getMyName()) == loggedSitsOut.end()) {
				logPlayerAction((*it_c)->getMyName(), LOG_ACTION_SIT_OUT);
				loggedSitsOut.insert((*it_c)->getMyName());
			}
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
Log::flushLog()
{
	// Force flush pending SQL statements regardless of LogInterval
	// Used when leaving game early to ensure all data is written
	if (!sql.empty()) {
		exec_transaction();
	}
}

void
Log::exec_transaction()
{
    // Execute accumulated SQL statements using QSqlQuery inside a Qt transaction.
    // Check if connection exists before accessing the database
    if (!myConnectionName.isEmpty()) {
        QSqlDatabase db = getDatabase();
        if (!(db.isValid() && db.isOpen())) {
            sql.clear();
            return;
        }
        
        QSqlError err;
        if(!db.transaction()) {
            err = db.lastError();
            cout << "Failed to begin transaction: " << err.text().toStdString() << endl;
            // Try to execute without transaction fallback
        }

    // Split the SQL buffer by ';' and execute each statement separately
    std::string buf = sql;
    sql.clear();

    bool hasError = false;
    size_t start = 0;
    while(true) {
        size_t pos = buf.find(';', start);
        std::string stmt;
        if(pos == std::string::npos) {
            stmt = buf.substr(start);
        } else {
            stmt = buf.substr(start, pos - start);
        }
        // trim whitespace
        auto l = stmt.find_first_not_of(" \t\r\n");
        auto r = stmt.find_last_not_of(" \t\r\n");
        if(l != std::string::npos && r != std::string::npos && l <= r) {
            stmt = stmt.substr(l, r - l + 1);
            QSqlQuery q(db);
            if(!q.exec(QString::fromStdString(stmt))) {
                QSqlError qe = q.lastError();
                cout << "Error in statement: " << stmt << " [" << qe.text().toStdString() << "]." << endl;
                hasError = true;
            }
        }
        if(pos == std::string::npos) break;
        start = pos + 1;
    }

    if(hasError) {
        db.rollback();
        cout << "Transaction rolled back due to errors." << endl;
    } else if(!db.commit()) {
        err = db.lastError();
        cout << "Failed to commit transaction: " << err.text().toStdString() << endl;
    }
    } else {
        sql.clear();
    }
}
