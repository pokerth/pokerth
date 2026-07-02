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
#include <QVariantMap>
#include <boost/shared_ptr.hpp>

class Session;
class SoundEvents;
class ConfigFile;
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
        IsGuestRole
    };

    explicit PlayerListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return m_players.count(); }

    void addPlayer(unsigned playerId, const QString &playerName, bool isAdmin = false, const QString &countryCode = QString(), bool isGuest = false);
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

    int runningCount() const { return m_runningCount; }
    int openCount() const { return m_openCount; }

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
    // Server-Admin (darf kickban / Spiele schließen) – authoritativ aus der
    // PlayerInfo der Session. Strikt getrennt vom Spiel-Admin (Host).
    Q_PROPERTY(bool isCurrentPlayerAdmin READ isCurrentPlayerAdmin NOTIFY isCurrentPlayerAdminChanged)
    // Spiel-Admin (Host/Ersteller des aktuellen Tisches) – darf das Spiel starten.
    Q_PROPERTY(bool isCurrentGameAdmin READ isCurrentGameAdmin NOTIFY isCurrentGameAdminChanged)
    Q_PROPERTY(bool canInviteFromCurrentGame READ canInviteFromCurrentGame NOTIFY gameContextChanged)
    Q_PROPERTY(int playerListFilterMode READ playerListFilterMode WRITE setPlayerListFilterMode NOTIFY playerListFilterModeChanged)
    Q_PROPERTY(int gameListFilterMode READ gameListFilterMode WRITE setGameListFilterMode NOTIFY gameListFilterModeChanged)
    Q_PROPERTY(int playerListRevision READ playerListRevision NOTIFY playerListRevisionChanged)
    Q_PROPERTY(int gameListRevision READ gameListRevision NOTIFY gameListRevisionChanged)
    Q_PROPERTY(int playerIgnoreListRevision READ playerIgnoreListRevision NOTIFY playerIgnoreListChanged)
    Q_PROPERTY(bool isInGame READ isInGame NOTIFY isInGameChanged)
    Q_PROPERTY(int currentGameId READ currentGameId NOTIFY currentGameIdChanged)
    // Persistenter Lobby-Chat-Verlauf (formatierte HTML-Zeilen). Erlaubt es
    // mehreren Seiten (Lobby + GameWait), denselben Chat inkl. History zu zeigen.
    Q_PROPERTY(QStringList chatLog READ chatLog NOTIFY chatLogChanged)

public:
    explicit LobbyHandler(QObject *parent = nullptr);
    virtual ~LobbyHandler();

    void setSession(boost::shared_ptr<Session> session);
    void setConfig(ConfigFile *config);

    PlayerListModel* playerListModel() { return &m_playerListModel; }
    QAbstractItemModel* playerListProxyModel() const { return m_playerListProxyModel; }
    GameListModel* gameListModel() { return &m_gameListModel; }
    QAbstractItemModel* gameListProxyModel() const { return m_gameListProxyModel; }
    
    QString myPlayerName() const { return m_myPlayerName; }
    unsigned myPlayerId() const { return m_myPlayerId; }
    QStringList chatLog() const { return m_chatLog; }
    bool isMyPlayerGuest() const;
    bool isCurrentPlayerAdmin() const { return m_isCurrentPlayerAdmin; }
    bool isCurrentGameAdmin() const { return m_isCurrentGameAdmin; }
    bool canInviteFromCurrentGame() const;
    bool isInGame() const { return m_isInGame; }
    int  currentGameId() const { return static_cast<int>(m_currentGameId); }
    Q_INVOKABLE QString currentGameName() const;
    int playerListFilterMode() const { return m_playerListFilterMode; }
    int gameListFilterMode() const { return m_gameListFilterMode; }
    int playerListRevision() const { return m_playerListRevision; }
    int gameListRevision() const { return m_gameListRevision; }
    int playerIgnoreListRevision() const { return m_playerIgnoreListRevision; }
    void setPlayerListFilterMode(int mode);
    void setGameListFilterMode(int mode);
    
    void setMyPlayerInfo(unsigned playerId, const QString &playerName);
    // Set the current player's game-admin status (e.g. on self-join as host).
    // Betrifft NUR den Spiel-Admin (Host), nicht den Server-Admin.
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
    void onLobbyChatMessage(const QString &playerName, const QString &message);
    void onPrivateChatMessage(const QString &playerName, const QString &message);

    // Spiel-Einladungen (eingehend). Von QmlGuiInterface aufgerufen.
    void onSelfGameInvitation(unsigned gameId, unsigned playerIdFrom);
    void onPlayerGameInvitation(unsigned gameId, unsigned playerIdWho, unsigned playerIdFrom);
    void onRejectedGameInvitation(unsigned gameId, unsigned playerIdWho, int reason);
    // Antwort aus QML auf das Einladungs-Popup.
    Q_INVOKABLE void acceptGameInvitation(unsigned gameId);
    Q_INVOKABLE void rejectGameInvitation(unsigned gameId, int reason);
    
    // Actions from QML
    Q_INVOKABLE void joinGame(unsigned gameId, const QString &password);
    Q_INVOKABLE void leaveGame();
    // Verlässt die Lobby/den Server vollständig (Verbindung trennen). Wird
    // beim Zurückkehren zur Startseite aufgerufen, damit der Client nicht
    // weiterhin im Hintergrund verbunden bleibt (Lobby-Chat, Pings etc.).
    Q_INVOKABLE void leaveServer();
    void onSelfJoinedGame();
    // Ein Spieler ist meinem aktuellen Spiel beigetreten → Benachrichtigungs-
    // Sound (playerconnected bzw. onlinegameready, wenn das Spiel voll ist).
    void onGamePlayerJoined();
    // AFK-Timeout-Warnung des Servers (Lobby wie ingame) → QML-Popup + Beep.
    void onTimeoutWarning(int reason, int remainingSec);
    // Server-Meldung (Klartext bzw. msgId aus socket_msg.h) → QML-Info-Popup.
    void onNetworkMessage(const QString &message);
    void onNetworkMessageId(unsigned msgId);
    // Server-Benachrichtigung (notificationId = NTF_NET_* aus socket_msg.h),
    // u. a. wenn das Beitreten/Erstellen eines Spiels fehlschlägt (z. B. der
    // Spielname ist bereits vergeben) → QML-Info-Popup.
    void onNetworkNotification(int notificationId);
    void onGameStarted();
    // reason = NTF_NET_REMOVED_* (socket_msg.h); wird an QML weitergereicht,
    // damit ein selbst angefordertes Verlassen (ON_REQUEST) anders navigiert
    // als z.B. ein geschlossenes/beendetes Spiel (GAME_CLOSED).
    void onRemovedFromGame(int reason);
    // Engine fordert das Verlassen des Gametables an (Spielende bzw. Entfernung):
    // den Gametable schließen und in den Warteraum des (ggf. wieder geöffneten)
    // Spiels zurückkehren. Bei Auto-Leave folgt onRemovedFromGame und poppt
    // weiter bis in die Lobbyliste.
    void onWaitGameDialog();

    // Player actions (QML-invokable)
    Q_INVOKABLE void createGame(const QString &name, const QString &password,
                               int gameType, bool allowSpectators, int maxPlayers,
                               int startCash, int firstSmallBlind,
                               int raiseIntervalMode, int raiseEveryHands,
                               int raiseEveryMinutes, int raiseMode,
                               int playerActionTimeout, int delayBetweenHands);
    Q_INVOKABLE void kickPlayer(unsigned playerId);
    Q_INVOKABLE void invitePlayer(unsigned playerId);
    Q_INVOKABLE bool isPlayerInAnyGame(unsigned playerId) const;
    Q_INVOKABLE void adminBanPlayer(unsigned playerId);
    Q_INVOKABLE void reportGameName(unsigned gameId);
    Q_INVOKABLE void adminCloseGame(unsigned gameId);
    Q_INVOKABLE void sendPrivateMessage(unsigned targetPlayerId, const QString &message);
    Q_INVOKABLE QVariantMap playerListEntry(int row) const;
    // Alle verbundenen Spielernamen (UNgefiltert) für die Chat-Tab-
    // Vervollständigung. Anders als playerListEntry() liest dies das
    // Quell-Modell, damit auch Spieler vervollständigt werden können, die
    // gerade in einem (offenen) Spiel sitzen und vom Spielerlisten-Filter
    // (Modus 2) ausgeblendet werden – analog zum Qt-Widgets-Client.
    Q_INVOKABLE QStringList playerNickList() const;
    Q_INVOKABLE QString playerCountryByName(const QString &name) const;
    Q_INVOKABLE QVariantList gamePlayersInGame(unsigned gameId) const;
    Q_INVOKABLE bool canJoinGame(unsigned gameId) const;
    Q_INVOKABLE bool openExternalUrl(const QString &url) const;
    Q_INVOKABLE bool isPlayerIgnored(unsigned playerId) const;
    Q_INVOKABLE void ignorePlayer(unsigned playerId);
    Q_INVOKABLE void unignorePlayer(unsigned playerId);
    Q_INVOKABLE void showPlayerStats(unsigned playerId);
    // URL zur Tisch-Statistikübersicht (alle aktiven Spieler) des laufenden
    // Ranglistenspiels – wie im Qt-Widgets-Client (MyNameLabel, tableview=1).
    // Leer, wenn kein Ranglistenspiel läuft (dann keine Übersicht verfügbar).
    Q_INVOKABLE QString currentTableStatsUrl() const;
    Q_INVOKABLE QString gameTypeText(int gameType) const;
    Q_INVOKABLE QString gameStatusText(int gameMode, int playerCount, int maxPlayers) const;
    Q_INVOKABLE void startGame(bool fillWithCpu = false);
    Q_INVOKABLE QVariantMap currentGameInfo() const;
    // Countdown des AFK-Timeout-Popups stoppen (wie timeoutMsgBoxImpl::stopTimeout).
    Q_INVOKABLE void resetNetworkTimeout();

signals:
    void chatLineReady(const QString &formattedLine);
    void chatLogChanged();
    void lobbyChatMentionDetected();
    void timeoutWarningReceived(int reason, int remainingSec);
    void networkMessageReceived(QString message);
    // Eingehende Spiel-Einladung → QML zeigt ein Ja/Nein-Popup.
    void gameInvitationReceived(int gameId, const QString &gameName, const QString &fromName);
    void gameCreated(unsigned gameId);
    void gameJoined(unsigned gameId);
    void selfJoinedGame();
    void gameStarted();
    void removedFromGame(int reason);
    // Gametable schließen und zurück in den Warteraum (siehe onWaitGameDialog).
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
    void isInGameChanged();
    void currentGameIdChanged();

private:
    // Hängt eine fertig formatierte Chat-Zeile an den Verlauf an (begrenzt) und
    // benachrichtigt sowohl die Live-Verbraucher (chatLineReady) als auch die
    // bindbare chatLog-Property.
    void pushChatLine(const QString &line);

    boost::shared_ptr<Session> m_session;
    SoundEvents *m_soundEvents = nullptr;
    ConfigFile *m_config;
    
    PlayerListModel m_playerListModel;
    QSortFilterProxyModel *m_playerListProxyModel;
    GameListModel m_gameListModel;
    QSortFilterProxyModel *m_gameListProxyModel;
    
    QString m_myPlayerName;
    unsigned m_myPlayerId;
    QStringList m_chatLog;      // formatierter Lobby-Chat-Verlauf (HTML-Zeilen)
    // Aktuell im QML-Popup angefragte Einladung (0 = keine). Verhindert, dass
    // mehrere Einladungs-Popups gleichzeitig erscheinen (weitere → "busy").
    unsigned m_pendingInviteGameId = 0;
    bool m_isCurrentPlayerAdmin = false;   // Server-Admin (kickban / Spiel schließen)
    bool m_isCurrentGameAdmin = false;     // Spiel-Admin (Host des aktuellen Tisches)
    bool m_isInGame = false;
    unsigned m_currentGameId = 0;
    int m_playerListFilterMode;
    int m_gameListFilterMode;
    int m_playerListRevision;
    int m_gameListRevision;
    int m_playerIgnoreListRevision;

    void refreshGameInfo(unsigned gameId);
    QString resolvedPlayerName(unsigned playerId) const;
    unsigned parsePrivateMessageTarget(QString &chatText) const;
};

#endif // LOBBYHANDLER_H
