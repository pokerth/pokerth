#!/bin/bash
set -e

# AppImage deploy script for PokerTH Linux
# Creates an AppImage that runs on all glibc versions,
# because glibc + ld-linux are bundled along.
#
# It solves the GLIBC_2.38 compatibility problem:
#   The binary is built on a current Ubuntu, but many users
#   have older systems (e.g. Ubuntu 22.04 with glibc 2.35).
#   The AppImage bundles EVERYTHING incl. glibc and uses its own ld-linux
#   loader, which makes the host glibc version irrelevant.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
ARCH="$(uname -m)"
APPDIR="${SCRIPT_DIR}/PokerTH.AppDir"
# The name can be overridden via an environment variable (e.g. by the Docker build with
# the glibc version in the name), otherwise the default with arch + timestamp.
APPIMAGE_NAME="${APPIMAGE_NAME:-PokerTH-${ARCH}-$(date +%Y%m%d_%H%M%S).AppImage}"

echo "=== PokerTH AppImage Erstellung ==="
echo "Project Root: $PROJECT_ROOT"
echo "Build Dir:    $BUILD_DIR"
echo "AppDir:       $APPDIR"
echo "Output:       $APPIMAGE_NAME"
echo ""

# --- Check the requirements ---

if [ ! -d "$BUILD_DIR" ]; then
    echo "ERROR: Build-Verzeichnis nicht gefunden: $BUILD_DIR"
    echo "Bitte führen Sie zuerst den Build-Prozess durch."
    exit 1
fi

if [ ! -f "$BUILD_DIR/bin/pokerth_client" ]; then
    echo "ERROR: pokerth_client Binary nicht gefunden!"
    exit 1
fi

# Download appimagetool if it is not present
# IMPORTANT: use a stable release instead of "continuous"!
# The continuous builds can contain unstable/incompatible AppImage runtimes.
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

# --- Collect the dependencies (INCLUDING glibc) ---

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
            # ld-linux (has no "=>")
            local ld
            ld=$(echo "$line" | grep -oP '/\S*ld-linux\S+' || true)
            [ -n "$ld" ] && [ -f "$ld" ] && libs+=("$ld")
        done < <(ldd "$bin" 2>/dev/null || true)

        for lib in "${libs[@]}"; do
            [ -f "$lib" ] || continue
            local libname
            libname="$(basename "$lib")"

            # Audio libs (libpulse, libasound) ARE bundled along.
            # libpulse is a pure client lib that communicates with the
            # host audio server (PulseAudio/PipeWire) via a socket.
            # Without bundling, libpulse.so.0 is missing on systems that have
            # only PipeWire installed without the pulseaudio compatibility package
            # (e.g. minimal Fedora installations).

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

# Make sure the glibc core libraries are present
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

# Copy the ld-linux loader (CRITICAL for the glibc isolation)
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
            # Collect the dependencies of the plugins
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
# IMPORTANT: path resolution in getDataPathStdString() (qthelper.cpp):
#   Qt's applicationDirPath() uses /proc/self/exe on Linux.
#   Since we use the bundled ld-linux loader
#     (exec ld-linux ... pokerth_client),
#   /proc/self/exe points to usr/lib/ld-linux-*.so.2, NOT to usr/bin/pokerth_client!
#   → applicationDirPath() = usr/lib/
#   → None of the regex checks match ("bin/?$" does not match "lib")
#   → Fallback: path += "/data/" → it looks in usr/lib/data/
#
# Solution: put the data in usr/share/pokerth/data/ AND
#          create a symlink usr/lib/data → ../share/pokerth/data
mkdir -p "$APPDIR/usr/share/pokerth"
if [ -d "$PROJECT_ROOT/data" ]; then
    cp -r "$PROJECT_ROOT/data" "$APPDIR/usr/share/pokerth/"
fi

# A symlink so that the data is found when applicationDirPath() points to usr/lib/
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
Name=PokerTH Widget
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

# Copy the icon (the AppImage needs the icon in the root directory)
if [ -f "$PROJECT_ROOT/pokerth.png" ]; then
    cp "$PROJECT_ROOT/pokerth.png" "$APPDIR/pokerth.png"
    # Additionally into the hicolor structure for the desktop integration
    mkdir -p "$APPDIR/usr/share/icons/hicolor/128x128/apps"
    cp "$PROJECT_ROOT/pokerth.png" "$APPDIR/usr/share/icons/hicolor/128x128/apps/pokerth.png"
else
    echo "WARNUNG: pokerth.png nicht gefunden, erstelle Platzhalter"
    # A minimal 1x1 PNG as a fallback (the AppImage needs an icon)
    printf '\x89PNG\r\n\x1a\n' > "$APPDIR/pokerth.png"
fi

# .DirIcon determines the icon of the AppImage FILE in the file manager. As a real file
# (instead of the symlink created by appimagetool) it is reliably visible
# everywhere.
cp "$APPDIR/pokerth.png" "$APPDIR/.DirIcon"

# --- Lizenz & Docs ---

[ -f "$PROJECT_ROOT/COPYING" ]   && cp "$PROJECT_ROOT/COPYING"   "$APPDIR/"
[ -f "$PROJECT_ROOT/ChangeLog" ] && cp "$PROJECT_ROOT/ChangeLog" "$APPDIR/"
[ -d "$PROJECT_ROOT/docs" ]      && { mkdir -p "$APPDIR/usr/share/doc/pokerth"; cp -r "$PROJECT_ROOT/docs"/* "$APPDIR/usr/share/doc/pokerth/"; }

# --- Create the AppRun (the CORE PIECE for the glibc isolation) ---

echo ""
echo "=== Erstelle AppRun ==="

# Determine the exact file name of the ld-linux loader
LD_LINUX_NAME=$(basename "$LD_LINUX" 2>/dev/null || echo "ld-linux-x86-64.so.2")

cat > "$APPDIR/AppRun" << 'RUNEOF'
#!/bin/bash
# AppRun: starts PokerTH with the bundled glibc + ld-linux loader.
# That makes the glibc version of the host system irrelevant.

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# --- AppImageLauncher detection ---
# AppImageLauncher is known for disturbing AppImage starts.
# Symptoms: "fuse: memory allocation failed", "Bad address", FUSE errors.
# If AppImageLauncher is detected, we warn the user.
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

# PokerTH AppImage marker — checked in the C++ code via AppImageUtils
export POKERTH_APPIMAGE=1

# Save the original LD_LIBRARY_PATH BEFORE we modify it.
# AppImageUtils::cleanProcessEnvironment() restores this value,
# so that external processes (xdg-open, paplay, etc.) use the system libs.
export POKERTH_ORIG_LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-}"

# Library and plugin paths
export LD_LIBRARY_PATH="${HERE}/usr/lib:${LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="${HERE}/usr/plugins"
export QT_QPA_PLATFORM_PLUGIN_PATH="${HERE}/usr/plugins/platforms"
export QT_MEDIA_BACKEND=ffmpeg
export XDG_DATA_DIRS="${HERE}/usr/share:${XDG_DATA_DIRS:-/usr/local/share:/usr/share}"

# Change into the AppDir so that bin/../share/pokerth/data/ resolves
cd "${HERE}/usr"

# Check whether the bundled ld-linux loader is present
RUNEOF

# Insert the ld-linux name into the script (has to be outside of 'HEREDOC')
cat >> "$APPDIR/AppRun" << RUNEOF
BUNDLED_LD="\${HERE}/usr/lib/${LD_LINUX_NAME}"
RUNEOF

cat >> "$APPDIR/AppRun" << 'RUNEOF'

if [ -x "${BUNDLED_LD}" ]; then
    # IMPORTANT: use the bundled ld-linux loader!
    # That bypasses the system glibc completely and uses our own version.
    exec "${BUNDLED_LD}" --inhibit-cache --library-path "${HERE}/usr/lib" \
         "${HERE}/usr/bin/pokerth_client" "$@"
else
    # Fallback: a normal start (works only if the host glibc is compatible)
    echo "WARNUNG: Gebündelter Loader nicht gefunden, verwende System-Loader" >&2
    exec "${HERE}/usr/bin/pokerth_client" "$@"
fi
RUNEOF
chmod +x "$APPDIR/AppRun"

# --- AppImage erstellen ---

echo ""
echo "=== Erstelle AppImage ==="
cd "$SCRIPT_DIR"

# appimagetool in --appimage-extract-and-run mode for containers without FUSE
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

# Check whether glibc is bundled
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
