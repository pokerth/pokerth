#!/bin/bash
set -e

# Builds the PokerTH ZIP binary deploy in an Ubuntu 24.04 Docker container.
#
# Why Ubuntu 24.04 (and not 26.04 like the AppImage)?
#   The ZIP tarball bundles NO glibc. That makes the glibc of the build container
#   the minimum host requirement. Built on the oldest supported LTS
#   (24.04 = glibc 2.39), the tarball runs on 24.04 and everything newer — incl.
#   Linux Mint 22.x (Noble base). In addition the bundled audio codec libs
#   (libFLAC.so.12 etc.) match the host PulseAudio client this way → sound works.
#   (For details see the comment in Dockerfile.binary-ubuntu24.)
#
# Requirement: Docker installed and running.
#
# Usage:
#   cd <project-root>
#   bash docker/linux/build_binary_deploy_ubuntu24.sh
#   # optionally a different Qt version:
#   QT_VERSION=6.9.2 bash docker/linux/build_binary_deploy_ubuntu24.sh
#   # optionally bypass the cache:
#   bash docker/linux/build_binary_deploy_ubuntu24.sh --no-cache

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
IMAGE_NAME="pokerth-binary-ubuntu24:latest"
QT_VERSION="${QT_VERSION:-6.9.3}"
NO_CACHE="${1:-}"

echo "=== PokerTH ZIP-Binary-Deploy – Docker-Build (Ubuntu 24.04, Qt ${QT_VERSION}) ==="
echo "Projekt-Root:  $PROJECT_ROOT"
echo "Docker-Image:  $IMAGE_NAME"
echo ""

# Branch note (the build copies the current working state via COPY)
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
    -f "$SCRIPT_DIR/Dockerfile.binary-ubuntu24" \
    -t "$IMAGE_NAME" \
    "$PROJECT_ROOT"

# --- Extract the ZIP from the container ---
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
