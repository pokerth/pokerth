#!/bin/bash
set -e

# Baut das PokerTH ZIP-Binary-Deploy in einem Ubuntu-24.04-Docker-Container.
#
# Warum Ubuntu 24.04 (und nicht 26.04 wie das AppImage)?
#   Der ZIP-Tarball bündelt KEIN glibc. Damit ist das glibc des Build-Containers
#   die minimale Host-Anforderung. Auf dem ältesten unterstützten LTS gebaut
#   (24.04 = glibc 2.39) läuft der Tarball auf 24.04 und allem Neueren — inkl.
#   Linux Mint 22.x (Noble-Basis). Zusätzlich passen so die gebündelten Audio-
#   Codec-Libs (libFLAC.so.12 etc.) zum Host-PulseAudio-Client → Ton funktioniert.
#   (Details siehe Kommentar in Dockerfile.binary-ubuntu24.)
#
# Voraussetzung: Docker installiert und laufend.
#
# Aufruf:
#   cd <projekt-root>
#   bash docker/linux/build_binary_deploy_ubuntu24.sh
#   # optional andere Qt-Version:
#   QT_VERSION=6.8.3 bash docker/linux/build_binary_deploy_ubuntu24.sh
#   # optional Cache umgehen:
#   bash docker/linux/build_binary_deploy_ubuntu24.sh --no-cache

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
IMAGE_NAME="pokerth-binary-ubuntu24:latest"
QT_VERSION="${QT_VERSION:-6.8.1}"
NO_CACHE="${1:-}"

echo "=== PokerTH ZIP-Binary-Deploy – Docker-Build (Ubuntu 24.04, Qt ${QT_VERSION}) ==="
echo "Projekt-Root:  $PROJECT_ROOT"
echo "Docker-Image:  $IMAGE_NAME"
echo ""

# Branch-Hinweis (der Build kopiert den aktuellen Arbeitsstand via COPY)
CURRENT_BRANCH=$(git -C "$PROJECT_ROOT" rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unbekannt")
if [ "$CURRENT_BRANCH" != "qt6-qml" ]; then
    echo "WARNUNG: Aktueller Branch ist '$CURRENT_BRANCH', erwartet 'qt6-qml'."
    read -r -p "Trotzdem fortfahren? [j/N] " REPLY
    [[ "$REPLY" =~ ^[jJyY]$ ]] || exit 1
fi
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
    -f "$SCRIPT_DIR/Dockerfile.binary-ubuntu24" \
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
ZIP=$(ls -t "$SCRIPT_DIR"/pokerth-linux-*.zip 2>/dev/null | head -1)
if [ -n "$ZIP" ]; then
    ls -lh "$ZIP"
    echo ""
    echo "Test:"
    echo "  unzip $(basename "$ZIP") && cd pokerth-linux-binary"
    echo "  ./pokerth-qml"
    echo "  # Audio-Diagnose:"
    echo "  ./pokerth-qml --debug-audio"
else
    echo "FEHLER: Kein ZIP in $SCRIPT_DIR gefunden!"
    exit 1
fi
