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
# GL/EGL/GBM/DRM (Mesa-Stack) MÜSSEN vom Host kommen: libGL/libEGL laden den
# GPU-spezifischen DRI-Treiber (z. B. radeonsi_dri.so) des Hosts und brauchen
# dazu passende libglapi/libdrm. Ein gebündelter Mesa-Stack überschattet den
# Host-Treiber -> "EGL not available" / "Failed to create context" / RHI-Abbruch.
# (Entspricht der linuxdeployqt-Excludelist für Grafik.)
# libwayland-* ebenso: sie sprechen direkt mit dem Host-Compositor und der
# Host-libEGL_mesa. Auf einer Wayland-Sitzung läuft GL NUR über EGL; eine
# gebündelte libwayland-client/-egl bricht den EGL-Wayland-Handshake -> exakt
# dasselbe "EGL not available". Daher Host-seitig lassen.
SKIP_PATTERN='^(libc[.-]|libm[.-]|libdl[.-]|libpthread[.-]|librt[.-]|libresolv[.-]|libutil[.-]|libnsl[.-]|ld-linux|ld-[0-9]|libpulse[.-]|libpulse-simple[.-]|libpulsecommon-|libasound[.-]|libfontconfig[.-]|libexpat[.-]|libGL[.-]|libGLX[.-]|libGLdispatch[.-]|libGLESv2[.-]|libEGL[.-]|libOpenGL[.-]|libglapi[.-]|libgbm[.-]|libdrm[.-]|libwayland-client[.-]|libwayland-cursor[.-]|libwayland-egl[.-]|libwayland-server[.-])'

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

# Bei nicht-System-Qt (aqtinstall) liegen die Qt-Libs NICHT in einem Standard-
# Suchpfad. Ohne diesen Export findet das ldd in copy_deps die Qt-Libs der
# Plugins/QML-Module nicht ("not found") und lässt sie still weg -> das Deploy
# startet nicht. Mit dem Distro-Qt (in /usr/lib) trat das nicht auf.
if [ -n "$QT6_ROOT" ] && [ -d "$QT6_ROOT/lib" ]; then
    export LD_LIBRARY_PATH="$QT6_ROOT/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi

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
            if [ "$cat" = "sqldrivers" ]; then
                # Nur SQLite – die Client nutzt kein MySQL/PostgreSQL/ODBC/Mimer.
                # Deren Treiber-Plugins bräuchten libmysqlclient/libpq/… die weder
                # gebündelt noch zwingend auf dem Host sind.
                cp "$QT6_PLUGINS/$cat"/libqsqlite.so "$DEPLOY_DIR/plugins/$cat/" 2>/dev/null && \
                    chmod +x "$DEPLOY_DIR/plugins/$cat"/*.so 2>/dev/null && \
                    echo "  $cat (nur sqlite)" || true
            else
                cp "$QT6_PLUGINS/$cat"/*.so "$DEPLOY_DIR/plugins/$cat/" 2>/dev/null && \
                    chmod +x "$DEPLOY_DIR/plugins/$cat"/*.so 2>/dev/null && \
                    echo "  $cat" || true
            fi
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
# Skalierbares App-Icon fürs Deploy: die Launcher registrieren damit eine
# .desktop-Datei in ~/.local/share, damit der Wayland-Compositor (KWin) das
# Fenster-/Taskbar-Icon über die app_id auflösen kann (Qt 6.8 kann es dort
# nicht via setWindowIcon setzen).
[ -f "$PROJECT_ROOT/pokerth.svg" ]         && cp "$PROJECT_ROOT/pokerth.svg"         "$DEPLOY_DIR/share/"

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

# --- Desktop-Integration (idempotent) -------------------------------------
# Wayland-Compositoren (z. B. KWin) lesen das Fenster-/Taskbar-Icon NICHT aus
# setWindowIcon (Qt 6.8 kann es dort nicht setzen), sondern aus der zur app_id
# passenden .desktop-Datei. Für das portable Deploy registrieren wir sie hier
# in ~/.local/share mit absoluten Exec-/Icon-Pfaden. app_id "pokerth" wird im
# Code via setDesktopFileName() gesetzt und matcht den Basenamen unten.
integrate_desktop_entry() {
    apps_dir="${XDG_DATA_HOME:-$HOME/.local/share}/applications"
    desktop_file="$apps_dir/pokerth.desktop"
    icon="$SCRIPT_DIR/share/pokerth.svg"
    [ -f "$icon" ] || icon="$SCRIPT_DIR/data/gfx/gui/misc/windowicon.png"
    # Soll-Inhalt bauen und bei jeder Abweichung neu schreiben. Ein Vergleich nur
    # der Exec-Zeile würde geänderte Name-/Icon-Felder in bereits integrierten
    # Einträgen für immer einfrieren.
    desired=$(cat <<DESKTOP
[Desktop Entry]
Type=Application
Name=PokerTH Widget
GenericName=Poker Card Game
Comment=Texas hold'em game
Exec=$SCRIPT_DIR/pokerth
Icon=$icon
Terminal=false
StartupWMClass=pokerth_client
Categories=Qt;Game;CardGame;
DESKTOP
)
    if [ ! -f "$desktop_file" ] || [ "$(cat "$desktop_file")" != "$desired" ]; then
        mkdir -p "$apps_dir"
        printf '%s\n' "$desired" > "$desktop_file"
        command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$apps_dir" >/dev/null 2>&1 || true
    fi
}
integrate_desktop_entry

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

# --- Desktop-Integration (idempotent) -------------------------------------
# Wayland-Compositoren (z. B. KWin auf dem Steam Deck) lesen das Fenster-/
# Taskbar-Icon NICHT aus setWindowIcon (Qt 6.8 kann es dort nicht setzen),
# sondern aus der zur app_id passenden .desktop-Datei. Für das portable Deploy
# registrieren wir sie hier in ~/.local/share mit absoluten Exec-/Icon-Pfaden.
# app_id "pokerth_qml" wird im Code via setDesktopFileName() gesetzt und matcht
# den Basenamen unten.
integrate_desktop_entry() {
    apps_dir="${XDG_DATA_HOME:-$HOME/.local/share}/applications"
    desktop_file="$apps_dir/pokerth_qml.desktop"
    icon="$SCRIPT_DIR/share/pokerth.svg"
    [ -f "$icon" ] || icon="$SCRIPT_DIR/data/gfx/gui/misc/windowicon.png"
    # Soll-Inhalt bauen und bei jeder Abweichung neu schreiben. Ein Vergleich nur
    # der Exec-Zeile würde geänderte Name-/Icon-Felder in bereits integrierten
    # Einträgen für immer einfrieren.
    desired=$(cat <<DESKTOP
[Desktop Entry]
Type=Application
Name=PokerTH QML
GenericName=Poker Card Game
Comment=Texas hold'em game
Exec=$SCRIPT_DIR/pokerth-qml
Icon=$icon
Terminal=false
StartupWMClass=pokerth_qml-client
Categories=Qt;Game;CardGame;
DESKTOP
)
    if [ ! -f "$desktop_file" ] || [ "$(cat "$desktop_file")" != "$desired" ]; then
        mkdir -p "$apps_dir"
        printf '%s\n' "$desired" > "$desktop_file"
        command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$apps_dir" >/dev/null 2>&1 || true
    fi
}
integrate_desktop_entry

cd "$SCRIPT_DIR"
exec "$SCRIPT_DIR/bin/pokerth_qml-client" "$@"
EOF
    chmod +x "$DEPLOY_DIR/pokerth-qml"
fi

echo ""
echo "=== Verifiziere Abhängigkeiten ==="
# copy_deps sammelt nur, was ldd auflöst. Fehlt eine Lib im BUILD-Container
# (z. B. libxcb-cursor0), meldet ldd "not found" und die Lib wird STILL
# übersprungen -> das Deploy startet auf dem Zielsystem nicht (Platform-Plugin
# lädt nicht). Hier prüfen wir mit dem Deploy-eigenen lib/ + Build-Host, ob
# noch etwas ungelöst ist, und brechen bei essentiellen Komponenten hart ab.
UNRESOLVED=$( { find "$DEPLOY_DIR/bin" -maxdepth 1 -type f;
                find "$DEPLOY_DIR/plugins" "$DEPLOY_DIR/lib" "$DEPLOY_DIR/qml" -name "*.so*" 2>/dev/null; } \
    | while read -r f; do
        LD_LIBRARY_PATH="$DEPLOY_DIR/lib" ldd "$f" 2>/dev/null \
            | awk -v F="$f" '/not found/ {print F"\t"$1}'
      done | sort -u )

if [ -n "$UNRESOLVED" ]; then
    echo "Hinweis: Ungelöste Abhängigkeiten im Build-Container:"
    echo "$UNRESOLVED" | sed 's/^/  /'
    echo "  (Optionale/host-seitige Plugins wie libqgtk3 [GTK-Theme], eglfs oder"
    echo "   der ffmpeg-Media-Plugin sind unkritisch – deren Libs liefert der"
    echo "   Desktop-Host bzw. sie werden nicht benötigt.)"
    # Nur das X11-Platform-Plugin ist zwingend: ohne libqxcb.so startet die App
    # auf einem X11-Desktop nicht. Alles andere ist optional -> nur hier hart abbrechen.
    if echo "$UNRESOLVED" | grep -q "/plugins/platforms/libqxcb.so"; then
        echo "FEHLER: X11-Platform-Plugin (libqxcb.so) hat ungelöste Abhängigkeiten -> Abbruch."
        echo "        Fehlende Libs im Build-Container installieren und neu bauen."
        exit 1
    fi
else
    echo "OK: keine ungelösten Abhängigkeiten."
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
