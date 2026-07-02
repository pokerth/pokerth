/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2025 Felix Hammer, Florian Thauer, Lothar May          *
 *****************************************************************************/

#include "lobbyhandler.h"
#include "chatemotes.h"
#include "gui/chat_emote_shortcuts.h"
#include "session.h"
#include "game.h"
#include "playerinterface.h"
#include "configfile.h"
#include "soundevents.h"
#include "net/socket_msg.h"
#include "gamedata.h"
#include "core/appimage_utils.h"

#include <QRegularExpression>
#include <QProcess>
#include <QProcessEnvironment>
#include <QUrl>
#include <QStringList>
#include <QDateTime>


class PlayerNickListSortFilterProxyModel : public QSortFilterProxyModel
{
public:
    explicit PlayerNickListSortFilterProxyModel(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
        , m_filterState(0)
        , m_lastFilterStateCountry(false)
        , m_lastFilterStateAlpha(true)
        , m_session(nullptr)
    {
    }

    void setSession(Session *session)
    {
        m_session = session;
        invalidateFilter();
    }

    void setFilterState(int state)
    {
        if (m_filterState == 0) {
            m_lastFilterStateCountry = false;
            m_lastFilterStateAlpha = true;
        } else if (m_filterState == 1) {
            m_lastFilterStateCountry = true;
            m_lastFilterStateAlpha = false;
        }

        m_filterState = state;
        invalidateFilter();
        sort(0, Qt::AscendingOrder);
    }

    void refresh()
    {
        invalidateFilter();
        sort(0, Qt::AscendingOrder);
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return sourceModel() ? sourceModel()->roleNames() : QHash<int, QByteArray>();
    }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        if (!QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent))
            return false;

        if (m_filterState == 2) {
            if (!m_session)
                return false;

            QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
            unsigned playerId = sourceModel()->data(idx, PlayerListModel::PlayerIdRole).toUInt();
            return m_session->getGameIdOfPlayer(playerId) == 0;
        }

        return true;
    }

    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override
    {
        QString leftName = sourceModel()->data(left, PlayerListModel::PlayerNameRole).toString().toLower();
        QString rightName = sourceModel()->data(right, PlayerListModel::PlayerNameRole).toString().toLower();

        if (m_filterState == 1) {
            QString leftCountry = sourceModel()->data(left, PlayerListModel::CountryCodeRole).toString().toUpper();
            QString rightCountry = sourceModel()->data(right, PlayerListModel::CountryCodeRole).toString().toUpper();
            return (leftCountry + leftName) < (rightCountry + rightName);
        }

        if (m_filterState == 2 && m_lastFilterStateCountry) {
            QString leftCountry = sourceModel()->data(left, PlayerListModel::CountryCodeRole).toString().toUpper();
            QString rightCountry = sourceModel()->data(right, PlayerListModel::CountryCodeRole).toString().toUpper();
            return (leftCountry + leftName) < (rightCountry + rightName);
        }

        return leftName < rightName;
    }

private:
    int m_filterState;
    bool m_lastFilterStateCountry;
    bool m_lastFilterStateAlpha;
    Session *m_session;
};

class GameListSortFilterProxyModel : public QSortFilterProxyModel
{
public:
    explicit GameListSortFilterProxyModel(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
        , m_filterMode(0)
        , m_session(nullptr)
    {
    }

    void setSession(Session *session)
    {
        m_session = session;
        invalidateFilter();
    }

    void setFilterMode(int mode)
    {
        if (mode < 0 || mode > 5)
            mode = 0;

        if (m_filterMode == mode)
            return;

        m_filterMode = mode;
        invalidateFilter();
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return sourceModel() ? sourceModel()->roleNames() : QHash<int, QByteArray>();
    }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        if (!QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent))
            return false;

        QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
        if (!idx.isValid())
            return false;

        const unsigned gameId = sourceModel()->data(idx, GameListModel::GameIdRole).toUInt();
        const int gameMode = sourceModel()->data(idx, GameListModel::GameModeRole).toInt();
        const int playerCount = sourceModel()->data(idx, GameListModel::PlayerCountRole).toInt();
        const int maxPlayers = sourceModel()->data(idx, GameListModel::MaxPlayersRole).toInt();
        const bool isPrivate = sourceModel()->data(idx, GameListModel::IsPrivateRole).toBool();
        const int gameType = sourceModel()->data(idx, GameListModel::GameTypeRole).toInt();

        if (m_session && gameId != 0 && m_session->getClientCurrentGameId() == gameId)
            return true;

        const bool isOpen = (gameMode == GAME_MODE_CREATED);
        const bool isNonFull = (playerCount < maxPlayers);
        const bool isRanking = (gameType == GAME_TYPE_RANKING);

        switch (m_filterMode) {
        case 0:
            return true;
        case 1:
            return isOpen;
        case 2:
            return isOpen && isNonFull;
        case 3:
            return isOpen && isNonFull && !isPrivate;
        case 4:
            return isOpen && isNonFull && isPrivate;
        case 5:
            return isOpen && isNonFull && isRanking;
        default:
            return true;
        }
    }

private:
    int m_filterMode;
    Session *m_session;
};

// PlayerListModel implementation
PlayerListModel::PlayerListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int PlayerListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_players.count();
}

QVariant PlayerListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_players.count())
        return QVariant();

    const PlayerInfo &player = m_players.at(index.row());
    
    switch (role) {
    case PlayerIdRole:
        return player.id;
    case PlayerNameRole:
        return player.name;
    case IsAdminRole:
        return player.isAdmin;
    case CountryCodeRole:
        return player.countryCode;
    case IsGuestRole:
        return player.isGuest;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> PlayerListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[PlayerIdRole] = "playerId";
    roles[PlayerNameRole] = "playerName";
    roles[IsAdminRole] = "isAdmin";
    roles[CountryCodeRole] = "countryCode";
    roles[IsGuestRole] = "isGuest";
    return roles;
}

void PlayerListModel::addPlayer(unsigned playerId, const QString &playerName, bool isAdmin, const QString &countryCode, bool isGuest)
{
    // Check if player already exists
    if (m_playerIndexMap.contains(playerId)) {
        qWarning() << "Player" << playerId << "already in list";
        return;
    }
    
    int newRow = m_players.count();
    beginInsertRows(QModelIndex(), newRow, newRow);
    
    PlayerInfo player;
    player.id = playerId;
    player.name = playerName;
    player.isAdmin = isAdmin;
    player.countryCode = countryCode;
    player.isGuest = isGuest;
    m_players.append(player);
    m_playerIndexMap[playerId] = newRow;
    
    endInsertRows();
    emit countChanged();
}

void PlayerListModel::removePlayer(unsigned playerId)
{
    if (!m_playerIndexMap.contains(playerId)) {
        qWarning() << "Player" << playerId << "not found";
        return;
    }
    
    int row = m_playerIndexMap[playerId];
    beginRemoveRows(QModelIndex(), row, row);
    
    m_players.removeAt(row);
    m_playerIndexMap.remove(playerId);
    
    // Update indices for remaining players
    for (int i = row; i < m_players.count(); ++i) {
        m_playerIndexMap[m_players[i].id] = i;
    }
    
    endRemoveRows();
    emit countChanged();
}

void PlayerListModel::updatePlayer(unsigned playerId, const QString &newName)
{
    if (!m_playerIndexMap.contains(playerId))
        return;
    
    int row = m_playerIndexMap[playerId];
    m_players[row].name = newName;
    
    QModelIndex idx = index(row);
    emit dataChanged(idx, idx, {PlayerNameRole});
}

void PlayerListModel::updatePlayerInfo(unsigned playerId, const QString &playerName, bool isAdmin, const QString &countryCode, bool isGuest)
{
    if (!m_playerIndexMap.contains(playerId))
        return;
    
    int row = m_playerIndexMap[playerId];
    m_players[row].name = playerName;
    m_players[row].isAdmin = isAdmin;
    if (!countryCode.isEmpty())
        m_players[row].countryCode = countryCode;
    m_players[row].isGuest = isGuest;
    
    QModelIndex idx = index(row);
    emit dataChanged(idx, idx);
}

void PlayerListModel::clear()
{
    beginResetModel();
    m_players.clear();
    m_playerIndexMap.clear();
    endResetModel();
    emit countChanged();
}

// GameListModel implementation
GameListModel::GameListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int GameListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_games.count();
}

QVariant GameListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_games.count())
        return QVariant();

    const GameEntry &game = m_games.at(index.row());
    
    switch (role) {
    case GameIdRole:
        return game.id;
    case GameNameRole:
        return game.name;
    case PlayerCountRole:
        return game.playerCount;
    case MaxPlayersRole:
        return game.maxPlayers;
    case GameModeRole:
        return game.gameMode;
    case IsPrivateRole:
        return game.isPrivate;
    case GameTypeRole:
        return game.gameType;
    case FirstSmallBlindRole:
        return game.firstSmallBlind;
    case StartMoneyRole:
        return game.startMoney;
    case RaiseIntervalModeRole:
        return game.raiseIntervalMode;
    case RaiseEveryHandsRole:
        return game.raiseEveryHands;
    case RaiseEveryMinutesRole:
        return game.raiseEveryMinutes;
    case RaiseModeRole:
        return game.raiseMode;
    case ManualBlindsTextRole:
        return game.manualBlindsText;
    case PlayerActionTimeoutRole:
        return game.playerActionTimeoutSec;
    case DelayBetweenHandsRole:
        return game.delayBetweenHandsSec;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> GameListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[GameIdRole] = "gameId";
    roles[GameNameRole] = "gameName";
    roles[PlayerCountRole] = "playerCount";
    roles[MaxPlayersRole] = "maxPlayers";
    roles[GameModeRole] = "gameMode";
    roles[IsPrivateRole] = "isPrivate";
    roles[GameTypeRole] = "gameType";
    roles[FirstSmallBlindRole] = "firstSmallBlind";
    roles[StartMoneyRole] = "startMoney";
    roles[RaiseIntervalModeRole] = "raiseIntervalMode";
    roles[RaiseEveryHandsRole] = "raiseEveryHands";
    roles[RaiseEveryMinutesRole] = "raiseEveryMinutes";
    roles[RaiseModeRole] = "raiseMode";
    roles[ManualBlindsTextRole] = "manualBlindsText";
    roles[PlayerActionTimeoutRole] = "playerActionTimeoutSec";
    roles[DelayBetweenHandsRole] = "delayBetweenHandsSec";
    return roles;
}

void GameListModel::addGame(unsigned gameId, const QString &gameName)
{
    if (m_gameIndexMap.contains(gameId)) {
        qWarning() << "Game" << gameId << "already in list";
        return;
    }
    
    int newRow = m_games.count();
    beginInsertRows(QModelIndex(), newRow, newRow);
    
    GameEntry game;
    game.id = gameId;
    game.name = gameName.isEmpty() ? QString("Game #%1").arg(gameId) : gameName;
    game.playerCount = 0;
    game.maxPlayers = 10;
    game.gameMode = GAME_MODE_CREATED;
    game.isPrivate = false;
    game.gameType = GAME_TYPE_NORMAL;
    game.firstSmallBlind = 10;
    game.startMoney = 1000;
    game.raiseIntervalMode = RAISE_ON_HANDNUMBER;
    game.raiseEveryHands = 8;
    game.raiseEveryMinutes = 1;
    game.raiseMode = DOUBLE_BLINDS;
    game.manualBlindsText.clear();
    game.playerActionTimeoutSec = 20;
    game.delayBetweenHandsSec = 6;
    m_games.append(game);
    m_gameIndexMap[gameId] = newRow;
    
    endInsertRows();
    recomputeCounts();
}

void GameListModel::removeGame(unsigned gameId)
{
    if (!m_gameIndexMap.contains(gameId)) {
        qWarning() << "Game" << gameId << "not found";
        return;
    }
    
    int row = m_gameIndexMap[gameId];
    beginRemoveRows(QModelIndex(), row, row);
    
    m_games.removeAt(row);
    m_gameIndexMap.remove(gameId);
    
    // Update indices
    for (int i = row; i < m_games.count(); ++i) {
        m_gameIndexMap[m_games[i].id] = i;
    }
    
    endRemoveRows();
    recomputeCounts();
}

void GameListModel::updateGameMode(unsigned gameId, int mode)
{
    if (!m_gameIndexMap.contains(gameId))
        return;
    
    int row = m_gameIndexMap[gameId];
    int oldMode = m_games[row].gameMode;
    m_games[row].gameMode = mode;
    
    QModelIndex idx = index(row);
    emit dataChanged(idx, idx, {GameModeRole});

    recomputeCounts();
}

void GameListModel::updateGameInfo(unsigned gameId, const ::GameInfo &info)
{
    if (!m_gameIndexMap.contains(gameId))
        return;

    const int row = m_gameIndexMap[gameId];
    GameEntry &entry = m_games[row];

    const QString nameFromSession = QString::fromStdString(info.name);
    if (!nameFromSession.isEmpty())
        entry.name = nameFromSession;

    entry.playerCount = static_cast<int>(info.players.size());
    entry.maxPlayers = info.data.maxNumberOfPlayers > 0 ? info.data.maxNumberOfPlayers : 10;
    entry.gameMode = static_cast<int>(info.mode);
    entry.isPrivate = info.isPasswordProtected;
    entry.gameType = static_cast<int>(info.data.gameType);
    entry.firstSmallBlind = info.data.firstSmallBlind > 0 ? info.data.firstSmallBlind : 10;
    entry.startMoney = info.data.startMoney > 0 ? info.data.startMoney : 1000;
    entry.raiseIntervalMode = static_cast<int>(info.data.raiseIntervalMode);
    entry.raiseEveryHands = info.data.raiseSmallBlindEveryHandsValue;
    entry.raiseEveryMinutes = info.data.raiseSmallBlindEveryMinutesValue;
    entry.raiseMode = static_cast<int>(info.data.raiseMode);
    entry.playerActionTimeoutSec = info.data.playerActionTimeoutSec;
    entry.delayBetweenHandsSec = info.data.delayBetweenHandsSec;

    QStringList manualBlinds;
    for (std::list<int>::const_iterator it = info.data.manualBlindsList.begin(); it != info.data.manualBlindsList.end(); ++it) {
        manualBlinds << QString::number(*it);
    }
    entry.manualBlindsText = manualBlinds.join(QStringLiteral(", "));

    QModelIndex idx = index(row);
    emit dataChanged(idx, idx);
    recomputeCounts();
}

void GameListModel::recomputeCounts()
{
    int newOpenCount = 0;
    int newRunningCount = 0;

    for (const GameEntry &entry : m_games) {
        if (entry.gameMode == GAME_MODE_CREATED) {
            ++newOpenCount;
        } else if (entry.gameMode == GAME_MODE_STARTED) {
            ++newRunningCount;
        }
    }

    if (m_openCount != newOpenCount) {
        m_openCount = newOpenCount;
        emit openCountChanged();
    }

    if (m_runningCount != newRunningCount) {
        m_runningCount = newRunningCount;
        emit runningCountChanged();
    }
}

void GameListModel::clear()
{
    beginResetModel();
    m_games.clear();
    m_gameIndexMap.clear();
    endResetModel();
    recomputeCounts();
}

// LobbyHandler implementation
LobbyHandler::LobbyHandler(QObject *parent)
    : QObject(parent)
    , m_session()
    , m_config(nullptr)
    , m_playerListModel(this)
    , m_playerListProxyModel(nullptr)
    , m_gameListModel(this)
    , m_gameListProxyModel(nullptr)
    , m_myPlayerId(0)
    , m_playerListFilterMode(0)
    , m_gameListFilterMode(0)
    , m_playerListRevision(0)
    , m_gameListRevision(0)
    , m_playerIgnoreListRevision(0)
{
    auto *proxy = new PlayerNickListSortFilterProxyModel(this);
    proxy->setSourceModel(&m_playerListModel);
    proxy->setDynamicSortFilter(true);
    proxy->sort(0, Qt::AscendingOrder);
    m_playerListProxyModel = proxy;

    auto *gameProxy = new GameListSortFilterProxyModel(this);
    gameProxy->setSourceModel(&m_gameListModel);
    gameProxy->setDynamicSortFilter(true);
    m_gameListProxyModel = gameProxy;
}

LobbyHandler::~LobbyHandler()
{
	delete m_soundEvents;
}

void LobbyHandler::setSession(boost::shared_ptr<Session> session)
{
    m_session = session;

    // Always reset lobby models on session assignment to avoid stale rows
    // when reconnect/resubscribe happens without pointer change.
    m_gameListModel.clear();
    m_playerListModel.clear();

    static_cast<PlayerNickListSortFilterProxyModel *>(m_playerListProxyModel)->setSession(m_session.get());
    static_cast<PlayerNickListSortFilterProxyModel *>(m_playerListProxyModel)->refresh();
    static_cast<GameListSortFilterProxyModel *>(m_gameListProxyModel)->setSession(m_session.get());
    ++m_playerListRevision;
    emit playerListRevisionChanged();
    ++m_gameListRevision;
    emit gameListRevisionChanged();
}

void LobbyHandler::setConfig(ConfigFile *config)
{
    m_config = config;

    if (!m_config)
        return;

    int storedMode = m_config->readConfigInt("DlgGameLobbyNickListSortFilterIndex");
    if (storedMode < 0 || storedMode > 2)
        storedMode = 0;

    setPlayerListFilterMode(storedMode);

    int storedGameListMode = m_config->readConfigInt("DlgGameLobbyGameListFilterIndex");
    if (storedGameListMode < 0 || storedGameListMode > 5)
        storedGameListMode = 0;

    setGameListFilterMode(storedGameListMode);
}

void LobbyHandler::onLobbyPlayerJoined(unsigned playerId, const QString &playerName)
{
    QString countryCode;
    bool isGuest = false;
    if (m_session) {
        PlayerInfo info = m_session->getClientPlayerInfo(playerId);
        countryCode = QString::fromStdString(info.countryCode).toLower();
        isGuest = info.isGuest;
    }
    const bool isAdmin = m_session ? m_session->getClientPlayerInfo(playerId).isAdmin : false;
    m_playerListModel.addPlayer(playerId, playerName, isAdmin, countryCode, isGuest);
    static_cast<PlayerNickListSortFilterProxyModel *>(m_playerListProxyModel)->refresh();
    ++m_playerListRevision;
    emit playerListRevisionChanged();
    ++m_gameListRevision;
    emit gameListRevisionChanged();

    // Track admin status for our own player
    if (m_session && playerId == m_session->getClientUniquePlayerId()) {
        if (m_isCurrentPlayerAdmin != isAdmin) {
            m_isCurrentPlayerAdmin = isAdmin;
            emit isCurrentPlayerAdminChanged();
        }
    }
}

void LobbyHandler::onLobbyPlayerLeft(unsigned playerId)
{
    m_playerListModel.removePlayer(playerId);
    static_cast<PlayerNickListSortFilterProxyModel *>(m_playerListProxyModel)->refresh();
    ++m_playerListRevision;
    emit playerListRevisionChanged();
    ++m_gameListRevision;
    emit gameListRevisionChanged();
}

void LobbyHandler::updatePlayerName(unsigned playerId, const QString &playerName, bool isAdmin)
{
    // Den durchgereichten isAdmin-Parameter NICHT für den Admin-Status nutzen:
    // er trägt je nach Aufrufer unterschiedliche Bedeutung (Server-Admin aus
    // SignalNetClientPlayerChanged vs. Spiel-Admin aus SignalNetClientPlayerJoined).
    // Maßgeblich für kickban / Spiel-schließen ist allein der Server-Admin aus
    // der PlayerInfo der Session. Diese ist nach Eintreffen des PlayerInfoReply
    // authoritativ – dadurch heilt sich der Status selbst und wird nicht mehr
    // von einem Spiel-Beitritt überschrieben.
    QString countryCode;
    bool isGuest = false;
    bool serverAdmin = isAdmin;
    if (m_session) {
        PlayerInfo info = m_session->getClientPlayerInfo(playerId);
        countryCode = QString::fromStdString(info.countryCode).toLower();
        isGuest = info.isGuest;
        serverAdmin = info.isAdmin;
    }
    // Update in player list model
    m_playerListModel.updatePlayerInfo(playerId, playerName, serverAdmin, countryCode, isGuest);
    static_cast<PlayerNickListSortFilterProxyModel *>(m_playerListProxyModel)->refresh();
    ++m_playerListRevision;
    emit playerListRevisionChanged();
    ++m_gameListRevision;
    emit gameListRevisionChanged();

    // Check if this is our own player by comparing with session's unique player ID
    if (m_session) {
        unsigned myId = m_session->getClientUniquePlayerId();
        if (playerId == myId) {
            setMyPlayerInfo(playerId, playerName);
            // Server-Admin-Status aktualisieren (selbstheilend aus der Session).
            if (m_isCurrentPlayerAdmin != serverAdmin) {
                m_isCurrentPlayerAdmin = serverAdmin;
                emit isCurrentPlayerAdminChanged();
            }
        }
    }
}

void LobbyHandler::onGameListNew(unsigned gameId, const QString &gameName)
{
    m_gameListModel.addGame(gameId, gameName.isEmpty() ? QString("Game #%1").arg(gameId) : gameName);
    refreshGameInfo(gameId);
    ++m_gameListRevision;
    emit gameListRevisionChanged();
    static_cast<PlayerNickListSortFilterProxyModel *>(m_playerListProxyModel)->refresh();
    emit gameContextChanged();
}

void LobbyHandler::onGameListRemove(unsigned gameId)
{
    m_gameListModel.removeGame(gameId);
    ++m_gameListRevision;
    emit gameListRevisionChanged();
    static_cast<PlayerNickListSortFilterProxyModel *>(m_playerListProxyModel)->refresh();
    emit gameContextChanged();
}

void LobbyHandler::onGameListUpdateMode(unsigned gameId, int mode)
{
    m_gameListModel.updateGameMode(gameId, mode);
    refreshGameInfo(gameId);
    ++m_gameListRevision;
    emit gameListRevisionChanged();
    static_cast<PlayerNickListSortFilterProxyModel *>(m_playerListProxyModel)->refresh();
    emit gameContextChanged();
}

void LobbyHandler::onGameListChanged(unsigned gameId)
{
    refreshGameInfo(gameId);
    ++m_gameListRevision;
    emit gameListRevisionChanged();
    // Ein Spieler ist einem (offenen) Spiel beigetreten / hat es verlassen
    // (SignalNetClientGameListPlayerJoined/Left mappen hierauf). Die Zugehörigkeit
    // zu einem Spiel lebt in der Session (getGameIdOfPlayer), nicht im Quell-Modell –
    // der Spielerlisten-Filter (Modus 2) wird daher nicht automatisch neu bewertet.
    // Ohne expliziten refresh() verweilen beigetretene Spieler weiter in der
    // "verfügbar"-Liste, bis ein anderes Event zufällig einen Refresh auslöst.
    static_cast<PlayerNickListSortFilterProxyModel *>(m_playerListProxyModel)->refresh();
    ++m_playerListRevision;
    emit playerListRevisionChanged();
    emit gameContextChanged();
}

void LobbyHandler::setCurrentGameAdmin(bool isGameAdmin)
{
    // Betrifft ausschließlich den Spiel-Admin (Host) – der Server-Admin-Status
    // (kickban / Spiel schließen) bleibt davon unberührt.
    if (m_isCurrentGameAdmin != isGameAdmin) {
        m_isCurrentGameAdmin = isGameAdmin;
        emit isCurrentGameAdminChanged();
    }
}

void LobbyHandler::setMyPlayerInfo(unsigned playerId, const QString &playerName)
{
    if (m_myPlayerId != playerId) {
        m_myPlayerId = playerId;
        emit myPlayerIdChanged();
    }
    
    if (m_myPlayerName != playerName) {
        m_myPlayerName = playerName;
        emit myPlayerNameChanged();
    }

    emit gameContextChanged();
}

bool LobbyHandler::canInviteFromCurrentGame() const
{
    if (!m_session)
        return false;

    const unsigned gameId = m_session->getClientCurrentGameId();
    if (!gameId)
        return false;

    const GameInfo currentGame = m_session->getClientGameInfo(gameId);
    return currentGame.data.gameType == GAME_TYPE_INVITE_ONLY;
}

bool LobbyHandler::isMyPlayerGuest() const
{
    if (!m_session || m_myPlayerId == 0)
        return false;

    const PlayerInfo info = m_session->getClientPlayerInfo(m_myPlayerId);
    return info.isGuest;
}

bool LobbyHandler::canJoinGame(unsigned gameId) const
{
    if (!m_session || gameId == 0)
        return false;

    const GameInfo info = m_session->getClientGameInfo(gameId);

    const int mode = static_cast<int>(info.mode);
    if (mode == GAME_MODE_STARTED || mode == GAME_MODE_CLOSED)
        return false;

    const int maxPlayers = info.data.maxNumberOfPlayers > 0 ? info.data.maxNumberOfPlayers : 10;
    const int playerCount = static_cast<int>(info.players.size());
    if (playerCount >= maxPlayers)
        return false;

    if (info.isPasswordProtected)
        return false;

    const int gameType = static_cast<int>(info.data.gameType);
    if (gameType == GAME_TYPE_INVITE_ONLY || gameType == GAME_TYPE_REGISTERED_ONLY)
        return false;

    if (gameType == GAME_TYPE_RANKING) {
        if (isMyPlayerGuest())
            return false;
    }

    return gameType == GAME_TYPE_NORMAL || gameType == GAME_TYPE_RANKING;
}

void LobbyHandler::setPlayerListFilterMode(int mode)
{
    if (mode < 0 || mode > 2)
        mode = 0;

    if (m_playerListFilterMode == mode)
        return;

    m_playerListFilterMode = mode;
    static_cast<PlayerNickListSortFilterProxyModel *>(m_playerListProxyModel)->setFilterState(mode);
    ++m_playerListRevision;
    emit playerListRevisionChanged();

    if (m_config) {
        m_config->writeConfigInt("DlgGameLobbyNickListSortFilterIndex", mode);
        m_config->writeBuffer();
    }

    emit playerListFilterModeChanged();
}

void LobbyHandler::setGameListFilterMode(int mode)
{
    if (mode < 0 || mode > 5)
        mode = 0;

    if (m_gameListFilterMode == mode)
        return;

    m_gameListFilterMode = mode;
    static_cast<GameListSortFilterProxyModel *>(m_gameListProxyModel)->setFilterMode(mode);

    if (m_config) {
        m_config->writeConfigInt("DlgGameLobbyGameListFilterIndex", mode);
        m_config->writeBuffer();
    }

    emit gameListFilterModeChanged();
}

void LobbyHandler::refreshGameInfo(unsigned gameId)
{
    if (!m_session)
        return;

    const ::GameInfo info = m_session->getClientGameInfo(gameId);
    m_gameListModel.updateGameInfo(gameId, info);
}

QVariantMap LobbyHandler::playerListEntry(int row) const
{
    QVariantMap entry;

    if (!m_playerListProxyModel || row < 0)
        return entry;

    const QModelIndex index = m_playerListProxyModel->index(row, 0);
    if (!index.isValid())
        return entry;

    const unsigned playerId = m_playerListProxyModel->data(index, PlayerListModel::PlayerIdRole).toUInt();
    QString playerName = m_playerListProxyModel->data(index, PlayerListModel::PlayerNameRole).toString();
    const bool isAdmin = m_playerListProxyModel->data(index, PlayerListModel::IsAdminRole).toBool();
    QString countryCode = m_playerListProxyModel->data(index, PlayerListModel::CountryCodeRole).toString();
    const bool isGuest = m_playerListProxyModel->data(index, PlayerListModel::IsGuestRole).toBool();

    if (m_session && playerId != 0) {
        static const QRegularExpression numericPlaceholderPattern("^#?\\d+$");
        const bool nameIsPlaceholder = playerName.isEmpty() || numericPlaceholderPattern.match(playerName).hasMatch();

        if (nameIsPlaceholder || countryCode.isEmpty()) {
            const PlayerInfo info = m_session->getClientPlayerInfo(playerId);
            const QString sessionName = QString::fromStdString(info.playerName);
            const QString sessionCountryCode = QString::fromStdString(info.countryCode).toLower();

            if (nameIsPlaceholder && !sessionName.isEmpty()) {
                playerName = sessionName;
            }

            if (countryCode.isEmpty() && !sessionCountryCode.isEmpty()) {
                countryCode = sessionCountryCode;
            }
        }
    }

    entry.insert("playerId", playerId);
    entry.insert("playerName", playerName);
    entry.insert("isAdmin", isAdmin);
    entry.insert("countryCode", countryCode);
    entry.insert("isGuest", isGuest);
    return entry;
}

QStringList LobbyHandler::playerNickList() const
{
    QStringList nicks;

    // Quell-Modell (ungefiltert) durchlaufen, NICHT den Proxy: der
    // Spielerlisten-Filter (Modus 2) blendet Spieler in Spielen aus, die
    // aber für die Chat-Vervollständigung erreichbar bleiben müssen.
    static const QRegularExpression numericPlaceholderPattern("^#?\\d+$");
    const int count = m_playerListModel.rowCount();
    for (int row = 0; row < count; ++row) {
        const QModelIndex index = m_playerListModel.index(row, 0);
        if (!index.isValid())
            continue;

        const unsigned playerId = m_playerListModel.data(index, PlayerListModel::PlayerIdRole).toUInt();
        QString playerName = m_playerListModel.data(index, PlayerListModel::PlayerNameRole).toString();

        // Platzhalternamen (z. B. "#123") über die Session auflösen.
        if (m_session && playerId != 0) {
            const bool nameIsPlaceholder = playerName.isEmpty()
                || numericPlaceholderPattern.match(playerName).hasMatch();
            if (nameIsPlaceholder) {
                const QString sessionName = QString::fromStdString(m_session->getClientPlayerInfo(playerId).playerName);
                if (!sessionName.isEmpty())
                    playerName = sessionName;
            }
        }

        if (!playerName.isEmpty() && !nicks.contains(playerName))
            nicks << playerName;
    }

    return nicks;
}

QVariantList LobbyHandler::gamePlayersInGame(unsigned gameId) const
{
    QVariantList players;

    if (!m_session || gameId == 0)
        return players;

    const ::GameInfo gameInfo = m_session->getClientGameInfo(gameId);
    for (PlayerIdList::const_iterator it = gameInfo.players.begin(); it != gameInfo.players.end(); ++it) {
        const unsigned playerId = *it;
        if (playerId == 0)
            continue;

        const PlayerInfo info = m_session->getClientPlayerInfo(playerId);

        QVariantMap entry;
        entry.insert("playerId", playerId);
        entry.insert("playerName", QString::fromStdString(info.playerName));
        entry.insert("countryCode", QString::fromStdString(info.countryCode).toLower());
        entry.insert("isAdmin", info.isAdmin);
        entry.insert("isGuest", info.isGuest);

        QString avatarUrl;
        if (info.hasAvatar) {
            std::string avatarFile;
            if (m_session->getAvatarFile(info.avatar, avatarFile) && !avatarFile.empty()) {
                avatarUrl = QUrl::fromLocalFile(QString::fromStdString(avatarFile)).toString();
            }
        }
        entry.insert("avatarUrl", avatarUrl);

        players.append(entry);
    }

    return players;
}

QString LobbyHandler::playerCountryByName(const QString &name) const
{
    for (int row = 0; row < m_playerListModel.rowCount(); ++row) {
        QModelIndex idx = m_playerListModel.index(row, 0);
        if (m_playerListModel.data(idx, PlayerListModel::PlayerNameRole).toString() == name)
            return m_playerListModel.data(idx, PlayerListModel::CountryCodeRole).toString();
    }
    return QString();
}

bool LobbyHandler::openExternalUrl(const QString &url) const
{
    if (url.trimmed().isEmpty())
        return false;

    const QUrl target = QUrl::fromUserInput(url.trimmed());
    if (!target.isValid())
        return false;

#ifdef Q_OS_LINUX
    const QString targetString = target.toString();

    // External host tools must not inherit bundled Qt libraries.
    auto startDetachedHostTool = [](const QString &program, const QStringList &args) {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

        const QString origLdLibraryPath = QString::fromLocal8Bit(qgetenv("POKERTH_ORIG_LD_LIBRARY_PATH"));
        if (origLdLibraryPath.isEmpty()) {
            env.remove(QStringLiteral("LD_LIBRARY_PATH"));
        } else {
            env.insert(QStringLiteral("LD_LIBRARY_PATH"), origLdLibraryPath);
        }
        env.remove(QStringLiteral("LD_PRELOAD"));

        QProcess process;
        process.setProcessEnvironment(env);
        process.setProgram(program);
        process.setArguments(args);
        return process.startDetached();
    };

    if (startDetachedHostTool(QStringLiteral("xdg-open"), {targetString}))
        return true;

    if (startDetachedHostTool(QStringLiteral("gio"), {QStringLiteral("open"), targetString}))
        return true;

    if (startDetachedHostTool(QStringLiteral("kde-open"), {targetString}))
        return true;
#endif

    if (AppImageUtils::openUrlSafe(target))
        return true;

    return false;
}

void LobbyHandler::sendChatMessage(const QString &message)
{
    if (!m_session || message.trimmed().isEmpty())
        return;

    // Guests cannot send chat messages
    if (isMyPlayerGuest()) {
        emit errorOccurred(tr("Guests cannot send chat messages"));
        return;
    }

    QString text = message;

    try {
        if (text.startsWith(QLatin1String("/msg "), Qt::CaseInsensitive)) {
            // Private message: /msg <nick> <text>  or  /msg "<nick with spaces>" <text>
            text.remove(0, 5);
            const unsigned targetId = parsePrivateMessageTarget(text);
            if (targetId == 0) {
                emit errorOccurred(tr("Player not found"));
                return;
            }
            // Truncate to 128 bytes UTF-8 at character boundary
            while (!text.isEmpty() && text.toUtf8().size() > 128)
                text.chop(1);
            if (text.isEmpty()) return;
            m_session->sendPrivateChatMessage(targetId, text.toStdString());
        } else {
            // Lobby chat (includes /me actions — server echoes them back)
            while (!text.isEmpty() && text.toUtf8().size() > 128)
                text.chop(1);
            if (text.isEmpty()) return;
            m_session->sendLobbyChatMessage(text.toStdString());
        }
    } catch (const std::exception &e) {
        qWarning() << "Failed to send chat message:" << e.what();
        emit errorOccurred(tr("Failed to send chat message"));
    }
}

void LobbyHandler::onGamePlayerJoined()
{
    // Benachrichtigungs-Sound wie der Widgets-Client (gamelobbydialogimpl /
    // startnetworkgamedialogimpl): solange das Spiel nicht voll ist
    // "playerconnected", beim letzten Spieler "onlinegameready" (das Spiel
    // startet gleich darauf).
    if (m_config && !m_config->readConfigInt("PlayNetworkGameNotification"))
        return;
    if (!m_session || m_currentGameId == 0)
        return;
    if (!m_soundEvents && m_config)
        m_soundEvents = new SoundEvents(m_config);
    if (!m_soundEvents)
        return;
    const GameInfo info = m_session->getClientGameInfo(m_currentGameId);
    if (info.data.maxNumberOfPlayers > 0
        && static_cast<int>(info.players.size()) >= info.data.maxNumberOfPlayers)
        m_soundEvents->playSound("onlinegameready", 0);
    else
        m_soundEvents->playSound("playerconnected", 0);
}

void LobbyHandler::onTimeoutWarning(int reason, int remainingSec)
{
    // Audio-Hinweis: das Popup kann übersehen werden (Lobby wie ingame).
    if (!m_soundEvents && m_config)
        m_soundEvents = new SoundEvents(m_config);
    if (m_soundEvents)
        m_soundEvents->playSound("yourturn", 0);
    emit timeoutWarningReceived(reason, remainingSec);
}

void LobbyHandler::resetNetworkTimeout()
{
    if (m_session)
        m_session->resetNetworkTimeout();
}

void LobbyHandler::onNetworkMessage(const QString &message)
{
    emit networkMessageReceived(message);
}

void LobbyHandler::onNetworkMessageId(unsigned msgId)
{
    // Texte 1:1 wie startWindowImpl::networkMessage(unsigned).
    QString msgText;
    switch (msgId) {
    case MSG_NET_AVATAR_REPORT_ACCEPTED:
        msgText = tr("The avatar report was accepted by the server. Thank you."); break;
    case MSG_NET_AVATAR_REPORT_DUP:
        msgText = tr("This avatar was already reported by another player."); break;
    case MSG_NET_AVATAR_REPORT_REJECTED:
        msgText = tr("An error occurred while reporting the avatar."); break;
    case MSG_NET_GAMENAME_REPORT_ACCEPTED:
        msgText = tr("The game name report was accepted by the server. Thank you."); break;
    case MSG_NET_GAMENAME_REPORT_DUP:
        msgText = tr("This game name was already reported by another player."); break;
    case MSG_NET_GAMENAME_REPORT_REJECTED:
        msgText = tr("An error occurred while reporting the game name."); break;
    case MSG_NET_ADMIN_REMOVE_GAME_ACCEPTED:
        msgText = tr("The game was closed."); break;
    case MSG_NET_ADMIN_REMOVE_GAME_REJECTED:
        msgText = tr("The game could not be closed."); break;
    case MSG_NET_ADMIN_BAN_PLAYER_ACCEPTED:
        msgText = tr("The player was kicked and banned permanently."); break;
    case MSG_NET_ADMIN_BAN_PLAYER_NODB:
        msgText = tr("The player was kicked, but could not be banned because it was a guest player."); break;
    case MSG_NET_ADMIN_BAN_PLAYER_DBERROR:
        msgText = tr("The player was kicked, but could not be banned, \nbecause the nick could not be found in the database"); break;
    case MSG_NET_ADMIN_BAN_PLAYER_REJECTED:
        msgText = tr("The player could not be found."); break;
    default:
        return;   // unbekannte IDs nicht anzeigen (wie der Widgets-Client)
    }
    emit networkMessageReceived(msgText);
}

void LobbyHandler::onNetworkNotification(int notificationId)
{
    // Texte 1:1 wie startWindowImpl::networkNotification(int). Ohne diese
    // Behandlung blieb beim Erstellen/Beitreten eines Spiels jede Server-
    // Ablehnung (z. B. bereits vergebener Spielname) unbemerkt.
    QString msgText;
    switch (notificationId) {
    case NTF_NET_JOIN_IP_BLOCKED:
        msgText = tr("You cannot join this game, because another player in that game has your network address."); break;
    case NTF_NET_REMOVED_GAME_FULL:
    case NTF_NET_JOIN_GAME_FULL:
        msgText = tr("Sorry, this game is already full."); break;
    case NTF_NET_REMOVED_ALREADY_RUNNING:
    case NTF_NET_JOIN_ALREADY_RUNNING:
        msgText = tr("Unable to join - the server has already started the game."); break;
    case NTF_NET_JOIN_NOT_INVITED:
        msgText = tr("This game is of type invite-only. You cannot join this game without being invited."); break;
    case NTF_NET_JOIN_GAME_NAME_IN_USE:
        msgText = tr("This game name is already in use. Please choose a different name."); break;
    case NTF_NET_JOIN_GAME_BAD_NAME:
        msgText = tr("The game name is invalid. Please choose a different name."); break;
    case NTF_NET_JOIN_INVALID_PASSWORD:
        msgText = tr("Invalid password when joining the game.\nPlease reenter the password and try again."); break;
    case NTF_NET_JOIN_GUEST_FORBIDDEN:
        msgText = tr("You cannot join this type of game as guest."); break;
    case NTF_NET_JOIN_INVALID_SETTINGS:
        msgText = tr("The settings are invalid for this type of game."); break;
    case NTF_NET_JOIN_NO_SPECTATORS:
        msgText = tr("This game does not allow spectators."); break;
    case NTF_NET_JOIN_GAME_INVALID:
    case NTF_NET_JOIN_REJOIN_FAILED:
        msgText = tr("Could not join the game."); break;
    default:
        return;   // unbekannte IDs nicht anzeigen (wie der Widgets-Client)
    }
    emit networkMessageReceived(msgText);
}

void LobbyHandler::onLobbyChatMessage(const QString &playerName, const QString &message)
{
    // Reload ignore list fresh on every message (matches chattools.cpp refreshIgnoreList pattern)
    std::list<std::string> ignoreList;
    if (m_config)
        ignoreList = m_config->readConfigStringList("PlayerIgnoreList");

    const QString myNick      = m_myPlayerName;
    const bool    isChatBot   = (playerName == QLatin1String("(chat bot)"));

    // Drop messages from ignored players; also drop chatbot messages that
    // start with an ignored player's name (same logic as chattools.cpp)
    bool chatBotWarnIgnored = false;
    for (const auto &entry : ignoreList) {
        const QString ignoredName = QString::fromUtf8(entry.c_str());
        if (playerName == ignoredName)
            return;
        if (isChatBot && message.startsWith(ignoredName))
            chatBotWarnIgnored = true;
    }
    if (chatBotWarnIgnored)
        return;

    // Determine theme-aware colours (matches Qt-widget palette.link / palette.text)
    const bool isDark       = !m_config || (m_config->readConfigInt("DarkMode") != 0);
    const QString colorAccent = QLatin1String("#E3C800");                                    // accent gold
    const QString colorText   = isDark ? QLatin1String("#cdd3e0") : QLatin1String("#394150"); // secondary text
    const QString colorDanger = QLatin1String("#e05050");                                    // chatbot warn

    // Detect /me action before escaping
    const bool isAction = message.startsWith(QLatin1String("/me "));
    const QString rawDisplay = isAction ? message.mid(4) : message;

    // HTML-escape user-supplied content (prevents tag injection)
    QString escapedMsg = rawDisplay.toHtmlEscaped();
    // ASCII-Kürzel auf dem rohen Text umsetzen, bevor Link-/Style-Markup
    // hinzukommt (verhindert Kollisionen mit "color:#..." o. Ä.).
    escapedMsg = applyChatEmoteShortcuts(escapedMsg);

    // URL linkification
    static const QRegularExpression urlRe(QLatin1String("(https?://\\S+)"));
    escapedMsg.replace(urlRe, QLatin1String("<a href=\"\\1\">\\1</a>"));

    // Determine message style based on content
    bool isMention = false;
    QString styledMsg;

    if (isChatBot && !myNick.isEmpty() && rawDisplay.startsWith(myNick)) {
        // Chatbot addressing me: bold red
        styledMsg = QLatin1String("<span style=\"font-weight:bold; color:") + colorDanger
                    + QLatin1String(";\">") + escapedMsg + QLatin1String("</span>");
    } else if (!myNick.isEmpty() && rawDisplay.contains(myNick, Qt::CaseInsensitive)) {
        // Mention: bold accent
        isMention = true;
        styledMsg = QLatin1String("<span style=\"font-weight:bold; color:") + colorAccent
                    + QLatin1String(";\">") + escapedMsg + QLatin1String("</span>");
    } else {
        // All other messages (including own): normal text colour
        styledMsg = QLatin1String("<span style=\"font-weight:normal; color:") + colorText
                    + QLatin1String(";\">") + escapedMsg + QLatin1String("</span>");
    }

    // Unicode-Emoji in der Anzeige vergrößern (wie im Game-Chat, ~22px).
    styledMsg = enlargeEmojis(styledMsg);

    // Sound notification on mention (wie chattools.cpp im Widgets-Client)
    if (isMention && playerName != myNick) {
        if (!m_config || m_config->readConfigInt("PlayLobbyChatNotification")) {
            if (!m_soundEvents && m_config)
                m_soundEvents = new SoundEvents(m_config);
            if (m_soundEvents)
                m_soundEvents->playSound("lobbychatnotify", 0);
            emit lobbyChatMentionDetected();
        }
    }

    // Build final line
    const QString ts          = QDateTime::currentDateTime().toString("HH:mm:ss");
    const QString escapedName = playerName.toHtmlEscaped();
    QString line;
    if (isAction) {
        line = QLatin1String("[") + ts + QLatin1String("] <i>*")
               + escapedName + QLatin1String(" ") + styledMsg + QLatin1String("*</i>");
    } else {
        line = QLatin1String("[") + ts + QLatin1String("] <b>")
               + escapedName + QLatin1String(":</b> ") + styledMsg;
    }

    pushChatLine(line);
}

void LobbyHandler::onPrivateChatMessage(const QString &playerName, const QString &message)
{
    // Colour for PMs: muted text (similar to chattools.cpp italic PM style)
    const bool isDark      = !m_config || (m_config->readConfigInt("DarkMode") != 0);
    const QString colorPM  = isDark ? QLatin1String("#a0acc4") : QLatin1String("#576378");

    QString escapedMsg  = message.toHtmlEscaped();
    escapedMsg = applyChatEmoteShortcuts(escapedMsg);
    escapedMsg = enlargeEmojis(escapedMsg);

    const QString ts   = QDateTime::currentDateTime().toString("HH:mm:ss");
    const QString line = QLatin1String("[") + ts + QLatin1String("] <i><span style=\"color:")
                         + colorPM + QLatin1String(";\">")
                         + playerName.toHtmlEscaped()
                         + QLatin1String("(pm): ") + escapedMsg
                         + QLatin1String("</span></i>");
    pushChatLine(line);
}

void LobbyHandler::pushChatLine(const QString &line)
{
    m_chatLog.append(line);
    const int kMaxLines = 400;
    if (m_chatLog.size() > kMaxLines)
        m_chatLog.erase(m_chatLog.begin(), m_chatLog.begin() + (m_chatLog.size() - kMaxLines));
    emit chatLogChanged();
    emit chatLineReady(line);
}

unsigned LobbyHandler::parsePrivateMessageTarget(QString &chatText) const
{
    QString targetName;
    int endPos = -1;
    // Support quoted names: /msg "player name" text
    if (chatText.startsWith(QLatin1Char('"'))) {
        chatText.remove(0, 1);
        endPos = chatText.indexOf(QLatin1Char('"'));
    } else {
        endPos = chatText.indexOf(QLatin1Char(' '));
    }
    if (endPos > 0) {
        targetName = chatText.left(endPos);
        chatText.remove(0, endPos + 1);
    }
    chatText = chatText.trimmed();

    if (targetName.isEmpty() || chatText.isEmpty())
        return 0;

    // Look up playerId by name in the player list model
    const int count = m_playerListModel.rowCount();
    for (int i = 0; i < count; ++i) {
        const QModelIndex idx  = m_playerListModel.index(i);
        const QString     name = m_playerListModel.data(idx, PlayerListModel::PlayerNameRole).toString();
        if (name == targetName)
            return m_playerListModel.data(idx, PlayerListModel::PlayerIdRole).toUInt();
    }
    return 0;
}

void LobbyHandler::createGame(const QString &name, const QString &password,
                              int gameType, bool allowSpectators, int maxPlayers,
                              int startCash, int firstSmallBlind,
                              int raiseIntervalMode, int raiseEveryHands,
                              int raiseEveryMinutes, int raiseMode,
                              int playerActionTimeout, int delayBetweenHands)
{
    if (!m_session) {
        emit errorOccurred(tr("Not connected to server"));
        return;
    }

    GameData gameData;
    gameData.gameType                     = static_cast<GameType>(gameType);
    gameData.allowSpectators              = allowSpectators;
    gameData.maxNumberOfPlayers           = maxPlayers;
    gameData.startMoney                   = startCash;
    gameData.firstSmallBlind              = firstSmallBlind;
    gameData.raiseIntervalMode            = static_cast<RaiseIntervalMode>(raiseIntervalMode);
    gameData.raiseSmallBlindEveryHandsValue   = raiseEveryHands;
    gameData.raiseSmallBlindEveryMinutesValue = raiseEveryMinutes;
    gameData.raiseMode                    = static_cast<RaiseMode>(raiseMode);
    gameData.afterManualBlindsMode        = AFTERMB_DOUBLE_BLINDS;
    gameData.afterMBAlwaysRaiseValue      = 0;
    gameData.guiSpeed                     = 4;
    gameData.delayBetweenHandsSec         = delayBetweenHands;
    gameData.playerActionTimeoutSec       = playerActionTimeout;

    m_session->clientCreateGame(gameData, name.toStdString(), password.toStdString());
}

void LobbyHandler::joinGame(unsigned gameId, const QString &password)
{
    if (!m_session) {
        emit errorOccurred(tr("Not connected to server"));
        return;
    }
    m_session->clientJoinGame(gameId, password.toStdString());
}

void LobbyHandler::leaveGame()
{
    if (!m_session)
        return;
    m_session->sendLeaveCurrentGame();
}

void LobbyHandler::leaveServer()
{
    if (!m_session)
        return;
    // Verbindung zum Server trennen (wie startWindowImpl beim Verlassen der
    // Lobby) und den lokalen Lobby-Zustand zurücksetzen.
    m_session->terminateNetworkClient();
    if (m_isInGame) {
        m_isInGame = false;
        m_currentGameId = 0;
        emit isInGameChanged();
        emit currentGameIdChanged();
    }
}

void LobbyHandler::onSelfJoinedGame()
{
    m_currentGameId = m_session ? m_session->getClientCurrentGameId() : 0;
    if (!m_isInGame) {
        m_isInGame = true;
        emit isInGameChanged();
        emit currentGameIdChanged();
    }
    emit selfJoinedGame();
}

void LobbyHandler::onGameStarted()
{
    // Spielstart bedeutet, dass die Engine die Lobby-Nachrichten abbestellt
    // (UnsubscribeLobbyMsg). Während des Spiels treffen daher keine
    // playerListLeft-Events mehr ein – Spieler, die in dieser Zeit die Verbindung
    // trennen, blieben sonst als veraltete "idle"-Einträge in der Liste stehen.
    // Beim Rückkehren in den Warteraum/die Lobby sendet der Server via
    // ResubscribeLobbyMsg die vollständige Spielerliste erneut (playerListNew),
    // sodass die Liste hier gefahrlos geleert und anschließend frisch aufgebaut
    // wird. Spiegelt das Verhalten des Widget-Clients (Nickliste leeren bei
    // MSG_NET_GAME_CLIENT_START).
    m_playerListModel.clear();
    static_cast<PlayerNickListSortFilterProxyModel *>(m_playerListProxyModel)->refresh();
    ++m_playerListRevision;
    emit playerListRevisionChanged();

    emit gameStarted();
}

void LobbyHandler::onWaitGameDialog()
{
    // m_isInGame/m_currentGameId NICHT zurücksetzen: Bei deaktiviertem Auto-Leave
    // bleiben wir nach Spielende im (wieder geöffneten) Spiel; der Warteraum soll
    // das aktuelle Spiel weiter anzeigen. Wird der Spieler tatsächlich entfernt
    // (Auto-Leave/Kick), räumt das nachfolgende onRemovedFromGame den Zustand auf.
    emit returnToWaitRoom();
}

void LobbyHandler::onRemovedFromGame(int reason)
{
    m_isInGame = false;
    m_currentGameId = 0;
    // Spiel-Admin (Host)-Status verfällt mit dem Verlassen des Tisches; der
    // Server-Admin-Status bleibt davon unberührt.
    setCurrentGameAdmin(false);
    emit isInGameChanged();
    emit currentGameIdChanged();
    emit removedFromGame(reason);
}

QString LobbyHandler::currentGameName() const
{
    if (!m_session || m_currentGameId == 0)
        return QString();
    const GameInfo info = m_session->getClientGameInfo(m_currentGameId);
    return QString::fromStdString(info.name);
}

void LobbyHandler::startGame(bool fillWithCpu)
{
    if (!m_session)
        return;
    m_session->sendStartEvent(fillWithCpu);
}

QVariantMap LobbyHandler::currentGameInfo() const
{
    QVariantMap result;
    if (!m_session || m_currentGameId == 0)
        return result;
    const GameInfo info = m_session->getClientGameInfo(m_currentGameId);
    result.insert("name",               QString::fromStdString(info.name));
    result.insert("gameType",           static_cast<int>(info.data.gameType));
    result.insert("maxPlayers",         info.data.maxNumberOfPlayers);
    result.insert("startMoney",         info.data.startMoney);
    result.insert("firstSmallBlind",    info.data.firstSmallBlind);
    result.insert("raiseIntervalMode",  static_cast<int>(info.data.raiseIntervalMode));
    result.insert("raiseEveryHands",    info.data.raiseSmallBlindEveryHandsValue);
    result.insert("raiseEveryMinutes",  info.data.raiseSmallBlindEveryMinutesValue);
    result.insert("raiseMode",          static_cast<int>(info.data.raiseMode));
    result.insert("playerActionTimeoutSec", info.data.playerActionTimeoutSec);
    result.insert("delayBetweenHandsSec",   info.data.delayBetweenHandsSec);
    result.insert("allowSpectators",    info.data.allowSpectators);
    result.insert("playerCount",        static_cast<int>(info.players.size()));
    result.insert("adminPlayerId",      static_cast<int>(info.adminPlayerId));
    return result;
}

void LobbyHandler::kickPlayer(unsigned playerId)
{
    if (!m_session) {
        emit errorOccurred(tr("Not connected to server"));
        return;
    }
    m_session->kickPlayer(playerId);
}

void LobbyHandler::invitePlayer(unsigned playerId)
{
    if (!m_session) {
        emit errorOccurred(tr("Not connected to server"));
        return;
    }
    m_session->invitePlayerToCurrentGame(playerId);
}

bool LobbyHandler::isPlayerInAnyGame(unsigned playerId) const
{
    if (!m_session || playerId == 0)
        return false;
    const int count = m_gameListModel.rowCount();
    for (int i = 0; i < count; ++i) {
        const unsigned gameId = m_gameListModel.data(
            m_gameListModel.index(i), GameListModel::GameIdRole).toUInt();
        const ::GameInfo gameInfo = m_session->getClientGameInfo(gameId);
        for (const unsigned pid : gameInfo.players) {
            if (pid == playerId)
                return true;
        }
    }
    return false;
}

// ── Eingehende Spiel-Einladungen (Invite-Only-Spiele) ──────────────────────
void LobbyHandler::onSelfGameInvitation(unsigned gameId, unsigned playerIdFrom)
{
    qDebug() << "[INVITE] onSelfGameInvitation: gameId=" << gameId << "fromPlayerId=" << playerIdFrom
             << "pendingInviteGameId=" << m_pendingInviteGameId
             << "ignored=" << isPlayerIgnored(playerIdFrom)
             << "session=" << (m_session ? "ok" : "NULL");
    if (!m_session)
        return;
    // Absender auf der Ignore-Liste ODER es ist bereits ein Einladungs-Popup
    // offen → automatisch mit "busy" ablehnen (wie der Qt-Widgets-Client).
    if (isPlayerIgnored(playerIdFrom) || m_pendingInviteGameId != 0) {
        qDebug() << "[INVITE] → auto-rejecting with BUSY (ignored or popup already open)";
        m_session->rejectGameInvitation(gameId, DENY_GAME_INVITATION_BUSY);
        return;
    }
    m_pendingInviteGameId = gameId;
    const QString gameName = QString::fromStdString(m_session->getClientGameInfo(gameId).name);
    const QString fromName = QString::fromStdString(m_session->getClientPlayerInfo(playerIdFrom).playerName);
    qDebug() << "[INVITE] → emitting gameInvitationReceived: game=" << gameName << "from=" << fromName;
    emit gameInvitationReceived(static_cast<int>(gameId), gameName, fromName);
}

void LobbyHandler::acceptGameInvitation(unsigned gameId)
{
    qDebug() << "[INVITE] acceptGameInvitation: gameId=" << gameId << "pendingWas=" << m_pendingInviteGameId;
    if (m_pendingInviteGameId == gameId)
        m_pendingInviteGameId = 0;
    if (!m_session)
        return;
    m_session->acceptGameInvitation(gameId);
}

void LobbyHandler::rejectGameInvitation(unsigned gameId, int reason)
{
    qDebug() << "[INVITE] rejectGameInvitation: gameId=" << gameId << "reason=" << reason << "pendingWas=" << m_pendingInviteGameId;
    if (m_pendingInviteGameId == gameId)
        m_pendingInviteGameId = 0;
    if (!m_session)
        return;
    const DenyGameInvitationReason deny = (reason == DENY_GAME_INVITATION_BUSY)
        ? DENY_GAME_INVITATION_BUSY : DENY_GAME_INVITATION_NO;
    m_session->rejectGameInvitation(gameId, deny);
}

void LobbyHandler::onPlayerGameInvitation(unsigned gameId, unsigned playerIdWho, unsigned playerIdFrom)
{
    if (!m_session)
        return;
    const QString who  = QString::fromStdString(m_session->getClientPlayerInfo(playerIdWho).playerName).toHtmlEscaped();
    const QString game = QString::fromStdString(m_session->getClientGameInfo(gameId).name).toHtmlEscaped();
    const QString from = QString::fromStdString(m_session->getClientPlayerInfo(playerIdFrom).playerName).toHtmlEscaped();
    const QString ts   = QDateTime::currentDateTime().toString("HH:mm:ss");
    pushChatLine(QStringLiteral("[") + ts + QStringLiteral("] <span style=\"color:#8ab4f8;\">")
                 + tr("%1 has been invited to %2 by %3.").arg(who, game, from)
                 + QStringLiteral("</span>"));
}

void LobbyHandler::onRejectedGameInvitation(unsigned gameId, unsigned playerIdWho, int reason)
{
    if (!m_session)
        return;
    const QString who  = QString::fromStdString(m_session->getClientPlayerInfo(playerIdWho).playerName).toHtmlEscaped();
    const QString game = QString::fromStdString(m_session->getClientGameInfo(gameId).name).toHtmlEscaped();
    const QString msg  = (reason == DENY_GAME_INVITATION_BUSY)
        ? tr("%1 cannot join %2 because he is busy.").arg(who, game)
        : tr("%1 has rejected the invitation to %2.").arg(who, game);
    const QString ts   = QDateTime::currentDateTime().toString("HH:mm:ss");
    pushChatLine(QStringLiteral("[") + ts + QStringLiteral("] <span style=\"color:#e0686d;\">") + msg + QStringLiteral("</span>"));
}

void LobbyHandler::adminBanPlayer(unsigned playerId)
{
    if (!m_session) {
        emit errorOccurred(tr("Not connected to server"));
        return;
    }
    m_session->adminActionBanPlayer(playerId);
}

void LobbyHandler::reportGameName(unsigned gameId)
{
    if (!m_session) {
        emit errorOccurred(tr("Not connected to server"));
        return;
    }
    if (gameId == 0)
        return;
    m_session->reportBadGameName(gameId);
}

void LobbyHandler::adminCloseGame(unsigned gameId)
{
    if (!m_session) {
        emit errorOccurred(tr("Not connected to server"));
        return;
    }
    if (gameId == 0)
        return;
    m_session->adminActionCloseGame(gameId);
}

void LobbyHandler::sendPrivateMessage(unsigned targetPlayerId, const QString &message)
{
    if (!m_session) {
        emit errorOccurred(tr("Not connected to server"));
        return;
    }
    m_session->sendPrivateChatMessage(targetPlayerId, message.toStdString());
}

// ── Player name helper ─────────────────────────────────────────────────────

QString LobbyHandler::resolvedPlayerName(unsigned playerId) const
{
    // Check model first
    const int count = m_playerListModel.rowCount();
    for (int i = 0; i < count; ++i) {
        const QModelIndex idx = m_playerListModel.index(i, 0);
        if (m_playerListModel.data(idx, PlayerListModel::PlayerIdRole).toUInt() == playerId) {
            const QString name = m_playerListModel.data(idx, PlayerListModel::PlayerNameRole).toString();
            if (!name.isEmpty()) return name;
            break;
        }
    }
    // Fall back to session cache
    if (m_session) {
        const QString name = QString::fromStdString(m_session->getClientPlayerInfo(playerId).playerName);
        if (!name.isEmpty()) return name;
    }
    return QString();
}

// ── Ignore list ────────────────────────────────────────────────────────────

bool LobbyHandler::isPlayerIgnored(unsigned playerId) const
{
    if (!m_config || playerId == 0) return false;
    const QString playerName = resolvedPlayerName(playerId);
    if (playerName.isEmpty()) return false;

    const std::list<std::string> ignoreList = m_config->readConfigStringList("PlayerIgnoreList");
    for (const auto &entry : ignoreList) {
        if (playerName == QString::fromUtf8(entry.c_str()))
            return true;
    }
    return false;
}

void LobbyHandler::ignorePlayer(unsigned playerId)
{
    if (!m_config || playerId == 0) return;
    const QString playerName = resolvedPlayerName(playerId);
    if (playerName.isEmpty()) return;

    std::list<std::string> ignoreList = m_config->readConfigStringList("PlayerIgnoreList");
    const std::string nameStd = playerName.toStdString();
    if (std::find(ignoreList.begin(), ignoreList.end(), nameStd) == ignoreList.end()) {
        ignoreList.push_back(nameStd);
        m_config->writeConfigStringList("PlayerIgnoreList", ignoreList);
        ++m_playerIgnoreListRevision;
        emit playerIgnoreListChanged();
    }
}

void LobbyHandler::unignorePlayer(unsigned playerId)
{
    if (!m_config || playerId == 0) return;
    const QString playerName = resolvedPlayerName(playerId);
    if (playerName.isEmpty()) return;

    std::list<std::string> ignoreList = m_config->readConfigStringList("PlayerIgnoreList");
    const std::string nameStd = playerName.toStdString();
    const size_t sizeBefore = ignoreList.size();
    ignoreList.remove(nameStd);
    if (ignoreList.size() != sizeBefore) {
        m_config->writeConfigStringList("PlayerIgnoreList", ignoreList);
        ++m_playerIgnoreListRevision;
        emit playerIgnoreListChanged();
    }
}

// ── Player stats ───────────────────────────────────────────────────────────

void LobbyHandler::showPlayerStats(unsigned playerId)
{
    if (playerId == 0) return;
    const QString playerName = resolvedPlayerName(playerId);
    if (playerName.isEmpty()) return;

    const QString url = QString("https://www.pokerth.net/redirect_user_profile.php?nick=%1")
        .arg(QString::fromUtf8(QUrl::toPercentEncoding(playerName)));
    openExternalUrl(url);
}

QString LobbyHandler::currentTableStatsUrl() const
{
    if (!m_session || m_currentGameId == 0)
        return QString();

    const GameInfo info = m_session->getClientGameInfo(m_currentGameId);
    // Nur Ranglistenspiele haben eine Tisch-Statistikübersicht (wie im
    // Qt-Widgets-Client, MyNameLabel): tableview=1 + Liste der aktiven Nicks.
    if (info.data.gameType != GAME_TYPE_RANKING)
        return QString();

    auto currentGame = m_session->getCurrentGame();
    if (!currentGame)
        return QString();

    QString nickList;
    int playerCounter = 0;
    PlayerList seatsList = currentGame->getSeatsList();
    for (PlayerListConstIterator it_c = seatsList->begin(); it_c != seatsList->end(); ++it_c) {
        if ((*it_c)->getMyActiveStatus()) {
            ++playerCounter;
            nickList += QString("&nick%1=").arg(playerCounter);
            nickList += QString::fromUtf8(QUrl::toPercentEncoding(
                QString::fromUtf8((*it_c)->getMyName().c_str())));
        }
    }

    return QStringLiteral("https://www.pokerth.net/redirect_user_profile.php?tableview=1")
        + nickList
        + QStringLiteral("&table=")
        + QString::fromUtf8(QUrl::toPercentEncoding(QString::fromStdString(info.name)));
}

// ── Domain text helpers ────────────────────────────────────────────────────

QString LobbyHandler::gameTypeText(int gameType) const
{
    switch (gameType) {
    case 2: return tr("Registered players only");
    case 3: return tr("Invited players only");
    case 4: return tr("Ranking game");
    default: return tr("Standard");
    }
}

QString LobbyHandler::gameStatusText(int gameMode, int playerCount, int maxPlayers) const
{
    if (gameMode == 2) return tr("Running");
    if (gameMode == 3) return tr("Closed");
    return playerCount < maxPlayers ? tr("Open") : tr("Full");
}
