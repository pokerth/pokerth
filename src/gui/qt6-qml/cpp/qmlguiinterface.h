/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2025 Felix Hammer, Florian Thauer, Lothar May          *
 *****************************************************************************/

#ifndef QMLGUIINTERFACE_H
#define QMLGUIINTERFACE_H

#include <guiinterface.h>
#include <boost/shared_ptr.hpp>

class ConfigFile;
class Session;
class Game;
class ServerConnectionHandler;
class LobbyHandler;

class QmlGuiInterface : public GuiInterface
{
public:
    explicit QmlGuiInterface(ConfigFile *config, ServerConnectionHandler *handler = nullptr, LobbyHandler *lobbyHandler = nullptr);
    virtual ~QmlGuiInterface();
    
    void setConnectionHandler(ServerConnectionHandler *handler) { m_handler = handler; }
    void setLobbyHandler(LobbyHandler *lobbyHandler) { m_lobbyHandler = lobbyHandler; }

    // Session management
    virtual boost::shared_ptr<Session> getSession() override { return m_session; }
    virtual void setSession(boost::shared_ptr<Session> session) override { m_session = session; }

    // GuiInterface implementation - required methods
    virtual void initGui(int speed) override {}
    virtual gameTableImpl *getMyW() const override { return nullptr; }
    virtual guiLog* getMyGuiLog() const override { return nullptr; }

    // Refresh functions
    virtual void refreshSet() const override {}
    virtual void refreshCash() const override {}
    virtual void refreshAction(int = -1, int = -1) const override {}
    virtual void refreshChangePlayer() const override {}
    virtual void refreshPot() const override {}
    virtual void refreshGroupbox(int = -1, int = -1) const override {}
    virtual void refreshAll() const override {}
    virtual void refreshPlayerName() const override {}
    virtual void refreshButton() const override {}
    virtual void refreshGameLabels(GameState state) const override {}
    
    virtual void setPlayerAvatar(int myUniqueID, const std::string &myAvatar) const override {}
    virtual void waitForGuiUpdateDone() const override {}
    
    // Card functions
    virtual void dealBeRoCards(int) override {}
    virtual void dealHoleCards() override {}
    virtual void dealFlopCards() override {}
    virtual void dealTurnCard() override {}
    virtual void dealRiverCard() override {}
    virtual void nextPlayerAnimation() override {}
    virtual void beRoAnimation2(int) override {}
    virtual void preflopAnimation1() override {}
    virtual void preflopAnimation2() override {}
    virtual void flopAnimation1() override {}
    virtual void flopAnimation2() override {}
    virtual void turnAnimation1() override {}
    virtual void turnAnimation2() override {}
    virtual void riverAnimation1() override {}
    virtual void riverAnimation2() override {}
    virtual void postRiverAnimation1() override {}
    virtual void postRiverRunAnimation1() override {}
    virtual void flipHolecardsAllIn() override {}
    virtual void nextRoundCleanGui() override {}
    virtual void meInAction() override {}
    virtual void updateMyButtonsState() override {}
    virtual void disableMyButtons() override {}
    virtual void startTimeoutAnimation(int playerNum, int timeoutSec) override {}
    virtual void stopTimeoutAnimation(int playerNum) override {}
    
    virtual void startVoteOnKick(unsigned playerId, unsigned voteStarterPlayerId, int timeoutSec, int numVotesNeededToKick) override {}
    virtual void changeVoteOnKickButtonsState(bool showHide) override {}
    virtual void refreshVotesMonitor(int currentVotes, int numVotesNeededToKick) override {}
    virtual void endVoteOnKick() override {}
    
    // Log functions
    virtual void logPlayerActionMsg(std::string playName, int action, int setValue) override {}
    virtual void logNewGameHandMsg(int gameID, int HandID) override {}
    virtual void logPlayerWinsMsg(std::string playerName, int pot, bool main) override {}
    virtual void logPlayerSitsOut(std::string playerName) override {}
    virtual void logNewBlindsSetsMsg(int sbSet, int bbSet, std::string sbName, std::string bbName) override {}
    virtual void logDealBoardCardsMsg(int roundID, int card1, int card2, int card3, int card4 = -1, int card5 = -1) override {}
    virtual void logFlipHoleCardsMsg(std::string playerName, int card1, int card2, int cardsValueInt = -1, std::string showHas = "shows") override {}
    virtual void logPlayerWinGame(std::string playerName, int gameID) override {}
    virtual void flushLogAtGame(int gameID) override {}
    virtual void flushLogAtHand() override {}

    // ServerCallback methods
    virtual void SignalNetServerSuccess(int actionID) override {}
    virtual void SignalNetServerError(int errorID, int osErrorID) override {}

    // ClientCallback methods - important ones implemented
    virtual void SignalNetClientConnect(int actionID) override;
    virtual void SignalNetClientGameInfo(int actionID) override {}
    virtual void SignalNetClientError(int errorID, int osErrorID) override;
    virtual void SignalNetClientLoginShow() override;
    
    virtual void SignalNetClientSelfJoined(unsigned playerId, const std::string &playerName, bool isGameAdmin) override;
    virtual void SignalNetClientNotification(int notificationId) override {}
    virtual void SignalNetClientStatsUpdate(const ServerStats &stats) override {}
    virtual void SignalNetClientPingUpdate(unsigned minPing, unsigned avgPing, unsigned maxPing) override {}
    virtual void SignalNetClientShowTimeoutDialog(NetTimeoutReason reason, unsigned remainingSec) override {}
    virtual void SignalNetClientRemovedFromGame(int notificationId) override {}
    
    virtual void SignalNetClientGameListNew(unsigned gameId) override;
    virtual void SignalNetClientGameListRemove(unsigned gameId) override;
    virtual void SignalNetClientGameListUpdateMode(unsigned gameId, GameMode mode) override;
    virtual void SignalNetClientGameListUpdateAdmin(unsigned gameId, unsigned adminPlayerId) override {}
    virtual void SignalNetClientGameListPlayerJoined(unsigned gameId, unsigned playerId) override {}
    virtual void SignalNetClientGameListPlayerLeft(unsigned gameId, unsigned playerId) override {}
    virtual void SignalNetClientGameListSpectatorJoined(unsigned gameId, unsigned playerId) override {}
    virtual void SignalNetClientGameListSpectatorLeft(unsigned gameId, unsigned playerId) override {}
    
    virtual void SignalNetClientGameStart(boost::shared_ptr<Game> game) override {}
    virtual void SignalNetClientPlayerJoined(unsigned playerId, const std::string &playerName, bool isGameAdmin) override;
    virtual void SignalNetClientPlayerChanged(unsigned playerId, const std::string &newPlayerName) override;
    virtual void SignalNetClientPlayerLeft(unsigned playerId, const std::string &playerName, int removeReason) override {}
    virtual void SignalNetClientSpectatorJoined(unsigned playerId, const std::string &playerName) override {}
    virtual void SignalNetClientSpectatorLeft(unsigned playerId, const std::string &playerName, int removeReason) override {}
    virtual void SignalNetClientNewGameAdmin(unsigned playerId, const std::string &playerName) override {}
    
    virtual void SignalNetClientGameChatMsg(const std::string &playerName, const std::string &msg) override {}
    virtual void SignalNetClientLobbyChatMsg(const std::string &playerName, const std::string &msg) override;
    virtual void SignalNetClientPrivateChatMsg(const std::string &playerName, const std::string &msg) override {}
    virtual void SignalNetClientMsgBox(const std::string &msg) override {}
    virtual void SignalNetClientMsgBox(unsigned msgId) override {}
    virtual void SignalNetClientWaitDialog() override {}
    
    virtual void SignalNetClientServerListAdd(unsigned serverId) override {}
    virtual void SignalNetClientServerListClear() override {}
    virtual void SignalNetClientServerListShow() override {}
    
    virtual void SignalNetClientRejoinPossible(unsigned gameId) override {}
    virtual void SignalNetClientPostRiverShowCards(unsigned playerId) override {}
    
    virtual void SignalLobbyPlayerJoined(unsigned playerId, const std::string &nickName) override;
    virtual void SignalLobbyPlayerKicked(const std::string &nickName, const std::string &byWhom, const std::string &reason) override {}
    virtual void SignalLobbyPlayerLeft(unsigned playerId) override;
    
    virtual void SignalSelfGameInvitation(unsigned gameId, unsigned playerIdFrom) override {}
    virtual void SignalPlayerGameInvitation(unsigned gameId, unsigned playerIdWho, unsigned playerIdFrom) override {}
    virtual void SignalRejectedGameInvitation(unsigned gameId, unsigned playerIdWho, DenyGameInvitationReason reason) override {}

private:
    ConfigFile *m_config;
    boost::shared_ptr<Session> m_session;
    ServerConnectionHandler *m_handler;
    LobbyHandler *m_lobbyHandler;
};

#endif // QMLGUIINTERFACE_H
