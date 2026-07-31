/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2025 Felix Hammer, Florian Thauer, Lothar May          *
 *****************************************************************************/

#include "gamehandler.h"
#include "chattranslator.h"
#include "chatemotes.h"
#include "gui/chat_emote_shortcuts.h"
#include <session.h>
#include <game.h>
#include <handinterface.h>
#include <playerinterface.h>
#include <boardinterface.h>
#include <berointerface.h>
#include <cardsvalue.h>
#include <playerdata.h>
#include <game_defs.h>
#include <gamedata.h>
#include <configfile.h>
#include <soundevents.h>
#include <net/socket_msg.h>
#include <QString>
#include <QTimer>
#include <QDebug>
#include <QUrl>
#include <QFileInfo>
#include <QDateTime>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QEvent>
#include <algorithm>
#include <list>

namespace {
// Karten-Code (0-51) → Kurzform mit Unicode-Farbsymbol, z. B. "K♥". Identisch zu
// QmlGuiInterface::fmtCard, damit Showdown-Karten genau wie die Board-Karten im
// Spielverlauf aussehen.  0-12 Karo(♦), 13-25 Herz(♥), 26-38 Pik(♠), 39-51 Kreuz(♣).
QString logCard(int code)
{
    if (code < 0 || code > 51)
        return QStringLiteral("?");
    static const char *ranks[] = {"2","3","4","5","6","7","8","9","10","J","Q","K","A"};
    static const QChar suits[] = { QChar(0x2666), QChar(0x2665), QChar(0x2660), QChar(0x2663) };
    return QString::fromLatin1(ranks[code % 13]) + QString(suits[code / 13]);
}

// Spielverlauf-Zeile als HTML einfärben – Farben/Stil 1:1 wie der Qt-Widgets-
// Client (Default-Tischstil): normal #F0F0F0, Gewinner Hauptpot #FFFF00, Side-Pot
// #FFFFCC, Sit-out/Board #FF6633.
QString formatLogLine(const QString &text, int type)
{
    const QString esc = text.toHtmlEscaped();
    switch (type) {
    case GameHandler::LogHeader:
        return QStringLiteral("<span style=\"color:#F0F0F0; font-weight:bold;\">") + esc + QStringLiteral("</span>");
    case GameHandler::LogWinnerMain:
        return QStringLiteral("<span style=\"color:#FFFF00;\">") + esc + QStringLiteral("</span>");
    case GameHandler::LogWinnerSide:
        return QStringLiteral("<span style=\"color:#FFFFCC;\">") + esc + QStringLiteral("</span>");
    case GameHandler::LogSitOut:
        return QStringLiteral("<i><span style=\"color:#FF6633;\">") + esc + QStringLiteral("</span></i>");
    case GameHandler::LogBoard:
        return QStringLiteral("<span style=\"color:#FF6633;\">") + esc + QStringLiteral("</span>");
    case GameHandler::LogGameWin:
        return QStringLiteral("<b><i><span style=\"color:#F0F0F0;\">") + esc + QStringLiteral("</span></i></b>");
    default:
        return QStringLiteral("<span style=\"color:#F0F0F0;\">") + esc + QStringLiteral("</span>");
    }
}

// Avatar-Pfad → QML-Bildquelle. getMyAvatar() liefert (wie im Widgets-Client)
// einen lokalen Dateipfad; existiert die Datei, als file://-URL zurückgeben.
QString resolveAvatarSource(const std::string &raw)
{
    if (raw.empty())
        return QString();
    const QString path = QString::fromStdString(raw);
    if (!QFileInfo::exists(path))
        return QString();
    return QUrl::fromLocalFile(path).toString();
}
} // namespace

GameHandler::GameHandler(QObject *parent)
    : QObject(parent), m_phaseText("Preflop")
{
    // Initialize empty player list (10 seats)
    for (int i = 0; i < 10; ++i) {
        QVariantMap p;
        p["name"]    = QString("");
        p["stack"]   = 0;
        p["bet"]     = 0;
        p["active"]  = false;
        p["myTurn"]  = false;
        p["seatId"]  = i;
        p["button"]  = 0;
        p["action"]  = 0;
        p["card0"]   = -1;
        p["card1"]   = -1;
        p["fade0"]   = false;
        p["fade1"]   = false;
        p["playerId"] = 0;
        p["countryCode"] = QString("");
        p["isGuest"] = false;
        p["reserved"] = false;
        m_players.append(p);
    }
    // Initialize empty board cards (5 slots, -1 = not dealt)
    for (int i = 0; i < 5; ++i)
        m_boardCards.append(-1);

    m_timeoutBeepTimer = new QTimer(this);
    m_timeoutBeepTimer->setSingleShot(true);
    connect(m_timeoutBeepTimer, &QTimer::timeout, this, [this]() {
        playYourTurnTimeoutSound();
    });

    // Chat-Übersetzer operiert direkt auf m_chatLog; jede von ihm veränderte
    // Zeile stößt chatLogChanged() an, damit die QML-Bindung neu rendert.
    m_chatTranslator = new ChatTranslator(&m_chatLog, this);
    connect(m_chatTranslator, &ChatTranslator::chatLogMutated,
            this, &GameHandler::chatLogChanged);

    // AFK-Reset: echte Nutzeraktivität (Maus/Tastatur) hält den serverseitigen
    // Inaktivitäts-Timeout zurück. WICHTIG: Spielaktionen (fold/call/raise)
    // zählen serverseitig NICHT als Aktivität (Type_MyActionRequestMessage ist
    // von IsClientActivity ausgenommen, damit Auto-Check/Fold den AFK-Timeout
    // nicht aushebelt) – nur eine Type_ResetTimeoutMessage setzt den In-Game-
    // Timer (21 min) zurück. Ohne dies wurde der QML-Client trotz aktiven
    // Spielens nach ~21 min vom Server gekickt (wie der Widgets-Client per
    // eventFilter). App-weiter Filter, ratenbegrenzt.
    m_afkResetTimer.start();
    if (qApp)
        qApp->installEventFilter(this);
}

bool GameHandler::eventFilter(QObject *watched, QEvent *event)
{
    const QEvent::Type t = event->type();
    if (t == QEvent::MouseButtonPress || t == QEvent::KeyPress) {
        if (m_session && m_session->isNetworkClientRunning()
            && m_afkResetTimer.elapsed() >= kAfkResetIntervalMs) {
            m_session->resetNetworkTimeout();
            m_afkResetTimer.restart();
        }
    }
    return QObject::eventFilter(watched, event);
}

GameHandler::~GameHandler()
{
    delete m_soundEventHandler;
    m_soundEventHandler = nullptr;
}

void GameHandler::setConfig(ConfigFile *config)
{
    m_config = config;
    if (m_chatTranslator)
        m_chatTranslator->setConfig(config);
    ensureSoundEventHandler();
}

QObject* GameHandler::chatTranslator() const
{
    return m_chatTranslator;
}

void GameHandler::setSession(boost::shared_ptr<Session> session)
{
    m_session = session;
}

void GameHandler::setGame(boost::shared_ptr<Game> game)
{
    ensureSoundEventHandler();
    if (m_soundEventHandler)
        m_soundEventHandler->newGameStarts();

    m_localGameExitRequested = false;
    m_game = game;
    // Zuschauer-Modus des Netzwerk-Clients übernehmen (im lokalen Spiel immer
    // false). Steht bereits fest, wenn die Engine das Spiel meldet: der
    // JoinGameAck kam vor dem Spielstart.
    const bool spectating = m_session && m_session->isNetworkClientRunning()
                            && m_session->isClientSpectating();
    if (spectating != m_spectating) {
        m_spectating = spectating;
        emit spectatingChanged();
    }
    m_leftPlayers.clear();
    // Ausstehende Busted-Player-Timer aus dem vorigen Spiel verwerfen.
    qDeleteAll(m_bustedLocalTimers);
    m_bustedLocalTimers.clear();
    // Reset state for new game
    m_pot = 0;
    m_gameId = m_game ? m_game->getMyGameID() : 0;
    m_phaseText = "Preflop";
    m_handNumber = 0;
    m_myTurn = false;
    m_myTurnWindowClosed = true;
    m_engineTurnPointedAtMe = false;
    m_awaitingMyAction = false;
    m_callAmount = 0;
    m_minRaiseAmount = 0;
    m_maxRaiseAmount = 0;
    m_boardCardCount = 0;
    m_boardCards = QVariantList{-1, -1, -1, -1, -1};
    m_winnerSeatIds.clear();
    m_winningHandText.clear();
    m_holeFade0.clear();
    m_holeFade1.clear();
    m_boardCardFade = QVariantList{false, false, false, false, false};
    setShowdownActive(false);
    m_gameLogModel.clear();
    m_chatLog.clear();
    emit chatLogChanged();
    for (int i = 0; i < 10; ++i) {
        m_lastSeenAction[i] = 0;
        m_actionToken[i] = -1;
    }

    // Re-build player list (seats may differ between games)
    refreshPlayerData();

    emit potChanged();
    emit gameIdChanged();
    emit phaseTextChanged();
    emit handNumberChanged();
    emit myTurnChanged();
    emit awaitingMyActionChanged();
    emit callAmountChanged();
    emit minRaiseAmountChanged();
    emit maxRaiseAmountChanged();
    emit boardCardCountChanged();
    emit boardCardsChanged();
    emit winnerSeatIdsChanged();
    emit winningHandTextChanged();
    emit boardCardFadeChanged();
}

QString GameHandler::tableStatsUrl() const
{
    if (!m_game || !m_session)
        return QString();

    // Tischname aus der Server-Spielinfo (wie refreshSpectators): der GameHandler
    // hält den Namen nicht selbst, wohl aber die aktuelle Game-ID der Session.
    const unsigned gameId = m_session->getClientCurrentGameId();
    if (gameId == 0)
        return QString();
    const GameInfo info = m_session->getClientGameInfo(gameId);

    QString nickList;
    const QStringList nicks = tableStatsNicks();
    for (int i = 0; i < nicks.size(); ++i) {
        nickList += QString("&nick%1=").arg(i + 1);
        nickList += QString::fromUtf8(QUrl::toPercentEncoding(nicks.at(i)));
    }

    return QStringLiteral("https://www.pokerth.net/redirect_user_profile.php?tableview=1")
        + nickList
        + QStringLiteral("&table=")
        + QString::fromUtf8(QUrl::toPercentEncoding(QString::fromStdString(info.name)));
}

QStringList GameHandler::tableStatsNicks() const
{
    QStringList nicks;
    if (!m_game || !m_session || m_session->getClientCurrentGameId() == 0)
        return nicks;

    // Nick-Liste 1:1 wie der Qt-Widgets-Client (MyNameLabel): nur aktive Spieler.
    // Zusätzlich – wie beim Aufbau von m_players – bereits ausgestiegene Spieler
    // (m_leftPlayers) überspringen, damit die Liste exakt den sichtbaren
    // Spielerboxen entspricht und sich beim Verlassen mitzieht.
    PlayerList seats = m_game->getSeatsList();
    for (auto it = seats->begin(); it != seats->end(); ++it) {
        if (m_leftPlayers.contains((*it)->getMyUniqueID()))
            continue;
        if (!(*it)->getMyActiveStatus())
            continue;
        nicks << QString::fromStdString((*it)->getMyName());
    }
    return nicks;
}

// ─── private helpers ────────────────────────────────────────────────────────

void GameHandler::ensureSoundEventHandler()
{
    if (!m_soundEventHandler && m_config)
        m_soundEventHandler = new SoundEvents(m_config);
}

void GameHandler::playYourTurnTimeoutSound()
{
    ensureSoundEventHandler();
    if (m_soundEventHandler)
        m_soundEventHandler->playSound("yourturn", 0);
}

void GameHandler::appendGameLog(const QString &message, int type)
{
    if (message.isEmpty()) return;
    // Inkrementelles Anhängen + Begrenzung im Modell (kein Full-Reset der View).
    const int kMaxLines = 400;
    m_gameLogModel.append(formatLogLine(message, type), kMaxLines);
}

void GameHandler::appendChat(const QString &playerName, const QString &message)
{
    if (message.isEmpty()) return;

    // Nachrichten ignorierter Spieler verwerfen (vor Reaktions-Behandlung, damit
    // auch deren Emoji-Reaktionen stumm bleiben). Liste wie im Lobby-Chat bei
    // jeder Nachricht frisch lesen (chattools.cpp-Muster, gilt dort ebenso für
    // den Spiel-Chat).
    if (m_config) {
        const std::list<std::string> ignoreList = m_config->readConfigStringList("PlayerIgnoreList");
        for (const auto &entry : ignoreList) {
            if (playerName == QString::fromUtf8(entry.c_str()))
                return;
        }
    }

    // Emoji-Reaktionen (Konvention des Web-Clients): "/emoji 🎉" bzw. legacy
    // "[R]🎉". Nicht in den Chat-Verlauf aufnehmen, sondern als Reaktions-
    // Animation am Sitz des Absenders abspielen. Auf dem getrimmten Text
    // prüfen (Clients können Whitespace anhängen); Längen-Limit großzügiger
    // als im Web-Client (22 statt 18), damit auch ZWJ-/Variation-Selector-
    // Sequenzen durchgehen – normale Nachrichten matchen trotzdem nicht.
    const QString trimmedMsg = message.trimmed();
    bool isReactionMsg = false;
    QString reactionEmoji;
    if (trimmedMsg.startsWith(QStringLiteral("/emoji ")) && trimmedMsg.size() < 22) {
        isReactionMsg = true;
        reactionEmoji = trimmedMsg.mid(7).trimmed();
    } else if (trimmedMsg.startsWith(QStringLiteral("[R]")) && trimmedMsg.size() < 14) {
        isReactionMsg = true;
        reactionEmoji = trimmedMsg.mid(3).trimmed();
    }
    if (isReactionMsg) {
        // Reaktions-Nachrichten erscheinen nie im Chat-Verlauf. Nur echte
        // Emojis abspielen – als Reaktion getarnter Text wird verworfen.
        if (isEmojiOnlyReaction(reactionEmoji)) {
            qDebug() << "[REACT] incoming reaction from" << playerName << ":" << reactionEmoji;
            emit reactionReceived(playerName, reactionEmoji);
        } else {
            qDebug() << "[REACT] discarding non-emoji reaction from" << playerName << ":" << reactionEmoji;
        }
        return;
    }

    // Formatierung analog zum Lobby-Chat: /me-Aktion, Emojis, Erwähnung.
    const QString myNick = m_config ? QString::fromStdString(m_config->readConfigString("MyName")) : QString();
    const bool isAction = message.startsWith(QStringLiteral("/me "));
    const QString rawDisplay = isAction ? message.mid(4) : message;

    QString escapedMsg = rawDisplay.toHtmlEscaped();
    // ASCII-Kürzel (":-)", "8-)", "<3", …) auf dem rohen Text umsetzen, bevor
    // Link-/Style-Markup hinzukommt – so kollidieren kurze Kürzel nie mit
    // unserem eigenen HTML (z. B. "color:#...").
    escapedMsg = applyChatEmoteShortcuts(escapedMsg);
    static const QRegularExpression urlRe(QStringLiteral("(https?://\\S+)"));
    escapedMsg.replace(urlRe, QStringLiteral("<a href=\"\\1\">\\1</a>"));

    const bool isMention = !myNick.isEmpty() && rawDisplay.contains(myNick, Qt::CaseInsensitive);
    const QString color = isMention ? QStringLiteral("#E3C800") : QStringLiteral("#e6e6e6");
    QString styledMsg = QStringLiteral("<span style=\"color:") + color
                        + (isMention ? QStringLiteral("; font-weight:bold") : QString())
                        + QStringLiteral(";\">") + escapedMsg + QStringLiteral("</span>");
    styledMsg = enlargeEmojis(styledMsg);

    const QString tsPrefix = chatTimestampPrefix(m_config);
    const QString name = playerName.toHtmlEscaped();
    QString line;
    if (isAction)
        line = tsPrefix + QStringLiteral("<i>* ") + name + QStringLiteral(" ") + styledMsg + QStringLiteral(" *</i>");
    else
        line = tsPrefix + QStringLiteral("<b>") + name + QStringLiteral(":</b> ") + styledMsg;

    // Übersetzen-Symbol nur an Nachrichten anderer (rawDisplay = Quelltext ohne
    // HTML/Style-Markup; styledMsg = Nachrichtenkörper, der beim Einblenden
    // durch die Übersetzung ersetzt wird).
    if (m_chatTranslator && playerName != myNick)
        line = m_chatTranslator->decorate(line, rawDisplay, styledMsg);

    m_chatLog.append(line);
    const int kMaxLines = 400;
    if (m_chatLog.size() > kMaxLines)
        m_chatLog.erase(m_chatLog.begin(), m_chatLog.begin() + (m_chatLog.size() - kMaxLines));
    emit chatLogChanged();
}

void GameHandler::sendChat(const QString &message)
{
    if (!m_session || message.trimmed().isEmpty()) return;
    // Auf 128 Bytes UTF-8 begrenzen (wie der Lobby-Chat).
    QString text = message;
    while (!text.isEmpty() && text.toUtf8().size() > 128)
        text.chop(1);
    if (text.isEmpty()) return;
    m_session->sendGameChatMessage(text.toStdString());
}

bool GameHandler::localGameCallbacksBlocked() const
{
    if (!m_localGameExitRequested) return false;
    if (!m_session) return true;
    return !m_session->isNetworkClientRunning();
}

void GameHandler::refreshPlayerData()
{
    // Build a fresh 10-slot list
    QVariantList newPlayers;
    for (int i = 0; i < 10; ++i) {
        QVariantMap p;
        p["name"]   = QString("");
        p["stack"]  = 0;
        p["bet"]    = 0;
        p["active"] = false;
        p["myTurn"] = false;
        p["seatId"] = i;
        p["button"] = 0;
        p["action"] = 0;
        p["card0"]  = -1;
        p["card1"]  = -1;
        p["fade0"]  = false;
        p["fade1"]  = false;
        p["playerId"] = 0;
        p["countryCode"] = QString("");
        p["isGuest"] = false;
        // Leerer Sitz eines Spielers, der den Tisch verlassen hat (Disconnect,
        // Kick, Verlassen, Ausgeschieden) – siehe Markierung unten.
        p["reserved"] = false;
        newPlayers.append(p);
    }

    // Lazy-init m_game for local games: session creates the game internally
    if (!m_game && m_session && !m_localGameExitRequested) {
        auto g = m_session->getCurrentGame();
        if (g) m_game = g;
    }

    // Eindeutiges Token der aktuellen Setzrunde (Hand-Nr. × 8 + Runde). Eine Aktion
    // wird nur angezeigt, solange dieses Token unverändert ist → zu Rundenbeginn
    // (auch über Hände hinweg) verschwinden alle Aktions-Anzeigen automatisch.
    int currentToken = -1;
    if (m_game) {
        auto hand = m_game->getCurrentHand();
        if (hand) {
            currentToken = hand->getMyID() * 8 + static_cast<int>(hand->getCurrentRound());
            // Showdown gilt nur in der Post-River-Phase. In jeder aktiven Setzrunde
            // (Preflop–River) zurücksetzen, damit ein stehengebliebenes Flag (z. B.
            // wenn onNextRoundCleanGui im Netzwerkspiel nicht feuert) nicht die
            // Action-Badges der Folgehände ausblendet.
            if (hand->getCurrentRound() != GAME_STATE_POST_RIVER)
                setShowdownActive(false);
        }
    }

    // Neue Setzrunde → die letzte Aggression (bet/raise) gilt nicht mehr.
    if (currentToken != m_aggressorToken) {
        m_aggressorToken = currentToken;
        m_lastAggressorSeq = 0;
    }

    int humanCount = 0;
    // Nur im Netzwerkspiel ist getMyUniqueID() die serverweite Spieler-Id, mit
    // der sich PlayerInfo (Flagge, Gast-Status) auflösen lässt.
    const bool networkGame = m_session && m_session->isNetworkClientRunning();
    if (m_game) {
        PlayerList seats = m_game->getSeatsList();

        // Vorab-Durchlauf: Aktionswechsel erfassen, jeder Aktion eine fortlaufende
        // Sequenznummer geben und die jüngste Aggression (bet/raise) der Runde
        // merken – unabhängig von der Sitzreihenfolge.
        for (auto it = seats->begin(); it != seats->end(); ++it) {
            int id = (*it)->getMyID();
            if (id < 0 || id >= 10) continue;
            int act = (*it)->getMyAction();
            int curSet = (*it)->getMySet();
            // Frische Aktion = Aktionstyp ODER Einsatz hat sich geändert. So zählt
            // auch ein erneutes Callen nach einer Erhöhung (Typ bleibt CALL, Einsatz
            // steigt) als neue Aktion → das zuvor geleerte Badge erscheint wieder.
            if (act != m_lastSeenAction[id] || curSet != m_lastSeenSet[id]) {
                m_lastSeenAction[id] = act;
                m_lastSeenSet[id] = curSet;
                m_actionToken[id] = currentToken;
                m_actionSeq[id] = ++m_actionCounter;
                // Aggression (= alle anderen müssen erneut reagieren): bet/raise
                // immer; ein All-In nur, wenn sein Einsatz ÜBER dem aktuellen
                // Höchsteinsatz der übrigen Spieler liegt (echtes Erhöhen – ein
                // All-In-Call auf/unter dem Höchsteinsatz löst nicht aus).
                bool aggressive = (act == PLAYER_ACTION_BET || act == PLAYER_ACTION_RAISE);
                if (act == PLAYER_ACTION_ALLIN) {
                    int maxOtherSet = 0;
                    for (auto jt = seats->begin(); jt != seats->end(); ++jt) {
                        if (jt == it) continue;
                        int s = (*jt)->getMySet();
                        if (s > maxOtherSet) maxOtherSet = s;
                    }
                    aggressive = (curSet > maxOtherSet);
                }
                if (aggressive)
                    m_lastAggressorSeq = m_actionSeq[id];
            }
        }

        for (auto it = seats->begin(); it != seats->end(); ++it) {
            int id = (*it)->getMyID();
            // Spieler, der das Spiel verlassen hat: Sitz als leer behandeln, ihn
            // aber als "reserviert" markieren. Die Tischansicht kann den Sitz damit
            // – je nach Einstellung – als unsichtbaren Platzhalter im Ring halten,
            // sodass die verbleibenden Boxen nicht nachrücken.
            if (m_leftPlayers.contains((*it)->getMyUniqueID())) {
                if (id >= 0 && id < 10) {
                    QVariantMap gone = newPlayers[id].toMap();
                    gone["reserved"] = true;
                    newPlayers[id] = gone;
                }
                continue;
            }
            if (!(*it)->getMyName().empty() && (*it)->getMyType() == PLAYER_TYPE_HUMAN)
                ++humanCount;
            if (id >= 0 && id < 10) {
                int cards[2] = {-1, -1};
                (*it)->getMyCards(cards);
                const bool cardsKnown = cards[0] >= 0 && cards[1] >= 0;
                // Gegnerkarten nur im echten Showdown anzeigen – und nur für die
                // Spieler, die laut Engine aufdecken müssen (wie im Widgets-Client:
                // nicht gefoldet UND checkIfINeedToShowCards()). Das Showdown-Flag
                // verhindert, dass die noch veraltete playerNeedToShowCards-Liste
                // während der River-Setzrunde der nächsten Hand fälschlich aufdeckt.
                // Fold-/Aufdeck-Zustand über den Showdown-Snapshot (die nächste Hand
                // kann getMyAction()/playerNeedToShowCards längst zurückgesetzt haben
                // – siehe captureShowdownSnapshot()).
                const bool isFolded = showdownFolded((*it)->getMyUniqueID(),
                                                     (*it)->getMyAction() == PLAYER_ACTION_FOLD);
                const bool showdownReveal = m_showdownActive
                                            && !isFolded
                                            && showdownNeedsToShow((*it)->getMyUniqueID(),
                                                                   (*it)->checkIfINeedToShowCards());
                // All-In-Aufdeckung: Karten sind nach AllInShowCardsMessage für alle
                // nicht-gefoldeten Spieler sichtbar (bis zur nächsten Hand).
                const bool allInReveal = m_allInRevealed && !isFolded;
                // Freiwilliges Zeigen nach der Hand (AfterHandShowCards): der Spieler
                // muss nicht zwingend in playerNeedToShowCards stehen (Gewinn ohne
                // Showdown), also separat aufdecken.
                const bool postRiverShown = m_postRiverShownPlayers.contains((*it)->getMyUniqueID());
                const bool faceUp = cardsKnown && (id == 0 || showdownReveal || allInReveal || postRiverShown);
                if (id != 0 && m_allInRevealed) {
                    qDebug() << "[ALLIN] refreshPD seat" << id
                             << "cardsKnown=" << cardsKnown
                             << "cards=" << cards[0] << "/" << cards[1]
                             << "action=" << (int)(*it)->getMyAction()
                             << "allInReveal=" << allInReveal
                             << "faceUp=" << faceUp;
                }

                // Im Showdown werden ALLE Aktions-Badges entfernt (auch All-In und
                // Fold) – jetzt zählen nur noch aufgedeckte Karten, Gewinner-Hand
                // und Sieger.
                // Sonst: All-In bleibt die ganze Hand über sichtbar (die Engine
                // behält PLAYER_ACTION_ALLIN über alle Runden bei und setzt es erst
                // zur nächsten Hand zurück). Übrige Aktionen verschwinden zu Runden-
                // beginn (Token-Logik) und sobald ein anderer Spieler bet/raise
                // gesetzt hat (Sequenz < letzte Aggression).
                int act = (*it)->getMyAction();
                const bool sameRound = (currentToken >= 0 && m_actionToken[id] == currentToken);
                int displayAction;
                if (m_showdownActive || allInReveal || postRiverShown) {
                    // Showdown, All-In-Runout und freiwilliges Zeigen (Karten
                    // aufgedeckt): Badge entfernen, damit die aufgedeckten Karten
                    // nicht verdeckt werden.
                    displayAction = 0;
                } else if (act == PLAYER_ACTION_ALLIN) {
                    displayAction = act;
                } else if (act == PLAYER_ACTION_FOLD) {
                    // "Fold" bleibt für die Runde stehen – eine spätere bet/raise
                    // eines anderen Spielers entfernt es nicht (nur nicht-gefoldete
                    // Spieler müssen erneut handeln).
                    displayAction = sameRound ? act : 0;
                } else if (sameRound && m_actionSeq[id] >= m_lastAggressorSeq) {
                    displayAction = act;
                } else {
                    displayAction = 0;
                }

                QVariantMap p;
                p["name"]   = QString::fromStdString((*it)->getMyName());
                p["stack"]  = (*it)->getMyCash();
                p["bet"]    = (*it)->getMySet();
                p["active"] = (*it)->getMyActiveStatus();
                p["myTurn"] = (*it)->getMyTurn();
                p["seatId"] = id;
                // Dealer/Small-/Big-Blind nur für aktive Spieler: ausgeschiedene
                // (0 Coins, raus) behalten sonst ihr altes BB/SB/D-Icon bis zur
                // nächsten Hand. All-In-Spieler bleiben aktiv und behalten es korrekt.
                p["button"] = (*it)->getMyActiveStatus() ? (*it)->getMyButton() : BUTTON_NONE;
                p["action"] = displayAction;
                // Gefoldete Spieler bleiben die ganze Hand über gefoldet → Karten
                // durchscheinend darstellen (wie im Qt-Widgets-Client).
                p["folded"] = isFolded;
                // Computer-Gegner: für sie gibt es keine Kontextaktionen
                // (Ignore/Stats) – wie im Qt-Widgets-Client (MyAvatarLabel).
                p["isComputer"] = ((*it)->getMyType() == PLAYER_TYPE_COMPUTER);
                // Lokales Spiel: keine Netzwerk-Spieler-Id, keine Flagge.
                p["playerId"] = 0;
                p["countryCode"] = QString("");
                p["isGuest"] = false;
                // Netzwerkspiel: Spieler-Id, Länderflagge und Gast-Status direkt
                // über die eindeutige Spieler-Id aus der Session – genau wie
                // gameTableImpl::refreshPlayerAvatar() im Qt-Widgets-Client. Der
                // Umweg über die Spielerliste des Spiels (gamePlayersInGame) ist
                // am Tisch unzuverlässig: Während des Spiels ist der Client von
                // den Lobby-Nachrichten abgemeldet, ein GamePlayerJoined trägt
                // den Spieler NICHT in die GameInfo-Spielerliste nach. Wer erst
                // während des laufenden Spiels an den Tisch kommt (Zuschauer,
                // den der Server zur nächsten Hand setzt), fehlt dort also –
                // seine Flagge blieb aus, obwohl der Warteraum sie noch zeigte.
                if (networkGame) {
                    const unsigned uniqueId = (*it)->getMyUniqueID();
                    const PlayerInfo info = m_session->getClientPlayerInfo(uniqueId);
                    p["playerId"] = uniqueId;
                    p["countryCode"] = QString::fromStdString(info.countryCode).toLower();
                    p["isGuest"] = info.isGuest;
                }
                // Avatar (gesetzter Spieler-Avatar); Sitz 0 notfalls aus der
                // Config – aber nur, wenn ich selbst dort sitze. Als Zuschauer
                // ist Sitz 0 ein fremder Spieler und bekäme sonst MEINEN Avatar.
                std::string avatarRaw = (*it)->getMyAvatar();
                if (avatarRaw.empty() && id == 0 && !m_spectating && m_config)
                    avatarRaw = m_config->readConfigString("MyAvatar");
                p["avatar"] = resolveAvatarSource(avatarRaw);
                p["card0"]  = faceUp ? cards[0] : -1;
                p["card1"]  = faceUp ? cards[1] : -1;
                // Showdown-Spotlight: Hole-Card des Gewinners abblenden, wenn sie
                // nicht zu seinem besten Blatt zählt (Mengen werden im Showdown
                // gefüllt, sonst leer → fade0/fade1 false).
                p["fade0"]  = m_holeFade0.contains(id);
                p["fade1"]  = m_holeFade1.contains(id);
                if (id == 0) {
                    // qDebug() << "[DBG] seat0 cards:" << cards[0] << cards[1]
                    //          << "faceUp:" << faceUp;
                }
                newPlayers[id] = p;
            }
        }
    }

    // Chat-Icon nur, wenn außer mir noch ein menschlicher Spieler dabei ist.
    const bool newHasHumanOpponents = humanCount > 1;
    if (newHasHumanOpponents != m_hasHumanOpponents) {
        m_hasHumanOpponents = newHasHumanOpponents;
        emit hasHumanOpponentsChanged();
    }

    m_players = newPlayers;
    emit playersChanged();

    // Chancen + aktuelles Blatt nachführen (Fold-/Aktiv-Wechsel, neue Hole-Cards).
    refreshChanceAndHand();

    // Lokales Spiel: Spieler mit 0 Coins nach 10 Sekunden ausblenden.
    checkBustedLocalPlayers();
}

void GameHandler::refreshPotData()
{
    if (!m_game) return;
    auto hand = m_game->getCurrentHand();
    if (!hand) return;
    auto board = hand->getBoard();
    if (!board) return;

    int newPot = board->getPot();
    if (newPot != m_pot) {
        m_pot = newPot;
        emit potChanged();
    }

    int newTotalPot = board->getPot() + board->getSets();
    if (newTotalPot != m_totalPot) {
        m_totalPot = newTotalPot;
        emit totalPotChanged();
    }
}

void GameHandler::computeCallAndRaiseAmounts()
{
    int newCallAmount = 0;
    int newMinRaise = 0;
    int newMaxRaise = 0;
    bool newCanAct = false;
    int dbgMyAction = -1;   // [ACTDBG] zuletzt gelesene Engine-Aktion (für Log)
    int dbgPrevId   = -99;  // [ACTDBG] getPreviousPlayerID() (für Log)
    int dbgHandId   = -1;   // [ACTDBG] aktuelle Hand-ID (für Log)
    bool dbgRoundClosed = false;  // [ACTDBG] Setzrunde abgeschlossen (für Log)

    // Als Zuschauer gibt es keinen eigenen Sitz: Call-/Raise-Beträge bleiben 0
    // und canAct false (die GamePage blendet die Action-Bar ohnehin aus).
    if (m_game && !m_spectating) {
        auto hand = m_game->getCurrentHand();
        if (hand) {
            auto bero = hand->getCurrentBeRo();
            auto seats = hand->getSeatsList();
            if (bero && seats && !seats->empty()) {
                auto humanPlayer = seats->front();
                const int highestSet = bero->getHighestSet();
                const int humanSet = humanPlayer->getMySet();
                const int humanCash = humanPlayer->getMyCash();

                // Buttons aktiv (für Vor-Auswahl ODER echten Zug), wenn ich in
                // der Hand und nicht all-in bin UND entweder gerade am Zug bin
                // (m_myTurn) ODER NICHT der zuletzt handelnde Spieler war.
                //
                // Das spiegelt exakt gameTableImpl::updateMyButtonsState() der
                // Widgets-Referenz: dort sind die Buttons „checkable" (Vorwahl)
                // solange getPreviousPlayerID() != 0 (= ich war nicht der letzte
                // Akteur). getMyAction() == NONE taugt NICHT als Kriterium: nach
                // meiner Aktion (CALL/CHECK) und einer anschließenden Erhöhung
                // eines Gegners muss ich erneut handeln können – getMyAction()
                // ist dann aber bereits != NONE, sodass die Vorwahl fälschlich
                // gesperrt blieb, bis ich am Zug bin. previousPlayerID wird beim
                // Rundenwechsel/Deal auf -1 gesetzt (Vorwahl bleibt offen, kein
                // Flackern) und nach jeder Aktion auf den Akteur – nach MEINER
                // Aktion also auf 0, was die Buttons sauber abschaltet, bis ein
                // Gegner handelt.
                const int myAction = humanPlayer->getMyAction();
                const int prevPlayerId = hand->getPreviousPlayerID();
                dbgMyAction = myAction;
                dbgPrevId   = prevPlayerId;
                dbgHandId   = hand->getMyID();
                const bool baseEligible =
                            myAction != PLAYER_ACTION_FOLD
                            && myAction != PLAYER_ACTION_ALLIN
                            && humanCash > 0
                            && humanPlayer->isSessionActive();

                // Setzrunde abgeschlossen? Nach der ersten Setzrunden-Runde
                // (!firstRound) und sobald alle noch laufenden Spieler den
                // höchsten Einsatz erreicht haben (allHighestSet), ist die Runde
                // entschieden – exakt das Kriterium aus LocalBeRo::run(). In
                // diesem Zeitfenster (letzte Aktion erfolgt, nächste Runde/Hand
                // noch nicht gestartet) darf KEINE Vorauswahl mit veralteten
                // Werten aktiv bleiben → Buttons inaktiv. Nicht greifen, wenn ich
                // selbst gerade am Zug bin (z.B. abschließender Check als letzter
                // Spieler) – das erledigt der reguläre m_myTurn-Pfad.
                //
                // „Erste Setzrunden-Runde vorbei?" liefert lokal die Engine über
                // getFirstRound(). Im NETZWERK-Client wird firstRound jedoch NIE
                // auf false gesetzt (clientstate.cpp ruft setFirstRound(false)
                // nirgends auf) → roundClosed wäre online immer false, canAct bliebe
                // nach dem letzten Zug true und bettingRoundEnded feuerte nie. Das
                // ist genau das 1–2-Sekunden-Fenster (letzte Aktion → nächste Runde),
                // in dem online weiterhin (mit veralteten Werten) vorgewählt werden
                // konnte. Netzwerktauglicher Ersatz: die Runde ist erst „vorbei",
                // wenn ALLE noch laufenden Spieler in dieser Setzrunde bereits
                // gehandelt haben (getMyAction() != NONE; zu Rundenbeginn setzt
                // ResetPlayerActions() alle auf NONE, lokal LocalBeRo::run()). Beide
                // Kriterien werden am selben Punkt wahr (letzter Spieler handelt),
                // schließen aber den Fehlalarm am Rundenbeginn aus (alle Sets ==
                // highestSet, BEVOR jemand gehandelt hat). Das OR lässt das lokale
                // Verhalten unverändert: !firstRound wird dort sogar früher wahr.
                bool roundClosed = false;
                if (!m_myTurn) {
                    auto running = hand->getRunningPlayerList();
                    if (running && !running->empty()) {
                        bool allRunningActed = true;
                        for (auto it = running->begin(); it != running->end(); ++it) {
                            if ((*it)->getMyAction() == PLAYER_ACTION_NONE) {
                                allRunningActed = false;
                                break;
                            }
                        }
                        if (!bero->getFirstRound() || allRunningActed) {
                            // Abgeschlossen, wenn alle laufenden Spieler den höchsten
                            // Einsatz erreicht haben. Im Übergang zur nächsten Runde
                            // sammelt der Client die Sets aber bereits in den Pot ein
                            // (alle mySet == 0), während highestSet noch kurz auf dem
                            // alten Wert steht – dann lieferte der reine Vergleich mit
                            // highestSet fälschlich „nicht abgeschlossen": canAct
                            // flackerte zurück auf true und zeigte für 1–2 s veraltete
                            // Call-/Raise-Werte mit (re-)aktivierten Buttons. Daher
                            // gilt der eingesammelte Zustand (alle Sets == 0) ebenfalls
                            // als abgeschlossen. Beide Fälle sind durch
                            // allRunningActed/!firstRound abgesichert, sodass der
                            // Rundenbeginn (Sets == 0, aber Aktionen frisch auf NONE)
                            // NICHT fälschlich als abgeschlossen gilt. Der heikle
                            // Heads-up-All-In-über-Bet-Fall bleibt korrekt: dort ist
                            // mein Set != 0 und != highestSet → weder allMatched noch
                            // allCollected, also nicht abgeschlossen (und es ist eh
                            // mein Zug → m_myTurn-Wächter greift).
                            bool allMatched = true;
                            bool allCollected = true;
                            for (auto it = running->begin(); it != running->end(); ++it) {
                                if ((*it)->getMySet() != highestSet) allMatched = false;
                                if ((*it)->getMySet() != 0)          allCollected = false;
                            }
                            roundClosed = allMatched || allCollected;
                        }
                    }
                }
                dbgRoundClosed = roundClosed;

                newCanAct = baseEligible
                            && (m_myTurn || prevPlayerId != 0)
                            && !m_showdownActive
                            && !roundClosed;

                if (humanCash + humanSet <= highestSet) {
                    newCallAmount = humanCash;
                } else {
                    newCallAmount = highestSet - humanSet;
                }
                if (newCallAmount < 0) {
                    newCallAmount = 0;
                }

                const bool buttonsDisabled =
                    humanPlayer->getMyAction() == PLAYER_ACTION_ALLIN ||
                    humanPlayer->getMyAction() == PLAYER_ACTION_FOLD ||
                    humanCash == 0 ||
                    (humanSet == highestSet && humanPlayer->getMyAction() != PLAYER_ACTION_NONE) ||
                    !humanPlayer->isSessionActive();

                // Raise-Beträge NUR publizieren, solange ich überhaupt handeln
                // darf (newCanAct). Im Rundenübergang liefert die Formel sonst
                // scheinbar gültige, aber VERALTETE Werte: sind die Sets schon
                // in den Pot eingesammelt (mySet == 0), BeRo/round aber noch
                // Preflop (Netzwerk: collectPot() vor dem Rundenwechsel), ergibt
                // minimum = highestSet + minimumRaise = 2×BB. QML sät daraus den
                // Default-Einsatz (syncRaiseAmount/Selbstheilung in GameActionBar)
                // und klemmt ihn später nur noch auf [min,max] – der veraltete
                // Default („Bet $2×BB" statt $BB am Flop) blieb so stehen.
                // newCanAct ist in genau diesen Fenstern false (roundClosed/
                // Showdown/bereits gehandelt) – dann gibt es auch keine gültigen
                // Raise-Beträge.
                if (newCanAct && !buttonsDisabled && !bero->getFullBetRule()) {
                    int minimum = 0;
                    bool canBetRaise = false;

                    if (hand->getCurrentRound() == 0) {
                        if (humanCash + humanSet > highestSet) {
                            minimum = highestSet - humanSet + bero->getMinimumRaise();
                            canBetRaise = true;
                        }
                    } else {
                        if (highestSet == 0) {
                            minimum = hand->getSmallBlind() * 2;
                            canBetRaise = true;
                        } else if (highestSet > humanSet && humanCash + humanSet > highestSet) {
                            minimum = highestSet - humanSet + bero->getMinimumRaise();
                            canBetRaise = true;
                        }
                    }

                    if (canBetRaise) {
                        if (minimum < 0) {
                            minimum = 0;
                        }
                        newMaxRaise = humanCash;
                        newMinRaise = std::min(minimum, newMaxRaise);
                    }
                }
            }
        }
    }

    // VERDACHT: myTurn=true aber Engine zeigt uns bereits als Fold/AllIn → stale Daten?
    if (m_myTurn && (dbgMyAction == PLAYER_ACTION_FOLD || dbgMyAction == PLAYER_ACTION_ALLIN)) {
        qDebug() << "[ACTDBG] SUSPECT: myTurn=true but myAction=" << dbgMyAction
                 << "(FOLD=1,ALLIN=6) handId=" << dbgHandId
                 << "prevId=" << dbgPrevId << "newCanAct=" << newCanAct;
    }
    // Flanke „Setzrunde gerade entschieden": exakt jetzt (letzte Aktion erfolgt,
    // nächste Runde/Hand startet erst noch) müssen die Aktions-Buttons sofort
    // inaktiv werden und veraltete Vorwahl/Beträge verworfen werden. roundClosed
    // ist nur in genau diesem Recompute true; danach (neue BeRo, firstRound wieder
    // true) fällt es zurück, daher Flankenerkennung über m_roundClosed.
    if (dbgRoundClosed && !m_roundClosed) {
        m_roundClosed = true;
        qDebug() << "[ACTDBG] bettingRoundEnded (roundClosed rising)"
                 << "handId=" << dbgHandId << "prevPlayerId=" << dbgPrevId;
        emit bettingRoundEnded();
    } else if (!dbgRoundClosed) {
        m_roundClosed = false;
    }
    if (newCanAct != m_canAct) {
        m_canAct = newCanAct;
        qDebug() << "[ACTDBG] canAct=" << m_canAct << "prevPlayerId=" << dbgPrevId
                 << "myAction=" << dbgMyAction
                 << "(NONE=0,FOLD=1,CHK=2,CALL=3,BET=4,RAISE=5,ALLIN=6)"
                 << "handId=" << dbgHandId << "roundClosed=" << dbgRoundClosed
                 << "myTurn=" << m_myTurn << "tSeat=" << m_timeoutSeatId;
        emit canActChanged();
    }
    if (newCallAmount != m_callAmount) {
        m_callAmount = newCallAmount;
        emit callAmountChanged();
    }
    if (newMinRaise != m_minRaiseAmount) {
        m_minRaiseAmount = newMinRaise;
        emit minRaiseAmountChanged();
    }
    if (newMaxRaise != m_maxRaiseAmount) {
        m_maxRaiseAmount = newMaxRaise;
        emit maxRaiseAmountChanged();
    }

    // Zentraler Nachführpunkt: computeCallAndRaiseAmounts() hängt an JEDEM
    // Refresh-Pfad (onRefreshSet/Cash/Pot/GameLabels, onMeInAction, doActionDone),
    // also auch an dem onRefreshSet, das die PlayersTurnMessage auslöst –
    // currentPlayersTurnId ist dort bereits gesetzt.
    updateAwaitingMyAction();
}

bool GameHandler::humanCanAct() const
{
    // Als Zuschauer gibt es keinen eigenen Sitz. Ohne diesen Wächter läse die
    // Funktion unten seats->front() – den FREMDEN Spieler auf Sitz 0 – und
    // startTimeoutAnimation() machte daraus fälschlich "ich bin am Zug".
    if (m_spectating) return false;
    if (!m_game) return false;
    auto hand = m_game->getCurrentHand();
    if (!hand) return false;
    auto seats = hand->getSeatsList();
    if (!seats || seats->empty()) return false;
    auto human = seats->front();
    if (!human) return false;
    const int a = human->getMyAction();
    return a != PLAYER_ACTION_FOLD
        && a != PLAYER_ACTION_ALLIN
        && human->getMyCash() > 0
        && human->isSessionActive();
}

bool GameHandler::isMyTurnToAct() const
{
    if (m_spectating) return false;
    if (m_session && m_session->isNetworkClientRunning()) {
        // Netzwerk: das Zugfenster-Flag ist maßgeblich. Es ersetzt das frühere
        // implizite „nach der Aktion sind beide Flags gelöscht" und erlaubt
        // zugleich die autoritative Engine-Quelle als dritten Öffner.
        return !m_myTurnWindowClosed
               && (m_myTurn || m_timeoutSeatId == 0 || engineAwaitsMyAction());
    }
    // Lokales Spiel: unverändert. Dort gibt es keinen Zugzeiger vom Server, der
    // ein Fenster wieder öffnen könnte – m_myTurn/m_timeoutSeatId bleiben die
    // einzige Quelle (gesetzt von LocalBeRo über meInAction/startTimeoutAnimation).
    return m_myTurn || m_timeoutSeatId == 0;
}

bool GameHandler::engineTurnPointsAtMe() const
{
    // REINER Zeigervergleich, absichtlich ohne humanCanAct(): nur so ist der
    // Wert monoton und taugt als Flanke zum ÖFFNEN eines Zugfensters. Nähme man
    // humanCanAct() mit hinein, könnte ein bloßer Zustandswechsel des eigenen
    // Spielers (Aktion per ResetPlayerActions auf NONE zurück, Cash nach
    // gewonnenem Pot wieder > 0) ein längst geschlossenes Fenster wieder
    // aufreißen, obwohl gar keine neue PlayersTurnMessage kam.
    //
    // Nur im Netzwerk-Spiel: dort vergibt der Server die Unique-IDs und 0 ist
    // ausdrücklich ungültig (serverlobbythread.cpp), sodass der Startwert eines
    // frischen BeRo (currentPlayersTurnId == 0) nie versehentlich auf mich
    // passt. Im lokalen Spiel ist die eigene Unique-ID dagegen 0 – dort wäre
    // der Startwert von meiner echten Zugmarkierung nicht zu unterscheiden.
    if (m_spectating) return false;
    if (!m_session || !m_session->isNetworkClientRunning()) return false;
    if (!m_game) return false;
    auto hand = m_game->getCurrentHand();
    if (!hand) return false;
    auto bero = hand->getCurrentBeRo();
    if (!bero) return false;
    auto seats = hand->getSeatsList();
    if (!seats || seats->empty()) return false;
    auto human = seats->front();
    if (!human) return false;
    return bero->getCurrentPlayersTurnId() == human->getMyUniqueID();
}

bool GameHandler::engineAwaitsMyAction() const
{
    // Offenes Zugfenster = der Zugzeiger steht auf mir, ich kann überhaupt
    // handeln, und das Fenster wurde nicht bereits geschlossen.
    return engineTurnPointsAtMe() && !m_myTurnWindowClosed && humanCanAct();
}

void GameHandler::openMyTurnWindow()
{
    if (!m_myTurnWindowClosed) return;
    m_myTurnWindowClosed = false;
    updateAwaitingMyAction();
}

void GameHandler::closeMyTurnWindow()
{
    if (m_myTurnWindowClosed) return;
    m_myTurnWindowClosed = true;
    updateAwaitingMyAction();
}

void GameHandler::updateAwaitingMyAction()
{
    // Steigende Flanke des Zugzeigers = eine PlayersTurnMessage für mich ist
    // eingetroffen → neues Fenster. Fällt der Zeiger weg (anderer Spieler dran,
    // neue Setzrunde/Hand mit frischem BeRo), bleibt das Fenster geschlossen,
    // bis es wieder auf mich zeigt.
    const bool pointsAtMe = engineTurnPointsAtMe();
    if (pointsAtMe != m_engineTurnPointedAtMe) {
        m_engineTurnPointedAtMe = pointsAtMe;
        m_myTurnWindowClosed = !pointsAtMe;
    }
    const bool waiting = pointsAtMe && !m_myTurnWindowClosed && humanCanAct();
    if (waiting != m_awaitingMyAction) {
        m_awaitingMyAction = waiting;
        emit awaitingMyActionChanged();
    }
}

void GameHandler::doActionDone()
{
    if (!m_session) return;
    if (localGameCallbacksBlocked()) return;

    // Aktion für dieses Zugfenster ist raus → Fenster zu. Das ersetzt für die
    // engine-basierte Zugerkennung das, was das Löschen der beiden Flags unten
    // bisher implizit erledigte: kein zweiter Zug im selben Fenster.
    closeMyTurnWindow();

    if (m_myTurn) {
        m_myTurn = false;
        emit myTurnChanged();
    }
    // Mein Aktionsfenster schließen: solange m_timeoutSeatId == 0 gälte ich über
    // isMyTurnToAct() weiter als „am Zug" → eine zweite Aktion könnte durchrutschen.
    // Jetzt, da gehandelt, sofort beenden (stopTimeoutAnimation folgt ohnehin).
    if (m_timeoutSeatId == 0) {
        m_timeoutSeatId = -1;
        emit timeoutChanged();
    }
    qDebug() << "[ACTDBG] doActionDone sent, net="
             << (m_session && m_session->isNetworkClientRunning());

    // Ich habe gehandelt → Buttons sofort inaktiv schalten. canAct leitet das
    // aus getMyAction() (!= NONE) ab; die Aktion ist zu diesem Zeitpunkt bereits
    // auf dem Spieler gesetzt (fold/call/raise), daher liefert das Recompute den
    // korrekten Wert (inaktiv bis zur nächsten Runde bzw. bis ich erneut dran bin).
    computeCallAndRaiseAmounts();

    if (m_session->isNetworkClientRunning()) {
        // Network game: send action to server
        m_session->sendClientPlayerAction();
    } else {
        // Local game: advance game loop (equivalent to Qt5 nextPlayerAnimation -> switchRounds)
        boost::shared_ptr<Game> game = m_game;
        QTimer::singleShot(300, this, [game]() {
            if (game && game->getCurrentHand())
                game->getCurrentHand()->switchRounds();
        });
    }
}

void GameHandler::setShowdownActive(bool active)
{
    if (m_showdownActive == active)
        return;
    m_showdownActive = active;
    emit showdownActiveChanged();
}

// ─── slots called from QmlGuiInterface ──────────────────────────────────────

void GameHandler::onRefreshSet()
{
    if (localGameCallbacksBlocked()) return;
    qDebug() << "[ACTDBG] >> onRefreshSet myTurn=" << m_myTurn;
    refreshPlayerData();
    computeCallAndRaiseAmounts();
}

void GameHandler::onRefreshAction(int playerId, int playerAction)
{
    if (localGameCallbacksBlocked()) return;
    qDebug() << "[ACTDBG] >> onRefreshAction id=" << playerId << "act=" << playerAction << "myTurn=" << m_myTurn;
    refreshPlayerData();
    computeCallAndRaiseAmounts();

    // Entspricht gametableimpl::refreshAction: nur bei spezifischer Aktion
    // (nicht bei globalem Refresh) den Aktionssound abspielen.
    if (playerId < 0 || playerAction <= 0 || playerAction > 6)
        return;
    // Echte Spieler-Aktion (CHECK/CALL/FOLD/BET/RAISE/ALLIN): Signal an QML
    emit refreshActionTriggered();
    if (!m_config || m_config->readConfigInt("PlayGameActions") == 0)
        return;

    static const char *kActionSounds[] = {
        "", "fold", "check", "call", "bet", "raise", "allin"
    };

    ensureSoundEventHandler();
    if (m_soundEventHandler)
        m_soundEventHandler->playSound(kActionSounds[playerAction], playerId);
}

void GameHandler::onRefreshCash()
{
    if (localGameCallbacksBlocked()) return;
    qDebug() << "[ACTDBG] >> onRefreshCash myTurn=" << m_myTurn;
    refreshPlayerData();
    computeCallAndRaiseAmounts();
}

void GameHandler::onRefreshPlayerName()
{
    if (localGameCallbacksBlocked()) return;
    refreshPlayerData();
}

void GameHandler::onRefreshPot()
{
    if (localGameCallbacksBlocked()) return;
    qDebug() << "[ACTDBG] >> onRefreshPot myTurn=" << m_myTurn;
    refreshPotData();
    refreshPlayerData();
    computeCallAndRaiseAmounts();
}

void GameHandler::onRefreshGameLabels(int gameState)
{
    if (localGameCallbacksBlocked()) return;
    QString newPhase;
    switch (gameState) {
    case 0:  newPhase = "Preflop"; break;
    case 1:  newPhase = "Flop";    break;
    case 2:  newPhase = "Turn";    break;
    case 3:  newPhase = "River";   break;
    default: newPhase = "";        break;
    }

    const bool phaseChanged = (newPhase != m_phaseText);
    if (phaseChanged) {
        // Vor Phasenwechsel myTurn zurücksetzen: onNextRoundCleanGui kommt via
        // QueuedConnection u.U. erst nach dieser synchronen Methode. Ohne Reset
        // wäre m_myTurn noch true → QML myTurnNow=true → Buttons enabled mit
        // veralteten Werten aus der vorherigen Runde.
        if (m_myTurn) {
            m_myTurn = false;
            emit myTurnChanged();
        }
        // Rundengrenze (Flop/Turn/River ausgeteilt): mein Zugfenster der
        // vorherigen Setzrunde ist vorbei. Erst die nächste PlayersTurnMessage
        // für mich öffnet wieder eines.
        closeMyTurnWindow();
        m_phaseText = newPhase;
    }

    bool handChanged = false;
    if (m_game) {
        auto hand = m_game->getCurrentHand();
        if (hand) {
            int newHandNum = hand->getMyID();
            if (newHandNum != m_handNumber) {
                m_handNumber = newHandNum;
                handChanged = true;
            }
        }
    }

    // ERST die Beträge neu berechnen, DANN die Runden-/Handgrenze signalisieren:
    // QML setzt bei phaseText-/handNumber-Wechsel raiseAmount auf 0 und füllt
    // ihn sofort wieder aus minRaiseAmount (Selbstheilung in GameActionBar).
    // Feuerten die Signale VOR dem Recompute (alte Reihenfolge), sah dieses
    // Re-Seeding noch die Werte der ALTEN Runde/Hand – und syncRaiseAmount()
    // senkt einen einmal gesetzten Betrag nie wieder auf das frische Minimum
    // ab (klemmt nur auf [min,max]). Folge: Default-Raise klebte z. B. am
    // Preflop-Minimum (2×BB) statt dem BB am Flop zu folgen.
    computeCallAndRaiseAmounts();
    if (phaseChanged)
        emit phaseTextChanged();
    if (handChanged)
        emit handNumberChanged();
    // Nach computeCallAndRaiseAmounts() sind alle Werte der neuen Runde korrekt
    // (switchRounds() ist bereits abgeschlossen). QML kann die Vorauswahl nun
    // freischalten – unabhängig davon, ob callAmountChanged gefeuert hat.
    if (phaseChanged)
        emit roundValuesReady();
}

void GameHandler::onMeInAction()
{
    qDebug() << "[ACTDBG] onMeInAction() blocked=" << localGameCallbacksBlocked()
             << "myTurn=" << m_myTurn << "tSeat=" << m_timeoutSeatId;
    if (localGameCallbacksBlocked()) return;
    // Maßgeblicher Zugstart der Engine → Fenster auf (siehe doActionDone).
    openMyTurnWindow();
    refreshPlayerData();
    // m_myTurn ZUERST setzen, DANN die Beträge berechnen. computeCallAndRaiseAmounts()
    // wertet roundClosed nur bei !m_myTurn aus – bin ich in Position (letzter Akteur)
    // und alle Gegner haben gecheckt, sind alle Sets gleich (postflop 0) → roundClosed
    // würde fälschlich true, newCanAct false und die Raise-Beträge blieben 0 (Slider/
    // Bet gesperrt, nur All-In möglich). Da es hier gerade MEIN Zug wird, ist m_myTurn=true
    // korrekt und schaltet die roundClosed-Fehlerkennung sauber ab.
    //
    // Das myTurnChanged-Signal ERST NACH dem Recompute feuern: QML reagiert darauf
    // (onMyTurnChanged) u.a. mit dem Fokus-Umschalten ins Betrag-Feld, sofern
    // raiseAvailable – das setzt korrekt berechnete min/maxRaiseAmount voraus.
    const bool becameMyTurn = !m_myTurn;
    if (becameMyTurn)
        m_myTurn = true;
    computeCallAndRaiseAmounts();
    if (becameMyTurn)
        emit myTurnChanged();
    qDebug() << "[ACTDBG] meInAction myTurn=" << m_myTurn << "tSeat=" << m_timeoutSeatId;
    if (m_game) {
        auto dh = m_game->getCurrentHand();
        if (dh) {
            auto db = dh->getCurrentBeRo();
            auto ds = dh->getSeatsList();
            if (db && ds && !ds->empty()) {
                auto hp = ds->front();
                qDebug() << "[ACTDBG]   amounts call=" << m_callAmount
                         << "minRaise=" << m_minRaiseAmount << "maxRaise=" << m_maxRaiseAmount
                         << "| mySet=" << hp->getMySet() << "highestSet=" << db->getHighestSet()
                         << "cash=" << hp->getMyCash() << "myButton=" << hp->getMyButton()
                         << "(1=D,2=SB,3=BB) myAction=" << hp->getMyAction()
                         << "round=" << dh->getCurrentRound()
                         << "fullBetRule=" << db->getFullBetRule()
                         << "minRaiseEngine=" << db->getMinimumRaise();
                qDebug() << "[BBDBG] onMeInAction BB-check:"
                         << "bbPosId=" << (int)db->getBigBlindPositionId()
                         << "sbPosId=" << (int)db->getSmallBlindPositionId()
                         << "p0UniqueId=" << hp->getMyUniqueID()
                         << "prevPlayerId=" << dh->getPreviousPlayerID()
                         << "firstRound=" << db->getFirstRound()
                         << "isP0BB=" << (hp->getMyUniqueID() == db->getBigBlindPositionId());
            }
        }
    }
    // Maßgeblicher „ich bin am Zug"-Punkt (wie meInAction im Widgets-Client):
    // hier – und nur hier – die vorgemerkte/automatische Aktion auslösen,
    // IMMER (auch wenn m_myTurn oben schon true war, z.B. via Action-Timer).
    emit meInActionTriggered();
}

void GameHandler::onDisableMyButtons()
{
    if (localGameCallbacksBlocked()) return;
    int dbgAct = -1;
    if (m_game) {
        auto hand = m_game->getCurrentHand();
        if (hand) {
            auto seats = hand->getSeatsList();
            if (seats && !seats->empty())
                dbgAct = seats->front()->getMyAction();
        }
    }
    qDebug() << "[ACTDBG] onDisableMyButtons myTurn=" << m_myTurn
             << "p0Action=" << dbgAct
             << "(NONE=0,FOLD=1,CHK=2,CALL=3,BET=4,RAISE=5,ALLIN=6)";
    if (m_myTurn) {
        m_myTurn = false;
        emit myTurnChanged();
    }
    // Der Server hat eine Aktion für MEINEN Sitz verbucht (eigener Zug, Timeout
    // mit Default-Aktion oder Auto-Fold) → mein Zugfenster ist definitiv vorbei.
    // Ohne dieses Schließen blieb awaitingMyAction nach einem Timeout bis zur
    // nächsten PlayersTurnMessage stehen und hielt die Buttons über das
    // Rundenende hinweg scharf.
    closeMyTurnWindow();
}

void GameHandler::onStartTimeoutAnimation(int playerNum, int timeoutSec)
{
    // Log VOR dem Guard, damit blockierte Aufrufe sichtbar sind
    if (playerNum == 0)
        qDebug() << "[ACTDBG] onStartTimeout(0) blocked=" << localGameCallbacksBlocked()
                 << "myTurn=" << m_myTurn << "humanCanAct=" << humanCanAct()
                 << "tSeat=" << m_timeoutSeatId << "timeoutSec=" << timeoutSec;
    if (localGameCallbacksBlocked()) return;

    // Der Server zählt jetzt MEINE Aktionszeit → Fenster auf.
    if (playerNum == 0)
        openMyTurnWindow();

    // Fortschrittsbalken (Ersatz fürs Action-Badge) für den gerade aktiven Sitz.
    if (m_timeoutSeatId != playerNum || m_timeoutSec != timeoutSec) {
        m_timeoutSeatId = playerNum;
        m_timeoutSec = timeoutSec;
        emit timeoutChanged();
    }

    // Der Server zählt jetzt die Aktionszeit für DIESEN Sitz herunter. Ist es
    // mein Sitz (0) und kann ich agieren, ist es definitiv mein Zug. m_myTurn
    // hier setzen (nicht erst in onMeInAction): startTimeoutAnimation kommt
    // VOR meInAction und markiert exakt das Fenster, in dem der Server auf
    // meine Aktion wartet. Sonst gab es ein Fenster, in dem die Buttons bereits
    // aktiv waren (canAct), aber m_myTurn noch false war → Klicks/Vorwahlen
    // wurden als „kein Zug" verworfen (fold/call/raise prüfen m_myTurn) und
    // liefen in den Timeout (Server-Auto-Check).
    if (playerNum == 0 && !m_myTurn && humanCanAct()) {
        m_myTurn = true;
        emit myTurnChanged();
    }
    updateAwaitingMyAction();
    if (playerNum == 0)
        qDebug() << "[ACTDBG] startTimeout seat0 myTurn=" << m_myTurn
                 << "humanCanAct=" << humanCanAct() << "tSeat=" << m_timeoutSeatId
                 << "awaiting=" << m_awaitingMyAction;

    // Wie im Widgets-Client: Ton erst nach 3 Sekunden Vorlauf – nur für mich
    // und nur, wenn ich noch am Zug bin (eine vorgemerkte Aktion kann oben
    // bereits synchron ausgeführt worden sein → dann kein Beep).
    if (playerNum == 0 && m_myTurn && timeoutSec >= 4)
        m_timeoutBeepTimer->start((timeoutSec - 3) * 1000);
}

void GameHandler::onStopTimeoutAnimation(int playerNum)
{
    if (m_timeoutSeatId == playerNum) {
        m_timeoutSeatId = -1;
        emit timeoutChanged();
    }
    m_timeoutBeepTimer->stop();

    // Mein Aktionsfenster ist vorbei (gehandelt oder Zug abgelaufen) → Zug
    // beenden, passend zum Setzen in onStartTimeoutAnimation.
    if (playerNum == 0 && m_myTurn) {
        m_myTurn = false;
        emit myTurnChanged();
    }
    // Timer auf meinem Sitz gestoppt = Fenster vorbei (gehandelt oder abgelaufen).
    if (playerNum == 0)
        closeMyTurnWindow();
    updateAwaitingMyAction();
}

void GameHandler::onNetworkGameEnded()
{
    // Aus dem Netzwerk-Spiel entfernt (Spielende, geschlossen, gekickt …):
    // den eigenen Zustand sauber zurücksetzen. Ohne dies bleiben m_myTurn und
    // m_game stale; eine spätere Aktion (z.B. Auto-Modus der ComboBox auf der
    // noch sichtbaren GamePage) würde fold()/call() mit gültig aussehender
    // Wache aufrufen und auf Engine-Seite einen null Game-shared_ptr
    // dereferenzieren.
    m_game.reset();
    if (m_myTurn) {
        m_myTurn = false;
        emit myTurnChanged();
    }
    if (m_timeoutSeatId != -1) {
        m_timeoutSeatId = -1;
        emit timeoutChanged();
    }
    closeMyTurnWindow();   // kein Spiel mehr → kein offenes Zugfenster
    m_timeoutBeepTimer->stop();
    if (m_pingState != 0 || m_pingAvg != -1) {
        m_pingState = 0;  // keine Netzwerkverbindung mehr → Ampel zurücksetzen
        m_pingAvg = m_pingMin = m_pingMax = -1;
        emit pingStateChanged();
    }
}

void GameHandler::onPingUpdate(int minPing, int avgPing, int maxPing)
{
    // Ampel-Schwellen wie der Qt-Widgets-Client (MyAvatarLabel::refreshPing):
    // ≤1000 ms grün, ≤2000 ms gelb, >2000 ms rot; negative Werte = keine Daten.
    int state;
    if (avgPing < 0)            state = 0;
    else if (avgPing <= 1000)   state = 1;
    else if (avgPing <= 2000)   state = 2;
    else                        state = 3;
    bool changed = false;
    if (state != m_pingState)   { m_pingState = state;  changed = true; }
    if (avgPing != m_pingAvg)   { m_pingAvg   = avgPing; changed = true; }
    if (minPing != m_pingMin)   { m_pingMin   = minPing; changed = true; }
    if (maxPing != m_pingMax)   { m_pingMax   = maxPing; changed = true; }
    if (changed)
        emit pingStateChanged();
}

void GameHandler::onNetClientPlayerLeft(unsigned uniquePlayerId, const QString &playerName, int removeReason)
{
    // Im Spielverlauf vermerken, ob der Spieler freiwillig gegangen, gekickt
    // oder die Verbindung verloren hat (removeReason aus clientstate.cpp).
    if (!playerName.isEmpty()) {
        QString line;
        switch (removeReason) {
        case NTF_NET_REMOVED_KICKED:
            line = playerName + QStringLiteral(" was kicked from the game");
            break;
        case NTF_NET_INTERNAL:
            line = playerName + QStringLiteral(" was disconnected");
            break;
        default: // NTF_NET_REMOVED_ON_REQUEST
            line = playerName + QStringLiteral(" has left the game");
            break;
        }
        appendGameLog(line, LogSitOut);
    }

    m_leftPlayers.insert(uniquePlayerId);
    refreshPlayerData();
    emit playersChanged();
}

void GameHandler::refreshSpectators()
{
    // Zuschauerliste des laufenden Spiels aus der Session lesen – analog zum
    // Qt-Widgets-Client (gameTableImpl::refreshSpectatorsDisplay).
    QStringList names;
    if (m_session && m_session->isNetworkClientRunning()) {
        const unsigned gameId = m_session->getClientCurrentGameId();
        if (gameId != 0) {
            const GameInfo info = m_session->getClientGameInfo(gameId);
            for (unsigned id : info.spectatorsDuringGame) {
                const PlayerInfo pi = m_session->getClientPlayerInfo(id);
                names << QString::fromUtf8(pi.playerName.c_str());
            }
            // Der Server schickt einem Zuschauer nur die JEWEILS ANDEREN
            // Zuschauer (AcceptNewSession sendet den eigenen SpectatorJoined an
            // alle übrigen Sessions). Ohne diese Ergänzung zeigte das Auge-Icon
            // dem einzigen Zuschauer eines Tisches "0".
            if (m_session->isClientSpectating()) {
                const PlayerInfo me = m_session->getClientPlayerInfo(
                    m_session->getClientUniquePlayerId());
                const QString myName = QString::fromUtf8(me.playerName.c_str());
                if (!myName.isEmpty())
                    names << myName;
            }
        }
    }
    if (names != m_spectatorNames) {
        m_spectatorNames = names;
        emit spectatorsChanged();
    }
}

void GameHandler::checkBustedLocalPlayers()
{
    // Nur im lokalen Spiel (kein Netzwerk-Client).
    if (!m_session || m_session->isNetworkClientRunning()) return;
    if (!m_game) return;

    PlayerList seats = m_game->getSeatsList();
    for (auto it = seats->begin(); it != seats->end(); ++it) {
        int id = (*it)->getMyID();
        // Sitz 0 = menschlicher Spieler, nie automatisch ausblenden.
        if (id <= 0) continue;
        // Leere Sitze überspringen.
        if ((*it)->getMyName().empty()) continue;

        unsigned uid = (*it)->getMyUniqueID();
        // Bereits verlassen → kein Timer nötig.
        if (m_leftPlayers.contains(uid)) continue;

        // Nur wirklich ausgeschiedene Spieler ausblenden: cash==0 UND nicht mehr
        // aktiv im Turnier. Ein All-In-Spieler hat mitten in der Hand ebenfalls
        // cash==0 (sein ganzer Stack liegt als Einsatz im Pot), behält aber
        // active==true und spielt noch um den Pot mit – der wird sonst nach 10 s
        // fälschlich vom Tisch entfernt, obwohl das Spiel noch läuft. Erst zum
        // nächsten Handstart setzt die Engine (game.cpp) cash==0-Spieler inaktiv.
        if ((*it)->getMyCash() == 0 && !(*it)->getMyActiveStatus()) {
            // Noch kein Timer für diesen Spieler: 10-Sekunden-Verzögerung starten.
            if (!m_bustedLocalTimers.contains(uid)) {
                QTimer *t = new QTimer(this);
                t->setSingleShot(true);
                connect(t, &QTimer::timeout, this, [this, uid]() {
                    m_bustedLocalTimers.remove(uid);
                    m_leftPlayers.insert(uid);
                    refreshPlayerData();
                });
                m_bustedLocalTimers.insert(uid, t);
                t->start(10000);
            }
        } else {
            // Spieler hat wieder Chips (z. B. Rebuy) → laufenden Timer verwerfen.
            if (m_bustedLocalTimers.contains(uid)) {
                delete m_bustedLocalTimers.take(uid);
            }
        }
    }
}

void GameHandler::onBlindsSet(int smallBlind)
{
    if (localGameCallbacksBlocked()) return;
    ensureSoundEventHandler();
    if (m_soundEventHandler)
        m_soundEventHandler->blindsWereSet(smallBlind);
}

void GameHandler::refreshBoardCards()
{
    if (!m_game) return;
    auto hand = m_game->getCurrentHand();
    if (!hand) return;
    auto board = hand->getBoard();
    if (!board) return;

    int raw[5] = {-1, -1, -1, -1, -1};
    board->getMyCards(raw);

    QVariantList newCards;
    for (int i = 0; i < 5; ++i)
        newCards.append(i < m_boardCardCount ? raw[i] : -1);

    if (newCards != m_boardCards) {
        m_boardCards = newCards;
        emit boardCardsChanged();
    }
    refreshChanceAndHand();
}

void GameHandler::refreshChanceAndHand()
{
    // Eigenen Spieler (Sitz 0) suchen. Als Zuschauer gibt es keinen – dann
    // bleibt die Chancen-/Blatt-Anzeige leer (haveCards == false).
    boost::shared_ptr<PlayerInterface> human;
    if (m_game && !m_spectating) {
        PlayerList seats = m_game->getSeatsList();
        if (seats) {
            for (auto it = seats->begin(); it != seats->end(); ++it) {
                if ((*it)->getMyID() == 0) { human = *it; break; }
            }
        }
    }

    int holeCards[2] = {-1, -1};
    int boardCards[5] = {-1, -1, -1, -1, -1};
    bool folded = false;
    bool haveCards = false;

    if (human && human->getMyActiveStatus()) {
        human->getMyCards(holeCards);
        haveCards = (holeCards[0] >= 0 && holeCards[1] >= 0);
        if (haveCards) {
            auto hand = m_game ? m_game->getCurrentHand() : nullptr;
            if (hand && hand->getBoard())
                hand->getBoard()->getMyCards(boardCards);
            folded = (human->getMyAction() == PLAYER_ACTION_FOLD);
        }
    }

    // Eingangssignatur: nur neu rechnen, wenn sich Karten/Board/Fold geändert haben.
    std::vector<int> inputs;
    inputs.reserve(9);
    inputs.push_back(haveCards ? holeCards[0] : -1);
    inputs.push_back(haveCards ? holeCards[1] : -1);
    inputs.push_back(m_boardCardCount);
    for (int i = 0; i < 5; ++i)
        inputs.push_back(haveCards ? boardCards[i] : -1);
    inputs.push_back(folded ? 1 : 0);
    if (inputs == m_lastChanceInputs)
        return;
    m_lastChanceInputs = inputs;

    QVariantList newChance;

    if (haveCards) {
        GameState gs = GAME_STATE_PREFLOP;
        if (m_boardCardCount >= 5)      gs = GAME_STATE_RIVER;
        else if (m_boardCardCount == 4) gs = GAME_STATE_TURN;
        else if (m_boardCardCount == 3) gs = GAME_STATE_FLOP;

        std::vector<std::vector<int>> chance =
            CardsValue::calcCardsChance(gs, holeCards, boardCards);
        if (chance.size() >= 2 && chance[0].size() >= 10 && chance[1].size() >= 10) {
            for (int cat = 0; cat < 10; ++cat) {
                QVariantMap m;
                m["prob"]     = chance[0][cat];
                m["possible"] = (chance[1][cat] != 0) && !folded;
                newChance.append(m);
            }
        }
    }

    if (newChance != m_cardsChance || folded != m_cardsChanceFolded) {
        m_cardsChance = newChance;
        m_cardsChanceFolded = folded;
        emit cardsChanceChanged();
    }
}

void GameHandler::onNextRoundCleanGui()
{
    if (localGameCallbacksBlocked()) return;
    onDisableMyButtons();
    m_pot = 0;
    m_totalPot = 0;
    emit potChanged();
    emit totalPotChanged();
    m_minRaiseAmount = 0;
    m_maxRaiseAmount = 0;
    emit minRaiseAmountChanged();
    emit maxRaiseAmountChanged();
    m_boardCardCount = 0;
    m_boardCards = QVariantList{-1, -1, -1, -1, -1};
    emit boardCardCountChanged();
    emit boardCardsChanged();
    if (!m_winnerSeatIds.isEmpty()) {
        m_winnerSeatIds.clear();
        emit winnerSeatIdsChanged();
    }
    if (!m_winningHandText.isEmpty()) {
        m_winningHandText.clear();
        emit winningHandTextChanged();
    }
    // Showdown-Spotlight für die neue Hand zurücksetzen (Hole- und Board-Fade).
    m_holeFade0.clear();
    m_holeFade1.clear();
    const QVariantList noBoardFade = {false, false, false, false, false};
    if (m_boardCardFade != noBoardFade) {
        m_boardCardFade = noBoardFade;
        emit boardCardFadeChanged();
    }
    // Showdown beenden, bevor die Spielerdaten neu gebaut werden → Karten zu.
    setShowdownActive(false);
    m_allInRevealed = false;
    m_postRiverShownPlayers.clear();
    // Showdown-Snapshot der Vorhand verwerfen → ab jetzt wieder Live-Zustand.
    m_foldedAtHandEnd.clear();
    m_needToShowAtHandEnd.clear();
    m_showdownSnapshotValid = false;
    if (m_canShowCards) {
        m_canShowCards = false;
        emit canShowCardsChanged();
    }
    refreshPlayerData();
    // Button-Zustand auffrischen: zum Hand-Ende/-Start ist getMyAction() noch
    // die letzte Aktion (!= NONE) → Buttons inaktiv, bis die Engine zur neuen
    // Hand auf NONE zurücksetzt.
    computeCallAndRaiseAmounts();
}

void GameHandler::onDealFlopCards()
{
    if (localGameCallbacksBlocked()) return;
    m_boardCardCount = 3;
    emit boardCardCountChanged();
    refreshBoardCards();
}

void GameHandler::onDealTurnCard()
{
    if (localGameCallbacksBlocked()) return;
    m_boardCardCount = 4;
    emit boardCardCountChanged();
    refreshBoardCards();
}

void GameHandler::onDealRiverCard()
{
    if (localGameCallbacksBlocked()) return;
    m_boardCardCount = 5;
    emit boardCardCountChanged();
    refreshBoardCards();
}

// ─── Q_INVOKABLE actions called from QML ────────────────────────────────────

void GameHandler::fold()
{
    qDebug() << "[FOLDDBG] fold() entry"
             << "myTurn=" << m_myTurn << "tSeat=" << m_timeoutSeatId
             << "isMyTurnToAct=" << isMyTurnToAct();
    if (!m_game || !m_session || !isMyTurnToAct()) {
        qDebug() << "[FOLDDBG] fold() EARLY-RETURN";
        return;
    }

    auto hand = m_game->getCurrentHand();
    if (!hand) return;
    auto seats = hand->getSeatsList();
    if (!seats || seats->empty()) return;
    auto humanPlayer = seats->front();

    qDebug() << "[FOLDDBG] fold() pre-dispatch"
             << "myButton=" << humanPlayer->getMyButton()
             << "round=" << (int)hand->getCurrentRound();

    // [RACEDBG] These mutate the shared Hand on the GUI thread. If a net-thread
    // "[RACEDBG] net: ... BEGIN/END" line appears interleaved with this one in
    // pokerth-debug.log (different thread id), the GUI- and network-thread are
    // touching the same Hand concurrently -> the suspected auto-fold freeze.
    qDebug() << "[RACEDBG] gui: fold() mutating Hand BEGIN";
    humanPlayer->setMyAction(PLAYER_ACTION_FOLD, true);
    humanPlayer->setMyTurn(false);
    hand->setPreviousPlayerID(0);
    qDebug() << "[RACEDBG] gui: fold() mutating Hand END";

    doActionDone();
}

bool GameHandler::call(int expectedAmount)
{
    qDebug() << "[CALLDBG] call() entry expected=" << expectedAmount
             << "myTurn=" << m_myTurn << "tSeat=" << m_timeoutSeatId
             << "isMyTurnToAct=" << isMyTurnToAct();
    if (!m_game || !m_session || !isMyTurnToAct()) {
        qDebug() << "[CALLDBG] call() EARLY-RETURN (game=" << (m_game ? 1 : 0)
                 << " session=" << (m_session ? 1 : 0)
                 << " turn=" << isMyTurnToAct() << ")";
        return false;
    }

    auto hand = m_game->getCurrentHand();
    if (!hand) return false;
    auto seats = hand->getSeatsList();
    if (!seats || seats->empty()) return false;
    auto humanPlayer = seats->front();
    auto bero = hand->getCurrentBeRo();
    if (!bero) return false;

    int highestSet = bero->getHighestSet();
    int humanSet = humanPlayer->getMySet();

    // Deckungsprüfung gegen den Betrag, den der Spieler gesehen hat (0 = Check).
    // Der Live-Zustand kann seit dem Klick/der Vorwahl vom Netz-Thread erhöht
    // worden sein; dann NICHTS senden (siehe Kommentar an der Deklaration).
    // Weniger als erwartet ist unkritisch (nie mehr Chips als gewollt) und wird
    // ausgeführt; −1 heißt „beliebiger Betrag" (Auto Check/Call).
    const int requiredAmount = (humanPlayer->getMyCash() + humanSet <= highestSet)
                               ? humanPlayer->getMyCash()
                               : std::max(0, highestSet - humanSet);
    if (expectedAmount >= 0 && requiredAmount > expectedAmount) {
        qDebug() << "[CALLDBG] call() REJECTED: required=" << requiredAmount
                 << "> expected=" << expectedAmount
                 << "(highestSet=" << highestSet << "mySet=" << humanSet << ")";
        emit actionRejected(requiredAmount, expectedAmount);
        return false;
    }

    qDebug() << "[CALLDBG] call() pre-dispatch"
             << "humanSet=" << humanSet << "highestSet=" << highestSet
             << "humanCash=" << humanPlayer->getMyCash()
             << "myButton=" << humanPlayer->getMyButton()
             << "(1=D,2=SB,3=BB)"
             << "round=" << (int)hand->getCurrentRound()
             << "myAction=" << humanPlayer->getMyAction();

    if (highestSet == 0 || humanSet >= highestSet) {
        // Check – entweder kein Einsatz gesetzt (highestSet == 0) ODER der
        // eigene Einsatz entspricht bereits dem höchsten (klassischer Fall:
        // BB-Option preflop, alle haben gelimpt). Server erwartet hier
        // explizit PLAYER_ACTION_CHECK; ein CALL ohne tatsächliche Chip-
        // Bewegung würde verworfen → Timeout mit Default-Action.
        humanPlayer->setMyAction(PLAYER_ACTION_CHECK, true);
        qDebug() << "[CALLDBG] call() -> CHECK branch";
    } else if (humanPlayer->getMyCash() + humanSet <= highestSet) {
        // All-in call
        humanPlayer->setMySet(humanPlayer->getMyCash());
        humanPlayer->setMyCash(0);
        humanPlayer->setMyAction(PLAYER_ACTION_ALLIN, true);
        qDebug() << "[CALLDBG] call() -> ALLIN branch lastRelSet=" << humanPlayer->getMyLastRelativeSet();
    } else {
        // Regular call
        humanPlayer->setMySet(highestSet - humanSet);
        humanPlayer->setMyAction(PLAYER_ACTION_CALL, true);
        qDebug() << "[CALLDBG] call() -> CALL branch lastRelSet=" << humanPlayer->getMyLastRelativeSet()
                 << "newMySet=" << humanPlayer->getMySet();
    }

    humanPlayer->setMyTurn(false);
    hand->getBoard()->collectSets();
    hand->setPreviousPlayerID(0);

    doActionDone();
    onRefreshPot();
    return true;
}

void GameHandler::raise(int amount)
{
    qDebug() << "[RAISEDBG] raise() entry amount=" << amount
             << "myTurn=" << m_myTurn << "tSeat=" << m_timeoutSeatId
             << "isMyTurnToAct=" << isMyTurnToAct();
    if (!m_game || !m_session || !isMyTurnToAct()) {
        qDebug() << "[RAISEDBG] raise() EARLY-RETURN";
        return;
    }

    auto hand = m_game->getCurrentHand();
    if (!hand) return;
    auto seats = hand->getSeatsList();
    if (!seats || seats->empty()) return;
    auto humanPlayer = seats->front();
    auto bero = hand->getCurrentBeRo();
    if (!bero) return;

    // Default to minimum raise if no amount specified
    if (amount <= 0) {
        amount = m_minRaiseAmount;
    }
    if (amount <= 0) {
        qDebug() << "[RAISEDBG] raise() amount<=0 after minRaise fallback (m_minRaiseAmount=" << m_minRaiseAmount << ") ABORT";
        return;
    }
    qDebug() << "[RAISEDBG] raise() pre-dispatch amount=" << amount
             << "humanSet=" << humanPlayer->getMySet()
             << "humanCash=" << humanPlayer->getMyCash()
             << "highestSet=" << bero->getHighestSet()
             << "minRaiseEngine=" << bero->getMinimumRaise()
             << "myButton=" << humanPlayer->getMyButton()
             << "round=" << (int)hand->getCurrentRound();

    int tempCash = humanPlayer->getMyCash();

    humanPlayer->setMySet(amount); // adds to set, deducts from cash

    if (amount >= tempCash) {
        // All-in
        humanPlayer->setMyCash(0);
        humanPlayer->setMyAction(PLAYER_ACTION_ALLIN, true);
        if (bero->getHighestSet() + bero->getMinimumRaise() > humanPlayer->getMySet()) {
            bero->setFullBetRule(true);
        }
        if (humanPlayer->getMySet() > bero->getHighestSet()) {
            bero->setMinimumRaise(humanPlayer->getMySet() - bero->getHighestSet());
            bero->setHighestSet(humanPlayer->getMySet());
            hand->setLastActionPlayerID(humanPlayer->getMyUniqueID());
        }
    } else {
        const bool firstBet = (bero->getHighestSet() == 0);
        humanPlayer->setMyAction(firstBet ? PLAYER_ACTION_BET : PLAYER_ACTION_RAISE, true);
        bero->setMinimumRaise(humanPlayer->getMySet() - bero->getHighestSet());
        bero->setHighestSet(humanPlayer->getMySet());
        hand->setLastActionPlayerID(humanPlayer->getMyUniqueID());
    }

    humanPlayer->setMyTurn(false);
    hand->getBoard()->collectSets();
    hand->setPreviousPlayerID(0);

    doActionDone();
    onRefreshPot();
}

void GameHandler::allIn()
{
    if (!m_game || !m_session || !isMyTurnToAct()) return;

    auto hand = m_game->getCurrentHand();
    if (!hand) return;
    auto seats = hand->getSeatsList();
    if (!seats || seats->empty()) return;
    auto humanPlayer = seats->front();
    auto bero = hand->getCurrentBeRo();
    if (!bero) return;

    humanPlayer->setMySet(humanPlayer->getMyCash());
    humanPlayer->setMyCash(0);
    humanPlayer->setMyAction(PLAYER_ACTION_ALLIN, true);

    if (bero->getHighestSet() + bero->getMinimumRaise() > humanPlayer->getMySet()) {
        bero->setFullBetRule(true);
    }
    if (humanPlayer->getMySet() > bero->getHighestSet()) {
        bero->setMinimumRaise(humanPlayer->getMySet() - bero->getHighestSet());
        bero->setHighestSet(humanPlayer->getMySet());
        hand->setLastActionPlayerID(humanPlayer->getMyUniqueID());
    }

    humanPlayer->setMyTurn(false);
    hand->getBoard()->collectSets();
    hand->setPreviousPlayerID(0);

    doActionDone();
    onRefreshPot();
}

void GameHandler::showMyCards()
{
    if (!m_canShowCards) return;
    if (m_session) m_session->showMyCards();
    m_canShowCards = false;
    emit canShowCardsChanged();
    // Visuelle Bestätigung in der Self-Box: eigene Karten „umdrehen" (wie der
    // Widget-Client via showHoleCards), damit man sieht, dass man wirklich zeigt.
    emit myCardsShown();
}

// ─── Local game startup ──────────────────────────────────────────────────────

void GameHandler::startLocalGame()
{
    if (!m_session) return;
    m_localGameExitRequested = false;

    GameData gameData;
    if (m_config) {
        gameData.maxNumberOfPlayers = m_config->readConfigInt("NumberOfPlayers");
        gameData.startMoney         = m_config->readConfigInt("StartCash");
        gameData.firstSmallBlind    = m_config->readConfigInt("FirstSmallBlind");
    }
    if (gameData.maxNumberOfPlayers < 2) gameData.maxNumberOfPlayers = 6;
    if (gameData.startMoney <= 0)        gameData.startMoney         = 1500;
    if (gameData.firstSmallBlind <= 0)   gameData.firstSmallBlind    = 10;

    // Defaults match Qt5 local game defaults
    gameData.raiseIntervalMode              = RAISE_ON_HANDNUMBER;
    gameData.raiseSmallBlindEveryHandsValue = 8;
    gameData.raiseMode                      = DOUBLE_BLINDS;
    gameData.guiSpeed                       = 4;
    gameData.delayBetweenHandsSec           = 7;
    gameData.playerActionTimeoutSec         = 20;

    StartData startData;
    startData.numberOfPlayers     = gameData.maxNumberOfPlayers;
    startData.startDealerPlayerId = 0;

    m_session->startLocalGame(gameData, startData);

    // Sync m_game so action methods and refreshes work immediately
    auto game = m_session->getCurrentGame();
    if (game) setGame(game);
}

void GameHandler::endLocalGame()
{
    if (!m_session) return;
    if (m_session->isNetworkClientRunning()) return;

    m_localGameExitRequested = true;
    m_game.reset();

    // Ausstehende Busted-Player-Timer abbrechen.
    qDeleteAll(m_bustedLocalTimers);
    m_bustedLocalTimers.clear();

    if (m_myTurn) {
        m_myTurn = false;
        emit myTurnChanged();
    }

    refreshPlayerData();
}

bool GameHandler::isLocalGameRunning() const
{
    if (m_localGameExitRequested) return false;
    if (!m_session) return false;
    if (m_session->isNetworkClientRunning()) return false;
    if (m_game) return true;
    return static_cast<bool>(m_session->getCurrentGame());
}

bool GameHandler::isInternetGameRunning() const
{
    return m_session
        && m_session->isNetworkClientRunning()
        && m_session->getGameType() == Session::GAME_TYPE_INTERNET;
}

void GameHandler::reportAvatar(int seatId)
{
    // Nur im Internet-Spiel sinnvoll (wie im Qt-Widgets-Client, MyAvatarLabel).
    if (!isInternetGameRunning() || !m_game)
        return;

    PlayerList seats = m_game->getSeatsList();
    for (auto it = seats->begin(); it != seats->end(); ++it) {
        if ((*it)->getMyID() != seatId)
            continue;
        // Avatar-Hash = Basisname der Avatardatei (ohne Pfad/Endung), exakt wie
        // MyAvatarLabel::reportBadAvatar den Wert an den Server übergibt.
        const std::string avatar = (*it)->getMyAvatar();
        if (avatar.empty())
            return;
        const QFileInfo fi(QString::fromStdString(avatar));
        m_session->reportBadAvatar((*it)->getMyUniqueID(),
                                   fi.baseName().toStdString());
        return;
    }
}

// ─── Game-loop advance slots (called via QMetaObject from QmlGuiInterface) ───

void GameHandler::onRunBeRo()
{
    if (localGameCallbacksBlocked()) return;
    if (!m_game) return;
    auto hand = m_game->getCurrentHand();
    if (hand && hand->getCurrentBeRo())
        hand->getCurrentBeRo()->run();
}

void GameHandler::onNextPlayerBeRo()
{
    if (localGameCallbacksBlocked()) return;
    if (!m_game) return;
    auto hand = m_game->getCurrentHand();
    if (hand && hand->getCurrentBeRo())
        hand->getCurrentBeRo()->nextPlayer();
}

// Called after the board cards of a street have been dealt. In an all-in
// condition there is no more betting (the running-player list is empty), so
// calling BeRo::run() would throw ERR_RUNNING_PLAYER_NOT_FOUND. Instead we
// advance straight to the next street/showdown via switchRounds(). In a normal
// (non-all-in) round we start the betting via BeRo::run().
void GameHandler::onAfterDealCards()
{
    if (localGameCallbacksBlocked()) return;
    if (!m_game) return;
    auto hand = m_game->getCurrentHand();
    if (!hand) return;

    if (hand->getAllInCondition()) {
        hand->switchRounds();
    } else if (hand->getCurrentBeRo()) {
        hand->getCurrentBeRo()->run();
    }
}

void GameHandler::onSwitchRounds()
{
    if (localGameCallbacksBlocked()) return;
    if (!m_game) return;
    auto hand = m_game->getCurrentHand();
    if (hand) hand->switchRounds();
}

void GameHandler::onPostRiverRunBeRo()
{
    if (localGameCallbacksBlocked()) return;
    if (!m_game) return;
    auto hand = m_game->getCurrentHand();
    if (hand && hand->getCurrentBeRo())
        hand->getCurrentBeRo()->postRiverRun();
}

// ─────────────────────────────────────────────────────────────────────────────
// Showdown-Snapshot
//
// Der Showdown darf den Fold-Zustand NICHT live aus den Player-Objekten lesen:
//
//   * Game::initHand() setzt zu Beginn JEDER Hand die Aktion ALLER Spieler auf
//     PLAYER_ACTION_NONE (game.cpp) – auch PLAYER_ACTION_FOLD. (Der Netz-Client
//     schützt FOLD in ClientStateRunHand::ResetPlayerActions() gezielt vor dem
//     Runden-Reset; initHand() tut das bewusst nicht.)
//   * Der Widgets-Client kann sich trotzdem auf getMyAction() verlassen, weil er
//     den Netz-Thread blockiert: waitForGuiUpdateDone() wartet dort auf eine
//     Semaphore, die erst prepareForNewHand() nach Ende der Post-River-Animation
//     freigibt (gametableimpl.cpp). initHand() läuft also garantiert später.
//   * Im QML-Client ist waitForGuiUpdateDone() ein No-op (qmlguiinterface.h) und
//     onShowdown() läuft per QueuedConnection. Im Netzwerkspiel startet der
//     Server die nächste Hand – initHand() kann die FOLD-Flags also längst
//     gelöscht haben, bevor onShowdown() drankommt.
//
// Folge ohne Snapshot: Beim Showdown gelten gefoldete Spieler als aktiv. Wer
// seine Karten kennt (im Netzwerkspiel mindestens man selbst), landet mit
// gewertetem Blattnamen im Spielverlauf – auch wenn dieses Blatt den echten
// Gewinner schlägt. Das sieht für den Spieler nach einem falschen Gewinner aus.
//
// Deshalb: Zustand am Hand-Ende einfrieren (synchron auf dem Netz-Thread, siehe
// QmlGuiInterface::postRiverRunAnimation1) und ab da nur noch den Snapshot lesen.
void GameHandler::captureShowdownSnapshot()
{
    m_foldedAtHandEnd.clear();
    m_needToShowAtHandEnd.clear();
    m_showdownSnapshotValid = false;
    if (!m_game) return;
    auto hand = m_game->getCurrentHand();
    if (!hand) return;
    auto activeList = hand->getActivePlayerList();
    if (!activeList) return;

    for (auto it = activeList->begin(); it != activeList->end(); ++it) {
        const unsigned uid = (*it)->getMyUniqueID();
        if ((*it)->getMyAction() == PLAYER_ACTION_FOLD)
            m_foldedAtHandEnd.insert(uid);
        // checkIfINeedToShowCards() liest board->getPlayerNeedToShowCards() –
        // dieselbe Liste überschreibt die nächste Hand ebenfalls.
        if ((*it)->checkIfINeedToShowCards())
            m_needToShowAtHandEnd.insert(uid);
    }
    m_showdownSnapshotValid = true;
}

bool GameHandler::showdownFolded(unsigned uniqueId, bool liveFolded) const
{
    return m_showdownSnapshotValid ? m_foldedAtHandEnd.contains(uniqueId) : liveFolded;
}

bool GameHandler::showdownNeedsToShow(unsigned uniqueId, bool liveNeedsToShow) const
{
    return m_showdownSnapshotValid ? m_needToShowAtHandEnd.contains(uniqueId) : liveNeedsToShow;
}

void GameHandler::onShowdown()
{
    if (localGameCallbacksBlocked()) return;
    if (!m_game) return;
    auto hand = m_game->getCurrentHand();
    if (!hand) return;
    auto board = hand->getBoard();
    if (!board) return;

    // Showdown ist jetzt aktiv → Gegnerkarten dürfen aufgedeckt werden
    // (determinePlayerNeedToShowCards() wurde in postRiverRun() bereits aufgerufen).
    setShowdownActive(true);
    // Während des Showdowns ist niemand am Zug. Ein eventuell noch gesetztes
    // m_myTurn (z. B. wenn der Zug über den Action-Timer aktiv war) würde sonst
    // die Aktions-Buttons in der QML-ActionBar weiter „armed" halten
    // (armed = myTurn || …) – die Buttons blieben im Showdown klickbar.
    if (m_myTurn) {
        m_myTurn = false;
        emit myTurnChanged();
    }
    closeMyTurnWindow();   // im Showdown wartet niemand mehr auf mich
    refreshPlayerData();
    computeCallAndRaiseAmounts(); // Buttons sofort deaktivieren

    // Pot ist bereits verteilt. Gewinner ermitteln und Haupt-/Side-Pot exakt
    // wie der Widgets-Client (gameTableImpl::postRiverRunAnimation3) trennen.
    std::list<unsigned> winners = board->getWinners();
    auto activeList = hand->getActivePlayerList();
    auto bero = hand->getCurrentBeRo();

    // Side-Pot-Kriterium exakt wie die Engine selbst (Log::logHandWinner): Hauptpot-
    // Gewinner = Geld-Gewinner mit dem besten Showdown-Blatt (cardsValueInt >=
    // highestCardsValue, das über alle nicht gefoldeten Spieler ermittelt wird). Geld-
    // Gewinner mit schlechterem Blatt haben nur einen Side-Pot gewonnen und bekommen
    // KEIN Winner-Badge. Bewusst OHNE getAllInCondition()-Gate: auf dem Netzwerk-Client
    // wird allInCondition nur bei komplettem All-In-Showdown gesetzt (per
    // AllInShowCardsMessage) und würde einen Side-Pot ohne globalen All-In – kurzer
    // Stack all-in, andere setzen normal weiter bis zum Showdown – nicht erkennen.
    const int highestWinnerCardsValue = bero ? bero->getHighestCardsValue() : 0;
    auto isMainPotWinner = [&](const auto &p) -> bool {
        const bool isW = std::find(winners.begin(), winners.end(), p->getMyUniqueID()) != winners.end();
        const bool hasActuallyWon = isW && p->getLastMoneyWon() > 0;
        if (showdownFolded(p->getMyUniqueID(), p->getMyAction() == PLAYER_ACTION_FOLD) || !hasActuallyWon)
            return false;
        if (highestWinnerCardsValue > 0 && p->getMyCardsValueInt() < highestWinnerCardsValue)
            return false; // Side-Pot-Gewinner
        return true;
    };

    // Winner-Badge: jeder nicht gefoldete HAUPTPOT-Gewinner (mehrere bei Split-
    // Pot). Side-Pot-Gewinner bekommen KEIN Badge – wie im Widgets-Client, der
    // das "Winner"-Label nur für isMainPot-Gewinner setzt.
    QVariantList newWinners;
    if (activeList) {
        for (auto it = activeList->begin(); it != activeList->end(); ++it)
            if (isMainPotWinner(*it))
                newWinners.append((*it)->getMyID());
    }
    if (newWinners != m_winnerSeatIds) {
        m_winnerSeatIds = newWinners;
        emit winnerSeatIdsChanged();
    }

    // Name der Gewinner-Hand ermitteln (wie label_WinningCombination im Widgets-
    // Client). Nur sinnvoll, wenn es einen echten Showdown gibt (mehr als ein
    // nicht gefoldeter Spieler) – andernfalls bleibt der Text leer.
    QString newHandText;
    int nonFold = 0;
    if (activeList) {
        for (auto it = activeList->begin(); it != activeList->end(); ++it)
            if (!showdownFolded((*it)->getMyUniqueID(), (*it)->getMyAction() == PLAYER_ACTION_FOLD)) ++nonFold;
    }
    if (activeList && bero && nonFold > 1) {
        std::string name = CardsValue::determineHandName(bero->getHighestCardsValue(), activeList);
        newHandText = QString::fromStdString(name);
    }

    if (newHandText != m_winningHandText) {
        m_winningHandText = newHandText;
        emit winningHandTextChanged();
    }

    // ── Showdown-Spotlight: Karten abseits des Siegerblatts abblenden ──────────
    // Wie der Widgets-Client (gameTableImpl::postRiverRunAnimation3): für jeden
    // Hauptpot-Gewinner liefert getMyBestHandPosition die 5 Positionen seines
    // besten Blatts – 0/1 sind seine Hole-Cards, 2..6 die Board-Karten 0..4.
    // Alle übrigen Karten werden beim Showdown auf 25 % Deckkraft abgeblendet
    // (das eigentliche Faden macht QML, abhängig von ShowFadeOutCardsAnimation).
    // Eine Board-Karte bleibt hell, sobald sie zu IRGENDEINEM Siegerblatt zählt
    // (bei Split-Pots sauberer als das per-Gewinner-Faden des Widgets-Clients).
    QSet<int> newHoleFade0, newHoleFade1;
    QVariantList newBoardFade = {false, false, false, false, false};
    if (nonFold > 1) {
        bool boardUsed[5] = {false, false, false, false, false};
        bool anyWinner = false;
        for (auto it = activeList->begin(); it != activeList->end(); ++it) {
            if (!isMainPotWinner(*it))
                continue;
            anyWinner = true;
            int pos[5];
            (*it)->getMyBestHandPosition(pos);
            bool useHole0 = false, useHole1 = false;
            for (int j = 0; j < 5; ++j) {
                const int p = pos[j];
                if (p == 0)               useHole0 = true;
                else if (p == 1)          useHole1 = true;
                else if (p >= 2 && p <= 6) boardUsed[p - 2] = true;
            }
            const int id = (*it)->getMyID();
            if (!useHole0) newHoleFade0.insert(id);
            if (!useHole1) newHoleFade1.insert(id);
        }
        if (anyWinner) {
            for (int i = 0; i < 5; ++i)
                newBoardFade[i] = !boardUsed[i];
        }
    }
    if (newHoleFade0 != m_holeFade0 || newHoleFade1 != m_holeFade1) {
        m_holeFade0 = newHoleFade0;
        m_holeFade1 = newHoleFade1;
        refreshPlayerData(); // schiebt fade0/fade1 in die Seat-Daten
    }
    if (newBoardFade != m_boardCardFade) {
        m_boardCardFade = newBoardFade;
        emit boardCardFadeChanged();
    }

    // ── Showdown im Spielverlauf protokollieren (Logik 1:1 aus dem Widgets-Client) ──
    // Die Engine ruft im PokerTH-Client weder logFlipHoleCardsMsg noch
    // logPlayerWinsMsg von selbst auf – im Qt-Widgets-Client macht das die GUI
    // (gameTableImpl::postRiverRunAnimation2/3). Daher hier nachgebildet, sonst
    // fehlen aufgedeckte Karten und der Sieger im "Spielverlauf"-Overlay.
    if (!activeList) return;

    // 1) Aufgedeckte Hole-Cards der Spieler, die laut Engine zeigen müssen
    //    (wie showHoleCards → setMyCardsFlip(1,1) für die Post-River-Runde:
    //    "name shows [c0, c1] - \"Handname\"").
    for (auto it = activeList->begin(); it != activeList->end(); ++it) {
        const unsigned uid = (*it)->getMyUniqueID();
        if (showdownFolded(uid, (*it)->getMyAction() == PLAYER_ACTION_FOLD)
            || !showdownNeedsToShow(uid, (*it)->checkIfINeedToShowCards()))
            continue;
        int cards[2] = {-1, -1};
        (*it)->getMyCards(cards);
        if (cards[0] < 0 || cards[1] < 0)
            continue;
        QString line = QString::fromStdString((*it)->getMyName())
                     + " shows [" + logCard(cards[0]) + ", " + logCard(cards[1]) + "]";
        const int cardsValueInt = (*it)->getMyCardsValueInt();
        if (cardsValueInt != -1) {
            std::string handName = CardsValue::determineHandName(cardsValueInt, activeList);
            if (!handName.empty())
                line += " - \"" + QString::fromStdString(handName) + "\"";
        }
        appendGameLog(line);
    }

    // 2) Gewinner – Haupt-/Side-Pot via isMainPotWinner (highestWinnerCardsValue
    //    oben berechnet). Echte Gewinner stehen in der winners-Liste UND haben
    //    tatsächlich Geld gewonnen.
    for (auto it = activeList->begin(); it != activeList->end(); ++it) {
        const bool isWinner = std::find(winners.begin(), winners.end(), (*it)->getMyUniqueID()) != winners.end();
        const bool hasActuallyWon = isWinner && (*it)->getLastMoneyWon() > 0;
        if (showdownFolded((*it)->getMyUniqueID(), (*it)->getMyAction() == PLAYER_ACTION_FOLD) || !hasActuallyWon)
            continue;
        // Bei All-In mit mehreren Gewinnern: bestes Blatt = Hauptpot, Rest Side-Pot.
        const bool isMainPot = isMainPotWinner(*it);
        QString msg = QString::fromStdString((*it)->getMyName())
                    + " wins $" + QString::number((*it)->getLastMoneyWon());
        if (!isMainPot)
            msg += QStringLiteral(" (side pot)");
        appendGameLog(msg, isMainPot ? LogWinnerMain : LogWinnerSide);
    }

    // 3) Sit-Out für Spieler ohne Cash (wie gameTableImpl nach der Pot-Verteilung).
    for (auto it = activeList->begin(); it != activeList->end(); ++it) {
        if ((*it)->getMyCash() == 0)
            appendGameLog(QString::fromStdString((*it)->getMyName()) + " sits out", LogSitOut);
    }

    // 4) "Show"-Button: Mensch-Spieler (Sitz 0) kann seine Karten freiwillig zeigen,
    //    wenn er nicht gefoldet hat und nicht zeigen MUSS (Logik 1:1 aus dem
    //    Qt-Widgets-Client, gameTableImpl::postRiverRunAnimation2).
    // Zuschauer besitzen keinen Sitz und damit keine Karten zum Zeigen.
    bool newCanShow = false;
    auto seatsList = hand->getSeatsList();
    if (!m_spectating && seatsList && !seatsList->empty()) {
        auto humanPlayer = seatsList->front(); // seat 0
        const unsigned humanUid = humanPlayer->getMyUniqueID();
        if (humanPlayer->getMyActiveStatus()
            && !showdownFolded(humanUid, humanPlayer->getMyAction() == PLAYER_ACTION_FOLD)) {
            if (nonFold == 1) {
                // Gewonnen ohne Showdown – kann zeigen
                newCanShow = true;
            } else if (nonFold > 1
                       && !showdownNeedsToShow(humanUid, humanPlayer->checkIfINeedToShowCards())) {
                // Mehrere aktive Spieler, Mensch muss aber nicht zeigen – kann freiwillig zeigen
                newCanShow = true;
            }
        }
    }
    if (newCanShow != m_canShowCards) {
        m_canShowCards = newCanShow;
        emit canShowCardsChanged();
    }
}

void GameHandler::onFlipHolecardsAllIn()
{
    // Karten aller nicht-gefoldeten Spieler aufdecken (All-in-Runout).
    // Die Engine hat setMyCards() bereits für alle All-In-Spieler aufgerufen
    // (clientstate.cpp: AllInShowCardsMessage-Handler).
    qDebug() << "[ALLIN] onFlipHolecardsAllIn() blocked=" << localGameCallbacksBlocked()
             << "hasGame=" << (bool)m_game;
    if (localGameCallbacksBlocked()) return;
    if (!m_game) return;
    auto hand = m_game->getCurrentHand();
    if (!hand) return;

    // Wie im Qt-Widgets-Client: nur aufdecken, wenn >= 2 Spieler nicht gefoldet
    // haben (anderenfalls hat jemand schon gewonnen und es gibt nichts zu zeigen).
    auto active = hand->getActivePlayerList();
    if (!active) return;
    int nonFolded = 0;
    for (auto it = active->begin(); it != active->end(); ++it) {
        int c[2] = {-1, -1};
        (*it)->getMyCards(c);
        qDebug() << "[ALLIN]   active seatId=" << (*it)->getMyID()
                 << "action=" << (int)(*it)->getMyAction()
                 << "cards=" << c[0] << "/" << c[1];
        if ((*it)->getMyAction() != PLAYER_ACTION_FOLD) ++nonFolded;
    }
    qDebug() << "[ALLIN]   nonFolded=" << nonFolded;
    if (nonFolded < 2) {
        qDebug() << "[ALLIN]   GUARD: nonFolded<2 - aborting";
        return;
    }

    m_allInRevealed = true;
    qDebug() << "[ALLIN]   m_allInRevealed set to true, calling refreshPlayerData";
    refreshPlayerData();
}

void GameHandler::onPlayerShowCards(unsigned playerId)
{
    // Ein Spieler hat nach der Hand freiwillig seine Karten gezeigt. Die Engine
    // hat setMyCards()/setMyCardsValueInt() im AfterHandShowCardsMessage-Handler
    // (clientstate.cpp) bereits aktualisiert. Im Widgets-Client deckt
    // gameTableImpl::showHoleCards die Karten auf und protokolliert die Aktion via
    // setMyCardsFlip(1,1) → logFlipHoleCardsMsg. Beides fehlte im QML-Client, weil
    // SignalNetClientPostRiverShowCards dort ein No-op war.
    if (localGameCallbacksBlocked()) return;
    if (!m_game) return;
    auto hand = m_game->getCurrentHand();
    if (!hand) return;
    auto activeList = hand->getActivePlayerList();
    if (!activeList) return;

    for (auto it = activeList->begin(); it != activeList->end(); ++it) {
        if ((*it)->getMyUniqueID() != playerId)
            continue;
        // Gefoldete Spieler gehören nicht in den Showdown – auch dann nicht, wenn
        // eine AfterHandShowCardsMessage für sie eintrudelt. Sonst erschiene ihr
        // Blatt gewertet im Spielverlauf und könnte den echten Gewinner "schlagen".
        // (Das Engine-SQL-Log filtert an derselben Stelle: Log::logHoleCardsHandName.)
        if (showdownFolded(playerId, (*it)->getMyAction() == PLAYER_ACTION_FOLD))
            return;
        int cards[2] = {-1, -1};
        (*it)->getMyCards(cards);
        if (cards[0] < 0 || cards[1] < 0)
            return;
        // Karten aufgedeckt lassen (bis zur nächsten Hand).
        m_postRiverShownPlayers.insert(playerId);
        // Spielverlauf protokollieren: "name shows [c0, c1] - \"Handname\""
        // (wie logFlipHoleCardsMsg mit state=1 im Widgets-Client).
        QString line = QString::fromStdString((*it)->getMyName())
                     + " shows [" + logCard(cards[0]) + ", " + logCard(cards[1]) + "]";
        const int cardsValueInt = (*it)->getMyCardsValueInt();
        if (cardsValueInt != -1) {
            std::string handName = CardsValue::determineHandName(cardsValueInt, activeList);
            if (!handName.empty())
                line += " - \"" + QString::fromStdString(handName) + "\"";
        }
        appendGameLog(line);
        refreshPlayerData();
        return;
    }
}
