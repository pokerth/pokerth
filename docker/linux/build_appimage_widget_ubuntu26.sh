#!/bin/bash
set -e

# Baut das PokerTH-Qt-Widgets-Client AppImage in einem Ubuntu-26.04-Docker-
# Container (Widget-Pendant zu build_appimage_qml_ubuntu26.sh).
#
# Warum Ubuntu 26.04?
#   Das AppImage bündelt glibc + ld-linux. Das gebündelte glibc muss mindestens
#   so neu sein wie das neueste Zielsystem (Ubuntu 26.04 / glibc 2.43), damit
#   Host-Libs (libglib etc.) die benötigten Symbole finden.
#   Ubuntu 26.04 (glibc 2.43) deckt alle Zielsysteme ab Ubuntu 22.04 aufwärts.
#
# Voraussetzung: Docker installiert und laufend.
#
# Aufruf:
#   cd <projekt-root>
#   bash docker/linux/build_appimage_widget_ubuntu26.sh
#   # oder mit --no-cache um den Image-Cache zu umgehen:
#   bash docker/linux/build_appimage_widget_ubuntu26.sh --no-cache

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
IMAGE_NAME="pokerth-appimage-widget-ubuntu26:latest"
NO_CACHE="${1:-}"

echo "=== PokerTH Widget AppImage – Docker-Build (Ubuntu 26.04) ==="
echo "Projekt-Root:  $PROJECT_ROOT"
echo "Docker-Image:  $IMAGE_NAME"
echo ""

# Sicherstellen, dass wir auf dem richtigen Branch sind
CURRENT_BRANCH=$(git -C "$PROJECT_ROOT" rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unbekannt")
if [ "$CURRENT_BRANCH" != "qt6-qml" ]; then
    echo "WARNUNG: Aktueller Branch ist '$CURRENT_BRANCH', erwartet 'qt6-qml'."
    echo "Bitte erst: git checkout qt6-qml"
    read -r -p "Trotzdem fortfahren? [j/N] " REPLY
    [[ "$REPLY" =~ ^[jJyY]$ ]] || exit 1
fi
echo "Branch: $CURRENT_BRANCH  (wird via COPY in den Container kopiert)"
echo ""

# --- Laufende Container des Images stoppen und entfernen ---
RUNNING=$(docker ps -q --filter "ancestor=$IMAGE_NAME" 2>/dev/null)
if [ -n "$RUNNING" ]; then
    echo "=== Stoppe laufende Container ==="
    docker stop $RUNNING
    docker rm $RUNNING 2>/dev/null || true
fi
# Gestoppte Container desselben Images ebenfalls bereinigen
STOPPED=$(docker ps -aq --filter "ancestor=$IMAGE_NAME" 2>/dev/null)
[ -n "$STOPPED" ] && docker rm $STOPPED 2>/dev/null || true

# --- Docker-Image bauen ---
echo "=== Baue Docker-Image ==="
docker build \
    ${NO_CACHE:+--no-cache} \
    -f "$SCRIPT_DIR/Dockerfile.appimage-widget-ubuntu26" \
    -t "$IMAGE_NAME" \
    "$PROJECT_ROOT"

# --- AppImage aus dem Container extrahieren ---
echo ""
echo "=== Extrahiere AppImage ==="
CONTAINER_ID=$(docker create "$IMAGE_NAME")
docker cp "${CONTAINER_ID}:/output/." "$SCRIPT_DIR/"
docker rm "${CONTAINER_ID}"

echo ""
echo "=== Fertig! ==="
APPIMAGE=$(ls "$SCRIPT_DIR"/PokerTH-Widget-*.AppImage 2>/dev/null | tail -1)
if [ -n "$APPIMAGE" ]; then
    ls -lh "$APPIMAGE"
    echo ""
    echo "Test:"
    echo "  chmod +x $(basename "$APPIMAGE")"
    echo "  ./$(basename "$APPIMAGE")"
    echo ""
    echo "Oder ohne FUSE (Docker/WSL):"
    echo "  ./$(basename "$APPIMAGE") --appimage-extract-and-run"
else
    echo "FEHLER: Kein AppImage in $SCRIPT_DIR gefunden!"
    exit 1
fi
