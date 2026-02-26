#!/bin/bash
set -e

# AppImage Deploy Script für PokerTH Linux
# Erstellt ein AppImage das auf allen glibc-Versionen läuft,
# da glibc + ld-linux mitgebündelt werden.
#
# Löst das GLIBC_2.38-Kompatibilitätsproblem:
#   Das Binary wird auf einem aktuellen Ubuntu gebaut, aber viele Nutzer
#   haben ältere Systeme (z.B. Ubuntu 22.04 mit glibc 2.35).
#   Das AppImage bündelt ALLES inkl. glibc und nutzt den eigenen ld-linux
#   Loader, wodurch die Host-glibc-Version irrelevant wird.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
ARCH="$(uname -m)"
APPDIR="${SCRIPT_DIR}/PokerTH.AppDir"
APPIMAGE_NAME="PokerTH-${ARCH}-$(date +%Y%m%d).AppImage"

echo "=== PokerTH AppImage Erstellung ==="
echo "Project Root: $PROJECT_ROOT"
echo "Build Dir:    $BUILD_DIR"
echo "AppDir:       $APPDIR"
echo "Output:       $APPIMAGE_NAME"
echo ""

# --- Voraussetzungen prüfen ---

if [ ! -d "$BUILD_DIR" ]; then
    echo "ERROR: Build-Verzeichnis nicht gefunden: $BUILD_DIR"
    echo "Bitte führen Sie zuerst den Build-Prozess durch."
    exit 1
fi

if [ ! -f "$BUILD_DIR/bin/pokerth_client" ]; then
    echo "ERROR: pokerth_client Binary nicht gefunden!"
    exit 1
fi

# appimagetool herunterladen falls nicht vorhanden
# WICHTIG: Stabilen Release verwenden statt "continuous"!
# Die continuous-Builds können instabile/inkompatible AppImage-Runtimes enthalten.
APPIMAGETOOL_VERSION="continuous"
APPIMAGETOOL="${SCRIPT_DIR}/appimagetool-${ARCH}.AppImage"
if [ ! -f "$APPIMAGETOOL" ]; then
    echo "=== Lade appimagetool herunter (${APPIMAGETOOL_VERSION}) ==="
    wget -q --show-progress -O "$APPIMAGETOOL" \
        "https://github.com/AppImage/appimagetool/releases/download/${APPIMAGETOOL_VERSION}/appimagetool-${ARCH}.AppImage" \
        || { echo "ERROR: appimagetool Download fehlgeschlagen"; exit 1; }
    chmod +x "$APPIMAGETOOL"
fi

# --- AppDir Struktur aufbauen ---

echo "=== Erstelle AppDir-Struktur ==="
rm -rf "$APPDIR"
mkdir -p "$APPDIR"/usr/{bin,lib,share/pokerth,plugins}

# --- Binaries kopieren ---

echo "=== Kopiere Binaries ==="
cp -v "$BUILD_DIR/bin/pokerth_client" "$APPDIR/usr/bin/"
if [ -f "$BUILD_DIR/bin/pokerth_qml-client" ]; then
    cp -v "$BUILD_DIR/bin/pokerth_qml-client" "$APPDIR/usr/bin/"
fi

# Botfiles kopieren
if [ -d "$BUILD_DIR/bin/botfiles" ]; then
    cp -rv "$BUILD_DIR/bin/botfiles" "$APPDIR/usr/bin/"
fi

# --- Abhängigkeiten sammeln (INKLUSIVE glibc) ---

echo ""
echo "=== Sammle ALLE Abhängigkeiten (inkl. glibc) ==="

collect_all_dependencies() {
    local binary="$1"
    local lib_dir="$2"
    local processed="$lib_dir/.processed_libs"

    [ -f "$processed" ] || touch "$processed"

    echo "Analysiere: $(basename "$binary")"

    _process() {
        local bin="$1"
        local libs=()
        while IFS= read -r line; do
            local lib
            lib=$(echo "$line" | grep "=>" | awk '{print $3}')
            [ -n "$lib" ] && [ -f "$lib" ] && libs+=("$lib")
            # ld-linux (hat kein "=>")
            local ld
            ld=$(echo "$line" | grep -oP '/\S*ld-linux\S+' || true)
            [ -n "$ld" ] && [ -f "$ld" ] && libs+=("$ld")
        done < <(ldd "$bin" 2>/dev/null || true)

        for lib in "${libs[@]}"; do
            [ -f "$lib" ] || continue
            local libname
            libname="$(basename "$lib")"

            # Audio-Libs (libpulse, libasound) werden MIT-gebündelt.
            # libpulse ist eine reine Client-Lib die per Socket mit dem
            # Host-Audio-Server (PulseAudio/PipeWire) kommuniziert.
            # Ohne Bundling fehlt libpulse.so.0 auf Systemen die nur
            # PipeWire ohne pulseaudio-Kompatibilitätspaket installiert haben
            # (z.B. Fedora-Minimalinstallationen).

            grep -qxF "$lib" "$processed" 2>/dev/null && continue
            echo "$lib" >> "$processed"

            if [ ! -f "$lib_dir/$libname" ]; then
                cp -L "$lib" "$lib_dir/" 2>/dev/null && chmod +x "$lib_dir/$libname" \
                    && echo "  + $libname" || true
            fi
            _process "$lib"
        done
    }

    _process "$binary"
}

for binary in "$APPDIR/usr/bin/pokerth_client" "$APPDIR/usr/bin/pokerth_qml-client"; do
    [ -f "$binary" ] && collect_all_dependencies "$binary" "$APPDIR/usr/lib"
done

# Stelle sicher, dass glibc-Kernbibliotheken vorhanden sind
echo ""
echo "=== Sicherstellung glibc-Bundle ==="
for glibc_lib in libc.so.6 libm.so.6 libdl.so.2 libpthread.so.0 librt.so.1 libresolv.so.2 libmvec.so.1; do
    src="/lib/${ARCH}-linux-gnu/${glibc_lib}"
    [ ! -f "$src" ] && src="/lib64/${glibc_lib}"
    [ ! -f "$src" ] && src=$(ldconfig -p | grep "${glibc_lib}" | head -1 | awk '{print $NF}')
    if [ -f "$src" ] && [ ! -f "$APPDIR/usr/lib/${glibc_lib}" ]; then
        cp -L "$src" "$APPDIR/usr/lib/" && echo "  + ${glibc_lib} (glibc)" || true
    fi
done

# ld-linux Loader kopieren (KRITISCH für glibc-Isolation)
LD_LINUX="/lib64/ld-linux-${ARCH//_/-}.so.2"
[ ! -f "$LD_LINUX" ] && LD_LINUX="/lib/${ARCH}-linux-gnu/ld-linux-${ARCH//_/-}.so.2"
[ ! -f "$LD_LINUX" ] && LD_LINUX=$(ldconfig -p | grep "ld-linux" | head -1 | awk '{print $NF}')
if [ -f "$LD_LINUX" ]; then
    cp -L "$LD_LINUX" "$APPDIR/usr/lib/" && echo "  + $(basename "$LD_LINUX") (loader)" || true
else
    echo "WARNUNG: ld-linux Loader nicht gefunden! AppImage wird möglicherweise nicht portabel sein."
fi

rm -f "$APPDIR/usr/lib/.processed_libs"

# --- Qt-Plugins ---

echo ""
echo "=== Sammle Qt-Plugins ==="
QT6_PLUGINS=$(find /usr/lib* -type d -name "qt6" -path "*/plugins" 2>/dev/null | head -1)
[ -z "$QT6_PLUGINS" ] && QT6_PLUGINS="/usr/lib/${ARCH}-linux-gnu/qt6/plugins"

if [ -d "$QT6_PLUGINS" ]; then
    echo "Qt6 Plugins: $QT6_PLUGINS"
    for cat in platforms xcbglintegrations platforminputcontexts imageformats platformthemes multimedia sqldrivers tls; do
        if [ -d "$QT6_PLUGINS/$cat" ]; then
            echo "  Kopiere $cat..."
            mkdir -p "$APPDIR/usr/plugins/$cat"
            cp "$QT6_PLUGINS/$cat"/*.so "$APPDIR/usr/plugins/$cat/" 2>/dev/null || true
            chmod +x "$APPDIR/usr/plugins/$cat"/*.so 2>/dev/null || true
            # Abhängigkeiten der Plugins sammeln
            for plugin in "$APPDIR/usr/plugins/$cat"/*.so; do
                [ -f "$plugin" ] && collect_all_dependencies "$plugin" "$APPDIR/usr/lib"
            done
        fi
    done
    rm -f "$APPDIR/usr/lib/.processed_libs"
else
    echo "WARNUNG: Qt6 Plugins nicht gefunden!"
fi

# --- Data-Verzeichnis ---

echo ""
echo "=== Kopiere Data ==="
# WICHTIG: Pfad-Auflösung in getDataPathStdString() (qthelper.cpp):
#   Qt's applicationDirPath() nutzt /proc/self/exe auf Linux.
#   Da wir den gebündelten ld-linux Loader verwenden
#     (exec ld-linux ... pokerth_client),
#   zeigt /proc/self/exe auf usr/lib/ld-linux-*.so.2, NICHT auf usr/bin/pokerth_client!
#   → applicationDirPath() = usr/lib/
#   → Keiner der Regex-Checks matched ("bin/?$" matched nicht "lib")
#   → Fallback: path += "/data/" → sucht in usr/lib/data/
#
# Lösung: Data in usr/share/pokerth/data/ ablegen UND
#          Symlink usr/lib/data → ../share/pokerth/data erstellen
mkdir -p "$APPDIR/usr/share/pokerth"
if [ -d "$PROJECT_ROOT/data" ]; then
    cp -r "$PROJECT_ROOT/data" "$APPDIR/usr/share/pokerth/"
fi

# Symlink damit data gefunden wird wenn applicationDirPath() auf usr/lib/ zeigt
ln -sf "../share/pokerth/data" "$APPDIR/usr/lib/data"
echo "  Symlink: usr/lib/data → ../share/pokerth/data (für ld-linux /proc/self/exe)"

# Lua-Script
[ -f "$PROJECT_ROOT/pokerth.lua" ] && cp "$PROJECT_ROOT/pokerth.lua" "$APPDIR/usr/share/pokerth/"

# --- qt.conf ---

echo ""
echo "=== Erstelle qt.conf ==="
cat > "$APPDIR/usr/bin/qt.conf" << 'EOF'
[Paths]
Plugins = ../plugins
Libraries = ../lib
EOF

# --- Desktop-Datei + Icon (AppImage-Pflicht) ---

echo "=== Erstelle Desktop-Datei und Icon ==="
cat > "$APPDIR/pokerth.desktop" << 'EOF'
[Desktop Entry]
Name=PokerTH
GenericName=Poker Card Game
GenericName[de]=Pokerspiel
Comment=Texas hold'em game
Comment[de]=Texas Hold'em Spiel
Exec=pokerth_client
Icon=pokerth
Terminal=false
Type=Application
Categories=Qt;Game;CardGame;
EOF

# Icon kopieren (AppImage braucht das Icon im Root-Verzeichnis)
if [ -f "$PROJECT_ROOT/pokerth.png" ]; then
    cp "$PROJECT_ROOT/pokerth.png" "$APPDIR/pokerth.png"
    # Zusätzlich in hicolor-Struktur für Desktop-Integration
    mkdir -p "$APPDIR/usr/share/icons/hicolor/128x128/apps"
    cp "$PROJECT_ROOT/pokerth.png" "$APPDIR/usr/share/icons/hicolor/128x128/apps/pokerth.png"
else
    echo "WARNUNG: pokerth.png nicht gefunden, erstelle Platzhalter"
    # Minimales 1x1 PNG als Fallback (AppImage braucht ein Icon)
    printf '\x89PNG\r\n\x1a\n' > "$APPDIR/pokerth.png"
fi

# --- Lizenz & Docs ---

[ -f "$PROJECT_ROOT/COPYING" ]   && cp "$PROJECT_ROOT/COPYING"   "$APPDIR/"
[ -f "$PROJECT_ROOT/ChangeLog" ] && cp "$PROJECT_ROOT/ChangeLog" "$APPDIR/"
[ -d "$PROJECT_ROOT/docs" ]      && { mkdir -p "$APPDIR/usr/share/doc/pokerth"; cp -r "$PROJECT_ROOT/docs"/* "$APPDIR/usr/share/doc/pokerth/"; }

# --- AppRun erstellen (KERNSTÜCK für glibc-Isolation) ---

echo ""
echo "=== Erstelle AppRun ==="

# Ermittle den genauen Dateinamen des ld-linux Loaders
LD_LINUX_NAME=$(basename "$LD_LINUX" 2>/dev/null || echo "ld-linux-x86-64.so.2")

cat > "$APPDIR/AppRun" << 'RUNEOF'
#!/bin/bash
# AppRun: Startet PokerTH mit gebündeltem glibc + ld-linux Loader.
# Dadurch ist die glibc-Version des Host-Systems irrelevant.

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# --- AppImageLauncher-Erkennung ---
# AppImageLauncher ist bekannt dafür, AppImage-Starts zu stören.
# Symptome: "fuse: memory allocation failed", "Bad address", FUSE-Fehler.
# Wenn AppImageLauncher erkannt wird, warnen wir den User.
if [ -n "${APPIMAGE_LAUNCHER_VERSION:-}" ] || \
   [ -f /usr/lib/x86_64-linux-gnu/libappimage_launcher.so ] || \
   dpkg -l appimagelauncher &>/dev/null 2>&1; then
    echo "" >&2
    echo "=== WARNUNG: AppImageLauncher erkannt! ===" >&2
    echo "AppImageLauncher kann FUSE-Fehler verursachen." >&2
    echo "Loesung: AppImageLauncher deinstallieren:" >&2
    echo "  sudo apt remove appimagelauncher" >&2
    echo "Oder PokerTH direkt starten mit:" >&2
    echo "  APPIMAGE_EXTRACT_AND_RUN=1 ${APPIMAGE:-$0}" >&2
    echo "=============================================" >&2
    echo "" >&2
fi

# PokerTH AppImage Marker — wird im C++ Code via AppImageUtils geprüft
export POKERTH_APPIMAGE=1

# Originale LD_LIBRARY_PATH sichern BEVOR wir sie modifizieren.
# AppImageUtils::cleanProcessEnvironment() stellt diesen Wert wieder her,
# damit externe Prozesse (xdg-open, paplay, etc.) die System-Libs nutzen.
export POKERTH_ORIG_LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-}"

# Bibliotheks- und Plugin-Pfade
export LD_LIBRARY_PATH="${HERE}/usr/lib:${LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="${HERE}/usr/plugins"
export QT_QPA_PLATFORM_PLUGIN_PATH="${HERE}/usr/plugins/platforms"
export QT_MEDIA_BACKEND=ffmpeg
export XDG_DATA_DIRS="${HERE}/usr/share:${XDG_DATA_DIRS:-/usr/local/share:/usr/share}"

# Wechsel in das AppDir damit bin/../share/pokerth/data/ aufgelöst wird
cd "${HERE}/usr"

# Prüfe ob der gebündelte ld-linux Loader vorhanden ist
RUNEOF

# ld-linux Name in das Script einsetzen (muss außerhalb von 'HEREDOC' sein)
cat >> "$APPDIR/AppRun" << RUNEOF
BUNDLED_LD="\${HERE}/usr/lib/${LD_LINUX_NAME}"
RUNEOF

cat >> "$APPDIR/AppRun" << 'RUNEOF'

if [ -x "${BUNDLED_LD}" ]; then
    # WICHTIG: Nutze den gebündelten ld-linux Loader!
    # Das umgeht das System-glibc komplett und nutzt unsere eigene Version.
    exec "${BUNDLED_LD}" --inhibit-cache --library-path "${HERE}/usr/lib" \
         "${HERE}/usr/bin/pokerth_client" "$@"
else
    # Fallback: Normaler Start (funktioniert nur wenn Host-glibc kompatibel ist)
    echo "WARNUNG: Gebündelter Loader nicht gefunden, verwende System-Loader" >&2
    exec "${HERE}/usr/bin/pokerth_client" "$@"
fi
RUNEOF
chmod +x "$APPDIR/AppRun"

# --- AppImage erstellen ---

echo ""
echo "=== Erstelle AppImage ==="
cd "$SCRIPT_DIR"

# appimagetool im --appimage-extract-and-run Modus für Container ohne FUSE
export ARCH
"$APPIMAGETOOL" --appimage-extract-and-run "$APPDIR" "$APPIMAGE_NAME" \
    || { echo "Versuche appimagetool mit --no-appstream..."; \
         "$APPIMAGETOOL" --appimage-extract-and-run --no-appstream "$APPDIR" "$APPIMAGE_NAME"; }

echo ""
echo "=== Zusammenfassung ==="
echo "AppImage erstellt:    ${SCRIPT_DIR}/${APPIMAGE_NAME}"
ls -lh "${SCRIPT_DIR}/${APPIMAGE_NAME}" 2>/dev/null || true
echo ""
echo "Anzahl Bibliotheken:  $(find "$APPDIR/usr/lib" -name '*.so*' | wc -l)"
echo "Gesamtgröße AppDir:   $(du -sh "$APPDIR" | cut -f1)"
echo ""

# Prüfe ob glibc gebündelt ist
if [ -f "$APPDIR/usr/lib/libc.so.6" ] && [ -f "$APPDIR/usr/lib/${LD_LINUX_NAME}" ]; then
    echo "✓ glibc + ld-linux gebündelt — sollte auf älteren Systemen funktionieren!"
else
    echo "⚠ glibc oder ld-linux fehlt — AppImage ist möglicherweise nicht voll portabel."
fi

echo ""
echo "=== Fertig! ==="
echo ""
echo "Test:"
echo "  chmod +x ${APPIMAGE_NAME}"
echo "  ./${APPIMAGE_NAME}"
echo ""
echo "Oder ohne FUSE (z.B. in Docker/WSL):"
echo "  ./${APPIMAGE_NAME} --appimage-extract-and-run"
echo ""
echo "=== Troubleshooting ==="
echo ""
echo "Problem: 'fuse: memory allocation failed' / 'Bad address' / FUSE-Fehler:"
echo "  1. AppImageLauncher deinstallieren (häufigste Ursache!):"
echo "     sudo apt remove appimagelauncher"
echo "  2. libfuse2 installieren (Ubuntu 22.04+):"
echo "     sudo apt install libfuse2"
echo "  3. Falls beides nicht hilft, --appimage-extract-and-run verwenden:"
echo "     ./${APPIMAGE_NAME} --appimage-extract-and-run"
echo "  4. Alternativ: APPIMAGE_EXTRACT_AND_RUN=1 ./${APPIMAGE_NAME}"
