#!/bin/bash
set -e

# Baut die PokerTH-QML-APK (arm64-v8a) mit Qt 6.7.3 und minSdkVersion 26
# (Android 8.0) in einem Docker-Container.
#
# Zielgerät:  HUAWEI RNE-L21 (Mate 10 Lite), Android 8.0.0, Kirin 659 (arm64-v8a).
#
# Warum Qt 6.7 für den QML-Client?
#   Qt 6.8+ unterstützt kein Android < 9 mehr. Der QML-Client nutzte als einzige
#   6.8-Abhängigkeit QtQuick.VectorImage; das ist durch den Image-basierten
#   components/SvgIcon.qml ersetzt, sodass der Client auf Qt 6.7 baut.
#   Der QML-Client umgeht zudem die Qt-6.7-Android-Backend-Bugs des Widget-
#   Clients (doppelte Touch-Events, unsichtbare modale Dialoge), weil QML in
#   EINEM Fenster rendert und keine modalen QDialog-Fenster nutzt.
#
#   Es wird dasselbe Image wie der Widget-6.7-Build verwendet (Dockerfile.qt67),
#   nur mit eigenem Tag und TARGET=pokerth_qml-client.
#
# Aufruf:
#   cd <projekt-root>
#   bash docker/android/build_android_qml_arm64_qt67_docker.sh
#   # oder ohne Image-Cache:
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

# Branch-Prüfung
CURRENT_BRANCH=$(git -C "$PROJECT_ROOT" rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unbekannt")
if [ "$CURRENT_BRANCH" != "qt6-qml" ]; then
    echo "WARNUNG: Aktueller Branch ist '$CURRENT_BRANCH', erwartet 'qt6-qml'."
    echo "Bitte erst: git checkout qt6-qml"
    read -r -p "Trotzdem fortfahren? [j/N] " REPLY
    [[ "$REPLY" =~ ^[jJyY]$ ]] || exit 1
fi
echo "Branch: $CURRENT_BRANCH  (wird via Volume in den Container eingebunden)"
echo ""

# Laufende/gestoppte Container dieses Images bereinigen
RUNNING=$(docker ps -q --filter "ancestor=$IMAGE_NAME" 2>/dev/null)
if [ -n "$RUNNING" ]; then
    echo "=== Stoppe laufende Container ==="
    docker stop $RUNNING
    docker rm $RUNNING 2>/dev/null || true
fi
STOPPED=$(docker ps -aq --filter "ancestor=$IMAGE_NAME" 2>/dev/null)
[ -n "$STOPPED" ] && docker rm $STOPPED 2>/dev/null || true

# Docker-Image bauen (Qt 6.7.3). Gleicher Dockerfile-Inhalt wie der Widget-6.7-
# Build -> Docker-Layer-Cache greift, das Tag wird i.d.R. in Sekunden erstellt.
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
# ANDROID_API_LEVEL=34: compileSdk/targetSdk = 34 (androidx.core:1.13.1 verlangt
#   >= 34; das Image hebt AGP auf 8.2.2, dessen aapt2 android-34 lesen kann).
# ANDROID_NATIVE_API_LEVEL=26: native Libs gezielt für Android 8.0 (RNE-L21).
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
