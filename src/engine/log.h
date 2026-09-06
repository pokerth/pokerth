/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 *****************************************************************************/

#ifndef LOG_H
#define LOG_H

#include <string>
#include <set>
#include <vector>
#include <atomic>
#include <mutex>
#include <boost/filesystem.hpp>

#include "engine_defs.h"
#include "game_defs.h"

#include <QSqlDatabase>
#include <QString>

class ConfigFile;

class Log
{

public:
    Log(ConfigFile *c);

    ~Log();

    // Legt den Namen der Logdatei fest. Die Datei selbst entsteht erst mit dem
    // ersten echten Log-Eintrag (createLogDb()) - wer nicht spielt, hinterlaesst
    // also keine leere .pdb-Datei.
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
    void flushLog();  // Force flush pending SQL statements (used when leaving game early)
//    void closeLogDbAtExit();

    // Zuschauer-Modus: Solange gesetzt, schreibt der Log nichts in die .pdb-Datei.
    // Ein Zuschauer nimmt am Spiel nicht teil, also entsteht auch kein Logfile-
    // Inhalt. Gesetzt/zurueckgesetzt von ClientThread::SetSpectating().
    void setRecordingSuspended(bool suspended)
    {
        myRecordingSuspended = suspended;
    }

    void setCurrentRound(GameState theValue)
    {
        currentRound = theValue;
    }

    std::string getMySqliteLogFileName()
    {
        return mySqliteLogFileName.string();
    }

private:

    struct PendingPlayerLog {
        PendingPlayerLog(int gameId, int seatNumber, const std::string &playerName)
            : uniqueGameID(gameId), seat(seatNumber), name(playerName) {}

        int uniqueGameID;
        int seat;
        std::string name;
    };

    void exec_transaction();
    QSqlDatabase getDatabase() const; // Verbindung zur bereits angelegten Logdatei (legt nichts an)
    QSqlDatabase getOrCreateDatabase(); // wie getDatabase(), legt die Logdatei beim ersten Eintrag an
    bool createLogDb(); // Logdatei + Tabellen anlegen (einmalig)

    QString myConnectionName;
    QString myDatabaseFileName;  // Store DB filename for thread-local connections

    boost::filesystem::path mySqliteLogFileName;
    ConfigFile *myConfig;
    int uniqueGameID;
    int currentHandID;
    GameState currentRound;
    std::string sql;
    std::vector<PendingPlayerLog> pendingPlayerLogs;
    std::set<std::string> loggedSitsOut;  // Track players already logged as "sits out"

    // Gesetzt, solange wir nur zuschauen (siehe setRecordingSuspended()). Wird
    // vom Netzwerk-Thread gesetzt und in getOrCreateDatabase() gelesen -> atomic.
    std::atomic<bool> myRecordingSuspended{false};

    // true, sobald die Logdatei samt Tabellen existiert (createLogDb()).
    bool myDbCreated = false;

    // Serializes every access to sql and every exec_transaction(). The log is
    // normally written from the network thread, but flushLog() is also called
    // synchronously from the GUI thread (ClientThread::SendLeaveCurrentGame on
    // leave/end of game). Without this lock the two threads race on the sql
    // buffer (UB) and can start concurrent SQLite transactions on the same .pdb
    // file, blocking the GUI thread on the file lock -> frozen GUI at round end.
    // Recursive because the public log* methods call exec_transaction() while
    // already holding the lock.
    std::recursive_mutex sqlMutex;
};

#endif // LOG_H
