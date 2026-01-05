# BBCBot Implementation Status

## Vollständig implementierte Features

### 1. Auto-Login System
- **Passwort-Übergabe**: Bot-Passwort wird über `argv[1]` übergeben
- **Automatischer Login**: Bot loggt sich automatisch mit gespeichertem Accountnamen ein
- **Fallback**: Verwendet accountname.txt falls kein Passwort übergeben wurde

### 2. Auto-Start zum Internet Lobby
- **QTimer Integration**: 100ms Verzögerung für zuverlässiges Auto-Connect
- **State Management**: Wartet auf richtigen Zustand vor dem Verbinden

### 3. Bot Datenstrukturen

#### GameCreateState Enum
- `GS_NORMAL`: Normaler Zustand, bereit für neue Befehle
- `GS_GOTCOMMAND`: Befehl empfangen, Spiel wird erstellt
- `GS_CREATED`: Spiel erstellt, bereit zum Einladen
- `GS_SENDINV`: Einladung gesendet, wartet auf Annahme
- `GS_ACCEPTED`: Einladung angenommen, verlässt Spiel nach Timer

#### bbcbotpermissiongroup
- `std::list<std::string> players`: Liste der Spieler
- `bool isblacklist`: Flag für Blacklist (true) oder Whitelist (false)

#### bbcbotgamedata
- `std::string commandname`: Befehlsname für das Spiel-Template
- `std::string gamenameprefix`: Präfix für erstellte Spielnamen
- `GameData gdata`: Spiel-Konfigurationsdaten
- `bbcbotpermissiongroup* pgroup`: Zeiger auf Berechtigungsgruppe

#### bbcbotdata
- `bool enabled`: Bot aktiviert/deaktiviert
- `unsigned creatorid`: ID des Spielers, der das Spiel anfordert
- `GameCreateState creategamestate`: Aktueller Zustand der Spielerstellung
- `int countdowninvite`: Countdown bis zur Einladung (Sekunden)
- `int countdownleave`: Countdown bis zum Verlassen (Sekunden)
- `int countdowninvitetimeout`: Timeout für Einladung (30 Sekunden)
- `unsigned stdcount`: Uptime-Zähler (Sekunden)
- `std::vector<std::string> fixedcommands`: Sortierte Liste fester Befehle
- `std::vector<std::string> fixedreply`: Entsprechende Antworten
- `std::list<bbcbotgamedata> gdata`: Liste der Spiel-Templates
- `std::list<bbcbotpermissiongroup> permgroups`: Berechtigungsgruppen

#### bbcbotplayerdb
- `std::map<unsigned, std::string> playerNames`: Spieler-ID zu Namen
- `std::set<unsigned> idleplayers`: Set inaktiver Spieler
- Methoden:
  - `addidleplayer(unsigned playerid)`
  - `removeidleplayer(unsigned playerid)`
  - `printidledebug()`: Debug-Ausgabe auf Konsole
  - `getidleplayersinfo()`: Gibt Idle-Spieler Info zurück
  - `printrating(playername)`: Rating-Information
  - `printtickets(playername)`: Ticket-Information
  - `printgamescount(playername)`: Spiele-Statistik
  - `printsuggest(step)`: Suggest-Algorithmus (Schritte 1-4)
  - `wecsuggest()`: WEC Suggest-Variante

### 4. Bot File Loading

#### bot_loadfiles()
- **fixedcommands.txt**: Lädt feste Befehle im Format "befehl=antwort"
- **Sortierung**: Sortiert Befehle alphabetisch für binäre Suche
- **gametemplates.txt**: Lädt Spiel-Templates (Implementierung ausstehend)

### 5. Timer System

#### bbcbotTimerCallback()
- **1-Sekunden-Tick**: Wird jede Sekunde aufgerufen
- **Uptime**: Inkrementiert `stdcount`
- **State Machine**: Verwaltet Spielerstellungs-Zustände
  - `GS_CREATED` → sendet Einladung → `GS_SENDINV`
  - `GS_SENDINV` → prüft Timeout (30 Sek.) → sendet Fehler, verlässt Spiel
  - `GS_ACCEPTED` → wartet 2 Sekunden → verlässt Spiel
- **Periodische Aktionen**: Ruft alle 600 Sekunden (10 Min.) `bot_every10min()` auf

#### Hilfsmethoden
- `bot_invite()`: Sendet Spieleinladung an Creator
- `bot_invitetimeout()`: Behandelt Timeout, sendet Fehlermeldung
- `bot_leave()`: Verlässt Spiel nach erfolgreicher Annahme
- `bot_every10min()`: Placeholder für periodische Wartung

### 6. Bot Commands

Alle Befehle sind vollständig implementiert und funktional:

#### Basis-Befehle
- **time**: Zeigt aktuelle Zeit (Wochentag, Stunde:Minute:Sekunde)
- **help**: Listet alle verfügbaren Befehle
- **uptime**: Zeigt Bot-Laufzeit in Sekunden
- **update**: Lädt Bot-Dateien neu (fixedcommands.txt, gametemplates.txt)
- **debug**: Zeigt Debug-Informationen (Idle-Spieler-Anzahl)

#### Admin-Befehle
- **gn <text>**: Global Notice (nur Server-seitig, Client ignoriert)

#### Spiel-Befehle
- **create <type> <name>**: Erstellt Spiel basierend auf Template
  - Syntaxprüfung
  - Template-Suche
  - Berechtigungsprüfung (Blacklist/Whitelist)
  - Busy-Check (nur ein Spiel gleichzeitig)
  - State Machine Integration

#### Datenbank-Befehle
- **rating <player>**: Zeigt Rating-Informationen
- **tickets <player>**: Zeigt Ticket-Informationen
- **games <player>**: Zeigt Spiele-Statistik

#### Suggest-Befehle
- **suggest s1** / **suggest step1** / **suggest1**: Suggest Schritt 1
- **suggest s2** / **suggest step2** / **suggest2**: Suggest Schritt 2
- **suggest s3** / **suggest step3** / **suggest3**: Suggest Schritt 3
- **suggest s4** / **suggest step4** / **suggest4**: Suggest Schritt 4
- **suggest wec** / **suggestwec**: WEC Suggest-Variante

#### Feste Befehle
- **Binäre Suche**: Alle Befehle aus fixedcommands.txt werden via binärer Suche gefunden
- **Fallback**: Wenn kein anderer Befehl matched, wird in fixedcommands gesucht

### 7. Hilfsfunktionen

#### Helper Functions (clientstate.cpp)
- `int2string(int i)`: Konvertiert Integer zu String
- `ciscompare(s1, s2)`: Case-insensitive String-Vergleich
- `bot_sendlongpm(client, playerId, text)`: Teilt lange Nachrichten automatisch bei 120 Zeichen
- `bot_fixedcommandssearch(client, command)`: Binäre Suche in fixedcommands

### 8. Architektur

#### Netzwerk-Layer (korrekt)
- Bot-Logik in `src/net/clientstate.cpp`
- Behandelt `chatTypePrivate` Nachrichten in `AbstractClientStateReceiving::HandlePacket()`
- Alle Befehle werden im Network Layer verarbeitet

#### Datenstrukturen
- `ClientThread::bot` (bbcbotdata): Haupt-Bot-Daten
- `ClientThread::botdb` (bbcbotplayerdb): Spieler-Datenbank
- Beide in `src/net/clientthread.h` deklariert

## Noch ausstehende Implementierungen

### 1. Datenbank-Integration
- Echte Datenbank-Abfragen für:
  - Rating-System
  - Ticket-System
  - Spiele-Statistik
- Aktuell: Placeholder-Strings werden zurückgegeben

### 2. Suggest-Algorithmus
- Implementierung des tatsächlichen Suggest-Algorithmus
- Aktuell: Placeholder-Strings

### 3. Game Templates
- Parsen von gametemplates.txt
- GameData-Strukturen füllen
- Aktuell: Datei wird geladen, aber nicht geparst

### 4. Periodische Wartung
- `bot_every10min()` Implementierung
- Mögliche Features:
  - Datenbank-Cleanup
  - Idle-Player-Management
  - Statistik-Updates

## Dateien

### Konfigurationsdateien (in build/bin/)
- **fixedcommands.txt**: Feste Befehle im Format "befehl=antwort"
  - Muss alphabetisch sortiert sein (oder wird beim Laden sortiert)
  - Beispiel: `about=I am BBCBot, a poker bot`
- **gametemplates.txt**: Spiel-Templates (Format noch zu definieren)
- **accountname.txt**: Bot-Account-Name für Auto-Login

### Quellcode-Dateien
- **src/session.h**: bbcbotpassword Feld
- **src/pokerth.cpp**: Passwort-Extraktion aus argv[1]
- **src/gui/qt/startwindow/startwindowimpl.cpp**: Auto-Login und Auto-Start
- **src/net/clientthread.h**: Bot-Datenstrukturen
- **src/net/clientthread.cpp**: Bot-Methoden, Timer, File-Loading
- **src/net/clientstate.cpp**: Befehlsverarbeitung, Helper-Funktionen

## Build Status

✅ **Kompiliert erfolgreich**
- Alle Strukturen korrekt definiert
- Alle Includes vorhanden
- Keine Compiler-Fehler

## Test-Empfehlungen

1. **Auto-Login testen**: Bot mit Passwort-Parameter starten
2. **Befehle testen**: Private Nachricht an Bot senden
   - `time` → sollte aktuelle Zeit zeigen
   - `help` → sollte alle Befehle auflisten
   - `test` → sollte Antwort aus fixedcommands.txt zeigen
3. **Create-Befehl**: Erfordert gametemplates.txt mit Spiel-Definitionen
4. **Datenbank-Befehle**: Geben aktuell Placeholder zurück

## Nächste Schritte

1. **Game Templates definieren**: Format festlegen und gametemplates.txt füllen
2. **Datenbank-Anbindung**: Rating/Tickets/Games aus echter Datenbank
3. **Suggest-Algorithmus**: Von Legacy-Code portieren
4. **Periodische Wartung**: bot_every10min() implementieren
5. **Testing**: Mit echten Benutzern im Spiel testen
