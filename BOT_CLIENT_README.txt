PokerTH Bot Client - Automated Testing Infrastructure
=====================================================

ÜBERSICHT
=========
Headless Bot-Clients für realistische Performance- und Bug-Tests ohne echte Spieler.

DATEIEN
=======
- src/pokerth_bot.cpp        - Bot Client Implementation
- test_with_bots.sh           - Test-Script für einfache Nutzung
- CMakeLists.txt              - Build-Integration (pokerth_bot Target)

FEATURES
========
✅ Modernes Protocol Buffer Protokoll
✅ TLS-Support (Plain-Text Auth über TLS)
✅ Multiple Bots gleichzeitig
✅ Game Creation & Join
✅ Konfigurierbar (Server, Port, Anzahl Bots)

BUILD
=====
cd build
cmake ..
ninja pokerth_bot

# Verify:
ls -lh build/bin/pokerth_bot

NUTZUNG
=======

1. Game mit 10 Bots erstellen:
   ./test_with_bots.sh pokerth.net 7234 10 1 create

2. 9 Bots zu bestehendem Game hinzufügen:
   ./test_with_bots.sh pokerth.net 7234 9 11 join 12345

3. Manuell mit mehr Optionen:
   ./build/bin/pokerth_bot -s pokerth.net -p 7234 -b 10 -i 1 -c

4. Parallel mit echtem Client testen:
   Terminal 1: ./test_with_bots.sh localhost 7234 9 1 create
   Terminal 2: ./build/bin/pokerth_client  # Join zum Bot-Game

OPTIONEN
========
  -s, --server <addr>     Server address (default: localhost)
  -p, --port <port>       Server port (default: 7234)
  -b, --bots <n>          Number of bots (default: 10)
  -i, --start-id <id>     First bot ID (test<id>) (default: 1)
  -c, --create-game       Create game with first bot
  -g, --game-name <name>  Game name (default: "Bot Test Game")
  -j, --join-game <id>    Join existing game
  --no-tls                Disable TLS (plain TCP)

BOT ACCOUNTS
============
Bots nutzen test* Accounts:
  Username: test1, test2, ..., test100
  Password: test1, test2, ..., test100 (gleich wie Username)

Diese Accounts müssen auf dem Server existieren!

TEST-SZENARIEN
==============

Performance-Test (Bug #1 - TLS Latenz):
  time ./test_with_bots.sh pokerth.net 7234 10 1 create
  → Messe Login-Zeit, Handshake-Dauer

Coins-Stack Bug (Bug #4):
  - Starte Bots + echten Client
  - Verliere Hand
  - Prüfe Coins-Display

Reconnect Bug (Bug #5):
  - Game mit Bots erstellen
  - Verliere Game
  - Versuche Reconnect aus Lobby

DB Ranking Bug (Bug #9):
  - Spiele komplettes Game mit Bots
  - Prüfe player_ranking Tabelle

DEBUGGING
=========
Bots loggen zu stdout:
  [test1] Connected
  [test1] Logged in, Player ID: 42
  [test1] Created game ID: 12345
  [test2] Joined game 12345

Bei Fehlern:
  [test3] Connect exception: Connection refused
  [test4] Join game failed

ERWEITERUNGEN (TODO)
====================
Aktuell: Passive Bots (empfangen nur, spielen nicht aktiv)

Mögliche Erweiterungen:
  - Auto-Play: Bots spielen automatisch (Call/Fold/Raise)
  - Aggressive vs. Passive Bot-Profiles
  - Reconnect-Simulation
  - Stress-Testing (viele schnelle Aktionen)

VORTEILE
========
✅ Tests ohne echte Spieler
✅ Reproduzierbare Szenarien
✅ Performance-Messungen unter Last
✅ Parallel zum echten Client nutzbar
✅ Schnelle Iteration beim Debugging

NEXT STEPS
==========
1. Build: ninja pokerth_bot
2. Test: ./test_with_bots.sh localhost 7234 10 1 create
3. Kombiniert mit echtem Client testen
4. Performance-Bugs analysieren (Bug #1)
