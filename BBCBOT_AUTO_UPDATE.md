# BBCBot Auto-Update Feature

## Implementierung abgeschlossen ✅

### Überblick
Der bbcbot-qt6 lädt jetzt alle 10 Minuten automatisch die neuesten Bot-Dateien vom BBC-Server herunter und aktualisiert sich selbst.

### Features

#### 1. Automatischer Timer
- Alle 10 Minuten (600 Sekunden) wird `bot_every10min()` aufgerufen
- Startet automatischen Download aller Bot-Dateien
- Nach erfolgreichem Download: Automatisches Reload via `bot_loadfiles()`

#### 2. Qt-basierter HTTP(S) Download
- Verwendet `QNetworkAccessManager` für asynchrone Downloads
- SSL-Verifizierung deaktiviert (wie `curl -k`)
- 24 Dateien werden parallel heruntergeladen

#### 3. Dateien die heruntergeladen werden
Von `https://bbc.pokerth.net/exp3/bbcbot/`:

**Settings-Dateien:**
- `bbcup_settings.txt`, `bbcupfinal_settings.txt`
- `mcup_settings.txt`, `mcupfinal_settings.txt`
- `step1_settings.txt`, `step2_settings.txt`, `step3_settings.txt`, `step4_settings.txt`
- `wec_settings.txt`, `wecgfinal_settings.txt`, `wecmfinal_settings.txt`
- `husc_settings.txt`, `husctest2_settings.txt`
- `duckscup_settings.txt`

**Listen & Daten:**
- `fixedcommands.txt`, `manual_fixedcommands.txt`
- `permissions.txt`, `manual_permissions.txt`
- `gameslist.txt`
- `minidb.txt` (Player-Datenbank)
- `weclist.txt`, `newweclist.txt`, `weclist.old3.txt`
- `hash2.txt`

#### 4. Automatisches Verzeichnis-Management
- `botfiles/` Verzeichnis wird automatisch erstellt falls nicht vorhanden
- Alle Dateien werden in `botfiles/` gespeichert

### Implementierte Dateien

#### [src/net/clientthread.h](src/net/clientthread.h)
```cpp
void bot_downloadfiles();
```

#### [src/net/clientthread.cpp](src/net/clientthread.cpp)

**Timer-Callback (alle 10 Minuten):**
```cpp
void ClientThread::bot_every10min()
{
    std::cout << "[BBCBot] Running 10-minute maintenance tasks" << std::endl;
    bot_downloadfiles();
}
```

**Download-Implementierung:**
```cpp
void ClientThread::bot_downloadfiles()
{
    // 1. Create botfiles/ directory if needed
    QDir botfilesDir("botfiles");
    if (!botfilesDir.exists()) {
        botfilesDir.mkpath(".");
    }
    
    // 2. Define all 24 files to download
    QStringList files;
    files << "bbcupfinal_settings.txt" << "gameslist.txt" << ...;
    
    // 3. Setup QNetworkAccessManager
    QNetworkAccessManager *manager = new QNetworkAccessManager();
    
    // 4. Download each file asynchronously
    for (const QString &filename : files) {
        QUrl url(baseUrl + filename);
        QNetworkRequest request(url);
        
        // Disable SSL verification (curl -k)
        QSslConfiguration sslConfig = request.sslConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        request.setSslConfiguration(sslConfig);
        
        QNetworkReply *reply = manager->get(request);
        
        // 5. Save when download completes
        QObject::connect(reply, &QNetworkReply::finished, [...]() {
            if (reply->error() == QNetworkReply::NoError) {
                QFile file("botfiles/" + filename);
                file.open(QIODevice::WriteOnly);
                file.write(reply->readAll());
                file.close();
            }
            
            // 6. Reload bot files when all downloads complete
            if (allDownloadsComplete) {
                bot_loadfiles();
            }
        });
    }
}
```

### Console-Output

```
[BBCBot] Running 10-minute maintenance tasks
[BBCBot] Downloading updated bot files from server...
[BBCBot] Downloaded: fixedcommands.txt
[BBCBot] Downloaded: minidb.txt
[BBCBot] Downloaded: permissions.txt
... (24 files total) ...
[BBCBot] All files downloaded. Reloading bot files...
[BBCBot] Loading bot files...
[BBCBot] Loaded 42 fixed commands
```

### Workflow

1. **Timer-Event** (alle 10 Min) → `bbcbotTimerCallback()` inkrementiert `bot.stdcount`
2. **Trigger Check** → `bot.stdcount % 600 == 0` (alle 10 Min = 600 Sekunden)
3. **Download Start** → `bot_downloadfiles()` wird aufgerufen
4. **Parallel Downloads** → Alle 24 Dateien werden gleichzeitig heruntergeladen
5. **Speichern** → Jede Datei wird in `botfiles/` gespeichert
6. **Counter** → Zählt abgeschlossene Downloads
7. **Reload** → Nach allen Downloads: `bot_loadfiles()` lädt neue Daten

### Vorteile

✅ **Immer aktuell** - Bot verwendet stets die neuesten Daten  
✅ **Keine manuelle Intervention** - Vollautomatisch alle 10 Minuten
✅ **Parallel Downloads** - Schnell durch asynchrone Requests
✅ **Fehlertoleranz** - Einzelne fehlgeschlagene Downloads stoppen den Rest nicht
✅ **Logging** - Jeder Download wird in Console geloggt

### Qt Network Module

**Verwendete Qt-Klassen:**
- `QNetworkAccessManager` - HTTP(S) Request-Manager
- `QNetworkRequest` - Request-Konfiguration
- `QNetworkReply` - Response-Handling
- `QSslConfiguration` - SSL-Einstellungen
- `QDir` - Verzeichnis-Management
- `QFile` - Datei-I/O

**Includes:**
```cpp
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QUrl>
#include <QDir>
```

### Build & Test

```bash
cd /home/min/Development/pokerth/build
ninja pokerth_client
./bin/pokerth_client <password>
```

**Status:** ✅ Kompiliert erfolgreich

### Testing

Um das Feature zu testen:
1. Starte den Bot
2. Warte 10 Minuten (oder ändere temporär `600` zu `60` für 1-Minute-Tests)
3. Beobachte Console-Output für Download-Aktivität
4. Prüfe `botfiles/` Verzeichnis auf aktualisierte Dateien

### Alternative: Manuelles Trigger

Der `update` Command triggert ebenfalls ein Reload:
```
/msg bbcbot update
```
Dies lädt die Bot-Dateien neu, lädt sie aber nicht vom Server herunter (nur lokales Reload).

### Verwandte Dateien

- [BBCBOT_IMPLEMENTATION.md](BBCBOT_IMPLEMENTATION.md) - Vollständige Bot-Features
- [BBCBOT_STATUS.md](BBCBOT_STATUS.md) - Implementierungsstatus
- [BBCBOT_RECONNECT.md](BBCBOT_RECONNECT.md) - Auto-Reconnect Feature
