#!/bin/bash
set -e

# Baut das ZIP-Binary-Deploy von pokerth_globalnotice (Admin-CLI) in einem
# Ubuntu-24.04-Docker-Container.
#
# Warum Ubuntu 24.04? Wie beim GUI-Tarball: das ZIP bündelt kein glibc, also ist
# die glibc des Build-Containers der Host-Floor (siehe Kommentar in
# Dockerfile.globalnotice-ubuntu24). Die Toolchain-Layer sind wortgleich mit
# Dockerfile.binary-ubuntu24 und werden daher aus dem Docker-Layer-Cache
# wiederverwendet, wenn das GUI-Image schon einmal gebaut wurde.
#
# Voraussetzung: Docker installiert und laufend.
#
# Aufruf:
#   cd <projekt-root>
#   bash docker/linux/build_globalnotice_deploy_ubuntu24.sh
#   # optional andere Qt-Version:
#   QT_VERSION=6.8.3 bash docker/linux/build_globalnotice_deploy_ubuntu24.sh
#   # optional Cache umgehen:
#   bash docker/linux/build_globalnotice_deploy_ubuntu24.sh --no-cache

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
IMAGE_NAME="pokerth-globalnotice-ubuntu24:latest"
QT_VERSION="${QT_VERSION:-6.8.1}"
NO_CACHE="${1:-}"

echo "=== pokerth_globalnotice ZIP-Deploy – Docker-Build (Ubuntu 24.04, Qt ${QT_VERSION}) ==="
echo "Projekt-Root:  $PROJECT_ROOT"
echo "Docker-Image:  $IMAGE_NAME"
echo ""

# Branch-Hinweis (der Build kopiert den aktuellen Arbeitsstand via COPY)
CURRENT_BRANCH=$(git -C "$PROJECT_ROOT" rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unbekannt")
echo "Branch: $CURRENT_BRANCH  (wird via COPY in den Container kopiert)"
echo ""

# --- Laufende/gestoppte Container desselben Images bereinigen ---
RUNNING=$(docker ps -q --filter "ancestor=$IMAGE_NAME" 2>/dev/null)
if [ -n "$RUNNING" ]; then
    echo "=== Stoppe laufende Container ==="
    docker stop $RUNNING
    docker rm $RUNNING 2>/dev/null || true
fi
STOPPED=$(docker ps -aq --filter "ancestor=$IMAGE_NAME" 2>/dev/null)
[ -n "$STOPPED" ] && docker rm $STOPPED 2>/dev/null || true

# --- Docker-Image bauen ---
echo "=== Baue Docker-Image ==="
docker build \
    ${NO_CACHE:+--no-cache} \
    --build-arg QT_VERSION="$QT_VERSION" \
    -f "$SCRIPT_DIR/Dockerfile.globalnotice-ubuntu24" \
    -t "$IMAGE_NAME" \
    "$PROJECT_ROOT"

# --- ZIP aus dem Container extrahieren ---
echo ""
echo "=== Extrahiere ZIP ==="
CONTAINER_ID=$(docker create "$IMAGE_NAME")
docker cp "${CONTAINER_ID}:/output/." "$SCRIPT_DIR/"
docker rm "${CONTAINER_ID}"

echo ""
echo "=== Fertig! ==="
ZIP=$(ls -t "$SCRIPT_DIR"/pokerth-globalnotice-linux-*.zip 2>/dev/null | head -1)
if [ -n "$ZIP" ]; then
    ls -lh "$ZIP"
    echo ""
    echo "Test:"
    echo "  unzip $(basename "$ZIP") && cd pokerth-globalnotice-linux-binary"
    echo "  ./pokerth-globalnotice --list-servers"
    echo "  ./pokerth-globalnotice -u <admin> \"Durchsage\""
else
    echo "FEHLER: Kein ZIP in $SCRIPT_DIR gefunden!"
    exit 1
fi
