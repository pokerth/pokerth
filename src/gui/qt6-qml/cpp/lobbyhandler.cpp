/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2025 Felix Hammer, Florian Thauer, Lothar May          *
 *****************************************************************************/

#include "lobbyhandler.h"
#include "session.h"
#include "configfile.h"
#include <QDebug>

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
    return roles;
}

void PlayerListModel::addPlayer(unsigned playerId, const QString &playerName, bool isAdmin)
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
    m_players.append(player);
    m_playerIndexMap[playerId] = newRow;
    
    endInsertRows();
    
    qDebug() << "Added player:" << playerName << "ID:" << playerId;
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
    
    qDebug() << "Removed player ID:" << playerId;
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

void PlayerListModel::updatePlayerInfo(unsigned playerId, const QString &playerName, bool isAdmin)
{
    if (!m_playerIndexMap.contains(playerId))
        return;
    
    int row = m_playerIndexMap[playerId];
    m_players[row].name = playerName;
    m_players[row].isAdmin = isAdmin;
    
    QModelIndex idx = index(row);
    emit dataChanged(idx, idx);
    qDebug() << "PlayerListModel: Updated player at index" << row << "Name:" << playerName;
}

void PlayerListModel::clear()
{
    beginResetModel();
    m_players.clear();
    m_playerIndexMap.clear();
    endResetModel();
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

    const GameInfo &game = m_games.at(index.row());
    
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
    
    GameInfo game;
    game.id = gameId;
    game.name = gameName.isEmpty() ? QString("Game #%1").arg(gameId) : gameName;
    game.playerCount = 0;
    game.maxPlayers = 10;
    game.gameMode = 0;
    game.isPrivate = false;
    m_games.append(game);
    m_gameIndexMap[gameId] = newRow;
    
    endInsertRows();
    
    qDebug() << "Added game:" << game.name << "ID:" << gameId;
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
    
    qDebug() << "Removed game ID:" << gameId;
}

void GameListModel::updateGameMode(unsigned gameId, int mode)
{
    if (!m_gameIndexMap.contains(gameId))
        return;
    
    int row = m_gameIndexMap[gameId];
    m_games[row].gameMode = mode;
    
    QModelIndex idx = index(row);
    emit dataChanged(idx, idx, {GameModeRole});
}

void GameListModel::clear()
{
    beginResetModel();
    m_games.clear();
    m_gameIndexMap.clear();
    endResetModel();
}

// LobbyHandler implementation
LobbyHandler::LobbyHandler(QObject *parent)
    : QObject(parent)
    , m_session()
    , m_config(nullptr)
    , m_playerListModel(this)
    , m_gameListModel(this)
    , m_myPlayerId(0)
{
}

LobbyHandler::~LobbyHandler()
{
}

void LobbyHandler::setSession(boost::shared_ptr<Session> session)
{
    m_session = session;
}

void LobbyHandler::setConfig(ConfigFile *config)
{
    m_config = config;
}

void LobbyHandler::onLobbyPlayerJoined(unsigned playerId, const QString &playerName)
{
    qDebug() << "LobbyHandler: Player joined -" << playerName << "ID:" << playerId;
    m_playerListModel.addPlayer(playerId, playerName);
}

void LobbyHandler::onLobbyPlayerLeft(unsigned playerId)
{
    qDebug() << "LobbyHandler: Player left - ID:" << playerId;
    m_playerListModel.removePlayer(playerId);
}

void LobbyHandler::updatePlayerName(unsigned playerId, const QString &playerName, bool isAdmin)
{
    qDebug() << "LobbyHandler: Update player name -" << playerName << "ID:" << playerId << "Admin:" << isAdmin;
    
    // Update in player list model
    m_playerListModel.updatePlayerInfo(playerId, playerName, isAdmin);
    
    // Check if this is our own player by comparing with session's unique player ID
    if (m_session) {
        unsigned myId = m_session->getClientUniquePlayerId();
        qDebug() << "Checking if this is me: playerId =" << playerId << ", myId =" << myId;
        if (playerId == myId) {
            qDebug() << "This is me! Setting my player info to:" << playerName;
            setMyPlayerInfo(playerId, playerName);
        }
    }
}

void LobbyHandler::onGameListNew(unsigned gameId, const QString &gameName)
{
    qDebug() << "LobbyHandler: New game - ID:" << gameId << "Name:" << gameName;
    m_gameListModel.addGame(gameId, gameName.isEmpty() ? QString("Game #%1").arg(gameId) : gameName);
}

void LobbyHandler::onGameListRemove(unsigned gameId)
{
    qDebug() << "LobbyHandler: Game removed - ID:" << gameId;
    m_gameListModel.removeGame(gameId);
}

void LobbyHandler::onGameListUpdateMode(unsigned gameId, int mode)
{
    qDebug() << "LobbyHandler: Game mode updated - ID:" << gameId << "Mode:" << mode;
    m_gameListModel.updateGameMode(gameId, mode);
}

void LobbyHandler::setMyPlayerInfo(unsigned playerId, const QString &playerName)
{
    qDebug() << "LobbyHandler::setMyPlayerInfo called - ID:" << playerId << "Name:" << playerName;
    qDebug() << "Current myPlayerId:" << m_myPlayerId << "Current myPlayerName:" << m_myPlayerName;
    
    if (m_myPlayerId != playerId) {
        m_myPlayerId = playerId;
        emit myPlayerIdChanged();
    }
    
    if (m_myPlayerName != playerName) {
        m_myPlayerName = playerName;
        emit myPlayerNameChanged();
        qDebug() << "Player name changed to:" << m_myPlayerName;
    }
    
    qDebug() << "LobbyHandler: My player info set -" << playerName << "ID:" << playerId;
}

void LobbyHandler::sendChatMessage(const QString &message)
{
    if (!m_session || message.trimmed().isEmpty())
        return;
    
    qDebug() << "LobbyHandler: Sending chat message:" << message;
    
    try {
        m_session->sendLobbyChatMessage(message.toStdString());
    } catch (const std::exception &e) {
        qWarning() << "Failed to send chat message:" << e.what();
        emit errorOccurred(tr("Failed to send chat message"));
    }
}

void LobbyHandler::onLobbyChatMessage(const QString &playerName, const QString &message)
{
    qDebug() << "LobbyHandler: Chat message from" << playerName << ":" << message;
    emit chatMessageReceived(playerName, message);
}

void LobbyHandler::createGame()
{
    if (!m_session) {
        emit errorOccurred(tr("Not connected to server"));
        return;
    }
    
    qDebug() << "LobbyHandler: Creating game...";
    // TODO: Implement game creation via session
}

void LobbyHandler::joinGame(unsigned gameId)
{
    if (!m_session) {
        emit errorOccurred(tr("Not connected to server"));
        return;
    }
    
    qDebug() << "LobbyHandler: Joining game ID:" << gameId;
    // TODO: Implement game join via session
    emit gameJoined(gameId);
}

void LobbyHandler::leaveGame()
{
    if (!m_session)
        return;
    
    qDebug() << "LobbyHandler: Leaving game...";
    // TODO: Implement game leave via session
}
