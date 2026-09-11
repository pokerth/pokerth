#!/bin/bash
set -e

# Builds the PokerTH QML APK (arm64-v8a) with Qt 6.7.3 and minSdkVersion 26
# (Android 8.0) in a Docker container.
#
# Target device:  HUAWEI RNE-L21 (Mate 10 Lite), Android 8.0.0, Kirin 659 (arm64-v8a).
#
# Why Qt 6.7 for the QML client?
#   Qt 6.8+ no longer supports Android < 9. The QML client used QtQuick.VectorImage
#   as its only 6.8 dependency; that is replaced by the image based
#   components/SvgIcon.qml, so that the client builds on Qt 6.7.
#   The QML client also avoids the Qt 6.7 Android backend bugs of the widget
#   client (duplicate touch events, invisible modal dialogs), because QML renders
#   in ONE window and uses no modal QDialog windows.
#
#   The same image as the widget 6.7 build is used (Dockerfile.qt67),
#   only with its own tag and TARGET=pokerth_qml-client.
#
# Usage:
#   cd <project-root>
#   bash docker/android/build_android_qml_arm64_qt67_docker.sh
#   # or without the image cache:
#   bash docker/android/build_android_qml_arm64_qt67_docker.sh --no-cache

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
DEVCONTAINER_DIR="$SCRIPT_DIR/.devcontainer"
DOCKERFILE="$DEVCONTAINER_DIR/Dockerfile.qt67"
IMAGE_NAME="pokerth-android-builder:qt67-qml"
ARCH="arm64-v8a"
MIN_SDK="26"
NO_CACHE="${1:-}"
BUILD_TIMESTAMP=$(date +%Y%m%d_%H%M%S)

echo "=== PokerTH Android QML-APK – Docker-Build (Qt 6.7.3, arm64-v8a, minSdk $MIN_SDK / Android 8.0) ==="
echo "Zielgerät:     HUAWEI RNE-L21 (Android 8.0.0)"
echo "Build-Target:  pokerth_qml-client (QML-Client)"
echo "Projekt-Root:  $PROJECT_ROOT"
echo "Docker-Image:  $IMAGE_NAME"
echo "Dockerfile:    $DOCKERFILE"
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

# Build the Docker image (Qt 6.7.3). Same Dockerfile content as the widget 6.7
# build -> the Docker layer cache applies, the tag is usually created in seconds.
echo "=== Baue Docker-Image (Qt 6.7.3) ==="
echo "    Bei vorhandenem Cache (vom Widget-6.7-Build) nur Sekunden."
echo ""
docker build \
    ${NO_CACHE:+--no-cache} \
    -f "$DOCKERFILE" \
    -t "$IMAGE_NAME" \
    "$DEVCONTAINER_DIR"

# PokerTH QML-Client im Container bauen – lokale Quellen via Volume eingebunden
echo ""
echo "=== Starte PokerTH Android QML Build (Qt 6.7.3, minSdk $MIN_SDK) ==="
echo "    Lokale Quellen: $PROJECT_ROOT"
echo "    Container-Pfad: /opt/pokerth-android/pokerth"
echo "    Build-Target:   pokerth_qml-client"
echo ""
# ANDROID_API_LEVEL=34: compileSdk/targetSdk = 34 (androidx.core:1.13.1 requires
#   >= 34; the image raises AGP to 8.2.2, whose aapt2 can read android-34).
# ANDROID_NATIVE_API_LEVEL=26: native libs targeted at Android 8.0 (RNE-L21).
docker run --rm \
    -e TARGET=pokerth_qml-client \
    -e ANDROID_MIN_SDK="$MIN_SDK" \
    -e ANDROID_API_LEVEL=34 \
    -e ANDROID_NATIVE_API_LEVEL=26 \
    -v "$PROJECT_ROOT:/opt/pokerth-android/pokerth" \
    -w /opt/pokerth-android/pokerth \
    "$IMAGE_NAME" \
    bash docker/android/build_android.sh

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

DEST_APK="$SCRIPT_DIR/pokerth-qml_qt67_${ARCH}_api${MIN_SDK}_${BUILD_TIMESTAMP}.apk"
cp -v "$APK_FILE" "$DEST_APK"

echo ""
echo "=== Fertig! ==="
ls -lh "$DEST_APK"
echo ""
echo "Nächster Schritt – APK signieren (außerhalb Docker):"
echo "  cd docker/android/"
echo "  apksigner sign --ks my.keystore --ks-key-alias app $(basename "$DEST_APK")"
