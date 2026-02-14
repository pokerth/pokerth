#!/bin/bash
set -e

# Binary Deploy Script für PokerTH Linux
# Erstellt ein vollständiges Binary-Paket mit allen Abhängigkeiten

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
DEPLOY_DIR="${SCRIPT_DIR}/pokerth-linux-binary"
DEPLOY_NAME="pokerth-linux-$(uname -m)-$(date +%Y%m%d)"

echo "=== PokerTH Binary Deploy Erstellung ==="
echo "Project Root: $PROJECT_ROOT"
echo "Build Dir: $BUILD_DIR"
echo "Deploy Name: $DEPLOY_NAME"
echo ""

# Prüfe ob Build existiert
if [ ! -d "$BUILD_DIR" ]; then
    echo "ERROR: Build-Verzeichnis nicht gefunden: $BUILD_DIR"
    echo "Bitte führen Sie zuerst den Build-Prozess durch."
    exit 1
fi

# Prüfe ob Binaries existieren
if [ ! -f "$BUILD_DIR/bin/pokerth_client" ]; then
    echo "ERROR: pokerth_client Binary nicht gefunden!"
    exit 1
fi

# Erstelle Deploy-Verzeichnis
rm -rf "$DEPLOY_DIR"
mkdir -p "$DEPLOY_DIR"/{bin,lib,data,share}

echo "=== Kopiere Binaries ==="
cp -v "$BUILD_DIR/bin/pokerth_client" "$DEPLOY_DIR/bin/"
if [ -f "$BUILD_DIR/bin/pokerth_qml-client" ]; then
    cp -v "$BUILD_DIR/bin/pokerth_qml-client" "$DEPLOY_DIR/bin/"
fi

# Kopiere botfiles falls vorhanden
if [ -d "$BUILD_DIR/bin/botfiles" ]; then
    echo "=== Kopiere Botfiles ==="
    cp -rv "$BUILD_DIR/bin/botfiles" "$DEPLOY_DIR/bin/"
fi

echo ""
echo "=== Sammle Abhängigkeiten ==="

# Funktion zum Sammeln aller Abhängigkeiten
collect_dependencies() {
    local binary="$1"
    local lib_dir="$2"
    
    echo "Analysiere: $(basename $binary)"
    
    # Sammle alle Bibliotheken rekursiv
    local processed_libs="$lib_dir/.processed"
    touch "$processed_libs"
    
    process_binary() {
        local bin="$1"
        
        # Verwende Array statt Pipe, um Subshell zu vermeiden
        local libs=()
        while IFS= read -r line; do
            local lib=$(echo "$line" | grep "=>" | awk '{print $3}')
            [ -n "$lib" ] && libs+=("$lib")
        done < <(ldd "$bin" 2>/dev/null)
        
        for lib in "${libs[@]}"; do
            if [ -z "$lib" ] || [ ! -f "$lib" ]; then
                continue
            fi
            
            local libname="$(basename $lib)"
            
            # Überspringe grundlegende glibc-Bibliotheken
            # sowie PulseAudio/ALSA Client-Libs (müssen zum System-Audiodaemon passen)
            case "$libname" in
                libc.so.* | libc-*.so | \
                libm.so.* | libm-*.so | \
                libdl.so.* | libdl-*.so | \
                libpthread.so.* | libpthread-*.so | \
                librt.so.* | librt-*.so | \
                libresolv.so.* | libresolv-*.so | \
                libutil.so.* | libutil-*.so | \
                libnsl.so.* | libnsl-*.so | \
                ld-linux*.so.* | ld-*.so | \
                libpulse.so.* | libpulse-simple.so.* | libpulsecommon-*.so | \
                libasound.so.*)
                    continue
                    ;;
            esac
            
            # Skip wenn schon bearbeitet
            if grep -q "^${lib}$" "$processed_libs" 2>/dev/null; then
                continue
            fi
            
            # Markiere als bearbeitet
            echo "$lib" >> "$processed_libs"
            
            # Kopiere die Bibliothek und setze Ausführungsrechte
            if [ ! -f "$lib_dir/$libname" ]; then
                cp -L "$lib" "$lib_dir/" 2>/dev/null && chmod +x "$lib_dir/$libname" && echo "  + $libname" || true
            fi
            
            # Rekursiv die Abhängigkeiten dieser Bibliothek sammeln
            process_binary "$lib"
        done
    }
    
    process_binary "$binary"
    rm -f "$processed_libs"
}

# Sammle Abhängigkeiten für alle Binaries
for binary in "$DEPLOY_DIR/bin/pokerth_client" "$DEPLOY_DIR/bin/pokerth_qml-client"; do
    if [ -f "$binary" ]; then
        collect_dependencies "$binary" "$DEPLOY_DIR/lib"
    fi
done

echo ""
echo "=== Sammle Qt-Plugins ==="
# Finde Qt6-Plugin-Verzeichnis
QT6_PLUGINS=$(find /usr/lib* -type d -name "qt6" -path "*/plugins" 2>/dev/null | head -1)
if [ -z "$QT6_PLUGINS" ]; then
    QT6_PLUGINS="/usr/lib/x86_64-linux-gnu/qt6/plugins"
fi

if [ -d "$QT6_PLUGINS" ]; then
    echo "Qt6 Plugins gefunden: $QT6_PLUGINS"
    
    # Kopiere wichtige Plugin-Kategorien
    for plugin_category in platforms xcbglintegrations platforminputcontexts imageformats platformthemes multimedia sqldrivers tls; do
        if [ -d "$QT6_PLUGINS/$plugin_category" ]; then
            echo "Kopiere $plugin_category plugins..."
            mkdir -p "$DEPLOY_DIR/plugins/$plugin_category"
            cp -v "$QT6_PLUGINS/$plugin_category"/*.so "$DEPLOY_DIR/plugins/$plugin_category/" 2>/dev/null || true
            chmod +x "$DEPLOY_DIR/plugins/$plugin_category"/*.so 2>/dev/null || true
            
            # Sammle Abhängigkeiten der Plugins
            for plugin in "$DEPLOY_DIR/plugins/$plugin_category"/*.so; do
                [ -f "$plugin" ] && collect_dependencies "$plugin" "$DEPLOY_DIR/lib"
            done
        fi
    done
else
    echo "WARNUNG: Qt6 Plugins nicht gefunden in $QT6_PLUGINS"
fi

echo ""
echo "=== Kopiere Data-Verzeichnis ==="
if [ -d "$PROJECT_ROOT/data" ]; then
    cp -rv "$PROJECT_ROOT/data"/* "$DEPLOY_DIR/data/"
fi

# Erstelle Symlink-Struktur für PokerTH's Datei-Such-Logik
# PokerTH sucht: bin/../share/pokerth/data/ wenn Binary in bin/ liegt
echo ""
echo "=== Erstelle Share-Symlink-Struktur ==="
mkdir -p "$DEPLOY_DIR/share/pokerth"
ln -sf "../../data" "$DEPLOY_DIR/share/pokerth/data"

echo ""
echo "=== Kopiere zusätzliche Ressourcen ==="
# Desktop-Dateien
if [ -f "$PROJECT_ROOT/pokerth.desktop" ]; then
    cp -v "$PROJECT_ROOT/pokerth.desktop" "$DEPLOY_DIR/share/"
fi
if [ -f "$PROJECT_ROOT/pokerth_qml.desktop" ]; then
    cp -v "$PROJECT_ROOT/pokerth_qml.desktop" "$DEPLOY_DIR/share/"
fi

# Lua-Script
if [ -f "$PROJECT_ROOT/pokerth.lua" ]; then
    cp -v "$PROJECT_ROOT/pokerth.lua" "$DEPLOY_DIR/share/"
fi

# Dokumentation
if [ -d "$PROJECT_ROOT/docs" ]; then
    mkdir -p "$DEPLOY_DIR/docs"
    cp -rv "$PROJECT_ROOT/docs"/* "$DEPLOY_DIR/docs/"
fi

# Lizenz
if [ -f "$PROJECT_ROOT/COPYING" ]; then
    cp -v "$PROJECT_ROOT/COPYING" "$DEPLOY_DIR/"
fi

# ChangeLog
if [ -f "$PROJECT_ROOT/ChangeLog" ]; then
    cp -v "$PROJECT_ROOT/ChangeLog" "$DEPLOY_DIR/"
fi

echo ""
echo "=== Erstelle qt.conf für Plugin-Pfade ==="
# qt.conf neben dem Binary ist die zuverlässigste Methode für Qt Plugin-Pfade
cat > "$DEPLOY_DIR/bin/qt.conf" << 'EOF'
[Paths]
Plugins = ../plugins
Libraries = ../lib
EOF
echo "qt.conf erstellt in bin/"

echo ""
echo "=== Erstelle Launcher-Scripts ==="

# Launcher für pokerth_client
cat > "$DEPLOY_DIR/pokerth" << 'EOF'
#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export LD_LIBRARY_PATH="$SCRIPT_DIR/lib:$LD_LIBRARY_PATH"
export QT_PLUGIN_PATH="$SCRIPT_DIR/plugins:$QT_PLUGIN_PATH"
export QT_QPA_PLATFORM_PLUGIN_PATH="$SCRIPT_DIR/plugins/platforms"

# Audio: FFmpeg-Backend explizit setzen (Qt6 Multimedia auf Linux)
export QT_MEDIA_BACKEND=ffmpeg

# Debug-Modus: mit --debug-audio starten für ausführliche Audio/Plugin-Diagnose
if [[ "$1" == "--debug-audio" ]]; then
    shift
    export QT_DEBUG_PLUGINS=1
    export QT_LOGGING_RULES="qt.multimedia.*=true"
    echo "[DEBUG] LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
    echo "[DEBUG] QT_PLUGIN_PATH=$QT_PLUGIN_PATH"
    echo "[DEBUG] QT_MEDIA_BACKEND=$QT_MEDIA_BACKEND"
    echo "[DEBUG] Multimedia plugins:"
    ls -la "$SCRIPT_DIR/plugins/multimedia/" 2>/dev/null || echo "  (keine gefunden!)"
    echo "[DEBUG] PulseAudio libs in deploy:"
    ls "$SCRIPT_DIR/lib/" | grep -i pulse || echo "  (keine gefunden!)"
fi

cd "$SCRIPT_DIR"
# Setze Working Directory sodass bin/../data/ gefunden wird
exec "$SCRIPT_DIR/bin/pokerth_client" "$@"
EOF
chmod +x "$DEPLOY_DIR/pokerth"

# Launcher für pokerth_qml-client (falls vorhanden)
if [ -f "$DEPLOY_DIR/bin/pokerth_qml-client" ]; then
    cat > "$DEPLOY_DIR/pokerth-qml" << 'EOF'
#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export LD_LIBRARY_PATH="$SCRIPT_DIR/lib:$LD_LIBRARY_PATH"
export QT_PLUGIN_PATH="$SCRIPT_DIR/plugins:$QT_PLUGIN_PATH"
export QT_QPA_PLATFORM_PLUGIN_PATH="$SCRIPT_DIR/plugins/platforms"
export QT_MEDIA_BACKEND=ffmpeg
cd "$SCRIPT_DIR"
exec "$SCRIPT_DIR/bin/pokerth_qml-client" "$@"
EOF
    chmod +x "$DEPLOY_DIR/pokerth-qml"
fi

echo ""
echo "=== Erstelle README ==="
cat > "$DEPLOY_DIR/README.txt" << EOF
PokerTH Binary Distribution for Linux
======================================

This is a portable binary distribution of PokerTH for Linux.
It contains all necessary dependencies and can be run without installation.

INSTALLATION:
-------------
1. Extract the archive to any location
2. Run ./pokerth to start the game

RUNNING:
--------
Standard Client (Qt):
  ./pokerth

QML Client (if available):
  ./pokerth-qml

IMPORTANT:
----------
⚠️  ALWAYS use the launcher scripts (./pokerth or ./pokerth-qml)!
⚠️  DO NOT run the binaries in bin/ directly - they won't find
    libraries and data files.

The launcher scripts automatically set the correct environment variables:
  - LD_LIBRARY_PATH for the included libraries
  - QT_PLUGIN_PATH for the Qt plugins
  - Working directory for the data files

SYSTEM REQUIREMENTS:
--------------------
- Linux mit glibc 2.x
- X11 oder Wayland Display-Server
- OpenGL-fähige Grafikkarte (empfohlen)

PROBLEMBEHEBUNG:
----------------
Falls die Anwendung nicht startet, versuchen Sie:

1. Prüfen Sie fehlende System-Bibliotheken:
   ldd ./bin/pokerth_client

2. Stellen Sie sicher, dass Sie die notwendigen Berechtigungen haben:
   chmod +x ./pokerth

3. Für Audio-Unterstützung benötigen Sie möglicherweise PulseAudio oder ALSA

LIZENZ:
-------
Siehe COPYING für Lizenzinformationen.

WEITERE INFORMATIONEN:
----------------------
Homepage: https://www.pokerth.net/
GitHub: https://github.com/pokerth/pokerth

Build-Informationen:
- Build-Datum: $(date)
- Architektur: $(uname -m)
- System: $(uname -s)

EOF

echo ""
echo "=== Erstelle Archiv ==="
cd "$(dirname $DEPLOY_DIR)"
# tar czf "${DEPLOY_NAME}.tar.gz" "$(basename $DEPLOY_DIR)"

# Erstelle auch ZIP für bessere Benutzerfreundlichkeit
if command -v zip &> /dev/null; then
    zip -qr "${DEPLOY_NAME}.zip" "$(basename $DEPLOY_DIR)"
    echo "ZIP-Archiv erstellt: ${DEPLOY_NAME}.zip"
else
    echo "WARNUNG: zip-Programm nicht gefunden, ZIP-Archiv wird übersprungen"
fi

echo ""
echo "=== Zusammenfassung ==="
echo "Deploy-Verzeichnis: $DEPLOY_DIR"
cd "$(dirname $DEPLOY_DIR)"
ls -lh "${DEPLOY_NAME}.zip" 2>/dev/null || true
echo ""
echo "Anzahl Bibliotheken: $(ls -1 $DEPLOY_DIR/lib 2>/dev/null | wc -l)"
echo "Gesamtgröße: $(du -sh $DEPLOY_DIR | cut -f1)"
echo ""
echo "=== Fertig! ==="
echo ""
echo "Um das Binary-Paket zu testen:"
echo "  cd $DEPLOY_DIR"
echo "  ./pokerth"
echo ""
echo "Um die Archive zu verteilen:"
[ -f "${DEPLOY_NAME}.zip" ] && echo "  ${DEPLOY_NAME}.zip"
