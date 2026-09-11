/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2025 Felix Hammer, Florian Thauer, Lothar May          *
 *****************************************************************************/

#include "lobbyhandler.h"
#include "androidconnectionservice.h"
#include "iosbackgroundsession.h"
#include "chattranslator.h"
#include "texttranslator.h"
#include "chatemotes.h"
#include "chatcolors.h"
#include "darkmode.h"
#include "gui/chat_emote_shortcuts.h"
#include "session.h"
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
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <algorithm>

// How many messages of a conversation are kept in memory/the dialog. The
// SQLite file keeps everything, the dialog shows the most recent window.
static const int kPrivateMessagesLoaded = 500;


class PlayerNickListSortFilterProxyModel : public QSortFilterProxyModel
{
public:
	explicit PlayerNickListSortFilterProxyModel(QObject *parent = nullptr)
		: QSortFilterProxyModel(parent)
		, m_filterState(0)
		, m_lastFilterStateCountry(false)
		, m_lastFilterStateAlpha(true)
	{
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

#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
		beginFilterChange();
#endif
		m_filterState = state;
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
		endFilterChange(QSortFilterProxyModel::Direction::Rows);
#else
		invalidateFilter();
#endif
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
			QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
			if (!idx.isValid())
				return false;
			// idle = at no table. The membership is a role in the
			// source model (LobbyHandler::syncPlayerGameMembership), not in
			// the session: if it changes, the model reports dataChanged and
			// the proxy re-evaluates the row by itself.
			return sourceModel()->data(idx, PlayerListModel::GameIdRole).toUInt() == 0;
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
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
		beginFilterChange();
#endif
		m_session = session;
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
		endFilterChange(QSortFilterProxyModel::Direction::Rows);
#else
		invalidateFilter();
#endif
	}

	void setFilterMode(int mode)
	{
		if (mode < 0 || mode > 5)
			mode = 0;

		if (m_filterMode == mode)
			return;

#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
		beginFilterChange();
#endif
		m_filterMode = mode;
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
		endFilterChange(QSortFilterProxyModel::Direction::Rows);
#else
		invalidateFilter();
#endif
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
	case GameIdRole:
		return player.gameId;
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
	roles[GameIdRole] = "gameId";
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
	// New players start as idle; LobbyHandler::syncPlayerGameMembership
	// catches up with the actual state immediately afterwards.
	player.gameId = 0;
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

void PlayerListModel::setPlayerGameId(unsigned playerId, unsigned gameId)
{
	if (!m_playerIndexMap.contains(playerId))
		return;

	int row = m_playerIndexMap[playerId];
	if (m_players[row].gameId == gameId)
		return;

	m_players[row].gameId = gameId;

	QModelIndex idx = index(row);
	emit dataChanged(idx, idx, {GameIdRole});
}

QList<unsigned> PlayerListModel::playerIds() const
{
	QList<unsigned> ids;
	ids.reserve(m_players.count());
	for (const PlayerInfo &player : m_players)
		ids.append(player.id);
	return ids;
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

	// The chat translator operates directly on m_chatLog; every line it changes
	// triggers (in a bundle) chatLogChanged(), so that the QML binding renders anew.
	m_chatTranslator = new ChatTranslator(&m_chatLog, this);
	connect(m_chatTranslator, &ChatTranslator::chatLogMutated,
			this, &LobbyHandler::notifyChatLogChanged);

	// With DarkMode = "automatic" the chat colours hang off the system theme:
	// if it changes during operation, the history has to be delivered
	// anew (the colour placeholders are only resolved in chatLog()).
	if (QStyleHints *hints = QGuiApplication::styleHints()) {
		connect(hints, &QStyleHints::colorSchemeChanged,
				this, &LobbyHandler::notifyChatLogChanged);
	}
}

QObject* LobbyHandler::chatTranslator() const
{
	return m_chatTranslator;
}

LobbyHandler::~LobbyHandler() = default;

void LobbyHandler::setSoundEvents(SoundEvents *soundEvents)
{
	m_soundEvents = soundEvents;
}

void LobbyHandler::setSession(boost::shared_ptr<Session> session)
{
	m_session = session;

	// Always reset lobby models on session assignment to avoid stale rows
	// when reconnect/resubscribe happens without pointer change.
	m_gameListModel.clear();
	m_playerListModel.clear();

	// The lobby chat belongs to the connection, whereas the LobbyHandler lives as
	// long as the app: without this clearing, the history of the previous session
	// would still stand in the chat box after another login. The counterpart to the
	// widgets client (clearDialog() → ChatTools::clearChat() before
	// startInternetClient()). setSession() is called for every new
	// client (SignalNetClientConnect, MSG_SOCK_INIT_DONE).
	if (m_chatTranslator)
		m_chatTranslator->reset();
	if (!m_chatLog.isEmpty()) {
		m_chatLog.clear();
		notifyChatLogChanged();
	}
	// The PM history outlives sessions (privatemessages.sqlite), but belongs
	// to exactly one account: until the next login no owner is known,
	// so the inbox stays empty. setMyPlayerInfo() then loads it for the
	// nick one has logged in with this time.
	setPrivateMessageOwner(QString());

	// A rejoin offer that may still be open belongs to the old connection;
	// a new one comes (if possible) with the InitAck of the new connection.
	if (m_rejoinOfferGameId != 0) {
		m_rejoinOfferGameId = 0;
		emit rejoinOfferChanged();
	}
	setRejoinWaiting(false);

	// Reset the game context: after a connection loss during a game no
	// onRemovedFromGame arrives any more - without the reset isInGame/currentGameId
	// would stay across the reconnect.
	setGameRunning(false);
	if (m_isInGame) {
		m_isInGame = false;
		m_currentGameId = 0;
		emit isInGameChanged();
		emit currentGameIdChanged();
	}
	setCurrentGameAdmin(false);

	static_cast<GameListSortFilterProxyModel *>(m_gameListProxyModel)->setSession(m_session.get());
	++m_playerListRevision;
	emit playerListRevisionChanged();
	++m_gameListRevision;
	emit gameListRevisionChanged();
}

void LobbyHandler::setConfig(ConfigFile *config)
{
	m_config = config;

	if (m_chatTranslator)
		m_chatTranslator->setConfig(config);

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

	// The inbox is NOT read here: which history applies is only settled
	// at the login (see setPrivateMessageOwner). Only create / upgrade the file
	// whose path is known now.
	openPrivateMessageDb();
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
	// The newcomer may already sit at a table (e.g. a rejoin or the
	// player list sent again after a resubscribe).
	syncPlayerGameMembership();
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
	++m_playerListRevision;
	emit playerListRevisionChanged();
	++m_gameListRevision;
	emit gameListRevisionChanged();
}

void LobbyHandler::updatePlayerName(unsigned playerId, const QString &playerName, bool isAdmin)
{
	// Do NOT use the isAdmin parameter that is passed through for the admin status:
	// it carries a different meaning depending on the caller (the server admin from
	// SignalNetClientPlayerChanged vs. the game admin from SignalNetClientPlayerJoined).
	// What counts for kickban / closing a game is solely the server admin from
	// the PlayerInfo of the session. That one is authoritative once the PlayerInfoReply
	// has arrived – so the status heals itself and is no longer
	// overwritten by a game join.
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
	syncPlayerGameMembership();
	++m_playerListRevision;
	emit playerListRevisionChanged();
	++m_gameListRevision;
	emit gameListRevisionChanged();

	// Check if this is our own player by comparing with session's unique player ID
	if (m_session) {
		unsigned myId = m_session->getClientUniquePlayerId();
		if (playerId == myId) {
			setMyPlayerInfo(playerId, playerName);
			// Update the server admin status (self-healing from the session).
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
	syncPlayerGameMembership();
	emit gameContextChanged();
}

void LobbyHandler::onGameListRemove(unsigned gameId)
{
	m_gameListModel.removeGame(gameId);
	++m_gameListRevision;
	emit gameListRevisionChanged();
	syncPlayerGameMembership();
	emit gameContextChanged();
}

void LobbyHandler::onGameListUpdateMode(unsigned gameId, int mode)
{
	m_gameListModel.updateGameMode(gameId, mode);
	refreshGameInfo(gameId);
	++m_gameListRevision;
	emit gameListRevisionChanged();
	syncPlayerGameMembership();
	emit gameContextChanged();
}

void LobbyHandler::onGameListChanged(unsigned gameId)
{
	refreshGameInfo(gameId);
	++m_gameListRevision;
	emit gameListRevisionChanged();
	// A player or a spectator has joined / left a game
	// (SignalNetClientGameListPlayerJoined/Left and the spectator counterparts map
	// onto this). Exactly here somebody leaves the idle list or returns into it.
	syncPlayerGameMembership();
	++m_playerListRevision;
	emit playerListRevisionChanged();
	emit gameContextChanged();
}

void LobbyHandler::setCurrentGameAdmin(bool isGameAdmin)
{
	// It concerns exclusively the game admin (host) – the server admin status
	// (kickban / closing a game) stays untouched by it.
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
		// The first point after the login at which your own nick is settled: from
		// here on the inbox of this account applies.
		setPrivateMessageOwner(playerName);
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

// The index of a PM bubble via its unique msgId (-1 = no longer in the
// memory window of the history).
static int indexOfPrivateMessage(const QVariantList &messages, int messageId)
{
	for (int i = 0; i < messages.size(); ++i) {
		if (messages.at(i).toMap().value(QStringLiteral("msgId")).toInt() == messageId)
			return i;
	}
	return -1;
}

void LobbyHandler::setTextTranslator(TextTranslator *translator)
{
	m_textTranslator = translator;
	if (m_textTranslator) {
		connect(m_textTranslator, &TextTranslator::translated,
				this, &LobbyHandler::onPrivateMessageTranslated, Qt::UniqueConnection);
	}
}

void LobbyHandler::togglePrivateMessageTranslation(const QString &playerName, int messageId)
{
	auto it = m_privateThreads.find(playerName);
	if (it == m_privateThreads.end())
		return;
	const int index = indexOfPrivateMessage(it->messages, messageId);
	if (index < 0)
		return;

	QVariantMap entry = it->messages.at(index).toMap();
	// Incoming messages only – your own are already written in your own
	// language (the same rule as in the chat history).
	if (entry.value(QStringLiteral("fromMe")).toBool()
			|| entry.value(QStringLiteral("translationPending")).toBool())
		return;

	// Already translated: only show/hide it, no renewed network fetch.
	if (!entry.value(QStringLiteral("translation")).toString().isEmpty()) {
		entry.insert(QStringLiteral("showTranslation"),
					 !entry.value(QStringLiteral("showTranslation")).toBool());
		it->messages[index] = entry;
		++m_privateMessagesRevision;
		emit privateMessagesChanged();
		return;
	}

	if (!m_textTranslator)
		return;
	const int requestId =
		m_textTranslator->translate(entry.value(QStringLiteral("text")).toString());
	if (requestId < 0)
		return;

	entry.insert(QStringLiteral("translationPending"), true);
	entry.insert(QStringLiteral("translationFailed"), false);
	it->messages[index] = entry;
	m_pmTranslationRequests.insert(requestId, qMakePair(playerName, messageId));

	++m_privateMessagesRevision;
	emit privateMessagesChanged();
}

void LobbyHandler::onPrivateMessageTranslated(int requestId, const QString &text, bool ok)
{
	if (!m_pmTranslationRequests.contains(requestId))
		return;   // A request of another view (the forum page).
	const QPair<QString, int> req = m_pmTranslationRequests.take(requestId);

	auto it = m_privateThreads.find(req.first);
	if (it == m_privateThreads.end())
		return;
	const int index = indexOfPrivateMessage(it->messages, req.second);
	if (index < 0)
		return;   // The message has fallen out of the memory window meanwhile.

	QVariantMap entry = it->messages.at(index).toMap();
	entry.insert(QStringLiteral("translationPending"), false);
	if (ok && !text.trimmed().isEmpty()) {
		entry.insert(QStringLiteral("translation"), text);
		entry.insert(QStringLiteral("showTranslation"), true);
		entry.insert(QStringLiteral("translationFailed"), false);
	} else {
		// Do NOT swallow a failure: the globe changes colour, another
		// click tries again.
		entry.insert(QStringLiteral("translationFailed"), true);
	}
	it->messages[index] = entry;

	++m_privateMessagesRevision;
	emit privateMessagesChanged();
}

bool LobbyHandler::isPlayerGuest(unsigned playerId) const
{
	if (!m_session || playerId == 0)
		return false;

	const PlayerInfo info = m_session->getClientPlayerInfo(playerId);
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

	// Password protected games can be joined: the LobbyPage asks for the
	// password before joining (joinPasswordPopup) and passes it to
	// joinGame(); it is checked on the server side (ServerGame::CheckPassword).

	const int gameType = static_cast<int>(info.data.gameType);

	// Invitation games are entered exclusively via the invitation itself
	// (acceptGameInvitation), never via the game list.
	if (gameType == GAME_TYPE_INVITE_ONLY)
		return false;

	// On the server side guests may only join normal games
	// (ServerLobbyThread::HandleNetPacketJoinGame).
	if (gameType != GAME_TYPE_NORMAL && isMyPlayerGuest())
		return false;

	return gameType == GAME_TYPE_NORMAL
		   || gameType == GAME_TYPE_REGISTERED_ONLY
		   || gameType == GAME_TYPE_RANKING;
}

bool LobbyHandler::canSpectateGame(unsigned gameId) const
{
	if (!m_session || gameId == 0)
		return false;

	// Only one table at a time: whoever already sits or spectates has to get out first.
	if (m_isInGame)
		return false;

	const GameInfo info = m_session->getClientGameInfo(gameId);

	// Running games only. A game in the waiting room has no table to
	// show yet; a closed one is over.
	if (static_cast<int>(info.mode) != GAME_MODE_STARTED)
		return false;

	// The only condition of the server (ServerLobbyThread::HandleNetPacketJoinGame):
	// with spectateOnly it checks NEITHER the password, the invitation nor the guest status.
	return info.data.allowSpectators;
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

// The only place where the idle criterion is maintained: from the known
// games the table of every player in the lobby list is derived and written into the
// model as the GameIdRole. Spectators count like fellow players as
// "at a table" (the widget client marks them as "active" in the same way).
// It only issues real changes, so that the call stays cheap on every lobby
// event; the model reports every change as dataChanged, whereupon the
// proxy filters the affected row anew by itself.
void LobbyHandler::syncPlayerGameMembership()
{
	if (!m_session)
		return;

	QHash<unsigned, unsigned> gameIdOfPlayer;
	const int gameCount = m_gameListModel.rowCount();
	for (int row = 0; row < gameCount; ++row) {
		const unsigned gameId = m_gameListModel.data(
									m_gameListModel.index(row), GameListModel::GameIdRole).toUInt();
		if (gameId == 0)
			continue;

		const ::GameInfo info = m_session->getClientGameInfo(gameId);
		for (const unsigned playerId : info.spectators)
			gameIdOfPlayer.insert(playerId, gameId);
		// After the spectators, so that a seat overwrites a possibly stale
		// spectator assignment.
		for (const unsigned playerId : info.players)
			gameIdOfPlayer.insert(playerId, gameId);
	}

	const QList<unsigned> playerIds = m_playerListModel.playerIds();
	for (const unsigned playerId : playerIds)
		m_playerListModel.setPlayerGameId(playerId, gameIdOfPlayer.value(playerId, 0));
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

	// Walk through the source model (unfiltered), NOT the proxy: the
	// player list filter (mode 2) hides players in games, which
	// have to stay reachable for the chat completion though.
	static const QRegularExpression numericPlaceholderPattern("^#?\\d+$");
	const int count = m_playerListModel.rowCount();
	for (int row = 0; row < count; ++row) {
		const QModelIndex index = m_playerListModel.index(row, 0);
		if (!index.isValid())
			continue;

		const unsigned playerId = m_playerListModel.data(index, PlayerListModel::PlayerIdRole).toUInt();
		QString playerName = m_playerListModel.data(index, PlayerListModel::PlayerNameRole).toString();

		// Resolve placeholder names (e.g. "#123") via the session.
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

QStringList LobbyHandler::idlePlayerNames() const
{
	QStringList names;
	if (!m_session)
		return names;

	static const QRegularExpression numericPlaceholderPattern("^#?\\d+$");
	const int count = m_playerListModel.rowCount();
	for (int row = 0; row < count; ++row) {
		const QModelIndex index = m_playerListModel.index(row, 0);
		if (!index.isValid())
			continue;

		const unsigned playerId = m_playerListModel.data(index, PlayerListModel::PlayerIdRole).toUInt();
		if (playerId == 0)
			continue;
		// Guests are neither in the BBC database nor on the WEC list.
		if (m_playerListModel.data(index, PlayerListModel::IsGuestRole).toBool())
			continue;
		// idle = at no table – the same source as the idle filter (mode 2),
		// so that the suggestion and the visible list never drift apart.
		if (m_playerListModel.data(index, PlayerListModel::GameIdRole).toUInt() != 0)
			continue;

		QString playerName = m_playerListModel.data(index, PlayerListModel::PlayerNameRole).toString();
		// Resolve placeholder names (e.g. "#123") via the session.
		const bool nameIsPlaceholder = playerName.isEmpty()
									   || numericPlaceholderPattern.match(playerName).hasMatch();
		if (nameIsPlaceholder) {
			const QString sessionName = QString::fromStdString(m_session->getClientPlayerInfo(playerId).playerName);
			if (!sessionName.isEmpty())
				playerName = sessionName;
		}

		if (!playerName.isEmpty() && !names.contains(playerName))
			names << playerName;
	}

	return names;
}

QVariantList LobbyHandler::playingPlayerEntries() const
{
	QVariantList entries;
	if (!m_session)
		return entries;

	// Do not suggest players at your own table – they already sit there.
	const unsigned ownGameId = m_session->getClientCurrentGameId();

	static const QRegularExpression numericPlaceholderPattern("^#?\\d+$");
	const int count = m_playerListModel.rowCount();
	for (int row = 0; row < count; ++row) {
		const QModelIndex index = m_playerListModel.index(row, 0);
		if (!index.isValid())
			continue;

		const unsigned playerId = m_playerListModel.data(index, PlayerListModel::PlayerIdRole).toUInt();
		if (playerId == 0)
			continue;
		if (m_playerListModel.data(index, PlayerListModel::IsGuestRole).toBool())
			continue;
		// Only players who currently sit at a table (the counterpart to the idle filter).
		const unsigned gameId = m_playerListModel.data(index, PlayerListModel::GameIdRole).toUInt();
		if (gameId == 0)
			continue;
		// ... but not those at your own table.
		if (ownGameId != 0 && gameId == ownGameId)
			continue;

		QString playerName = m_playerListModel.data(index, PlayerListModel::PlayerNameRole).toString();
		const bool nameIsPlaceholder = playerName.isEmpty()
									   || numericPlaceholderPattern.match(playerName).hasMatch();
		if (nameIsPlaceholder) {
			const QString sessionName = QString::fromStdString(m_session->getClientPlayerInfo(playerId).playerName);
			if (!sessionName.isEmpty())
				playerName = sessionName;
		}
		if (playerName.isEmpty())
			continue;

		QVariantMap entry;
		entry.insert("name", playerName);
		entry.insert("game", QString::fromUtf8(m_session->getClientGameInfo(gameId).name.c_str()));
		entries.append(entry);
	}
	return entries;
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
		// The table admin (creator/host of this game) – strictly separate from the
		// server admin above. The ClientThread keeps adminPlayerId up to date
		// (UpdateGameInfoAdmin), the QML lists are re-evaluated via
		// gameListRevision.
		entry.insert("isGameAdmin", gameInfo.adminPlayerId != 0 && playerId == gameInfo.adminPlayerId);
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

QVariantList LobbyHandler::chatEmoteShortcodes() const
{
	// Built once (the map is static); sorted alphabetically, so that
	// the suggestion list of the ChatBox is stable and predictable.
	static const QVariantList list = [] {
		const QHash<QString, QString> &m = chatEmoteShortcodeMap();
		QStringList codes = m.keys();
		codes.sort();
		QVariantList l;
		l.reserve(codes.size());
		for (const QString &code : codes)
		{
			QVariantMap entry;
			entry.insert(QStringLiteral("code"), code);
			entry.insert(QStringLiteral("emoji"), m.value(code));
			l << entry;
		}
		return l;
	}();
	return list;
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
		if (text.startsWith(QLatin1String("/gn "), Qt::CaseInsensitive)) {
			// A server-wide announcement as a chat shortcut (the same path as the
			// announcement button). Only the server decides about the rights;
			// the local admin status only controls the visibility of the button.
			// If it were additionally checked locally here, the announcement of an
			// admin whose PlayerInfo has not arrived yet would accidentally end up
			// as normal chat text in the lobby.
			adminSendGlobalNotice(text.mid(4));
		} else if (text.startsWith(QLatin1String("/msg "), Qt::CaseInsensitive)) {
			// Private message: /msg <nick> <text>  or  /msg "<nick with spaces>" <text>
			// Blocked at a running table – like the PM symbol of the player list.
			if (m_gameRunning) {
				emit errorOccurred(tr("Private messages are not available at the table."));
				return;
			}
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
			const QString targetName = resolvedPlayerName(targetId);
			pushPrivateMessageSentLine(targetName, text);
			appendPrivateMessage(targetName, text, true);
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

void LobbyHandler::postLocalChatNote(const QString &message)
{
	if (message.trimmed().isEmpty())
		return;

	// Local display only: the same timestamp/colour/emote preparation as a
	// normal chat entry, but in muted italics (like PMs) as a hint that
	// only the triggering user sees the line and that it is NOT sent.
	//
	// Multi-line notices (plain text with "\n", e.g. the community suggestion
	// with one player per line) become SEVERAL chatLog entries – NOT
	// one entry with an embedded <br>. The history is one
	// single rich text document, but the translate symbols determine the line
	// under the mouse cursor via its index in chatLog
	// (ChatBox._updateHoverLine → ChatTranslator::setHoveredLine). An entry
	// with a line break of its own would shift this mapping for the whole rest of the
	// history: the globe would appear on a different line from the one the mouse passed over.
	const QStringList parts = message.split(QLatin1Char('\n'));
	const QString tsPrefix = chatTimestampPrefix(m_config);
	bool first = true;
	for (const QString &part : parts) {
		if (part.trimmed().isEmpty())
			continue;
		QString escapedMsg = ChatColors::chatEscape(part);
		escapedMsg = applyChatEmoteShortcuts(escapedMsg);
		escapedMsg = enlargeEmojis(escapedMsg);

		// A timestamp only on the first line – the following lines belong
		// visibly to the same notice.
		const QString line = (first ? tsPrefix : QString())
							 + QLatin1String("<i><span style=\"")
							 + ChatColors::colorStyle(ChatColors::Muted)
							 + QLatin1String(";\">") + escapedMsg
							 + QLatin1String("</span></i>");
		pushChatLine(line);
		first = false;
	}
}

void LobbyHandler::onGamePlayerJoined()
{
	// A notification sound as in the widgets client (gamelobbydialogimpl /
	// startnetworkgamedialogimpl): while the game is not full
	// "playerconnected", for the last player "onlinegameready" (the game
	// starts right afterwards).
	// Only in the waiting room, not in a running game: there PlayerJoined means
	// a rejoin after a disconnect, and "onlinegameready" (the game is full again)
	// would sound like a game start in the middle of the hand.
	if (m_gameRunning)
		return;
	// As a spectator NEVER: when we join, the server reports every player who is
	// already sitting individually as PlayerJoined (AcceptNewSession) – that would give
	// a salvo of join sounds for a game that has long been running.
	if (m_isSpectating)
		return;
	if (m_config && !m_config->readConfigInt("PlayNetworkGameNotification"))
		return;
	if (!m_session || m_currentGameId == 0)
		return;
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
	// An audio notice: the popup can be overlooked (in the lobby as well as in-game).
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
	// The texts 1:1 as in startWindowImpl::networkMessage(unsigned).
	QString msgText;
	switch (msgId) {
	case MSG_NET_AVATAR_REPORT_ACCEPTED:
		msgText = tr("The avatar report was accepted by the server. Thank you.");
		break;
	case MSG_NET_AVATAR_REPORT_DUP:
		msgText = tr("This avatar was already reported by another player.");
		break;
	case MSG_NET_AVATAR_REPORT_REJECTED:
		msgText = tr("An error occurred while reporting the avatar.");
		break;
	case MSG_NET_GAMENAME_REPORT_ACCEPTED:
		msgText = tr("The game name report was accepted by the server. Thank you.");
		break;
	case MSG_NET_GAMENAME_REPORT_DUP:
		msgText = tr("This game name was already reported by another player.");
		break;
	case MSG_NET_GAMENAME_REPORT_REJECTED:
		msgText = tr("An error occurred while reporting the game name.");
		break;
	case MSG_NET_ADMIN_REMOVE_GAME_ACCEPTED:
		msgText = tr("The game was closed.");
		break;
	case MSG_NET_ADMIN_REMOVE_GAME_REJECTED:
		msgText = tr("The game could not be closed.");
		break;
	case MSG_NET_ADMIN_BAN_PLAYER_ACCEPTED:
		msgText = tr("The player was kicked and banned permanently.");
		break;
	case MSG_NET_ADMIN_BAN_PLAYER_NODB:
		msgText = tr("The player was kicked, but could not be banned because it was a guest player.");
		break;
	case MSG_NET_ADMIN_BAN_PLAYER_DBERROR:
		msgText = tr("The player was kicked, but could not be banned, \nbecause the nick could not be found in the database");
		break;
	case MSG_NET_ADMIN_BAN_PLAYER_REJECTED:
		msgText = tr("The player could not be found.");
		break;
	case MSG_NET_ADMIN_GLOBAL_NOTICE_ACCEPTED:
		msgText = tr("The global notice was sent to all players.");
		break;
	case MSG_NET_ADMIN_GLOBAL_NOTICE_REJECTED:
		msgText = tr("The global notice was rejected by the server.");
		break;
	default:
		return;   // do not show unknown IDs (as in the widgets client)
	}
	emit networkMessageReceived(msgText);
}

void LobbyHandler::onNetworkNotification(int notificationId)
{
	// The texts 1:1 as in startWindowImpl::networkNotification(int). Without this
	// handling, every server rejection when creating/joining a game (e.g. a
	// game name that is already taken) went unnoticed.
	QString msgText;
	switch (notificationId) {
	case NTF_NET_JOIN_IP_BLOCKED:
		msgText = tr("You cannot join this game, because another player in that game has your network address.");
		break;
	case NTF_NET_REMOVED_GAME_FULL:
	case NTF_NET_JOIN_GAME_FULL:
		msgText = tr("Sorry, this game is already full.");
		break;
	case NTF_NET_REMOVED_ALREADY_RUNNING:
	case NTF_NET_JOIN_ALREADY_RUNNING:
		msgText = tr("Unable to join - the server has already started the game.");
		break;
	case NTF_NET_JOIN_NOT_INVITED:
		msgText = tr("This game is of type invite-only. You cannot join this game without being invited.");
		break;
	case NTF_NET_JOIN_GAME_NAME_IN_USE:
		msgText = tr("This game name is already in use. Please choose a different name.");
		break;
	case NTF_NET_JOIN_GAME_BAD_NAME:
		msgText = tr("The game name is invalid. Please choose a different name.");
		break;
	case NTF_NET_JOIN_INVALID_PASSWORD:
		msgText = tr("Invalid password when joining the game.\nPlease reenter the password and try again.");
		break;
	case NTF_NET_JOIN_GUEST_FORBIDDEN:
		msgText = tr("You cannot join this type of game as guest.");
		break;
	case NTF_NET_JOIN_INVALID_SETTINGS:
		msgText = tr("The settings are invalid for this type of game.");
		break;
	case NTF_NET_JOIN_NO_SPECTATORS:
		msgText = tr("This game does not allow spectators.");
		break;
	case NTF_NET_JOIN_GAME_INVALID:
	case NTF_NET_JOIN_REJOIN_FAILED:
		msgText = tr("Could not join the game.");
		break;
	case NTF_NET_REMOVED_START_FAILED:
		// The start synchronization (on a rejoin as well) took too long.
		msgText = tr("Your connection to the server is very slow, the game had to start without you.");
		break;
	case NTF_NET_REMOVED_KICKED:
		msgText = tr("You were kicked from the game.");
		break;
	case NTF_NET_REMOVED_TIMEOUT:
		// The AFK kick of the server. The countdown warning that preceded it is closed by the
		// onRemovedFromGame handler in pokerth.qml.
		msgText = tr("You were removed due to inactivity.");
		break;
	default:
		return;   // do not show unknown IDs (as in the widgets client)
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

	// Detect /me action before escaping
	const bool isAction = message.startsWith(QLatin1String("/me "));
	const QString rawDisplay = isAction ? message.mid(4) : message;

	// HTML-escape user-supplied content (prevents tag injection)
	QString escapedMsg = ChatColors::chatEscape(rawDisplay);
	// Convert the ASCII shortcuts on the raw text before link/style markup
	// is added (it prevents collisions with "color:#..." and the like).
	escapedMsg = applyChatEmoteShortcuts(escapedMsg);

	// URL linkification
	static const QRegularExpression urlRe(QLatin1String("(https?://\\S+)"));
	escapedMsg.replace(urlRe, QLatin1String("<a href=\"\\1\">\\1</a>"));

	// Determine message style based on content
	bool isMention = false;
	QString styledMsg;

	if (isChatBot && !myNick.isEmpty() && rawDisplay.startsWith(myNick)) {
		// Chatbot addressing me: bold red
		styledMsg = QLatin1String("<span style=\"font-weight:bold; ")
					+ ChatColors::colorStyle(ChatColors::Danger)
					+ QLatin1String(";\">") + escapedMsg + QLatin1String("</span>");
	} else if (!myNick.isEmpty() && rawDisplay.contains(myNick, Qt::CaseInsensitive)) {
		// Mention: bold accent
		isMention = true;
		styledMsg = QLatin1String("<span style=\"font-weight:bold; ")
					+ ChatColors::colorStyle(ChatColors::Accent)
					+ QLatin1String(";\">") + escapedMsg + QLatin1String("</span>");
	} else {
		// All other messages (including own): normal text colour
		styledMsg = QLatin1String("<span style=\"font-weight:normal; ")
					+ ChatColors::colorStyle(ChatColors::Text)
					+ QLatin1String(";\">") + escapedMsg + QLatin1String("</span>");
	}

	// Display Unicode emojis larger (as in the game chat, ~22px).
	styledMsg = enlargeEmojis(styledMsg);

	// Sound notification on mention (wie chattools.cpp im Widgets-Client)
	if (isMention && playerName != myNick) {
		if (!m_config || m_config->readConfigInt("PlayLobbyChatNotification")) {
			if (m_soundEvents)
				m_soundEvents->playSound("lobbychatnotify", 0);
			emit lobbyChatMentionDetected();
		}
	}

	// Build final line
	const QString tsPrefix    = chatTimestampPrefix(m_config);
	const QString escapedName = ChatColors::chatEscape(playerName);
	QString line;
	if (isAction) {
		line = tsPrefix + QLatin1String("<i>*")
			   + escapedName + QLatin1String(" ") + styledMsg + QLatin1String("*</i>");
	} else {
		line = tsPrefix + QLatin1String("<b>")
			   + escapedName + QLatin1String(":</b> ") + styledMsg;
	}

	// The translate symbol only on messages of others (you do not have to
	// translate your own). rawDisplay is the source text without HTML/style markup; styledMsg
	// is the message body in the line, which is replaced when it is shown.
	if (m_chatTranslator && playerName != myNick)
		line = m_chatTranslator->decorate(line, rawDisplay, styledMsg);

	pushChatLine(line);
}

void LobbyHandler::onPrivateChatMessage(const QString &playerName, const QString &message)
{
	// Discard PMs of ignored players — the widgets client filters them via
	// the same ignore loop in ChatTools::receiveMessage (pm=true).
	if (m_config) {
		const std::list<std::string> ignoreList = m_config->readConfigStringList("PlayerIgnoreList");
		for (const auto &entry : ignoreList) {
			if (playerName == QString::fromUtf8(entry.c_str()))
				return;
		}
	}

	// Colour for PMs: muted text (similar to chattools.cpp italic PM style)
	QString escapedMsg  = ChatColors::chatEscape(message);
	escapedMsg = applyChatEmoteShortcuts(escapedMsg);
	escapedMsg = enlargeEmojis(escapedMsg);

	const QString tsPrefix = chatTimestampPrefix(m_config);
	QString line       = tsPrefix + QLatin1String("<i><span style=\"")
						 + ChatColors::colorStyle(ChatColors::Muted)
						 + QLatin1String(";\">")
						 + ChatColors::chatEscape(playerName)
						 + QLatin1String("(pm): ") + escapedMsg
						 + QLatin1String("</span></i>");
	// Incoming private messages are always from others -> translatable.
	// escapedMsg is the message body in the line.
	if (m_chatTranslator)
		line = m_chatTranslator->decorate(line, message, escapedMsg);
	pushChatLine(line);

	// A PM is always directed at you personally and would otherwise be lost in the running
	// lobby chat: ALWAYS a sound (only the global switch
	// "PlaySoundEffects" in the audio player decides), plus the unread
	// counter at the chat header. Unlike with a nick match it can NOT be switched off via
	// "PlayLobbyChatNotification".
	if (m_soundEvents)
		m_soundEvents->playSound("lobbychatnotify", 0);
	appendPrivateMessage(playerName, message, false);
}

void LobbyHandler::setGameRunning(bool running)
{
	if (m_gameRunning == running)
		return;
	m_gameRunning = running;
	emit gameRunningChanged();
}

// ── Privater Nachrichtenverlauf (Posteingang) ──────────────────────────────

void LobbyHandler::appendPrivateMessage(const QString &playerName, const QString &message, bool fromMe)
{
	if (playerName.isEmpty())
		return;
	PrivateThread &thread = m_privateThreads[playerName];

	QVariantMap entry;
	entry.insert(QStringLiteral("msgId"), m_nextPrivateMessageId++);
	entry.insert(QStringLiteral("fromMe"), fromMe);
	entry.insert(QStringLiteral("text"), message);
	// A full timestamp: the history outlives sessions, "HH:mm"
	// alone would be misleading for a message from the day before yesterday. The
	// display form is built by privateConversation().
	entry.insert(QStringLiteral("ts"),
				 QDateTime::currentDateTime().toString(Qt::ISODate));
	thread.messages.append(entry);

	// Keep only a window of the conversation in memory; the database keeps
	// the complete history.
	while (thread.messages.size() > kPrivateMessagesLoaded)
		thread.messages.removeFirst();

	thread.lastActivity = QDateTime::currentMSecsSinceEpoch();
	if (!fromMe)
		++thread.unread;

	persistPrivateMessage(playerName, entry);
	persistPrivateThreadMeta(playerName);

	++m_privateMessagesRevision;
	emit privateMessagesChanged();
	recountUnreadPrivateMessages();
}

void LobbyHandler::recountUnreadPrivateMessages()
{
	int total = 0;
	for (auto it = m_privateThreads.constBegin(); it != m_privateThreads.constEnd(); ++it)
		total += it->unread;
	if (total == m_unreadPrivateMessages)
		return;
	m_unreadPrivateMessages = total;
	emit unreadPrivateMessagesChanged();
}

namespace
{

// The storage location of the inbox: a SQLite file of its own next to the config.xml, so that
// it follows the same user profile as all the other settings – but stays
// separate from them and from the game logs.
QString privateMessagesDbPath(ConfigFile *config)
{
	if (!config)
		return QString();
	const QString configPath = QString::fromUtf8(config->configFileName.c_str());
	if (configPath.isEmpty())
		return QString();
	return QFileInfo(configPath).absolutePath() + QStringLiteral("/privatemessages.sqlite");
}

// Does the table already have the owner column? (The first version of the file knew
// only ONE inbox for the whole installation.)
bool privateMessageTableHasOwner(const QSqlDatabase &db, const QString &table)
{
	QSqlQuery query(db);
	if (!query.exec(QStringLiteral("PRAGMA table_info(%1)").arg(table)))
		return false;
	while (query.next()) {
		if (query.value(1).toString() == QLatin1String("owner"))
			return true;
	}
	return false;
}

} // namespace

void LobbyHandler::openPrivateMessageDb()
{
	if (!m_privateDbConn.isEmpty())
		return;

	const QString path = privateMessagesDbPath(m_config);
	if (path.isEmpty())
		return;

	const QString connName = QStringLiteral("pokerth_pm");
	bool opened = false;
	// A scope of its own: while a QSqlDatabase copy lives,
	// removeDatabase() warns about a connection that is still in use.
	{
		QSqlDatabase db = QSqlDatabase::contains(connName)
						  ? QSqlDatabase::database(connName, false)
						  : QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
		db.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=5000"));
		db.setDatabaseName(path);
		if (!db.open()) {
			qWarning() << "[PM] cannot open private message database" << path << db.lastError().text();
		} else {
			QSqlQuery query(db);
			// The upgrade of the first version (without an owner): pm_message gets
			// the column appended, pm_thread needs a new table because of the
			// composite primary key. The old rows
			// stay without an owner at first (owner = "") and are adopted at the first
			// login – see loadPrivateMessages().
			const QStringList tables = db.tables();
			if (tables.contains(QLatin1String("pm_message"))
					&& !privateMessageTableHasOwner(db, QStringLiteral("pm_message"))) {
				query.exec(QStringLiteral(
							   "ALTER TABLE pm_message ADD COLUMN owner TEXT NOT NULL DEFAULT ''"));
			}
			if (tables.contains(QLatin1String("pm_thread"))
					&& !privateMessageTableHasOwner(db, QStringLiteral("pm_thread"))) {
				query.exec(QStringLiteral("ALTER TABLE pm_thread RENAME TO pm_thread_v1"));
			}
			// Look again instead of remembering the result of the rename:
			// that way the remnants of an aborted upgrade are adopted
			// as well.
			const bool migrateThreads = db.tables().contains(QLatin1String("pm_thread_v1"));

			// pm_thread: one row per account and conversation partner (the unread
			// counter and the sorting), pm_message: the messages themselves.
			opened =
				query.exec(QStringLiteral(
							   "CREATE TABLE IF NOT EXISTS pm_thread ("
							   "  owner TEXT NOT NULL,"
							   "  partner TEXT NOT NULL,"
							   "  unread INTEGER NOT NULL DEFAULT 0,"
							   "  last_activity INTEGER NOT NULL DEFAULT 0,"
							   "  PRIMARY KEY (owner, partner))"))
				&& query.exec(QStringLiteral(
								  "CREATE TABLE IF NOT EXISTS pm_message ("
								  "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
								  "  owner TEXT NOT NULL DEFAULT '',"
								  "  partner TEXT NOT NULL,"
								  "  from_me INTEGER NOT NULL,"
								  "  text TEXT NOT NULL,"
								  "  ts TEXT NOT NULL)"))
				&& query.exec(QStringLiteral("DROP INDEX IF EXISTS pm_message_partner"))
				&& query.exec(QStringLiteral(
								  "CREATE INDEX IF NOT EXISTS pm_message_owner "
								  "ON pm_message (owner, partner, id)"));

			if (opened && migrateThreads) {
				opened = query.exec(QStringLiteral(
										"INSERT OR IGNORE INTO pm_thread (owner, partner, unread, last_activity) "
										"SELECT '', partner, unread, last_activity FROM pm_thread_v1"))
						 && query.exec(QStringLiteral("DROP TABLE pm_thread_v1"));
			}
			if (!opened) {
				qWarning() << "[PM] cannot create private message tables:" << query.lastError().text();
				db.close();
			}
		}
	}

	if (!opened) {
		QSqlDatabase::removeDatabase(connName);
		return;   // without a database the history only lives until the program ends
	}
	m_privateDbConn = connName;
}

void LobbyHandler::setPrivateMessageOwner(const QString &owner)
{
	if (owner == m_privateMessagesOwner)
		return;
	m_privateMessagesOwner = owner;

	// What is in memory belongs to the previous account – clear it away first, then
	// fetch the history of the new login from the database (with an empty name
	// the inbox stays empty).
	m_privateThreads.clear();
	// Running translations point at bubbles that do not exist any more; their
	// replies should go nowhere instead of into the new history.
	m_pmTranslationRequests.clear();
	loadPrivateMessages();

	++m_privateMessagesRevision;
	emit privateMessagesChanged();
	recountUnreadPrivateMessages();
}

void LobbyHandler::loadPrivateMessages()
{
	openPrivateMessageDb();
	if (m_privateDbConn.isEmpty() || m_privateMessagesOwner.isEmpty())
		return;

	QSqlDatabase db = QSqlDatabase::database(m_privateDbConn, false);

	// History from the time before the account separation (owner = "") belongs to whoever
	// logs in first – that is the user who wrote it.
	// After that there are no ownerless rows any more.
	{
		QSqlQuery adoptQuery(db);
		adoptQuery.prepare(QStringLiteral(
							   "UPDATE OR REPLACE pm_thread SET owner = :owner WHERE owner = ''"));
		adoptQuery.bindValue(QStringLiteral(":owner"), m_privateMessagesOwner);
		if (adoptQuery.exec() && adoptQuery.numRowsAffected() > 0) {
			QSqlQuery adoptMessages(db);
			adoptMessages.prepare(QStringLiteral(
									  "UPDATE pm_message SET owner = :owner WHERE owner = ''"));
			adoptMessages.bindValue(QStringLiteral(":owner"), m_privateMessagesOwner);
			adoptMessages.exec();
		}
	}

	QSqlQuery threadQuery(db);
	threadQuery.prepare(QStringLiteral(
							"SELECT partner, unread, last_activity FROM pm_thread WHERE owner = :owner "
							"ORDER BY last_activity DESC"));
	threadQuery.bindValue(QStringLiteral(":owner"), m_privateMessagesOwner);
	if (!threadQuery.exec()) {
		qWarning() << "[PM] cannot read conversations:" << threadQuery.lastError().text();
		return;
	}

	QSqlQuery messageQuery(db);
	// Only fetch the most recent messages into memory; the database keeps
	// the complete history.
	messageQuery.prepare(QStringLiteral(
							 "SELECT from_me, text, ts FROM pm_message WHERE owner = :owner AND partner = :partner "
							 "ORDER BY id DESC LIMIT :limit"));
	messageQuery.bindValue(QStringLiteral(":owner"), m_privateMessagesOwner);

	while (threadQuery.next()) {
		const QString name = threadQuery.value(0).toString();
		if (name.isEmpty())
			continue;
		PrivateThread thread;
		thread.unread       = threadQuery.value(1).toInt();
		thread.lastActivity = threadQuery.value(2).toLongLong();

		messageQuery.bindValue(QStringLiteral(":partner"), name);
		messageQuery.bindValue(QStringLiteral(":limit"), kPrivateMessagesLoaded);
		if (messageQuery.exec()) {
			while (messageQuery.next()) {
				QVariantMap entry;
				entry.insert(QStringLiteral("msgId"), m_nextPrivateMessageId++);
				entry.insert(QStringLiteral("fromMe"), messageQuery.value(0).toInt() != 0);
				entry.insert(QStringLiteral("text"),   messageQuery.value(1).toString());
				entry.insert(QStringLiteral("ts"),     messageQuery.value(2).toString());
				// The query ran in descending order (the newest first) – inserting at the front
				// yields the chronological order again.
				thread.messages.prepend(entry);
			}
		}
		// Unread can never be more than the history provides.
		thread.unread = qBound(0, thread.unread, static_cast<int>(thread.messages.size()));
		m_privateThreads.insert(name, thread);
	}
	// The report to the user interface is made by the only caller
	// (setPrivateMessageOwner) – it has to send it anyway even when
	// there was nothing to load here (logging out, an account without a history).
}

void LobbyHandler::persistPrivateMessage(const QString &playerName, const QVariantMap &entry)
{
	// Without a known owner (not logged in yet) nothing is stored:
	// the history could otherwise not be assigned to any account.
	if (m_privateDbConn.isEmpty() || m_privateMessagesOwner.isEmpty())
		return;
	QSqlQuery query(QSqlDatabase::database(m_privateDbConn, false));
	query.prepare(QStringLiteral(
					  "INSERT INTO pm_message (owner, partner, from_me, text, ts) "
					  "VALUES (:owner, :partner, :fromMe, :text, :ts)"));
	query.bindValue(QStringLiteral(":owner"), m_privateMessagesOwner);
	query.bindValue(QStringLiteral(":partner"), playerName);
	query.bindValue(QStringLiteral(":fromMe"), entry.value(QStringLiteral("fromMe")).toBool() ? 1 : 0);
	query.bindValue(QStringLiteral(":text"), entry.value(QStringLiteral("text")).toString());
	query.bindValue(QStringLiteral(":ts"), entry.value(QStringLiteral("ts")).toString());
	if (!query.exec())
		qWarning() << "[PM] cannot store private message:" << query.lastError().text();
}

void LobbyHandler::persistPrivateThreadMeta(const QString &playerName)
{
	if (m_privateDbConn.isEmpty() || m_privateMessagesOwner.isEmpty())
		return;
	const auto it = m_privateThreads.constFind(playerName);
	if (it == m_privateThreads.constEnd())
		return;
	QSqlQuery query(QSqlDatabase::database(m_privateDbConn, false));
	// INSERT OR REPLACE instead of UPSERT: the table consists only of these four
	// columns, and a placeholder may thus occur exactly once each.
	query.prepare(QStringLiteral(
					  "INSERT OR REPLACE INTO pm_thread (owner, partner, unread, last_activity) "
					  "VALUES (:owner, :partner, :unread, :last)"));
	query.bindValue(QStringLiteral(":owner"), m_privateMessagesOwner);
	query.bindValue(QStringLiteral(":partner"), playerName);
	query.bindValue(QStringLiteral(":unread"), it->unread);
	query.bindValue(QStringLiteral(":last"), it->lastActivity);
	if (!query.exec())
		qWarning() << "[PM] cannot store conversation:" << query.lastError().text();
}

void LobbyHandler::persistDeletePrivateThread(const QString &playerName)
{
	if (m_privateDbConn.isEmpty() || m_privateMessagesOwner.isEmpty())
		return;
	QSqlDatabase db = QSqlDatabase::database(m_privateDbConn, false);
	QSqlQuery messageQuery(db);
	messageQuery.prepare(QStringLiteral(
							 "DELETE FROM pm_message WHERE owner = :owner AND partner = :partner"));
	messageQuery.bindValue(QStringLiteral(":owner"), m_privateMessagesOwner);
	messageQuery.bindValue(QStringLiteral(":partner"), playerName);
	messageQuery.exec();
	QSqlQuery threadQuery(db);
	threadQuery.prepare(QStringLiteral(
							"DELETE FROM pm_thread WHERE owner = :owner AND partner = :partner"));
	threadQuery.bindValue(QStringLiteral(":owner"), m_privateMessagesOwner);
	threadQuery.bindValue(QStringLiteral(":partner"), playerName);
	threadQuery.exec();
}

void LobbyHandler::deletePrivateConversation(const QString &playerName)
{
	if (m_privateThreads.remove(playerName) == 0)
		return;
	persistDeletePrivateThread(playerName);
	++m_privateMessagesRevision;
	emit privateMessagesChanged();
	recountUnreadPrivateMessages();
}

namespace
{

// The display form of a stored timestamp: within the same day only
// the time, before that the date in addition.
QString privateMessageDisplayTime(const QString &isoTimestamp)
{
	const QDateTime ts = QDateTime::fromString(isoTimestamp, Qt::ISODate);
	if (!ts.isValid())
		return QString();
	if (ts.date() == QDate::currentDate())
		return ts.toString(QStringLiteral("HH:mm"));
	return ts.toString(QStringLiteral("dd.MM. HH:mm"));
}

} // namespace

QVariantList LobbyHandler::privateConversationPartners() const
{
	QVariantList out;
	out.reserve(m_privateThreads.size());
	for (auto it = m_privateThreads.constBegin(); it != m_privateThreads.constEnd(); ++it) {
		const PrivateThread &thread = it.value();
		QVariantMap entry;
		entry.insert(QStringLiteral("name"), it.key());
		entry.insert(QStringLiteral("unread"), thread.unread);
		entry.insert(QStringLiteral("playerId"), playerIdByName(it.key()));
		entry.insert(QStringLiteral("lastActivity"), thread.lastActivity);
		if (!thread.messages.isEmpty()) {
			const QVariantMap last = thread.messages.last().toMap();
			entry.insert(QStringLiteral("lastText"), last.value(QStringLiteral("text")));
			entry.insert(QStringLiteral("lastTime"),
						 privateMessageDisplayTime(last.value(QStringLiteral("ts")).toString()));
			entry.insert(QStringLiteral("fromMe"), last.value(QStringLiteral("fromMe")));
		} else {
			entry.insert(QStringLiteral("lastText"), QString());
			entry.insert(QStringLiteral("lastTime"), QString());
			entry.insert(QStringLiteral("fromMe"), false);
		}
		out.append(entry);
	}
	// The newest conversation first (a QHash is unsorted).
	std::sort(out.begin(), out.end(), [](const QVariant &a, const QVariant &b) {
		return a.toMap().value(QStringLiteral("lastActivity")).toLongLong()
			   > b.toMap().value(QStringLiteral("lastActivity")).toLongLong();
	});
	return out;
}

QVariantList LobbyHandler::privateConversation(const QString &playerName) const
{
	const auto it = m_privateThreads.constFind(playerName);
	if (it == m_privateThreads.constEnd())
		return QVariantList();
	QVariantList out;
	out.reserve(it->messages.size());
	for (const QVariant &messageValue : it->messages) {
		QVariantMap entry = messageValue.toMap();
		entry.insert(QStringLiteral("time"),
					 privateMessageDisplayTime(entry.value(QStringLiteral("ts")).toString()));
		out.append(entry);
	}
	return out;
}

void LobbyHandler::ensurePrivateConversation(const QString &playerName)
{
	if (playerName.isEmpty() || m_privateThreads.contains(playerName))
		return;
	PrivateThread &thread = m_privateThreads[playerName];
	// Without a timestamp a freshly opened (empty) conversation would always stand
	// at the very bottom of the list, although it is the active one.
	thread.lastActivity = QDateTime::currentMSecsSinceEpoch();
	// Deliberately NOT stored: a conversation that is only opened and empty should not
	// stand in the inbox permanently. The first message creates the row.
	++m_privateMessagesRevision;
	emit privateMessagesChanged();
}

void LobbyHandler::markPrivateConversationRead(const QString &playerName)
{
	const auto it = m_privateThreads.find(playerName);
	if (it == m_privateThreads.end() || it->unread == 0)
		return;
	it->unread = 0;
	persistPrivateThreadMeta(playerName);
	++m_privateMessagesRevision;
	emit privateMessagesChanged();
	recountUnreadPrivateMessages();
}

unsigned LobbyHandler::playerIdByName(const QString &playerName) const
{
	if (playerName.isEmpty())
		return 0;
	const int count = m_playerListModel.rowCount();
	for (int i = 0; i < count; ++i) {
		const QModelIndex idx = m_playerListModel.index(i);
		if (m_playerListModel.data(idx, PlayerListModel::PlayerNameRole).toString() == playerName)
			return m_playerListModel.data(idx, PlayerListModel::PlayerIdRole).toUInt();
	}
	return 0;
}

void LobbyHandler::sendPrivateMessageToName(const QString &playerName, const QString &message)
{
	const unsigned targetId = playerIdByName(playerName);
	if (targetId == 0) {
		emit errorOccurred(tr("%1 is not in the lobby at the moment.").arg(playerName));
		return;
	}
	sendPrivateMessage(targetId, message);
}

bool LobbyHandler::chatDarkMode() const
{
	// No config -> dark mode (the default of the user interface). "Automatic" resolves
	// DarkMode::resolve() via the system – the same semantics as in QML.
	if (!m_config)
		return true;
	return DarkMode::resolve(m_config->readConfigInt("DarkMode"));
}

QStringList LobbyHandler::chatLog() const
{
	// Only resolve the colour placeholders here: that way a light/dark change recolours
	// the history that has already been received as well (see chatcolors.h).
	const bool dark = chatDarkMode();
	QStringList out;
	out.reserve(m_chatLog.size());
	for (const QString &line : m_chatLog)
		out.append(ChatColors::expand(line, dark));
	return out;
}

void LobbyHandler::pushChatLine(const QString &line)
{
	m_chatLog.append(line);
	const int kMaxLines = 400;
	if (m_chatLog.size() > kMaxLines)
		m_chatLog.erase(m_chatLog.begin(), m_chatLog.begin() + (m_chatLog.size() - kMaxLines));
	notifyChatLogChanged();
	// The live consumers get the line fully coloured (not with placeholders).
	emit chatLineReady(ChatColors::expand(line, chatDarkMode()));
}

void LobbyHandler::notifyChatLogChanged()
{
	// It collects all history changes of one event loop pass into a
	// single notification (for the reasoning see lobbyhandler.h). The
	// delivery itself happens via a queued call: lines of the same pass that are
	// still pending (a multi-line notice, the salvo when entering) thus land
	// in the same build as well.
	if (m_chatLogNotifyPending)
		return;
	m_chatLogNotifyPending = true;
	QMetaObject::invokeMethod(this, [this]() {
		m_chatLogNotifyPending = false;
		emit chatLogChanged();
	}, Qt::QueuedConnection);
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

	return playerIdByName(targetName);
}

void LobbyHandler::createGame(const QString &name, const QString &password,
							  int gameType, bool allowSpectators, int maxPlayers,
							  int startCash, int firstSmallBlind,
							  int raiseIntervalMode, int raiseEveryHands,
							  int raiseEveryMinutes, int raiseMode,
							  int playerActionTimeout, int delayBetweenHands,
							  const QVariantList &manualBlinds)
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
	if (gameData.raiseMode == MANUAL_BLINDS_ORDER) {
		for (const QVariant &blind : manualBlinds)
			gameData.manualBlindsList.push_back(blind.toInt());
	}
	// The behaviour after the manual blind list and the GUI speed
	// have no controls on the create page; as in the
	// widget client they come from the options.
	if (m_config) {
		if (m_config->readConfigInt("NetAfterMBAlwaysRaiseAbout")) {
			gameData.afterManualBlindsMode   = AFTERMB_RAISE_ABOUT;
			gameData.afterMBAlwaysRaiseValue = m_config->readConfigInt("NetAfterMBAlwaysRaiseValue");
		} else if (m_config->readConfigInt("NetAfterMBStayAtLastBlind")) {
			gameData.afterManualBlindsMode   = AFTERMB_STAY_AT_LAST_BLIND;
		}
		gameData.guiSpeed = m_config->readConfigInt("GameSpeed");
	}
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

void LobbyHandler::spectateGame(unsigned gameId)
{
	if (!m_session) {
		emit errorOccurred(tr("Not connected to server"));
		return;
	}
	m_session->clientJoinGame(gameId, std::string(), true);
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
	// Disconnect from the server (as startWindowImpl does when leaving the
	// lobby) and reset the local lobby state.
	m_session->terminateNetworkClient();
	// No active online session any more → stop the foreground service.
	AndroidConnectionService::stop();
	IosBackgroundSession::stop();
	setGameRunning(false);
	setRejoinWaiting(false);
	if (m_isSpectating) {
		m_isSpectating = false;
		emit isSpectatingChanged();
	}
	if (m_isInGame) {
		m_isInGame = false;
		m_currentGameId = 0;
		emit isInGameChanged();
		emit currentGameIdChanged();
	}
}

void LobbyHandler::onSelfJoinedGame()
{
	// A fresh join → the waiting room (with a rejoin into a running game,
	// onGameStarted follows immediately afterwards).
	setGameRunning(false);
	m_currentGameId = m_session ? m_session->getClientCurrentGameId() : 0;
	// Whether the server has accepted us as a spectator is in the JoinGameAck –
	// that one is already processed when this signal reaches the GUI.
	const bool spectating = m_session && m_session->isClientSpectating();
	if (spectating != m_isSpectating) {
		m_isSpectating = spectating;
		emit isSpectatingChanged();
	}
	if (!m_isInGame) {
		m_isInGame = true;
		emit isInGameChanged();
		emit currentGameIdChanged();
	}
	emit selfJoinedGame();
}

void LobbyHandler::onGameStarted()
{
	// A game start means that the engine unsubscribes from the lobby messages
	// (UnsubscribeLobbyMsg). During the game no
	// playerListLeft events arrive any more – players who disconnect during
	// that time would otherwise stay in the list as stale "idle" entries.
	// When returning into the waiting room/the lobby the server sends the complete
	// player list again via ResubscribeLobbyMsg (playerListNew),
	// so that the list can safely be cleared here and rebuilt freshly
	// afterwards. It mirrors the behaviour of the widget client (clearing the nick list on
	// MSG_NET_GAME_CLIENT_START).
	m_playerListModel.clear();
	++m_playerListRevision;
	emit playerListRevisionChanged();

	setGameRunning(true);
	// We sit at the table → a rejoin wait that may be running is finished.
	setRejoinWaiting(false);

	emit gameStarted();
}

void LobbyHandler::onWaitGameDialog()
{
	// Do NOT reset m_isInGame/m_currentGameId: with auto-leave disabled
	// we stay in the (reopened) game after the end of the game; the waiting room should
	// keep showing the current game. If the player really is removed
	// (auto-leave/kick), the following onRemovedFromGame cleans the state up.
	setGameRunning(false);
	emit returnToWaitRoom();
}

void LobbyHandler::onRemovedFromGame(int reason)
{
	m_isInGame = false;
	setGameRunning(false);
	// It covers NTF_NET_REMOVED_START_FAILED as well: the server has started the hand without
	// us, so waiting for the rejoin is void.
	setRejoinWaiting(false);
	m_currentGameId = 0;
	if (m_isSpectating) {
		m_isSpectating = false;
		emit isSpectatingChanged();
	}
	// The game admin (host) status expires when leaving the table; the
	// server admin status stays untouched by it.
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
	// The manual blind order: it serves the community suggest as a fingerprint to
	// recognise a foreign BBC step table (the table name is freely editable
	// and is unsuitable for that) – see Config.BotSuggest.suggestTypeForGameInfo.
	QVariantList manualBlinds;
	for (std::list<int>::const_iterator it = info.data.manualBlindsList.begin();
			it != info.data.manualBlindsList.end(); ++it) {
		manualBlinds.append(*it);
	}
	result.insert("manualBlinds",       manualBlinds);
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

bool LobbyHandler::isPlayerInRunningGame(unsigned playerId) const
{
	if (!m_session || playerId == 0)
		return false;
	const unsigned gameId = m_session->getGameIdOfPlayer(playerId);
	if (gameId == 0)
		return false;
	return m_session->getClientGameInfo(gameId).mode == GAME_MODE_STARTED;
}

QString LobbyHandler::playerInGameName(unsigned playerId) const
{
	if (!m_session || playerId == 0)
		return QString();
	const unsigned gameId = m_session->getGameIdOfPlayer(playerId);
	if (gameId == 0)
		return QString();
	return QString::fromUtf8(m_session->getClientGameInfo(gameId).name.c_str());
}

// ── Rejoin after a connection loss ──────────────────────────────────────────
// At the login the server recognises by the player name + the old session GUID
// that a running game session still exists, and offers it in the InitAck
// (rejoinGameId). The LobbyPage shows the popup for it (rejoinOfferGameId).
void LobbyHandler::onRejoinPossible(unsigned gameId)
{
	qDebug() << "[REJOIN] onRejoinPossible: gameId=" << gameId
			 << "autoRejoin=" << m_autoRejoin;
	if (m_rejoinOfferGameId == gameId)
		return;
	m_rejoinOfferGameId = gameId;
	emit rejoinOfferChanged();

	// After an automatic reconnect, back to the table without a question:
	// the player never left it voluntarily, a yes/no popup
	// would only be a hurdle here - and the 5 minute deadline of the server is running.
	if (m_autoRejoin) {
		m_autoRejoin = false;
		qInfo() << "[REJOIN] auto-accepting after reconnect, gameId=" << gameId;
		acceptRejoin();
	}
}

void LobbyHandler::setAutoRejoin(bool on)
{
	m_autoRejoin = on;
}

void LobbyHandler::acceptRejoin()
{
	const unsigned gameId = m_rejoinOfferGameId;
	qDebug() << "[REJOIN] acceptRejoin: gameId=" << gameId;
	m_rejoinOfferGameId = 0;
	emit rejoinOfferChanged();
	if (!m_session || gameId == 0)
		return;
	m_session->clientRejoinGame(gameId);
}

void LobbyHandler::setRejoinWaiting(bool waiting)
{
	if (m_rejoinWaiting == waiting)
		return;
	m_rejoinWaiting = waiting;
	emit rejoinWaitingChanged();
}

// The server has accepted the rejoin and sends the StartEvent of type
// rejoinEvent. We are only put at the table at the beginning of the
// next hand - until then the waiting room stays.
void LobbyHandler::onRejoinSyncWait()
{
	qDebug() << "[REJOIN] onRejoinSyncWait: waiting for next hand";
	setRejoinWaiting(true);
}

void LobbyHandler::declineRejoin()
{
	qDebug() << "[REJOIN] declineRejoin: gameId=" << m_rejoinOfferGameId;
	if (m_rejoinOfferGameId != 0) {
		m_rejoinOfferGameId = 0;
		emit rejoinOfferChanged();
	}
}

// ── Incoming game invitations (invite-only games) ──────────────────────────
void LobbyHandler::onSelfGameInvitation(unsigned gameId, unsigned playerIdFrom)
{
	qDebug() << "[INVITE] onSelfGameInvitation: gameId=" << gameId << "fromPlayerId=" << playerIdFrom
			 << "pendingInviteGameId=" << m_pendingInviteGameId
			 << "ignored=" << isPlayerIgnored(playerIdFrom)
			 << "session=" << (m_session ? "ok" : "NULL");
	if (!m_session)
		return;
	// The sender is on the ignore list OR an invitation popup is already
	// open → decline automatically with "busy" (as in the Qt widgets client).
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
	const QString tsPrefix = chatTimestampPrefix(m_config);
	pushChatLine(tsPrefix + QStringLiteral("<span style=\"")
				 + ChatColors::colorStyle(ChatColors::Info) + QStringLiteral(";\">")
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
	const QString tsPrefix = chatTimestampPrefix(m_config);
	pushChatLine(tsPrefix + QStringLiteral("<span style=\"")
				 + ChatColors::colorStyle(ChatColors::Reject) + QStringLiteral(";\">")
				 + msg + QStringLiteral("</span>"));
}

void LobbyHandler::adminBanPlayer(unsigned playerId)
{
	if (!m_session) {
		emit errorOccurred(tr("Not connected to server"));
		return;
	}
	m_session->adminActionBanPlayer(playerId);
}

void LobbyHandler::adminSendGlobalNotice(const QString &noticeText)
{
	if (!m_session) {
		emit errorOccurred(tr("Not connected to server"));
		return;
	}
	QString text = noticeText.trimmed();
	if (text.isEmpty())
		return;
	// The server distributes the announcement as a chat message – hence the same
	// 128 byte limit as for the chat (otherwise the packet validator discards it).
	while (!text.isEmpty() && text.toUtf8().size() > 128)
		text.chop(1);
	m_session->adminActionGlobalNotice(text.toStdString());
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
	if (targetPlayerId == 0)
		return;
	// Deliberately blocked at a running table: private arrangements during a hand
	// should not be possible in the first place (the server does not deliver PMs to players in
	// running games anyway).
	if (m_gameRunning) {
		emit errorOccurred(tr("Private messages are not available at the table."));
		return;
	}
	// On the server side guests are not allowed to chat at all (not privately either) –
	// the same message as in the lobby chat, instead of a silent rejection.
	if (isMyPlayerGuest()) {
		emit errorOccurred(tr("Guests cannot send chat messages"));
		return;
	}
	// The same the other way round: the server delivers nothing to a guest. The check
	// sits here and not only at the buttons, because this is the only bottleneck
	// that EVERY sending path runs through.
	if (isPlayerGuest(targetPlayerId)) {
		emit errorOccurred(tr("Guests cannot receive private messages."));
		return;
	}
	QString text = message.trimmed();
	// The same 128 byte limit as in the chat: the packet validator of the server
	// discards longer messages (and cuts the connection if in doubt).
	while (!text.isEmpty() && text.toUtf8().size() > 128)
		text.chop(1);
	if (text.isEmpty())
		return;
	m_session->sendPrivateChatMessage(targetPlayerId, text.toStdString());
	const QString targetName = resolvedPlayerName(targetPlayerId);
	pushPrivateMessageSentLine(targetName, text);
	appendPrivateMessage(targetName, text, true);
}

void LobbyHandler::pushPrivateMessageSentLine(const QString &targetName, const QString &message)
{
	// The same preparation and colour as an INCOMING PM (onPrivateChatMessage),
	// only with "to <name>" instead of "<name>(pm)". The full text deliberately stands in the
	// line: a sent PM would otherwise not appear anywhere in your own history.
	QString escapedMsg = ChatColors::chatEscape(message);
	escapedMsg = applyChatEmoteShortcuts(escapedMsg);
	escapedMsg = enlargeEmojis(escapedMsg);
	// Multi-line notices (e.g. the community suggestion with one player per
	// line) come as plain text with "\n" – in the rich text chat that would only be a
	// space, so convert it into <br> here (after the escaping, so that no
	// foreign text influences the markup).
	escapedMsg.replace(QLatin1Char('\n'), QLatin1String("<br>"));

	const QString tsPrefix = chatTimestampPrefix(m_config);
	const QString line = tsPrefix + QLatin1String("<i><span style=\"")
						 + ChatColors::colorStyle(ChatColors::Muted)
						 + QLatin1String(";\">")
						 + tr("Private message to %1:").arg(ChatColors::chatEscape(targetName))
						 + QLatin1String(" ") + escapedMsg
						 + QLatin1String("</span></i>");
	pushChatLine(line);
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

// It resolves the playerId to the name and reports the request to QML
// (pokerth.qml pushes the native PokerthPlayerPage) – previously the
// browser link redirect_user_profile.php?nick=… was opened here.
void LobbyHandler::showPlayerStats(unsigned playerId)
{
	if (playerId == 0) return;
	const QString playerName = resolvedPlayerName(playerId);
	if (playerName.isEmpty()) return;

	emit playerStatsRequested(playerName);
}

// ── Domain text helpers ────────────────────────────────────────────────────

QString LobbyHandler::gameTypeText(int gameType) const
{
	switch (gameType) {
	case 2:
		return tr("Registered players only");
	case 3:
		return tr("Invited players only");
	case 4:
		return tr("Ranking game");
	default:
		return tr("Standard");
	}
}

QString LobbyHandler::gameStatusText(int gameMode, int playerCount, int maxPlayers) const
{
	if (gameMode == 2) return tr("Running");
	if (gameMode == 3) return tr("Closed");
	return playerCount < maxPlayers ? tr("Open") : tr("Full");
}
