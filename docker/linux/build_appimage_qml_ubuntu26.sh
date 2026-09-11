#!/bin/bash
set -e

# Builds the PokerTH QML client AppImage in an Ubuntu 26.04 Docker container.
#
# Why Ubuntu 26.04?
#   The AppImage bundles glibc + ld-linux. The bundled glibc must be at least
#   as new as the newest target system (Ubuntu 26.04 / glibc 2.43) so that
#   host libs (libglib etc.) find the symbols they need.
#   Ubuntu 26.04 (glibc 2.43) covers all target systems from Ubuntu 22.04 upwards.
#
# Requirement: Docker installed and running.
#
# Usage:
#   cd <project-root>
#   bash docker/linux/build_appimage_qml_ubuntu26.sh
#   # or with --no-cache to bypass the image cache:
#   bash docker/linux/build_appimage_qml_ubuntu26.sh --no-cache

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
IMAGE_NAME="pokerth-appimage-qml-ubuntu26:latest"
NO_CACHE="${1:-}"

echo "=== PokerTH QML AppImage – Docker-Build (Ubuntu 26.04) ==="
echo "Projekt-Root:  $PROJECT_ROOT"
echo "Docker-Image:  $IMAGE_NAME"
echo ""

# Make sure we are on the right branch
CURRENT_BRANCH=$(git -C "$PROJECT_ROOT" rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unbekannt")
echo "Branch: $CURRENT_BRANCH  (wird via COPY in den Container kopiert)"
echo ""

# --- Stop and remove running containers of the image ---
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
    -f "$SCRIPT_DIR/Dockerfile.appimage-qml-ubuntu26" \
    -t "$IMAGE_NAME" \
    "$PROJECT_ROOT"

# --- Extract the AppImage from the container ---
echo ""
echo "=== Extrahiere AppImage ==="
CONTAINER_ID=$(docker create "$IMAGE_NAME")
docker cp "${CONTAINER_ID}:/output/." "$SCRIPT_DIR/"
docker rm "${CONTAINER_ID}"

echo ""
echo "=== Fertig! ==="
APPIMAGE=$(ls "$SCRIPT_DIR"/PokerTH-QML-*.AppImage 2>/dev/null | tail -1)
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
