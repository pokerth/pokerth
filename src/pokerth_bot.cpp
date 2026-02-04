/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2026 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/

// Headless Bot Client für PokerTH - Automatisierte Test-Clients
// Nutzt modernes Protocol Buffer Protokoll und TLS-basierte Plain-Auth

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/program_options.hpp>
#include <boost/array.hpp>
#include <third_party/protobuf/pokerth.pb.h>
#include <net/netpacket.h>

#include <iostream>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <future>
#include <sys/socket.h>
#include <sys/time.h>

using namespace std;
using boost::asio::ip::tcp;
namespace po = boost::program_options;
namespace ssl = boost::asio::ssl;

// Global flag für graceful shutdown
static atomic<bool> g_shutdownRequested(false);

#define NET_VERSION_MAJOR 5
#define NET_VERSION_MINOR 1
#define BUF_SIZE 4096

// Helper: Get current timestamp as string (HH:MM:SS.mmm)
static string getTimestamp() {
    auto now = chrono::system_clock::now();
    auto now_c = chrono::system_clock::to_time_t(now);
    auto ms = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    char buf[32];
    strftime(buf, sizeof(buf), "%H:%M:%S", localtime(&now_c));
    snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), ".%03d", (int)ms.count());
    return string(buf);
}

// Bot Session - Ein Bot-Client
class BotSession {
public:
    BotSession(boost::asio::io_context &io, ssl::context &sslCtx, 
               const string &name, const string &password)
        : socket_(io, sslCtx), name_(name), password_(password), 
          playerId_(0), gameId_(0), handNum_(0), mySet_(0), highestSet_(0), 
          myCash_(10000), lastGameState_(netStatePreflop), currentGameState_(netStatePreflop), isAllIn_(false), recBufPos_(0) {
        recBuf_.fill(0);
    }

    ssl::stream<tcp::socket>& socket() { return socket_; }
    const string& name() const { return name_; }
    const string& password() const { return password_; }
    uint32_t playerId() const { return playerId_; }
    void setPlayerId(uint32_t id) { playerId_ = id; }
    uint32_t gameId() const { return gameId_; }
    void setGameId(uint32_t id) { gameId_ = id; }
    uint32_t handNum() const { return handNum_; }
    void setHandNum(uint32_t num) { handNum_ = num; }
    uint32_t mySet() const { return mySet_; }
    void setMySet(uint32_t val) { mySet_ = val; }
    uint32_t highestSet() const { return highestSet_; }
    void setHighestSet(uint32_t val) { highestSet_ = val; }
    uint32_t myCash() const { return myCash_; }
    void setMyCash(uint32_t val) { myCash_ = val; }
    NetGameState lastGameState() const { return lastGameState_; }
    void setLastGameState(NetGameState state) { lastGameState_ = state; }
    NetGameState currentGameState() const { return currentGameState_; }
    void setCurrentGameState(NetGameState state) { currentGameState_ = state; }
    bool isAllIn() const { return isAllIn_; }
    void setIsAllIn(bool val) { isAllIn_ = val; }

    // Empfange Nachricht (blocking)
    boost::shared_ptr<NetPacket> receiveMessage() {
        boost::shared_ptr<NetPacket> tmpPacket;

        do {
            if (recBufPos_ >= NET_HEADER_SIZE) {
                uint32_t nativeVal;
                memcpy(&nativeVal, recBuf_.data(), sizeof(uint32_t));
                size_t packetSize = ntohl(nativeVal);
                
                if (packetSize > MAX_PACKET_SIZE) {
                    recBufPos_ = 0;
                    cerr << "[" << name_ << "] Packet too large: " << packetSize << endl;
                    return boost::shared_ptr<NetPacket>();
                }
                
                if (recBufPos_ >= packetSize + NET_HEADER_SIZE) {
                    try {
                        tmpPacket = NetPacket::Create(&recBuf_[NET_HEADER_SIZE], packetSize);
                        if (tmpPacket) {
                            recBufPos_ -= (packetSize + NET_HEADER_SIZE);
                            if (recBufPos_) {
                                memmove(recBuf_.data(), recBuf_.data() + packetSize + NET_HEADER_SIZE, recBufPos_);
                            }
                        }
                    } catch (const exception &e) {
                        recBufPos_ = 0;
                        cerr << "[" << name_ << "] Packet parse error: " << e.what() << endl;
                        return boost::shared_ptr<NetPacket>();
                    }
                }
            }

            if (!tmpPacket) {
                boost::system::error_code ec;
                size_t bytesRead = socket_.read_some(
                    boost::asio::buffer(recBuf_.data() + recBufPos_, BUF_SIZE - recBufPos_), ec);
                
                if (ec == boost::asio::error::eof) {
                    cerr << "[" << name_ << "] Connection closed by server" << endl;
                    return boost::shared_ptr<NetPacket>();
                } else if (ec == boost::asio::error::timed_out) {
                    cerr << "[" << name_ << "] Read timeout (server not responding)" << endl;
                    return boost::shared_ptr<NetPacket>();
                } else if (ec) {
                    cerr << "[" << name_ << "] Read error: " << ec.message() << " (code: " << ec.value() << ")" << endl;
                    return boost::shared_ptr<NetPacket>();
                }
                
                if (bytesRead == 0) {
                    cerr << "[" << name_ << "] No data read (connection closed?)" << endl;
                    return boost::shared_ptr<NetPacket>();
                }
                
                recBufPos_ += bytesRead;
            }
        } while (!tmpPacket);

        return tmpPacket;
    }

    // Sende Nachricht
    bool sendMessage(boost::shared_ptr<NetPacket> packet) {
        if (!packet) return false;

        uint32_t packetSize = packet->GetMsg()->ByteSizeLong();
        vector<google::protobuf::uint8> buf(packetSize + NET_HEADER_SIZE);
        
        *((uint32_t *)buf.data()) = htonl(packetSize);
        packet->GetMsg()->SerializeWithCachedSizesToArray(&buf[NET_HEADER_SIZE]);
        
        boost::system::error_code ec;
        boost::asio::write(socket_, boost::asio::buffer(buf), ec);
        
        if (ec) {
            cerr << "[" << name_ << "] Send error: " << ec.message() << endl;
            return false;
        }
        return true;
    }

private:
    ssl::stream<tcp::socket> socket_;
    string name_;
    string password_;
    uint32_t playerId_;
    uint32_t gameId_;
    uint32_t handNum_;
    uint32_t mySet_;        // Mein aktueller Einsatz in der Runde
    uint32_t highestSet_;   // Höchster Einsatz am Tisch
    uint32_t myCash_;       // Verfügbares Cash
    NetGameState lastGameState_; // Letzter GameState für Rejection-Fallback
    NetGameState currentGameState_; // Aktueller GameState (Preflop/Flop/Turn/River)
    bool isAllIn_;          // Merke ob Bot All-In ist in dieser Hand
    boost::array<char, BUF_SIZE> recBuf_;
    size_t recBufPos_;
};

// Bot Controller - Verwaltet alle Bots
class BotController {
public:
    BotController(const string &server, const string &port, bool useTls)
        : io_(), sslCtx_(ssl::context::sslv23_client),  // sslv23 = TLS 1.0-1.3 (wie GUI Client)
          server_(server), port_(port), useTls_(useTls) {
        
        if (useTls_) {
            sslCtx_.set_verify_mode(ssl::verify_none);
            // Disable alte/unsichere Protokolle
            sslCtx_.set_options(
                ssl::context::default_workarounds |
                ssl::context::no_sslv2 |
                ssl::context::no_sslv3 |
                ssl::context::no_tlsv1 |
                ssl::context::no_tlsv1_1);
        }
    }

    // Erstelle und starte N Bots
    bool createBots(int numBots, int startId, const string &password) {
        cout << "Creating " << numBots << " bots..." << endl;

        for (int i = 0; i < numBots; i++) {
            string botName = "test" + to_string(startId + i);
            auto bot = make_shared<BotSession>(io_, sslCtx_, botName, password);
            
            if (!connectBot(bot)) {
                cerr << "[" << getTimestamp() << "] Failed to connect bot: " << botName << endl;
                return false;
            }
            
            bots_.push_back(bot);
            cout << "[" << getTimestamp() << "] [" << botName << "] Connected" << endl;
            
            // Pause zwischen Bot-Logins
            if (i < numBots - 1) {
                this_thread::sleep_for(chrono::milliseconds(500));
            }
        }

        return true;
    }

    // Ein spezifischer Bot joint ein Game
    bool joinBotToGame(int botIndex, uint32_t gameId) {
        if (botIndex < 0 || botIndex >= (int)bots_.size()) {
            cerr << "Invalid bot index: " << botIndex << endl;
            return false;
        }
        
        auto &bot = bots_[botIndex];
        return sendJoinGame(bot, gameId);
    }

    // Alle Bots joinen ein Game
    bool joinGame(uint32_t gameId) {
        cout << "All bots joining game " << gameId << "..." << endl;

        for (auto &bot : bots_) {
            if (!sendJoinGame(bot, gameId)) {
                cerr << "[" << bot->name() << "] Failed to join game" << endl;
                return false;
            }
        }

        return true;
    }

    // Erster Bot erstellt ein Game
    uint32_t createGame(const string &gameName, const string &password) {
        if (bots_.empty()) {
            cerr << "No bots available" << endl;
            return 0;
        }

        auto creator = bots_[0];
        cout << "[" << creator->name() << "] Creating game: " << gameName << endl;

        boost::shared_ptr<NetPacket> packet(new NetPacket);
        packet->GetMsg()->set_messagetype(PokerTHMessage::Type_JoinNewGameMessage);
        
        JoinNewGameMessage *newGameMsg = packet->GetMsg()->mutable_joinnewgamemessage();
        if (!password.empty()) {
            newGameMsg->set_password(password);
        }
        newGameMsg->set_autoleave(false);
        
        NetGameInfo *gameInfo = newGameMsg->mutable_gameinfo();
        
        // Ranking Game mit FESTEN Server-Einstellungen (siehe game_defs.h):
        // RANKING_GAME_START_CASH = 10000
        // RANKING_GAME_NUMBER_OF_PLAYERS = 10
        // RANKING_GAME_START_SBLIND = 50
        // RANKING_GAME_RAISE_EVERY_HAND = 11
        // raiseMode MUSS doubleBlinds sein
        // Kein Passwort erlaubt, allowSpectators muss true sein
        
        gameInfo->set_gamename(gameName);
        gameInfo->set_netgametype(NetGameInfo::rankingGame);
        gameInfo->set_maxnumplayers(10);
        gameInfo->set_raiseintervalmode(NetGameInfo::raiseOnHandNum);
        gameInfo->set_raiseeveryhands(11);  // MUSS 11 sein für Ranking Games!
        gameInfo->set_endraisemode(NetGameInfo::doubleBlinds);  // MUSS doubleBlinds sein!
        gameInfo->set_proposedguispeed(5);
        gameInfo->set_delaybetweenhands(2);
        gameInfo->set_playeractiontimeout(10);
        gameInfo->set_endraisesmallblindvalue(0);
        gameInfo->set_firstsmallblind(50);  // MUSS 50 sein
        gameInfo->set_startmoney(10000);    // MUSS 10000 sein für Ranking Games!
        gameInfo->set_allowspectators(true); // MUSS true sein

        if (!creator->sendMessage(packet)) {
            return 0;
        }

        // Warte auf JoinGameAck (Server kann mehrere Messages senden)
        uint32_t createdGameId = 0;
        for (int attempts = 0; attempts < 20; attempts++) {  // Max 20 Messages
            auto reply = creator->receiveMessage();
            if (!reply) {
                cerr << "[" << creator->name() << "] Connection lost while waiting for JoinGameAck" << endl;
                return 0;
            }

            auto msgType = reply->GetMsg()->messagetype();
            
            if (msgType == PokerTHMessage::Type_JoinGameAckMessage) {
                createdGameId = reply->GetMsg()->joingameackmessage().gameid();
                creator->setGameId(createdGameId);
                cout << "[" << creator->name() << "] Created game ID: " << createdGameId << endl;
                
                // WICHTIG: Nach Game-Create pending Messages konsumieren!
                this_thread::sleep_for(chrono::milliseconds(100));
                flushPendingMessages(creator);
                
                return createdGameId;
            } else if (msgType == PokerTHMessage::Type_JoinGameFailedMessage) {
                auto reason = reply->GetMsg()->joingamefailedmessage().joingamefailurereason();
                cerr << "[" << creator->name() << "] JoinGameFailed, reason: " << reason << endl;
                return 0;
            }
            // Alle anderen Messages ignorieren (nicht mehr loggen)
        }

        cerr << "[" << creator->name() << "] Timeout waiting for JoinGameAck" << endl;
        return 0;
    }

    // Keep alive - empfange Messages
    void run() {
        cout << "Bots running... Press Ctrl+C to exit" << endl;
        
        while (!g_shutdownRequested) {
            for (auto &bot : bots_) {
                // ALLE verfügbaren Messages sofort verarbeiten
                boost::system::error_code ec;
                size_t available = bot->socket().lowest_layer().available(ec);
                
                while (!ec && available > 0) {
                    auto msg = bot->receiveMessage();
                    if (msg) {
                        handleMessage(bot, msg);
                    } else {
                        break;  // Keine Message mehr verfügbar oder Fehler
                    }
                    // Check wieder für weitere Messages
                    available = bot->socket().lowest_layer().available(ec);
                }
            }
            
            this_thread::sleep_for(chrono::milliseconds(10));  // Kürzerer Poll-Interval
        }
        
        // Graceful Shutdown: Alle Bots sauber disconnecten
        cout << "Shutting down gracefully..." << endl;
        for (auto &bot : bots_) {
            try {
                // LeaveGameRequestMessage senden falls in einem Spiel
                if (bot->gameId() != 0) {
                    cout << "[" << bot->name() << "] Leaving game " << bot->gameId() << "..." << endl;
                    boost::shared_ptr<NetPacket> leave(new NetPacket);
                    leave->GetMsg()->set_messagetype(PokerTHMessage::Type_LeaveGameRequestMessage);
                    LeaveGameRequestMessage *leaveMsg = leave->GetMsg()->mutable_leavegamerequestmessage();
                    leaveMsg->set_gameid(bot->gameId());
                    bot->sendMessage(leave);
                    this_thread::sleep_for(chrono::milliseconds(100));  // Warte auf Server-Verarbeitung
                }
                
                cout << "[" << bot->name() << "] Closing SSL connection..." << endl;
                
                // SSL Shutdown (bidirektional - wichtig!)
                boost::system::error_code ec;
                
                // Erst SSL shutdown versuchen (aber mit Timeout, falls Server nicht antwortet)
                if (useTls_) {
                    // Setze non-blocking für den Shutdown um nicht zu blocken
                    bot->socket().lowest_layer().non_blocking(true, ec);
                    
                    // SSL async_shutdown mit kurzem Timeout
                    auto shutdownTimer = std::make_shared<boost::asio::deadline_timer>(io_);
                    shutdownTimer->expires_from_now(boost::posix_time::seconds(2));
                    
                    std::atomic<bool> shutdownDone(false);
                    
                    shutdownTimer->async_wait([&shutdownDone, &bot](const boost::system::error_code&) {
                        if (!shutdownDone) {
                            // Timeout: Force close
                            boost::system::error_code ec2;
                            bot->socket().lowest_layer().close(ec2);
                            shutdownDone = true;
                        }
                    });
                    
                    bot->socket().async_shutdown([&shutdownDone, shutdownTimer](const boost::system::error_code&) {
                        shutdownTimer->cancel();
                        shutdownDone = true;
                    });
                    
                    // Poll bis shutdown fertig oder timeout
                    while (!shutdownDone) {
                        io_.poll_one();
                        this_thread::sleep_for(chrono::milliseconds(10));
                    }
                }
                
                // Socket schließen (falls noch offen)
                if (bot->socket().lowest_layer().is_open()) {
                    bot->socket().lowest_layer().shutdown(tcp::socket::shutdown_both, ec);
                    bot->socket().lowest_layer().close(ec);
                }
                
                cout << "[" << bot->name() << "] Disconnected." << endl;
            } catch (const std::exception& e) {
                cerr << "[" << bot->name() << "] Shutdown error: " << e.what() << endl;
            } catch (...) {
                // Ignoriere Fehler beim Shutdown
            }
        }
        cout << "All bots disconnected." << endl;
    }

private:
    bool connectBot(shared_ptr<BotSession>& bot) {
        // Retry-Logik: Bis zu 2 Versuche (initial + 1 retry)
        for (int attempt = 0; attempt < 2; attempt++) {
            if (attempt > 0) {
                cout << "\n[" << getTimestamp() << "] [" << bot->name() << "] Retry " << attempt << "/1..." << endl;
                this_thread::sleep_for(chrono::milliseconds(500)); // 0.5s delay vor retry
            }
            
            try {
                cout << "[" << getTimestamp() << "] [" << bot->name() << "] Resolving..." << flush;
                // Resolve
                tcp::resolver resolver(io_);
                boost::system::error_code resolveEc;
                auto endpoints = resolver.resolve(server_, port_, resolveEc);
                if (resolveEc) {
                    cerr << "\n[" << bot->name() << "] Resolve failed: " << resolveEc.message() << endl;
                    if (attempt < 1) continue; // Retry
                    return false;
                }
                
                cout << " [" << getTimestamp() << "] Connecting..." << flush;
                // Connect TCP with timeout (async)
                atomic<bool> connectComplete(false);
                boost::system::error_code connectEc;
                
                auto connectTimer = make_shared<boost::asio::deadline_timer>(io_);
                connectTimer->expires_from_now(boost::posix_time::seconds(10));
                
                connectTimer->async_wait([&connectComplete, &connectEc, &bot, connectTimer](const boost::system::error_code& ec) {
                    if (!ec && !connectComplete) {
                        boost::system::error_code closeEc;
                        bot->socket().lowest_layer().close(closeEc);
                        connectEc = boost::asio::error::timed_out;
                        connectComplete = true;
                    }
                });
                
                boost::asio::async_connect(
                    bot->socket().lowest_layer(), 
                    endpoints,
                    [&connectComplete, &connectEc, connectTimer](const boost::system::error_code& ec, const tcp::endpoint&) {
                        if (!connectComplete) {
                            connectTimer->cancel();
                            connectEc = ec;
                            connectComplete = true;
                        }
                    });
                
                while (!connectComplete) {
                    io_.run_one();
                }
                
                connectTimer->cancel();
                io_.poll();
                io_.restart();
                
                if (connectEc) {
                    cerr << "\n[" << getTimestamp() << "] [" << bot->name() << "] TCP connect failed: " << connectEc.message() << endl;
                    if (attempt < 1) continue; // Retry
                    return false;
                }
                
                // Deaktiviere Nagle's Algorithm für sofortiges Senden
                boost::system::error_code ec;
                tcp::no_delay no_delay_option(true);
                bot->socket().lowest_layer().set_option(no_delay_option, ec);
                if (ec) {
                    cerr << "\n[" << bot->name() << "] Warning: Could not set TCP_NODELAY: " << ec.message() << endl;
                }
                
                // TLS Handshake mit async + deadline_timer (exakt wie GUI Client)
                if (useTls_) {
                    cout << " [" << getTimestamp() << "] TLS handshake..." << flush;
                    
                    atomic<bool> handshakeComplete(false);
                    boost::system::error_code handshakeEc;
                    
                    // Erstelle Timeout-Timer (als shared_ptr um Lebensdauer zu kontrollieren)
                    auto handshakeTimer = make_shared<boost::asio::deadline_timer>(io_);
                    handshakeTimer->expires_from_now(boost::posix_time::seconds(12));
                    
                    // Timer-Callback für Timeout
                    handshakeTimer->async_wait([&handshakeComplete, &handshakeEc, &bot, handshakeTimer](const boost::system::error_code& ec) {
                        if (!ec && !handshakeComplete) {
                            // Timeout! Socket schließen um Handshake abzubrechen
                            boost::system::error_code closeEc;
                            bot->socket().lowest_layer().close(closeEc);
                            handshakeEc = boost::asio::error::timed_out;
                            handshakeComplete = true;
                        }
                    });
                    
                    // Async Handshake starten
                    bot->socket().async_handshake(
                        ssl::stream_base::client,
                        [&handshakeComplete, &handshakeEc, handshakeTimer](const boost::system::error_code& ec) {
                            if (!handshakeComplete) {
                                handshakeTimer->cancel();  // WICHTIG: Timer canceln bei Erfolg
                                handshakeEc = ec;
                                handshakeComplete = true;
                            }
                        });
                    
                    // Warte bis Handshake fertig (blocking mit run_one)
                    while (!handshakeComplete) {
                        io_.run_one();
                    }
                    
                    // CRITICAL: Timer explizit canceln um sicherzustellen dass keine callbacks mehr kommen
                    handshakeTimer->cancel();
                    // Alle pending handler verarbeiten (damit cancel-callbacks durchlaufen)
                    io_.poll();
                    io_.restart();  // Restart io_context für nächsten Bot/Retry
                    
                    if (handshakeEc) {
                        cerr << "\n[" << getTimestamp() << "] [" << bot->name() << "] TLS handshake failed: " << handshakeEc.message() 
                             << " (code: " << handshakeEc.value() << ")" << endl;
                        
                        // WICHTIG: Speichere Name/Passwort VOR reset()
                        string botName = bot->name();
                        string botPassword = bot->password();
                        
                        // Sauber schließen - bei Timeout KEIN shutdown() (verhindert 'stream truncated')
                        try {
                            boost::system::error_code closeEc;
                            if (handshakeEc != boost::asio::error::timed_out) {
                                // Nur bei echten Fehlern graceful shutdown versuchen
                                bot->socket().lowest_layer().shutdown(tcp::socket::shutdown_both, closeEc);
                            }
                            // Socket immer schließen
                            bot->socket().lowest_layer().close(closeEc);
                        } catch (...) {
                            // Ignoriere Fehler beim Cleanup
                        }
                        
                        if (attempt < 1) {
                            cerr << "[" << getTimestamp() << "] [" << botName << "] Will retry with new connection..." << endl;
                            // WICHTIG: Altes Socket-Objekt vollständig verwerfen
                            bot.reset();
                            // Neues Socket erstellen für retry mit gespeicherten Werten
                            bot = make_shared<BotSession>(io_, sslCtx_, botName, botPassword);
                            continue; // Retry
                        }
                        return false;
                    }
                }
                
                // TLS Handshake erfolgreich - weiter mit Announce/Init
                cout << " Waiting for announce..." << flush;
                // Empfange AnnounceMessage
                auto announce = bot->receiveMessage();
                if (!announce || announce->GetMsg()->messagetype() != PokerTHMessage::Type_AnnounceMessage) {
                    cerr << "\n[" << bot->name() << "] No announce message" << endl;
                    if (attempt < 1) {
                        string botName = bot->name();
                        string botPassword = bot->password();
                        // Socket sauber schließen vor Retry
                        try {
                            boost::system::error_code closeEc;
                            bot->socket().lowest_layer().close(closeEc);
                        } catch (...) {}
                        bot.reset();
                        bot = make_shared<BotSession>(io_, sslCtx_, botName, botPassword);
                        continue; // Retry
                    }
                    return false;
                }

                cout << " Sending init..." << flush;
                boost::shared_ptr<NetPacket> init(new NetPacket);
                init->GetMsg()->set_messagetype(PokerTHMessage::Type_InitMessage);
                
                InitMessage *initMsg = init->GetMsg()->mutable_initmessage();
                initMsg->mutable_requestedversion()->set_majorversion(NET_VERSION_MAJOR);
                initMsg->mutable_requestedversion()->set_minorversion(NET_VERSION_MINOR);
                initMsg->set_buildid(0);
                initMsg->set_login(InitMessage::authenticatedLogin);
                initMsg->set_nickname(bot->name());
                initMsg->set_clientuserdata(bot->name());  // Password = username (für test* accounts)

                if (!bot->sendMessage(init)) {
                    cerr << "\n[" << bot->name() << "] Failed to send init" << endl;
                    if (attempt < 1) {
                        string botName = bot->name();
                        string botPassword = bot->password();
                        try {
                            boost::system::error_code closeEc;
                            bot->socket().lowest_layer().close(closeEc);
                        } catch (...) {}
                        bot.reset();
                        bot = make_shared<BotSession>(io_, sslCtx_, botName, botPassword);
                        continue; // Retry
                    }
                    return false;
                }

                cout << " Waiting for init ack..." << flush;
                // Empfange InitAckMessage
                auto initAck = bot->receiveMessage();
                if (!initAck) {
                    cerr << "\n[" << bot->name() << "] Connection lost waiting for init ack" << endl;
                    if (attempt < 1) {
                        string botName = bot->name();
                        string botPassword = bot->password();
                        try {
                            boost::system::error_code closeEc;
                            bot->socket().lowest_layer().close(closeEc);
                        } catch (...) {}
                        bot.reset();
                        bot = make_shared<BotSession>(io_, sslCtx_, botName, botPassword);
                        continue; // Retry
                    }
                    return false;
                }
                
                if (initAck->GetMsg()->messagetype() != PokerTHMessage::Type_InitAckMessage) {
                    cerr << "\n[" << bot->name() << "] Expected InitAck, got message type: " 
                         << initAck->GetMsg()->messagetype() << endl;
                    if (attempt < 1) {
                        string botName = bot->name();
                        string botPassword = bot->password();
                        try {
                            boost::system::error_code closeEc;
                            bot->socket().lowest_layer().close(closeEc);
                        } catch (...) {}
                        bot.reset();
                        bot = make_shared<BotSession>(io_, sslCtx_, botName, botPassword);
                        continue; // Retry
                    }
                    return false;
                }

                bot->setPlayerId(initAck->GetMsg()->initackmessage().yourplayerid());
                cout << "[" << getTimestamp() << "] [" << bot->name() << "] Logged in, Player ID: " << bot->playerId() << endl;

                return true; // Erfolg!

            } catch (const exception &e) {
                cerr << "\n[" << bot->name() << "] Connect exception: " << e.what() << endl;
                if (attempt < 1) {
                    string botName = bot->name();
                    string botPassword = bot->password();
                    // Socket sauber schließen
                    try {
                        boost::system::error_code closeEc;
                        bot->socket().lowest_layer().close(closeEc);
                    } catch (...) {}
                    // Altes Objekt vollständig verwerfen
                    bot.reset();
                    // Neues Socket erstellen für retry
                    bot = make_shared<BotSession>(io_, sslCtx_, botName, botPassword);
                    continue; // Retry
                }
                return false;
            }
        } // Ende retry-loop
        
        return false; // Alle Versuche fehlgeschlagen
    }

    bool sendJoinGame(shared_ptr<BotSession> bot, uint32_t gameId) {
        boost::shared_ptr<NetPacket> packet(new NetPacket);
        packet->GetMsg()->set_messagetype(PokerTHMessage::Type_JoinExistingGameMessage);
        
        JoinExistingGameMessage *joinMsg = packet->GetMsg()->mutable_joinexistinggamemessage();
        joinMsg->set_gameid(gameId);
        joinMsg->set_autoleave(false);

        if (!bot->sendMessage(packet)) {
            return false;
        }

        // Warte auf JoinGameAck - ALLE Messages konsumieren!
        for (int attempts = 0; attempts < 50; attempts++) {
            auto reply = bot->receiveMessage();
            if (!reply) {
                cerr << "[" << bot->name() << "] Connection lost while waiting for JoinGameAck" << endl;
                return false;
            }

            auto msgType = reply->GetMsg()->messagetype();
            
            if (msgType == PokerTHMessage::Type_JoinGameAckMessage) {
                bot->setGameId(gameId);
                cout << "[" << bot->name() << "] Joined game " << gameId << endl;
                
                // WICHTIG: Nach dem Join alle pending Messages konsumieren!
                // Der Server sendet GamePlayerJoinedMessage, GameListPlayerJoinedMessage, etc.
                this_thread::sleep_for(chrono::milliseconds(100));
                flushPendingMessages(bot);
                
                return true;
            } else if (msgType == PokerTHMessage::Type_JoinGameFailedMessage) {
                auto reason = reply->GetMsg()->joingamefailedmessage().joingamefailurereason();
                cerr << "[" << bot->name() << "] JoinGameFailed, reason: " << reason << endl;
                return false;
            }
            // ALLE anderen Messages einfach ignorieren (nicht loggen - zu viel Output)
        }

        cerr << "[" << bot->name() << "] Timeout waiting for JoinGameAck" << endl;
        return false;
    }
    
    // Konsumiere alle verfügbaren Messages ohne zu warten
    void flushPendingMessages(shared_ptr<BotSession> bot) {
        for (int i = 0; i < 10; i++) {  // Max 10 pending messages
            boost::system::error_code ec;
            size_t available = bot->socket().lowest_layer().available(ec);
            if (ec || available == 0) break;
            
            auto msg = bot->receiveMessage();
            if (!msg) break;
            // Message ignorieren (PlayerJoinedMessage, GameListUpdate, etc.)
        }
    }

    void handleMessage(shared_ptr<BotSession> bot, boost::shared_ptr<NetPacket> msg) {
        auto msgType = msg->GetMsg()->messagetype();
        
        // DEBUG: Nur unbekannte Message-Types loggen
        if (msgType != PokerTHMessage::Type_PlayersTurnMessage && 
            msgType != PokerTHMessage::Type_HandStartMessage &&
            msgType != PokerTHMessage::Type_PlayersActionDoneMessage &&
            msgType != PokerTHMessage::Type_DealFlopCardsMessage &&
            msgType != PokerTHMessage::Type_DealTurnCardMessage &&
            msgType != PokerTHMessage::Type_DealRiverCardMessage &&
            msgType != PokerTHMessage::Type_EndOfHandShowCardsMessage &&
            msgType != PokerTHMessage::Type_EndOfHandHideCardsMessage &&
            msgType != PokerTHMessage::Type_AllInShowCardsMessage) {
            cout << "[" << bot->name() << "] Received msgType: " << msgType << endl;
        }
        
        if (msgType == PokerTHMessage::Type_HandStartMessage) {
            // Hand-Nummer inkrementieren, Sets zurücksetzen
            bot->setHandNum(bot->handNum() + 1);
            bot->setMySet(0);
            bot->setHighestSet(0);
            bot->setIsAllIn(false); // Reset All-In Status für neue Hand
            bot->setCurrentGameState(netStatePreflop); // Start in Preflop
            // myCash bleibt unverändert - wird von EndOfHandShowCardsMessage der vorherigen Hand gesetzt
            cout << "[" << bot->name() << "] Hand #" << bot->handNum() << " started (myCash=" << bot->myCash() << ")" << endl;
                    } else if (msgType == PokerTHMessage::Type_EndOfHandShowCardsMessage) {
            // End of hand: Update cash from playerResults
            auto endOfHand = msg->GetMsg()->endofhandshowcardsmessage();
            for (int i = 0; i < endOfHand.playerresults_size(); i++) {
                auto result = endOfHand.playerresults(i);
                if (result.playerid() == bot->playerId()) {
                    bot->setMyCash(result.playermoney());
                    cout << "[" << bot->name() << "] Hand ended - myCash=" << bot->myCash() << endl;
                    break;
                }
            }
            
        } else if (msgType == PokerTHMessage::Type_EndOfHandHideCardsMessage) {
            // End of hand (player folded or didn't show): Update cash
            auto endOfHand = msg->GetMsg()->endofhandhidecardsmessage();
            if (endOfHand.playerid() == bot->playerId()) {
                bot->setMyCash(endOfHand.playermoney());
                cout << "[" << bot->name() << "] Hand ended (hidden) - myCash=" << bot->myCash() << endl;
            }
            
        } else if (msgType == PokerTHMessage::Type_PlayersActionDoneMessage) {
            // Tracke highestSet und mySet
            auto actionDone = msg->GetMsg()->playersactiondonemessage();
            
            // CRITICAL: Nur Messages aus der aktuellen Runde verarbeiten
            // Nach Flop/Turn/River kommen noch verspätete Messages aus Preflop/Flop/Turn
            // die highestSet auf alte Werte setzen würden
            bool isCurrentRound = false;
            switch (actionDone.gamestate()) {
                case netStatePreflopSmallBlind:
                case netStatePreflopBigBlind:
                case netStatePreflop:
                    isCurrentRound = (bot->currentGameState() == netStatePreflop);
                    break;
                case netStateFlop:
                    isCurrentRound = (bot->currentGameState() == netStateFlop);
                    break;
                case netStateTurn:
                    isCurrentRound = (bot->currentGameState() == netStateTurn);
                    break;
                case netStateRiver:
                    isCurrentRound = (bot->currentGameState() == netStateRiver);
                    break;
                default:
                    isCurrentRound = true;
                    break;
            }
            
            if (!isCurrentRound) {
                // Ignoriere Messages aus alten Runden
                return;
            }
            
            bot->setHighestSet(actionDone.highestset());
            
            // Wenn es meine Action war, update mySet UND Cash
            if (actionDone.playerid() == bot->playerId()) {
                bot->setMySet(actionDone.totalplayerbet());
                bot->setMyCash(actionDone.playermoney());
                
                // WICHTIG: Wenn Cash=0, dann sind wir All-In!
                if (bot->myCash() == 0) {
                    bot->setIsAllIn(true);
                    cout << "[" << bot->name() << "] Now All-In! (myCash=0)" << endl;
                }
                
                cout << "[" << bot->name() << "] My action done: mySet=" << bot->mySet() 
                     << ", highestSet=" << bot->highestSet() 
                     << ", myCash=" << bot->myCash() << endl;
            }
            // WICHTIG: Update myCash auch für ANDERE Spieler wenn sie All-In gehen
            // Das hilft, wenn der Bot selbst mal gewinnt aber keine Action macht
            // (dann kriegen wir den neuen Cash-Stand über die Actions anderer Spieler mit)
            
        } else if (msgType == PokerTHMessage::Type_DealFlopCardsMessage ||
                   msgType == PokerTHMessage::Type_DealTurnCardMessage ||
                   msgType == PokerTHMessage::Type_DealRiverCardMessage) {
            // Neue Betting-Runde: Sets zurücksetzen (aber nicht die Hand!)
            // Debug-Ausgabe nur bei DealFlopCardsMessage (erste neue Runde)
            if (msgType == PokerTHMessage::Type_DealFlopCardsMessage) {
                bot->setCurrentGameState(netStateFlop);
                cout << "[" << bot->name() << "] Flop dealt - resetting sets" << endl;
            } else if (msgType == PokerTHMessage::Type_DealTurnCardMessage) {
                bot->setCurrentGameState(netStateTurn);
            } else if (msgType == PokerTHMessage::Type_DealRiverCardMessage) {
                bot->setCurrentGameState(netStateRiver);
            }
            bot->setMySet(0);
            bot->setHighestSet(0);
            
        } else if (msgType == PokerTHMessage::Type_PlayersTurnMessage) {
            // Ein Spieler ist am Zug - prüfen ob wir es sind
            auto playersTurn = msg->GetMsg()->playersturnmessage();
            
            // Debug nur wenn ich am Zug bin
            if (playersTurn.playerid() == bot->playerId()) {
                cout << "[" << bot->name() << "] MY TURN (hand=" << bot->handNum() 
                     << ") - mySet=" << bot->mySet() << ", highestSet=" << bot->highestSet() << ", myCash=" << bot->myCash() << endl;
                
                // WICHTIG: Erst ALLE ausstehenden Messages verarbeiten (z.B. PlayersActionDoneMessage)
                // um sicherzustellen, dass mySet/highestSet aktuell sind!
                boost::system::error_code ec;
                size_t available = bot->socket().lowest_layer().available(ec);
                while (!ec && available > 0) {
                    auto pendingMsg = bot->receiveMessage();
                    if (pendingMsg) {
                        handleMessage(bot, pendingMsg);
                    } else {
                        break;
                    }
                    available = bot->socket().lowest_layer().available(ec);
                }
                
                // Wenn Bot bereits All-In ist (kein Cash mehr), sende CALL 0
                // Der Server erkennt das und handhabt es korrekt (skip oder auto-check)
                if (bot->myCash() == 0 || bot->isAllIn()) {
                    cout << "[" << bot->name() << "] Already All-In, sending CALL 0" << endl;
                    bot->setIsAllIn(true); // Merke All-In Status
                    boost::shared_ptr<NetPacket> action(new NetPacket);
                    action->GetMsg()->set_messagetype(PokerTHMessage::Type_MyActionRequestMessage);
                    MyActionRequestMessage *actionMsg = action->GetMsg()->mutable_myactionrequestmessage();
                    actionMsg->set_gameid(bot->gameId());
                    actionMsg->set_handnum(bot->handNum());
                    actionMsg->set_gamestate(playersTurn.gamestate());
                    actionMsg->set_myaction(netActionCall);
                    actionMsg->set_myrelativebet(0);
                    bot->sendMessage(action);
                    
                    // CRITICAL: Wait for PlayersActionDoneMessage to confirm myCash stays at 0
                    // This prevents the bot from thinking it has cash in the next round
                    auto confirmMsg = bot->receiveMessage();
                    if (confirmMsg && confirmMsg->GetMsg()->messagetype() == PokerTHMessage::Type_PlayersActionDoneMessage) {
                        auto actionDone = confirmMsg->GetMsg()->playersactiondonemessage();
                        if (actionDone.playerid() == bot->playerId()) {
                            bot->setMyCash(actionDone.playermoney());
                            cout << "[" << bot->name() << "] All-In confirmed: myCash=" << bot->myCash() << endl;
                        }
                    }
                    return;
                }
                
                // Jetzt mit aktuellen Werten reagieren - Auto-check/auto-call Logik
                boost::shared_ptr<NetPacket> action(new NetPacket);
                action->GetMsg()->set_messagetype(PokerTHMessage::Type_MyActionRequestMessage);
                MyActionRequestMessage *actionMsg = action->GetMsg()->mutable_myactionrequestmessage();
                actionMsg->set_gameid(bot->gameId());
                actionMsg->set_handnum(bot->handNum());
                actionMsg->set_gamestate(playersTurn.gamestate());
                
                // Speichere gamestate für eventuelle Rejection-Fallback
                bot->setLastGameState(playersTurn.gamestate());
                
                // Prüfe ob check möglich ist (kein zusätzliches Geld nötig)
                if (bot->highestSet() == bot->mySet()) {
                    // CHECK: Kein Bet liegt oder bereits gematched
                    actionMsg->set_myaction(netActionCheck);
                    actionMsg->set_myrelativebet(0);
                    cout << "[" << bot->name() << "] CHECK (hand=" << bot->handNum() 
                         << ", final mySet=" << bot->mySet() << ", highestSet=" << bot->highestSet() << ")" << endl;
                } else {
                    // CALL: Gehe mit bis zum höchsten Bet (aber max. verfügbares Cash)
                    uint32_t callAmount = bot->highestSet() - bot->mySet();
                    if (callAmount > bot->myCash()) {
                        callAmount = bot->myCash(); // All-In mit verfügbarem Cash
                        bot->setIsAllIn(true); // Merke All-In Status
                    }
                    actionMsg->set_myaction(netActionCall);
                    actionMsg->set_myrelativebet(callAmount);
                    cout << "[" << bot->name() << "] CALL " << callAmount << " (hand=" << bot->handNum() 
                         << ", final mySet=" << bot->mySet() << ", highestSet=" << bot->highestSet() << ")" << endl;
                }
                
                bot->sendMessage(action);
            }
            
        } else if (msgType == PokerTHMessage::Type_YourActionRejectedMessage) {
            auto rejected = msg->GetMsg()->youractionrejectedmessage();
            cerr << "[" << bot->name() << "] ACTION REJECTED! Reason: " << rejected.rejectionreason() 
                 << " Action: " << rejected.youraction() << " Bet: " << rejected.yourrelativebet() << endl;
            
            // Fallback: Sende neue Action basierend auf verfügbarem Cash
            boost::shared_ptr<NetPacket> fallbackAction(new NetPacket);
            fallbackAction->GetMsg()->set_messagetype(PokerTHMessage::Type_MyActionRequestMessage);
            MyActionRequestMessage *fallbackMsg = fallbackAction->GetMsg()->mutable_myactionrequestmessage();
            fallbackMsg->set_gameid(bot->gameId());
            fallbackMsg->set_handnum(bot->handNum());
            fallbackMsg->set_gamestate(bot->lastGameState());
            
            // Auto-Check/Auto-Call Verhalten: NIEMALS FOLD!
            if (bot->myCash() == 0) {
                // Kein Cash mehr: CALL 0 (Server handled als Skip oder Auto-Check)
                fallbackMsg->set_myaction(netActionCall);
                fallbackMsg->set_myrelativebet(0);
                cerr << "[" << bot->name() << "] Fallback: CALL 0 (All-In)" << endl;
            } else {
                // Noch Cash vorhanden: Calle mit allem was ich habe (All-In)
                fallbackMsg->set_myaction(netActionCall);
                fallbackMsg->set_myrelativebet(bot->myCash());
                cerr << "[" << bot->name() << "] Fallback: CALL " << bot->myCash() << " (All-In)" << endl;
                bot->setIsAllIn(true);
            }
            
            bot->sendMessage(fallbackAction);
            
        } else if (msgType == PokerTHMessage::Type_EndOfGameMessage) {
            cout << "[" << bot->name() << "] Game ended" << endl;
            
        } else if (msgType == PokerTHMessage::Type_StartEventMessage) {
            cout << "[" << bot->name() << "] Game starting! Sending Ack..." << endl;
            
            // WICHTIG: StartEventAckMessage senden, sonst hängt der Server!
            boost::shared_ptr<NetPacket> ack(new NetPacket);
            ack->GetMsg()->set_messagetype(PokerTHMessage::Type_StartEventAckMessage);
            StartEventAckMessage *ackMsg = ack->GetMsg()->mutable_starteventackmessage();
            ackMsg->set_gameid(bot->gameId());
            
            this_thread::sleep_for(chrono::milliseconds(100));
            bot->sendMessage(ack);
        }
        // Weitere Messages stillschweigend ignorieren
    }

    boost::asio::io_context io_;
    ssl::context sslCtx_;
    string server_;
    string port_;
    bool useTls_;
    vector<shared_ptr<BotSession>> bots_;
};

int main(int argc, char *argv[]) {
    // SIGINT Handler für graceful shutdown
    signal(SIGINT, [](int) {
        cout << "\nReceived interrupt signal, shutting down..." << endl;
        g_shutdownRequested = true;
    });
    
    try {
        po::options_description desc("PokerTH Bot Client - Automated test clients");
        desc.add_options()
            ("help,h", "Show help message")
            ("server,s", po::value<string>()->default_value("localhost"), "Server address")
            ("port,p", po::value<string>()->default_value("7234"), "Server port")
            ("bots,b", po::value<int>(), "Number of bots (default: 10 - humans)")
            ("humans,H", po::value<int>()->default_value(1), "Number of human player slots (1-10)")
            ("start-id,i", po::value<int>()->default_value(1), "First bot ID (test<id>)")
            ("password,P", po::value<string>()->default_value(""), "Password (default: same as username)")
            ("create-game,c", "Create a game with first bot")
            ("game-name,g", po::value<string>()->default_value("Bot Test Game"), "Game name")
            ("game-password,G", po::value<string>()->default_value(""), "Game password")
            ("join-game,j", po::value<uint32_t>(), "Join existing game ID")
            ("no-tls", "Disable TLS (use plain TCP)")
        ;

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            cout << desc << endl;
            cout << "\nDescription:" << endl;
            cout << "  Creates automated bot players for PokerTH testing. Bots use auto-check/auto-fold" << endl;
            cout << "  strategy and react instantly to their turns. Perfect for load testing and" << endl;
            cout << "  ranking game database validation." << endl;
            cout << "\nExamples:" << endl;
            cout << "  Create game with 9 bots + 1 human slot:" << endl;
            cout << "    pokerth_bot -s pokerth.net -p 7236 -c --humans 1" << endl;
            cout << "\n  Create game with 7 bots + 3 human slots:" << endl;
            cout << "    pokerth_bot -s pokerth.net -p 7236 -c --humans 3" << endl;
            cout << "\n  Create game with all 10 bots (no humans):" << endl;
            cout << "    pokerth_bot -s pokerth.net -p 7236 -c --humans 0" << endl;
            cout << "\n  Join existing game with 5 bots:" << endl;
            cout << "    pokerth_bot -s pokerth.net -p 7236 -j 12345 -b 5" << endl;
            cout << "\nNotes:" << endl;
            cout << "  - Ranking games require exactly 10 players" << endl;
            cout << "  - Bots are named test1, test2, ..., test<N>" << endl;
            cout << "  - If both --bots and --humans are given, --humans takes priority" << endl;
            cout << "  - Use Ctrl+C for graceful shutdown" << endl;
            return 0;
        }

        string server = vm["server"].as<string>();
        string port = vm["port"].as<string>();
        int numHumans = vm["humans"].as<int>();
        int numBots;
        
        // Validiere humans parameter
        if (numHumans < 0 || numHumans > 10) {
            cerr << "Error: --humans must be between 0 and 10" << endl;
            return 1;
        }
        
        // -H hat Priorität: Immer 10 - humans berechnen
        // -b wird nur verwendet wenn -H nicht angegeben wurde (für join-game Szenarien)
        if (vm.count("bots") && numHumans != 1) {  // numHumans != 1 bedeutet -H wurde explizit gesetzt
            cout << "Warning: Both --bots and --humans specified. Using --humans, ignoring --bots." << endl;
        }
        
        numBots = 10 - numHumans;
        
        if (numBots < 1) {
            cerr << "Error: Need at least 1 bot (humans must be < 10)" << endl;
            return 1;
        }
        
        int startId = vm["start-id"].as<int>();
        string password = vm["password"].as<string>();
        bool useTls = !vm.count("no-tls");

        cout << "PokerTH Bot Client" << endl;
        cout << "=================="<< endl;
        cout << "Server: " << server << ":" << port << endl;
        cout << "TLS: " << (useTls ? "enabled" : "disabled") << endl;
        cout << "Bots: " << numBots << " (test" << startId << " - test" << (startId + numBots - 1) << ")" << endl;
        cout << "Human slots: " << numHumans << endl;
        cout << "Total players: " << (numBots + numHumans) << "/10" << endl;
        cout << endl;

        BotController controller(server, port, useTls);

        if (!controller.createBots(numBots, startId, password)) {
            cerr << "Failed to create bots" << endl;
            return 1;
        }

        if (vm.count("create-game")) {
            string gameName = vm["game-name"].as<string>();
            string gamePassword = vm["game-password"].as<string>();
            
            uint32_t gameId = controller.createGame(gameName, gamePassword);
            if (gameId == 0) {
                cerr << "Failed to create game" << endl;
                return 1;
            }

            // Weitere Bots joinen lassen (test1 hat schon das Game erstellt)
            if (numBots > 1) {
                this_thread::sleep_for(chrono::milliseconds(500));
                int botsToJoin = numBots - 1;  // -1 weil test1 bereits im Game ist
                cout << "Joining " << botsToJoin << " more bots to game " << gameId << "..." << endl;
                
                for (int i = 1; i <= botsToJoin; i++) {
                    if (!controller.joinBotToGame(i, gameId)) {
                        cerr << "Failed to join bot " << i << " to game" << endl;
                        return 1;
                    }
                    this_thread::sleep_for(chrono::milliseconds(200));
                }
                
                if (numHumans > 0) {
                    cout << "All bots joined. Waiting for " << numHumans << " human player(s) to start game..." << endl;
                } else {
                    cout << "All 10 bots joined. Game will start automatically." << endl;
                }
            }
        } else if (vm.count("join-game")) {
            uint32_t gameId = vm["join-game"].as<uint32_t>();
            if (!controller.joinGame(gameId)) {
                cerr << "Failed to join game" << endl;
                return 1;
            }
        }

        controller.run();

    } catch (const exception &e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
