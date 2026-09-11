#!/bin/bash
set -e

# Builds the PokerTH QML APK (x86_64) in a Docker container.
#
# The devcontainer Dockerfile is used as the build environment – Qt, NDK
# and vcpkg are already contained in the image. The local sources are bound
# in via a volume, so there is no need to enter the container.
#
# Usage:
#   cd <project-root>
#   bash docker/android/build_android_qml_x86_64_docker.sh
#   # or without the image cache:
#   bash docker/android/build_android_qml_x86_64_docker.sh --no-cache

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
DEVCONTAINER_DIR="$SCRIPT_DIR/.devcontainer"
IMAGE_NAME="pokerth-android-builder:x86_64"
ARCH="x86_64"
NO_CACHE="${1:-}"
BUILD_TIMESTAMP=$(date +%Y%m%d_%H%M%S)

echo "=== PokerTH Android APK – Docker-Build (x86_64) ==="
echo "Projekt-Root:  $PROJECT_ROOT"
echo "Docker-Image:  $IMAGE_NAME"
echo ""

# Branch-Hinweis
CURRENT_BRANCH=$(git -C "$PROJECT_ROOT" rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unbekannt")
echo "Branch: $CURRENT_BRANCH  (wird via Volume in den Container eingebunden)"
echo ""

# Clean up running/stopped containers of this image
RUNNING=$(docker ps -q --filter "ancestor=$IMAGE_NAME" 2>/dev/null)
if [ -n "$RUNNING" ]; then
    echo "=== Stoppe laufende Container ==="
    docker stop $RUNNING
    docker rm $RUNNING 2>/dev/null || true
fi
STOPPED=$(docker ps -aq --filter "ancestor=$IMAGE_NAME" 2>/dev/null)
[ -n "$STOPPED" ] && docker rm $STOPPED 2>/dev/null || true

# Docker-Image bauen
echo "=== Baue Docker-Image ==="
echo "    Erster Aufruf: Qt, NDK und vcpkg werden installiert – ca. 1 Stunde."
echo "    Folgeaufrufe starten dank Cache in Sekunden."
echo ""
docker build \
    ${NO_CACHE:+--no-cache} \
    --build-arg ANDROID_ARCH=x86_64 \
    --build-arg VCPKG_ARCH=x64 \
    --build-arg QT_ARCH=x86_64 \
    -f "$DEVCONTAINER_DIR/Dockerfile" \
    -t "$IMAGE_NAME" \
    "$DEVCONTAINER_DIR"

# PokerTH im Container bauen – lokale Quellen via Volume eingebunden
echo ""
echo "=== Starte PokerTH Android Build ==="
echo "    Lokale Quellen: $PROJECT_ROOT"
echo "    Container-Pfad: /opt/pokerth-android/pokerth"
echo ""
docker run --rm \
    -v "$PROJECT_ROOT:/opt/pokerth-android/pokerth" \
    -w /opt/pokerth-android/pokerth \
    "$IMAGE_NAME" \
    bash docker/android/build_android.sh --arch "$ARCH"

# APK in docker/android/ kopieren
echo ""
echo "=== Suche und kopiere APK ==="
APK_SEARCH_DIR="$PROJECT_ROOT/build-android-${ARCH}/android-build/build/outputs/apk"
APK_FILE=$(find "$APK_SEARCH_DIR" -type f -name "*.apk" ! -name "*unaligned*" 2>/dev/null | head -1)

if [ -z "$APK_FILE" ]; then
    APK_FILE=$(find "$APK_SEARCH_DIR" -type f -name "*.apk" 2>/dev/null | head -1)
fi

if [ -z "$APK_FILE" ]; then
    echo "FEHLER: Keine APK in $APK_SEARCH_DIR gefunden!"
    exit 1
fi

DEST_APK="$SCRIPT_DIR/pokerth-qml_${ARCH}_${BUILD_TIMESTAMP}.apk"
cp -v "$APK_FILE" "$DEST_APK"

echo ""
echo "=== Fertig! ==="
ls -lh "$DEST_APK"
echo ""
echo "Nächster Schritt – APK signieren (außerhalb Docker):"
echo "  cd docker/android/"
echo "  apksigner sign --ks my.keystore --ks-key-alias app $(basename "$DEST_APK")"
