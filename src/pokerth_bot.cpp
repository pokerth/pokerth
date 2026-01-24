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

using namespace std;
using boost::asio::ip::tcp;
namespace po = boost::program_options;
namespace ssl = boost::asio::ssl;

#define NET_VERSION_MAJOR 5
#define NET_VERSION_MINOR 1
#define BUF_SIZE 4096

// Bot Session - Ein Bot-Client
class BotSession {
public:
    BotSession(boost::asio::io_context &io, ssl::context &sslCtx, 
               const string &name, const string &password)
        : socket_(io, sslCtx), name_(name), password_(password), 
          playerId_(0), gameId_(0), recBufPos_(0) {
        recBuf_.fill(0);
    }

    ssl::stream<tcp::socket>& socket() { return socket_; }
    const string& name() const { return name_; }
    uint32_t playerId() const { return playerId_; }
    void setPlayerId(uint32_t id) { playerId_ = id; }
    uint32_t gameId() const { return gameId_; }
    void setGameId(uint32_t id) { gameId_ = id; }

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
                
                if (ec) {
                    cerr << "[" << name_ << "] Read error: " << ec.message() << endl;
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
    boost::array<char, BUF_SIZE> recBuf_;
    size_t recBufPos_;
};

// Bot Controller - Verwaltet alle Bots
class BotController {
public:
    BotController(const string &server, const string &port, bool useTls)
        : io_(), sslCtx_(ssl::context::tlsv12_client), 
          server_(server), port_(port), useTls_(useTls) {
        
        if (useTls_) {
            sslCtx_.set_verify_mode(ssl::verify_none);
        }
    }

    // Erstelle und starte N Bots
    bool createBots(int numBots, int startId, const string &password) {
        cout << "Creating " << numBots << " bots..." << endl;

        for (int i = 0; i < numBots; i++) {
            string botName = "test" + to_string(startId + i);
            auto bot = make_shared<BotSession>(io_, sslCtx_, botName, password);
            
            if (!connectBot(bot)) {
                cerr << "Failed to connect bot: " << botName << endl;
                return false;
            }
            
            bots_.push_back(bot);
            cout << "[" << botName << "] Connected" << endl;
            
            // Lange Pause zwischen Bot-Logins (Server-Schonung - Server ist langsam!)
            if (i < numBots - 1) {
                this_thread::sleep_for(chrono::seconds(2));  // 2 Sekunden!
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
        gameInfo->set_delaybetweenhands(6);
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
        
        while (true) {
            for (auto &bot : bots_) {
                // Non-blocking check für verfügbare Daten
                boost::system::error_code ec;
                size_t available = bot->socket().lowest_layer().available(ec);
                
                if (!ec && available > 0) {
                    auto msg = bot->receiveMessage();
                    if (msg) {
                        handleMessage(bot, msg);
                    }
                }
            }
            
            this_thread::sleep_for(chrono::milliseconds(100));
        }
    }

private:
    bool connectBot(shared_ptr<BotSession> bot) {
        try {
            cout << "[" << bot->name() << "] Resolving..." << flush;
            // Resolve
            tcp::resolver resolver(io_);
            auto endpoints = resolver.resolve(server_, port_);
            
            cout << " Connecting..." << flush;
            // Connect TCP
            boost::asio::connect(bot->socket().lowest_layer(), endpoints);
            
            // TLS Handshake
            if (useTls_) {
                cout << " TLS handshake..." << flush;
                bot->socket().handshake(ssl::stream_base::client);
            }

            cout << " Waiting for announce..." << flush;
            // Empfange AnnounceMessage
            auto announce = bot->receiveMessage();
            if (!announce || announce->GetMsg()->messagetype() != PokerTHMessage::Type_AnnounceMessage) {
                cerr << "[" << bot->name() << "] No announce message" << endl;
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
                return false;
            }

            cout << " Waiting for init ack..." << flush;
            // Empfange InitAckMessage
            auto initAck = bot->receiveMessage();
            if (!initAck) {
                cerr << "[" << bot->name() << "] Connection lost waiting for init ack" << endl;
                return false;
            }
            
            if (initAck->GetMsg()->messagetype() != PokerTHMessage::Type_InitAckMessage) {
                cerr << "[" << bot->name() << "] Expected InitAck, got message type: " 
                     << initAck->GetMsg()->messagetype() << endl;
                return false;
            }

            bot->setPlayerId(initAck->GetMsg()->initackmessage().yourplayerid());
            cout << "[" << bot->name() << "] Logged in, Player ID: " << bot->playerId() << endl;

            return true;

        } catch (const exception &e) {
            cerr << "[" << bot->name() << "] Connect exception: " << e.what() << endl;
            return false;
        }
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
        
        if (msgType == PokerTHMessage::Type_PlayersTurnMessage) {
            // Ein Spieler ist am Zug - prüfen ob wir es sind
            auto playersTurn = msg->GetMsg()->playersturnmessage();
            
            if (playersTurn.playerid() == bot->playerId()) {
                // Wir sind am Zug - check wenn möglich, sonst fold
                boost::shared_ptr<NetPacket> action(new NetPacket);
                action->GetMsg()->set_messagetype(PokerTHMessage::Type_MyActionRequestMessage);
                MyActionRequestMessage *actionMsg = action->GetMsg()->mutable_myactionrequestmessage();
                actionMsg->set_gameid(bot->gameId());
                actionMsg->set_handnum(0);  // TODO: Track hand number
                actionMsg->set_gamestate(playersTurn.gamestate());
                actionMsg->set_myrelativebet(0);
                
                // Immer fold für einfache Tests
                actionMsg->set_myaction(netActionFold);
                cout << "[" << bot->name() << "] Fold" << endl;
                
                // Kleine Pause vor Aktion-Senden (Server-Schonung)
                this_thread::sleep_for(chrono::milliseconds(200));
                bot->sendMessage(action);
            }
            
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
            
        } else if (msgType == PokerTHMessage::Type_HandStartMessage) {
            cout << "[" << bot->name() << "] New hand started" << endl;
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
    try {
        po::options_description desc("PokerTH Bot Client - Automated test clients");
        desc.add_options()
            ("help,h", "Show help message")
            ("server,s", po::value<string>()->default_value("localhost"), "Server address")
            ("port,p", po::value<string>()->default_value("7234"), "Server port")
            ("bots,b", po::value<int>()->default_value(10), "Number of bots")
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
            cout << "\nExamples:" << endl;
            cout << "  Create game with 10 bots:  pokerth_bot -s pokerth.net -b 10 -c" << endl;
            cout << "  Join existing game:         pokerth_bot -s pokerth.net -b 9 -j 12345" << endl;
            return 0;
        }

        string server = vm["server"].as<string>();
        string port = vm["port"].as<string>();
        int numBots = vm["bots"].as<int>();
        int startId = vm["start-id"].as<int>();
        string password = vm["password"].as<string>();
        bool useTls = !vm.count("no-tls");

        cout << "PokerTH Bot Client" << endl;
        cout << "=================="<< endl;
        cout << "Server: " << server << ":" << port << endl;
        cout << "TLS: " << (useTls ? "enabled" : "disabled") << endl;
        cout << "Bots: " << numBots << " (test" << startId << " - test" << (startId + numBots - 1) << ")" << endl;
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

            // Nur 8 weitere Bots joinen (test2-test9), damit ein menschlicher Spieler als 10. joinen kann
            if (numBots > 1) {
                this_thread::sleep_for(chrono::seconds(3));
                int botsToJoin = min(numBots - 1, 8);  // Max 8 weitere Bots (test1 ist schon drin)
                cout << "Joining " << botsToJoin << " more bots to game " << gameId << "..." << endl;
                
                for (int i = 1; i <= botsToJoin; i++) {
                    if (!controller.joinBotToGame(i, gameId)) {
                        cerr << "Failed to join bot " << i << " to game" << endl;
                        return 1;
                    }
                    this_thread::sleep_for(chrono::seconds(2));  // 2 Sekunden zwischen Joins!
                }
                
                cout << "All bots joined. Waiting for 10th player (human) to start game..." << endl;
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
