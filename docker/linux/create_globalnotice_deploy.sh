#!/bin/bash
set -e

# Binary Deploy Script für pokerth_globalnotice (Admin-CLI, headless).
#
# Aufbau analog zu create_binary_deploy.sh, aber deutlich kleiner: das Werkzeug
# ist eine reine Konsolen-Anwendung (QCoreApplication) und braucht weder
# Platform-Plugins noch QML-Module. Zwingend ist nur das Qt-TLS-Plugin —
# ohne libqopensslbackend.so kann Qt Network kein HTTPS und der Download der
# Serverliste (Default-Modus) schlägt fehl.

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

# System-Libs die nicht mitgeliefert werden (regex auf basename).
# Nur der glibc-Kern: das Bundle bringt kein libc/ld-linux mit, deshalb ist die
# glibc des Build-Containers die Mindestanforderung an den Host (siehe
# Dockerfile.globalnotice-ubuntu24). Grafik-/Audio-Ausnahmen wie im GUI-Deploy
# sind hier unnötig, das Werkzeug lädt weder GL noch PulseAudio.
SKIP_PATTERN='^(libc[.-]|libm[.-]|libdl[.-]|libpthread[.-]|librt[.-]|libresolv[.-]|libutil[.-]|libnsl[.-]|ld-linux|ld-[0-9])'

# ldd löst bereits ALLE transitiven Abhängigkeiten auf – keine Rekursion nötig.
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
# Suchpfad; ohne diesen Export meldet ldd sie als "not found" und copy_deps
# ließe sie still weg.
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
    # tls: Pflicht für den HTTPS-Download der Serverliste.
    # networkinformation: optional (Reachability), schadet aber nicht.
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

# qt.conf: Qt findet Plugins und Libs relativ zum Binary
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
# copy_deps sammelt nur, was ldd auflöst. Fehlt eine Lib im Build-Container,
# meldet ldd "not found" und sie wird STILL übersprungen -> das Deploy startet
# auf dem Zielsystem nicht. Hier gegen das Deploy-eigene lib/ gegenprüfen.
UNRESOLVED=$( { find "$DEPLOY_DIR/bin" -maxdepth 1 -type f;
                find "$DEPLOY_DIR/plugins" "$DEPLOY_DIR/lib" -name "*.so*" 2>/dev/null; } \
    | while read -r f; do
        LD_LIBRARY_PATH="$DEPLOY_DIR/lib" ldd "$f" 2>/dev/null \
            | awk -v F="$f" '/not found/ {print F"\t"$1}'
      done | sort -u )

if [ -n "$UNRESOLVED" ]; then
    echo "Ungelöste Abhängigkeiten:"
    echo "$UNRESOLVED" | sed 's/^/  /'
    # Binary und TLS-Plugin sind beide zwingend -> harter Abbruch.
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
# --help braucht kein Netz und keine Credentials: prüft nur, ob das Bundle
# überhaupt startet (Libs/Plugins vollständig).
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
