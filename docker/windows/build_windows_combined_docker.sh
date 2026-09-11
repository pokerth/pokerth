#!/bin/bash
set -e

# Builds ONE PokerTH Windows installer containing BOTH clients
# (widget + QML) in an Ubuntu 25.10 Docker container (MinGW cross).
#
# Why Docker?
#   The build needs Qt 6.x for Windows (win64_mingw), vcpkg dependencies
#   and NSIS – all of it is provided in the container without changing the
#   host system. The Docker image is cached, i.e. after the first build
#   (about 30–60 min for Qt + vcpkg) every further build runs in ~5–10 min.
#
# Result:
#   PokerTH-Combined-<version>-<timestamp>-Setup.exe
#   installs pokerth_client.exe AND pokerth_qml-client.exe into a
#   shared directory (shared DLLs / data / plugins / qml).
#
# Requirement: Docker installed and running.
#
# Usage:
#   cd <project-root>
#   bash docker/windows/build_windows_combined_docker.sh
#   # bust the cache:
#   bash docker/windows/build_windows_combined_docker.sh --no-cache

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
IMAGE_NAME="pokerth-windows-combined-builder:latest"
NO_CACHE="${1:-}"

echo "=== PokerTH Combined-Installer (Widget + QML) – Docker-Build ==="
echo "Projekt-Root:  $PROJECT_ROOT"
echo "Docker-Image:  $IMAGE_NAME"
echo ""

# Branch-Hinweis
CURRENT_BRANCH=$(git -C "$PROJECT_ROOT" rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unbekannt")
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
    -f "$SCRIPT_DIR/Dockerfile.windows-combined" \
    -t "$IMAGE_NAME" \
    "$PROJECT_ROOT"

# --- Extract the installer from the container ---------------------------------
echo ""
echo "=== Extrahiere Windows-Installer ==="
CONTAINER_ID=$(docker create "$IMAGE_NAME")
docker cp "${CONTAINER_ID}:/output/." "$SCRIPT_DIR/"
docker rm "${CONTAINER_ID}"

echo ""
echo "=== Fertig! ==="
INSTALLER=$(ls "$SCRIPT_DIR"/PokerTH-Combined-*-Setup.exe 2>/dev/null | tail -1)
if [ -n "$INSTALLER" ]; then
    ls -lh "$INSTALLER"
    echo ""
    echo "Installation auf Windows (enthält beide Clients):"
    echo "  $(basename "$INSTALLER")"
    echo ""
    echo "Testen mit Wine (optional):"
    echo "  wine $(basename "$INSTALLER")"
else
    echo "FEHLER: Kein Installer in $SCRIPT_DIR gefunden!"
    exit 1
fi
