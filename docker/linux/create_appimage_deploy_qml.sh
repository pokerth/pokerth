#!/bin/bash
set -e

# AppImage Deploy Script für den PokerTH QtQuick/QML-Client (pokerth_qml-client).
#
# Bündelt glibc + ld-linux (identischer Ansatz wie create_appimage_deploy.sh):
#   Qt Quick / QSGRenderLoop + bundled glibc ist in der Praxis stabil, solange
#   GPU-Libs (libGL/libEGL) NICHT gebündelt werden (die kommen weiterhin vom Host).
#   Ohne gebündeltes glibc crasht Qt6Core beim static initializer auf neueren
#   Host-Systemen (Ubuntu 26.04 / GCC 16), weil aqtinstall-Prebuilds und das
#   Host-Laufzeitsystem inkompatibel sind.
#
# Was gebündelt wird:   Qt-Libs, QML-Module, Qt-Plugins, glibc, ld-linux,
#                       libstdc++, libgcc_s, app-eigene Deps (boost, protobuf, ssl)
# Was NICHT gebündelt:  GPU/GL, Windowing (X11/XCB/Wayland), Fonts

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
ARCH="$(uname -m)"
APPDIR="${SCRIPT_DIR}/PokerTH-QML.AppDir"
APPIMAGE_NAME="${APPIMAGE_NAME:-PokerTH-QML-${ARCH}-$(date +%Y%m%d_%H%M%S).AppImage}"

echo "=== PokerTH QML-Client AppImage Erstellung ==="
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

if [ ! -f "$BUILD_DIR/bin/pokerth_qml-client" ]; then
    echo "ERROR: pokerth_qml-client Binary nicht gefunden!"
    echo "Bitte zuerst bauen: cmake --build build --target pokerth_qml-client"
    exit 1
fi

# appimagetool herunterladen falls nicht vorhanden
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
mkdir -p "$APPDIR"/usr/{bin,lib,share/pokerth,plugins,qml}

# --- Binaries kopieren ---

echo "=== Kopiere Binaries ==="
cp -v "$BUILD_DIR/bin/pokerth_qml-client" "$APPDIR/usr/bin/"

# Botfiles kopieren (für lokale Spiele gegen Computergegner)
if [ -d "$BUILD_DIR/bin/botfiles" ]; then
    cp -rv "$BUILD_DIR/bin/botfiles" "$APPDIR/usr/bin/"
fi

# --- Abhängigkeiten sammeln (inkl. glibc) ---

echo ""
echo "=== Sammle ALLE Abhängigkeiten (inkl. glibc) ==="

# Bibliotheken, die NICHT gebündelt werden dürfen:
#
# 1. GPU/Grafiktreiber: eng an Host-Kernel & GPU-Treiber gebunden.
#    Gebündelt scheitert die OpenGL/EGL-Initialisierung ("EGL not available").
#
# 2. Windowing (X11/Wayland/XCB/XKB) + GLib/DBus: tief ins Host-Desktop-
#    Environment integriert; gemischte Versionen führen zu Crashes oder
#    IPC-Fehlern (z. B. D-Bus-Socket-Protokoll-Mismatch).
#
# 3. Fontconfig/Freetype: nutzen die Host-Font-Datenbank; gebündelt werden
#    Systemschriften nicht gefunden.
#
# glibc + ld-linux werden MITGEBÜNDELT (siehe glibc-Bundle-Sektion unten).
is_excluded_lib() {
    case "$1" in
        # --- GPU / Grafiktreiber ---
        libEGL.so*|libEGL_*.so*)           return 0 ;;
        libGLdispatch.so*|libGLX.so*|libGLX_*.so*) return 0 ;;
        libGL.so*|libGLES*.so*|libOpenGL.so*|libGLU.so*) return 0 ;;
        libgallium*.so*|libglapi.so*|libgbm.so*) return 0 ;;
        libdrm.so*|libdrm_*.so*)           return 0 ;;
        libvulkan.so*|libvulkan_*.so*)     return 0 ;;
        libva.so*|libva-*.so*)             return 0 ;;
        libnvidia*.so*|libcuda*.so*|libnvcuvid*.so*) return 0 ;;

        # --- Windowing: Wayland ---
        libwayland-client.so*|libwayland-server.so*) return 0 ;;
        libwayland-cursor.so*|libwayland-egl.so*)    return 0 ;;

        # --- Windowing: X11 / XCB / XKB ---
        libX11.so*|libX11-xcb.so*|libXext.so*|libXrender.so*) return 0 ;;
        libXi.so*|libXss.so*|libXcursor.so*|libXrandr.so*)    return 0 ;;
        libXinerama.so*|libXfixes.so*|libXcomposite.so*)       return 0 ;;
        libXdamage.so*|libXtst.so*|libXau.so*|libXdmcp.so*)   return 0 ;;
        libxcb.so*|libxcb-util.so*|libxcb-icccm.so*)          return 0 ;;
        libxcb-image.so*|libxcb-keysyms.so*|libxcb-randr.so*) return 0 ;;
        libxcb-render*.so*|libxcb-shape.so*|libxcb-shm.so*)   return 0 ;;
        libxcb-sync.so*|libxcb-xfixes.so*|libxcb-xinerama.so*) return 0 ;;
        libxcb-xkb.so*|libxcb-dri*.so*)                       return 0 ;;
        # libxcb-cursor.so* wird BEWUSST NICHT ausgeschlossen (= mitgebündelt):
        # Ab Qt 6.5 ist sie Pflichtabhängigkeit des xcb-Plattform-Plugins
        # (libqxcb.so), aber auf vielen Zielsystemen nicht vorinstalliert.
        # Fehlt sie, scheitert der Start mit:
        #   "xcb-cursor0 or libxcb-cursor0 is needed to load the Qt xcb platform plugin"
        # Ihre eigenen Abhängigkeiten (libxcb, libxcb-render, …) bleiben Host-Libs.
        libxkbcommon.so*|libxkbcommon-x11.so*)                 return 0 ;;

        # --- GLib / GObject / GIO / DBus ---
        # Fuer Jammy-Kompatibilitaet werden diese bewusst mitgebuendelt,
        # da Qt6-Multimedia-Backends (z. B. libpxbackend) sonst an fehlenden
        # oder zu alten Host-Symbolen scheitern koennen.

        # --- Audio-Backends (PipeWire/pxbackend) ---
        # Ebenfalls mitbuendeln, damit keine Host-Abhaengigkeit auf
        # libpxbackend-1.0.so besteht.

        # --- Fonts / System-Infrastruktur ---
        libfontconfig.so*|libfreetype.so*) return 0 ;;
        libexpat.so*|libp11-kit.so*)       return 0 ;;
        libudev.so*|libsystemd.so*)        return 0 ;;

        *) return 1 ;;
    esac
}

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

            if is_excluded_lib "$libname"; then
                if ! grep -qxF "$lib" "$processed" 2>/dev/null; then
                    echo "$lib" >> "$processed"
                    echo "  - $libname (System/Host, nicht gebündelt)"
                fi
                continue
            fi

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

collect_all_dependencies "$APPDIR/usr/bin/pokerth_qml-client" "$APPDIR/usr/lib"

# Zusatzabsicherung fuer Qt6-Multimedia auf Jammy:
# Falls der initiale ldd-Rekursionslauf bestimmte Audio-Backend-Libs nicht
# erfasst (z. B. durch abweichende Paketlayouts), sammeln wir sie explizit.
for forced_audio_lib in libpxbackend-1.0.so libpipewire-0.3.so.0 libspa-0.2.so; do
    forced_path=$(ldconfig -p 2>/dev/null | grep "${forced_audio_lib}" | head -1 | awk '{print $NF}')
    if [ -n "$forced_path" ] && [ -f "$forced_path" ]; then
        echo "Erzwinge Bundle: ${forced_audio_lib}"
        collect_all_dependencies "$forced_path" "$APPDIR/usr/lib"
    fi
done

# Qt-6.5+-Pflicht: libxcb-cursor.so.0 explizit mitbündeln.
# Das xcb-Plattform-Plugin (libqxcb.so) linkt diese Lib direkt, sie ist aber
# auf vielen Zielsystemen nicht vorinstalliert. Ohne sie startet das AppImage
# nicht ("xcb-cursor0 ... is needed to load the Qt xcb platform plugin").
# Direkt kopieren (nicht über collect_all_dependencies, das nur die Deps der
# übergebenen Lib bündelt, nicht die Lib selbst).
xcb_cursor_path=$(ldconfig -p 2>/dev/null | grep "libxcb-cursor.so.0" | head -1 | awk '{print $NF}')
[ -z "$xcb_cursor_path" ] && for cand in \
    "/usr/lib/${ARCH}-linux-gnu/libxcb-cursor.so.0" \
    "/lib/${ARCH}-linux-gnu/libxcb-cursor.so.0"; do
    [ -f "$cand" ] && { xcb_cursor_path="$cand"; break; }
done
if [ -n "$xcb_cursor_path" ] && [ -f "$xcb_cursor_path" ]; then
    if [ ! -f "$APPDIR/usr/lib/libxcb-cursor.so.0" ]; then
        cp -L "$xcb_cursor_path" "$APPDIR/usr/lib/" \
            && chmod +x "$APPDIR/usr/lib/libxcb-cursor.so.0" \
            && echo "Erzwinge Bundle: libxcb-cursor.so.0 (Qt-6.5-xcb-Plugin)"
    fi
else
    echo "WARNUNG: libxcb-cursor.so.0 nicht gefunden — xcb-Plugin startet evtl. nicht!"
    echo "         apt install libxcb-cursor0 im Build-Container nötig."
fi

# --- Sicherstellung glibc-Bundle ---

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
# QT_DIR kann als Umgebungsvariable übergeben werden (z. B. bei aqtinstall-Qt).
if [ -n "${QT_DIR:-}" ]; then
    QT6_PLUGINS="${QT_DIR}/plugins"
    echo "QT_DIR gesetzt: ${QT_DIR}"
else
    QT6_PLUGINS=$(find /usr/lib* /opt/qt6 -type d -name "plugins" 2>/dev/null \
        | grep -E "gcc_64|qt6/plugins" | head -1)
    [ -z "$QT6_PLUGINS" ] && QT6_PLUGINS="/usr/lib/${ARCH}-linux-gnu/qt6/plugins"
fi

if [ -d "$QT6_PLUGINS" ]; then
    echo "Qt6 Plugins: $QT6_PLUGINS"
    for cat in platforms xcbglintegrations platforminputcontexts wayland-shell-integration wayland-decoration-client wayland-graphics-integration-client imageformats iconengines platformthemes multimedia sqldrivers tls; do
        if [ -d "$QT6_PLUGINS/$cat" ]; then
            echo "  Kopiere $cat..."
            mkdir -p "$APPDIR/usr/plugins/$cat"
            cp "$QT6_PLUGINS/$cat"/*.so "$APPDIR/usr/plugins/$cat/" 2>/dev/null || true
            chmod +x "$APPDIR/usr/plugins/$cat"/*.so 2>/dev/null || true
            for plugin in "$APPDIR/usr/plugins/$cat"/*.so; do
                [ -f "$plugin" ] && collect_all_dependencies "$plugin" "$APPDIR/usr/lib"
            done
        fi
    done
    rm -f "$APPDIR/usr/lib/.processed_libs"
else
    echo "WARNUNG: Qt6 Plugins nicht gefunden!"
fi

# --- Qt-QML-Module (für den QtQuick/QML-Client zwingend nötig) ---

echo ""
echo "=== Sammle Qt-QML-Module ==="
if [ -n "${QT_DIR:-}" ]; then
    QT6_QML="${QT_DIR}/qml"
else
    QT6_QML=$(find /usr/lib* /opt/qt6 -type d -name "qml" 2>/dev/null \
        | grep -E "gcc_64|qt6/qml" | head -1)
    [ -z "$QT6_QML" ] && QT6_QML="/usr/lib/${ARCH}-linux-gnu/qt6/qml"
fi

if [ -d "$QT6_QML" ]; then
    echo "Qt6 QML: $QT6_QML"
    for mod in QtCore QtQml QtQuick; do
        if [ -d "$QT6_QML/$mod" ]; then
            echo "  Kopiere Modul $mod ..."
            cp -r "$QT6_QML/$mod" "$APPDIR/usr/qml/"
        else
            echo "  WARNUNG: QML-Modul $mod nicht gefunden!"
        fi
    done
    while IFS= read -r qmlplugin; do
        [ -f "$qmlplugin" ] && collect_all_dependencies "$qmlplugin" "$APPDIR/usr/lib"
    done < <(find "$APPDIR/usr/qml" -name '*.so' 2>/dev/null)
    rm -f "$APPDIR/usr/lib/.processed_libs"
    echo "  QML-Module: $(find "$APPDIR/usr/qml" -maxdepth 1 -mindepth 1 -type d | wc -l)"
else
    echo "WARNUNG: Qt6-QML-Module nicht gefunden! Der QML-Client wird NICHT starten."
fi

# --- Data-Verzeichnis ---

echo ""
echo "=== Kopiere Data ==="
# Mit gebündeltem ld-linux zeigt /proc/self/exe auf usr/lib/ld-linux-*.so.2,
# daher: applicationDirPath() = usr/lib/
# getDataPathStdString() findet "bin/?$" nicht → Fallback sucht in usr/lib/data/
# Lösung: Data in usr/share/pokerth/data/ ablegen UND Symlink usr/lib/data anlegen.
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
# In usr/bin/ für normalen Start
cat > "$APPDIR/usr/bin/qt.conf" << 'EOF'
[Paths]
Plugins = ../plugins
QmlImports = ../qml
Libraries = ../lib
EOF

# In usr/lib/ für gebündelten ld-linux (applicationDirPath = usr/lib/)
cat > "$APPDIR/usr/lib/qt.conf" << 'EOF'
[Paths]
Plugins = ../plugins
QmlImports = ../qml
Libraries = .
EOF

# --- Desktop-Datei + Icon (AppImage-Pflicht) ---

echo "=== Erstelle Desktop-Datei und Icon ==="
# Dateiname = pokerth_qml (Unterstrich): muss zur app_id passen, die der Client
# via QGuiApplication::setDesktopFileName("pokerth_qml") setzt. So ordnet ein
# Compositor ohne xdg-toplevel-icon (Fallback-Pfad) Fenster und Icon korrekt zu.
cat > "$APPDIR/pokerth_qml.desktop" << 'EOF'
[Desktop Entry]
Name=PokerTH QML
GenericName=Poker Card Game
GenericName[de]=Pokerspiel
Comment=Texas hold'em game (QML client)
Comment[de]=Texas Hold'em Spiel (QML-Client)
Exec=pokerth_qml-client
Icon=pokerth
Terminal=false
Type=Application
StartupWMClass=pokerth_qml-client
Categories=Qt;Game;CardGame;
EOF

# Icon kopieren
if [ -f "$PROJECT_ROOT/pokerth.png" ]; then
    cp "$PROJECT_ROOT/pokerth.png" "$APPDIR/pokerth.png"
    mkdir -p "$APPDIR/usr/share/icons/hicolor/128x128/apps"
    cp "$PROJECT_ROOT/pokerth.png" "$APPDIR/usr/share/icons/hicolor/128x128/apps/pokerth.png"
else
    echo "WARNUNG: pokerth.png nicht gefunden, erstelle Platzhalter"
    printf '\x89PNG\r\n\x1a\n' > "$APPDIR/pokerth.png"
fi

# .DirIcon bestimmt das Icon der AppImage-DATEI im Dateimanager. appimagetool
# würde sonst nur einen Symlink .DirIcon -> pokerth.png anlegen, den manche
# Thumbnailer nicht auflösen. Als echte Datei (kein Symlink) ist es überall
# zuverlässig sichtbar.
cp "$APPDIR/pokerth.png" "$APPDIR/.DirIcon"

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
# AppRun: Startet den PokerTH QML-Client mit gebündeltem glibc + ld-linux Loader.
# Dadurch ist die glibc-Version des Host-Systems irrelevant.

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# --- AppImageLauncher-Erkennung ---
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

# Gebündelte Libs zuerst, System-Libs als Fallback (für GPU-Treiber etc.)
export LD_LIBRARY_PATH="${HERE}/usr/lib:${LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="${HERE}/usr/plugins"
export QT_QPA_PLATFORM_PLUGIN_PATH="${HERE}/usr/plugins/platforms"
export QML_IMPORT_PATH="${HERE}/usr/qml"
export QML2_IMPORT_PATH="${HERE}/usr/qml"
export QT_MEDIA_BACKEND=ffmpeg
export XDG_DATA_DIRS="${HERE}/usr/share:${XDG_DATA_DIRS:-/usr/local/share:/usr/share}"

# Host-Library-Pfade explizit sammeln. Diese werden dem gebündelten Loader
# zusaetzlich uebergeben, damit absichtlich nicht gebuendelte GPU/GL/EGL-Libs
# (z. B. libEGL.so.1, libGLX, Mesa/NVIDIA-Treiber) auf dem Zielsystem gefunden
# werden. Die Reihenfolge bleibt: zuerst AppImage-Libs, dann Host-Libs.
HOST_LIB_DIRS=""
# Kein externes `uname`: mit gebuendelter libc kann ein Host-Binary hier
# symbol lookup errors erzeugen. Deshalb nur Bash-interne Architekturableitung.
APP_ARCH="${MACHTYPE%%-*}"
[ -z "$APP_ARCH" ] && APP_ARCH="${HOSTTYPE:-x86_64}"
for d in /lib /usr/lib /lib64 /usr/lib64 /lib/${APP_ARCH}-linux-gnu /usr/lib/${APP_ARCH}-linux-gnu /lib/x86_64-linux-gnu /usr/lib/x86_64-linux-gnu; do
    if [ -d "$d" ]; then
        if [ -z "$HOST_LIB_DIRS" ]; then
            HOST_LIB_DIRS="$d"
        else
            HOST_LIB_DIRS="${HOST_LIB_DIRS}:$d"
        fi
    fi
done

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
    exec "${BUNDLED_LD}" --inhibit-cache --library-path "${HERE}/usr/lib${HOST_LIB_DIRS:+:${HOST_LIB_DIRS}}" \
         "${HERE}/usr/bin/pokerth_qml-client" "$@"
else
    # Fallback: Normaler Start (funktioniert nur wenn Host-glibc kompatibel ist)
    echo "WARNUNG: Gebündelter Loader nicht gefunden, verwende System-Loader" >&2
    exec "${HERE}/usr/bin/pokerth_qml-client" "$@"
fi
RUNEOF
chmod +x "$APPDIR/AppRun"

# --- AppImage erstellen ---

echo ""
echo "=== Erstelle AppImage ==="
cd "$SCRIPT_DIR"

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
echo "QML-Module:           $(find "$APPDIR/usr/qml" -maxdepth 1 -mindepth 1 -type d 2>/dev/null | wc -l)"
echo "Gesamtgröße AppDir:   $(du -sh "$APPDIR" | cut -f1)"
echo ""

if [ -f "$APPDIR/usr/lib/libc.so.6" ] && [ -f "$APPDIR/usr/lib/${LD_LINUX_NAME}" ]; then
    echo "✓ glibc + ld-linux gebündelt — sollte auf älteren Systemen funktionieren!"
else
    echo "⚠ glibc oder ld-linux fehlt — AppImage ist möglicherweise nicht voll portabel."
fi

if [ -d "$APPDIR/usr/qml/QtQuick" ]; then
    echo "✓ Qt-QML-Module gebündelt (QtQuick vorhanden)."
else
    echo "⚠ Qt-QML-Module fehlen — der QML-Client wird nicht starten!"
fi

if [ -d "$APPDIR/usr/share/pokerth/data" ]; then
    echo "✓ Data-Verzeichnis vorhanden (usr/share/pokerth/data)."
else
    echo "⚠ Data-Verzeichnis fehlt!"
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
echo "Problem: 'module \"QtQuick\" is not installed' o.ä.:"
echo "  → qt6-declarative (QML-Module) muss auf dem Build-System installiert sein."
echo ""
echo "Problem: 'fuse: memory allocation failed' / FUSE-Fehler:"
echo "  1. AppImageLauncher deinstallieren: sudo apt remove appimagelauncher"
echo "  2. libfuse2 installieren: sudo apt install libfuse2"
echo "  3. Alternativ: APPIMAGE_EXTRACT_AND_RUN=1 ./${APPIMAGE_NAME}"
echo ""
echo "Problem: 'EGL not available' / OpenGL-Fehler:"
echo "  → Mesa/GPU-Treiber auf dem Zielsystem prüfen:"
echo "     sudo apt install libgl1-mesa-dri libegl-mesa0"
