#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "=== Binary Deploy Test ==="
echo ""

# Prüfe, ob Binary Deploy existiert
if [ ! -d "$SCRIPT_DIR/pokerth-linux-binary" ]; then
    echo "FEHLER: Binary Deploy nicht gefunden!"
    echo "Bitte erst im DevContainer ausführen: ./docker/linux/create_binary_deploy.sh"
    exit 1
fi

echo "1. Baue Test-Container (minimal, ohne Build-Tools)..."
docker build --no-cache -t pokerth-test -f "$SCRIPT_DIR/Dockerfile.test" "$SCRIPT_DIR"

echo ""
echo "2. Starte Test-Container mit Binary..."
docker run --rm -it \
    -v "$SCRIPT_DIR/pokerth-linux-binary:/app/pokerth" \
    -e DISPLAY="${DISPLAY:-:0}" \
    -v /tmp/.X11-unix:/tmp/.X11-unix:ro \
    pokerth-test \
    bash -c "
        echo '=== Prüfe Binary ==='
        ls -lh /app/pokerth/
        echo ''
        echo '=== Prüfe Bibliotheken im lib/ ==='
        echo \"Anzahl Bibliotheken: \$(ls -1 /app/pokerth/lib/*.so* 2>/dev/null | wc -l)\"
        echo ''
        echo '=== Prüfe Qt-Plugins ==='
        if [ -d /app/pokerth/plugins ]; then
            echo \"Qt-Plugins gefunden:\"
            find /app/pokerth/plugins -name '*.so' | wc -l
        else
            echo \"WARNUNG: Keine Qt-Plugins gefunden\"
        fi
        echo ''
        echo '=== Prüfe Abhängigkeiten (mit LD_LIBRARY_PATH) ==='
        cd /app/pokerth
        export LD_LIBRARY_PATH=/app/pokerth/lib:\$LD_LIBRARY_PATH
        export QT_PLUGIN_PATH=/app/pokerth/plugins:\$QT_PLUGIN_PATH
        if ldd ./bin/pokerth_client | grep 'not found'; then
            echo 'FEHLER: Fehlende Abhängigkeiten!'
            exit 1
        else
            echo 'OK: Alle Abhängigkeiten gefunden'
        fi
        echo ''
        echo '=== Starte Anwendung im Test-Modus ==='
        ./pokerth --version 2>&1 || echo 'Binary konnte nicht gestartet werden (normal wenn kein X-Server/Display vorhanden)'
        echo ''
        echo '=== Test abgeschlossen ==='
    "

echo ""
echo "=== Zusammenfassung ==="
echo "Wenn keine 'not found' Fehler auftraten, ist das Binary-Deploy erfolgreich!"
