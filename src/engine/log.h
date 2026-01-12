/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 *****************************************************************************/

#ifndef LOG_H
#define LOG_H

#include <string>
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

    void setCurrentRound(GameState theValue)
    {
        currentRound = theValue;
    }

    std::string getMySqliteLogFileName()
    {
        return mySqliteLogFileName.string();
    }

private:

    void exec_transaction();
    QSqlDatabase getDatabase() const; // Helper to get the database connection

    QString myConnectionName;

    boost::filesystem::path mySqliteLogFileName;
    ConfigFile *myConfig;
    int uniqueGameID;
    int currentHandID;
    GameState currentRound;
    std::string sql;
};

#endif // LOG_H
