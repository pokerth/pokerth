#!/bin/bash
set -e

# Binary deploy script for pokerth_globalnotice (admin CLI, headless).
#
# Structured like create_binary_deploy.sh, but considerably smaller: the tool
# is a pure console application (QCoreApplication) and needs neither platform
# plugins nor QML modules. Only the Qt TLS plugin is mandatory —
# without libqopensslbackend.so Qt Network cannot do HTTPS and downloading
# the server list (default mode) fails.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
DEPLOY_DIR="${SCRIPT_DIR}/pokerth-globalnotice-linux-binary"
DEPLOY_NAME="pokerth-globalnotice-linux-$(uname -m)-$(date +%Y%m%d_%H%M%S)"
DEPLOY_PARENT_DIR="$(dirname "$DEPLOY_DIR")"

echo "=== pokerth_globalnotice Binary Deploy Erstellung ==="
echo "Deploy: $DEPLOY_DIR"
echo ""

echo "=== Bereinigung (vorab) ==="
if [ -d "$DEPLOY_DIR" ]; then
    rm -rf "$DEPLOY_DIR"
    echo "Entfernt: $DEPLOY_DIR"
else
    echo "Kein altes Deploy-Verzeichnis gefunden"
fi

LAST_ZIP=$(find "$DEPLOY_PARENT_DIR" -maxdepth 1 -type f -name "pokerth-globalnotice-linux-*.zip" -printf "%T@ %p\n" 2>/dev/null | sort -nr | head -1 | cut -d' ' -f2-)
if [ -n "$LAST_ZIP" ] && [ -f "$LAST_ZIP" ]; then
    rm -f "$LAST_ZIP"
    echo "Entfernt: $LAST_ZIP"
else
    echo "Keine alte ZIP-Datei gefunden"
fi
echo ""

if [ ! -f "$BUILD_DIR/bin/pokerth_globalnotice" ]; then
    echo "ERROR: pokerth_globalnotice Binary nicht gefunden in $BUILD_DIR/bin/"
    echo "       Bauen mit: cmake --build $BUILD_DIR --target pokerth_globalnotice"
    exit 1
fi

mkdir -p "$DEPLOY_DIR"/{bin,lib,plugins}

echo "=== Kopiere Binary ==="
cp -v "$BUILD_DIR/bin/pokerth_globalnotice" "$DEPLOY_DIR/bin/"

# System libs that are not shipped along (regex on the basename).
# Only the glibc core: the bundle brings no libc/ld-linux along, so the
# glibc of the build container is the minimum requirement for the host (see
# Dockerfile.globalnotice-ubuntu24). Graphics/audio exceptions as in the GUI deploy
# are unnecessary here, the tool loads neither GL nor PulseAudio.
SKIP_PATTERN='^(libc[.-]|libm[.-]|libdl[.-]|libpthread[.-]|librt[.-]|libresolv[.-]|libutil[.-]|libnsl[.-]|ld-linux|ld-[0-9])'

# ldd already resolves ALL transitive dependencies – no recursion needed.
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

# With a non-system Qt (aqtinstall) the Qt libs are NOT in a standard search
# path; without this export ldd reports them as "not found" and copy_deps
# would silently leave them out.
if [ -n "$QT6_ROOT" ] && [ -d "$QT6_ROOT/lib" ]; then
    export LD_LIBRARY_PATH="$QT6_ROOT/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi

echo ""
echo "=== Sammle Abhängigkeiten (Binary) ==="
find "$DEPLOY_DIR/bin" -maxdepth 1 -type f | copy_deps

echo ""
echo "=== Sammle Qt-Plugins ==="
if [ -n "$QT6_ROOT" ] && [ -d "$QT6_ROOT/plugins" ]; then
    QT6_PLUGINS="$QT6_ROOT/plugins"
else
    QT6_PLUGINS=$(find /usr/lib* -type d -name "plugins" -path "*/qt6/*" 2>/dev/null | head -1)
    [ -z "$QT6_PLUGINS" ] && QT6_PLUGINS="/usr/lib/x86_64-linux-gnu/qt6/plugins"
fi

if [ -d "$QT6_PLUGINS" ]; then
    echo "Qt6 Plugins: $QT6_PLUGINS"
    # tls: mandatory for the HTTPS download of the server list.
    # networkinformation: optional (reachability), but it does no harm.
    for cat in tls networkinformation; do
        if [ -d "$QT6_PLUGINS/$cat" ]; then
            mkdir -p "$DEPLOY_DIR/plugins/$cat"
            cp "$QT6_PLUGINS/$cat"/*.so "$DEPLOY_DIR/plugins/$cat/" 2>/dev/null && \
                chmod +x "$DEPLOY_DIR/plugins/$cat"/*.so 2>/dev/null && \
                echo "  $cat" || true
        fi
    done
    find "$DEPLOY_DIR/plugins" -name "*.so" | copy_deps
else
    echo "WARNUNG: Qt6 Plugins nicht gefunden in $QT6_PLUGINS"
fi

if ! ls "$DEPLOY_DIR/plugins/tls/"*.so >/dev/null 2>&1; then
    echo "FEHLER: Qt-TLS-Plugin nicht gefunden -> ohne HTTPS kein Serverlisten-Download. Abbruch."
    exit 1
fi

echo ""
echo "=== Kopiere Dokumentation ==="
cp -v "$SCRIPT_DIR/globalnotice/README.md" "$DEPLOY_DIR/"
[ -f "$PROJECT_ROOT/COPYING" ] && cp "$PROJECT_ROOT/COPYING" "$DEPLOY_DIR/"

echo ""
echo "=== Erstelle Konfiguration und Launcher ==="

# qt.conf: Qt finds plugins and libs relative to the binary
cat > "$DEPLOY_DIR/bin/qt.conf" << 'EOF'
[Paths]
Plugins = ../plugins
Libraries = ../lib
EOF

cat > "$DEPLOY_DIR/pokerth-globalnotice" << 'EOF'
#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export LD_LIBRARY_PATH="$SCRIPT_DIR/lib:$LD_LIBRARY_PATH"
export QT_PLUGIN_PATH="$SCRIPT_DIR/plugins"
exec "$SCRIPT_DIR/bin/pokerth_globalnotice" "$@"
EOF
chmod +x "$DEPLOY_DIR/pokerth-globalnotice"

echo ""
echo "=== Verifiziere Abhängigkeiten ==="
# copy_deps only collects what ldd resolves. If a lib is missing in the build
# container, ldd reports "not found" and it is SILENTLY skipped -> the deploy
# does not start on the target system. Cross-check against the deploy's own lib/ here.
UNRESOLVED=$( { find "$DEPLOY_DIR/bin" -maxdepth 1 -type f;
                find "$DEPLOY_DIR/plugins" "$DEPLOY_DIR/lib" -name "*.so*" 2>/dev/null; } \
    | while read -r f; do
        LD_LIBRARY_PATH="$DEPLOY_DIR/lib" ldd "$f" 2>/dev/null \
            | awk -v F="$f" '/not found/ {print F"\t"$1}'
      done | sort -u )

if [ -n "$UNRESOLVED" ]; then
    echo "Ungelöste Abhängigkeiten:"
    echo "$UNRESOLVED" | sed 's/^/  /'
    # The binary and the TLS plugin are both mandatory -> hard abort.
    if echo "$UNRESOLVED" | grep -qE "/bin/pokerth_globalnotice|/plugins/tls/"; then
        echo "FEHLER: Binary bzw. TLS-Plugin unvollständig -> Abbruch."
        echo "        Fehlende Libs im Build-Container installieren und neu bauen."
        exit 1
    fi
else
    echo "OK: keine ungelösten Abhängigkeiten."
fi

echo ""
echo "=== Smoke-Test ==="
# --help needs neither network nor credentials: it only checks whether the
# bundle starts at all (libs/plugins complete).
if "$DEPLOY_DIR/pokerth-globalnotice" --help > /dev/null; then
    echo "OK: ./pokerth-globalnotice --help läuft."
else
    echo "FEHLER: Das Bundle startet nicht."
    exit 1
fi

echo ""
echo "=== Erstelle Archiv ==="
cd "$DEPLOY_PARENT_DIR"
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
echo "Testen: cd $DEPLOY_DIR && ./pokerth-globalnotice --list-servers"
[ -f "${DEPLOY_NAME}.zip" ] && echo "Archiv: ${DEPLOY_PARENT_DIR}/${DEPLOY_NAME}.zip"
