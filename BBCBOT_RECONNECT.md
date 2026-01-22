# BBCBot Auto-Reconnect Feature

## Implementierung abgeschlossen ✅

### Überblick
Der bbcbot-qt6 hat jetzt eine vollständige automatische Wiederverbindungsfunktion, die bei Verbindungsabbrüchen automatisch eine Neuverbindung zum Server versucht.

### Features

#### 1. Automatische Erkennung von Verbindungsabbrüchen
- Erkennt `ERR_SOCK_CONN_RESET` (Connection Reset)
- Erkennt `ERR_SOCK_RECV_FAILED` (Receive Failed)

#### 2. Intelligentes Retry-Timing mit Exponential Backoff
```cpp
Versuch 1: nach 5 Sekunden
Versuch 2: nach 10 Sekunden
Versuch 3: nach 15 Sekunden
Versuch 4: nach 20 Sekunden
Versuch 5: nach 25 Sekunden
Versuch 6+: nach 30 Sekunden (Maximum)
```

#### 3. Automatisches Zurücksetzen bei Erfolg
- Bei erfolgreicher Verbindung wird der Reconnect-Counter zurückgesetzt
- Signal: `signalNetClientSelfJoined` triggert `bbcbotResetReconnectAttempts()`

#### 4. Nur für BBCBot aktiviert
- Feature ist nur aktiv, wenn Bot-Passwort gesetzt ist
- Normale Clients bekommen weiterhin die Fehlermeldung

### Implementierte Dateien

#### [src/gui/qt/startwindow/startwindowimpl.h](src/gui/qt/startwindow/startwindowimpl.h)
```cpp
// Neue Member-Variablen:
QTimer *bbcbotReconnectTimer;
bool bbcbotReconnectEnabled;
int bbcbotReconnectAttempts;

// Neue Slots:
void bbcbotAttemptReconnect();
void bbcbotResetReconnectAttempts();
```

#### [src/gui/qt/startwindow/startwindowimpl.cpp](src/gui/qt/startwindow/startwindowimpl.cpp)

**Konstruktor-Initialisierung:**
```cpp
if (!mySession->bbcbotpassword.empty()) {
    bbcbotReconnectTimer = new QTimer(this);
    bbcbotReconnectTimer->setSingleShot(true);
    connect(bbcbotReconnectTimer, SIGNAL(timeout()), 
            this, SLOT(bbcbotAttemptReconnect()));
    bbcbotReconnectEnabled = true;
    bbcbotReconnectAttempts = 0;
}
```

**Error-Handler mit Auto-Reconnect:**
```cpp
case ERR_SOCK_RECV_FAILED:
case ERR_SOCK_CONN_RESET: {
    if (bbcbotReconnectEnabled && !mySession->bbcbotpassword.empty()) {
        bbcbotReconnectAttempts++;
        int delaySeconds = std::min(5 + (bbcbotReconnectAttempts - 1) * 5, 30);
        std::cout << "[BBCBot] Connection lost. Attempting reconnect #" 
                  << bbcbotReconnectAttempts << " in " << delaySeconds 
                  << " seconds..." << std::endl;
        bbcbotReconnectTimer->start(delaySeconds * 1000);
    } else {
        // Normal error message for non-bot clients
        MyMessageBox::warning(...);
    }
}
```

**Reconnect-Methode:**
```cpp
void startWindowImpl::bbcbotAttemptReconnect()
{
    std::cout << "[BBCBot] Attempting to reconnect to server..." << std::endl;
    
    // Close dialogs
    if (myGameLobbyDialog && myGameLobbyDialog->isVisible()) {
        myGameLobbyDialog->close();
    }
    if (myStartNetworkGameDialog && myStartNetworkGameDialog->isVisible()) {
        myStartNetworkGameDialog->close();
    }
    
    // Terminate old connection
    if (mySession) {
        mySession->terminateNetworkClient();
    }
    
    // Trigger reconnect after cleanup delay
    QTimer::singleShot(500, this, SLOT(callGameLobbyDialog()));
}
```

**Reset-Methode:**
```cpp
void startWindowImpl::bbcbotResetReconnectAttempts()
{
    if (bbcbotReconnectEnabled) {
        std::cout << "[BBCBot] Connection successful. "
                  << "Resetting reconnect attempts counter." << std::endl;
        bbcbotReconnectAttempts = 0;
    }
}
```

### Workflow

1. **Verbindungsabbruch erkannt** → `networkError()` wird aufgerufen
2. **BBCBot-Check** → Ist `bbcbotReconnectEnabled` und Passwort gesetzt?
3. **Timer starten** → Exponential Backoff berechnen und Timer starten
4. **Reconnect-Versuch** → `bbcbotAttemptReconnect()` wird ausgeführt:
   - Dialogs schließen
   - Alte Verbindung terminieren
   - Nach 500ms Neuverbindung via `callGameLobbyDialog()`
5. **Erfolg?** → `bbcbotResetReconnectAttempts()` setzt Counter zurück
6. **Fehler?** → Gehe zu Schritt 3 mit höherem Delay

### Inspiriert von IRC-Reconnect-Logik

Die Implementierung orientiert sich an der bewährten IRC-Thread Reconnect-Logik:
- `IRC_MIN_RECONNECT_INTERVAL_SEC = 60` (für IRC Bots)
- Automatische Wiederverbindungsschleife im `IrcThread::Main()`
- Client-Reconnect verwendet ähnliches Pattern, aber GUI-basiert

### Vorteile

✅ **Keine manuelle Intervention nötig** - Bot verbindet sich automatisch wieder
✅ **Exponential Backoff** - Verhindert Server-Überlastung bei Massenabbrüchen  
✅ **Transparente Logging** - Console-Output für jeden Reconnect-Versuch
✅ **Clean State Management** - Counter wird bei Erfolg zurückgesetzt
✅ **Abwärtskompatibel** - Normale Clients sind nicht betroffen

### Build & Test

```bash
cd /home/min/Development/pokerth/build
ninja pokerth_client
```

**Status:** ✅ Kompiliert erfolgreich ohne Fehler

### Nächste Schritte

1. **Testing mit echtem Server** - Verbindungsabbrüche simulieren
2. **Max-Retry-Limit** (optional) - Nach X Versuchen aufgeben?
3. **Reconnect-Delay konfigurierbar** - Via Config-File?
4. **Logging verbessern** - Zusätzlich ins Log-File schreiben?

### Verwandte Dateien

- [BBCBOT_IMPLEMENTATION.md](BBCBOT_IMPLEMENTATION.md) - Vollständige Bot-Features
- [BBCBOT_STATUS.md](BBCBOT_STATUS.md) - Implementierungsstatus
- [src/net/ircthread.cpp](src/net/ircthread.cpp#L373) - IRC Reconnect-Referenz
