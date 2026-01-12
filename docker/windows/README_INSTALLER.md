# PokerTH Windows Installer

Dieses Verzeichnis enthält die Konfiguration für den Windows-Installer von PokerTH.

## Übersicht

Der Build-Prozess erstellt automatisch einen professionellen Windows-Installer mit NSIS (Nullsoft Scriptable Install System), der folgende Features beinhaltet:

- ✅ Vollständige Installation aller benötigten Dateien
- ✅ Desktop-Verknüpfung mit Icon
- ✅ Startmenü-Einträge
- ✅ Deinstallationsprogramm
- ✅ Windows Add/Remove Programs Integration
- ✅ Automatische Icon-Konvertierung von SVG/PNG zu ICO
- ✅ Mehrsprachige Unterstützung (Deutsch/Englisch)
- ✅ Moderne Benutzeroberfläche

## Voraussetzungen

Die benötigten Tools sind bereits im Docker-Container installiert:
- **NSIS** (Nullsoft Scriptable Install System)
- **ImageMagick** (für Icon-Konvertierung)
- **MinGW** (Cross-Compiler für Windows)

## Verwendung

### Automatischer Build

Der Installer wird automatisch beim normalen Build-Prozess erstellt:

```bash
./build_windows.sh
```

Das Script führt folgende Schritte aus:
1. Kompiliert PokerTH für Windows
2. Sammelt alle DLLs und Abhängigkeiten
3. Kopiert Spieldaten (Grafiken, Sounds, Übersetzungen)
4. Konvertiert das Icon von SVG zu ICO
5. Erstellt den Installer mit NSIS
6. Ausgabe: `PokerTH-X.X.X-Setup.exe`

### Manueller Build

Falls nur der Installer neu erstellt werden soll:

```bash
cd docker/windows
makensis installer.nsi
```

## Installer-Konfiguration

Die Installer-Konfiguration befindet sich in `installer.nsi` und kann angepasst werden:

### Produktinformationen

```nsis
!define PRODUCT_NAME "PokerTH"
!define PRODUCT_VERSION "1.1.2"
!define PRODUCT_PUBLISHER "PokerTH Team"
!define PRODUCT_WEB_SITE "http://www.pokerth.net"
```

### Installationsverzeichnis

Standardmäßig wird installiert nach:
- `C:\Program Files\PokerTH`

### Startmenü

Der Installer erstellt folgende Einträge:
- **PokerTH** - Hauptprogramm
- **PokerTH Dedicated Server** - Dedizierter Server (falls vorhanden)
- **Uninstall** - Deinstallationsprogramm

## Icon-Verwaltung

Das Build-Script konvertiert automatisch das PokerTH-Logo:

1. **Quelle**: `pokerth.svg` (Vektorgrafik)
2. **Alternative**: `pokerth.png` (Rastergrafik)
3. **Ziel**: `pokerth.ico` (Multi-Resolution Windows Icon)

Die ICO-Datei enthält automatisch mehrere Auflösungen:
- 256x256, 128x128, 64x64, 48x48, 32x32, 16x16 Pixel

## Testen des Installers

### Mit Wine unter Linux

```bash
wine PokerTH-X.X.X-Setup.exe
```

### Auf Windows

Einfach die `PokerTH-X.X.X-Setup.exe` doppelklicken.

## Installer-Features

### Installation

- Lizenzvereinbarung (COPYING-Datei)
- Auswahl des Installationsverzeichnisses
- Auswahl des Startmenü-Ordners
- Fortschrittsanzeige
- Option zum direkten Start nach Installation

### Deinstallation

- Entfernt alle installierten Dateien
- Löscht Startmenü-Einträge
- Entfernt Desktop-Verknüpfung
- Bereinigt Registry-Einträge
- Hinterlässt ein sauberes System

## Troubleshooting

### NSIS nicht gefunden

Falls NSIS fehlt:
```bash
apt-get update && apt-get install -y nsis
```

### ImageMagick nicht gefunden

Falls ImageMagick fehlt:
```bash
apt-get update && apt-get install -y imagemagick
```

### Icon-Konvertierung schlägt fehl

Das Build-Script funktioniert auch ohne Icon-Konvertierung. NSIS verwendet dann das Standard-Icon.

### Fehlende DLLs im Installer

Das Build-Script sammelt automatisch alle benötigten DLLs aus:
- Qt Framework (`${QT_WINDOWS_DIR}/bin`)
- Qt Plugins (`${QT_WINDOWS_DIR}/plugins`)
- vcpkg Dependencies (`${VCPKG_ROOT}/installed/x64-mingw-static/bin`)
- MinGW Runtime (libgcc, libstdc++, libwinpthread)

## Erweiterungen

### Weitere Executables hinzufügen

Um weitere Programme zum Installer hinzuzufügen, bearbeite `installer.nsi`:

```nsis
CreateShortcut "$SMPROGRAMS\$StartMenuFolder\Mein Programm.lnk" "$INSTDIR\mein_programm.exe"
```

### Lizenzdatei ändern

Die Lizenz wird aus `../../COPYING` gelesen. Um eine andere Datei zu verwenden:

```nsis
!insertmacro MUI_PAGE_LICENSE "pfad/zur/lizenz.txt"
```

### Sprachen hinzufügen

Weitere Sprachen können hinzugefügt werden:

```nsis
!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "Spanish"
```

## Dateistruktur

```
docker/windows/
├── build_windows.sh       # Haupt-Build-Script
├── installer.nsi          # NSIS-Installer-Konfiguration
├── pokerth.ico           # Generiertes Windows-Icon (nach Build)
└── README_INSTALLER.md    # Diese Datei

../../build/deploy/        # Deployment-Verzeichnis (nach Build)
├── pokerth_client.exe
├── *.dll
├── plugins/
│   ├── platforms/
│   ├── styles/
│   └── ...
├── data/
│   ├── gfx/
│   ├── sounds/
│   └── translations/
└── qt.conf
```

## Ausgabe

Nach erfolgreichem Build:

```
docker/windows/
└── PokerTH-1.1.2-Setup.exe  (ca. 50-100 MB)
```

Der Installer enthält:
- Vollständige PokerTH-Anwendung
- Alle Qt-DLLs und Plugins
- Alle Spieldaten
- MinGW-Runtime-Bibliotheken
- Icons und Verknüpfungen

## Weitere Informationen

- NSIS Dokumentation: https://nsis.sourceforge.io/Docs/
- NSIS Modern UI: https://nsis.sourceforge.io/Docs/Modern%20UI/Readme.html
- PokerTH Website: http://www.pokerth.net
