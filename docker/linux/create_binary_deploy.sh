#!/bin/bash
set -e

# Binary deploy script for PokerTH Linux

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

# System libs that are not shipped along (regex on the basename)
# PulseAudio/ALSA are excluded: they have to match the system audio daemon.
# Fontconfig/expat stay on the host, so that the font config syntax there
# (e.g. newer guessfamily rules on SteamOS/CachyOS) is reliably understood.
# GL/EGL/GBM/DRM (the Mesa stack) MUST come from the host: libGL/libEGL load the
# GPU specific DRI driver (e.g. radeonsi_dri.so) of the host and need
# matching libglapi/libdrm for it. A bundled Mesa stack shadows the
# host driver -> "EGL not available" / "Failed to create context" / an RHI abort.
# (This corresponds to the linuxdeployqt exclude list for graphics.)
# libwayland-* likewise: they talk directly to the host compositor and the
# host libEGL_mesa. In a Wayland session GL runs ONLY via EGL; a
# bundled libwayland-client/-egl breaks the EGL Wayland handshake -> exactly
# the same "EGL not available". So leave them on the host side.
SKIP_PATTERN='^(libc[.-]|libm[.-]|libdl[.-]|libpthread[.-]|librt[.-]|libresolv[.-]|libutil[.-]|libnsl[.-]|ld-linux|ld-[0-9]|libpulse[.-]|libpulse-simple[.-]|libpulsecommon-|libasound[.-]|libfontconfig[.-]|libexpat[.-]|libGL[.-]|libGLX[.-]|libGLdispatch[.-]|libGLESv2[.-]|libEGL[.-]|libOpenGL[.-]|libglapi[.-]|libgbm[.-]|libdrm[.-]|libwayland-client[.-]|libwayland-cursor[.-]|libwayland-egl[.-]|libwayland-server[.-])'

# ldd already resolves ALL transitive dependencies – no recursion needed.
# Takes the file list via stdin (piped from find), processes everything in one ldd call.
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

# With a non-system Qt (aqtinstall) the Qt libs are NOT in a standard
# search path. Without this export the ldd in copy_deps does not find the Qt libs of the
# plugins/QML modules ("not found") and silently leaves them out -> the deploy
# does not start. With the distro Qt (in /usr/lib) this did not occur.
if [ -n "$QT6_ROOT" ] && [ -d "$QT6_ROOT/lib" ]; then
    export LD_LIBRARY_PATH="$QT6_ROOT/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi

echo ""
echo "=== Sammle Abhängigkeiten (Binaries) ==="
find "$DEPLOY_DIR/bin" -maxdepth 1 -type f | copy_deps

echo ""
echo "=== Sammle Qt-Plugins ==="
# QT6_ROOT allows a non-system Qt (e.g. aqtinstall under /opt/Qt/<ver>/gcc_64),
# needed when the system Qt is too old (Ubuntu 24.04 = 6.4 < the required 6.7).
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
                # SQLite only – the client uses no MySQL/PostgreSQL/ODBC/Mimer.
                # Their driver plugins would need libmysqlclient/libpq/… which are neither
                # bundled nor necessarily present on the host.
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
    # All plugin dependencies in a single ldd call
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
    # All QML plugin dependencies in a single ldd call
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
# The static pokerth*.desktop files in the repo root are SYSTEM install
# variants (system Qt via /usr/lib, the binary from the PATH; used by .deb/snap).
# For the relocatable ZIP bundle they are wrong (absolute system paths, no
# $SCRIPT_DIR, no QML_DISABLE_DISK_CACHE) and would bypass the wrapper.
# The correct desktop integration is written by the launchers ./pokerth and
# ./pokerth-qml themselves at runtime (integrate_desktop_entry → ~/.local/share
# with Exec=$SCRIPT_DIR/... and the bundled libs). So do NOT ship them here.
[ -f "$PROJECT_ROOT/pokerth.lua" ]         && cp "$PROJECT_ROOT/pokerth.lua"         "$DEPLOY_DIR/share/"
# A scalable app icon for the deploy: with it the launchers register a
# .desktop file in ~/.local/share, so that the Wayland compositor (KWin) can
# resolve the window/taskbar icon via the app_id (Qt 6.8 cannot set it
# there via setWindowIcon).
[ -f "$PROJECT_ROOT/pokerth.svg" ]         && cp "$PROJECT_ROOT/pokerth.svg"         "$DEPLOY_DIR/share/"

# A share symlink for PokerTH's file lookup logic (bin/../share/pokerth/data/)
mkdir -p "$DEPLOY_DIR/share/pokerth"
ln -sf "../../data" "$DEPLOY_DIR/share/pokerth/data"

echo ""
echo "=== Erstelle Konfiguration und Launcher ==="

# qt.conf: Qt finds plugins and libs relative to the binary
cat > "$DEPLOY_DIR/bin/qt.conf" << 'EOF'
[Paths]
Plugins = ../plugins
Libraries = ../lib
QmlImports = ../qml
EOF

cat > "$DEPLOY_DIR/pokerth" << 'EOF'
#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Save the original LD_LIBRARY_PATH BEFORE we modify it, so that
# AppImageUtils::cleanProcessEnvironment() can restore it for external processes
# (xdg-open, paplay, …). We bundle Qt libs of our own,
# so runningWithBundledLibs() applies in the tarball deploy as well.
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

# --- Desktop integration (idempotent) -------------------------------------
# Wayland compositors (e.g. KWin) do NOT read the window/taskbar icon from
# setWindowIcon (Qt 6.8 cannot set it there) but from the .desktop file
# matching the app_id. For the portable deploy we register it here
# in ~/.local/share with absolute exec/icon paths. The app_id "pokerth" is set
# in the code via setDesktopFileName() and matches the basename below.
integrate_desktop_entry() {
    apps_dir="${XDG_DATA_HOME:-$HOME/.local/share}/applications"
    desktop_file="$apps_dir/pokerth.desktop"
    icon="$SCRIPT_DIR/share/pokerth.svg"
    [ -f "$icon" ] || icon="$SCRIPT_DIR/data/gfx/gui/misc/windowicon.png"
    # Build the target content and rewrite it on every deviation. Comparing only
    # the Exec line would freeze changed name/icon fields in already integrated
    # entries forever.
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

# --- Desktop integration (idempotent) -------------------------------------
# Wayland compositors (e.g. KWin on the Steam Deck) do NOT read the window/
# taskbar icon from setWindowIcon (Qt 6.8 cannot set it there)
# but from the .desktop file matching the app_id. For the portable deploy
# we register it here in ~/.local/share with absolute exec/icon paths.
# The app_id "pokerth_qml" is set in the code via setDesktopFileName() and matches
# the basename below.
integrate_desktop_entry() {
    apps_dir="${XDG_DATA_HOME:-$HOME/.local/share}/applications"
    desktop_file="$apps_dir/pokerth_qml.desktop"
    icon="$SCRIPT_DIR/share/pokerth.svg"
    [ -f "$icon" ] || icon="$SCRIPT_DIR/data/gfx/gui/misc/windowicon.png"
    # Build the target content and rewrite it on every deviation. Comparing only
    # the Exec line would freeze changed name/icon fields in already integrated
    # entries forever.
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
# copy_deps only collects what ldd resolves. If a lib is missing in the BUILD container
# (e.g. libxcb-cursor0), ldd reports "not found" and the lib is SILENTLY
# skipped -> the deploy does not start on the target system (the platform plugin
# does not load). Here we check against the deploy's own lib/ + the build host whether
# anything is still unresolved, and abort hard for essential components.
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
    # Only the X11 platform plugin is mandatory: without libqxcb.so the app does not
    # start on an X11 desktop. Everything else is optional -> only abort hard here.
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
