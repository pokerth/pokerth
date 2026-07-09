/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2025 Felix Hammer, Florian Thauer, Lothar May          *
 *****************************************************************************/

#ifndef GAMEHANDLER_H
#define GAMEHANDLER_H

#include <QObject>
#include <QAbstractListModel>
#include <QVariantList>
#include <QStringList>
#include <QElapsedTimer>
#include <QSet>
#include <vector>
#include <boost/shared_ptr.hpp>

class ConfigFile;
class Session;
class Game;
class SoundEvents;
class QTimer;

// Inkrementelles Listenmodell für den Spielverlauf (Log). Bewusst KEIN
// QStringList-Property: ein QStringList ist für QML ein Werttyp, der bei jeder
// neuen Zeile komplett neu gelesen wird → die gebundene ListView setzt sich
// vollständig zurück und baut alle (RichText-)Delegates neu auf. Da der Verlauf
// während einer Hand mehrfach pro Sekunde wächst, schaukelt sich das mit der
// Listenlänge zu spürbarem Ruckeln auf. Mit beginInsertRows/beginRemoveRows
// fügt die View nur die eine neue Zeile ein (O(1)) statt neu aufzubauen.
class GameLogModel : public QAbstractListModel
{
    Q_OBJECT

    // Gesamter Verlauf als EIN RichText-Dokument (Zeilen mit <br> verkettet) –
    // für die TextEdit-basierte Anzeige (durchgehende Selektion + Kopieren,
    // analog zur ChatBox). Ändert sich bei jedem append()/clear().
    Q_PROPERTY(QString html READ html NOTIFY htmlChanged)

public:
    enum Roles { LineRole = Qt::UserRole + 1 };

    explicit GameLogModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}

    QString html() const { return m_lines.join(QStringLiteral("<br>")); }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : m_lines.size();
    }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (index.row() < 0 || index.row() >= m_lines.size())
            return QVariant();
        if (role == LineRole || role == Qt::DisplayRole)
            return m_lines.at(index.row());
        return QVariant();
    }
    QHash<int, QByteArray> roleNames() const override
    {
        return { { LineRole, QByteArrayLiteral("line") } };
    }

    // Eine Zeile anhängen und – wie zuvor – auf maxLines begrenzen.
    void append(const QString &line, int maxLines)
    {
        beginInsertRows(QModelIndex(), m_lines.size(), m_lines.size());
        m_lines.append(line);
        endInsertRows();
        if (m_lines.size() > maxLines) {
            const int n = m_lines.size() - maxLines;
            beginRemoveRows(QModelIndex(), 0, n - 1);
            m_lines.erase(m_lines.begin(), m_lines.begin() + n);
            endRemoveRows();
        }
        emit htmlChanged();
    }
    void clear()
    {
        if (m_lines.isEmpty())
            return;
        beginResetModel();
        m_lines.clear();
        endResetModel();
        emit htmlChanged();
    }

signals:
    void htmlChanged();

private:
    QStringList m_lines;
};

class GameHandler : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantList players READ players NOTIFY playersChanged)
    Q_PROPERTY(int pot READ pot NOTIFY potChanged)
    Q_PROPERTY(int gameId READ gameId NOTIFY gameIdChanged)
    Q_PROPERTY(QString phaseText READ phaseText NOTIFY phaseTextChanged)
    Q_PROPERTY(int handNumber READ handNumber NOTIFY handNumberChanged)
    Q_PROPERTY(bool myTurn READ myTurn NOTIFY myTurnChanged)
    Q_PROPERTY(bool canAct READ canAct NOTIFY canActChanged)
    Q_PROPERTY(int callAmount READ callAmount NOTIFY callAmountChanged)
    Q_PROPERTY(int minRaiseAmount READ minRaiseAmount NOTIFY minRaiseAmountChanged)
    Q_PROPERTY(int maxRaiseAmount READ maxRaiseAmount NOTIFY maxRaiseAmountChanged)
    Q_PROPERTY(int totalPot READ totalPot NOTIFY totalPotChanged)
    Q_PROPERTY(int boardCardCount READ boardCardCount NOTIFY boardCardCountChanged)
    Q_PROPERTY(QVariantList boardCards READ boardCards NOTIFY boardCardsChanged)
    // Liste aller Haupt-Pot-Gewinner-Sitze (mehrere bei Split-Pot). Der Widgets-
    // Client zeigt das Winner-Label auf jedem nicht gefoldeten Spieler, der den
    // Hauptpot gewonnen hat – Side-Pot-Gewinner bekommen KEIN Badge.
    Q_PROPERTY(QVariantList winnerSeatIds READ winnerSeatIds NOTIFY winnerSeatIdsChanged)
    Q_PROPERTY(QString winningHandText READ winningHandText NOTIFY winningHandTextChanged)
    // Showdown-Spotlight: 5 Bool-Werte (je Board-Karte). true = diese Board-Karte
    // gehört NICHT zum besten Blatt des Gewinners und wird beim Showdown
    // abgeblendet (Einstellung "Ausblend-Animation für Verliererkarten",
    // Config-Key ShowFadeOutCardsAnimation – wie der Widgets-Client, der die
    // nicht zum Siegerblatt zählenden Karten auf 25 % Deckkraft fadet). Die
    // entsprechenden Hole-Cards der Gewinner kommen über p["fade0"]/p["fade1"].
    Q_PROPERTY(QVariantList boardCardFade READ boardCardFade NOTIFY boardCardFadeChanged)
    // Aktiver Action-Timeout: Sitz, der gerade am Zug ist (−1 = keiner) und die
    // Timeout-Dauer in Sekunden. Die Player-Box zeigt dafür anstelle des
    // Action-Badges einen kleinen Fortschrittsbalken.
    Q_PROPERTY(int timeoutSeatId READ timeoutSeatId NOTIFY timeoutChanged)
    Q_PROPERTY(int timeoutSec READ timeoutSec NOTIFY timeoutChanged)
    // Spielverlauf als inkrementelles Modell (siehe GameLogModel). CONSTANT, weil
    // der Modell-Zeiger fix ist – Aktualisierungen laufen über die Modell-Signale.
    Q_PROPERTY(GameLogModel* gameLog READ gameLog CONSTANT)
    Q_PROPERTY(QStringList chatLog READ chatLog NOTIFY chatLogChanged)
    // true, sobald außer mir noch (mind.) ein menschlicher Spieler im Spiel ist
    Q_PROPERTY(bool hasHumanOpponents READ hasHumanOpponents NOTIFY hasHumanOpponentsChanged)
    // true im Post-River, wenn der Mensch-Spieler seine Karten freiwillig zeigen kann
    Q_PROPERTY(bool canShowCards READ canShowCards NOTIFY canShowCardsChanged)
    // true während der Showdown-/Ergebnisanzeige (Post-River, bis zur nächsten Hand).
    // QML deaktiviert in dieser Phase die Aktions-Buttons (Fold/Call/Raise), damit
    // kein verfrühter Klick für die nächste Runde ins Leere läuft.
    Q_PROPERTY(bool showdownActive READ showdownActive NOTIFY showdownActiveChanged)
    // Karten-Chancen des eigenen Blatts (Sitz 0) – analog zum CardsChanceMonitor
    // des Widgets-Clients. Liste mit 10 Einträgen, Index 0 = Höchste Karte …
    // 9 = Royal Flush; jeder Eintrag ist eine Map {"prob": int %, "possible": bool}.
    Q_PROPERTY(QVariantList cardsChance READ cardsChance NOTIFY cardsChanceChanged)
    // true, wenn der eigene Spieler gefoldet hat (Chancen werden abgeblendet).
    Q_PROPERTY(bool cardsChanceFolded READ cardsChanceFolded NOTIFY cardsChanceChanged)
    // Netzwerkstatus-Ampel des eigenen Clients aus dem Durchschnitts-Ping
    // (Einstellung ShowPingStateInAvatar): 0 = unbekannt/keine Daten,
    // 1 = grün (≤1000 ms), 2 = gelb (≤2000 ms), 3 = rot (>2000 ms). Wie der
    // Qt-Widgets-Client (MyAvatarLabel::refreshPing), nur am eigenen Avatar.
    Q_PROPERTY(int pingState READ pingState NOTIFY pingStateChanged)
    // Roh-Werte der letzten Server-Antwortzeiten (ms): Durchschnitt/Min/Max.
    // −1 = noch keine Daten. Speisen das Overlay am Netzwerkstatus-Punkt
    // (Mouseover) – wie der Tooltip des Qt-Widgets-Clients (refreshPing).
    Q_PROPERTY(int pingAvg READ pingAvg NOTIFY pingStateChanged)
    Q_PROPERTY(int pingMin READ pingMin NOTIFY pingStateChanged)
    Q_PROPERTY(int pingMax READ pingMax NOTIFY pingStateChanged)
    // Zuschauer des laufenden Spiels (spectatorsDuringGame) – wie der
    // Qt-Widgets-Client (gameTableImpl::refreshSpectatorsDisplay): Anzahl für
    // ein Auge-Icon mit Badge, Namen für den Tooltip.
    Q_PROPERTY(int spectatorCount READ spectatorCount NOTIFY spectatorsChanged)
    Q_PROPERTY(QStringList spectatorNames READ spectatorNames NOTIFY spectatorsChanged)
    // true, wenn ich diesem Tisch nur zuschaue. Dann gibt es keinen eigenen
    // Sitz: die GamePage zeichnet alle Sitze als Ring (ohne Self-Box) und
    // blendet die Action-Bar aus.
    Q_PROPERTY(bool spectating READ spectating NOTIFY spectatingChanged)

public:
    explicit GameHandler(QObject *parent = nullptr);
    ~GameHandler() override;

    void setSession(boost::shared_ptr<Session> session);
    void setGame(boost::shared_ptr<Game> game);
    void setConfig(ConfigFile *config);

    // Called from QML to start a local game
    Q_INVOKABLE void startLocalGame();
    Q_INVOKABLE void endLocalGame();
    Q_INVOKABLE bool isLocalGameRunning() const;
    // URL zur Tisch-Statistikübersicht (tableview=1 + Nicks der aktiven Spieler
    // am Tisch) – 1:1 wie der Qt-Widgets-Client (MyNameLabel). Baut aus den
    // Live-Seats des laufenden Spiels; leer, wenn kein Spiel läuft.
    Q_INVOKABLE QString tableStatsUrl() const;
    // Nicks der aktiven Spieler am laufenden Netzwerktisch (Seat-Reihenfolge,
    // ohne bereits ausgestiegene Spieler) – Datengrundlage für tableStatsUrl()
    // und die native Tisch-Ranking-Seite (GameTableStatsPage).
    Q_INVOKABLE QStringList tableStatsNicks() const;
    QVariantList players() const { return m_players; }
    int pot() const { return m_pot; }
    int gameId() const { return m_gameId; }
    QString phaseText() const { return m_phaseText; }
    int handNumber() const { return m_handNumber; }
    bool myTurn() const { return m_myTurn; }
    bool canAct() const { return m_canAct; }
    int callAmount() const { return m_callAmount; }
    int minRaiseAmount() const { return m_minRaiseAmount; }
    int maxRaiseAmount() const { return m_maxRaiseAmount; }
    int totalPot() const { return m_totalPot; }

    int boardCardCount() const { return m_boardCardCount; }
    QVariantList boardCards() const { return m_boardCards; }
    QVariantList winnerSeatIds() const { return m_winnerSeatIds; }
    QString winningHandText() const { return m_winningHandText; }
    QVariantList boardCardFade() const { return m_boardCardFade; }
    int timeoutSeatId() const { return m_timeoutSeatId; }
    int timeoutSec() const { return m_timeoutSec; }
    GameLogModel* gameLog() { return &m_gameLogModel; }
    QStringList chatLog() const { return m_chatLog; }
    bool hasHumanOpponents() const { return m_hasHumanOpponents; }
    bool canShowCards() const { return m_canShowCards; }
    bool showdownActive() const { return m_showdownActive; }
    QVariantList cardsChance() const { return m_cardsChance; }
    bool cardsChanceFolded() const { return m_cardsChanceFolded; }
    int pingState() const { return m_pingState; }
    int pingAvg() const { return m_pingAvg; }
    int pingMin() const { return m_pingMin; }
    int pingMax() const { return m_pingMax; }
    int spectatorCount() const { return static_cast<int>(m_spectatorNames.size()); }
    QStringList spectatorNames() const { return m_spectatorNames; }
    bool spectating() const { return m_spectating; }

    // Zeilentyp für die Einfärbung des Spielverlaufs – Farben/Stil 1:1 wie der
    // Qt-Widgets-Client (Default-Tischstil).
    enum LogLineType {
        LogNormal = 0,   // Aktionen, Blinds, aufgedeckte Karten (#F0F0F0)
        LogHeader,       // "## Game | Hand ##" (fett)
        LogWinnerMain,   // Gewinner Hauptpot (#FFFF00)
        LogWinnerSide,   // Gewinner Side-Pot (#FFFFCC)
        LogSitOut,       // "… sits out" (kursiv, #FF6633)
        LogBoard,        // "--- Flop/Turn/River ---" (#FF6633)
        LogGameWin       // "… wins game X!" (fett+kursiv)
    };
    Q_ENUM(LogLineType)

    // Append a line to the in-game action log (called from QmlGuiInterface).
    Q_INVOKABLE void appendGameLog(const QString &message, int type = LogNormal);
    // In-game chat: append a received message / send one to the table.
    Q_INVOKABLE void appendChat(const QString &playerName, const QString &message);
    Q_INVOKABLE void sendChat(const QString &message);

    // Called from QmlGuiInterface callbacks (must be Q_INVOKABLE for invokeMethod)
    Q_INVOKABLE void onRefreshSet();
    Q_INVOKABLE void onRefreshAction(int playerId, int playerAction);
    Q_INVOKABLE void onRefreshCash();
    Q_INVOKABLE void onRefreshPlayerName();
    Q_INVOKABLE void onRefreshPot();
    Q_INVOKABLE void onRefreshGameLabels(int gameState);
    Q_INVOKABLE void onMeInAction();
    Q_INVOKABLE void onDisableMyButtons();
    Q_INVOKABLE void onStartTimeoutAnimation(int playerNum, int timeoutSec);
    Q_INVOKABLE void onStopTimeoutAnimation(int playerNum);
    // Netzwerk-Spiel beendet / aus dem Spiel entfernt: GameHandler-Zustand
    // zurücksetzen, damit kein stale m_myTurn/m_game zurückbleibt (sonst kann
    // eine späte Aktion ins tote Spiel laufen → siehe ClientThread::SendPlayerAction).
    Q_INVOKABLE void onNetworkGameEnded();
    // Netzwerk: Spieler hat das Spiel verlassen → Sitz leeren und im Log
    // vermerken (verlassen / gekickt / getrennt – removeReason aus socket_msg.h).
    Q_INVOKABLE void onNetClientPlayerLeft(unsigned uniquePlayerId,
                                           const QString &playerName = QString(),
                                           int removeReason = 0);
    // Netzwerk: Zuschauerliste des laufenden Spiels aus der Session neu einlesen
    // (bei Spectator-Join/-Left/-Umbenennung aufgerufen).
    Q_INVOKABLE void refreshSpectators();
    // Netzwerk: eigener Client-Ping aktualisiert → Netzwerkstatus-Ampel ableiten.
    Q_INVOKABLE void onPingUpdate(int minPing, int avgPing, int maxPing);
    Q_INVOKABLE void onBlindsSet(int smallBlind);
    Q_INVOKABLE void onNextRoundCleanGui();
    Q_INVOKABLE void onDealFlopCards();
    Q_INVOKABLE void onDealTurnCard();
    Q_INVOKABLE void onDealRiverCard();
    // Game-loop advance callbacks (called via QMetaObject from QmlGuiInterface)
    Q_INVOKABLE void onRunBeRo();
    Q_INVOKABLE void onAfterDealCards();
    Q_INVOKABLE void onNextPlayerBeRo();
    Q_INVOKABLE void onSwitchRounds();
    Q_INVOKABLE void onPostRiverRunBeRo();
    Q_INVOKABLE void onShowdown();
    Q_INVOKABLE void onFlipHolecardsAllIn();
    // Ein Spieler zeigt nach der Hand freiwillig seine Karten (AfterHandShowCards).
    Q_INVOKABLE void onPlayerShowCards(unsigned playerId);

    // Called from QML
    Q_INVOKABLE void fold();
    Q_INVOKABLE void call();
    Q_INVOKABLE void raise(int amount = 0);
    Q_INVOKABLE void allIn();
    Q_INVOKABLE void showMyCards();

signals:
    void playersChanged();
    void potChanged();
    void gameIdChanged();
    void phaseTextChanged();
    void handNumberChanged();
    void myTurnChanged();
    // Wird bei JEDEM „ich bin am Zug"-Callback der Engine ausgelöst (meInAction),
    // unabhängig davon, ob sich m_myTurn dabei ändert. Die QML-Seite führt darauf
    // die vorgemerkte/automatische Aktion aus – wie der Widgets-Client, der die
    // gemerkte Aktion direkt in meInAction() ausführt (Button-Klick). So hängt die
    // Ausführung NICHT mehr am myTurn-Flankenwechsel (der z.B. ausbleibt, wenn der
    // Zug schon über den Action-Timer als aktiv markiert wurde).
    void meInActionTriggered();
    void refreshActionTriggered();   // echte Spieler-Aktion (kein globaler Refresh)
    void roundValuesReady();          // nach Rundenwechsel: frische Werte verfügbar
    // Setzrunde gerade entschieden (letzte Aktion der Runde ist erfolgt, nächste
    // Runde/Hand noch nicht gestartet). QML sperrt darauf hin die Aktions-Buttons
    // sofort und verwirft veraltete Vorwahl/Beträge, bis roundValuesReady() bzw.
    // der nächste eigene Zug wieder frische Werte liefert.
    void bettingRoundEnded();
    void canActChanged();
    void callAmountChanged();
    void minRaiseAmountChanged();
    void maxRaiseAmountChanged();
    void totalPotChanged();
    void boardCardCountChanged();
    void boardCardsChanged();
    void winnerSeatIdsChanged();
    void winningHandTextChanged();
    void boardCardFadeChanged();
    void timeoutChanged();
    void chatLogChanged();
    void hasHumanOpponentsChanged();
    void canShowCardsChanged();
    void showdownActiveChanged();
    void cardsChanceChanged();
    void pingStateChanged();
    void spectatorsChanged();
    void spectatingChanged();
    // Emoji-Reaktion empfangen (Chat-Konvention "/emoji 🎉" des Web-Clients) –
    // wird nicht im Chat angezeigt, sondern als Animation am Sitz abgespielt.
    void reactionReceived(const QString &playerName, const QString &emoji);
    // Eigene Karten wurden (auf Klick) aufgedeckt → Self-Box spielt eine Flip-
    // Bestätigung wie der Widget-Client (showHoleCards), obwohl die eigenen
    // Karten bereits offen liegen.
    void myCardsShown();

protected:
    // App-weiter Filter: echte Nutzeraktivität (Maus/Tastatur) → ResetTimeout
    // an den Server, damit der In-Game-AFK-Timeout (21 min) nicht zuschlägt.
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    bool localGameCallbacksBlocked() const;
    void ensureSoundEventHandler();
    void playYourTurnTimeoutSound();
    void refreshPlayerData();
    void refreshBoardCards();
    // Chancen (CardsValue::calcCardsChance) + aktuell bestes Blatt des eigenen
    // Spielers neu berechnen und – nur bei Änderung – die QML-Seite benachrichtigen.
    void refreshChanceAndHand();
    void refreshPotData();
    void computeCallAndRaiseAmounts();
    // Lokales Spiel: Spieler mit 0 Coins nach 10 Sekunden aus der Anzeige
    // entfernen (analog zu onNetClientPlayerLeft bei Online-Spielen).
    void checkBustedLocalPlayers();
    // True, wenn der menschliche Spieler (Sitz 0) gerade agieren kann (in der
    // Hand, nicht all-in/gefoldet, Cash > 0, aktiv). Engine-basiert.
    bool humanCanAct() const;
    // True, wenn der Server gerade auf MEINE Aktion wartet. Maßgeblich ist der
    // Aktions-Timer auf meinem Sitz (m_timeoutSeatId == 0), der bereits ab
    // startTimeoutAnimation gesetzt ist (vor meInAction). Zusätzlich m_myTurn,
    // falls der Timer-Pfad mal nicht greift. Verhindert verworfene Aktionen.
    //
    // Als Zuschauer NIE: dort ist Sitz 0 ein fremder Spieler, dessen Aktions-
    // Timer m_timeoutSeatId auf 0 setzt. Ohne diesen Wächter könnten fold()/
    // call()/raise()/showMyCards() (Tastenkürzel!) für ihn ausgelöst werden.
    bool isMyTurnToAct() const { return !m_spectating && (m_myTurn || m_timeoutSeatId == 0); }
    void doActionDone();
    // Showdown-Flag setzen und – nur bei echter Änderung – die QML-Seite
    // benachrichtigen (showdownActive gatet u. a. die Aktions-Buttons).
    void setShowdownActive(bool active);

    boost::shared_ptr<Session> m_session;
    boost::shared_ptr<Game> m_game;
    ConfigFile *m_config = nullptr;
    SoundEvents *m_soundEventHandler = nullptr;
    QTimer *m_timeoutBeepTimer = nullptr;
    // Ratenbegrenzung für AFK-Reset (ResetTimeoutMessage). Wie der Widgets-
    // Client: höchstens alle paar Minuten senden, bei echter Nutzeraktivität.
    QElapsedTimer m_afkResetTimer;
    static constexpr qint64 kAfkResetIntervalMs = 3 * 60 * 1000; // 3 min

    QVariantList m_players;
    int m_pot = 0;
    int m_gameId = 0;
    QString m_phaseText;
    int m_handNumber = 0;
    bool m_myTurn = false;
    bool m_canAct = false;
    // Flankenerkennung für bettingRoundEnded(): true, solange computeCallAndRaise-
    // Amounts() die Setzrunde als abgeschlossen erkennt (roundClosed). Das Signal
    // feuert nur auf der steigenden Flanke (Runde gerade entschieden).
    bool m_roundClosed = false;
    int m_callAmount = 0;
    int m_minRaiseAmount = 0;
    int m_maxRaiseAmount = 0;
    int m_totalPot = 0;
    int m_boardCardCount = 0;
    QVariantList m_boardCards;  // 5 slots: card index (0-51) or -1 if not dealt
    QVariantList m_winnerSeatIds;
    QString m_winningHandText;  // Name der Gewinner-Hand (nur während des Showdowns)
    // Showdown-Spotlight (siehe boardCardFade-Property). m_boardCardFade hat 5
    // Bool-Einträge; m_holeFade0/1 enthalten die Sitze, deren Hole-Card 0/1
    // abgeblendet wird. Werden zu Beginn jeder Hand zurückgesetzt.
    QVariantList m_boardCardFade = {false, false, false, false, false};
    QSet<int> m_holeFade0;
    QSet<int> m_holeFade1;
    int m_timeoutSeatId = -1;   // Sitz mit laufendem Action-Timeout (−1 = keiner)
    int m_timeoutSec = 0;       // Dauer des Action-Timeouts in Sekunden
    GameLogModel m_gameLogModel; // Live-Aktions-Log (Spielverlauf) für das Overlay
    QStringList m_chatLog;      // In-Game-Chat-Verlauf
    bool m_hasHumanOpponents = false;
    bool m_canShowCards = false;
    // Showdown aktiv: erst dann dürfen Gegnerkarten aufgedeckt werden. Verhindert,
    // dass die (noch veraltete) playerNeedToShowCards-Liste während der River-
    // Setzrunde der nächsten Hand fälschlich Karten aufdeckt.
    bool m_showdownActive = false;
    // Karten-Chancen (10 Maps {prob, possible}) + Fold-Zustand + aktuelles Blatt
    // des eigenen Spielers. Werden in refreshChanceAndHand() aktualisiert.
    QVariantList m_cardsChance;
    bool m_cardsChanceFolded = false;
    int m_pingState = 0;  // Netzwerkstatus-Ampel (0 unbekannt,1 grün,2 gelb,3 rot)
    int m_pingAvg = -1;   // letzte Server-Antwortzeiten in ms (−1 = keine Daten)
    int m_pingMin = -1;
    int m_pingMax = -1;
    QStringList m_spectatorNames;  // Namen der Zuschauer des laufenden Spiels
    bool m_spectating = false;     // ich schaue diesem Tisch nur zu
    // Eingangssignatur der letzten Chancen-Berechnung (Hole-Cards, Board, Fold-
    // Zustand). Bleibt sie gleich, wird die teure calcCardsChance-Schleife
    // übersprungen – refreshChanceAndHand() läuft sonst bei jedem Mikro-Refresh.
    std::vector<int> m_lastChanceInputs;
    // All-in-Aufdeckung: alle nicht-gefoldeten Spielerkarten sichtbar
    // (AllInShowCardsMessage), bis zur nächsten Hand zurückgesetzt.
    bool m_allInRevealed = false;
    // Spieler (Unique-ID), die nach der Hand freiwillig ihre Karten gezeigt haben
    // (AfterHandShowCardsMessage → SignalNetClientPostRiverShowCards). Ihre Karten
    // bleiben aufgedeckt bis zur nächsten Hand. Im Widgets-Client macht das
    // gameTableImpl::showHoleCards; die QML-Showdown-Aufdeckung greift hier nicht,
    // weil der Zeiger nicht in playerNeedToShowCards steht (Gewinn ohne Showdown).
    QSet<unsigned> m_postRiverShownPlayers;
    // Aktions-Anzeige: pro Sitz die zuletzt gesehene Aktion + das Runden-Token,
    // in dem sie gesetzt wurde. So wird die Aktion nur in ihrer eigenen Runde
    // angezeigt und zu Rundenbeginn überall automatisch entfernt.
    int m_lastSeenAction[10] = {};
    // Zusätzlich zum Aktionstyp den Einsatz pro Sitz merken: ein erneutes Callen
    // nach einer Erhöhung bleibt Typ CALL, erhöht aber den Einsatz → gilt als
    // frische Aktion, damit das (zuvor geleerte) Badge wieder erscheint.
    int m_lastSeenSet[10] = {};
    int m_actionToken[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
    // Setzt ein Spieler bet/raise, müssen die Aktions-Badges aller anderen (noch
    // nicht gefoldeten) Spieler verschwinden – sie sind wieder am Zug. Dazu bekommt
    // jede Aktion eine fortlaufende Sequenznummer; angezeigt wird sie nur, wenn sie
    // mindestens so neu ist wie die letzte Aggression (bet/raise) der Runde.
    int m_actionSeq[10] = {};
    int m_actionCounter = 0;
    int m_lastAggressorSeq = 0;
    int m_aggressorToken = -1;
    bool m_localGameExitRequested = false;
    // Unique-IDs von Spielern, die das Netzwerkspiel verlassen haben.
    // Ihr Sitz wird in refreshPlayerData() als leer dargestellt.
    QSet<unsigned> m_leftPlayers;
    // Lokales Spiel: laufende 10-Sekunden-Timer für Spieler mit 0 Coins.
    // Schlüssel = Unique-Player-ID; nach Ablauf wird der Spieler wie ein
    // verlassener Online-Spieler behandelt (Sitz ausgeblendet).
    QMap<unsigned, QTimer*> m_bustedLocalTimers;
};

#endif // GAMEHANDLER_H
