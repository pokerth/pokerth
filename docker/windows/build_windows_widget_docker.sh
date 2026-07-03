#!/bin/bash
set -e

# Baut den PokerTH Widget-Client Windows-Installer (.exe) in einem
# Ubuntu-25.10-Docker-Container (MinGW-Crosskompilierung).
#
# Warum Docker?
#   Der Build benötigt Qt 6.x für Windows (win64_mingw), vcpkg-Abhängigkeiten
#   und NSIS – alles wird im Container bereitgestellt, ohne das Host-System
#   zu verändern. Das Docker-Image wird gecacht, d. h. nach dem ersten Build
#   (ca. 30–60 min für Qt + vcpkg) läuft jeder weitere Build in ~5–10 min.
#
# Voraussetzung: Docker installiert und laufend.
#
# Aufruf:
#   cd <projekt-root>
#   bash docker/windows/build_windows_widget_docker.sh
#   # Cache-buste:
#   bash docker/windows/build_windows_widget_docker.sh --no-cache

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
IMAGE_NAME="pokerth-windows-widget-builder:latest"
NO_CACHE="${1:-}"

echo "=== PokerTH Widget-Client Windows-Installer – Docker-Build ==="
echo "Projekt-Root:  $PROJECT_ROOT"
echo "Docker-Image:  $IMAGE_NAME"
echo ""

# Branch-Prüfung
CURRENT_BRANCH=$(git -C "$PROJECT_ROOT" rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unbekannt")
if [ "$CURRENT_BRANCH" != "qt6-qml" ]; then
    echo "WARNUNG: Aktueller Branch ist '$CURRENT_BRANCH', erwartet 'qt6-qml'."
    echo "Bitte erst: git checkout qt6-qml"
    read -r -p "Trotzdem fortfahren? [j/N] " REPLY
    [[ "$REPLY" =~ ^[jJyY]$ ]] || exit 1
fi
echo "Branch: $CURRENT_BRANCH  (wird via COPY in den Container kopiert)"
echo ""

# --- Laufende/gestoppte Container desselben Images bereinigen ----------------
RUNNING=$(docker ps -q --filter "ancestor=$IMAGE_NAME" 2>/dev/null)
[ -n "$RUNNING" ] && docker stop $RUNNING && docker rm $RUNNING 2>/dev/null || true
STOPPED=$(docker ps -aq --filter "ancestor=$IMAGE_NAME" 2>/dev/null)
[ -n "$STOPPED" ] && docker rm $STOPPED 2>/dev/null || true

# --- Docker-Image bauen -------------------------------------------------------
echo "=== Baue Docker-Image (beim ersten Aufruf ~30–60 min für Qt + vcpkg) ==="
docker build \
    ${NO_CACHE:+--no-cache} \
    -f "$SCRIPT_DIR/Dockerfile.windows-widget" \
    -t "$IMAGE_NAME" \
    "$PROJECT_ROOT"

# --- Installer aus dem Container extrahieren ----------------------------------
echo ""
echo "=== Extrahiere Windows-Installer ==="
CONTAINER_ID=$(docker create "$IMAGE_NAME")
docker cp "${CONTAINER_ID}:/output/." "$SCRIPT_DIR/"
docker rm "${CONTAINER_ID}"

echo ""
echo "=== Fertig! ==="
INSTALLER=$(ls "$SCRIPT_DIR"/PokerTH-Widget-*-Setup.exe 2>/dev/null | tail -1)
if [ -n "$INSTALLER" ]; then
    ls -lh "$INSTALLER"
    echo ""
    echo "Installation auf Windows:"
    echo "  $(basename "$INSTALLER")"
    echo ""
    echo "Testen mit Wine (optional):"
    echo "  wine $(basename "$INSTALLER")"
else
    echo "FEHLER: Kein Installer in $SCRIPT_DIR gefunden!"
    exit 1
fi
