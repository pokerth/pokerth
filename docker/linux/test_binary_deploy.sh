#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "=== Binary Deploy Test ==="
echo ""
echo "1. Erstelle Binary Deploy..."
"$SCRIPT_DIR/create_binary_deploy.sh"

echo ""
echo "2. Baue Test-Container (minimal, ohne Build-Tools)..."
docker build -t pokerth-test -f "$SCRIPT_DIR/Dockerfile.test" "$SCRIPT_DIR"

echo ""
echo "3. Starte Test-Container mit Binary..."
docker run --rm -it \
    -v "$PROJECT_ROOT/pokerth-linux-binary:/app/pokerth" \
    -e DISPLAY="${DISPLAY:-:0}" \
    -v /tmp/.X11-unix:/tmp/.X11-unix:ro \
    pokerth-test \
    bash -c "
        echo '=== Prüfe Binary ==='
        ls -lh /app/pokerth/
        echo ''
        echo '=== Prüfe Abhängigkeiten ==='
        cd /app/pokerth
        ldd ./bin/pokerth_client | grep 'not found' && echo 'FEHLER: Fehlende Abhängigkeiten!' || echo 'OK: Alle Abhängigkeiten gefunden'
        echo ''
        echo '=== Starte Anwendung im Test-Modus ==='
        ./pokerth --version 2>&1 || echo 'Binary konnte nicht gestartet werden (normal wenn kein X-Server vorhanden)'
        echo ''
        echo '=== Test abgeschlossen ==='
    "

echo ""
echo "=== Zusammenfassung ==="
echo "Wenn keine 'not found' Fehler auftraten, ist das Binary-Deploy erfolgreich!"
