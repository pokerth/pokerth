/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2025 Felix Hammer, Florian Thauer, Lothar May          *
 *****************************************************************************/

#ifndef LOBBYHANDLER_H
#define LOBBYHANDLER_H

#include <QObject>
#include <QAbstractListModel>
#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QPair>
#include <QVariantMap>
#include <boost/shared_ptr.hpp>

class Session;
class SoundEvents;
class ConfigFile;
class ChatTranslator;
class TextTranslator;
struct GameInfo;

// Model for players in lobby
class PlayerListModel : public QAbstractListModel
{
	Q_OBJECT
	Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
	enum PlayerRoles {
		PlayerIdRole = Qt::UserRole + 1,
		PlayerNameRole,
		IsAdminRole,
		CountryCodeRole,
		IsGuestRole,
		// The table the player sits at or spectates; 0 = idle. Deliberately a
		// model datum and not a query of the session: only that way does the
		// idle filter of the proxy re-evaluate the row by itself when joining
		// (dataChanged). The counterpart to role 34 of the widget client.
		GameIdRole
	};

	explicit PlayerListModel(QObject *parent = nullptr);

	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QHash<int, QByteArray> roleNames() const override;

	int count() const
	{
		return m_players.count();
	}

	void addPlayer(unsigned playerId, const QString &playerName, bool isAdmin = false, const QString &countryCode = QString(), bool isGuest = false);
	// Keeps the table membership up to date (0 = idle) and only reports real
	// changes as dataChanged.
	void setPlayerGameId(unsigned playerId, unsigned gameId);
	QList<unsigned> playerIds() const;
	void removePlayer(unsigned playerId);
	void updatePlayer(unsigned playerId, const QString &newName);
	void updatePlayerInfo(unsigned playerId, const QString &playerName, bool isAdmin, const QString &countryCode = QString(), bool isGuest = false);
	void clear();

signals:
	void countChanged();

private:
	struct PlayerInfo {
		unsigned id;
		QString name;
		bool isAdmin;
		QString countryCode;
		bool isGuest;
		unsigned gameId;
	};

	QList<PlayerInfo> m_players;
	QHash<unsigned, int> m_playerIndexMap; // playerId -> index
};

// Model for games in lobby
class GameListModel : public QAbstractListModel
{
	Q_OBJECT
	Q_PROPERTY(int runningCount READ runningCount NOTIFY runningCountChanged)
	Q_PROPERTY(int openCount READ openCount NOTIFY openCountChanged)

public:
	enum GameRoles {
		GameIdRole = Qt::UserRole + 1,
		GameNameRole,
		PlayerCountRole,
		MaxPlayersRole,
		GameModeRole,
		IsPrivateRole,
		GameTypeRole,
		FirstSmallBlindRole,
		StartMoneyRole,
		RaiseIntervalModeRole,
		RaiseEveryHandsRole,
		RaiseEveryMinutesRole,
		RaiseModeRole,
		ManualBlindsTextRole,
		PlayerActionTimeoutRole,
		DelayBetweenHandsRole
	};

	explicit GameListModel(QObject *parent = nullptr);

	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QHash<int, QByteArray> roleNames() const override;

	int runningCount() const
	{
		return m_runningCount;
	}
	int openCount() const
	{
		return m_openCount;
	}

	void addGame(unsigned gameId, const QString &gameName);
	void removeGame(unsigned gameId);
	void updateGameMode(unsigned gameId, int mode);
	void updateGameInfo(unsigned gameId, const ::GameInfo &info);
	void clear();

signals:
	void runningCountChanged();
	void openCountChanged();

private:
	struct GameEntry {
		unsigned id;
		QString name;
		int playerCount;
		int maxPlayers;
		int gameMode;
		bool isPrivate;
		int gameType;
		int firstSmallBlind;
		int startMoney;
		int raiseIntervalMode;
		int raiseEveryHands;
		int raiseEveryMinutes;
		int raiseMode;
		QString manualBlindsText;
		int playerActionTimeoutSec;
		int delayBetweenHandsSec;
	};

	void recomputeCounts();

	QList<GameEntry> m_games;
	QHash<unsigned, int> m_gameIndexMap; // gameId -> index
	int m_runningCount = 0;
	int m_openCount = 0;
};

// Main lobby handler
class LobbyHandler : public QObject
{
	Q_OBJECT
	Q_PROPERTY(PlayerListModel* playerListModel READ playerListModel CONSTANT)
	Q_PROPERTY(QAbstractItemModel* playerListProxyModel READ playerListProxyModel CONSTANT)
	Q_PROPERTY(GameListModel* gameListModel READ gameListModel CONSTANT)
	Q_PROPERTY(QAbstractItemModel* gameListProxyModel READ gameListProxyModel CONSTANT)
	Q_PROPERTY(QString myPlayerName READ myPlayerName NOTIFY myPlayerNameChanged)
	Q_PROPERTY(unsigned myPlayerId READ myPlayerId NOTIFY myPlayerIdChanged)
	Q_PROPERTY(bool isMyPlayerGuest READ isMyPlayerGuest NOTIFY gameContextChanged)
	// Unread private messages (the counter at the inbox symbol of the
	// header). It drops as soon as a conversation is read in the dialog
	// (markPrivateConversationRead).
	Q_PROPERTY(int unreadPrivateMessages READ unreadPrivateMessages NOTIFY unreadPrivateMessagesChanged)
	// A counter property as a reactive dependency for QML: the history itself is
	// read via Q_INVOKABLE functions (as with the player list).
	Q_PROPERTY(int privateMessagesRevision READ privateMessagesRevision NOTIFY privateMessagesChanged)
	// Server admin (may kickban / close games) – authoritatively from the
	// PlayerInfo of the session. Strictly separate from the game admin (host).
	Q_PROPERTY(bool isCurrentPlayerAdmin READ isCurrentPlayerAdmin NOTIFY isCurrentPlayerAdminChanged)
	// Game admin (host/creator of the current table) – may start the game.
	Q_PROPERTY(bool isCurrentGameAdmin READ isCurrentGameAdmin NOTIFY isCurrentGameAdminChanged)
	Q_PROPERTY(bool canInviteFromCurrentGame READ canInviteFromCurrentGame NOTIFY gameContextChanged)
	Q_PROPERTY(int playerListFilterMode READ playerListFilterMode WRITE setPlayerListFilterMode NOTIFY playerListFilterModeChanged)
	Q_PROPERTY(int gameListFilterMode READ gameListFilterMode WRITE setGameListFilterMode NOTIFY gameListFilterModeChanged)
	Q_PROPERTY(int playerListRevision READ playerListRevision NOTIFY playerListRevisionChanged)
	Q_PROPERTY(int gameListRevision READ gameListRevision NOTIFY gameListRevisionChanged)
	Q_PROPERTY(int playerIgnoreListRevision READ playerIgnoreListRevision NOTIFY playerIgnoreListChanged)
	Q_PROPERTY(bool isInGame READ isInGame NOTIFY isInGameChanged)
	// Do we sit at a RUNNING table? Private messages are deliberately blocked
	// there (collusion), and the server does not deliver them anyway.
	Q_PROPERTY(bool atRunningTable READ atRunningTable NOTIFY gameRunningChanged)
	// true if we have joined the current game as a spectator
	// (the eye icon in the lobby). Spectators do not sit at the table.
	Q_PROPERTY(bool isSpectating READ isSpectating NOTIFY isSpectatingChanged)
	Q_PROPERTY(int currentGameId READ currentGameId NOTIFY currentGameIdChanged)
	// A rejoin into a running game offered by the server (InitAck) after a
	// connection loss (0 = no offer). The LobbyPage shows a
	// yes/no popup for it; a property instead of a pure signal, because the offer already
	// arrives at the login - before the LobbyPage is instantiated.
	Q_PROPERTY(int rejoinOfferGameId READ rejoinOfferGameId NOTIFY rejoinOfferChanged)
	// true between an accepted rejoin (server: rejoinEvent) and the beginning
	// of the next hand, at which the server puts us at the table. The
	// waiting room shows a notice of its own for that and locks "leave game"
	// - exactly like the widgets client (waitRejoinStartGameMsgBox).
	Q_PROPERTY(bool rejoinWaiting READ rejoinWaiting NOTIFY rejoinWaitingChanged)
	// The persistent lobby chat history (formatted HTML lines). It allows
	// several pages (lobby + game wait) to show the same chat including its history.
	Q_PROPERTY(QStringList chatLog READ chatLog NOTIFY chatLogChanged)
	// The translator for the lobby chat. The ChatBox routes taps on the globe
	// symbol to chatTranslator.requestTranslation(id).
	Q_PROPERTY(QObject* chatTranslator READ chatTranslator CONSTANT)

public:
	explicit LobbyHandler(QObject *parent = nullptr);
	virtual ~LobbyHandler();

	void setSession(boost::shared_ptr<Session> session);
	void setConfig(ConfigFile *config);
	// The shared SoundEvents instance from main() (not owned) – see
	// GameHandler::setSoundEvents().
	void setSoundEvents(SoundEvents *soundEvents);

	PlayerListModel* playerListModel()
	{
		return &m_playerListModel;
	}
	QAbstractItemModel* playerListProxyModel() const
	{
		return m_playerListProxyModel;
	}
	GameListModel* gameListModel()
	{
		return &m_gameListModel;
	}
	QAbstractItemModel* gameListProxyModel() const
	{
		return m_gameListProxyModel;
	}

	QString myPlayerName() const
	{
		return m_myPlayerName;
	}
	unsigned myPlayerId() const
	{
		return m_myPlayerId;
	}
	QStringList chatLog() const;
	QObject* chatTranslator() const;
	bool isMyPlayerGuest() const;
	// The guest status of an ARBITRARY player. Guests are not allowed to chat at all
	// on the server side – no private message goes to them either.
	Q_INVOKABLE bool isPlayerGuest(unsigned playerId) const;

	// The translator for the bubbles of the private message dialog. Deliberately
	// the same one that the forum page uses (TextTranslator, created in
	// pokerth.cpp) – not the ChatTranslator, which works exclusively on chat
	// LINES.
	void setTextTranslator(TextTranslator *translator);

	// A globe click on a PM bubble: it translates the message or toggles a
	// translation that has already been fetched. The state lives – as with the chat history –
	// in the handler and not in QML: the bubble simply renders anew as soon as
	// privateMessagesChanged() arrives. It is addressed by the msgId of the
	// entry, NOT by the list index (which shifts when the
	// history is trimmed at the front in memory).
	Q_INVOKABLE void togglePrivateMessageTranslation(const QString &playerName, int messageId);
	bool atRunningTable() const
	{
		return m_gameRunning;
	}
	int unreadPrivateMessages() const
	{
		return m_unreadPrivateMessages;
	}
	int privateMessagesRevision() const
	{
		return m_privateMessagesRevision;
	}
	// ── Private message history (the inbox) ────────────────────────────────
	// The conversation partners, the most recent conversation first:
	// { name, unread, lastText, lastTime, fromMe, playerId } (playerId 0 = offline).
	Q_INVOKABLE QVariantList privateConversationPartners() const;
	// The history of a conversation, the oldest message first:
	// { fromMe, text, ts, time } (time = the ready display form of ts).
	Q_INVOKABLE QVariantList privateConversation(const QString &playerName) const;
	// Creates a (possibly still empty) conversation thread – so that the dialog also
	// opens for a player one has never written to.
	Q_INVOKABLE void ensurePrivateConversation(const QString &playerName);
	Q_INVOKABLE void markPrivateConversationRead(const QString &playerName);
	// Removes a conversation from the inbox (and the file) for good.
	Q_INVOKABLE void deletePrivateConversation(const QString &playerName);
	// A reply from the dialog: the player id is fetched freshly from the
	// player list when sending (it only applies to the current session of the partner).
	Q_INVOKABLE void sendPrivateMessageToName(const QString &playerName, const QString &message);
	// The player id for a name (0 = currently not in the lobby).
	Q_INVOKABLE unsigned playerIdByName(const QString &playerName) const;
	bool isCurrentPlayerAdmin() const
	{
		return m_isCurrentPlayerAdmin;
	}
	bool isCurrentGameAdmin() const
	{
		return m_isCurrentGameAdmin;
	}
	bool canInviteFromCurrentGame() const;
	bool isInGame() const
	{
		return m_isInGame;
	}
	bool isSpectating() const
	{
		return m_isSpectating;
	}
	int  currentGameId() const
	{
		return static_cast<int>(m_currentGameId);
	}
	int  rejoinOfferGameId() const
	{
		return static_cast<int>(m_rejoinOfferGameId);
	}
	bool rejoinWaiting() const
	{
		return m_rejoinWaiting;
	}
	Q_INVOKABLE QString currentGameName() const;
	int playerListFilterMode() const
	{
		return m_playerListFilterMode;
	}
	int gameListFilterMode() const
	{
		return m_gameListFilterMode;
	}
	int playerListRevision() const
	{
		return m_playerListRevision;
	}
	int gameListRevision() const
	{
		return m_gameListRevision;
	}
	int playerIgnoreListRevision() const
	{
		return m_playerIgnoreListRevision;
	}
	void setPlayerListFilterMode(int mode);
	void setGameListFilterMode(int mode);

	void setMyPlayerInfo(unsigned playerId, const QString &playerName);
	// Set the current player's game-admin status (e.g. on self-join as host).
	// It concerns ONLY the game admin (host), not the server admin.
	void setCurrentGameAdmin(bool isGameAdmin);

public slots:
	// Player management
	void onLobbyPlayerJoined(unsigned playerId, const QString &playerName);
	void onLobbyPlayerLeft(unsigned playerId);
	void updatePlayerName(unsigned playerId, const QString &playerName, bool isAdmin);

	// Game management
	void onGameListNew(unsigned gameId, const QString &gameName);
	void onGameListRemove(unsigned gameId);
	void onGameListUpdateMode(unsigned gameId, int mode);
	void onGameListChanged(unsigned gameId);

	// Chat
	void sendChatMessage(const QString &message);
	// Appends a line ONLY locally to your own chat history (no network,
	// no broadcast) – for notices that only the triggering user should see,
	// e.g. the community "suggest" result.
	void postLocalChatNote(const QString &message);
	void onLobbyChatMessage(const QString &playerName, const QString &message);
	void onPrivateChatMessage(const QString &playerName, const QString &message);

	// Game invitations (incoming). Called by QmlGuiInterface.
	void onSelfGameInvitation(unsigned gameId, unsigned playerIdFrom);
	void onPlayerGameInvitation(unsigned gameId, unsigned playerIdWho, unsigned playerIdFrom);
	void onRejectedGameInvitation(unsigned gameId, unsigned playerIdWho, int reason);
	// The reply from QML to the invitation popup.
	Q_INVOKABLE void acceptGameInvitation(unsigned gameId);
	Q_INVOKABLE void rejectGameInvitation(unsigned gameId, int reason);

	// Actions from QML
	Q_INVOKABLE void joinGame(unsigned gameId, const QString &password);
	// Attend a running game as a spectator (the eye icon of the lobby).
	// No password needed: with spectateOnly the server checks neither the password
	// nor the invitation, but exclusively GameData::allowSpectators.
	Q_INVOKABLE void spectateGame(unsigned gameId);
	Q_INVOKABLE void leaveGame();
	// Leaves the lobby/the server completely (disconnect). It is called
	// when returning to the start page, so that the client does not
	// stay connected in the background (the lobby chat, pings etc.).
	Q_INVOKABLE void leaveServer();
	void onSelfJoinedGame();
	// A player has joined my current game → a notification
	// sound (playerconnected or onlinegameready if the game is full).
	void onGamePlayerJoined();
	// After a connection loss the server offers to resume the
	// old game session (InitAck.rejoinGameId). Called by QmlGuiInterface;
	// the LobbyPage shows a yes/no popup for it.
	void onRejoinPossible(unsigned gameId);
	// The reply from QML to the rejoin popup.
	Q_INVOKABLE void acceptRejoin();
	Q_INVOKABLE void declineRejoin();
	// Armed by the ServerConnectionHandler during an automatic reconnect:
	// accept the offer of the server without a question then - the player
	// never wanted to leave the table. It is one-shot, the flag falls back
	// after use.
	void setAutoRejoin(bool on);
	// The server has confirmed the rejoin (StartEvent rejoinEvent → SYNCREJOIN):
	// wait for the beginning of the next hand. Called by QmlGuiInterface.
	void onRejoinSyncWait();
	// The AFK timeout warning of the server (in the lobby as well as in-game) → a QML popup + a beep.
	void onTimeoutWarning(int reason, int remainingSec);
	// A server message (plain text or a msgId from socket_msg.h) → a QML info popup.
	void onNetworkMessage(const QString &message);
	void onNetworkMessageId(unsigned msgId);
	// A server notification (notificationId = NTF_NET_* from socket_msg.h),
	// among others when joining/creating a game fails (e.g. the
	// game name is already taken) → a QML info popup.
	void onNetworkNotification(int notificationId);
	void onGameStarted();
	// reason = NTF_NET_REMOVED_* (socket_msg.h); it is passed on to QML,
	// so that leaving on your own request (ON_REQUEST) navigates differently
	// from e.g. a closed/finished game (GAME_CLOSED).
	void onRemovedFromGame(int reason);
	// The engine requests leaving the game table (the end of the game or a removal):
	// close the game table and return into the waiting room of the (possibly reopened)
	// game. With auto-leave, onRemovedFromGame follows and pops
	// on into the lobby list.
	void onWaitGameDialog();

	// Player actions (QML-invokable)
	// manualBlinds: only relevant with raiseMode == MANUAL_BLINDS_ORDER (2);
	// a fixed blind list, e.g. for the community templates (BBC steps).
	Q_INVOKABLE void createGame(const QString &name, const QString &password,
								int gameType, bool allowSpectators, int maxPlayers,
								int startCash, int firstSmallBlind,
								int raiseIntervalMode, int raiseEveryHands,
								int raiseEveryMinutes, int raiseMode,
								int playerActionTimeout, int delayBetweenHands,
								const QVariantList &manualBlinds = QVariantList());
	Q_INVOKABLE void kickPlayer(unsigned playerId);
	Q_INVOKABLE void invitePlayer(unsigned playerId);
	Q_INVOKABLE bool isPlayerInAnyGame(unsigned playerId) const;
	// Does the player sit at a RUNNING table? The server discards private
	// messages to such players (HandleNetPacketChatRequest) – the UI
	// therefore hides the PM action instead of pretending a delivery.
	Q_INVOKABLE bool isPlayerInRunningGame(unsigned playerId) const;
	Q_INVOKABLE QString playerInGameName(unsigned playerId) const;
	Q_INVOKABLE void adminBanPlayer(unsigned playerId);
	// A server-wide announcement (server admins only). The server checks the
	// permission itself; the UI only hides the action in addition.
	Q_INVOKABLE void adminSendGlobalNotice(const QString &noticeText);
	Q_INVOKABLE void reportGameName(unsigned gameId);
	Q_INVOKABLE void adminCloseGame(unsigned gameId);
	Q_INVOKABLE void sendPrivateMessage(unsigned targetPlayerId, const QString &message);
	Q_INVOKABLE QVariantMap playerListEntry(int row) const;
	// All connected player names (UNfiltered) for the chat Tab
	// completion. Unlike playerListEntry() this reads the
	// source model, so that players can be completed as well who
	// currently sit in an (open) game and are hidden by the player list filter
	// (mode 2) – analogous to the Qt widgets client.
	Q_INVOKABLE QStringList playerNickList() const;
	// The names of the currently idle (not sitting at a table) non-guest players
	// in the lobby – the same predicate as the idle filter of the player list
	// (getGameIdOfPlayer == 0). The basis for the community "suggest" feature.
	Q_INVOKABLE QStringList idlePlayerNames() const;
	// The counterpart to idlePlayerNames: non-guest players who currently sit at a
	// table – each as { name, game } (the table name). For "suggest" they are,
	// if they are in the DB/WEC list, appended at the end and annotated with the game name.
	Q_INVOKABLE QVariantList playingPlayerEntries() const;
	Q_INVOKABLE QString playerCountryByName(const QString &name) const;
	Q_INVOKABLE QVariantList gamePlayersInGame(unsigned gameId) const;
	Q_INVOKABLE bool canJoinGame(unsigned gameId) const;
	Q_INVOKABLE bool canSpectateGame(unsigned gameId) const;
	Q_INVOKABLE bool openExternalUrl(const QString &url) const;
	// All supported emoji shortcodes (":smile:" → 😄) as a sorted list
	// of {code, emoji} for the auto-completion of the ChatBox. The source is
	// the same map that replaces them when sending (chat_emote_shortcuts.h) – so
	// only what really works is offered.
	Q_INVOKABLE QVariantList chatEmoteShortcodes() const;
	Q_INVOKABLE bool isPlayerIgnored(unsigned playerId) const;
	Q_INVOKABLE void ignorePlayer(unsigned playerId);
	Q_INVOKABLE void unignorePlayer(unsigned playerId);
	Q_INVOKABLE void showPlayerStats(unsigned playerId);
	Q_INVOKABLE QString gameTypeText(int gameType) const;
	Q_INVOKABLE QString gameStatusText(int gameMode, int playerCount, int maxPlayers) const;
	Q_INVOKABLE void startGame(bool fillWithCpu = false);
	Q_INVOKABLE QVariantMap currentGameInfo() const;
	// Stop the countdown of the AFK timeout popup (like timeoutMsgBoxImpl::stopTimeout).
	Q_INVOKABLE void resetNetworkTimeout();
	// Called from QML when the light/dark mode has been toggled: the
	// history only carries colour placeholders (chatcolors.h), so it is enough to let the
	// chatLog binding be re-evaluated – the history is thereby delivered including the
	// translations and globe anchors in the new colours.
	Q_INVOKABLE void refreshChatColors()
	{
		notifyChatLogChanged();
	}

signals:
	void chatLineReady(const QString &formattedLine);
	void chatLogChanged();
	void lobbyChatMentionDetected();
	void unreadPrivateMessagesChanged();
	void privateMessagesChanged();
	void timeoutWarningReceived(int reason, int remainingSec);
	void networkMessageReceived(QString message);
	// An incoming game invitation → QML shows a yes/no popup.
	void gameInvitationReceived(int gameId, const QString &gameName, const QString &fromName);
	void gameCreated(unsigned gameId);
	void gameJoined(unsigned gameId);
	void selfJoinedGame();
	void gameStarted();
	void removedFromGame(int reason);
	// Close the game table and go back into the waiting room (see onWaitGameDialog).
	void returnToWaitRoom();
	void errorOccurred(const QString &errorMessage);
	void myPlayerNameChanged();
	void myPlayerIdChanged();
	void isCurrentPlayerAdminChanged();
	void isCurrentGameAdminChanged();
	void gameContextChanged();
	void playerListFilterModeChanged();
	void gameListFilterModeChanged();
	void playerListRevisionChanged();
	void gameListRevisionChanged();
	void playerIgnoreListChanged();
	// "Show player stats" (the lobby icon / the table context menu): QML shows the
	// native player page of the player.
	void playerStatsRequested(const QString &playerName);
	void isInGameChanged();
	void gameRunningChanged();
	void isSpectatingChanged();
	void currentGameIdChanged();
	void rejoinOfferChanged();
	void rejoinWaitingChanged();

private:
	// Appends a fully formatted chat line to the history (limited) and
	// notifies both the live consumers (chatLineReady) and the
	// bindable chatLog property.
	void pushChatLine(const QString &line);
	// The only place that reports changes to the chat history. It bundles several
	// changes of the same event loop pass into ONE chatLogChanged():
	// every notification delivers the complete history (up to 400 lines) anew
	// to QML, where every ChatBox rebuilds its complete rich text document from it.
	// A multi-line notice (postLocalChatNote, e.g. the
	// community suggestion with 13 lines) triggered that 13 times otherwise, and
	// moving the translate symbol twice per line the mouse passes over – and
	// each of these rebuilds gets more expensive as the history grows.
	void notifyChatLogChanged();
	// The current display mode for the chat colours (config "DarkMode", 0 = light).
	bool chatDarkMode() const;
	// The confirmation of a SENT private message in your own chat history –
	// with the full message text, because a PM would otherwise be missing from the history without a trace.
	// It is used by both sending paths (the chat shortcut /msg and the PM dialog).
	void pushPrivateMessageSentLine(const QString &targetName, const QString &message);
	// Enters a (sent or received) private message into the history of the
	// conversation partner and reports the change to the user interface.
	void appendPrivateMessage(const QString &playerName, const QString &message, bool fromMe);
	void recountUnreadPrivateMessages();
	// The only place that writes m_gameRunning – it reports the change to QML.
	void setGameRunning(bool running);
	// Save the inbox between sessions – a SQLite file of its own next to the
	// config.xml (privatemessages.sqlite). A database of its own, so that long
	// conversations touch neither the settings nor the game logs.
	void openPrivateMessageDb();
	// Changes the owner of the inbox (your own nick). The history is
	// kept separate per account: after a login with a different user, neither
	// their history may be visible nor may it be continued. An empty
	// name = nobody is logged in (the inbox is empty, nothing is stored).
	void setPrivateMessageOwner(const QString &owner);
	// Reads the history of the current owner from the database.
	void loadPrivateMessages();
	void persistPrivateMessage(const QString &playerName, const QVariantMap &entry);
	void persistPrivateThreadMeta(const QString &playerName);
	void persistDeletePrivateThread(const QString &playerName);
	// The reply of the translator for a PM bubble (it ignores foreign request ids,
	// the TextTranslator serves the forum page as well).
	void onPrivateMessageTranslated(int requestId, const QString &text, bool ok);

	boost::shared_ptr<Session> m_session;
	SoundEvents *m_soundEvents = nullptr;   // not owned
	ConfigFile *m_config;

	PlayerListModel m_playerListModel;
	QSortFilterProxyModel *m_playerListProxyModel;
	GameListModel m_gameListModel;
	QSortFilterProxyModel *m_gameListProxyModel;

	QString m_myPlayerName;
	unsigned m_myPlayerId;
	int m_unreadPrivateMessages = 0;
	int m_privateMessagesRevision = 0;
	// A running number per message, only for this session. It addresses a
	// bubble unambiguously – the timestamp is NOT suitable for that (a second's resolution,
	// two messages of the same second are indistinguishable) and the
	// list index shifts when the history is trimmed.
	int m_nextPrivateMessageId = 1;
	// The private message history per conversation partner: the window of the
	// conversation shown in the dialog (the SQLite file holds the complete history).
	struct PrivateThread {
		QVariantList messages;      // { msgId, fromMe, text, ts, ggf. translation* }
		int unread = 0;
		qint64 lastActivity = 0;    // ms since epoch, for the sorting
	};
	QHash<QString, PrivateThread> m_privateThreads;
	// The connection name of the PM database; empty = no database (then the
	// history only lives until the program ends).
	QString m_privateDbConn;
	// The account the loaded inbox belongs to (your own nick). Empty while
	// nobody is logged in – then the history stays empty.
	QString m_privateMessagesOwner;
	// The formatted lobby chat history (HTML lines). The text colours are in it
	// ONLY as role placeholders (chatcolors.h) and are only resolved in chatLog()
	// to the current light/dark mode.
	QStringList m_chatLog;
	// Is a bundled chatLogChanged notification already running? (notifyChatLogChanged)
	bool m_chatLogNotifyPending = false;
	ChatTranslator *m_chatTranslator = nullptr; // it appends the translate symbols and translates them
	TextTranslator *m_textTranslator = nullptr; // it translates individual PM bubbles
	// Running PM translations: request id -> (conversation partner, msgId).
	QHash<int, QPair<QString, int>> m_pmTranslationRequests;
	// The invitation currently asked about in the QML popup (0 = none). It prevents
	// several invitation popups from appearing at the same time (further ones → "busy").
	unsigned m_pendingInviteGameId = 0;
	// A rejoin offered by the server after a connection loss (0 = none).
	unsigned m_rejoinOfferGameId = 0;
	bool m_autoRejoin = false;
	// true while we wait for the next hand after an accepted rejoin.
	bool m_rejoinWaiting = false;
	void setRejoinWaiting(bool waiting);
	bool m_isCurrentPlayerAdmin = false;   // Server admin (kickban / close a game)
	bool m_isCurrentGameAdmin = false;     // Game admin (the host of the current table)
	bool m_isInGame = false;
	// true between joining as a spectator and leaving the table. It is taken from the
	// JoinGameAck of the server (Session::isClientSpectating).
	bool m_isSpectating = false;
	// true between the game start and the return into the waiting room/the lobby. The
	// join sounds (playerconnected/onlinegameready) belong only into the
	// waiting room; a PlayerJoined in a running game is a rejoin after a
	// disconnect (widgets client: the isVisible() guard in the GameLobbyDialog).
	bool m_gameRunning = false;
	unsigned m_currentGameId = 0;
	int m_playerListFilterMode;
	int m_gameListFilterMode;
	int m_playerListRevision;
	int m_gameListRevision;
	int m_playerIgnoreListRevision;

	void refreshGameInfo(unsigned gameId);
	// Transfers the table membership of all players from the session into the
	// player list model (GameIdRole). The only place where the idle criterion
	// is maintained; to be called on every event that can change it.
	void syncPlayerGameMembership();
	QString resolvedPlayerName(unsigned playerId) const;
	unsigned parsePrivateMessageTarget(QString &chatText) const;
};

#endif // LOBBYHANDLER_H
