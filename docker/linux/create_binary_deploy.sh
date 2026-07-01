#!/bin/bash
set -e

# Binary Deploy Script für PokerTH Linux

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
DEPLOY_DIR="${SCRIPT_DIR}/pokerth-linux-binary"
DEPLOY_NAME="pokerth-linux-$(uname -m)-$(date +%Y%m%d_%H%M%S)"
DEPLOY_PARENT_DIR="$(dirname "$DEPLOY_DIR")"

echo "=== PokerTH Binary Deploy Erstellung ==="
echo "Deploy: $DEPLOY_DIR"
echo ""

echo "=== Bereinigung (vorab) ==="
if [ -d "$DEPLOY_DIR" ]; then
    rm -rf "$DEPLOY_DIR"
    echo "Entfernt: $DEPLOY_DIR"
else
    echo "Kein altes Deploy-Verzeichnis gefunden"
fi

LAST_ZIP=$(find "$DEPLOY_PARENT_DIR" -maxdepth 1 -type f -name "pokerth-linux-*.zip" -printf "%T@ %p\n" 2>/dev/null | sort -nr | head -1 | cut -d' ' -f2-)
if [ -n "$LAST_ZIP" ] && [ -f "$LAST_ZIP" ]; then
    rm -f "$LAST_ZIP"
    echo "Entfernt: $LAST_ZIP"
else
    echo "Keine alte ZIP-Datei gefunden"
fi
echo ""

if [ ! -f "$BUILD_DIR/bin/pokerth_client" ]; then
    echo "ERROR: pokerth_client Binary nicht gefunden in $BUILD_DIR/bin/"
    exit 1
fi

mkdir -p "$DEPLOY_DIR"/{bin,lib,data,share,plugins}

echo "=== Kopiere Binaries ==="
cp -v "$BUILD_DIR/bin/pokerth_client" "$DEPLOY_DIR/bin/"
[ -f "$BUILD_DIR/bin/pokerth_qml-client" ] && cp -v "$BUILD_DIR/bin/pokerth_qml-client" "$DEPLOY_DIR/bin/"
[ -d "$BUILD_DIR/bin/botfiles" ] && cp -r "$BUILD_DIR/bin/botfiles" "$DEPLOY_DIR/bin/"

# System-Libs die nicht mitgeliefert werden (regex auf basename)
# PulseAudio/ALSA werden ausgeschlossen: müssen zum System-Audiodaemon passen.
# Fontconfig/Expat bleiben auf dem Host, damit die dortige Font-Config-Syntax
# (z. B. neuere guessfamily-Regeln auf SteamOS/CachyOS) sicher verstanden wird.
SKIP_PATTERN='^(libc[.-]|libm[.-]|libdl[.-]|libpthread[.-]|librt[.-]|libresolv[.-]|libutil[.-]|libnsl[.-]|ld-linux|ld-[0-9]|libpulse[.-]|libpulse-simple[.-]|libpulsecommon-|libasound[.-]|libfontconfig[.-]|libexpat[.-])'

# ldd löst bereits ALLE transitiven Abhängigkeiten auf – keine Rekursion nötig.
# Nimmt Dateiliste per stdin (via pipe aus find), verarbeitet alles in einem ldd-Aufruf.
copy_deps() {
    xargs -r ldd 2>/dev/null \
        | awk '/=>/ {print $3}' \
        | grep '^/' \
        | sort -u \
        | while read -r lib; do
            name="$(basename "$lib")"
            if ! [[ "$name" =~ $SKIP_PATTERN ]] && [ ! -f "$DEPLOY_DIR/lib/$name" ]; then
                cp -L "$lib" "$DEPLOY_DIR/lib/$name" && chmod +x "$DEPLOY_DIR/lib/$name" && echo "  + $name"
            fi
        done
}

echo ""
echo "=== Sammle Abhängigkeiten (Binaries) ==="
find "$DEPLOY_DIR/bin" -maxdepth 1 -type f | copy_deps

echo ""
echo "=== Sammle Qt-Plugins ==="
# QT6_ROOT erlaubt ein nicht-System-Qt (z. B. aqtinstall unter /opt/Qt/<ver>/gcc_64),
# nötig wenn das System-Qt zu alt ist (Ubuntu 24.04 = 6.4 < benötigte 6.7).
if [ -n "$QT6_ROOT" ] && [ -d "$QT6_ROOT/plugins" ]; then
    QT6_PLUGINS="$QT6_ROOT/plugins"
else
    QT6_PLUGINS=$(find /usr/lib* -type d -name "plugins" -path "*/qt6/*" 2>/dev/null | head -1)
    [ -z "$QT6_PLUGINS" ] && QT6_PLUGINS="/usr/lib/x86_64-linux-gnu/qt6/plugins"
fi

if [ -d "$QT6_PLUGINS" ]; then
    echo "Qt6 Plugins: $QT6_PLUGINS"
    for cat in platforms xcbglintegrations platforminputcontexts imageformats iconengines platformthemes multimedia sqldrivers tls wayland-shell-integration wayland-decoration-client wayland-graphics-integration-client; do
        if [ -d "$QT6_PLUGINS/$cat" ]; then
            mkdir -p "$DEPLOY_DIR/plugins/$cat"
            cp "$QT6_PLUGINS/$cat"/*.so "$DEPLOY_DIR/plugins/$cat/" 2>/dev/null && \
                chmod +x "$DEPLOY_DIR/plugins/$cat"/*.so 2>/dev/null && \
                echo "  $cat" || true
        fi
    done
    # Alle Plugin-Abhängigkeiten in einem einzigen ldd-Aufruf
    find "$DEPLOY_DIR/plugins" -name "*.so" | copy_deps
else
    echo "WARNUNG: Qt6 Plugins nicht gefunden in $QT6_PLUGINS"
fi

echo ""
echo "=== Sammle Qt-QML-Module ==="
if [ -n "$QT6_ROOT" ] && [ -d "$QT6_ROOT/qml" ]; then
    QT6_QML="$QT6_ROOT/qml"
else
    QT6_QML=$(find /usr/lib* -type d -name "qml" -path "*/qt6/*" 2>/dev/null | head -1)
    [ -z "$QT6_QML" ] && QT6_QML="/usr/lib/x86_64-linux-gnu/qt6/qml"
fi

if [ -d "$QT6_QML" ]; then
    echo "Qt6 QML: $QT6_QML"
    mkdir -p "$DEPLOY_DIR/qml"
    for mod in QtCore QtQuick QtQml Qt5Compat QtMultimedia; do
        if [ -d "$QT6_QML/$mod" ]; then
            cp -r "$QT6_QML/$mod" "$DEPLOY_DIR/qml/" && echo "  $mod"
        fi
    done
    # Alle QML-Plugin-Abhängigkeiten in einem einzigen ldd-Aufruf
    find "$DEPLOY_DIR/qml" -name "*.so" | copy_deps
else
    echo "WARNUNG: Qt6 QML-Module nicht gefunden in $QT6_QML"
fi

echo ""
echo "=== Kopiere Daten und Ressourcen ==="
[ -d "$PROJECT_ROOT/data" ]          && cp -r "$PROJECT_ROOT/data/." "$DEPLOY_DIR/data/"
[ -d "$PROJECT_ROOT/docs" ]          && cp -r "$PROJECT_ROOT/docs"   "$DEPLOY_DIR/"
[ -f "$PROJECT_ROOT/COPYING" ]       && cp    "$PROJECT_ROOT/COPYING"          "$DEPLOY_DIR/"
[ -f "$PROJECT_ROOT/ChangeLog" ]     && cp    "$PROJECT_ROOT/ChangeLog"        "$DEPLOY_DIR/"
[ -f "$PROJECT_ROOT/pokerth.desktop" ]     && cp "$PROJECT_ROOT/pokerth.desktop"     "$DEPLOY_DIR/share/"
[ -f "$PROJECT_ROOT/pokerth_qml.desktop" ] && cp "$PROJECT_ROOT/pokerth_qml.desktop" "$DEPLOY_DIR/share/"
[ -f "$PROJECT_ROOT/pokerth.lua" ]         && cp "$PROJECT_ROOT/pokerth.lua"         "$DEPLOY_DIR/share/"

# Share-Symlink für PokerTH's Datei-Such-Logik (bin/../share/pokerth/data/)
mkdir -p "$DEPLOY_DIR/share/pokerth"
ln -sf "../../data" "$DEPLOY_DIR/share/pokerth/data"

echo ""
echo "=== Erstelle Konfiguration und Launcher ==="

# qt.conf: Qt findet Plugins und Libs relativ zum Binary
cat > "$DEPLOY_DIR/bin/qt.conf" << 'EOF'
[Paths]
Plugins = ../plugins
Libraries = ../lib
QmlImports = ../qml
EOF

cat > "$DEPLOY_DIR/pokerth" << 'EOF'
#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Originale LD_LIBRARY_PATH sichern BEVOR wir sie modifizieren, damit
# AppImageUtils::cleanProcessEnvironment() sie für externe Prozesse
# (xdg-open, paplay, …) wiederherstellen kann. Wir bündeln eigene Qt-Libs,
# daher greift runningWithBundledLibs() auch im Tarball-Deploy.
export POKERTH_ORIG_LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-}"
export LD_LIBRARY_PATH="$SCRIPT_DIR/lib:$LD_LIBRARY_PATH"
export QT_PLUGIN_PATH="$SCRIPT_DIR/plugins"
export QT_QPA_PLATFORM_PLUGIN_PATH="$SCRIPT_DIR/plugins/platforms"
export QT_MEDIA_BACKEND=ffmpeg

if [[ "$1" == "--debug-audio" ]]; then
    shift
    export QT_DEBUG_PLUGINS=1
    export QT_LOGGING_RULES="pokerth.audio.info=true;qt.multimedia.*=true"
    echo "[DEBUG] LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
    echo "[DEBUG] QT_PLUGIN_PATH=$QT_PLUGIN_PATH"
    echo "[DEBUG] Multimedia plugins:"; ls -la "$SCRIPT_DIR/plugins/multimedia/" 2>/dev/null || echo "  (keine gefunden!)"
    echo "[DEBUG] PulseAudio libs:";    ls "$SCRIPT_DIR/lib/" | grep -i pulse || echo "  (keine gefunden!)"
fi

cd "$SCRIPT_DIR"
exec "$SCRIPT_DIR/bin/pokerth_client" "$@"
EOF
chmod +x "$DEPLOY_DIR/pokerth"

if [ -f "$DEPLOY_DIR/bin/pokerth_qml-client" ]; then
    cat > "$DEPLOY_DIR/pokerth-qml" << 'EOF'
#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
unset QT_PLUGIN_PATH
unset QT_QPA_PLATFORM_PLUGIN_PATH
unset QML_IMPORT_PATH
unset QML2_IMPORT_PATH
# Originale LD_LIBRARY_PATH sichern (siehe cleanProcessEnvironment()).
export POKERTH_ORIG_LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-}"
export LD_LIBRARY_PATH="$SCRIPT_DIR/lib:$LD_LIBRARY_PATH"
export QT_PLUGIN_PATH="$SCRIPT_DIR/plugins"
export QT_QPA_PLATFORM_PLUGIN_PATH="$SCRIPT_DIR/plugins/platforms"
export QML2_IMPORT_PATH="$SCRIPT_DIR/qml"
export QML_DISABLE_DISK_CACHE=1
export QT_MEDIA_BACKEND=ffmpeg

if [[ "$1" == "--debug-audio" ]]; then
    shift
    export QT_LOGGING_RULES="pokerth.audio.info=true;qt.multimedia.*=true"
    echo "[DEBUG] LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
    echo "[DEBUG] Multimedia-Plugins:"; ls -la "$SCRIPT_DIR/plugins/multimedia/" 2>/dev/null || echo "  (keine gefunden!)"
    echo "[DEBUG] Gebündelte Audio-Codec-Libs:"; ls "$SCRIPT_DIR/lib/" | grep -iE "flac|sndfile|vorbis|opus|pulse" || echo "  (keine)"
    echo "[DEBUG] Host-libpulse:"; ldd "$SCRIPT_DIR/bin/pokerth_qml-client" 2>/dev/null | grep -iE "pulse|sndfile|flac" || echo "  (keine)"
    echo "[DEBUG] Tipp: Wenn kein Ton — 'POKERTH_AUDIO_BACKEND=paplay ./pokerth-qml' testen (nutzt reine Host-Libs)."
fi

cd "$SCRIPT_DIR"
exec "$SCRIPT_DIR/bin/pokerth_qml-client" "$@"
EOF
    chmod +x "$DEPLOY_DIR/pokerth-qml"
fi

echo ""
echo "=== Erstelle Archiv ==="
cd "$(dirname "$DEPLOY_DIR")"
if command -v zip &>/dev/null; then
    zip -qr "${DEPLOY_NAME}.zip" "$(basename "$DEPLOY_DIR")"
    echo "ZIP: ${DEPLOY_NAME}.zip ($(du -sh "${DEPLOY_NAME}.zip" | cut -f1))"
else
    echo "WARNUNG: zip nicht gefunden, ZIP-Archiv übersprungen"
fi

echo ""
echo "=== Fertig ==="
echo "Bibliotheken : $(ls -1 "$DEPLOY_DIR/lib" 2>/dev/null | wc -l)"
echo "Gesamtgröße  : $(du -sh "$DEPLOY_DIR" | cut -f1)"
echo ""
echo "Testen: cd $DEPLOY_DIR && ./pokerth"
[ -f "${DEPLOY_NAME}.zip" ] && echo "Archiv: $(dirname "$DEPLOY_DIR")/${DEPLOY_NAME}.zip"
