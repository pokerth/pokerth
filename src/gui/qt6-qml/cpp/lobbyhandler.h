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
    // Ungelesene private Nachrichten (Zähler am Posteingang-Symbol der
    // Kopfzeile). Sinkt, sobald ein Gespräch im Dialog gelesen wird
    // (markPrivateConversationRead).
    Q_PROPERTY(int unreadPrivateMessages READ unreadPrivateMessages NOTIFY unreadPrivateMessagesChanged)
    // Zähler-Property als reaktive Abhängigkeit für QML: der Verlauf selbst wird
    // über Q_INVOKABLE-Funktionen gelesen (wie bei der Spielerliste).
    Q_PROPERTY(int privateMessagesRevision READ privateMessagesRevision NOTIFY privateMessagesChanged)
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
    // Sitzen wir an einem LAUFENDEN Tisch? Private Nachrichten sind dort
    // bewusst gesperrt (Absprachen), und der Server stellt sie ohnehin nicht zu.
    Q_PROPERTY(bool atRunningTable READ atRunningTable NOTIFY gameRunningChanged)
    // true, wenn wir dem aktuellen Spiel als Zuschauer beigetreten sind
    // (Auge-Icon in der Lobby). Zuschauer sitzen nicht am Tisch.
    Q_PROPERTY(bool isSpectating READ isSpectating NOTIFY isSpectatingChanged)
    Q_PROPERTY(int currentGameId READ currentGameId NOTIFY currentGameIdChanged)
    // Vom Server (InitAck) angebotenes Rejoin in ein laufendes Spiel nach
    // Verbindungsabbruch (0 = kein Angebot). Die LobbyPage zeigt dazu ein
    // Ja/Nein-Popup; als Property statt reinem Signal, weil das Angebot schon
    // beim Login eintrifft - bevor die LobbyPage instanziiert ist.
    Q_PROPERTY(int rejoinOfferGameId READ rejoinOfferGameId NOTIFY rejoinOfferChanged)
    // true zwischen angenommenem Rejoin (Server: rejoinEvent) und dem Beginn
    // der nächsten Hand, an dem uns der Server an den Tisch setzt. Der
    // Warteraum zeigt dafür einen eigenen Hinweistext und sperrt "Leave Game"
    // - genau wie der Widgets-Client (waitRejoinStartGameMsgBox).
    Q_PROPERTY(bool rejoinWaiting READ rejoinWaiting NOTIFY rejoinWaitingChanged)
    // Persistenter Lobby-Chat-Verlauf (formatierte HTML-Zeilen). Erlaubt es
    // mehreren Seiten (Lobby + GameWait), denselben Chat inkl. History zu zeigen.
    Q_PROPERTY(QStringList chatLog READ chatLog NOTIFY chatLogChanged)
    // Übersetzer für den Lobby-Chat. Die ChatBox routet Taps auf das Globus-
    // Symbol an chatTranslator.requestTranslation(id).
    Q_PROPERTY(QObject* chatTranslator READ chatTranslator CONSTANT)

public:
    explicit LobbyHandler(QObject *parent = nullptr);
    virtual ~LobbyHandler();

    void setSession(boost::shared_ptr<Session> session);
    void setConfig(ConfigFile *config);
    // Gemeinsame SoundEvents-Instanz aus main() (nicht besessen) – siehe
    // GameHandler::setSoundEvents().
    void setSoundEvents(SoundEvents *soundEvents);

    PlayerListModel* playerListModel() { return &m_playerListModel; }
    QAbstractItemModel* playerListProxyModel() const { return m_playerListProxyModel; }
    GameListModel* gameListModel() { return &m_gameListModel; }
    QAbstractItemModel* gameListProxyModel() const { return m_gameListProxyModel; }
    
    QString myPlayerName() const { return m_myPlayerName; }
    unsigned myPlayerId() const { return m_myPlayerId; }
    QStringList chatLog() const;
    QObject* chatTranslator() const;
    bool isMyPlayerGuest() const;
    // Gast-Status eines BELIEBIGEN Spielers. Gäste dürfen serverseitig gar nicht
    // chatten – an sie geht auch keine private Nachricht.
    Q_INVOKABLE bool isPlayerGuest(unsigned playerId) const;

    // Übersetzer für die Blasen des privaten Nachrichten-Dialogs. Bewusst
    // derselbe, den auch die Forum-Seite nutzt (TextTranslator, in pokerth.cpp
    // erzeugt) – nicht der ChatTranslator, der ausschließlich auf Chat-ZEILEN
    // arbeitet.
    void setTextTranslator(TextTranslator *translator);

    // Globus-Klick an einer PM-Blase: übersetzt die Nachricht bzw. blendet eine
    // bereits geholte Übersetzung um. Der Zustand liegt – wie beim Chat-Verlauf –
    // im Handler und nicht im QML: die Blase rendert einfach neu, sobald
    // privateMessagesChanged() kommt. Adressiert wird über die msgId des
    // Eintrags, NICHT über den Listenindex (der verschiebt sich, wenn der
    // Verlauf im Speicher vorne beschnitten wird).
    Q_INVOKABLE void togglePrivateMessageTranslation(const QString &playerName, int messageId);
    bool atRunningTable() const { return m_gameRunning; }
    int unreadPrivateMessages() const { return m_unreadPrivateMessages; }
    int privateMessagesRevision() const { return m_privateMessagesRevision; }
    // ── Privater Nachrichtenverlauf (Posteingang) ──────────────────────────
    // Gesprächspartner, neueste Unterhaltung zuerst:
    // { name, unread, lastText, lastTime, fromMe, playerId } (playerId 0 = offline).
    Q_INVOKABLE QVariantList privateConversationPartners() const;
    // Verlauf einer Unterhaltung, älteste Nachricht zuerst:
    // { fromMe, text, ts, time } (time = fertige Anzeigeform von ts).
    Q_INVOKABLE QVariantList privateConversation(const QString &playerName) const;
    // Legt einen (ggf. noch leeren) Gesprächsfaden an – damit der Dialog auch
    // für einen Spieler öffnet, mit dem noch nie geschrieben wurde.
    Q_INVOKABLE void ensurePrivateConversation(const QString &playerName);
    Q_INVOKABLE void markPrivateConversationRead(const QString &playerName);
    // Eine Unterhaltung endgültig aus dem Posteingang (und der Datei) entfernen.
    Q_INVOKABLE void deletePrivateConversation(const QString &playerName);
    // Antwort aus dem Dialog: die Spieler-Id wird beim Senden frisch aus der
    // Spielerliste geholt (sie gilt nur für die aktuelle Sitzung des Partners).
    Q_INVOKABLE void sendPrivateMessageToName(const QString &playerName, const QString &message);
    // Spieler-Id zum Namen (0 = gerade nicht in der Lobby).
    Q_INVOKABLE unsigned playerIdByName(const QString &playerName) const;
    bool isCurrentPlayerAdmin() const { return m_isCurrentPlayerAdmin; }
    bool isCurrentGameAdmin() const { return m_isCurrentGameAdmin; }
    bool canInviteFromCurrentGame() const;
    bool isInGame() const { return m_isInGame; }
    bool isSpectating() const { return m_isSpectating; }
    int  currentGameId() const { return static_cast<int>(m_currentGameId); }
    int  rejoinOfferGameId() const { return static_cast<int>(m_rejoinOfferGameId); }
    bool rejoinWaiting() const { return m_rejoinWaiting; }
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
    // Hängt eine Zeile NUR lokal an den eigenen Chat-Verlauf an (kein Netzwerk,
    // kein Broadcast) – für Hinweise, die nur der auslösende Nutzer sehen soll,
    // z. B. das Community-„Suggest"-Ergebnis.
    void postLocalChatNote(const QString &message);
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
    // Einem laufenden Spiel als Zuschauer beiwohnen (Auge-Icon der Lobby).
    // Kein Passwort nötig: der Server prüft bei spectateOnly weder Passwort
    // noch Einladung, sondern ausschließlich GameData::allowSpectators.
    Q_INVOKABLE void spectateGame(unsigned gameId);
    Q_INVOKABLE void leaveGame();
    // Verlässt die Lobby/den Server vollständig (Verbindung trennen). Wird
    // beim Zurückkehren zur Startseite aufgerufen, damit der Client nicht
    // weiterhin im Hintergrund verbunden bleibt (Lobby-Chat, Pings etc.).
    Q_INVOKABLE void leaveServer();
    void onSelfJoinedGame();
    // Ein Spieler ist meinem aktuellen Spiel beigetreten → Benachrichtigungs-
    // Sound (playerconnected bzw. onlinegameready, wenn das Spiel voll ist).
    void onGamePlayerJoined();
    // Server bietet nach einem Verbindungsabbruch das Wiederaufnehmen der
    // alten Spielsitzung an (InitAck.rejoinGameId). Von QmlGuiInterface
    // aufgerufen; die LobbyPage zeigt dazu ein Ja/Nein-Popup.
    void onRejoinPossible(unsigned gameId);
    // Antwort aus QML auf das Rejoin-Popup.
    Q_INVOKABLE void acceptRejoin();
    Q_INVOKABLE void declineRejoin();
    // Vom ServerConnectionHandler während einer automatischen Wiederverbindung
    // scharf geschaltet: Das Angebot des Servers dann ohne Rückfrage annehmen -
    // der Spieler wollte den Tisch ja nie verlassen. Einmalig, das Flag fällt
    // nach Gebrauch zurück.
    void setAutoRejoin(bool on);
    // Server hat den Rejoin bestätigt (StartEvent rejoinEvent → SYNCREJOIN):
    // Warten auf den Beginn der nächsten Hand. Von QmlGuiInterface aufgerufen.
    void onRejoinSyncWait();
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
    // manualBlinds: nur relevant bei raiseMode == MANUAL_BLINDS_ORDER (2);
    // feste Blindliste, z.B. für die Community-Vorlagen (BBC Steps).
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
    // Sitzt der Spieler an einem LAUFENDEN Tisch? Der Server verwirft private
    // Nachrichten an solche Spieler (HandleNetPacketChatRequest) – die UI
    // blendet die PM-Aktion deshalb aus, statt eine Zustellung vorzutäuschen.
    Q_INVOKABLE bool isPlayerInRunningGame(unsigned playerId) const;
    Q_INVOKABLE QString playerInGameName(unsigned playerId) const;
    Q_INVOKABLE void adminBanPlayer(unsigned playerId);
    // Server-weite Durchsage (nur Server-Admins). Der Server prüft die
    // Berechtigung selbst; die UI blendet die Aktion nur zusätzlich aus.
    Q_INVOKABLE void adminSendGlobalNotice(const QString &noticeText);
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
    // Namen der aktuell idle (nicht an einem Tisch sitzenden) Nicht-Gast-Spieler
    // in der Lobby – dasselbe Prädikat wie der Idle-Filter der Spielerliste
    // (getGameIdOfPlayer == 0). Basis für das Community-„Suggest"-Feature.
    Q_INVOKABLE QStringList idlePlayerNames() const;
    // Gegenstück zu idlePlayerNames: Nicht-Gast-Spieler, die gerade an einem
    // Tisch sitzen – je als { name, game } (Tischname). Für „Suggest" werden sie,
    // sofern in DB/WEC-Liste, ans Ende gehängt und mit dem Spielnamen annotiert.
    Q_INVOKABLE QVariantList playingPlayerEntries() const;
    Q_INVOKABLE QString playerCountryByName(const QString &name) const;
    Q_INVOKABLE QVariantList gamePlayersInGame(unsigned gameId) const;
    Q_INVOKABLE bool canJoinGame(unsigned gameId) const;
    Q_INVOKABLE bool canSpectateGame(unsigned gameId) const;
    Q_INVOKABLE bool openExternalUrl(const QString &url) const;
    // Alle unterstützten Emoji-Shortcodes (":smile:" → 😄) als sortierte Liste
    // von {code, emoji} für die Autovervollständigung der ChatBox. Quelle ist
    // dieselbe Map, die beim Senden ersetzt (chat_emote_shortcuts.h) – es wird
    // also nur angeboten, was auch wirklich funktioniert.
    Q_INVOKABLE QVariantList chatEmoteShortcodes() const;
    Q_INVOKABLE bool isPlayerIgnored(unsigned playerId) const;
    Q_INVOKABLE void ignorePlayer(unsigned playerId);
    Q_INVOKABLE void unignorePlayer(unsigned playerId);
    Q_INVOKABLE void showPlayerStats(unsigned playerId);
    Q_INVOKABLE QString gameTypeText(int gameType) const;
    Q_INVOKABLE QString gameStatusText(int gameMode, int playerCount, int maxPlayers) const;
    Q_INVOKABLE void startGame(bool fillWithCpu = false);
    Q_INVOKABLE QVariantMap currentGameInfo() const;
    // Countdown des AFK-Timeout-Popups stoppen (wie timeoutMsgBoxImpl::stopTimeout).
    Q_INVOKABLE void resetNetworkTimeout();
    // Vom QML aufgerufen, wenn der Hell/Dunkel-Modus umgeschaltet wurde: der
    // Verlauf trägt nur Farb-Platzhalter (chatcolors.h), es genügt also, die
    // chatLog-Bindung neu auswerten zu lassen – der Verlauf wird dadurch samt
    // Übersetzungen und Globus-Ankern in den neuen Farben ausgeliefert.
    Q_INVOKABLE void refreshChatColors() { emit chatLogChanged(); }

signals:
    void chatLineReady(const QString &formattedLine);
    void chatLogChanged();
    void lobbyChatMentionDetected();
    void unreadPrivateMessagesChanged();
    void privateMessagesChanged();
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
    // "Show player stats" (Lobby-Icon / Tisch-Kontextmenü): QML zeigt die
    // native Player-Page des Spielers.
    void playerStatsRequested(const QString &playerName);
    void isInGameChanged();
    void gameRunningChanged();
    void isSpectatingChanged();
    void currentGameIdChanged();
    void rejoinOfferChanged();
    void rejoinWaitingChanged();

private:
    // Hängt eine fertig formatierte Chat-Zeile an den Verlauf an (begrenzt) und
    // benachrichtigt sowohl die Live-Verbraucher (chatLineReady) als auch die
    // bindbare chatLog-Property.
    void pushChatLine(const QString &line);
    // Aktueller Anzeigemodus für die Chat-Farben (Config "DarkMode", 0 = hell).
    bool chatDarkMode() const;
    // Bestätigung einer GESENDETEN privaten Nachricht im eigenen Chat-Verlauf –
    // mit vollem Nachrichtentext, weil eine PM sonst spurlos im Verlauf fehlt.
    // Wird von beiden Sendewegen benutzt (Chat-Kurzbefehl /msg und PM-Dialog).
    void pushPrivateMessageSentLine(const QString &targetName, const QString &message);
    // Trägt eine (gesendete oder empfangene) private Nachricht in den Verlauf des
    // Gesprächspartners ein und meldet die Änderung an die Oberfläche.
    void appendPrivateMessage(const QString &playerName, const QString &message, bool fromMe);
    void recountUnreadPrivateMessages();
    // Einzige Schreibstelle für m_gameRunning – meldet den Wechsel an QML.
    void setGameRunning(bool running);
    // Posteingang zwischen Sitzungen sichern – eigene SQLite-Datei neben der
    // config.xml (privatemessages.sqlite). Eigene Datenbank, damit lange
    // Unterhaltungen weder die Einstellungen noch die Spiel-Logs berühren.
    void openPrivateMessageDb();
    // Wechselt den Besitzer des Posteingangs (eigener Nick). Der Verlauf ist
    // pro Konto getrennt: nach einem Login mit einem anderen Benutzer darf
    // weder dessen Verlauf sichtbar sein noch fortgeschrieben werden. Leerer
    // Name = niemand angemeldet (Posteingang leer, es wird nichts gespeichert).
    void setPrivateMessageOwner(const QString &owner);
    // Liest den Verlauf des aktuellen Besitzers aus der Datenbank ein.
    void loadPrivateMessages();
    void persistPrivateMessage(const QString &playerName, const QVariantMap &entry);
    void persistPrivateThreadMeta(const QString &playerName);
    void persistDeletePrivateThread(const QString &playerName);
    // Antwort des Übersetzers auf eine PM-Blase (ignoriert fremde Request-Ids,
    // der TextTranslator bedient auch die Forum-Seite).
    void onPrivateMessageTranslated(int requestId, const QString &text, bool ok);

    boost::shared_ptr<Session> m_session;
    SoundEvents *m_soundEvents = nullptr;   // nicht besessen
    ConfigFile *m_config;
    
    PlayerListModel m_playerListModel;
    QSortFilterProxyModel *m_playerListProxyModel;
    GameListModel m_gameListModel;
    QSortFilterProxyModel *m_gameListProxyModel;
    
    QString m_myPlayerName;
    unsigned m_myPlayerId;
    int m_unreadPrivateMessages = 0;
    int m_privateMessagesRevision = 0;
    // Laufende Nummer je Nachricht, nur für diese Sitzung. Sie adressiert eine
    // Blase eindeutig – der Zeitstempel taugt dafür NICHT (Sekundenauflösung,
    // zwei Nachrichten derselben Sekunde sind nicht unterscheidbar) und der
    // Listenindex verschiebt sich beim Beschneiden des Verlaufs.
    int m_nextPrivateMessageId = 1;
    // Privater Nachrichtenverlauf je Gesprächspartner: das im Dialog gezeigte
    // Fenster der Unterhaltung (den vollständigen Verlauf hält die SQLite-Datei).
    struct PrivateThread {
        QVariantList messages;      // { msgId, fromMe, text, ts, ggf. translation* }
        int unread = 0;
        qint64 lastActivity = 0;    // ms since epoch, für die Sortierung
    };
    QHash<QString, PrivateThread> m_privateThreads;
    // Verbindungsname der PM-Datenbank; leer = keine Datenbank (dann lebt der
    // Verlauf nur bis zum Beenden).
    QString m_privateDbConn;
    // Konto, dem der geladene Posteingang gehört (eigener Nick). Leer, solange
    // niemand angemeldet ist – dann bleibt der Verlauf leer.
    QString m_privateMessagesOwner;
    // Formatierter Lobby-Chat-Verlauf (HTML-Zeilen). Die Textfarben stehen darin
    // NUR als Rollen-Platzhalter (chatcolors.h) und werden erst in chatLog()
    // zum aktuellen Hell/Dunkel-Modus aufgelöst.
    QStringList m_chatLog;
    ChatTranslator *m_chatTranslator = nullptr; // hängt Übersetzen-Symbole an und übersetzt sie
    TextTranslator *m_textTranslator = nullptr; // übersetzt einzelne PM-Blasen
    // Laufende PM-Übersetzungen: Request-Id -> (Gesprächspartner, msgId).
    QHash<int, QPair<QString, int>> m_pmTranslationRequests;
    // Aktuell im QML-Popup angefragte Einladung (0 = keine). Verhindert, dass
    // mehrere Einladungs-Popups gleichzeitig erscheinen (weitere → "busy").
    unsigned m_pendingInviteGameId = 0;
    // Vom Server angebotenes Rejoin nach Verbindungsabbruch (0 = keines).
    unsigned m_rejoinOfferGameId = 0;
    bool m_autoRejoin = false;
    // true, solange wir nach angenommenem Rejoin auf die nächste Hand warten.
    bool m_rejoinWaiting = false;
    void setRejoinWaiting(bool waiting);
    bool m_isCurrentPlayerAdmin = false;   // Server-Admin (kickban / Spiel schließen)
    bool m_isCurrentGameAdmin = false;     // Spiel-Admin (Host des aktuellen Tisches)
    bool m_isInGame = false;
    // true zwischen Zuschauer-Beitritt und Verlassen des Tisches. Wird aus dem
    // JoinGameAck des Servers übernommen (Session::isClientSpectating).
    bool m_isSpectating = false;
    // true zwischen Spielstart und Rückkehr in den Warteraum/die Lobby. Die
    // Join-Sounds (playerconnected/onlinegameready) gehören nur in den
    // Warteraum; ein PlayerJoined im laufenden Spiel ist ein Rejoin nach
    // Disconnect (Widgets-Client: isVisible()-Guard im GameLobbyDialog).
    bool m_gameRunning = false;
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
